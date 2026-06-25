// SeedFile — the STABLE, VERSIONED MultiShip multiworld seed contract.
//
// `SeedData` is the single in-memory result struct the generator produces, the
// .multiship writer/reader persists, and (in a later ticket) the server->client
// wire payload carries. It is the PERSISTENT contract that lets the client show
// the correct item at each check AND label cross-world items by their owner. It is
// designed to survive a server restart: everything needed to rebuild a session is
// in the file (seed id, settings, full placement table with owners, player names).
//
// The .multiship is NOT a SoH save; it's the table the server hands to / loads for
// a session. The companion .txt spoiler is the human-readable view.
//
// Binary layout (v3):
//   "MSHP"  | version:u32 | seed:u64 | seedId:str | numWorlds:u8
//   per world: player name:str
//   placementCount:u32 | per placement: locWorld:u8, loc:u16, ownerWorld:u8, item:u16
//   settingCount:u16    | per setting:   rskKey:u16, value:u16
// (str = u8 length prefix + bytes). v2 files (reset-baseline empties) still load.
#pragma once
#include "randomizerEnums.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SeedFile {

// Current on-disk / wire schema version. Bump when the layout changes.
constexpr uint32_t kVersion = 3;

// One routed placement: location `loc` in world `locWorld` holds `item`, which is
// OWNED by `ownerWorld` (delivered to that player). ownerWorld == locWorld means a
// own-world item the client grants locally; otherwise it routes cross-world.
struct Placement {
    RandomizerCheck loc = RC_UNKNOWN_CHECK;
    int locWorld = 0;
    RandomizerGet item = RG_NONE;
    int ownerWorld = 0;
};

// One curated setting exactly as chosen in the Create UI: the base SoH
// RandomizerSettingKey and its option value. `value` is the BASE option index
// (Opt::value from CuratedSettings), verbatim — not a remapped subset index — so the
// client can apply it without translation. `key`'s enum NAME is SettingKeyName(key).
struct Setting {
    uint16_t key = 0;    // RandomizerSettingKey enum value
    uint16_t value = 0;  // base SoH option index
};

// The full multiworld seed contract. The generator fills it; WriteMultiship persists
// it; ReadMultiship reconstructs it. Round-trips byte-for-byte through write/read.
struct SeedData {
    uint32_t version = kVersion;
    uint64_t seed = 0;                  // numeric RNG seed (the determinism source)
    std::string seedId;                // human-readable id derived from `seed`
    int numWorlds = 0;
    std::vector<std::string> players;  // one name per world
    std::vector<Placement> placements; // per-world routed placement table
    std::vector<Setting> settings;     // curated settings (RSK key -> option index)
};

// The loader fills the very same contract it was written from.
using Loaded = SeedData;

// RandomizerSettingKey enum value -> its enum NAME (e.g. "RSK_STARTING_AGE").
// Returns "RSK_NONE" for out-of-range keys. Stable preset/spoiler/label key.
const char* SettingKeyName(uint16_t key);

// Binary .multiship. Returns false + fills err on failure.
bool WriteMultiship(const std::string& path, const SeedData& data, std::string& err);
bool ReadMultiship(const std::string& path, SeedData& out, std::string& err);

// Serialize SeedData to the EXACT v3 byte layout WriteMultiship persists to disk,
// returned as an in-memory byte buffer for the server->client wire. Both this and
// WriteMultiship go through the same serializer, so the wire bytes are byte-for-byte
// identical to the .multiship file (file == wire, no drift). The transport may
// re-encode this buffer (e.g. base64) to survive a text / NUL-delimited channel; the
// DECODED bytes are what match the file. See docs/multiship-wire-v3.md.
std::string SerializeToBytes(const SeedData& data);

// Human-readable spoiler log: seed id, players, settings (by name), and every
// placement grouped by the world its location belongs to, labelled with the owner.
bool WriteSpoiler(const std::string& path, const SeedData& data, std::string& err);

} // namespace SeedFile
