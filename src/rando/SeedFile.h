// SeedFile — compact binary .multiship writer/reader (the server's authoritative
// placement table) + human-readable .txt spoiler. The .multiship is NOT a SoH save;
// it's the table the server consults to route items between worlds.
#pragma once
#include "rando/Fill.h"
#include <string>
#include <vector>
#include <cstdint>

namespace SeedFile {

struct Loaded {
    uint32_t version = 0;
    uint64_t seed = 0;
    int numWorlds = 0;
    std::vector<std::string> players;
    std::vector<Fill::Placement> placements;
    std::vector<uint8_t> settings;  // per-RSK option values (RSK_MAX entries)
};

// Binary .multiship. Returns false + fills err on failure.
bool WriteMultiship(const std::string& path, const Fill::Result& r,
                    const std::vector<std::string>& players, std::string& err);
bool ReadMultiship(const std::string& path, Loaded& out, std::string& err);

// Human-readable spoiler log (reference AP format).
bool WriteSpoiler(const std::string& path, const Fill::Result& r,
                  const std::vector<std::string>& players, std::string& err);

} // namespace SeedFile
