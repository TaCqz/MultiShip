// F-041 verification: Generator must emit exactly one Link's Pocket dungeon-reward placement per
// world (same-world, owned locally), deterministically per seed. Standalone (engine is dep-free).
#include "rando/Generator.h"
#include "rando/randomizerEnums.h"

#include <cstdio>
#include <map>

static int Fail(const char* msg) {
    std::printf("FAIL: %s\n", msg);
    return 1;
}

int main() {
    const int rewardBase = (int)RG_KOKIRI_EMERALD;
    const int rewardEnd = (int)RG_LIGHT_MEDALLION;

    Generator::Options opts;
    opts.seed = 12648430ULL; // same numeric seed the foundation spoiler used
    opts.players = { "Mori", "TaCqz" };

    Generator::Output out = Generator::Generate(opts, nullptr);

    // Collect Link's Pocket placements per world.
    std::map<int, int> lpRewardByWorld; // locWorld -> item
    int lpCount = 0;
    for (const auto& p : out.seed.placements) {
        if (p.loc == RC_LINKS_POCKET) {
            ++lpCount;
            lpRewardByWorld[p.locWorld] = (int)p.item;
            if (p.ownerWorld != p.locWorld) return Fail("Link's Pocket must be a same-world placement");
            if ((int)p.item < rewardBase || (int)p.item > rewardEnd)
                return Fail("Link's Pocket item is not a dungeon reward");
        }
    }

    if (lpCount != (int)opts.players.size()) return Fail("expected exactly one Link's Pocket placement per world");
    for (int w = 0; w < (int)opts.players.size(); ++w)
        if (lpRewardByWorld.find(w) == lpRewardByWorld.end()) return Fail("a world is missing its Link's Pocket reward");

    // Determinism: a second generation with the same inputs must yield the same rewards.
    Generator::Output out2 = Generator::Generate(opts, nullptr);
    for (const auto& p : out2.seed.placements)
        if (p.loc == RC_LINKS_POCKET && lpRewardByWorld[p.locWorld] != (int)p.item)
            return Fail("Link's Pocket reward not deterministic across runs");

    std::printf("PASS: %d Link's Pocket placements; world0 reward=RG#%d world1 reward=RG#%d (rewards in [%d..%d])\n",
                lpCount, lpRewardByWorld[0], lpRewardByWorld[1], rewardBase, rewardEnd);
    std::printf("      total placements=%zu beatable=%d\n", out.seed.placements.size(), (int)out.beatable);
    return 0;
}
