#include "rando/Generator.h"

#include "rando/Fill.h"
#include "rando/MetaData.h"

#include <algorithm>
#include <cstdio>
#include <random>
#include <unordered_map>
#include <vector>

namespace Generator {
namespace {

// splitmix64 — cheap, well-distributed avalanche mix. Used to derive the seed id and
// the ice-trap salt from the numeric seed so both are deterministic functions of it.
uint64_t Mix(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// 64 short, distinct, easy-to-say words -> 3 picks index cleanly with 6 bits each.
const char* const kWords[64] = {
    "amber", "anvil", "arrow", "azure", "basil", "birch", "blaze", "brook",
    "cedar", "clay",  "cliff", "cobra", "comet", "coral", "crane", "delta",
    "drift", "ember", "fable", "fern",  "flint", "frost", "glade", "grove",
    "hazel", "heron", "ivory", "jade",  "kelp",  "lark",  "lily",  "lotus",
    "lunar", "maple", "marsh", "mint",  "moss",  "nova",  "oak",   "onyx",
    "otter", "petal", "pine",  "quartz","raven", "reed",  "river", "rose",
    "sage",  "shale", "slate", "snow",  "spark", "storm", "tide",  "thorn",
    "topaz", "umber", "vale",  "willow","wren",  "yarn",  "zephyr","zinc",
};

bool IsJunkItem(const std::unordered_map<int, bool>& advOf, RandomizerGet item) {
    auto it = advOf.find((int)item);
    return it != advOf.end() && !it->second;  // known + not advancement
}

} // namespace

std::string MakeSeedId(uint64_t seed) {
    uint64_t h = Mix(seed ^ 0xA5A5A5A5DEADBEEFULL);
    const char* w0 = kWords[h & 63];
    h = Mix(h);
    const char* w1 = kWords[h & 63];
    h = Mix(h);
    const char* w2 = kWords[h & 63];
    unsigned suffix = (unsigned)(Mix(seed) & 0xFFFFu);  // 4 hex digits, collision insurance
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s-%s-%s-%04x", w0, w1, w2, suffix);
    return std::string(buf);
}

Output Generate(const Options& opts, const ProgressFn& progress) {
    Output out;
    const int numWorlds = (int)opts.players.size();

    // 1. Run the restored assumed-fill at the engine's baked defaults (no overrides):
    //    a jointly-beatable 2-world placement where any world's check can hold any
    //    world's item. VerifyReachable inside Fill confirms every progression item is
    //    reachable across both worlds before it returns beatable. Fill drives `progress`
    //    across [~0.01, ~0.98]; we add the finishing stages below.
    Fill::Result r = Fill::Generate(opts.seed, numWorlds, {}, progress);

    out.beatable = r.beatable;
    out.attempts = r.attempts;
    out.crossWorld = r.crossWorld;

    // 2. Assemble the contract. ownerWorld == the world that owns the item (Fill's
    //    itemWorld); the server routes it there, the client labels it by that player.
    SeedFile::SeedData& sd = out.seed;
    sd.version = SeedFile::kVersion;
    sd.seed = opts.seed;
    sd.seedId = MakeSeedId(opts.seed);
    sd.numWorlds = numWorlds;
    sd.players = opts.players;
    sd.settings = opts.settings;  // curated settings pass straight through to the output

    sd.placements.reserve(r.placements.size());
    for (const auto& p : r.placements)
        sd.placements.push_back({ p.loc, p.locWorld, p.item, p.itemWorld });

    // 3. Season leftover junk with a fixed number of ice traps. We only ever rewrite
    //    JUNK placements (never progression, never an existing ice trap), so beatability
    //    is preserved. Selection is deterministic: collect junk slots in placement
    //    order, shuffle with a seed-derived RNG, convert the first N.
    if (progress) progress(0.99f, "Seasoning junk with ice traps...");
    std::unordered_map<int, bool> advOf;
    for (int i = 0; i < kItemCount; ++i) advOf[(int)kItems[i].rg] = kItems[i].advancement;

    std::vector<size_t> junkSlots;
    for (size_t i = 0; i < sd.placements.size(); ++i)
        if (sd.placements[i].item != RG_ICE_TRAP && IsJunkItem(advOf, sd.placements[i].item))
            junkSlots.push_back(i);

    std::mt19937_64 trapRng(Mix(opts.seed ^ 0x1CE7A91F00DULL));
    std::shuffle(junkSlots.begin(), junkSlots.end(), trapRng);
    const int want = std::max(0, opts.iceTraps);
    const int n = std::min((int)junkSlots.size(), want);
    for (int i = 0; i < n; ++i) sd.placements[junkSlots[i]].item = RG_ICE_TRAP;
    out.iceTraps = n;

    // 4. Link's Pocket starting dungeon reward (F-041). The curated config fixes Link's Pocket to
    //    a dungeon reward with "Any Reward" and Dungeon Rewards to End-of-Dungeons, so every world
    //    starts with exactly ONE medallion or spiritual stone. The restored engine at baked
    //    defaults does NOT emit a Link's Pocket placement (verified against the spoiler), so add it
    //    here, deterministically per (seed, world). It is a normal SAME-WORLD placement at
    //    RC_LINKS_POCKET (ownerWorld == locWorld) — so NO v3 schema change is needed — that the
    //    client grants once at save init (not via the cross-world collect flow). Appended AFTER the
    //    ice-trap step so the junk-slot selection above is byte-for-byte unchanged (rewards are
    //    advancement, never junk, so they could never be ice-trapped anyway). End-of-Dungeons is
    //    assumed for now; the conditional on the Dungeon Rewards setting arrives in the per-setting
    //    phase.
    {
        // The nine dungeon rewards are contiguous in RandomizerGet: the three spiritual stones
        // (Kokiri's Emerald .. Zora's Sapphire) then the six medallions (.. Light Medallion).
        const int rewardBase = (int)RG_KOKIRI_EMERALD;
        const int rewardCount = (int)RG_LIGHT_MEDALLION - rewardBase + 1;  // 9
        for (int w = 0; w < numWorlds; ++w) {
            // Distinct salt per world, mixed from the seed -> deterministic + stable across runs.
            uint64_t h = Mix(opts.seed ^ (0x10CCE7B19E3779F9ULL + (uint64_t)w * 0x9E3779B97F4A7C15ULL));
            RandomizerGet reward = (RandomizerGet)(rewardBase + (int)(h % (uint64_t)rewardCount));
            // { loc, locWorld, item, ownerWorld } — owned + granted locally by world w's player.
            sd.placements.push_back({ RC_LINKS_POCKET, w, reward, w });
        }
    }

    return out;
}

} // namespace Generator
