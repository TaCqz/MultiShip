// Smoke test: generate a seed, write .multiship + spoiler, read .multiship back,
// verify the placement table round-trips exactly.
#include "rando/Fill.h"
#include "rando/SeedFile.h"
#include <cstdio>
#include <filesystem>
#include <fstream>

int main() {
    // Output paths are relative to "_test/"; create it so the test is CWD-independent
    // (running from anywhere just makes a local _test/ — otherwise the writes fail silently).
    std::error_code ec;
    std::filesystem::create_directories("_test", ec);

    std::vector<std::string> players = { "Mori", "TaCqz" };
    Fill::Result r = Fill::Generate(12345ULL, 2);
    std::printf("generated: %zu placements, beatable=%s, %d attempt(s)\n",
                r.placements.size(), r.beatable ? "YES" : "no", r.attempts);

    std::string err;
    const char* bin = "_test/out.multiship";
    const char* txt = "_test/out_spoiler.txt";
    if (!SeedFile::WriteMultiship(bin, r, players, err)) { std::printf("write bin FAIL: %s\n", err.c_str()); return 1; }
    if (!SeedFile::WriteSpoiler(txt, r, players, err)) { std::printf("write txt FAIL: %s\n", err.c_str()); return 1; }

    SeedFile::Loaded ld;
    if (!SeedFile::ReadMultiship(bin, ld, err)) { std::printf("read FAIL: %s\n", err.c_str()); return 1; }

    bool ok = ld.seed == r.seed && ld.numWorlds == 2 &&
              ld.players.size() == 2 && ld.players[0] == "Mori" && ld.players[1] == "TaCqz" &&
              ld.placements.size() == r.placements.size();
    for (size_t i = 0; i < r.placements.size() && ok; ++i)
        ok = ld.placements[i].loc == r.placements[i].loc &&
             ld.placements[i].locWorld == r.placements[i].locWorld &&
             ld.placements[i].item == r.placements[i].item &&
             ld.placements[i].itemWorld == r.placements[i].itemWorld;

    std::ifstream f(bin, std::ios::binary | std::ios::ate);
    std::printf(".multiship size: %lld bytes, players=[%s,%s], seed=%llu\n",
                (long long)f.tellg(), ld.players[0].c_str(), ld.players[1].c_str(),
                (unsigned long long)ld.seed);
    std::printf("round-trip: %s\n", ok ? "OK" : "MISMATCH");

    std::printf("--- spoiler head ---\n");
    std::ifstream sp(txt);
    std::string line; int n = 0;
    while (std::getline(sp, line) && n++ < 16) std::printf("%s\n", line.c_str());
    return ok ? 0 : 1;
}
