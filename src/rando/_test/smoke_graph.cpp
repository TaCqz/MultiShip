// Smoke test: build the whole region graph and report basic stats.
#include "rando/Graph.h"
#include <cstdio>

int main() {
    RegionTable_Init();
    int regions = 0, locs = 0, exits = 0, events = 0;
    for (const auto& r : areaTable) {
        if (r.regionName == "Invalid Region") continue;
        regions++;
        locs += (int)r.locations.size();
        exits += (int)r.exits.size();
        events += (int)r.events.size();
    }
    std::printf("graph built: %d regions, %d location-refs, %d exits, %d events\n",
                regions, locs, exits, events);
    const Region& kf = areaTable[RR_KOKIRI_FOREST];
    std::printf("RR_KOKIRI_FOREST = \"%s\"  locations=%zu exits=%zu\n",
                kf.regionName.c_str(), kf.locations.size(), kf.exits.size());
    return 0;
}
