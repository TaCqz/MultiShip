// F-045 (part b) verification: Key Rings replace a dungeon's individual small keys with a single
// ring item that grants all of that dungeon's keys.
//
// Key Rings is on the production honored-settings allowlist (Generator.cpp). The generator
// resolves Off / Random / Count into a deterministic per-dungeon selection (Fill.cpp), then for
// each selected dungeon emits ONE RG_*_KEY_RING (progression) plus junk for the now-redundant key
// slots — a 1:1 pool substitution, so the location/placement count is unchanged. Logic::CollectItem
// maps a ring to a small-key count of 10, so a key-gated dungeon stays reachable with just the
// ring (the joint-beatability verify proves it). Asserts, with a non-zero exit on any failure:
//
//   1. OFF: no ring items; the individual small keys are still in the pool.
//   2. COUNT=N: exactly N dungeons are converted; each converted dungeon has one ring per world
//      and zero individual keys, each non-converted dungeon keeps its keys; the seed stays
//      jointly beatable; the total placement count matches Off (1:1 substitution).
//   3. RANDOM: deterministic and jointly beatable.
//   4. NO EFFECT under Vanilla keys: a ring is pointless when small keys are not shuffled, so the
//      selection is empty.
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

// The eight key-ring dungeons governed by Small Key Shuffle (Gerudo Fortress has its own setting
// and is excluded here since the fortress keys stay Vanilla in these cases).
struct Dungeon { const char* name; RandomizerGet smallKey; RandomizerGet ring; };
const Dungeon kDungeons[] = {
    { "Forest",      RG_FOREST_TEMPLE_SMALL_KEY,          RG_FOREST_TEMPLE_KEY_RING },
    { "Fire",        RG_FIRE_TEMPLE_SMALL_KEY,            RG_FIRE_TEMPLE_KEY_RING },
    { "Water",       RG_WATER_TEMPLE_SMALL_KEY,           RG_WATER_TEMPLE_KEY_RING },
    { "Spirit",      RG_SPIRIT_TEMPLE_SMALL_KEY,          RG_SPIRIT_TEMPLE_KEY_RING },
    { "Shadow",      RG_SHADOW_TEMPLE_SMALL_KEY,          RG_SHADOW_TEMPLE_KEY_RING },
    { "Well",        RG_BOTTOM_OF_THE_WELL_SMALL_KEY,     RG_BOTTOM_OF_THE_WELL_KEY_RING },
    { "GTG",         RG_GERUDO_TRAINING_GROUND_SMALL_KEY, RG_GERUDO_TRAINING_GROUND_KEY_RING },
    { "GanonCastle", RG_GANONS_CASTLE_SMALL_KEY,          RG_GANONS_CASTLE_KEY_RING },
};

int CountItem(const SeedFile::SeedData& sd, RandomizerGet rg) {
    int n = 0;
    for (const auto& p : sd.placements) if (p.item == (int)rg) ++n;
    return n;
}
int TotalRings(const SeedFile::SeedData& sd) {
    int n = 0;
    for (const auto& d : kDungeons) n += CountItem(sd, d.ring);
    return n;
}
bool SamePlacements(const SeedFile::SeedData& a, const SeedFile::SeedData& b) {
    if (a.placements.size() != b.placements.size()) return false;
    for (size_t i = 0; i < a.placements.size(); ++i) {
        const auto& x = a.placements[i]; const auto& y = b.placements[i];
        if (x.loc != y.loc || x.locWorld != y.locWorld || x.item != y.item || x.ownerWorld != y.ownerWorld)
            return false;
    }
    return true;
}

Generator::Options keyOpts(uint16_t keysMode, uint16_t keyringMode, uint16_t count) {
    Generator::Options o;
    o.seed = 0x4EE817A6ULL;
    o.players = { "Mori", "TaCqz" };
    o.settings = {
        { (uint16_t)RSK_KEYSANITY,             keysMode },
        { (uint16_t)RSK_KEYRINGS,              keyringMode },
        { (uint16_t)RSK_KEYRINGS_RANDOM_COUNT, count },
    };
    return o;
}

} // namespace

int main() {
    const int numWorlds = 2;

    // --- 1. Off: no rings, individual keys present --------------------------------------------
    std::printf("=== Key Rings Off (Own Dungeon keys) ===\n");
    Generator::Output off = Generator::Generate(keyOpts(RO_DUNGEON_ITEM_LOC_OWN_DUNGEON, RO_KEYRINGS_OFF, 0));
    std::printf("  %zu placements, beatable=%d, rings=%d\n",
                off.seed.placements.size(), (int)off.beatable, TotalRings(off.seed));
    check(off.beatable, "Off seed is jointly beatable");
    check(TotalRings(off.seed) == 0, "Off has no key-ring items");
    check(CountItem(off.seed, RG_FOREST_TEMPLE_SMALL_KEY) > 0, "Off keeps individual small keys");

    // --- 2. Count = 3: exactly three dungeons converted; 1:1 substitution; beatable -----------
    const int kCount = 3;
    std::printf("=== Key Rings Count = %d ===\n", kCount);
    Generator::Output cnt = Generator::Generate(keyOpts(RO_DUNGEON_ITEM_LOC_OWN_DUNGEON, RO_KEYRINGS_COUNT, (uint16_t)kCount));
    int converted = 0, malformed = 0;
    for (const auto& d : kDungeons) {
        int rings = CountItem(cnt.seed, d.ring);
        int keys  = CountItem(cnt.seed, d.smallKey);
        bool isConverted = (rings == numWorlds && keys == 0);   // one ring per world, no loose keys
        bool isUntouched = (rings == 0 && keys > 0);            // keys intact, no ring
        if (isConverted) ++converted;
        else if (!isUntouched) {
            ++malformed;
            std::printf("    !! %s: rings=%d keys=%d (neither cleanly converted nor untouched)\n",
                        d.name, rings, keys);
        }
    }
    std::printf("  %zu placements, beatable=%d, converted=%d, malformed=%d, total rings=%d\n",
                cnt.seed.placements.size(), (int)cnt.beatable, converted, malformed, TotalRings(cnt.seed));
    check(cnt.beatable, "Count seed is jointly beatable (dungeons clearable with their ring)");
    check(malformed == 0, "every dungeon is cleanly converted (1 ring/world, 0 keys) or untouched (keys, no ring)");
    check(converted == kCount, "exactly Count dungeons are converted to key rings");
    check(TotalRings(cnt.seed) == kCount * numWorlds, "one ring per converted dungeon per world");
    check(cnt.seed.placements.size() == off.seed.placements.size(),
          "key rings are a 1:1 pool substitution (placement count unchanged vs Off)");

    // --- 3. Random: deterministic + beatable --------------------------------------------------
    std::printf("=== Key Rings Random ===\n");
    Generator::Output rnd  = Generator::Generate(keyOpts(RO_DUNGEON_ITEM_LOC_OWN_DUNGEON, RO_KEYRINGS_RANDOM, 0));
    Generator::Output rnd2 = Generator::Generate(keyOpts(RO_DUNGEON_ITEM_LOC_OWN_DUNGEON, RO_KEYRINGS_RANDOM, 0));
    std::printf("  beatable=%d, total rings=%d (0..%d expected)\n",
                (int)rnd.beatable, TotalRings(rnd.seed), (int)(sizeof(kDungeons)/sizeof(kDungeons[0])) * numWorlds);
    check(rnd.beatable, "Random seed is jointly beatable");
    check(SamePlacements(rnd.seed, rnd2.seed), "Random key-ring selection is deterministic");

    // --- 4. No effect when small keys are Vanilla ---------------------------------------------
    std::printf("=== Key Rings Count=5 but Small Keys Vanilla ===\n");
    Generator::Output vk = Generator::Generate(keyOpts(RO_DUNGEON_ITEM_LOC_VANILLA, RO_KEYRINGS_COUNT, 5));
    std::printf("  beatable=%d, total rings=%d (expect 0 — keys not shuffled)\n",
                (int)vk.beatable, TotalRings(vk.seed));
    check(vk.beatable, "Vanilla-keys + key-rings seed is jointly beatable");
    check(TotalRings(vk.seed) == 0, "key rings have no effect when small keys are Vanilla");

    std::printf("\n%s (%d failure(s))\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
