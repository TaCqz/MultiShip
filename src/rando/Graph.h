// Clean-room region-graph layer: the Region/Entrance/EventAccess/LocationAccess
// types + the LOCATION/ENTRANCE/EVENT_ACCESS macros that SoH's verbatim rule files
// (location_access/*.cpp) expand into. Structure mirrors SoH's location_access.h /
// entrance.h so those files compile unchanged.
#pragma once

#include "randomizerEnums.h"
#include "SceneIds.h"
#include "Logic.h"
#include "Context.h"

#include <array>
#include <list>
#include <set>
#include <string>
#include <vector>

typedef bool (*ConditionFn)();

inline std::string CleanConditionString(const std::string& s) { return s; }

// EVENT_ACCESS(event, condition): rule true -> the LogicVal flag becomes set.
#define EVENT_ACCESS(event, condition) \
    EventAccess(event, #event, [] { return condition; }, CleanConditionString(#condition))

// LOCATION(check, condition): rule true -> this RandomizerCheck is reachable here.
#define LOCATION(check, condition) \
    LocationAccess(check, [] { return condition; }, CleanConditionString(#condition))

// ENTRANCE(region, condition, ...): rule true -> you can travel to region.
#define ENTRANCE(check, condition, ...) \
    Rando::Entrance(RandomizerRegion::check, [] { return condition; }, \
                    CleanConditionString(#condition), ##__VA_ARGS__)

class EventAccess {
  public:
    EventAccess(LogicVal event_, std::string eventStr_, ConditionFn cond_, std::string condStr_)
        : event(event_), eventStr(std::move(eventStr_)), cond(cond_), condStr(std::move(condStr_)) {}
    LogicVal event;
    std::string eventStr;
    ConditionFn cond;
    std::string condStr;
};

class LocationAccess {
  public:
    LocationAccess(RandomizerCheck loc_, ConditionFn cond_, std::string condStr_)
        : location(loc_), cond(cond_), condStr(std::move(condStr_)) {}
    RandomizerCheck location;
    ConditionFn cond;
    std::string condStr;
};

namespace Rando {
class Entrance {
  public:
    Entrance(RandomizerRegion to_, ConditionFn cond_, std::string condStr_, bool spreadsAreas_ = true)
        : connectedRegion(to_), cond(cond_), condStr(std::move(condStr_)), spreadsAreas(spreadsAreas_) {}
    RandomizerRegion connectedRegion;
    ConditionFn cond;
    std::string condStr;
    bool spreadsAreas = true;
};
} // namespace Rando

class Region {
  public:
    Region() = default;
    Region(std::string name_, SceneID scene_, std::vector<EventAccess> events_,
           std::vector<LocationAccess> locations_, std::list<Rando::Entrance> exits_)
        : regionName(std::move(name_)), scene(scene_), events(std::move(events_)),
          locations(std::move(locations_)), exits(std::move(exits_)) {}
    Region(std::string name_, SceneID scene_, bool timePass_, std::set<RandomizerArea> areas_,
           std::vector<EventAccess> events_, std::vector<LocationAccess> locations_,
           std::list<Rando::Entrance> exits_)
        : regionName(std::move(name_)), scene(scene_), timePass(timePass_), areas(std::move(areas_)),
          events(std::move(events_)), locations(std::move(locations_)), exits(std::move(exits_)) {}

    std::string regionName;
    SceneID scene{};
    bool timePass = false;
    std::set<RandomizerArea> areas;
    std::vector<EventAccess> events;
    std::vector<LocationAccess> locations;
    std::list<Rando::Entrance> exits;

    // age/time reachability, filled by the search
    bool childDay = false, childNight = false, adultDay = false, adultNight = false;
    bool addedToPool = false;
    RandomizerRegion key = RR_NONE;

    bool Child() const { return childDay || childNight; }
    bool Adult() const { return adultDay || adultNight; }
    bool HasAccess() const { return Child() || Adult(); }
};

extern std::array<Region, RR_MAX> areaTable;
extern std::vector<EventAccess> grottoEvents;
Region* RegionTable(RandomizerRegion key);

// --- free helpers used inside rule conditions (defined in location_access.cpp) ---
uint16_t GetCheckPrice(RandomizerCheck check = RC_UNKNOWN_CHECK);
uint16_t GetWalletCapacity();
bool AnyAgeTime(ConditionFn condition);
bool CanPlantBean(RandomizerRegion region, RandomizerGet bean);
bool BothAges(RandomizerRegion region);
bool ChildCanAccess(RandomizerRegion region);
bool AdultCanAccess(RandomizerRegion region);
bool SpiritCertainAccess(RandomizerRegion region);
bool SpiritShared(RandomizerRegion region, ConditionFn condition, bool anyAge = false,
                  RandomizerRegion otherRegion = RR_NONE, ConditionFn otherCondition = [] { return false; },
                  RandomizerRegion thirdRegion = RR_NONE, ConditionFn thirdCondition = [] { return false; });

// --- region table init (each defined in its location_access/*.cpp) ---
void RegionTable_Init();
void RegionTable_Init_Root();
void RegionTable_Init_KokiriForest();
void RegionTable_Init_LostWoods();
void RegionTable_Init_SacredForestMeadow();
void RegionTable_Init_HyruleField();
void RegionTable_Init_LakeHylia();
void RegionTable_Init_LonLonRanch();
void RegionTable_Init_Market();
void RegionTable_Init_TempleOfTime();
void RegionTable_Init_CastleGrounds();
void RegionTable_Init_Kakariko();
void RegionTable_Init_Graveyard();
void RegionTable_Init_DeathMountainTrail();
void RegionTable_Init_GoronCity();
void RegionTable_Init_DeathMountainCrater();
void RegionTable_Init_ZoraRiver();
void RegionTable_Init_ZorasDomain();
void RegionTable_Init_ZorasFountain();
void RegionTable_Init_GerudoValley();
void RegionTable_Init_GerudoFortress();
void RegionTable_Init_HauntedWasteland();
void RegionTable_Init_DesertColossus();
void RegionTable_Init_DekuTree();
void RegionTable_Init_DodongosCavern();
void RegionTable_Init_JabuJabusBelly();
void RegionTable_Init_ForestTemple();
void RegionTable_Init_FireTemple();
void RegionTable_Init_WaterTemple();
void RegionTable_Init_SpiritTemple();
void RegionTable_Init_ShadowTemple();
void RegionTable_Init_BottomOfTheWell();
void RegionTable_Init_IceCavern();
void RegionTable_Init_ThievesHideout();
void RegionTable_Init_GerudoTrainingGround();
void RegionTable_Init_GanonsCastle();
