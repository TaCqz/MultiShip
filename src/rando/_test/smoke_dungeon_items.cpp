// F-045 (part a) verification: dungeon-item placement (maps & compasses, small keys, boss keys,
// Ganon's Boss Key, Gerudo Fortress keys) is honored end-to-end by the generator.
//
// These settings are on the production honored-settings allowlist (Generator.cpp), so their
// curated value restricts WHERE the matching item may land (Fill.cpp's FEAT-5 restricted fill)
// and the value round-trips into the seed so the SoH client enforces the same world. Asserts,
// with a non-zero exit on any failure:
//
//   1. OWN DUNGEON: every placed map/compass/small key/boss key lands in a location of its own
//      dungeon, in its own world, and the seed stays jointly beatable.
//   2. ANYWHERE: small keys escape their home dungeon (the constraint is genuinely lifted) and
//      the seed stays jointly beatable.
//   3. GERUDO FORTRESS KEYS: Vanilla keeps the four keys out of the pool; Anywhere puts all four
//      (per world) into the pool, and the seed stays beatable.
//   4. GANON'S BOSS KEY: Vanilla keeps it out of the pool; Own Dungeon places it inside Ganon's
//      Castle, in its own world.
//   5. START WITH: small keys are granted at the start (absent from the pool) yet the seed stays
//      beatable, and the value ships as Start-With so the client grants them at save init.
//   6. SHIPPED / WRITE-BACK: the curated dungeon-item values round-trip into the seed settings.
//
// Standalone: the engine is dependency-free, so this links only multiship_rando.
#include "rando/Generator.h"
#include "rando/SeedFile.h"
#include "rando/MetaData.h"        // kLocations -> per-location zone, dungeon-item categories
#include "rando/randomizerEnums.h"

#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {

int failures = 0;
void check(bool cond, const char* what) {
    std::printf("  [%s] %s\n", cond ? "OK" : "FAIL", what);
    if (!cond) ++failures;
}

int shipped(const SeedFile::SeedData& sd, RandomizerSettingKey key) {
    for (const auto& s : sd.settings)
        if (s.key == (uint16_t)key) return (int)s.value;
    return -1;
}

// Is this location category one of the pooled dungeon-item categories?
bool IsDungeonItemCat(LocCategory c) {
    return c == LC_SMALL_KEY || c == LC_BOSS_KEY || c == LC_MAP || c == LC_COMPASS ||
           c == LC_GANON_BOSS_KEY;
}

// Built from kLocations: each dungeon item's HOME zone (the dungeon it belongs to) and each
// location's zone. Lets us assert "Own Dungeon" placement without hard-coding the layout.
std::unordered_map<int, uint8_t> gItemHomeZone;   // RG dungeon item -> home DungeonZone
std::unordered_map<int, uint8_t> gLocZone;        // RC location     -> DungeonZone
std::unordered_set<int> gSmallKeyItems, gBossKeyItems, gMapCompassItems;
int gGanonBkItem = (int)RG_GANONS_CASTLE_BOSS_KEY;
int gGfKeyItem   = (int)RG_GERUDO_FORTRESS_SMALL_KEY;

void BuildMetadata() {
    for (int i = 0; i < kLocationCount; ++i) {
        const auto& L = kLocations[i];
        gLocZone[(int)L.rc] = L.zone;
        if (!IsDungeonItemCat(L.category)) continue;
        gItemHomeZone[(int)L.vanilla] = L.zone;
        if (L.vanilla == RG_GERUDO_FORTRESS_SMALL_KEY) continue;  // its own setting, not keysanity
        if (L.category == LC_SMALL_KEY)                 gSmallKeyItems.insert((int)L.vanilla);
        else if (L.category == LC_BOSS_KEY)             gBossKeyItems.insert((int)L.vanilla);
        else if (L.category == LC_MAP || L.category == LC_COMPASS) gMapCompassItems.insert((int)L.vanilla);
    }
}

int CountItem(const SeedFile::SeedData& sd, RandomizerGet rg) {
    int n = 0;
    for (const auto& p : sd.placements) if (p.item == (int)rg) ++n;
    return n;
}

Generator::Options baseOpts() {
    Generator::Options o;
    o.seed = 0xD0E17E33ULL;
    o.players = { "Mori", "TaCqz" };
    return o;
}

} // namespace

int main() {
    BuildMetadata();

    // --- 1. Own Dungeon: every placed dungeon item stays in its home dungeon, own world -------
    std::printf("=== Own Dungeon (keys / boss keys / maps & compasses) ===\n");
    Generator::Options ownO = baseOpts();
    ownO.settings = {
        { (uint16_t)RSK_KEYSANITY,             (uint16_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON },
        { (uint16_t)RSK_BOSS_KEYSANITY,        (uint16_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON },
        { (uint16_t)RSK_SHUFFLE_MAPANDCOMPASS, (uint16_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON },
    };
    Generator::Output own = Generator::Generate(ownO);
    std::printf("  %zu placements, beatable=%d\n", own.seed.placements.size(), (int)own.beatable);
    check(own.beatable, "Own Dungeon seed is jointly beatable");

    int ownItems = 0, ownViolations = 0;
    for (const auto& p : own.seed.placements) {
        auto h = gItemHomeZone.find(p.item);
        bool isDungeonItem = h != gItemHomeZone.end() &&
            (gSmallKeyItems.count(p.item) || gBossKeyItems.count(p.item) || gMapCompassItems.count(p.item));
        if (!isDungeonItem) continue;
        ++ownItems;
        bool ok = p.locWorld == p.ownerWorld && gLocZone[p.loc] == h->second;
        if (!ok) ++ownViolations;
    }
    std::printf("  checked %d placed dungeon items, %d outside own dungeon\n", ownItems, ownViolations);
    check(ownItems > 0, "Own Dungeon actually places dungeon items into the pool");
    check(ownViolations == 0, "every Own-Dungeon item is in its home dungeon, own world");
    check(shipped(own.seed, RSK_KEYSANITY) == RO_DUNGEON_ITEM_LOC_OWN_DUNGEON, "shipped Small Keys = Own Dungeon");
    check(shipped(own.seed, RSK_BOSS_KEYSANITY) == RO_DUNGEON_ITEM_LOC_OWN_DUNGEON, "shipped Boss Keys = Own Dungeon");
    check(shipped(own.seed, RSK_SHUFFLE_MAPANDCOMPASS) == RO_DUNGEON_ITEM_LOC_OWN_DUNGEON, "shipped Maps & Compasses = Own Dungeon");

    // --- 2. Anywhere: small keys escape their home dungeon, still beatable --------------------
    std::printf("=== Anywhere small keys ===\n");
    Generator::Options anyO = baseOpts();
    anyO.settings = { { (uint16_t)RSK_KEYSANITY, (uint16_t)RO_DUNGEON_ITEM_LOC_ANYWHERE } };
    Generator::Output any = Generator::Generate(anyO);
    int escaped = 0;
    for (const auto& p : any.seed.placements)
        if (gSmallKeyItems.count(p.item)) {
            auto h = gItemHomeZone.find(p.item);
            if (p.locWorld != p.ownerWorld || (h != gItemHomeZone.end() && gLocZone[p.loc] != h->second)) ++escaped;
        }
    std::printf("  %zu placements, beatable=%d, %d small keys outside their home dungeon\n",
                any.seed.placements.size(), (int)any.beatable, escaped);
    check(any.beatable, "Anywhere seed is jointly beatable");
    check(escaped > 0, "Anywhere lets small keys leave their home dungeon (cross-world / cross-dungeon)");
    check(shipped(any.seed, RSK_KEYSANITY) == RO_DUNGEON_ITEM_LOC_ANYWHERE, "shipped Small Keys = Anywhere");

    // --- 3. Gerudo Fortress keys: Vanilla absent, Anywhere all four pooled --------------------
    std::printf("=== Gerudo Fortress keys ===\n");
    Generator::Output gfVanilla = Generator::Generate(baseOpts());  // default = Vanilla
    int gfV = CountItem(gfVanilla.seed, RG_GERUDO_FORTRESS_SMALL_KEY);
    Generator::Options gfO = baseOpts();
    gfO.settings = { { (uint16_t)RSK_GERUDO_KEYS, (uint16_t)RO_GERUDO_KEYS_ANYWHERE } };
    Generator::Output gfAny = Generator::Generate(gfO);
    int gfA = CountItem(gfAny.seed, RG_GERUDO_FORTRESS_SMALL_KEY);
    std::printf("  vanilla placed=%d (expect 0), anywhere placed=%d (expect 8 = 4 keys x 2 worlds), beatable=%d\n",
                gfV, gfA, (int)gfAny.beatable);
    check(gfV == 0, "Gerudo Fortress keys Vanilla are not pooled");
    check(gfA == 8, "Gerudo Fortress keys Anywhere pool all four keys per world");
    check(gfAny.beatable, "Gerudo Fortress keys Anywhere seed is jointly beatable");
    check(shipped(gfAny.seed, RSK_GERUDO_KEYS) == RO_GERUDO_KEYS_ANYWHERE, "shipped Gerudo Fortress Keys = Anywhere");

    // --- 4. Ganon's Boss Key: Vanilla absent, Own Dungeon inside Ganon's Castle ---------------
    std::printf("=== Ganon's Boss Key ===\n");
    Generator::Output gbkVanilla = Generator::Generate(baseOpts());  // default = Vanilla
    check(CountItem(gbkVanilla.seed, RG_GANONS_CASTLE_BOSS_KEY) == 0, "Ganon's Boss Key Vanilla is not pooled");
    Generator::Options gbkO = baseOpts();
    gbkO.settings = { { (uint16_t)RSK_GANONS_BOSS_KEY, (uint16_t)RO_GANON_BOSS_KEY_OWN_DUNGEON } };
    Generator::Output gbk = Generator::Generate(gbkO);
    int gbkN = 0, gbkBad = 0;
    for (const auto& p : gbk.seed.placements)
        if (p.item == (int)RG_GANONS_CASTLE_BOSS_KEY) {
            ++gbkN;
            if (p.locWorld != p.ownerWorld || gLocZone[p.loc] != (uint8_t)DZ_GANONS_CASTLE) ++gbkBad;
        }
    std::printf("  own-dungeon placed=%d (expect 2), outside Ganon's Castle=%d, beatable=%d\n",
                gbkN, gbkBad, (int)gbk.beatable);
    check(gbkN == 2, "Ganon's Boss Key Own Dungeon places one per world");
    check(gbkBad == 0, "Ganon's Boss Key Own Dungeon stays inside Ganon's Castle, own world");
    check(gbk.beatable, "Ganon's Boss Key Own Dungeon seed is jointly beatable");

    // --- 5. Start With: small keys granted at init (absent from pool), still beatable ----------
    std::printf("=== Start With small keys ===\n");
    Generator::Options swO = baseOpts();
    swO.settings = { { (uint16_t)RSK_KEYSANITY, (uint16_t)RO_DUNGEON_ITEM_LOC_STARTWITH } };
    Generator::Output sw = Generator::Generate(swO);
    int swKeys = 0;
    for (const auto& p : sw.seed.placements) if (gSmallKeyItems.count(p.item)) ++swKeys;
    std::printf("  small keys in pool=%d (expect 0), beatable=%d, shipped=%d\n",
                swKeys, (int)sw.beatable, shipped(sw.seed, RSK_KEYSANITY));
    check(swKeys == 0, "Start-With small keys are not placed (granted at init instead)");
    check(sw.beatable, "Start-With small keys seed is jointly beatable (keys assumed from start)");
    check(shipped(sw.seed, RSK_KEYSANITY) == RO_DUNGEON_ITEM_LOC_STARTWITH,
          "Start-With value ships verbatim so the client grants the keys");

    // --- 6. Determinism with dungeon items honored --------------------------------------------
    Generator::Output own2 = Generator::Generate(ownO);
    bool same = own.seed.placements.size() == own2.seed.placements.size();
    for (size_t i = 0; same && i < own.seed.placements.size(); ++i) {
        const auto& a = own.seed.placements[i];
        const auto& b = own2.seed.placements[i];
        same = a.loc == b.loc && a.locWorld == b.locWorld && a.item == b.item && a.ownerWorld == b.ownerWorld;
    }
    check(same, "Own Dungeon generation is deterministic (same seed -> identical placements)");

    std::printf("\n%s (%d failure(s))\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
