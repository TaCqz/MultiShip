#include "rando/SeedFile.h"
#include "rando/MetaData.h"

#include <fstream>
#include <sstream>
#include <unordered_map>

namespace SeedFile {

namespace {
constexpr char kMagic[4] = { 'M', 'S', 'H', 'P' };

template <class T> void wr(std::ostream& o, T v) { o.write(reinterpret_cast<const char*>(&v), sizeof(T)); }
template <class T> T rd(std::istream& i) { T v{}; i.read(reinterpret_cast<char*>(&v), sizeof(T)); return v; }

void wrStr(std::ostream& o, const std::string& s) {
    uint8_t n = (uint8_t)(s.size() > 255 ? 255 : s.size());
    wr<uint8_t>(o, n);
    o.write(s.data(), n);
}
std::string rdStr(std::istream& i) {
    uint8_t n = rd<uint8_t>(i);
    std::string s(n, '\0');
    i.read(s.data(), n);
    return s;
}

std::unordered_map<int, const char*> LocNames() {
    std::unordered_map<int, const char*> m;
    for (int i = 0; i < kLocationCount; ++i) m[(int)kLocations[i].rc] = kLocations[i].name;
    return m;
}
std::unordered_map<int, const char*> ItemNames() {
    std::unordered_map<int, const char*> m;
    for (int i = 0; i < kItemCount; ++i) m[(int)kItems[i].rg] = kItems[i].name;
    return m;
}
} // namespace

const char* SettingKeyName(uint16_t key) {
    switch ((RandomizerSettingKey)key) {
#define RANDO_ENUM_BEGIN(EnumName)
#define RANDO_ENUM_ITEM(name, ...) case name: return #name;
#define RANDO_ENUM_END(EnumName)
#include "rando/enums/RandomizerSettingKey.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
        default: break;
    }
    return "RSK_NONE";
}

// THE single v3 serializer. Both the .multiship file writer (WriteMultiship) and the
// server->client wire path (SerializeToBytes) go through this, so the bytes they
// produce are identical — file == wire, no drift. Bump kVersion + this layout
// together if the schema ever changes.
static void SerializeV3(std::ostream& o, const SeedData& data) {
    o.write(kMagic, 4);
    wr<uint32_t>(o, kVersion);
    wr<uint64_t>(o, data.seed);
    wrStr(o, data.seedId);
    const uint8_t nWorlds = (uint8_t)data.players.size();
    wr<uint8_t>(o, nWorlds);
    for (const auto& p : data.players) wrStr(o, p);

    wr<uint32_t>(o, (uint32_t)data.placements.size());
    for (const auto& pl : data.placements) {
        wr<uint8_t>(o, (uint8_t)pl.locWorld);
        wr<uint16_t>(o, (uint16_t)pl.loc);
        wr<uint8_t>(o, (uint8_t)pl.ownerWorld);
        wr<uint16_t>(o, (uint16_t)pl.item);
    }

    wr<uint16_t>(o, (uint16_t)data.settings.size());
    for (const auto& s : data.settings) {
        wr<uint16_t>(o, s.key);
        wr<uint16_t>(o, s.value);
    }
}

bool WriteMultiship(const std::string& path, const SeedData& data, std::string& err) {
    std::ofstream o(path, std::ios::binary);
    if (!o) { err = "cannot open " + path; return false; }
    SerializeV3(o, data);
    if (!o) { err = "write error"; return false; }
    return true;
}

std::string SerializeToBytes(const SeedData& data) {
    // Plain ostringstream: string streams never do text/newline translation, so the
    // bytes match the binary-mode ofstream WriteMultiship uses, byte for byte.
    std::ostringstream o;
    SerializeV3(o, data);
    return o.str();
}

bool ReadMultiship(const std::string& path, SeedData& out, std::string& err) {
    std::ifstream i(path, std::ios::binary);
    if (!i) { err = "cannot open " + path; return false; }
    char magic[4];
    i.read(magic, 4);
    if (std::string(magic, 4) != std::string(kMagic, 4)) { err = "bad magic"; return false; }
    out = SeedData{};
    out.version = rd<uint32_t>(i);
    if (out.version != kVersion && out.version != 2) { err = "unsupported version"; return false; }

    out.seed = rd<uint64_t>(i);
    if (out.version >= 3) out.seedId = rdStr(i);  // v2 had no seed id
    out.numWorlds = rd<uint8_t>(i);
    out.players.clear();
    for (int w = 0; w < out.numWorlds; ++w) out.players.push_back(rdStr(i));

    uint32_t count = rd<uint32_t>(i);
    out.placements.clear();
    out.placements.reserve(count);
    for (uint32_t k = 0; k < count; ++k) {
        Placement pl;
        pl.locWorld = rd<uint8_t>(i);
        pl.loc = (RandomizerCheck)rd<uint16_t>(i);
        pl.ownerWorld = rd<uint8_t>(i);
        pl.item = (RandomizerGet)rd<uint16_t>(i);
        out.placements.push_back(pl);
    }

    if (out.version >= 3) {
        uint16_t nSettings = rd<uint16_t>(i);
        out.settings.clear();
        out.settings.reserve(nSettings);
        for (uint16_t s = 0; s < nSettings; ++s) {
            Setting st;
            st.key = rd<uint16_t>(i);
            st.value = rd<uint16_t>(i);
            out.settings.push_back(st);
        }
    } else {
        // v2 legacy: a flat per-RSK byte block. Obsolete (reset-baseline empties), so we
        // skip it rather than re-key it — old files carried no curated selection anyway.
        uint16_t nSettings = rd<uint16_t>(i);
        for (uint16_t s = 0; s < nSettings; ++s) rd<uint8_t>(i);
    }
    if (!i) { err = "read error / truncated"; return false; }
    return true;
}

bool WriteSpoiler(const std::string& path, const SeedData& data, std::string& err) {
    std::ofstream o(path);
    if (!o) { err = "cannot open " + path; return false; }
    auto locName = LocNames();
    auto itemName = ItemNames();
    auto pname = [&](int w) {
        return (w >= 0 && w < (int)data.players.size()) ? data.players[w]
                                                        : ("Player " + std::to_string(w + 1));
    };

    o << "MultiShip MultiWorld Seed\n";
    o << "Seed ID: " << (data.seedId.empty() ? "(none)" : data.seedId) << "\n";
    o << "Seed: " << data.seed << "\n";
    o << "Players: " << data.players.size() << "\n\n";
    for (size_t w = 0; w < data.players.size(); ++w)
        o << "Player " << (w + 1) << ": " << data.players[w] << "\n";

    o << "\nSettings (curated, RSK name -> option index):\n";
    if (data.settings.empty()) {
        o << "  (none)\n";
    } else {
        for (const auto& s : data.settings)
            o << "  " << SettingKeyName(s.key) << " = " << s.value << "\n";
    }

    o << "\nLocations:\n\n";
    for (int w = 0; w < (int)data.players.size(); ++w) {
        for (const auto& pl : data.placements) {
            if (pl.locWorld != w) continue;
            const char* ln = locName.count((int)pl.loc) ? locName[(int)pl.loc] : "?";
            const char* in = itemName.count((int)pl.item) ? itemName[(int)pl.item] : "?";
            o << ln << " (" << pname(pl.locWorld) << "): " << in << " (" << pname(pl.ownerWorld) << ")\n";
        }
    }
    if (!o) { err = "write error"; return false; }
    return true;
}

} // namespace SeedFile
