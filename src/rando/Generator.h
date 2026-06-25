// Generator — the MultiShip FOUNDATION multiworld pipeline.
//
// One call turns (seed, players, curated settings) into a finished SeedFile::SeedData
// that is JOINTLY beatable (both worlds finishable), has its leftover junk seasoned
// with a fixed number of ice traps, and carries a human-readable seed id plus the
// curated settings verbatim. The result is the persistent contract (see SeedFile.h).
//
// FOUNDATION baseline (this ticket): placement runs the restored assumed-fill engine
// at ITS OWN baked defaults (Rando::Context::ApplyDefaultSettings) — the curated UI
// settings are passed straight through to the OUTPUT but do NOT branch placement yet.
// Later per-setting tickets fold each curated setting into the engine one at a time.
// See docs/multiship-generator-foundation.md for the exact baseline.
//
// Determinism: same (seed, players, settings) -> byte-identical SeedData. The engine
// fill, the ice-trap selection, and the seed id are all derived from `seed`.
#pragma once
#include "rando/SeedFile.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Generator {

// Ice traps injected into junk for a foundation seed (total across all worlds). The
// ice-trap SETTING is wired in a later ticket; this is a fixed sane default for now.
constexpr int kDefaultIceTraps = 12;

struct Options {
    uint64_t seed = 0;
    std::vector<std::string> players;            // one per world (size == numWorlds)
    std::vector<SeedFile::Setting> settings;     // curated UI settings, passed through verbatim
    int iceTraps = kDefaultIceTraps;             // junk placements reseeded as ice traps
};

struct Output {
    SeedFile::SeedData seed;   // the contract to serialize (.multiship)
    bool beatable = false;     // both worlds jointly finishable (every progression item reachable)
    int attempts = 0;          // fill attempts the engine took
    int crossWorld = 0;        // placements whose item is owned by the other world
    int iceTraps = 0;          // ice traps actually injected
};

// Human-readable id derived deterministically from the numeric seed (e.g.
// "river-amber-otter-3f9a"). Same seed -> same id, so both players can confirm a match.
std::string MakeSeedId(uint64_t seed);

// Progress sink: called from the SAME thread as Generate(), with a fraction in [0,1]
// and a short transient stage label (copy it if you keep it). Lets a GUI show a
// progress bar instead of appearing to hang. May be null.
using ProgressFn = std::function<void(float frac, const char* stage)>;

// Run the foundation pipeline. numWorlds is taken from opts.players.size().
// `progress` (if set) is invoked throughout fill + finishing so callers can render
// generation progress; it must not touch GUI state from another thread itself.
Output Generate(const Options& opts, const ProgressFn& progress = nullptr);

} // namespace Generator
