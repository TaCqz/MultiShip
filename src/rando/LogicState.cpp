// CollectItem — ported from SoH logic.cpp's ApplyItemEffect, keyed on RandomizerGet
// instead of Item/GIEntry (the only GIEntry uses were stick/nut object detection,
// per-item itemIds, and infinite-upgrade variants; the latter are off under default
// settings and dropped). Also the save-context (re)init helpers.
#include "rando/Logic.h"
#include "rando/Context.h"
#include "rando/ItemTypes.h"
#include <cstring>

namespace Rando {

// free lookup tables defined verbatim in Logic.cpp (inside namespace Rando)
extern uint32_t HookshotLookup[3];
extern uint32_t OcarinaLookup[3];
extern std::map<uint32_t, uint32_t> BottleRandomizerGetToItemID;

void Logic::CollectItem(RandomizerGet rg, bool state) {
    auto randoGet = rg;
    auto it = kItemType.find(rg);
    if (it == kItemType.end()) {
        return;
    }
    switch (it->second) {
        case ITEMTYPE_ITEM: {
            switch (randoGet) {
                case RG_STONE_OF_AGONY:
                case RG_GERUDO_MEMBERSHIP_CARD:
                    SetQuestItem(RandoGetToQuestItem.at(randoGet), state);
                    break;
                case RG_WEIRD_EGG:
                    SetRandoInf(RAND_INF_WEIRD_EGG, state);
                    break;
                case RG_ZELDAS_LETTER:
                    SetRandoInf(RAND_INF_ZELDAS_LETTER, state);
                    break;
                case RG_DOUBLE_DEFENSE:
                    mSaveContext->isDoubleDefenseAcquired = state;
                    break;
                case RG_POCKET_EGG:
                    SetRandoInf(RAND_INF_ADULT_TRADES_HAS_POCKET_EGG, state);
                    break;
                case RG_COJIRO:
                case RG_ODD_MUSHROOM:
                case RG_ODD_POTION:
                case RG_POACHERS_SAW:
                case RG_BROKEN_SWORD:
                case RG_PRESCRIPTION:
                case RG_EYEBALL_FROG:
                case RG_EYEDROPS:
                case RG_CLAIM_CHECK:
                    SetRandoInf(randoGet - RG_COJIRO + RAND_INF_ADULT_TRADES_HAS_COJIRO, state);
                    break;
                case RG_CLIMB:
                    SetRandoInf(RAND_INF_CAN_CLIMB, state);
                    break;
                case RG_CRAWL:
                    SetRandoInf(RAND_INF_CAN_CRAWL, state);
                    break;
                case RG_OPEN_CHEST:
                    SetRandoInf(RAND_INF_CAN_OPEN_CHEST, state);
                    break;
                case RG_PROGRESSIVE_HOOKSHOT: {
                    int i;
                    for (i = 0; i < 3; i++) {
                        if (CurrentInventory(ITEM_HOOKSHOT) == HookshotLookup[i]) break;
                    }
                    auto newItem = i + (!state ? -1 : 1);
                    if (newItem < 0) newItem = 0; else if (newItem > 2) newItem = 2;
                    SetInventory(ITEM_HOOKSHOT, HookshotLookup[newItem]);
                } break;
                case RG_PROGRESSIVE_STRENGTH: {
                    auto currentLevel = CurrentUpgrade(UPG_STRENGTH);
                    if (!CheckRandoInf(RAND_INF_CAN_GRAB) && state) {
                        SetRandoInf(RAND_INF_CAN_GRAB, true);
                    } else if (currentLevel == 0 && !state) {
                        SetRandoInf(RAND_INF_CAN_GRAB, false);
                    } else {
                        SetUpgrade(UPG_STRENGTH, currentLevel + (!state ? -1 : 1));
                    }
                } break;
                case RG_PROGRESSIVE_BOMB_BAG: {
                    auto currentLevel = CurrentUpgrade(UPG_BOMB_BAG);
                    if ((currentLevel == 0 && state) || (currentLevel == 1 && !state))
                        SetInventory(ITEM_BOMB, (!state ? ITEM_NONE : ITEM_BOMB));
                    SetUpgrade(UPG_BOMB_BAG, currentLevel + (!state ? -1 : 1));
                } break;
                case RG_PROGRESSIVE_BOW: {
                    auto currentLevel = CurrentUpgrade(UPG_QUIVER);
                    if ((currentLevel == 0 && state) || (currentLevel == 1 && !state))
                        SetInventory(ITEM_BOW, (!state ? ITEM_NONE : ITEM_BOW));
                    SetUpgrade(UPG_QUIVER, currentLevel + (!state ? -1 : 1));
                } break;
                case RG_PROGRESSIVE_SLINGSHOT: {
                    auto currentLevel = CurrentUpgrade(UPG_BULLET_BAG);
                    if ((currentLevel == 0 && state) || (currentLevel == 1 && !state))
                        SetInventory(ITEM_SLINGSHOT, (!state ? ITEM_NONE : ITEM_SLINGSHOT));
                    SetUpgrade(UPG_BULLET_BAG, currentLevel + (!state ? -1 : 1));
                } break;
                case RG_PROGRESSIVE_WALLET: {
                    auto currentLevel = CurrentUpgrade(UPG_WALLET);
                    if (!CheckRandoInf(RAND_INF_HAS_WALLET) && state) {
                        SetRandoInf(RAND_INF_HAS_WALLET, true);
                    } else if (currentLevel == 0 && !state) {
                        SetRandoInf(RAND_INF_HAS_WALLET, false);
                    } else {
                        SetUpgrade(UPG_WALLET, currentLevel + (!state ? -1 : 1));
                    }
                } break;
                case RG_PROGRESSIVE_SCALE: {
                    auto currentLevel = CurrentUpgrade(UPG_SCALE);
                    if (!CheckRandoInf(RAND_INF_CAN_SWIM) && state) {
                        SetRandoInf(RAND_INF_CAN_SWIM, true);
                    } else if (currentLevel == 0 && !state) {
                        SetRandoInf(RAND_INF_CAN_SWIM, false);
                    } else {
                        SetUpgrade(UPG_SCALE, currentLevel + (!state ? -1 : 1));
                    }
                } break;
                case RG_PROGRESSIVE_NUT_UPGRADE: {
                    auto currentLevel = CurrentUpgrade(UPG_NUTS);
                    if ((currentLevel == 0 && state) || (currentLevel == 1 && !state))
                        SetInventory(ITEM_NUT, (!state ? ITEM_NONE : ITEM_NUT));
                    SetUpgrade(UPG_NUTS, currentLevel + (!state ? -1 : 1));
                } break;
                case RG_PROGRESSIVE_STICK_UPGRADE: {
                    auto currentLevel = CurrentUpgrade(UPG_STICKS);
                    if ((currentLevel == 0 && state) || (currentLevel == 1 && !state))
                        SetInventory(ITEM_STICK, (!state ? ITEM_NONE : ITEM_STICK));
                    SetUpgrade(UPG_STICKS, currentLevel + (!state ? -1 : 1));
                } break;
                case RG_PROGRESSIVE_BOMBCHU_BAG:
                    SetInventory(ITEM_BOMBCHU, (!state ? ITEM_NONE : ITEM_BOMBCHU));
                    break;
                case RG_PROGRESSIVE_MAGIC_METER:
                    mSaveContext->magicLevel += (!state ? -1 : 1);
                    break;
                case RG_PROGRESSIVE_OCARINA: {
                    int i;
                    for (i = 0; i < 3; i++) {
                        if (CurrentInventory(ITEM_OCARINA_FAIRY) == OcarinaLookup[i]) break;
                    }
                    i += (!state ? -1 : 1);
                    if (i < 0) i = 0; else if (i > 2) i = 2;
                    SetInventory(ITEM_OCARINA_FAIRY, OcarinaLookup[i]);
                } break;
                case RG_HEART_CONTAINER:
                    mSaveContext->healthCapacity += (!state ? -16 : 16);
                    break;
                case RG_PIECE_OF_HEART:
                    mSaveContext->healthCapacity += (!state ? -4 : 4);
                    break;
                case RG_BOOMERANG:     SetInventory(ITEM_BOOMERANG,  !state ? ITEM_NONE : ITEM_BOOMERANG);  break;
                case RG_LENS_OF_TRUTH: SetInventory(ITEM_LENS,       !state ? ITEM_NONE : ITEM_LENS);       break;
                case RG_MEGATON_HAMMER:SetInventory(ITEM_HAMMER,     !state ? ITEM_NONE : ITEM_HAMMER);     break;
                case RG_DINS_FIRE:     SetInventory(ITEM_DINS_FIRE,  !state ? ITEM_NONE : ITEM_DINS_FIRE);  break;
                case RG_FARORES_WIND:  SetInventory(ITEM_FARORES_WIND,!state ? ITEM_NONE : ITEM_FARORES_WIND); break;
                case RG_NAYRUS_LOVE:   SetInventory(ITEM_NAYRUS_LOVE,!state ? ITEM_NONE : ITEM_NAYRUS_LOVE); break;
                case RG_FIRE_ARROWS:   SetInventory(ITEM_ARROW_FIRE, !state ? ITEM_NONE : ITEM_ARROW_FIRE); break;
                case RG_ICE_ARROWS:    SetInventory(ITEM_ARROW_ICE,  !state ? ITEM_NONE : ITEM_ARROW_ICE);  break;
                case RG_LIGHT_ARROWS:  SetInventory(ITEM_ARROW_LIGHT,!state ? ITEM_NONE : ITEM_ARROW_LIGHT);break;
                case RG_MAGIC_BEAN:
                case RG_MAGIC_BEAN_PACK: {
                    auto change = (randoGet == RG_MAGIC_BEAN ? 1 : 10);
                    SetAmmo(ITEM_BEAN, GetAmmo(ITEM_BEAN) + (!state ? -change : change));
                } break;
                case RG_EMPTY_BOTTLE:
                case RG_BOTTLE_WITH_MILK:
                case RG_BOTTLE_WITH_RED_POTION:
                case RG_BOTTLE_WITH_GREEN_POTION:
                case RG_BOTTLE_WITH_BLUE_POTION:
                case RG_BOTTLE_WITH_FAIRY:
                case RG_BOTTLE_WITH_FISH:
                case RG_BOTTLE_WITH_BLUE_FIRE:
                case RG_BOTTLE_WITH_BUGS:
                case RG_BOTTLE_WITH_POE:
                case RG_BOTTLE_WITH_BIG_POE: {
                    uint8_t slot = SLOT_BOTTLE_1;
                    while (slot != SLOT_BOTTLE_4) {
                        if (mSaveContext->inventory.items[slot] == ITEM_NONE) break;
                        slot++;
                    }
                    uint16_t itemId = ITEM_BOTTLE_EMPTY;
                    if (BottleRandomizerGetToItemID.contains(randoGet))
                        itemId = static_cast<uint16_t>(BottleRandomizerGetToItemID[randoGet]);
                    if (randoGet == RG_BOTTLE_WITH_BIG_POE) BigPoes++;
                    mSaveContext->inventory.items[slot] = static_cast<uint8_t>(itemId);
                } break;
                case RG_RUTOS_LETTER:
                    SetRandoInf(RAND_INF_OBTAINED_RUTOS_LETTER, state);
                    break;
                case RG_DEATH_MOUNTAIN_CRATER_BEAN_SOUL:
                case RG_DEATH_MOUNTAIN_TRAIL_BEAN_SOUL:
                case RG_DESERT_COLOSSUS_BEAN_SOUL:
                case RG_GERUDO_VALLEY_BEAN_SOUL:
                case RG_GRAVEYARD_BEAN_SOUL:
                case RG_KOKIRI_FOREST_BEAN_SOUL:
                case RG_LAKE_HYLIA_BEAN_SOUL:
                case RG_LOST_WOODS_BRIDGE_BEAN_SOUL:
                case RG_LOST_WOODS_BEAN_SOUL:
                case RG_ZORAS_RIVER_BEAN_SOUL:
                case RG_GOHMA_SOUL: case RG_KING_DODONGO_SOUL: case RG_BARINADE_SOUL:
                case RG_PHANTOM_GANON_SOUL: case RG_VOLVAGIA_SOUL: case RG_MORPHA_SOUL:
                case RG_BONGO_BONGO_SOUL: case RG_TWINROVA_SOUL: case RG_GANON_SOUL:
                case RG_OCARINA_A_BUTTON: case RG_OCARINA_C_UP_BUTTON: case RG_OCARINA_C_DOWN_BUTTON:
                case RG_OCARINA_C_LEFT_BUTTON: case RG_OCARINA_C_RIGHT_BUTTON:
                case RG_KEATON_MASK: case RG_SKULL_MASK: case RG_SPOOKY_MASK: case RG_BUNNY_HOOD:
                case RG_GORON_MASK: case RG_ZORA_MASK: case RG_GERUDO_MASK: case RG_MASK_OF_TRUTH:
                case RG_GREG_RUPEE:
                case RG_SPEAK_DEKU: case RG_SPEAK_GERUDO: case RG_SPEAK_GORON:
                case RG_SPEAK_HYLIAN: case RG_SPEAK_KOKIRI: case RG_SPEAK_ZORA:
                case RG_FISHING_POLE:
                case RG_GUARD_HOUSE_KEY: case RG_MARKET_BAZAAR_KEY: case RG_MARKET_POTION_SHOP_KEY:
                case RG_MASK_SHOP_KEY: case RG_MARKET_SHOOTING_GALLERY_KEY: case RG_BOMBCHU_BOWLING_KEY:
                case RG_TREASURE_CHEST_GAME_BUILDING_KEY: case RG_BOMBCHU_SHOP_KEY: case RG_RICHARDS_HOUSE_KEY:
                case RG_ALLEY_HOUSE_KEY: case RG_KAK_BAZAAR_KEY: case RG_KAK_POTION_SHOP_KEY:
                case RG_BOSS_HOUSE_KEY: case RG_GRANNYS_POTION_SHOP_KEY: case RG_SKULLTULA_HOUSE_KEY:
                case RG_IMPAS_HOUSE_KEY: case RG_WINDMILL_KEY: case RG_KAK_SHOOTING_GALLERY_KEY:
                case RG_DAMPES_HUT_KEY: case RG_TALONS_HOUSE_KEY: case RG_STABLES_KEY:
                case RG_BACK_TOWER_KEY: case RG_HYLIA_LAB_KEY: case RG_FISHING_HOLE_KEY:
                    SetRandoInf(RandoGetToRandInf.at(randoGet), state);
                    break;
                case RG_TRIFORCE_PIECE:
                    mSaveContext->ship.quest.data.randomizer.triforcePiecesCollected += (!state ? -1 : 1);
                    break;
                case RG_BOMBCHU_5: case RG_BOMBCHU_10: case RG_BOMBCHU_20:
                    SetInventory(ITEM_BOMBCHU, (!state ? ITEM_NONE : ITEM_BOMBCHU));
                    break;
                default:
                    break;
            }
        } break;
        case ITEMTYPE_EQUIP: {
            RandomizerGet itemRG = randoGet;
            switch (itemRG) {
                case RG_DEKU_SHIELD:   SetRandoInf(RAND_INF_HAS_FOUND_DEKU_SHIELD, state);   break;
                case RG_HYLIAN_SHIELD: SetRandoInf(RAND_INF_HAS_FOUND_HYLIAN_SHIELD, state); break;
                case RG_GORON_TUNIC:   SetRandoInf(RAND_INF_HAS_FOUND_GORON_TUNIC, state);   break;
                case RG_ZORA_TUNIC:    SetRandoInf(RAND_INF_HAS_FOUND_ZORA_TUNIC, state);    break;
                default: break;
            }
            if (itemRG == RG_DEKU_SHIELD || itemRG == RG_HYLIAN_SHIELD) return;
            uint32_t equipId = RandoGetToEquipFlag.find(itemRG)->second;
            if (!state) {
                mSaveContext->inventory.equipment &= ~equipId;
                if (equipId == EQUIP_FLAG_SWORD_BGS && itemRG != RG_GIANTS_KNIFE) mSaveContext->bgsFlag = false;
            } else {
                mSaveContext->inventory.equipment |= equipId;
                if (equipId == EQUIP_FLAG_SWORD_BGS && itemRG != RG_GIANTS_KNIFE) mSaveContext->bgsFlag = true;
            }
        } break;
        case ITEMTYPE_DUNGEONREWARD:
        case ITEMTYPE_SONG:
            SetQuestItem(RandoGetToQuestItem.find(randoGet)->second, state);
            break;
        case ITEMTYPE_MAP:
            SetDungeonItem(DUNGEON_MAP, RandoGetToDungeonScene.find(randoGet)->second, state);
            break;
        case ITEMTYPE_COMPASS:
            SetDungeonItem(DUNGEON_COMPASS, RandoGetToDungeonScene.find(randoGet)->second, state);
            break;
        case ITEMTYPE_BOSSKEY:
            SetDungeonItem(DUNGEON_KEY_BOSS, RandoGetToDungeonScene.find(randoGet)->second, state);
            break;
        case ITEMTYPE_FORTRESS_SMALLKEY:
        case ITEMTYPE_SMALLKEY: {
            auto keyring = randoGet >= RG_FOREST_TEMPLE_KEY_RING && randoGet <= RG_GANONS_CASTLE_KEY_RING;
            auto dungeonIndex = RandoGetToDungeonScene.find(randoGet)->second;
            auto count = GetSmallKeyCount(dungeonIndex);
            if (!state) count = keyring ? 0 : count - 1;
            else        count = keyring ? 10 : count + 1;
            SetSmallKeyCount(dungeonIndex, count);
        } break;
        case ITEMTYPE_TOKEN:
            mSaveContext->inventory.gsTokens += (!state ? -1 : 1);
            break;
        case ITEMTYPE_EVENT:
            break;
        case ITEMTYPE_DROP:
        case ITEMTYPE_REFILL:
        case ITEMTYPE_SHOP: {
            RandomizerGet itemRG = randoGet;
            if (itemRG == RG_BUY_HYLIAN_SHIELD || itemRG == RG_BUY_DEKU_SHIELD ||
                itemRG == RG_BUY_GORON_TUNIC || itemRG == RG_BUY_ZORA_TUNIC) {
                uint32_t equipId = RandoGetToEquipFlag.find(itemRG)->second;
                if (!state) mSaveContext->inventory.equipment &= ~equipId;
                else        mSaveContext->inventory.equipment |= equipId;
            }
            switch (itemRG) {
                case RG_DEKU_NUTS_5: case RG_DEKU_NUTS_10:
                case RG_BUY_DEKU_NUTS_5: case RG_BUY_DEKU_NUTS_10:
                    SetInventory(ITEM_NUT, (!state ? ITEM_NONE : ITEM_NUT)); break;
                case RG_DEKU_STICK_1: case RG_BUY_DEKU_STICK_1: case RG_STICKS:
                    SetInventory(ITEM_STICK, (!state ? ITEM_NONE : ITEM_STICK)); break;
                case RG_BOMBCHU_5: case RG_BOMBCHU_10: case RG_BOMBCHU_20:
                    SetInventory(ITEM_BOMBCHU, (!state ? ITEM_NONE : ITEM_BOMBCHU)); break;
                default: break;
            }
        } break;
    }
}

void Logic::NewSaveContext() {
    mSaveContext = &mSaveCtxStorage;
    *mSaveContext = SaveContext{};
    for (auto& k : mSaveContext->inventory.dungeonKeys) k = -1;
}

void Logic::InitSaveContext() {
    NewSaveContext();
}

} // namespace Rando
