// Search.h — reachability over the region graph for the CURRENT logic inventory.
// Ported (default-settings subset) from SoH's GetAccessibleLocations fixpoint:
// propagate child/adult x day/night access through exits, fire events, time-travel
// at the Temple of Time, iterate to a fixpoint. Operates on the global areaTable +
// global `logic` (set logic to the world being searched before calling).
#pragma once
#include "randomizerEnums.h"
#include <vector>

namespace Search {
// Run the access fixpoint over areaTable using logic's current inventory/events.
void Run();
// Run() then return every reachable location check (deduped across regions).
std::vector<RandomizerCheck> ReachableLocations();
}
