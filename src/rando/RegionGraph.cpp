// RegionGraph.cpp — globals + free helpers ported from SoH location_access.cpp:
// areaTable, the logic/ctx globals, the Spirit Temple shared-access data + helpers,
// AnyAgeTime/CanPlantBean/BothAges/etc., and RegionTable_Init() which builds the graph.
#include "rando/Graph.h"
#include "rando/Logic.h"
#include "rando/Context.h"
#include <memory>

// --- globals the rule-file lambdas and logic reference ---
std::array<Region, RR_MAX> areaTable;
std::vector<EventAccess> grottoEvents;
std::shared_ptr<Rando::Logic> logic;
Rando::Context* ctx;

Region* RegionTable(RandomizerRegion key) { return &areaTable[key]; }

// Shop wallet/price helpers. Shopsanity is off by default, so prices are vanilla;
// for v1 reachability treat shop items as affordable (price 0) — TODO(parity): real prices.
uint16_t GetWalletCapacity() {
    switch (logic->CurrentUpgrade(UPG_WALLET)) {
        case 0:  return 99;
        case 1:  return 200;
        case 2:  return 500;
        default: return 999;
    }
}
uint16_t GetCheckPrice(RandomizerCheck) { return 0; }

// --- Region methods that need logic/ctx (declared in Graph.h) ---
bool Region::AnyAgeTime(ConditionFn condition) const {
    bool pastAdult = logic->IsAdult;
    bool pastChild = logic->IsChild;
    logic->IsChild = Child();
    logic->IsAdult = Adult();
    bool hereVal = condition() && (logic->IsAdult || logic->IsChild);
    logic->IsChild = pastChild;
    logic->IsAdult = pastAdult;
    return hereVal;
}

bool Region::CanPlantBeanCheck(RandomizerGet bean) const {
    return logic->HasItem(bean) && logic->GetAmmo(ITEM_BEAN) > 0 &&
           (ctx->GetOption(RSK_SKIP_PLANTING_BEANS) || BothAgesCheck());
}

// Spirit Temple shared-age key access data (verbatim from SoH location_access.cpp).
std::map<RandomizerRegion, SpiritLogicData> Region::spiritLogicData = {
    {RR_SPIRIT_TEMPLE_SUN_ON_FLOOR_1F,       {5, 0, 3, 0,
                                                 []{return true;},
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS);},
                                             }},
    {RR_SPIRIT_TEMPLE_SUN_ON_FLOOR_2F,       {5, 0, 3, 0,
                                                 []{return logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT);},
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS);},
                                             }},
    {RR_SPIRIT_TEMPLE_2F_MIRROR_ROOM,        {5, 0, 3, 0,
                                                 []{return false;},
                                                 []{return logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT);},
                                                 []{return logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS);},
                                             }},
    {RR_SPIRIT_TEMPLE_STATUE_ROOM_CHILD,     {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS);},
                                             }},
    {RR_SPIRIT_TEMPLE_INNER_WEST_HAND,       {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS);},
                                             }},
    {RR_SPIRIT_TEMPLE_GS_LEDGE,              {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic() && logic->SpiritWestToSkull() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return logic->SpiritExplosiveKeyLogic() && logic->SpiritWestToSkull() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->SpiritWestToSkull() && ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) || logic->CanUse(RG_HOVER_BOOTS));},
                                             }},
    {RR_SPIRIT_TEMPLE_STATUE_ROOM,           {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic();},
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return true;},
                                             }},
    {RR_SPIRIT_TEMPLE_SUN_BLOCK_CHEST_LEDGE, {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return (((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) || logic->CanUse(RG_HOVER_BOOTS)) && logic->HasItem(RG_POWER_BRACELET)) || (logic->CanKillEnemy(RE_BEAMOS) && logic->CanUse(RG_LONGSHOT));},
                                             }},
    {RR_SPIRIT_TEMPLE_SKULLTULA_STAIRS,      {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->SpiritExplosiveKeyLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return (((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) || logic->CanUse(RG_HOVER_BOOTS)) && logic->HasItem(RG_POWER_BRACELET)) || (logic->CanKillEnemy(RE_BEAMOS) && logic->CanUse(RG_LONGSHOT));},
                                             }},
    {RR_SPIRIT_TEMPLE_OUTER_RIGHT_HAND,      {5, 5, 3, 3,
                                                 []{return logic->OuterWestHandLogic();},
                                                 []{return logic->OuterWestHandLogic();},
                                                 []{return logic->OuterWestHandLogic();},
                                             }},
    {RR_SPIRIT_TEMPLE_STATUE_ROOM_ADULT,     {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic() && logic->CanUse(RG_HOOKSHOT) && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS);},
                                             }},
    {RR_SPIRIT_TEMPLE_INNER_LEFT_HAND,       {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic() && logic->CanUse(RG_HOOKSHOT) && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS);},
                                             }},
    {RR_SPIRIT_TEMPLE_SHORTCUT_SWITCH,       {5, 0, 3, 0,
                                                 []{return logic->SpiritExplosiveKeyLogic() && logic->CanUse(RG_HOOKSHOT) && logic->SpiritEastToSwitch();},
                                                 []{return logic->SpiritEastToSwitch() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->SpiritEastToSwitch() && (logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS));}}},
    {RR_SPIRIT_TEMPLE_MQ_UNDER_LIKE_LIKE,    {7, 6, 7, 7,
                                                 []{return logic->StatueRoomMQKeyLogic();},
                                                 []{return logic->SmallKeys(SCENE_SPIRIT_TEMPLE, 6) && logic->CanHitSwitch() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT));},
                                                 []{return logic->StatueRoomMQKeyLogic() && logic->CanHitSwitch() && ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) || logic->CanUse(RG_HOVER_BOOTS));},
                                             }},
    {RR_SPIRIT_TEMPLE_MQ_SUN_ON_FLOOR,       {7, 6, 7, 7,
                                                 []{return logic->StatueRoomMQKeyLogic() && logic->CanHitSwitch() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return logic->SmallKeys(SCENE_SPIRIT_TEMPLE, 6) && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT));},
                                                 []{return logic->StatueRoomMQKeyLogic() && ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) || logic->CanUse(RG_HOVER_BOOTS));},
                                             }},
    {RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM_CHILD,  {7, 0, 0, 0,
                                                 []{return logic->CanHitSwitch() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT);},
                                                 []{return logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS);},
                                             }},
    {RR_SPIRIT_TEMPLE_MQ_POT_LEDGE,          {7, 0, 0, 0,
                                                 []{return logic->CanHitSwitch() && logic->MQSpiritWestToPots() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return logic->MQSpiritWestToPots() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT));},
                                                 []{return logic->CanUse(RG_HOVER_BOOTS) || ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) && logic->MQSpiritWestToPots());},
                                             }},
    {RR_SPIRIT_TEMPLE_MQ_INNER_RIGHT_HAND,   {7, 0, 0, 0,
                                                 []{return logic->CanHitSwitch() && logic->MQSpiritWestToPots() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return logic->MQSpiritWestToPots() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT));},
                                                 []{return logic->CanUse(RG_HOVER_BOOTS) || ((logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT)) && logic->MQSpiritWestToPots());},
                                             }},
    {RR_SPIRIT_TEMPLE_MQ_STATUE_ROOM,        {7, 0, 0, 0,
                                                 []{return logic->CanHitSwitch() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return true;},
                                                 []{return true;},
                                             }},
    {RR_SPIRIT_TEMPLE_MQ_SUN_BLOCK_ROOM,     {7, 0, 0, 0,
                                                 []{return logic->CanHitSwitch() && logic->MQSpiritStatueToSunBlock() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT));},
                                                 []{return logic->MQSpiritStatueToSunBlock() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT));},
                                                 []{return logic->MQSpiritStatueToSunBlock() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_HOOKSHOT) || logic->CanUse(RG_HOVER_BOOTS));},
                                             }},
    {RR_SPIRIT_TEMPLE_MQ_OUTER_RIGHT_HAND,   {7, 7, 4, 4,
                                                 []{return logic->CanHitSwitch() && logic->OuterWestHandMQLogic() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && logic->HasItem(RG_POWER_BRACELET);},
                                                 []{return logic->OuterWestHandMQLogic();},
                                                 []{return logic->OuterWestHandMQLogic();},
                                             }},
    {RR_SPIRIT_TEMPLE_MQ_BIG_BLOCKS_DOOR,    {7, 0, 0, 0,
                                                 []{return logic->CanHitSwitch() && (logic->HasItem(RG_CLIMB) || logic->CanUse(RG_LONGSHOT)) && areaTable[RR_SPIRIT_TEMPLE_MQ_BIG_BLOCKS_DOOR].AnyAgeTime([]{return logic->MQSpiritStatueSouthDoor();});},
                                                 []{return true;},
                                                 []{return areaTable[RR_SPIRIT_TEMPLE_MQ_BIG_BLOCKS_DOOR].AnyAgeTime([]{return logic->MQSpiritStatueSouthDoor();});},
                                             }},
};

bool SpiritCertainAccess(RandomizerRegion region) {
    SpiritLogicData& curRegionData = Region::spiritLogicData[region];
    if (logic->IsChild) {
        uint8_t keys = curRegionData.childKeys;
        uint8_t revKeys = curRegionData.childRevKeys;
        bool knownFrontAccess = logic->Get(LOGIC_FORWARDS_SPIRIT_CHILD) || !logic->IsReverseAccessPossible();
        return ((knownFrontAccess && curRegionData.childAccess()) && logic->SmallKeys(SCENE_SPIRIT_TEMPLE, keys)) ||
               ((logic->Get(LOGIC_REVERSE_SPIRIT_CHILD) && curRegionData.reverseAccess()) &&
                logic->SmallKeys(SCENE_SPIRIT_TEMPLE, revKeys)) ||
               (curRegionData.childAccess() && curRegionData.reverseAccess() &&
                logic->SmallKeys(SCENE_SPIRIT_TEMPLE, keys > revKeys ? keys : revKeys));
    } else {
        uint8_t keys = curRegionData.adultKeys;
        uint8_t revKeys = curRegionData.adultRevKeys;
        bool knownFrontAccess = logic->Get(LOGIC_FORWARDS_SPIRIT_ADULT) || !logic->IsReverseAccessPossible();
        return ((knownFrontAccess && curRegionData.adultAccess()) && logic->SmallKeys(SCENE_SPIRIT_TEMPLE, keys)) ||
               ((logic->Get(LOGIC_REVERSE_SPIRIT_ADULT) && curRegionData.reverseAccess()) &&
                logic->SmallKeys(SCENE_SPIRIT_TEMPLE, revKeys)) ||
               (curRegionData.adultAccess() && curRegionData.reverseAccess() &&
                logic->SmallKeys(SCENE_SPIRIT_TEMPLE, keys > revKeys ? keys : revKeys));
    }
}

bool SpiritShared(RandomizerRegion region, ConditionFn condition, bool anyAge, RandomizerRegion otherRegion,
                  ConditionFn otherCondition, RandomizerRegion thirdRegion, ConditionFn thirdCondition) {
    SpiritLogicData& curRegionData = Region::spiritLogicData[region];
    bool result = false;
    bool pastAdult = logic->IsAdult;
    bool pastChild = logic->IsChild;

    logic->IsChild = true;  logic->IsAdult = false;
    bool ChildCertainAccess = SpiritCertainAccess(region);
    logic->IsChild = false; logic->IsAdult = true;
    bool AdultCertainAccess = SpiritCertainAccess(region);

    if (anyAge && (ChildCertainAccess || AdultCertainAccess)) {
        logic->IsChild = ChildCertainAccess;
        logic->IsAdult = AdultCertainAccess;
        result = condition();
    } else if (areaTable[region].Child() && pastChild) {
        logic->IsChild = true; logic->IsAdult = false;
        result = condition();
        if (!ChildCertainAccess && result) {
            logic->IsChild = false; logic->IsAdult = true;
            result = (curRegionData.adultAccess() &&
                      (!logic->IsReverseAccessPossible() || curRegionData.reverseAccess()) && condition()) ||
                     (otherRegion != RR_NONE &&
                      (Region::spiritLogicData[otherRegion].adultAccess() &&
                       (!logic->IsReverseAccessPossible() || Region::spiritLogicData[otherRegion].reverseAccess()) &&
                       otherCondition())) ||
                     (thirdRegion != RR_NONE &&
                      (Region::spiritLogicData[thirdRegion].adultAccess() &&
                       (!logic->IsReverseAccessPossible() || Region::spiritLogicData[thirdRegion].reverseAccess()) &&
                       thirdCondition()));
        }
    } else if (areaTable[region].Adult() && pastAdult) {
        result = condition();
        if (!AdultCertainAccess && result) {
            logic->IsChild = true; logic->IsAdult = false;
            result = (curRegionData.childAccess() &&
                      (!logic->IsReverseAccessPossible() || curRegionData.reverseAccess()) && condition()) ||
                     (otherRegion != RR_NONE &&
                      (Region::spiritLogicData[otherRegion].childAccess() &&
                       (!logic->IsReverseAccessPossible() || Region::spiritLogicData[otherRegion].reverseAccess()) &&
                       otherCondition())) ||
                     (thirdRegion != RR_NONE &&
                      (Region::spiritLogicData[thirdRegion].childAccess() &&
                       (!logic->IsReverseAccessPossible() || Region::spiritLogicData[thirdRegion].reverseAccess()) &&
                       thirdCondition()));
        }
    }
    logic->IsChild = pastChild;
    logic->IsAdult = pastAdult;
    return result;
}

bool AnyAgeTime(ConditionFn condition) {
    return areaTable[logic->CurrentRegionKey].AnyAgeTime(condition);
}

// Generation has no playthrough scene flags, so a planted bean only counts when the
// player starts with planting skipped + starting beans.
bool BeanPlanted(RandomizerGet bean) {
    if (!logic->HasItem(bean)) return false;
    if (ctx->GetOption(RSK_SKIP_PLANTING_BEANS) && ctx->GetOption(RSK_STARTING_BEANS)) return true;
    return false;
}

bool CanPlantBean(RandomizerRegion region, RandomizerGet bean) {
    return areaTable[region].CanPlantBeanCheck(bean) || BeanPlanted(bean);
}
bool BothAges(RandomizerRegion region)      { return areaTable[region].BothAgesCheck(); }
bool ChildCanAccess(RandomizerRegion region){ return areaTable[region].Child(); }
bool AdultCanAccess(RandomizerRegion region){ return areaTable[region].Adult(); }

static std::shared_ptr<Rando::Context> gCtx;
static std::shared_ptr<Rando::Logic> gLogic0;

// Create an additional world's Logic, sharing the one settings Context. Each world
// has its own inventory/events but evaluates the same (settings-identical) graph.
std::shared_ptr<Rando::Logic> NewWorldLogic() {
    auto l = std::make_shared<Rando::Logic>();
    l->SetContext(gCtx);
    l->Reset();
    return l;
}

void RegionTable_Init() {
    if (!gCtx) gCtx = std::make_shared<Rando::Context>();
    if (!gLogic0) gLogic0 = std::make_shared<Rando::Logic>();
    // F-042 gCtx reset: the settings Context is a singleton that persists across
    // Fill::Generate calls in one process. Re-baseline it every init so option overrides
    // + the normalizations Fill applies to one seed can't leak into the next. (On the very
    // first call this just re-applies the defaults the constructor already set — a no-op.)
    gCtx->ResetToDefaults();
    ctx = gCtx.get();
    logic = gLogic0;
    logic->SetContext(gCtx);
    logic->Reset();

    grottoEvents = {
        EVENT_ACCESS(LOGIC_FAIRY_ACCESS, logic->CallGossipFairy() || logic->CanUse(RG_STICKS)),
        EVENT_ACCESS(LOGIC_BUG_ACCESS, logic->CanCutShrubs()),
        EVENT_ACCESS(LOGIC_FISH_ACCESS, true),
    };
    areaTable.fill(Region("Invalid Region", SCENE_ID_MAX_SHIM, {}, {}, {}));

    RegionTable_Init_Root();
    RegionTable_Init_KokiriForest();      RegionTable_Init_LostWoods();
    RegionTable_Init_SacredForestMeadow();RegionTable_Init_HyruleField();
    RegionTable_Init_LakeHylia();         RegionTable_Init_LonLonRanch();
    RegionTable_Init_Market();            RegionTable_Init_TempleOfTime();
    RegionTable_Init_CastleGrounds();     RegionTable_Init_Kakariko();
    RegionTable_Init_Graveyard();         RegionTable_Init_DeathMountainTrail();
    RegionTable_Init_GoronCity();         RegionTable_Init_DeathMountainCrater();
    RegionTable_Init_ZoraRiver();         RegionTable_Init_ZorasDomain();
    RegionTable_Init_ZorasFountain();     RegionTable_Init_GerudoValley();
    RegionTable_Init_GerudoFortress();    RegionTable_Init_ThievesHideout();
    RegionTable_Init_HauntedWasteland();  RegionTable_Init_DesertColossus();
    RegionTable_Init_DekuTree();          RegionTable_Init_DodongosCavern();
    RegionTable_Init_JabuJabusBelly();    RegionTable_Init_ForestTemple();
    RegionTable_Init_FireTemple();        RegionTable_Init_WaterTemple();
    RegionTable_Init_SpiritTemple();      RegionTable_Init_ShadowTemple();
    RegionTable_Init_BottomOfTheWell();   RegionTable_Init_IceCavern();
    RegionTable_Init_GerudoTrainingGround(); RegionTable_Init_GanonsCastle();

    for (size_t i = 0; i < areaTable.size(); ++i)
        areaTable[i].key = static_cast<RandomizerRegion>(i);
}
