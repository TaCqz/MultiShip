// Smoke test for the MultiShip FOUNDATION pipeline (STEP 2).
//
// Verifies, with non-zero exit on any failure:
//   1. a generated 2-world seed is JOINTLY beatable (both worlds finishable)
//   2. generation is deterministic (same inputs -> identical placements + seed id)
//   3. ice traps were injected into junk
//   4. the serialized .multiship round-trips: seed, seed id, players, every
//      placement (loc / locWorld / item / ownerWorld) and every curated setting.
#include "rando/Generator.h"
#include "rando/SeedFile.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {

int failures = 0;
void check(bool cond, const char* what) {
    std::printf("  [%s] %s\n", cond ? "OK" : "FAIL", what);
    if (!cond) ++failures;
}

bool SamePlacements(const SeedFile::SeedData& a, const SeedFile::SeedData& b) {
    if (a.placements.size() != b.placements.size()) return false;
    for (size_t i = 0; i < a.placements.size(); ++i) {
        const auto& x = a.placements[i];
        const auto& y = b.placements[i];
        if (x.loc != y.loc || x.locWorld != y.locWorld || x.item != y.item ||
            x.ownerWorld != y.ownerWorld)
            return false;
    }
    return true;
}

} // namespace

int main() {
    // Curated settings the UI might carry — fabricated here; they pass through verbatim
    // to the output and must round-trip (placement does not branch on them yet).
    std::vector<SeedFile::Setting> curated = {
        { (uint16_t)RSK_STARTING_AGE, 1 },
        { (uint16_t)RSK_SHUFFLE_SONGS, 2 },
        { (uint16_t)RSK_RAINBOW_BRIDGE, 4 },
    };

    Generator::Options opts;
    opts.seed = 0xC0FFEEULL;
    opts.players = { "Mori", "TaCqz" };
    opts.settings = curated;

    std::printf("=== generate ===\n");
    Generator::Output g = Generator::Generate(opts);
    std::printf("  seed id: %s\n", g.seed.seedId.c_str());
    std::printf("  %zu placements, %d cross-world, %d ice traps, %d attempt(s)\n",
                g.seed.placements.size(), g.crossWorld, g.iceTraps, g.attempts);

    check(g.beatable, "2-world seed is jointly beatable");
    check(!g.seed.placements.empty(), "placement table is non-empty");
    check(g.iceTraps > 0, "ice traps injected into junk");
    check(g.seed.seedId == Generator::MakeSeedId(opts.seed), "seed id derives from seed");
    check(g.seed.numWorlds == 2, "two worlds");

    // Determinism: regenerate with identical inputs.
    Generator::Output g2 = Generator::Generate(opts);
    check(SamePlacements(g.seed, g2.seed), "deterministic placements (same seed -> same)");
    check(g.seed.seedId == g2.seed.seedId, "deterministic seed id");

    std::printf("=== serialize round-trip ===\n");
    std::error_code ec;
    std::filesystem::create_directories("_test", ec);
    const std::string bin = "_test/foundation.multiship";
    const std::string txt = "_test/foundation_spoiler.txt";

    std::string err;
    check(SeedFile::WriteMultiship(bin, g.seed, err), "write .multiship");
    check(SeedFile::WriteSpoiler(txt, g.seed, err), "write spoiler");

    SeedFile::SeedData ld;
    check(SeedFile::ReadMultiship(bin, ld, err), "read .multiship back");

    check(ld.version == SeedFile::kVersion, "version round-trips");
    check(ld.seed == g.seed.seed, "seed round-trips");
    check(ld.seedId == g.seed.seedId, "seed id round-trips");
    check(ld.numWorlds == 2, "numWorlds round-trips");
    check(ld.players == g.seed.players, "players round-trip");
    check(SamePlacements(ld, g.seed), "placements + owners round-trip");

    bool settingsOk = ld.settings.size() == curated.size();
    for (size_t i = 0; settingsOk && i < curated.size(); ++i)
        settingsOk = ld.settings[i].key == curated[i].key &&
                     ld.settings[i].value == curated[i].value;
    check(settingsOk, "curated settings round-trip (RSK key -> option index)");

    // file == wire: the in-memory wire serializer (server->client path) must produce the
    // EXACT same bytes as the .multiship file, so the SoH client deserializes one format.
    std::ifstream f(bin, std::ios::binary);
    std::string fileBytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    std::string wireBytes = SeedFile::SerializeToBytes(g.seed);
    check(!wireBytes.empty(), "SerializeToBytes produced bytes");
    check(fileBytes == wireBytes, "SerializeToBytes == .multiship file bytes (file == wire)");

    std::printf("\n%s (%d failure(s))\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
