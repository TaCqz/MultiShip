// GameShim — stand-ins for the z64 game constants & SaveContext layout that the ported
// logic.cpp reads/writes. Values are arbitrary-but-consistent: logic.cpp goes through
// the accessors below, so only internal consistency matters, not the real game layout.
#pragma once

#include "randomizerEnums.h"
#include "SceneIds.h"
#include <cstdint>
#include <array>

// --- item ids (CheckInventory/SetInventory/GetAmmo operate on these; identity slots) ---
enum : uint32_t {
    ITEM_NONE = 0,
    ITEM_STICK, ITEM_NUT, ITEM_BOMB, ITEM_BOW, ITEM_ARROW_FIRE, ITEM_DINS_FIRE,
    ITEM_SLINGSHOT, ITEM_OCARINA_FAIRY, ITEM_OCARINA_TIME, ITEM_BOMBCHU,
    ITEM_HOOKSHOT, ITEM_LONGSHOT, ITEM_ARROW_ICE, ITEM_FARORES_WIND, ITEM_BOOMERANG,
    ITEM_LENS, ITEM_BEAN, ITEM_ARROW_LIGHT, ITEM_NAYRUS_LOVE, ITEM_HAMMER,
    ITEM_BOTTLE_EMPTY, ITEM_POTION_RED, ITEM_POTION_GREEN, ITEM_POTION_BLUE,
    ITEM_FAIRY, ITEM_FISH, ITEM_BLUE_FIRE, ITEM_BUG, ITEM_POE, ITEM_BIG_POE,
    ITEM_LETTER_RUTO, ITEM_MILK,
    ITEM_SHIM_COUNT
};
// dedicated bottle slots (outside the identity item range)
enum : uint32_t { SLOT_BOTTLE_1 = 60, SLOT_BOTTLE_2, SLOT_BOTTLE_3, SLOT_BOTTLE_4 };
inline constexpr uint32_t kInvItemsSize = 64;

// item type (replaces Item::GetItemType; table lives in ItemTypes.h)
enum ItemType {
    ITEMTYPE_ITEM, ITEMTYPE_EQUIP, ITEMTYPE_DUNGEONREWARD, ITEMTYPE_SONG, ITEMTYPE_MAP,
    ITEMTYPE_COMPASS, ITEMTYPE_BOSSKEY, ITEMTYPE_SMALLKEY, ITEMTYPE_FORTRESS_SMALLKEY,
    ITEMTYPE_TOKEN, ITEMTYPE_EVENT, ITEMTYPE_REFILL, ITEMTYPE_DROP, ITEMTYPE_SHOP
};

// --- upgrades (packed into inventory.upgrades; 3 bits each) ---
enum : uint32_t {
    UPG_STRENGTH = 0, UPG_BOMB_BAG, UPG_QUIVER, UPG_BULLET_BAG,
    UPG_WALLET, UPG_SCALE, UPG_NUTS, UPG_STICKS, UPG_SHIM_COUNT
};
inline const uint32_t gUpgradeShifts[UPG_SHIM_COUNT] = { 0, 3, 6, 9, 12, 15, 18, 21 };
inline const uint32_t gUpgradeMasks[UPG_SHIM_COUNT] = {
    0x7u<<0, 0x7u<<3, 0x7u<<6, 0x7u<<9, 0x7u<<12, 0x7u<<15, 0x7u<<18, 0x7u<<21 };
inline const uint32_t gUpgradeNegMasks[UPG_SHIM_COUNT] = {
    ~(0x7u<<0), ~(0x7u<<3), ~(0x7u<<6), ~(0x7u<<9),
    ~(0x7u<<12), ~(0x7u<<15), ~(0x7u<<18), ~(0x7u<<21) };

// --- equipment flags (distinct bits) ---
enum : uint32_t {
    EQUIP_FLAG_SWORD_KOKIRI = 1u<<0, EQUIP_FLAG_SWORD_MASTER = 1u<<1,
    EQUIP_FLAG_SWORD_BGS = 1u<<2, EQUIP_FLAG_SHIELD_DEKU = 1u<<3,
    EQUIP_FLAG_SHIELD_HYLIAN = 1u<<4, EQUIP_FLAG_SHIELD_MIRROR = 1u<<5,
    EQUIP_FLAG_TUNIC_GORON = 1u<<6, EQUIP_FLAG_TUNIC_ZORA = 1u<<7,
    EQUIP_FLAG_BOOTS_IRON = 1u<<8, EQUIP_FLAG_BOOTS_HOVER = 1u<<9,
};

// --- quest items (bit indices into inventory.questItems) ---
enum : uint32_t {
    QUEST_MEDALLION_FOREST = 0, QUEST_MEDALLION_FIRE, QUEST_MEDALLION_WATER,
    QUEST_MEDALLION_SPIRIT, QUEST_MEDALLION_SHADOW, QUEST_MEDALLION_LIGHT,
    QUEST_SONG_MINUET, QUEST_SONG_BOLERO, QUEST_SONG_SERENADE, QUEST_SONG_REQUIEM,
    QUEST_SONG_NOCTURNE, QUEST_SONG_PRELUDE, QUEST_SONG_LULLABY, QUEST_SONG_EPONA,
    QUEST_SONG_SARIA, QUEST_SONG_SUN, QUEST_SONG_TIME, QUEST_SONG_STORMS,
    QUEST_KOKIRI_EMERALD, QUEST_GORON_RUBY, QUEST_ZORA_SAPPHIRE,
    QUEST_STONE_OF_AGONY, QUEST_GERUDO_CARD,
};
enum : uint32_t { QUEST_NORMAL = 0, QUEST_MASTER = 1 };

// --- dungeon item kinds (bit index via gBitFlags) ---
enum : uint32_t { DUNGEON_MAP = 0, DUNGEON_COMPASS = 1, DUNGEON_KEY_BOSS = 2 };
inline const uint32_t gBitFlags[8] = { 1u<<0, 1u<<1, 1u<<2, 1u<<3, 1u<<4, 1u<<5, 1u<<6, 1u<<7 };

// --- event flags (only referenced by DungeonCount's never-taken in-game branch) ---
enum : int32_t {
    EVENTCHKINF_USED_DEKU_TREE_BLUE_WARP = 0x10, EVENTCHKINF_USED_DODONGOS_CAVERN_BLUE_WARP = 0x11,
    EVENTCHKINF_USED_JABU_JABUS_BELLY_BLUE_WARP = 0x12, EVENTCHKINF_USED_FOREST_TEMPLE_BLUE_WARP = 0x13,
    EVENTCHKINF_USED_FIRE_TEMPLE_BLUE_WARP = 0x14, EVENTCHKINF_USED_WATER_TEMPLE_BLUE_WARP = 0x15,
};

// gItemSlots: mostly identity, but progressive items that REPLACE one another in a
// single inventory slot must share that slot (else CheckInventory reads the wrong slot
// after SetInventory writes the upgraded id) — Hookshot/Longshot and the two Ocarinas.
inline const std::array<uint32_t, ITEM_SHIM_COUNT> gItemSlots = [] {
    std::array<uint32_t, ITEM_SHIM_COUNT> a{};
    for (uint32_t i = 0; i < ITEM_SHIM_COUNT; ++i) a[i] = i;
    a[ITEM_LONGSHOT] = ITEM_HOOKSHOT;          // hookshot slot holds NONE/HOOKSHOT/LONGSHOT
    a[ITEM_OCARINA_TIME] = ITEM_OCARINA_FAIRY; // ocarina slot holds NONE/FAIRY/TIME
    return a;
}();

namespace Rando {

struct ShimInventory {
    uint8_t  items[kInvItemsSize] = {};
    int8_t   ammo[kInvItemsSize] = {};
    uint32_t equipment = 0;
    uint32_t upgrades = 0;
    uint32_t questItems = 0;
    uint8_t  dungeonItems[SCENE_ID_MAX_SHIM] = {};
    int8_t   dungeonKeys[SCENE_ID_MAX_SHIM];   // init to -1 in NewSaveContext
    uint16_t gsTokens = 0;
    uint16_t defenseHearts = 0;
};

struct SaveContext {
    ShimInventory inventory;
    uint8_t  magicLevel = 0;
    int16_t  magic = 0;
    bool     isMagicAcquired = false;
    bool     isDoubleMagicAcquired = false;
    bool     isDoubleDefenseAcquired = false;
    uint8_t  bgsFlag = 0;
    int16_t  bgsDayCount = 0;
    uint16_t healthCapacity = 0;
    uint16_t health = 0;
    uint8_t  swordHealth = 0;
    uint8_t  linkAge = 0;
    struct {
        uint16_t randomizerInf[(RAND_INF_MAX >> 4) + 1] = {};
        struct { struct { struct { int triforcePiecesCollected = 0; } randomizer; } data; } quest;
    } ship;
    uint16_t eventChkInf[64] = {};
};

} // namespace Rando
