// Diagnostic: with a full inventory, is a deep-dungeon location's region reached, and do
// the events it depends on fire? Distinguishes region-unreached vs condition-false.
#include "rando/Graph.h"
#include "rando/Logic.h"
#include "rando/Search.h"
#include "rando/MetaData.h"
#include <cstdio>

static const char* TF(bool b) { return b ? "Y" : "."; }

int main() {
    RegionTable_Init();
    // Full inventory: collect every location's vanilla item (keys/tokens/medallions/etc.).
    for (int i = 0; i < kLocationCount; ++i)
        if (kLocations[i].vanilla != RG_NONE) logic->CollectItem(kLocations[i].vanilla, true);

    Search::Run();

    auto dumpRegionsWith = [&](RandomizerCheck target, const char* label) {
        std::printf("== %s (RC=%d) ==\n", label, (int)target);
        for (Region& r : areaTable) {
            for (LocationAccess& loc : r.locations) {
                if (loc.location != target) continue;
                std::printf("  region '%s'  access[cD%s cN%s aD%s aN%s]\n",
                            r.regionName.c_str(), TF(r.childDay), TF(r.childNight),
                            TF(r.adultDay), TF(r.adultNight));
            }
        }
    };

    // a few representative deep-dungeon checks
    dumpRegionsWith(RC_FOREST_TEMPLE_BOW_CHEST, "Forest Bow Chest");

    // are key dungeon regions reached at all?
    auto reg = [&](RandomizerRegion k, const char* n) {
        Region& r = areaTable[k];
        std::printf("  %-34s cD%s cN%s aD%s aN%s\n", n, TF(r.childDay), TF(r.childNight),
                    TF(r.adultDay), TF(r.adultNight));
    };
    std::printf("== overworld region access (full inventory) ==\n");
    reg(RR_ROOT, "ROOT");
    reg(RR_KOKIRI_FOREST, "KOKIRI_FOREST");
    reg(RR_THE_LOST_WOODS, "THE_LOST_WOODS");
    reg(RR_SACRED_FOREST_MEADOW, "SACRED_FOREST_MEADOW");
    reg(RR_HYRULE_FIELD, "HYRULE_FIELD");
    reg(RR_LAKE_HYLIA, "LAKE_HYLIA");
    reg(RR_THE_GRAVEYARD, "GRAVEYARD");
    reg(RR_DESERT_COLOSSUS, "DESERT_COLOSSUS");
    reg(RR_ZORAS_FOUNTAIN, "ZORAS_FOUNTAIN");
    std::printf("== dungeon-entry region access ==\n");
    reg(RR_FOREST_TEMPLE_ENTRYWAY, "FOREST_TEMPLE_ENTRYWAY");
    reg(RR_FIRE_TEMPLE_ENTRYWAY, "FIRE_TEMPLE_ENTRYWAY");
    reg(RR_WATER_TEMPLE_ENTRYWAY, "WATER_TEMPLE_ENTRYWAY");
    reg(RR_SHADOW_TEMPLE_ENTRYWAY, "SHADOW_TEMPLE_ENTRYWAY");
    reg(RR_SPIRIT_TEMPLE_ENTRYWAY, "SPIRIT_TEMPLE_ENTRYWAY");
    reg(RR_FIRE_TEMPLE_FOYER, "FIRE_TEMPLE_FOYER");

    // water temple state machine?
    std::printf("== water temple (full inv) ==\n");
    reg(RR_WATER_TEMPLE_ENTRYWAY, "WATER_TEMPLE_ENTRYWAY");
    reg(RR_WATER_TEMPLE_BOSS_KEY_ROOM, "WATER_BOSS_KEY_ROOM");
    reg(RR_WATER_TEMPLE_BOSS_ROOM, "WATER_BOSS_ROOM");
    std::printf("  LOGIC_WATER_HIGH=%s LOW=%s MIDDLE=%s WATER_TEMPLE_CLEAR=%s\n",
                TF(logic->Get(LOGIC_WATER_HIGH)), TF(logic->Get(LOGIC_WATER_LOW)),
                TF(logic->Get(LOGIC_WATER_MIDDLE)), TF(logic->Get(LOGIC_WATER_TEMPLE_CLEAR)));

    // do some clear-events fire?
    std::printf("== events ==\n");
    std::printf("  FOREST_CLEAR_BETWEEN_JOELLE_AND_BETH = %s\n",
                TF(logic->Get(LOGIC_FOREST_CLEAR_BETWEEN_JOELLE_AND_BETH)));

    // song / ocarina checks (full inventory). evaluate as adult.
    logic->IsChild = false; logic->IsAdult = true; logic->AtDay = true; logic->AtNight = false;
    std::printf("== songs/ocarina (full inv, as adult) ==\n");
    std::printf("  HasItem(FAIRY_OCARINA)        = %s\n", TF(logic->HasItem(RG_FAIRY_OCARINA)));
    std::printf("  HasItem(OCARINA_C_LEFT_BUTTON)= %s\n", TF(logic->HasItem(RG_OCARINA_C_LEFT_BUTTON)));
    std::printf("  HasItem(SARIAS_SONG)          = %s\n", TF(logic->HasItem(RG_SARIAS_SONG)));
    std::printf("  CanUse(SARIAS_SONG)           = %s\n", TF(logic->CanUse(RG_SARIAS_SONG)));
    std::printf("  CanUse(REQUIEM_OF_SPIRIT)     = %s\n", TF(logic->CanUse(RG_REQUIEM_OF_SPIRIT)));
    std::printf("  CanUse(PROGRESSIVE_HOOKSHOT)? HasItem(HOOKSHOT)=%s\n", TF(logic->HasItem(RG_HOOKSHOT)));
    return 0;
}
