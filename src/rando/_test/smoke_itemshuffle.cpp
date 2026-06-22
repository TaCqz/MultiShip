// Verifies the item-shuffle toggles actually gate the pool, and that the shipped
// settings never disagree with the generated pool (the failure mode the normalization
// block exists to prevent). Covers: songs, ocarinas, adult trade, weird egg, kokiri
// sword, master sword. For each: pool membership must track the setting, the seed must
// stay beatable, and the shipped setting must match what was pooled.
#include "rando/Fill.h"
#include <cstdio>
#include <vector>

// A location is in the shuffled pool iff some placement lands on it (every shuffled slot
// gets filled; a non-shuffled vanilla location is never a placement target).
static bool InPool(const Fill::Result& r, RandomizerCheck rc) {
    for (const auto& p : r.placements)
        if (p.loc == rc) return true;
    return false;
}

int main() {
    int fail = 0;
    auto check = [&](const char* what, bool cond) {
        if (!cond) { std::printf("  FAIL: %s\n", what); ++fail; }
    };

    // --- Plain on/off toggles: representative location pooled iff the setting is on. ---
    struct BoolCase { const char* name; RandomizerSettingKey key; RandomizerCheck rc; } bcases[] = {
        { "ocarina",      RSK_SHUFFLE_OCARINA,      RC_HF_OCARINA_OF_TIME_ITEM },
        { "adult-trade",  RSK_SHUFFLE_ADULT_TRADE,  RC_LW_TRADE_COJIRO },
        { "weird-egg",    RSK_SHUFFLE_WEIRD_EGG,    RC_HC_MALON_EGG },
        { "kokiri-sword", RSK_SHUFFLE_KOKIRI_SWORD, RC_KF_KOKIRI_SWORD_CHEST },
        { "master-sword", RSK_SHUFFLE_MASTER_SWORD, RC_TOT_MASTER_SWORD },
    };
    for (const auto& c : bcases) {
        for (uint8_t v : { (uint8_t)0, (uint8_t)1 }) {
            std::vector<Fill::SettingOverride> ov = { { (uint16_t)c.key, v } };
            Fill::Result r = Fill::Generate(12345, 2, ov);
            bool pooled = InPool(r, c.rc);
            uint8_t shipped = r.settings[(size_t)c.key];
            std::printf("%-12s = %u: pooled=%-3s beatable=%-3s shipped=%u\n",
                        c.name, v, pooled ? "YES" : "no", r.beatable ? "YES" : "NO", shipped);
            check("not beatable", r.beatable);
            check("pool must track setting", pooled == (v != 0));
            check("shipped value must match pool", (shipped != 0) == pooled);
        }
    }

    // --- Songs: Off keeps song locations vanilla; any non-off pools them with a full
    // (Anywhere) shuffle, and the shipped value is folded to Anywhere to match. ---
    struct SongCase { const char* name; uint8_t v; bool expectPooled; } scases[] = {
        { "OFF",             RO_SONG_SHUFFLE_OFF,             false },
        { "SONG_LOCATIONS",  RO_SONG_SHUFFLE_SONG_LOCATIONS,  true  },
        { "DUNGEON_REWARDS", RO_SONG_SHUFFLE_DUNGEON_REWARDS, true  },
        { "ANYWHERE",        RO_SONG_SHUFFLE_ANYWHERE,        true  },
    };
    for (const auto& c : scases) {
        std::vector<Fill::SettingOverride> ov = { { (uint16_t)RSK_SHUFFLE_SONGS, c.v } };
        Fill::Result r = Fill::Generate(12345, 2, ov);
        bool pooled = InPool(r, RC_SONG_FROM_IMPA);
        uint8_t shipped = r.settings[(size_t)RSK_SHUFFLE_SONGS];
        std::printf("songs %-16s: pooled=%-3s beatable=%-3s shipped=%u\n",
                    c.name, pooled ? "YES" : "no", r.beatable ? "YES" : "NO", shipped);
        check("songs not beatable", r.beatable);
        check("song pool must track setting", pooled == c.expectPooled);
        check("shipped songs must match pool",
              c.expectPooled ? (shipped == RO_SONG_SHUFFLE_ANYWHERE)
                             : (shipped == RO_SONG_SHUFFLE_OFF));
    }

    std::printf(fail ? "\n%d ITEM-SHUFFLE CHECK(S) FAILED\n"
                     : "\nALL ITEM-SHUFFLE CHECKS PASSED\n", fail);
    return fail ? 1 : 0;
}
