// F-042 verification: the gCtx reset + the honored-settings override plumbing.
//
// Asserts, with non-zero exit on any failure:
//   1. DETERMINISM (empty allowlist): generating twice with identical inputs yields
//      identical placements + seed id (the baseline contract; reinforces foundation_smoke).
//   2. OVERRIDE PLUMBING: with a TEMPORARY probe key on the honored allowlist, flipping
//      that curated setting changes the placement table — i.e. the override reaches the
//      engine's pool/placement (with an empty allowlist the same setting is ignored).
//   3. DETERMINISM (with the override): generating twice under the probe is identical too.
//   4. NO CROSS-SEED LEAK (the gCtx reset): after removing the probe, the baseline is
//      reproduced EXACTLY — the probe run left no residue in the singleton settings ctx.
//      Without the RegionTable_Init reset, the probe's RSK_SHUFFLE_COWS=On would survive
//      and this final seed would still pool cows (base3 != base) — the regression this guards.
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

bool SamePlacements(const SeedFile::SeedData& a, const SeedFile::SeedData& b) {
    if (a.placements.size() != b.placements.size()) return false;
    for (size_t i = 0; i < a.placements.size(); ++i) {
        const auto& x = a.placements[i];
        const auto& y = b.placements[i];
        if (x.loc != y.loc || x.locWorld != y.locWorld || x.item != y.item ||
            x.ownerWorld != y.ownerWorld)
            return false;
    }
    return true;
}

} // namespace

int main() {
    // The probe: RSK_SHUFFLE_COWS=On pools the 9 cow checks per world (vanilla item =
    // RG_MILK, junk), so honoring it visibly enlarges + relayouts the placement table
    // while never stranding progression. It is NOT on the production allowlist, so by
    // default it is ignored for placement (carried to the output only).
    const uint16_t kProbeKey = (uint16_t)RSK_SHUFFLE_COWS;
    const uint16_t kProbeOn  = 1;  // base option index for "On"

    Generator::Options opts;
    opts.seed = 0xC0FFEEULL;
    opts.players = { "Mori", "TaCqz" };
    opts.settings = { { kProbeKey, kProbeOn } };

    // --- 1. Determinism with the (empty) production allowlist ---------------------------
    Generator::SetHonoredSettingsForTest({});  // explicit baseline, independent of the default
    std::printf("=== baseline (empty allowlist) ===\n");
    Generator::Output base  = Generator::Generate(opts);
    Generator::Output base2 = Generator::Generate(opts);
    std::printf("  %zu placements, beatable=%d, seedId=%s\n",
                base.seed.placements.size(), (int)base.beatable, base.seed.seedId.c_str());
    check(base.beatable, "baseline 2-world seed is jointly beatable");
    check(SamePlacements(base.seed, base2.seed), "baseline deterministic (twice -> identical placements)");
    check(base.seed.seedId == base2.seed.seedId, "baseline seed id deterministic");

    // --- 2. Override plumbing: a probe on the allowlist changes placement ---------------
    Generator::SetHonoredSettingsForTest({ kProbeKey });
    std::printf("=== probe (RSK_SHUFFLE_COWS honored) ===\n");
    Generator::Output probe  = Generator::Generate(opts);
    Generator::Output probe2 = Generator::Generate(opts);
    std::printf("  %zu placements, beatable=%d\n", probe.seed.placements.size(), (int)probe.beatable);
    check(!SamePlacements(base.seed, probe.seed), "honoring the probe setting changes placement");
    check(probe.seed.placements.size() > base.seed.placements.size(),
          "cowsanity pools extra checks (more placements than baseline)");
    check(probe.beatable, "probe seed is still jointly beatable");

    // --- 3. Determinism with the override -----------------------------------------------
    check(SamePlacements(probe.seed, probe2.seed), "probe deterministic (twice -> identical placements)");

    // --- 4. No cross-seed leak: removing the probe reproduces the EXACT baseline --------
    Generator::SetHonoredSettingsForTest({});
    std::printf("=== baseline reproduced (probe removed) ===\n");
    Generator::Output base3 = Generator::Generate(opts);
    check(SamePlacements(base.seed, base3.seed),
          "no cross-seed leak: baseline reproduced after the probe runs (gCtx reset)");

    std::printf("\n%s (%d failure(s))\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
