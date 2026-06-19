// MultiShipGen — standalone CLI multiworld seed generator (no SDL/ImGui).
// Usage: MultiShipGen [--seed N] [--p1 NAME] [--p2 NAME] [--out DIR]
//   --seed N   deterministic seed value (default: random)
//   --p1/--p2  player names (default Player 1 / Player 2)
//   --out DIR  output directory (default: current dir)
// Writes <out>/<seed>.multiship and <out>/<seed>_spoiler.txt.
#include "rando/Fill.h"
#include "rando/SeedFile.h"
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    uint64_t seed = 0;
    bool haveSeed = false;
    std::string p1 = "Player 1", p2 = "Player 2", out = ".";
    for (int i = 1; i < argc; ++i) {
        auto next = [&](const char* def) { return (i + 1 < argc) ? argv[++i] : def; };
        if (!std::strcmp(argv[i], "--seed")) { seed = std::strtoull(next("0"), nullptr, 10); haveSeed = true; }
        else if (!std::strcmp(argv[i], "--p1")) p1 = next("Player 1");
        else if (!std::strcmp(argv[i], "--p2")) p2 = next("Player 2");
        else if (!std::strcmp(argv[i], "--out")) out = next(".");
    }
    if (!haveSeed) {
        std::random_device rd;
        seed = ((uint64_t)rd() << 32) ^ rd();
    }

    std::vector<std::string> players = { p1, p2 };
    std::printf("Generating MultiWorld seed %llu for [%s, %s]...\n",
                (unsigned long long)seed, p1.c_str(), p2.c_str());
    Fill::Result r = Fill::Generate(seed, 2);
    std::printf("  %zu placements, %d cross-world, %d attempt(s), beatable=%s\n",
                r.placements.size(), r.crossWorld, r.attempts, r.beatable ? "YES" : "NO");
    if (!r.beatable) { std::printf("  WARNING: seed not beatable (progression stranded)\n"); }

    std::string base = out + "/" + std::to_string(seed);
    std::string err;
    if (!SeedFile::WriteMultiship(base + ".multiship", r, players, err)) {
        std::printf("ERROR writing .multiship: %s\n", err.c_str()); return 1;
    }
    if (!SeedFile::WriteSpoiler(base + "_spoiler.txt", r, players, err)) {
        std::printf("ERROR writing spoiler: %s\n", err.c_str()); return 1;
    }
    std::printf("Wrote %s.multiship and %s_spoiler.txt\n", base.c_str(), base.c_str());
    return 0;
}
