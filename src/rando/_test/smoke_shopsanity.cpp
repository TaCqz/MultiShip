// Verifies shopsanity pools shop locations + stays beatable, and that any non-off mode
// is canonicalized to Specific-Count / Eight-Items in the shipped settings (so the pool
// the engine generated matches the settings shipped to clients).
#include "rando/Fill.h"
#include "rando/MetaData.h"
#include <cstdio>
#include <unordered_set>
#include <vector>

int main() {
    int fail = 0;

    // Set of shop-category locations, to confirm they actually enter the pool when on.
    std::unordered_set<int> shopLocs;
    for (int i = 0; i < kLocationCount; ++i)
        if (kLocations[i].category == LC_SHOP) shopLocs.insert((int)kLocations[i].rc);

    for (uint8_t v : { (uint8_t)RO_SHOPSANITY_OFF, (uint8_t)RO_SHOPSANITY_SPECIFIC_COUNT,
                       (uint8_t)RO_SHOPSANITY_RANDOM }) {
        std::vector<Fill::SettingOverride> ov = { { (uint16_t)RSK_SHOPSANITY, v } };
        Fill::Result r = Fill::Generate(12345, 2, ov);

        int shopPlaced = 0;
        for (const auto& pl : r.placements)
            if (shopLocs.count((int)pl.loc)) ++shopPlaced;

        std::printf("shopsanity=%u: %zu placements (%d at shop slots), %d attempt(s), "
                    "beatable=%s; synced shopsanity=%u count=%u\n",
                    v, r.placements.size(), shopPlaced, r.attempts, r.beatable ? "YES" : "NO",
                    r.settings[(size_t)RSK_SHOPSANITY], r.settings[(size_t)RSK_SHOPSANITY_COUNT]);

        if (!r.beatable) ++fail;
        if (v == RO_SHOPSANITY_OFF) {
            if (shopPlaced != 0) { std::printf("  FAIL: shop slots pooled while off\n"); ++fail; }
        } else {
            if (shopPlaced == 0) { std::printf("  FAIL: shop slots not pooled while on\n"); ++fail; }
            if (r.settings[(size_t)RSK_SHOPSANITY] != RO_SHOPSANITY_SPECIFIC_COUNT) {
                std::printf("  FAIL: non-off not canonicalized to Specific-Count\n"); ++fail;
            }
            if (r.settings[(size_t)RSK_SHOPSANITY_COUNT] != RO_SHOPSANITY_COUNT_EIGHT_ITEMS) {
                std::printf("  FAIL: count not canonicalized to Eight-Items\n"); ++fail;
            }
        }
    }

    std::printf(fail ? "\n%d CHECK(S) FAILED\n" : "\nALL SHOPSANITY CHECKS PASSED\n", fail);
    return fail ? 1 : 0;
}
