// Verifies scrubsanity pools scrub locations + stays beatable, and that unsupported
// pool settings (keys etc.) are normalized to vanilla in the shipped settings.
#include "rando/Fill.h"
#include <cstdio>
#include <vector>

int main() {
    int fail = 0;

    for (uint8_t v : { (uint8_t)RO_SCRUBS_OFF, (uint8_t)RO_SCRUBS_ONE_TIME_ONLY, (uint8_t)RO_SCRUBS_ALL }) {
        std::vector<Fill::SettingOverride> ov = { { (uint16_t)RSK_SHUFFLE_SCRUBS, v } };
        Fill::Result r = Fill::Generate(12345, 2, ov);
        std::printf("scrubs=%u: %zu placements, %d attempt(s), beatable=%s; synced scrubs=%u\n",
                    v, r.placements.size(), r.attempts, r.beatable ? "YES" : "NO",
                    r.settings[(size_t)RSK_SHUFFLE_SCRUBS]);
        if (!r.beatable) ++fail;
        if (v == RO_SCRUBS_ONE_TIME_ONLY && r.settings[(size_t)RSK_SHUFFLE_SCRUBS] != RO_SCRUBS_ALL) {
            std::printf("  FAIL: One-Time not normalized to All\n");
            ++fail;
        }
    }

    // Unsupported key shuffle: pick Own Dungeon, expect it normalized to Vanilla.
    {
        std::vector<Fill::SettingOverride> ov = {
            { (uint16_t)RSK_KEYSANITY, (uint8_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON }
        };
        Fill::Result r = Fill::Generate(12345, 2, ov);
        bool ok = r.settings[(size_t)RSK_KEYSANITY] == RO_DUNGEON_ITEM_LOC_VANILLA;
        std::printf("keysanity Own-Dungeon -> synced %u (want Vanilla=%u): %s\n",
                    r.settings[(size_t)RSK_KEYSANITY], (unsigned)RO_DUNGEON_ITEM_LOC_VANILLA,
                    ok ? "normalized OK" : "FAIL");
        if (!ok) ++fail;
    }

    std::printf(fail ? "\n%d CHECK(S) FAILED\n" : "\nALL SCRUB/NORMALIZE CHECKS PASSED\n", fail);
    return fail ? 1 : 0;
}
