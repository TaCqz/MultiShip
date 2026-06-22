// Standalone test for the server-side delivery engine (Rando::Session).
// Builds with: cl ... smoke_session.cpp Session.cpp Fill.cpp SeedFile.cpp <engine>
#include "rando/Session.h"
#include "rando/Fill.h"
#include "rando/SeedFile.h"
#include "rando/SettingsSpec.h"

#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

static int gFail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++gFail; } \
                              else { std::printf("  ok:   %s\n", msg); } } while (0)

int main() {
    // The .multiship/.session paths below are relative to "_test/"; create it so the test
    // is CWD-independent (otherwise the file writes fail silently when run from elsewhere).
    std::error_code ec;
    std::filesystem::create_directories("_test", ec);

    // 1. Generate a deterministic seed and wrap it as a loaded table.
    Fill::Result r = Fill::Generate(12345, 2);
    std::printf("seed 12345: %zu placements, %d cross-world, beatable=%s\n",
                r.placements.size(), r.crossWorld, r.beatable ? "YES" : "NO");

    CHECK(!r.settings.empty(), "Fill captured the per-RSK settings");

    // Settings spec + override plumbing: an override is applied + captured.
    CHECK(!Rando::MultiShipSettings().empty(), "settings spec is non-empty");
    {
        std::vector<Fill::SettingOverride> ov = {
            { (uint16_t)RSK_GERUDO_FORTRESS, (uint8_t)RO_GF_CARPENTERS_FREE }  // easier; stays beatable
        };
        Fill::Result r2 = Fill::Generate(777, 2, ov);
        bool applied = r2.settings.size() > (size_t)RSK_GERUDO_FORTRESS &&
                       r2.settings[(size_t)RSK_GERUDO_FORTRESS] == (uint8_t)RO_GF_CARPENTERS_FREE;
        CHECK(applied, "setting override is applied + captured into Result.settings");
    }

    // Settings round-trip through the .multiship binary format (v2).
    {
        std::vector<std::string> players = { "Alice", "Bob" };
        std::string err, p = "_test/12345_settings.multiship";
        SeedFile::Loaded back;
        bool ok = SeedFile::WriteMultiship(p, r, players, err) && SeedFile::ReadMultiship(p, back, err);
        CHECK(ok, "WriteMultiship+ReadMultiship round-trip ok");
        CHECK(ok && back.settings == r.settings, ".multiship preserves the settings block");
        std::remove(p.c_str());
        std::remove("_test/12345_settings_spoiler.txt");
    }

    SeedFile::Loaded loaded;
    loaded.version = 2;
    loaded.seed = r.seed;
    loaded.numWorlds = 2;
    loaded.players = { "Alice", "Bob" };
    loaded.placements = r.placements;
    loaded.settings = r.settings;

    // Find two distinct locations in world 0 whose items are OWNED by world 1.
    std::vector<Fill::Placement> crossW0toW1;
    for (const auto& p : r.placements)
        if (p.locWorld == 0 && p.itemWorld == 1) crossW0toW1.push_back(p);
    CHECK(crossW0toW1.size() >= 2, "have >=2 cross-world placements (W0 loc -> W1 item)");
    if (crossW0toW1.size() < 2) { std::printf("cannot continue\n"); return 1; }

    const auto pA = crossW0toW1[0];
    const auto pB = crossW0toW1[1];

    Rando::Session s;
    s.LoadSeed(loaded);
    CHECK(s.Settings() == r.settings, "Session exposes the seed settings");
    CHECK(s.WorldOfPlayer("Alice") == 0, "WorldOfPlayer(Alice)==0");
    CHECK(s.WorldOfPlayer("Bob") == 1, "WorldOfPlayer(Bob)==1");
    CHECK(s.WorldOfPlayer("Nobody") == -1, "WorldOfPlayer(unknown)==-1");

    std::string sessPath = "_test/12345.session";
    std::remove(sessPath.c_str());
    std::string err;
    CHECK(s.AttachSessionFile(sessPath, err), "AttachSessionFile (fresh) ok");

    // 2. Alice (world 0) collects a location holding Bob's item -> deliver to Bob, seq 0.
    auto d1 = s.RecordCheck(0, pA.loc);
    CHECK(d1.size() == 1, "RecordCheck routes one delivery");
    CHECK(!d1.empty() && d1[0].world == 1, "delivery goes to world 1");
    CHECK(!d1.empty() && d1[0].player == "Bob", "delivery recipient is Bob");
    CHECK(!d1.empty() && d1[0].item == pA.item, "delivered item matches placement");
    CHECK(!d1.empty() && d1[0].seq == 0, "first delivery seq == 0");

    // 3. Duplicate collection -> no delivery.
    auto dup = s.RecordCheck(0, pA.loc);
    CHECK(dup.empty(), "duplicate check yields no delivery");

    // 4. A second cross-world check -> seq 1.
    auto d2 = s.RecordCheck(0, pB.loc);
    CHECK(d2.size() == 1 && d2[0].seq == 1, "second delivery seq == 1");

    // 4b. An own-world check (W0 loc holding a W0 item) is NOT routed — the client
    //     grants it locally.
    Fill::Placement ownW0{};
    bool haveOwn = false;
    for (const auto& p : r.placements)
        if (p.locWorld == 0 && p.itemWorld == 0) { ownW0 = p; haveOwn = true; break; }
    CHECK(haveOwn, "have an own-world placement (W0 loc -> W0 item)");
    if (haveOwn) {
        auto own = s.RecordCheck(0, ownW0.loc);
        CHECK(own.empty(), "own-world check yields no server delivery");
    }

    // 5. Crash re-send: SyncPlayer(world1, receivedSeq).
    auto syncFrom0 = s.SyncPlayer(1, 0);
    CHECK(syncFrom0.size() == 2, "SyncPlayer(1,0) re-sends both");
    auto syncFrom1 = s.SyncPlayer(1, 1);
    CHECK(syncFrom1.size() == 1 && syncFrom1[0].seq == 1, "SyncPlayer(1,1) re-sends only seq 1");
    auto syncFrom2 = s.SyncPlayer(1, 2);
    CHECK(syncFrom2.empty(), "SyncPlayer(1,2) re-sends nothing (all applied)");

    // 6. Persistence + replay: a fresh Session that re-attaches the .session file
    //    must rebuild identical outboxes (deterministic seq assignment).
    Rando::Session s2;
    s2.LoadSeed(loaded);
    CHECK(s2.AttachSessionFile(sessPath, err), "AttachSessionFile (replay) ok");
    auto re = s2.SyncPlayer(1, 0);
    bool same = re.size() == syncFrom0.size();
    for (size_t i = 0; same && i < re.size(); ++i)
        same = (re[i].seq == syncFrom0[i].seq && re[i].item == syncFrom0[i].item &&
                re[i].player == syncFrom0[i].player);
    CHECK(same, "replayed session reproduces identical deliveries");
    CHECK(s2.CollectedCount(0) == 3, "replayed collected-count for world 0 == 3 (2 cross + 1 own)");

    // 7. WorldPlacements: each world's placement list = locations IN that world.
    auto wp0 = s.WorldPlacements(0);
    auto wp1 = s.WorldPlacements(1);
    size_t loc0 = 0, loc1 = 0;
    for (const auto& p : r.placements) (p.locWorld == 0 ? loc0 : loc1)++;
    CHECK(wp0.size() == loc0, "WorldPlacements(0) count == world-0 location count");
    CHECK(wp1.size() == loc1, "WorldPlacements(1) count == world-1 location count");
    // Spot-check that a returned (check,item) matches the seed's placement table.
    bool wpMatch = false, ownerMatch = false;
    for (const auto& w : wp0)
        for (const auto& p : r.placements)
            if (p.locWorld == 0 && p.loc == w.loc) {
                wpMatch = (p.item == w.item);
                ownerMatch = (p.itemWorld == w.owner);
                break;
            }
    CHECK(wpMatch, "WorldPlacements item matches the seed placement");
    CHECK(ownerMatch, "WorldPlacements owner matches the seed placement");

    std::remove(sessPath.c_str());
    std::printf(gFail ? "\n%d CHECK(S) FAILED\n" : "\nALL SESSION CHECKS PASSED\n", gFail);
    return gFail ? 1 : 0;
}
