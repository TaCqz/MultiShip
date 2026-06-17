// Fill.h — deterministic multiworld assumed-fill. Distributes the combined item pool
// of all worlds across all worlds' locations (true mix: a world's location can hold
// another world's item), guaranteeing every progression item is reachable in logic.
#pragma once
#include "randomizerEnums.h"
#include <cstdint>
#include <vector>

namespace Fill {

struct Placement {
    RandomizerCheck loc;     // the location (a check in world `locWorld`)
    int locWorld;            // which world the location belongs to
    RandomizerGet item;      // the item placed there
    int itemWorld;           // which world OWNS the item (delivered to this player)
};

struct Result {
    std::vector<Placement> placements;
    bool beatable = false;       // every placed location reachable via sphere search
    int reached = 0;             // locations reachable in verification (sphere search)
    int reachableFullInv = 0;    // placed locations reachable with the full inventory
    int unreachedAdvancement = 0;// progression items not reachable (must be 0 to be beatable)
    int crossWorld = 0;          // placements where itemWorld != locWorld
    uint64_t seed = 0;
};

// Same seed -> same generation. numWorlds defaults to 2 (extensible).
Result Generate(uint64_t seed, int numWorlds = 2);

} // namespace Fill
