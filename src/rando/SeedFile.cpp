#include "rando/SeedFile.h"

#include <fstream>

namespace SeedFile {

namespace {
constexpr char kMagic[4] = { 'M', 'S', 'H', 'P' };
constexpr uint32_t kVersion = 2;  // v2 layout (settings block present, count 0 in the reset baseline)

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
} // namespace

bool WriteMultiship(const std::string& path, uint64_t seed,
                    const std::vector<std::string>& players, std::string& err) {
    std::ofstream o(path, std::ios::binary);
    if (!o) { err = "cannot open " + path; return false; }
    o.write(kMagic, 4);
    wr<uint32_t>(o, kVersion);
    wr<uint64_t>(o, seed);
    wr<uint8_t>(o, (uint8_t)players.size());
    for (const auto& p : players) wrStr(o, p);
    // Reset baseline: no placements, no settings.
    wr<uint32_t>(o, (uint32_t)0);  // placement count
    wr<uint16_t>(o, (uint16_t)0);  // settings count
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
        Placement pl;
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

bool WriteSpoiler(const std::string& path, uint64_t seed,
                  const std::vector<std::string>& players, std::string& err) {
    std::ofstream o(path);
    if (!o) { err = "cannot open " + path; return false; }
    o << "MultiShip Seed\n";
    o << "Seed: " << seed << "\n";
    o << "Players: " << players.size() << "\n\n";
    for (size_t w = 0; w < players.size(); ++w)
        o << "Player " << (w + 1) << ": " << players[w] << "\n";
    o << "\nLocations: (none — reset baseline, no placements)\n";
    if (!o) { err = "write error"; return false; }
    return true;
}

} // namespace SeedFile
