// F-045 follow-up regression: Shuffle Dungeon Rewards = "End of Dungeons" must stay JOINTLY
// BEATABLE when the Door of Time is CLOSED (the curated default).
//
// With a closed Door of Time the three spiritual stones gate adult access — temple_of_time.cpp
// only opens RR_TOT_BEYOND_DOOR_OF_TIME when StoneCount()==3 (+ Ocarina + Song of Time). So every
// spiritual stone must be obtainable as CHILD: placed at one of the three CHILD-dungeon boss
// rewards (Queen Gohma / King Dodongo / Barinade), never behind an adult dungeon. Dungeon rewards
// are own-world (Fill's `w != owner` guard), so this is an EXACT per-world constraint: three stones
// into exactly three child reward locs, with the six medallions taking the six adult reward locs
// (the five adult bosses + Rauru, the latter re-homed to Link's Pocket).
//
// This guards the FILL ORDER. All nine rewards share the ZR_REWARD_LOC zone; if a medallion were
// placed into a child reward loc before the stones, a stone would strand behind an adult dungeon
// and — for an unlucky seed — the 40-attempt retry could ship a LOCKED seed. We assert across many
// seeds that (a) every seed is jointly beatable and (b) no spiritual stone ever lands at an adult
// boss reward.
//
// Standalone: the engine is dependency-free, so this links only multiship_rando.
#include "rando/Generator.h"
#include "rando/SeedFile.h"
#include "rando/randomizerEnums.h"

#include <cstdio>
#include <vector>

namespace {

int failures = 0;
void check(bool cond, const char* what) {
    std::printf("  [%s] %s\n", cond ? "OK" : "FAIL", what);
    if (!cond) ++failures;
}

bool IsStone(int rg) {
    return rg == (int)RG_KOKIRI_EMERALD || rg == (int)RG_GORON_RUBY || rg == (int)RG_ZORA_SAPPHIRE;
}
// The three child-dungeon boss rewards — reachable as child, so a stone here keeps a closed
// Door of Time openable.
bool IsChildBossRewardLoc(int rc) {
    return rc == (int)RC_QUEEN_GOHMA || rc == (int)RC_KING_DODONGO || rc == (int)RC_BARINADE;
}
// The five adult-dungeon boss rewards — a spiritual stone here is unreachable as child, so with a
// closed Door of Time it strands (no re-home can save it; only Rauru is re-homed to Link's Pocket).
bool IsAdultBossRewardLoc(int rc) {
    return rc == (int)RC_PHANTOM_GANON || rc == (int)RC_VOLVAGIA || rc == (int)RC_MORPHA ||
           rc == (int)RC_TWINROVA || rc == (int)RC_BONGO_BONGO;
}

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);  // unbuffered: progress is visible even mid-run
    // Each generation runs the full 2-world assumed fill, so keep N modest: the fix makes the stone
    // placement DETERMINISTIC (stones always take the child reward locs), so this is a revert guard,
    // not a probabilistic search. Pre-fix this same config thrashed ~37/40 attempts and could ship a
    // locked seed; post-fix it settles in ~2-3 attempts with every stone obtainable as child.
    const int N = 12;
    std::printf("=== Closed Door of Time + End-of-Dungeons rewards: %d seeds, 2 worlds, child start ===\n", N);

    int locked = 0;            // Fill's own verdict: not jointly beatable (conservative)
    int stoneAtAdultBoss = 0;  // definitive lock: a stone sits behind an adult dungeon
    int reportedLocks = 0;
    long long totalAttempts = 0;

    for (int i = 0; i < N; ++i) {
        Generator::Options o;
        o.seed = 0xD00D0F7100000000ULL + (uint64_t)i * 0x100000001B3ULL;  // vary per iteration
        o.players = { "Mori", "TaCqz" };
        o.settings = {
            { (uint16_t)RSK_SHUFFLE_DUNGEON_REWARDS, (uint16_t)RO_DUNGEON_REWARDS_END_OF_DUNGEON },
            { (uint16_t)RSK_DOOR_OF_TIME,            (uint16_t)RO_DOOROFTIME_CLOSED },
            { (uint16_t)RSK_STARTING_AGE,            (uint16_t)RO_AGE_CHILD },
        };
        std::printf("  seed %2d/%d (0x%llx) ... ", i + 1, N, (unsigned long long)o.seed);
        Generator::Output out = Generator::Generate(o);
        totalAttempts += out.attempts;
        std::printf("beatable=%d attempts=%d placements=%zu\n",
                    (int)out.beatable, out.attempts, out.seed.placements.size());

        if (!out.beatable) {
            ++locked;
            if (reportedLocks < 6) {
                std::printf("    LOCKED seed #%d (0x%llx) after %d attempts\n",
                            i, (unsigned long long)o.seed, out.attempts);
                ++reportedLocks;
            }
        }
        bool badStone = false;
        for (const auto& p : out.seed.placements)
            if (IsStone(p.item) && IsAdultBossRewardLoc(p.loc)) { badStone = true; break; }
        if (badStone) ++stoneAtAdultBoss;
    }

    std::printf("\n  locked (Fill verdict, not jointly beatable): %d / %d\n", locked, N);
    std::printf("  seeds with a spiritual stone at an ADULT boss reward (true lock): %d / %d\n",
                stoneAtAdultBoss, N);
    std::printf("  avg fill attempts: %.2f\n", (double)totalAttempts / (double)N);

    check(stoneAtAdultBoss == 0, "no closed-DoT seed strands a spiritual stone behind an adult dungeon");
    check(locked == 0, "every closed-DoT End-of-Dungeons seed is jointly beatable");

    std::printf("\n%s (%d failure(s))\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
