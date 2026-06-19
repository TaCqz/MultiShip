// FEAT-5: dungeon-restricted fill for small keys / boss keys / map & compass / Ganon's
// boss key / dungeon rewards. Verifies that across the placement modes the seed stays
// beatable AND every restricted item lands in a location its mode permits (independently
// re-deriving the zone rule here and asserting against the actual placements).
#include "rando/Fill.h"
#include "rando/MetaData.h"
#include <cstdio>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace {
std::unordered_map<int, uint8_t> locZone;      // rc -> DungeonZone
std::unordered_map<int, uint8_t> itemHome;     // dungeon-item rg -> home zone
std::unordered_map<int, LocCategory> itemCat;  // dungeon-item rg -> category
std::unordered_set<int> rewardLoc;

bool isDI(LocCategory c) {
    return c == LC_SMALL_KEY || c == LC_BOSS_KEY || c == LC_MAP || c == LC_COMPASS ||
           c == LC_GANON_BOSS_KEY || c == LC_DUNGEON_REWARD;
}

// Re-derive the allowed-zone predicate from the (already-folded) shipped settings.
enum ZR { ZR_NONE, ZR_OWN, ZR_ANY, ZR_OW, ZR_REWARD };
ZR dloc(uint8_t m) {
    if (m == RO_DUNGEON_ITEM_LOC_OWN_DUNGEON) return ZR_OWN;
    if (m == RO_DUNGEON_ITEM_LOC_ANY_DUNGEON) return ZR_ANY;
    if (m == RO_DUNGEON_ITEM_LOC_OVERWORLD)   return ZR_OW;
    return ZR_NONE;
}
ZR gbk(uint8_t m) {
    if (m == RO_GANON_BOSS_KEY_OWN_DUNGEON) return ZR_OWN;
    if (m == RO_GANON_BOSS_KEY_ANY_DUNGEON) return ZR_ANY;
    if (m == RO_GANON_BOSS_KEY_OVERWORLD)   return ZR_OW;
    return ZR_NONE;
}
ZR rew(uint8_t m) {
    if (m == RO_DUNGEON_REWARDS_END_OF_DUNGEON) return ZR_REWARD;
    if (m == RO_DUNGEON_REWARDS_OWN_DUNGEON)    return ZR_OWN;
    if (m == RO_DUNGEON_REWARDS_ANY_DUNGEON)    return ZR_ANY;
    if (m == RO_DUNGEON_REWARDS_OVERWORLD)      return ZR_OW;
    return ZR_NONE;
}

// Check one result: beatable + every restricted dungeon-item placement obeys its rule.
int check(const char* label, const std::vector<Fill::SettingOverride>& ov) {
    Fill::Result r = Fill::Generate(12345, 2, ov);
    uint8_t mK = r.settings[(size_t)RSK_KEYSANITY], mB = r.settings[(size_t)RSK_BOSS_KEYSANITY];
    uint8_t mM = r.settings[(size_t)RSK_SHUFFLE_MAPANDCOMPASS];
    uint8_t mG = r.settings[(size_t)RSK_GANONS_BOSS_KEY], mR = r.settings[(size_t)RSK_SHUFFLE_DUNGEON_REWARDS];

    int placedDI = 0, viol = 0;
    for (const auto& pl : r.placements) {
        auto ci = itemCat.find((int)pl.item);
        if (ci == itemCat.end()) continue;  // not a dungeon item
        ZR zr = ZR_NONE;
        switch (ci->second) {
            case LC_SMALL_KEY: zr = dloc(mK); break;
            case LC_BOSS_KEY:  zr = dloc(mB); break;
            case LC_MAP: case LC_COMPASS: zr = dloc(mM); break;
            case LC_GANON_BOSS_KEY: zr = gbk(mG); break;
            case LC_DUNGEON_REWARD: zr = rew(mR); break;
            default: break;
        }
        if (zr == ZR_NONE) continue;  // anywhere / vanilla -> unconstrained
        ++placedDI;
        uint8_t z = locZone[(int)pl.loc];
        bool ok = (pl.itemWorld == pl.locWorld);  // restricted -> own world
        if (ok) switch (zr) {
            case ZR_OWN:    ok = (z == itemHome[(int)pl.item]); break;
            case ZR_ANY:    ok = (z != (uint8_t)DZ_OVERWORLD); break;
            case ZR_OW:     ok = (z == (uint8_t)DZ_OVERWORLD); break;
            case ZR_REWARD: ok = (rewardLoc.count((int)pl.loc) != 0); break;
            default: break;
        }
        if (!ok) ++viol;
    }
    std::printf("%-34s beatable=%s; %zu pl, %d restricted-DI placed, %d zone-violation(s); "
                "synced K=%u B=%u MC=%u GBK=%u REW=%u\n",
                label, r.beatable ? "YES" : "NO", r.placements.size(), placedDI, viol,
                mK, mB, mM, mG, mR);
    int f = 0;
    if (!r.beatable) ++f;
    if (viol) ++f;
    return f;
}
}  // namespace

int main() {
    for (int i = 0; i < kLocationCount; ++i) {
        const auto& L = kLocations[i];
        locZone[(int)L.rc] = L.zone;
        if (isDI(L.category)) { itemHome[(int)L.vanilla] = L.zone; itemCat[(int)L.vanilla] = L.category; }
        if (L.category == LC_DUNGEON_REWARD) rewardLoc.insert((int)L.rc);
    }

    int fail = 0;
    using SO = Fill::SettingOverride;
    auto K = (uint16_t)RSK_KEYSANITY; auto B = (uint16_t)RSK_BOSS_KEYSANITY;
    auto M = (uint16_t)RSK_SHUFFLE_MAPANDCOMPASS; auto G = (uint16_t)RSK_GANONS_BOSS_KEY;
    auto R = (uint16_t)RSK_SHUFFLE_DUNGEON_REWARDS;
    // Base = everything vanilla; each case overrides one axis (others vanilla) for isolation.
    auto base = [&](std::vector<SO> extra) {
        std::vector<SO> ov = { {K,(uint8_t)RO_DUNGEON_ITEM_LOC_VANILLA},
                               {B,(uint8_t)RO_DUNGEON_ITEM_LOC_VANILLA},
                               {M,(uint8_t)RO_DUNGEON_ITEM_LOC_VANILLA},
                               {G,(uint8_t)RO_GANON_BOSS_KEY_VANILLA},
                               {R,(uint8_t)RO_DUNGEON_REWARDS_VANILLA} };
        for (auto& e : extra) ov.push_back(e);
        return ov;
    };

    fail += check("small keys: own dungeon",  base({ {K,(uint8_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON} }));
    fail += check("small keys: any dungeon",  base({ {K,(uint8_t)RO_DUNGEON_ITEM_LOC_ANY_DUNGEON} }));
    fail += check("small keys: overworld",    base({ {K,(uint8_t)RO_DUNGEON_ITEM_LOC_OVERWORLD} }));
    fail += check("small keys: anywhere",     base({ {K,(uint8_t)RO_DUNGEON_ITEM_LOC_ANYWHERE} }));
    fail += check("boss keys: own dungeon",   base({ {B,(uint8_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON} }));
    fail += check("boss keys: anywhere",      base({ {B,(uint8_t)RO_DUNGEON_ITEM_LOC_ANYWHERE} }));
    fail += check("map/compass: own dungeon", base({ {M,(uint8_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON} }));
    fail += check("ganon bk: own dungeon",    base({ {G,(uint8_t)RO_GANON_BOSS_KEY_OWN_DUNGEON} }));
    fail += check("ganon bk: anywhere",       base({ {G,(uint8_t)RO_GANON_BOSS_KEY_ANYWHERE} }));
    fail += check("rewards: end of dungeon",  base({ {R,(uint8_t)RO_DUNGEON_REWARDS_END_OF_DUNGEON} }));
    fail += check("rewards: own dungeon",     base({ {R,(uint8_t)RO_DUNGEON_REWARDS_OWN_DUNGEON} }));
    fail += check("rewards: anywhere",        base({ {R,(uint8_t)RO_DUNGEON_REWARDS_ANYWHERE} }));

    // Fold checks: Start-With -> Vanilla; Ganon-BK LACS/100GS -> Own Dungeon.
    {
        Fill::Result r = Fill::Generate(12345, 2, base({ {K,(uint8_t)RO_DUNGEON_ITEM_LOC_STARTWITH} }));
        bool ok = r.settings[(size_t)RSK_KEYSANITY] == RO_DUNGEON_ITEM_LOC_VANILLA;
        std::printf("small keys: start-with -> synced=%u (want Vanilla=%u): %s\n",
                    r.settings[(size_t)RSK_KEYSANITY], (unsigned)RO_DUNGEON_ITEM_LOC_VANILLA,
                    ok ? "OK" : "FAIL");
        if (!ok || !r.beatable) ++fail;
    }
    {
        Fill::Result r = Fill::Generate(12345, 2, base({ {G,(uint8_t)RO_GANON_BOSS_KEY_LACS_MEDALLIONS} }));
        bool ok = r.settings[(size_t)RSK_GANONS_BOSS_KEY] == RO_GANON_BOSS_KEY_OWN_DUNGEON;
        std::printf("ganon bk: LACS -> synced=%u (want Own=%u): %s\n",
                    r.settings[(size_t)RSK_GANONS_BOSS_KEY], (unsigned)RO_GANON_BOSS_KEY_OWN_DUNGEON,
                    ok ? "OK" : "FAIL");
        if (!ok || !r.beatable) ++fail;
    }

    // Heaviest: every dungeon-item axis Own Dungeon (rewards end-of-dungeon) together.
    fail += check("ALL own-dungeon + rewards EoD", {
        {K,(uint8_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON}, {B,(uint8_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON},
        {M,(uint8_t)RO_DUNGEON_ITEM_LOC_OWN_DUNGEON}, {G,(uint8_t)RO_GANON_BOSS_KEY_OWN_DUNGEON},
        {R,(uint8_t)RO_DUNGEON_REWARDS_END_OF_DUNGEON} });
    // Full keysanity: everything Anywhere.
    fail += check("ALL anywhere", {
        {K,(uint8_t)RO_DUNGEON_ITEM_LOC_ANYWHERE}, {B,(uint8_t)RO_DUNGEON_ITEM_LOC_ANYWHERE},
        {M,(uint8_t)RO_DUNGEON_ITEM_LOC_ANYWHERE}, {G,(uint8_t)RO_GANON_BOSS_KEY_ANYWHERE},
        {R,(uint8_t)RO_DUNGEON_REWARDS_ANYWHERE} });

    std::printf(fail ? "\n%d CHECK(S) FAILED\n" : "\nALL DUNGEON-ITEM CHECKS PASSED\n", fail);
    return fail ? 1 : 0;
}
