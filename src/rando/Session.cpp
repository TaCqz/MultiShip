#include "rando/Session.h"

#include <cstdio>
#include <cstring>

namespace Rando {

namespace {
constexpr char kMagic[4] = { 'M', 'S', 'S', 'N' };  // MultiShip SessioN
constexpr uint32_t kVersion = 1;

template <typename T> void Put(std::string& b, T v) {
    b.append(reinterpret_cast<const char*>(&v), sizeof(T));
}
template <typename T> bool Get(const std::string& b, size_t& o, T& v) {
    if (o + sizeof(T) > b.size()) return false;
    std::memcpy(&v, b.data() + o, sizeof(T));
    o += sizeof(T);
    return true;
}
} // namespace

long long Session::Key(int world, RandomizerCheck rc) const {
    return (long long)world * (long long)RC_MAX + (long long)rc;
}

void Session::LoadSeed(const SeedFile::Loaded& seed) {
    mLoaded = true;
    mSeed = seed.seed;
    mNumWorlds = seed.numWorlds > 0 ? seed.numWorlds : (int)seed.players.size();
    if (mNumWorlds <= 0) mNumWorlds = 1;
    mPlayers = seed.players;
    mPlayers.resize(mNumWorlds);  // ensure one name slot per world
    mSettings = seed.settings;
    mSessionPath.clear();

    mPlacementByLoc.clear();
    for (const auto& p : seed.placements)
        mPlacementByLoc[Key(p.locWorld, p.loc)] = { p.item, p.itemWorld };

    mOutbox.assign(mNumWorlds, {});
    mCollected.assign(mNumWorlds, {});
    mLog.clear();
}

int Session::WorldOfPlayer(const std::string& name) const {
    for (int w = 0; w < (int)mPlayers.size(); ++w)
        if (mPlayers[w] == name) return w;
    return -1;
}

Delivery Session::MakeDelivery(const OutboxEntry& e) const {
    Delivery d;
    // The recipient world is the outbox this entry lives in; resolved by the caller
    // which knows the world index. Filled in by RecordCheck/SyncPlayer.
    d.item = e.item;
    d.seq = e.seq;
    return d;
}

void Session::Apply(int srcWorld, RandomizerCheck check) {
    // Caller guarantees srcWorld is in range and the check is not a duplicate.
    mCollected[srcWorld][(int)check] = true;
    mLog.push_back({ srcWorld, check });

    auto it = mPlacementByLoc.find(Key(srcWorld, check));
    if (it == mPlacementByLoc.end()) return;  // unknown / non-shuffled: nothing to route
    RandomizerGet item = it->second.first;
    int owner = it->second.second;
    if (owner < 0 || owner >= mNumWorlds) return;
    if (owner == srcWorld) return;  // own-world item: the client grants it locally, not routed

    OutboxEntry e;
    e.seq = (uint32_t)mOutbox[owner].size();  // seq == index in the owner's outbox
    e.item = item;
    e.srcCheck = check;
    e.srcWorld = srcWorld;
    mOutbox[owner].push_back(e);
}

std::vector<Delivery> Session::RecordCheck(int world, RandomizerCheck check) {
    std::vector<Delivery> out;
    if (!mLoaded || world < 0 || world >= mNumWorlds) return out;

    auto& set = mCollected[world];
    if (set.find((int)check) != set.end()) return out;  // duplicate — already routed

    size_t ownerBefore = 0;
    auto it = mPlacementByLoc.find(Key(world, check));
    int owner = -1;
    if (it != mPlacementByLoc.end()) {
        owner = it->second.second;
        if (owner >= 0 && owner < mNumWorlds) ownerBefore = mOutbox[owner].size();
    }

    Apply(world, check);

    // Surface the newly created outbox entry (if any) as a delivery.
    if (owner >= 0 && owner < mNumWorlds && mOutbox[owner].size() > ownerBefore) {
        const OutboxEntry& e = mOutbox[owner].back();
        Delivery d = MakeDelivery(e);
        d.world = owner;
        d.player = mPlayers[owner];
        out.push_back(d);
    }

    std::string err;
    SaveSessionFile(err);  // best-effort persist; logged by caller if it matters
    return out;
}

std::vector<Delivery> Session::SyncPlayer(int world, uint32_t receivedSeq) const {
    std::vector<Delivery> out;
    if (!mLoaded || world < 0 || world >= mNumWorlds) return out;
    for (const OutboxEntry& e : mOutbox[world]) {
        if (e.seq < receivedSeq) continue;  // receivedSeq is the count already applied
        Delivery d = MakeDelivery(e);
        d.world = world;
        d.player = mPlayers[world];
        out.push_back(d);
    }
    return out;
}

std::vector<Session::WorldPlacement> Session::WorldPlacements(int world) const {
    std::vector<WorldPlacement> out;
    if (!mLoaded || world < 0 || world >= mNumWorlds) return out;
    const long long base = (long long)world * (long long)RC_MAX;
    for (const auto& kv : mPlacementByLoc) {
        if (kv.first < base || kv.first >= base + (long long)RC_MAX) continue;  // other world
        WorldPlacement wp;
        wp.loc = (RandomizerCheck)(kv.first - base);
        wp.item = kv.second.first;
        wp.owner = kv.second.second;
        out.push_back(wp);
    }
    return out;
}

uint32_t Session::OutboxHigh(int world) const {
    if (world < 0 || world >= mNumWorlds || mOutbox[world].empty()) return 0;
    return mOutbox[world].back().seq + 1;  // count of entries
}

size_t Session::CollectedCount(int world) const {
    if (world < 0 || world >= mNumWorlds) return 0;
    return mCollected[world].size();
}

bool Session::AttachSessionFile(const std::string& path, std::string& err) {
    mSessionPath = path;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return true;  // no existing session — fresh start, not an error

    std::string buf;
    std::fseek(f, 0, SEEK_END);
    long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz > 0) {
        buf.resize((size_t)sz);
        size_t rd = std::fread(buf.data(), 1, (size_t)sz, f);
        buf.resize(rd);
    }
    std::fclose(f);

    size_t o = 0;
    char magic[4];
    if (buf.size() < 4) { err = "session file truncated"; return false; }
    std::memcpy(magic, buf.data(), 4);
    o = 4;
    if (std::memcmp(magic, kMagic, 4) != 0) { err = "bad session magic"; return false; }
    uint32_t ver = 0;
    uint64_t seed = 0;
    uint32_t nEvents = 0;
    if (!Get(buf, o, ver) || !Get(buf, o, seed) || !Get(buf, o, nEvents)) {
        err = "session header truncated";
        return false;
    }
    if (ver != kVersion) { err = "unsupported session version"; return false; }
    if (seed != mSeed) {
        // Stale session for a different seed — ignore it and start fresh; the next
        // RecordCheck overwrites the file for the current seed.
        return true;
    }

    // Replay the ordered log to rebuild outboxes + collected sets deterministically.
    for (uint32_t i = 0; i < nEvents; ++i) {
        uint8_t w = 0;
        uint16_t rc = 0;
        if (!Get(buf, o, w) || !Get(buf, o, rc)) { err = "session log truncated"; return false; }
        if (w >= mNumWorlds) continue;
        if (mCollected[w].find((int)rc) != mCollected[w].end()) continue;  // defensive dedupe
        Apply((int)w, (RandomizerCheck)rc);
    }
    return true;
}

bool Session::SaveSessionFile(std::string& err) const {
    if (mSessionPath.empty()) return true;  // not bound to disk
    std::string buf;
    buf.append(kMagic, 4);
    Put<uint32_t>(buf, kVersion);
    Put<uint64_t>(buf, mSeed);
    Put<uint32_t>(buf, (uint32_t)mLog.size());
    for (const auto& ev : mLog) {
        Put<uint8_t>(buf, (uint8_t)ev.first);
        Put<uint16_t>(buf, (uint16_t)ev.second);
    }
    FILE* f = std::fopen(mSessionPath.c_str(), "wb");
    if (!f) { err = "cannot open session file for write"; return false; }
    size_t wr = std::fwrite(buf.data(), 1, buf.size(), f);
    std::fclose(f);
    if (wr != buf.size()) { err = "short write to session file"; return false; }
    return true;
}

} // namespace Rando
