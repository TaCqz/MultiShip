// Session — the server-side multiworld delivery engine.
//
// Owns the routing of items between worlds for one loaded .multiship seed:
//   * a player in world W collects a location L  ->  RecordCheck(W, L)
//   * the engine looks up who OWNS the item at (W, L) in the placement table and
//     appends it to that owner's ordered outbox with the next monotonic `seq`.
//
// Crash-safety is built in via that per-recipient monotonic `seq`:
//   * every delivery carries its seq; the client persists the highest applied seq
//     in the SAME save as its inventory, so on a crash both roll back together.
//   * on (re)connect the client reports its receivedSeq and the server re-sends
//     every outbox entry past it (SyncPlayer) — re-granting exactly what the
//     inventory is missing, never double-granting.
//
// The authoritative collection history is persisted to a small binary `.session`
// file next to the .multiship, so the engine survives a server restart. The file
// stores the *ordered* collection log; outboxes + seqs are rebuilt by replay, so
// seq assignment is deterministic and stable across restarts.
//
// Deliberately free of SDL/JSON/networking so it lives in multiship_rando and is
// unit-testable standalone. Network glue (JSON parse + unicast) lives in Server.
#pragma once
#include "rando/SeedFile.h"
#include "randomizerEnums.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Rando {

// One item routed to a player. The server turns this into a give_item command.
struct Delivery {
    int world = 0;            // recipient world (0-based)
    std::string player;       // recipient player name (from the seed)
    RandomizerGet item = RG_NONE;
    uint32_t seq = 0;         // monotonic per recipient world; client persists this
};

class Session {
  public:
    // Bind the loaded seed (placement table + player names). Resets all state.
    void LoadSeed(const SeedFile::Loaded& seed);

    bool HasSeed() const { return mLoaded; }
    uint64_t Seed() const { return mSeed; }
    int NumWorlds() const { return mNumWorlds; }
    const std::vector<std::string>& Players() const { return mPlayers; }
    const std::vector<uint8_t>& Settings() const { return mSettings; }

    // -1 if the name doesn't match any world in the seed.
    int WorldOfPlayer(const std::string& name) const;

    // Bind the on-disk .session path and, if it exists and matches this seed,
    // replay it to restore collection history + outboxes. If it's missing or for a
    // different seed, starts fresh (and will overwrite on the next RecordCheck).
    // Must be called after LoadSeed. Returns false + fills err only on a hard read
    // error of an existing file (a missing file is success / fresh start).
    bool AttachSessionFile(const std::string& path, std::string& err);

    // Record that `world` collected location `check`. Returns the deliveries to
    // send now (normally one — the routed item to its owner; empty if the check is
    // a duplicate, unknown, or holds no shuffled item). Persists the session.
    std::vector<Delivery> RecordCheck(int world, RandomizerCheck check);

    // Everything in `world`'s outbox past receivedSeq — the crash re-send. Pure
    // read; does not mutate or persist.
    std::vector<Delivery> SyncPlayer(int world, uint32_t receivedSeq) const;

    // (check, item) for every shuffled location IN `world` — the placements that
    // world's client needs so its chests/locations show the right item locally
    // (the item is still delivered remotely by the server). Item ownership is the
    // server's concern; the client only needs check->item for display.
    struct WorldPlacement {
        RandomizerCheck loc = RC_UNKNOWN_CHECK;
        RandomizerGet item = RG_NONE;
        int owner = 0;  // world that owns the item here (== loc's world for own items)
    };
    std::vector<WorldPlacement> WorldPlacements(int world) const;

    // Highest seq currently in a world's outbox (0 if empty). For logging.
    uint32_t OutboxHigh(int world) const;
    size_t CollectedCount(int world) const;

  private:
    struct OutboxEntry {
        uint32_t seq = 0;
        RandomizerGet item = RG_NONE;
        RandomizerCheck srcCheck = RC_UNKNOWN_CHECK;
        int srcWorld = 0;
    };

    // (locWorld, loc) -> (item, ownerWorld), from the seed's placement table.
    long long Key(int world, RandomizerCheck rc) const;
    Delivery MakeDelivery(const OutboxEntry& e) const;
    void Apply(int srcWorld, RandomizerCheck check); // replay/record core (no persist)
    bool SaveSessionFile(std::string& err) const;

    bool mLoaded = false;
    uint64_t mSeed = 0;
    int mNumWorlds = 0;
    std::vector<std::string> mPlayers;
    std::vector<uint8_t> mSettings;
    std::string mSessionPath;

    std::unordered_map<long long, std::pair<RandomizerGet, int>> mPlacementByLoc;

    // Per recipient world: the ordered outbox. seq == index in this vector.
    std::vector<std::vector<OutboxEntry>> mOutbox;
    // Per source world: set of already-collected checks (dedupe).
    std::vector<std::unordered_map<int, bool>> mCollected;
    // The ordered collection log (srcWorld, check) — what gets persisted/replayed.
    std::vector<std::pair<int, RandomizerCheck>> mLog;
};

} // namespace Rando
