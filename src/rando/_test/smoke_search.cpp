// Smoke test: build the graph, then measure reachable checks with no items vs. with a
// set of key progression items — the count must grow as the inventory grows.
#include "rando/Graph.h"
#include "rando/Logic.h"
#include "rando/Search.h"
#include <cstdio>

int main() {
    RegionTable_Init();   // builds graph + Reset()s logic to the default starting state

    // --- small-key diagnostic ---
    std::printf("Fire Temple keys before: GetSmallKeyCount=%d\n", (int)logic->GetSmallKeyCount(SCENE_FIRE_TEMPLE));
    for (int i = 0; i < 8; ++i) logic->CollectItem(RG_FIRE_TEMPLE_SMALL_KEY, true);
    std::printf("Fire Temple keys after collecting 8: GetSmallKeyCount=%d, SmallKeys(scene,5)=%s\n",
                (int)logic->GetSmallKeyCount(SCENE_FIRE_TEMPLE),
                logic->SmallKeys(SCENE_FIRE_TEMPLE, 5) ? "true" : "false");
    logic->Reset();   // reset before the reachability measurements below


    int base = (int)Search::ReachableLocations().size();
    std::printf("reachable with starting state only: %d\n", base);

    // adult access should already open via Door-of-Time time travel (no items needed)
    bool adult = areaTable[RR_ROOT].Adult();
    std::printf("adult access reached from start (time travel): %s\n", adult ? "yes" : "no");

    const RandomizerGet keyItems[] = {
        RG_KOKIRI_SWORD, RG_PROGRESSIVE_BOMB_BAG, RG_PROGRESSIVE_HOOKSHOT, RG_PROGRESSIVE_HOOKSHOT,
        RG_PROGRESSIVE_STRENGTH, RG_PROGRESSIVE_STRENGTH, RG_PROGRESSIVE_STRENGTH,
        RG_PROGRESSIVE_SCALE, RG_PROGRESSIVE_SCALE, RG_PROGRESSIVE_BOW, RG_PROGRESSIVE_SLINGSHOT,
        RG_DINS_FIRE, RG_PROGRESSIVE_MAGIC_METER, RG_HOVER_BOOTS, RG_IRON_BOOTS, RG_MEGATON_HAMMER,
        RG_MIRROR_SHIELD, RG_GORON_TUNIC, RG_ZORA_TUNIC, RG_PROGRESSIVE_OCARINA, RG_PROGRESSIVE_OCARINA,
        RG_BOOMERANG, RG_LENS_OF_TRUTH, RG_FIRE_ARROWS, RG_LIGHT_ARROWS, RG_MAGIC_BEAN,
        RG_SONG_OF_TIME, RG_SONG_OF_STORMS, RG_ZELDAS_LULLABY, RG_SARIAS_SONG, RG_SUNS_SONG,
        RG_EPONAS_SONG, RG_MINUET_OF_FOREST, RG_BOLERO_OF_FIRE, RG_SERENADE_OF_WATER,
        RG_REQUIEM_OF_SPIRIT, RG_NOCTURNE_OF_SHADOW, RG_PRELUDE_OF_LIGHT,
        RG_KOKIRI_EMERALD, RG_GORON_RUBY, RG_ZORA_SAPPHIRE,
        RG_FOREST_MEDALLION, RG_FIRE_MEDALLION, RG_WATER_MEDALLION,
        RG_SPIRIT_MEDALLION, RG_SHADOW_MEDALLION, RG_LIGHT_MEDALLION,
    };
    for (RandomizerGet rg : keyItems) logic->CollectItem(rg, true);

    int withItems = (int)Search::ReachableLocations().size();
    std::printf("reachable with key progression items: %d\n", withItems);
    std::printf("delta: +%d\n", withItems - base);
    return 0;
}
