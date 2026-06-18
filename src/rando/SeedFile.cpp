#include "rando/SeedFile.h"
#include "rando/MetaData.h"

#include <fstream>
#include <unordered_map>

namespace SeedFile {

namespace {
constexpr char kMagic[4] = { 'M', 'S', 'H', 'P' };
constexpr uint32_t kVersion = 2;  // v2 adds the per-RSK settings block

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

bool WriteMultiship(const std::string& path, const Fill::Result& r,
                    const std::vector<std::string>& players, std::string& err) {
    std::ofstream o(path, std::ios::binary);
    if (!o) { err = "cannot open " + path; return false; }
    o.write(kMagic, 4);
    wr<uint32_t>(o, kVersion);
    wr<uint64_t>(o, r.seed);
    wr<uint8_t>(o, (uint8_t)players.size());
    for (const auto& p : players) wrStr(o, p);
    wr<uint32_t>(o, (uint32_t)r.placements.size());
    for (const auto& pl : r.placements) {
        wr<uint8_t>(o, (uint8_t)pl.locWorld);
        wr<uint16_t>(o, (uint16_t)pl.loc);
        wr<uint8_t>(o, (uint8_t)pl.itemWorld);
        wr<uint16_t>(o, (uint16_t)pl.item);
    }
    // Settings block (v2): per-RSK option values generation ran under.
    wr<uint16_t>(o, (uint16_t)r.settings.size());
    for (uint8_t v : r.settings) wr<uint8_t>(o, v);
    if (!o) { err = "write error"; return false; }
    return true;
}

bool ReadMultiship(const std::string& path, Loaded& out, std::string& err) {
    std::ifstream i(path, std::ios::binary);
    if (!i) { err = "cannot open " + path; return false; }
    char magic[4];
    i.read(magic, 4);
    if (std::string(magic, 4) != std::string(kMagic, 4)) { err = "bad magic"; return false; }
    out.version = rd<uint32_t>(i);
    if (out.version != kVersion) { err = "unsupported version"; return false; }
    out.seed = rd<uint64_t>(i);
    out.numWorlds = rd<uint8_t>(i);
    out.players.clear();
    for (int w = 0; w < out.numWorlds; ++w) out.players.push_back(rdStr(i));
    uint32_t count = rd<uint32_t>(i);
    out.placements.clear();
    out.placements.reserve(count);
    for (uint32_t k = 0; k < count; ++k) {
        Fill::Placement pl;
        pl.locWorld = rd<uint8_t>(i);
        pl.loc = (RandomizerCheck)rd<uint16_t>(i);
        pl.itemWorld = rd<uint8_t>(i);
        pl.item = (RandomizerGet)rd<uint16_t>(i);
        out.placements.push_back(pl);
    }
    uint16_t nSettings = rd<uint16_t>(i);
    out.settings.clear();
    out.settings.reserve(nSettings);
    for (uint16_t s = 0; s < nSettings; ++s) out.settings.push_back(rd<uint8_t>(i));
    if (!i) { err = "read error / truncated"; return false; }
    return true;
}

bool WriteSpoiler(const std::string& path, const Fill::Result& r,
                  const std::vector<std::string>& players, std::string& err) {
    std::ofstream o(path);
    if (!o) { err = "cannot open " + path; return false; }
    auto locName = LocNames();
    auto itemName = ItemNames();
    auto pname = [&](int w) { return (w >= 0 && w < (int)players.size()) ? players[w] : ("Player " + std::to_string(w + 1)); };

    o << "MultiShip MultiWorld Seed\n";
    o << "Seed: " << r.seed << "\n";
    o << "Players: " << players.size() << "\n";
    o << "Fill attempts: " << r.attempts << "\n\n";
    for (size_t w = 0; w < players.size(); ++w)
        o << "Player " << (w + 1) << ": " << players[w] << "\n";
    o << "\nLocations:\n\n";

    // grouped by the world the location belongs to
    for (int w = 0; w < (int)players.size(); ++w) {
        for (const auto& pl : r.placements) {
            if (pl.locWorld != w) continue;
            const char* ln = locName.count((int)pl.loc) ? locName[(int)pl.loc] : "?";
            const char* in = itemName.count((int)pl.item) ? itemName[(int)pl.item] : "?";
            o << ln << " (" << pname(pl.locWorld) << "): " << in << " (" << pname(pl.itemWorld) << ")\n";
        }
    }
    if (!o) { err = "write error"; return false; }
    return true;
}

} // namespace SeedFile
