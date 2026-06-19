// Verifies cowsanity + fishsanity pool their locations and stay beatable, and that any
// non-off fishsanity mode is canonicalized to Both in the shipped settings (the engine
// pools all fish regardless of the Loach/Pond/Overworld/Both sub-mode).
#include "rando/Fill.h"
#include "rando/MetaData.h"
#include <cstdio>
#include <unordered_set>
#include <vector>

static int countAt(const Fill::Result& r, const std::unordered_set<int>& locs) {
    int n = 0;
    for (const auto& pl : r.placements) if (locs.count((int)pl.loc)) ++n;
    return n;
}

int main() {
    int fail = 0;
    std::unordered_set<int> cowLocs, fishLocs;
    for (int i = 0; i < kLocationCount; ++i) {
        if (kLocations[i].category == LC_COW)  cowLocs.insert((int)kLocations[i].rc);
        if (kLocations[i].category == LC_FISH) fishLocs.insert((int)kLocations[i].rc);
    }

    // --- Cowsanity (toggle) ---
    for (uint8_t v : { (uint8_t)0, (uint8_t)1 }) {
        std::vector<Fill::SettingOverride> ov = { { (uint16_t)RSK_SHUFFLE_COWS, v } };
        Fill::Result r = Fill::Generate(12345, 2, ov);
        int cows = countAt(r, cowLocs);
        std::printf("cows=%u: %zu placements (%d at cows), beatable=%s\n",
                    v, r.placements.size(), cows, r.beatable ? "YES" : "NO");
        if (!r.beatable) ++fail;
        if (v == 0 && cows != 0) { std::printf("  FAIL: cows pooled while off\n"); ++fail; }
        if (v == 1 && cows == 0) { std::printf("  FAIL: cows not pooled while on\n"); ++fail; }
    }

    // --- Fishsanity (off + every non-off mode -> all fish, canonicalized to Both) ---
    for (uint8_t v : { (uint8_t)RO_FISHSANITY_OFF, (uint8_t)RO_FISHSANITY_HYRULE_LOACH,
                       (uint8_t)RO_FISHSANITY_POND, (uint8_t)RO_FISHSANITY_OVERWORLD,
                       (uint8_t)RO_FISHSANITY_BOTH }) {
        std::vector<Fill::SettingOverride> ov = { { (uint16_t)RSK_FISHSANITY, v } };
        Fill::Result r = Fill::Generate(12345, 2, ov);
        int fish = countAt(r, fishLocs);
        std::printf("fish=%u: %zu placements (%d at fish), beatable=%s; synced fish=%u\n",
                    v, r.placements.size(), fish, r.beatable ? "YES" : "NO",
                    r.settings[(size_t)RSK_FISHSANITY]);
        if (!r.beatable) ++fail;
        if (v == RO_FISHSANITY_OFF) {
            if (fish != 0) { std::printf("  FAIL: fish pooled while off\n"); ++fail; }
        } else {
            if (fish == 0) { std::printf("  FAIL: fish not pooled while on\n"); ++fail; }
            if (r.settings[(size_t)RSK_FISHSANITY] != RO_FISHSANITY_BOTH) {
                std::printf("  FAIL: non-off fishsanity not canonicalized to Both\n"); ++fail;
            }
        }
    }

    std::printf(fail ? "\n%d CHECK(S) FAILED\n" : "\nALL COW/FISH CHECKS PASSED\n", fail);
    return fail ? 1 : 0;
}
