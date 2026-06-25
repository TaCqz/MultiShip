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

    return out;
}

} // namespace Generator
