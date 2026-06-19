// LogicData — the static Ship of Harkinian randomizer logic graph, loaded from
// `multiship_logic.json` (produced by rando-logic/logic_compiler.py).
//
// This is the clean-room, data-driven logic model: SoH's own engine can't be linked
// (it pulls in OTRGlobals / libultraship / actor overlays), so the rules are compiled
// ahead of time into a reduced boolean AST (settings baked to the reference defaults,
// tricks off, helpers from logic.cpp inlined). The C++ runtime only loads & evaluates.
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

namespace rando {

// --- Requirement AST -------------------------------------------------------
enum class ReqOp : uint8_t {
    Const,   // cval
    And,     // kids (all)
    Or,      // kids (any)
    Not,     // kids[0]
    Has,     // a = item index, b = count
    Event,   // a = event index
    Age,     // a = 0 child / 1 adult
    Time,    // a = 0 day / 1 night
    Keys,    // a = dungeon index, b = count (small keys)
    Hearts,  // a = effective-health threshold
    Tokens,  // a = gold-skulltula-token threshold
    Atom,    // a = atom-string index (opaque predicate — see LogicData::atoms)
};

struct Req {
    ReqOp op = ReqOp::Const;
    bool cval = false;          // Const
    int a = 0;                  // item/event/dungeon/atom index, age/time/threshold
    int b = 0;                  // count
    std::vector<Req> kids;      // And/Or/Not
};

// --- Catalog records -------------------------------------------------------
struct ItemDef {
    std::string key;            // "RG_KOKIRI_SWORD"
    std::string name;           // "Kokiri Sword"
    std::string type;           // "EQUIP", "ITEM", "LOGIC", ...
    std::string category;       // "MAJOR", "JUNK", ...
    bool progression = false;   // advancement item — relevant to logic
};

struct LocationDef {
    std::string key;            // "RC_KF_KOKIRI_SWORD_CHEST"
    std::string name;           // "Kokiri Sword Chest"
    std::string type;           // RCTYPE_* (no prefix)
    std::string vanilla;        // vanilla item key ("RG_...")
    bool shuffled = false;      // in the shuffle pool under default settings
};

// rule attached to a target (location/event/region) inside a region
struct GuardedRef {
    int target = 0;             // index into locations / events / regions
    Req rule;
};

struct Region {
    std::string key;            // "RR_KOKIRI_FOREST"
    std::string name;           // "Kokiri Forest"
    std::string scene;          // "SCENE_KOKIRI_FOREST"
    std::vector<GuardedRef> events;   // target = event index
    std::vector<GuardedRef> checks;   // target = location index
    std::vector<GuardedRef> exits;    // target = region index
};

// --- The whole static graph ------------------------------------------------
class LogicData {
  public:
    int version = 0;
    std::vector<ItemDef> items;
    std::vector<std::string> events;     // event keys ("LOGIC_*")
    std::vector<LocationDef> locations;
    std::vector<std::string> dungeons;   // dungeon region keys for Keys atoms
    std::vector<std::string> atoms;      // opaque atom names
    std::vector<Region> regions;
    int startRegion = 0;

    // name -> index lookups (built after load)
    std::unordered_map<std::string, int> itemIndex;
    std::unordered_map<std::string, int> eventIndex;
    std::unordered_map<std::string, int> locationIndex;
    std::unordered_map<std::string, int> regionIndex;

    // Load from the JSON asset. Returns false and fills `error` on failure.
    bool Load(const std::string& path, std::string& error);

    int ItemIdx(const std::string& key) const { return Lookup(itemIndex, key); }
    int LocationIdx(const std::string& key) const { return Lookup(locationIndex, key); }
    int RegionIdx(const std::string& key) const { return Lookup(regionIndex, key); }

  private:
    static int Lookup(const std::unordered_map<std::string, int>& m, const std::string& k) {
        auto it = m.find(k);
        return it == m.end() ? -1 : it->second;
    }
};

} // namespace rando
