// F-043 (b) verification: Rainbow Bridge + Ganon's Trials are honored by the generator.
//
// Asserts, with non-zero exit on any failure:
//   1. TRIALS RESOLUTION (direct): with Ganon's Trials = Set Number + Trial Count = N, EXACTLY N
//      of the six engine trials are marked required after generation (read straight from the engine
//      Context), Skip marks zero, and Random Number rolls a stable count in [0,6]. The shipped
//      settings carry the resolved count and a concrete mode (Set Number) so the client matches.
//   2. TRIALS BEATABLE: a seed with all six trials required is still jointly beatable.
//   3. BRIDGE: Always-Open and Medallions(6) both generate beatable seeds, and the bridge mode +
//      count round-trip into the shipped settings (so the client's in-game bridge check matches).
//   4. DETERMINISM with bridge + trials honored.
//
// Standalone: links only multiship_rando (the engine is dependency-free). Reads `ctx` directly to
// verify the trial state the generator left behind.
#include "rando/Generator.h"
#include "rando/SeedFile.h"
#include "rando/Context.h"          // `ctx` — inspect resolved trial state after generation
#include "rando/randomizerEnums.h"

#include <cstdio>
#include <vector>

namespace {

int failures = 0;
void check(bool cond, const char* what) {
    std::printf("  [%s] %s\n", cond ? "OK" : "FAIL", what);
    if (!cond) ++failures;
}

bool SamePlacements(const SeedFile::SeedData& a, const SeedFile::SeedData& b) {
    if (a.placements.size() != b.placements.size()) return false;
    for (size_t i = 0; i < a.placements.size(); ++i) {
        const auto& x = a.placements[i];
        const auto& y = b.placements[i];
        if (x.loc != y.loc || x.locWorld != y.locWorld || x.item != y.item || x.ownerWorld != y.ownerWorld)
            return false;
    }
    return true;
}

int shipped(const SeedFile::SeedData& sd, RandomizerSettingKey key) {
    for (const auto& s : sd.settings)
        if (s.key == (uint16_t)key) return (int)s.value;
    return -1;
}

// Count engine trials currently marked required (valid right after a Generate, before the next).
int requiredTrials() {
    int n = 0;
    for (int i = 0; i < TK_MAX; ++i)
        if (ctx->GetTrial((TrialKey)i)->IsRequired()) ++n;
    return n;
}

Generator::Options baseOpts() {
    Generator::Options o;
    o.seed = 0xC0FFEEULL;
    o.players = { "Mori", "TaCqz" };
    return o;
}

} // namespace

int main() {
    // --- 1. Trials resolution -----------------------------------------------------------
    std::printf("=== Ganon's Trials = Set Number (3) ===\n");
    Generator::Options t3 = baseOpts();
    t3.settings = { { (uint16_t)RSK_GANONS_TRIALS, (uint16_t)RO_GANONS_TRIALS_SET_NUMBER },
                    { (uint16_t)RSK_TRIAL_COUNT, 3 } };
    Generator::Output g3 = Generator::Generate(t3);
    int req3 = requiredTrials();
    std::printf("  required trials = %d, shipped count = %d, shipped mode = %d, beatable = %d\n",
                req3, shipped(g3.seed, RSK_TRIAL_COUNT), shipped(g3.seed, RSK_GANONS_TRIALS), (int)g3.beatable);
    check(req3 == 3, "Set Number 3 marks exactly 3 trials required in the engine");
    check(shipped(g3.seed, RSK_TRIAL_COUNT) == 3, "shipped Trial Count == 3");
    check(shipped(g3.seed, RSK_GANONS_TRIALS) == RO_GANONS_TRIALS_SET_NUMBER, "shipped mode collapsed to Set Number");
    check(g3.beatable, "3-trial seed is jointly beatable");

    std::printf("=== Ganon's Trials = Skip ===\n");
    Generator::Options tSkip = baseOpts();
    // RSK_TRIAL_COUNT is always shipped in production (a curated setting); include it so the
    // overwrite-only write-back can carry the resolved value. The engine resolves it to 0 for Skip.
    tSkip.settings = { { (uint16_t)RSK_GANONS_TRIALS, (uint16_t)RO_GANONS_TRIALS_SKIP },
                       { (uint16_t)RSK_TRIAL_COUNT, 4 } };
    Generator::Output gSkip = Generator::Generate(tSkip);
    check(requiredTrials() == 0, "Skip marks zero trials required");
    check(shipped(gSkip.seed, RSK_TRIAL_COUNT) == 0, "shipped Trial Count == 0 for Skip");

    std::printf("=== Ganon's Trials = Set Number (6) ===\n");
    Generator::Options t6 = baseOpts();
    t6.settings = { { (uint16_t)RSK_GANONS_TRIALS, (uint16_t)RO_GANONS_TRIALS_SET_NUMBER },
                    { (uint16_t)RSK_TRIAL_COUNT, 6 } };
    Generator::Output g6 = Generator::Generate(t6);
    check(requiredTrials() == 6, "Set Number 6 marks all six trials required");
    check(g6.beatable, "all-six-trials seed is jointly beatable");

    std::printf("=== Ganon's Trials = Random Number ===\n");
    Generator::Options tRand = baseOpts();
    tRand.settings = { { (uint16_t)RSK_GANONS_TRIALS, (uint16_t)RO_GANONS_TRIALS_RANDOM_NUMBER },
                       { (uint16_t)RSK_TRIAL_COUNT, 0 } };  // engine overwrites with the rolled count
    Generator::Output gR1 = Generator::Generate(tRand);
    int reqR = requiredTrials();
    int shipR = shipped(gR1.seed, RSK_TRIAL_COUNT);
    std::printf("  random rolled %d trials (shipped count %d)\n", reqR, shipR);
    check(reqR >= 0 && reqR <= 6, "Random Number rolls a count in [0,6]");
    check(shipR == reqR, "shipped Trial Count matches the rolled requirement");
    Generator::Output gR2 = Generator::Generate(tRand);
    check(shipped(gR2.seed, RSK_TRIAL_COUNT) == shipR, "Random Number count is deterministic per seed");

    // --- 3. Bridge ----------------------------------------------------------------------
    std::printf("=== Rainbow Bridge ===\n");
    Generator::Options bOpen = baseOpts();
    bOpen.settings = { { (uint16_t)RSK_RAINBOW_BRIDGE, (uint16_t)RO_BRIDGE_ALWAYS_OPEN } };
    Generator::Output gOpen = Generator::Generate(bOpen);
    check(gOpen.beatable, "Bridge = Always Open is jointly beatable");

    Generator::Options bMed = baseOpts();
    bMed.settings = { { (uint16_t)RSK_RAINBOW_BRIDGE, (uint16_t)RO_BRIDGE_MEDALLIONS },
                      { (uint16_t)RSK_RAINBOW_BRIDGE_MEDALLION_COUNT, 6 } };
    Generator::Output gMed = Generator::Generate(bMed);
    std::printf("  Medallions(6): beatable=%d, shipped mode=%d, shipped count=%d\n",
                (int)gMed.beatable, shipped(gMed.seed, RSK_RAINBOW_BRIDGE),
                shipped(gMed.seed, RSK_RAINBOW_BRIDGE_MEDALLION_COUNT));
    check(gMed.beatable, "Bridge = Medallions(6) is jointly beatable");
    check(shipped(gMed.seed, RSK_RAINBOW_BRIDGE) == RO_BRIDGE_MEDALLIONS, "shipped bridge mode == Medallions");
    check(shipped(gMed.seed, RSK_RAINBOW_BRIDGE_MEDALLION_COUNT) == 6, "shipped medallion count == 6");

    // --- 4. Determinism with bridge + trials honored ------------------------------------
    Generator::Output g6b = Generator::Generate(t6);
    check(SamePlacements(g6.seed, g6b.seed), "deterministic placements with trials honored");

    std::printf("\n%s (%d failure(s))\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
