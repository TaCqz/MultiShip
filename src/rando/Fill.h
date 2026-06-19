// Fill.h — deterministic multiworld assumed-fill. Distributes the combined item pool
// of all worlds across all worlds' locations (true mix: a world's location can hold
// another world's item), guaranteeing every progression item is reachable in logic.
#pragma once
#include "randomizerEnums.h"
#include <cstdint>
#include <functional>
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
    int attempts = 0;            // fill attempts taken (retries on stranded progression)
    uint64_t seed = 0;
    std::vector<uint8_t> settings; // per-RSK option values generation ran under (RSK_MAX entries)
};

// Optional progress reporting. Called from the same thread as Generate(), with a
// fraction in [0,1] and a short human-readable stage label. Stage strings are
// transient — copy them if you need to keep them.
using ProgressFn = std::function<void(float frac, const char* stage)>;

// A single settings override applied on top of the engine's baked defaults.
struct SettingOverride {
    uint16_t key;    // RandomizerSettingKey
    uint8_t value;   // RO_* option value
};

// Same seed + same settings -> same generation. numWorlds defaults to 2 (extensible).
// `settingOverrides` tweak the baked default settings before generation (and are
// captured into Result.settings, so they flow to clients). `progress` may be null.
Result Generate(uint64_t seed, int numWorlds = 2,
                const std::vector<SettingOverride>& settingOverrides = {},
                const ProgressFn& progress = nullptr);

} // namespace Fill
