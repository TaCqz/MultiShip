#ifndef MULTISHIP_SERVER_H
#define MULTISHIP_SERVER_H

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include "rando/SeedFile.h"

// A small TCP server that accepts connections from ShipwreckCombo's MultiShip
// network module and reads its NUL-delimited ('\0') JSON packets.
//
// Mirrors the wire protocol of the SoH `Network` client: same raw TCP transport
// (SDL2_net) and same packet framing, just on the listening side.
class Server {
  public:
    static constexpr int MAX_CLIENTS = 16;

    Server() = default;
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // Starts listening on every local interface (the machine's IP) for `port`.
    // Returns false if the server is already running or the socket setup fails.
    bool Start(uint16_t port);

    // Stops listening and disconnects all clients. Safe to call when stopped.
    void Stop();

    bool IsRunning() const {
        return mRunning.load();
    }
    int ClientCount() const {
        return mClientCount.load();
    }

    // Returns a copy of the current log lines for rendering in the UI.
    std::vector<std::string> LogSnapshot() const;
    void ClearLog();

    // Queues a packet to be sent to every connected client. Thread-safe: the
    // payload is actually delivered on the server thread (which owns the
    // sockets) on its next loop iteration. `payload` is sent as one
    // NUL-delimited packet, matching the client's wire protocol.
    void BroadcastToClients(const std::string& payload);

    // Queues a packet for a single client, identified by its player name (or its
    // host:port label if the name isn't known yet). Same threading/framing as
    // BroadcastToClients.
    void UnicastToClient(const std::string& clientName, const std::string& payload);

    // Hands the server the loaded multiworld seed so it can answer 'Start Multiworld
    // Save' requests: validate a requested player name against the seed's player
    // list, assign + lock that player's world to the requesting client, and push the
    // byte-identical v3 SeedData. Replaces any previously set seed and resets every
    // world lock (a new seed starts a fresh session). Thread-safe; may be called
    // before or while the server is running.
    void SetSeed(const SeedFile::SeedData& seed);

    // Forgets the loaded seed and releases all world locks. 'Start Multiworld Save'
    // requests are then refused with reason "no_seed". Thread-safe.
    void ClearSeed();

  private:
    void Run(uint16_t port);
    void Log(const std::string& line);

    // Outcome of a world-lock attempt for a 'Start Multiworld Save' request.
    enum class LockResult { Granted, DeniedLocked, DeniedUnknownName, DeniedNoSeed };

    // Validates `name` against the loaded seed and, if its world is free or already
    // held by `connId`, locks that world to `connId`. On Granted, fills `outWire`
    // with the byte-identical v3 SeedData and `outWorld` with the player's world
    // index. Thread-safe (takes mSeedMutex).
    LockResult TryLockWorld(const std::string& name, uint64_t connId,
                            std::string& outWire, int& outWorld);

    // Releases every world lock held by `connId` (called when a client disconnects
    // or the session ends). Thread-safe.
    void ReleaseLocksFor(uint64_t connId);

    // Builds the NON-locking 'seed info' packet ({type, seedId, players[]}) a client
    // needs to know which world names are valid before it claims one. Returns "" if no
    // seed is loaded. Thread-safe (takes mSeedMutex).
    std::string BuildSeedInfoJson();

    // ---- F-040 Part B: cross-world routing + per-player delivery queue --------
    // One queued cross-world delivery for a recipient world. `seq` is the per-recipient
    // monotonic stream index (== its position in mDeliveries[world]) that the receiving
    // client dedups against via its persisted receivedSeq high-water mark.
    struct PendingDelivery {
        uint32_t seq = 0;
        int item = 0;         // RandomizerGet value to give the owner
        int sourceCheck = 0;  // the collector-world check that produced it (logging / F-039)
        int sourceWorld = 0;  // the world that collected it
    };

    // A client reported collecting `check` in `collectorWorld` whose item belongs to
    // another world. Resolve the owner from the locked seed, append the item to the
    // owner's delivery queue (assigning the next per-owner seq), and push it now if the
    // owner is connected. Deduped by (collectorWorld, check) so a replayed report can't
    // double-route. Thread-safe; runs on the server thread.
    void HandleCheckCollected(int check, int collectorWorld);

    // A client (re)loaded its save and reported the highest delivery seq it has applied.
    // Replay every queued delivery for that client's world with seq >= receivedSeq, so a
    // player picks up everything routed to it while it was offline / on another save.
    // Thread-safe; runs on the server thread.
    void HandleLoadGameCatchup(const std::string& userName, uint32_t receivedSeq);

    std::thread mThread;
    std::atomic<bool> mRunning{ false };
    std::atomic<int> mClientCount{ 0 };

    mutable std::mutex mLogMutex;
    std::deque<std::string> mLog;
    static constexpr size_t MAX_LOG_LINES = 500;

    // Packets queued by BroadcastToClients()/UnicastToClient(), drained on the
    // server thread. mPendingUnicasts holds (clientName, payload) pairs.
    std::mutex mOutMutex;
    std::vector<std::string> mPendingBroadcasts;
    std::vector<std::pair<std::string, std::string>> mPendingUnicasts;

    // ---- Multiworld seed + world locks (F-035) -------------------------------
    // Guards mHasSeed, mSeedPlayers, mSeedWireV3 and mWorldLocks. Touched by the UI
    // thread (SetSeed/ClearSeed) and the server thread (request handling / disconnect
    // / session reset), so every access takes this lock.
    std::mutex mSeedMutex;
    bool mHasSeed = false;
    std::string mSeedId;                    // human-readable seed id (for the seed-info packet)
    std::vector<std::string> mSeedPlayers;  // index = world id; one player name per world
    std::string mSeedWireV3;                // byte-identical v3 SeedData bytes (== .multiship)
    // Parsed placements (F-040 Part B): needed to resolve the owner of a collected check.
    // mSeedWireV3 alone is opaque, so SetSeed keeps the structured table too.
    std::vector<SeedFile::Placement> mSeedPlacements;
    // Per-recipient delivery queue: mDeliveries[world] is that player's ordered stream of
    // routed items (index == seq). Append-only within a session; reset on SetSeed/ClearSeed.
    // In-memory only — durable persistence/reconcile is the later F-039.
    std::vector<std::vector<PendingDelivery>> mDeliveries;
    // Dedup of already-routed collections, keyed by (collectorWorld, check), so a replayed
    // OnCheckCollected report does not enqueue the same item twice.
    std::set<std::pair<int, int>> mRoutedChecks;
    // World-lock table: player name -> owning connection id. A name present here is
    // claimed; only its owning connection may re-request it. In-memory only and reset
    // each server session (durable persistence is the later F-039).
    std::unordered_map<std::string, uint64_t> mWorldLocks;
};

#endif // MULTISHIP_SERVER_H
