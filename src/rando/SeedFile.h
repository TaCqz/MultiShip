// SeedFile — compact binary .multiship writer/reader (the server's seed table) +
// human-readable .txt spoiler. The .multiship is NOT a SoH save; it's the table
// the server hands to / loads for a session.
//
// MultiShip reset baseline: the randomization engine is gone, so a freshly created
// seed carries ZERO placements and ZERO settings — only a header, the seed value
// and the player names. The binary format is UNCHANGED (magic + version 2 + seed +
// players + placement count + per-RSK settings count) so the F-030 SoH client and
// ReadMultiship still load it; the placement/settings counts are simply written as 0.
#pragma once
#include "randomizerEnums.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SeedFile {

// One routed placement (loc in world locWorld holds item owned by itemWorld). The
// engine no longer produces these, but the reader still decodes any that exist so
// the format stays backward-compatible.
struct Placement {
    RandomizerCheck loc = RC_UNKNOWN_CHECK;
    int locWorld = 0;
    RandomizerGet item = RG_NONE;
    int itemWorld = 0;
};

struct Loaded {
    uint32_t version = 0;
    uint64_t seed = 0;
    int numWorlds = 0;
    std::vector<std::string> players;
    std::vector<Placement> placements;
    std::vector<uint8_t> settings;  // per-RSK option values (empty in the reset baseline)
};

// Binary .multiship: header + seed + player names, with zero placements and zero
// settings. Returns false + fills err on failure.
bool WriteMultiship(const std::string& path, uint64_t seed,
                    const std::vector<std::string>& players, std::string& err);
bool ReadMultiship(const std::string& path, Loaded& out, std::string& err);

// Human-readable spoiler log. Lists the seed and players; no placements to show.
bool WriteSpoiler(const std::string& path, uint64_t seed,
                  const std::vector<std::string>& players, std::string& err);

} // namespace SeedFile
