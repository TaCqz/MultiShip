#include "rando/Search.h"
#include "rando/Graph.h"
#include "rando/Logic.h"
#include "rando/Context.h"

namespace {

// Evaluate a stored condition under any age/time the region currently has access to
// (mirrors SoH's LocationAccess::ConditionsMet / CheckConditionAtAgeTime).
bool EvalAt(const Region& r, ConditionFn cond) {
    logic->CurrentRegionKey = r.key;
    auto chk = [&](bool& age, bool& time) -> bool {
        logic->IsChild = false; logic->IsAdult = false;
        logic->AtDay = false;   logic->AtNight = false;
        age = true; time = true;
        return cond();
    };
    if (r.childDay   && chk(logic->IsChild, logic->AtDay))   return true;
    if (r.childNight && chk(logic->IsChild, logic->AtNight)) return true;
    if (r.adultDay   && chk(logic->IsAdult, logic->AtDay))   return true;
    if (r.adultNight && chk(logic->IsAdult, logic->AtNight)) return true;
    return false;
}

// Propagate a parent region's age/time access across an exit (SoH's UpdateToDAccess).
bool UpdateToD(Rando::Entrance& e, Region& parent, Region& conn) {
    bool prop = false;
    logic->CurrentRegionKey = parent.key;
    auto chk = [&](bool& age, bool& time) -> bool {
        logic->IsChild = false; logic->IsAdult = false;
        logic->AtDay = false;   logic->AtNight = false;
        age = true; time = true;
        return e.cond();
    };
    if (!conn.childDay   && parent.childDay   && chk(logic->IsChild, logic->AtDay))   { conn.childDay = true;   prop = true; }
    if (!conn.childNight && parent.childNight && chk(logic->IsChild, logic->AtNight)) { conn.childNight = true; prop = true; }
    if (!conn.adultDay   && parent.adultDay   && chk(logic->IsAdult, logic->AtDay))   { conn.adultDay = true;   prop = true; }
    if (!conn.adultNight && parent.adultNight && chk(logic->IsAdult, logic->AtNight)) { conn.adultNight = true; prop = true; }
    return prop;
}

} // namespace

namespace Search {

void Run() {
    Region& root = areaTable[RR_ROOT];
    for (Region& r : areaTable) {
        r.childDay = r.childNight = r.adultDay = r.adultNight = false;
    }
    logic->ClearEvents();
    // Starting age seeds the root's initial age/time access (you can wait for day/night, so
    // both times of day are granted). The OTHER age opens once time travel at the Temple of
    // Time becomes reachable — handled by the bidirectional swap in the fixpoint below. Reads
    // the live RSK_SELECTED_STARTING_AGE, which Fill::Generate resolves from the user-facing
    // RSK_STARTING_AGE (and forces the Door of Time open on an adult start so child access can
    // be reached back through time travel).
    if (ctx->GetOption(RSK_SELECTED_STARTING_AGE).Is(RO_AGE_ADULT))
        root.adultDay = root.adultNight = true;
    else
        root.childDay = root.childNight = true;

    bool changed = true;
    int guard = 0;
    while (changed && guard++ < 2000) {
        changed = false;
        for (Region& r : areaTable) {
            if (!r.HasAccess()) continue;
            // Time pass: a reachable time-passing region grants day+night to itself + root.
            if (r.timePass) {
                if (r.Child() && !(r.childDay && r.childNight)) {
                    r.childDay = r.childNight = true; root.childDay = root.childNight = true; changed = true;
                }
                if (r.Adult() && !(r.adultDay && r.adultNight)) {
                    r.adultDay = r.adultNight = true; root.adultDay = root.adultNight = true; changed = true;
                }
            }
            // Events: fire any newly-satisfied event flags.
            for (EventAccess& ev : r.events) {
                if (!logic->Get(ev.event) && EvalAt(r, ev.cond)) { logic->Set(ev.event, true); changed = true; }
            }
            // Exits: propagate access to connected regions.
            for (Rando::Entrance& e : r.exits) {
                if (UpdateToD(e, r, areaTable[e.connectedRegion])) changed = true;
            }
        }
        // Time travel at the Temple of Time: reaching beyond the Door of Time as one age
        // grants Root access as the other age (which opens that age's whole graph).
        Region& tot = areaTable[RR_TOT_BEYOND_DOOR_OF_TIME];
        if (!root.Adult() && tot.Child()) { root.adultDay = tot.childDay; root.adultNight = tot.childNight; changed = true; }
        if (!root.Child() && tot.Adult()) { root.childDay = tot.adultDay; root.childNight = tot.adultNight; changed = true; }
    }
}

std::vector<RandomizerCheck> ReachableLocations() {
    Run();
    std::vector<RandomizerCheck> out;
    std::vector<bool> seen(RC_MAX, false);
    for (Region& r : areaTable) {
        if (!r.HasAccess()) continue;
        for (LocationAccess& loc : r.locations) {
            if (seen[loc.location]) continue;
            if (EvalAt(r, loc.cond)) { seen[loc.location] = true; out.push_back(loc.location); }
        }
    }
    return out;
}

} // namespace Search
