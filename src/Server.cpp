#include "Server.h"

#include <SDL2/SDL_net.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstring>

namespace {
// Per-connection state: the socket plus a buffer that accumulates bytes until a
// full '\0'-delimited packet is available.
struct ClientConn {
    TCPsocket socket = nullptr;
    std::string buffer;
    std::string label;       // host:port, used until the player's name is known
    std::string userName;    // name sent by the client in its connection handshake
    uint64_t id = 0;         // per-session connection id (owns its world lock, if any)
    std::string lockedName;  // player/world this connection has locked, "" if none
};

// Standard base64 (RFC 4648, '+' '/' alphabet, '=' padding). The v3 SeedData is raw
// binary and freely contains '\0', so it can't ride the NUL-delimited transport
// directly; we base64 it into a JSON string field. The client base64-decodes back to
// the byte-identical v3 bytes before deserializing. (Encode only — the client decodes.)
std::string Base64Encode(const std::string& in) {
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(((in.size() + 2) / 3) * 4);
    const size_t n = in.size();
    size_t i = 0;
    for (; i + 3 <= n; i += 3) {
        unsigned v = ((unsigned)(unsigned char)in[i] << 16) |
                     ((unsigned)(unsigned char)in[i + 1] << 8) |
                     (unsigned)(unsigned char)in[i + 2];
        out.push_back(tbl[(v >> 18) & 63]);
        out.push_back(tbl[(v >> 12) & 63]);
        out.push_back(tbl[(v >> 6) & 63]);
        out.push_back(tbl[v & 63]);
    }
    const size_t rem = n - i;
    if (rem == 1) {
        unsigned v = (unsigned)(unsigned char)in[i] << 16;
        out.push_back(tbl[(v >> 18) & 63]);
        out.push_back(tbl[(v >> 12) & 63]);
        out.push_back('=');
        out.push_back('=');
    } else if (rem == 2) {
        unsigned v = ((unsigned)(unsigned char)in[i] << 16) |
                     ((unsigned)(unsigned char)in[i + 1] << 8);
        out.push_back(tbl[(v >> 18) & 63]);
        out.push_back(tbl[(v >> 12) & 63]);
        out.push_back(tbl[(v >> 6) & 63]);
        out.push_back('=');
    }
    return out;
}

// Keep player names to a single short, printable line so a client cannot inject
// newlines or control characters into the server log.
std::string SanitizeName(const std::string& name) {
    std::string out;
    for (char c : name) {
        if (static_cast<unsigned char>(c) >= 0x20 && c != '\x7f') {
            out.push_back(c);
        }
        if (out.size() >= 32) {
            break;
        }
    }
    return out;
}
} // namespace

Server::~Server() {
    Stop();
}

bool Server::Start(uint16_t port) {
    if (mRunning.load()) {
        return false;
    }

    // Probe that we can actually open the listening socket before flipping the
    // running flag, so the UI can report failure immediately.
    IPaddress probe;
    if (SDLNet_ResolveHost(&probe, nullptr, port) == -1) {
        Log(std::string("[SERVER] Error: SDLNet_ResolveHost: ") + SDLNet_GetError());
        return false;
    }
    TCPsocket test = SDLNet_TCP_Open(&probe);
    if (!test) {
        Log(std::string("[SERVER] Error: Could not bind port ") + std::to_string(port) + ": " + SDLNet_GetError());
        return false;
    }
    SDLNet_TCP_Close(test);

    mRunning.store(true);
    mThread = std::thread(&Server::Run, this, port);
    return true;
}

void Server::Stop() {
    if (!mRunning.exchange(false)) {
        return;
    }
    if (mThread.joinable()) {
        mThread.join();
    }
    mClientCount.store(0);
}

void Server::Run(uint16_t port) {
    IPaddress ip;
    // host == nullptr -> INADDR_ANY: listen on all of the machine's interfaces.
    if (SDLNet_ResolveHost(&ip, nullptr, port) == -1) {
        Log(std::string("[SERVER] Error: SDLNet_ResolveHost: ") + SDLNet_GetError());
        mRunning.store(false);
        return;
    }

    TCPsocket serverSocket = SDLNet_TCP_Open(&ip);
    if (!serverSocket) {
        Log(std::string("[SERVER] Error: SDLNet_TCP_Open: ") + SDLNet_GetError());
        mRunning.store(false);
        return;
    }

    // Socket set holds the listening socket plus up to MAX_CLIENTS clients.
    SDLNet_SocketSet socketSet = SDLNet_AllocSocketSet(MAX_CLIENTS + 1);
    SDLNet_TCP_AddSocket(socketSet, serverSocket);

    std::vector<ClientConn> clients;

    // Fresh per-session connection ids and world locks. Clearing here (not just in
    // SetSeed) means a Stop/Start cycle starts a clean lock session even if the seed
    // is left in place — ids restart at 1, so no stale id could match a new client.
    uint64_t nextConnId = 1;
    {
        std::lock_guard<std::mutex> lock(mSeedMutex);
        mWorldLocks.clear();
    }

    Log("[SERVER] Listening on port " + std::to_string(port) + " (all interfaces)");

    char recvBuf[1024];
    while (mRunning.load()) {
        // Wait up to 100ms for activity so we can re-check mRunning regularly.
        int ready = SDLNet_CheckSockets(socketSet, 100);

        // Deliver any queued broadcasts (from BroadcastToClients) to all clients.
        // Done here, on the server thread, so socket I/O stays single-threaded.
        std::vector<std::string> broadcasts;
        {
            std::lock_guard<std::mutex> lock(mOutMutex);
            broadcasts.swap(mPendingBroadcasts);
        }
        for (const std::string& payload : broadcasts) {
            for (ClientConn& conn : clients) {
                // +1 sends the trailing '\0' so the packet is NUL-delimited,
                // matching the client's framing.
                SDLNet_TCP_Send(conn.socket, payload.c_str(), static_cast<int>(payload.size()) + 1);
            }
            Log("[SERVER] Sent request to " + std::to_string(clients.size()) + " client(s)");
        }

        // Deliver any queued unicasts to their single target client.
        std::vector<std::pair<std::string, std::string>> unicasts;
        {
            std::lock_guard<std::mutex> lock(mOutMutex);
            unicasts.swap(mPendingUnicasts);
        }
        for (const auto& [name, payload] : unicasts) {
            bool sent = false;
            for (ClientConn& conn : clients) {
                if (conn.userName == name || conn.label == name) {
                    SDLNet_TCP_Send(conn.socket, payload.c_str(), static_cast<int>(payload.size()) + 1);
                    sent = true;
                    break; // only the first matching client
                }
            }
            Log(sent ? "[SERVER] Sent request to " + name
                     : "[SERVER] Unicast target not found: " + name);
        }

        if (ready <= 0) {
            continue;
        }

        // New incoming connection.
        if (SDLNet_SocketReady(serverSocket)) {
            TCPsocket client = SDLNet_TCP_Accept(serverSocket);
            if (client) {
                if (static_cast<int>(clients.size()) >= MAX_CLIENTS) {
                    Log("[SERVER] Connection refused (client limit reached)");
                    SDLNet_TCP_Close(client);
                } else {
                    ClientConn conn;
                    conn.socket = client;
                    conn.id = nextConnId++;

                    IPaddress* remote = SDLNet_TCP_GetPeerAddress(client);
                    if (remote) {
                        uint32_t host = SDLNet_Read32(&remote->host);
                        uint16_t rport = SDLNet_Read16(&remote->port);
                        conn.label = std::to_string((host >> 24) & 0xFF) + "." +
                                     std::to_string((host >> 16) & 0xFF) + "." +
                                     std::to_string((host >> 8) & 0xFF) + "." +
                                     std::to_string(host & 0xFF) + ":" + std::to_string(rport);
                    } else {
                        conn.label = "unknown";
                    }

                    SDLNet_TCP_AddSocket(socketSet, client);
                    clients.push_back(std::move(conn));
                    mClientCount.store(static_cast<int>(clients.size()));
                    Log("[SERVER] Client connected: " + clients.back().label);

                    // Tell the new client which world names this seed has, so its
                    // 'Start Multiworld Save' UI can validate the chosen name before
                    // claiming. Purely informational: no lock, no full seed pushed.
                    std::string info = BuildSeedInfoJson();
                    if (!info.empty()) {
                        SDLNet_TCP_Send(clients.back().socket, info.c_str(),
                                        static_cast<int>(info.size()) + 1);
                    }
                }
            }
        }

        // Read from existing clients.
        for (size_t i = 0; i < clients.size();) {
            ClientConn& conn = clients[i];
            if (!SDLNet_SocketReady(conn.socket)) {
                ++i;
                continue;
            }

            int len = SDLNet_TCP_Recv(conn.socket, recvBuf, sizeof(recvBuf));
            if (len <= 0) {
                const std::string who = conn.userName.empty() ? conn.label : conn.userName;
                // Free this client's world lock so it can be reclaimed (e.g. on reconnect).
                if (!conn.lockedName.empty()) {
                    ReleaseLocksFor(conn.id);
                    Log("[SERVER] Released world lock for '" + conn.lockedName + "' (" + who + " disconnected)");
                }
                Log("[SERVER] Client disconnected: " + who);
                SDLNet_TCP_DelSocket(socketSet, conn.socket);
                SDLNet_TCP_Close(conn.socket);
                clients.erase(clients.begin() + i);
                mClientCount.store(static_cast<int>(clients.size()));
                continue;
            }

            conn.buffer.append(recvBuf, len);

            // Process every complete '\0'-delimited packet in the buffer.
            size_t delim = conn.buffer.find('\0');
            while (delim != std::string::npos) {
                std::string packet = conn.buffer.substr(0, delim);
                conn.buffer.erase(0, delim + 1);

                std::string summary;
                try {
                    nlohmann::json json = nlohmann::json::parse(packet);
                    // The client sends its chosen name in the connection
                    // handshake; remember it so this client's messages are
                    // labelled with the player name instead of their address.
                    if (json.contains("userName") && json["userName"].is_string()) {
                        std::string name = SanitizeName(json["userName"].get<std::string>());
                        if (!name.empty() && name != conn.userName) {
                            conn.userName = name;
                            Log("[SERVER] " + conn.label + " is now known as " + conn.userName);
                        }
                    }

                    // 'Start Multiworld Save': the client claims a player's world by
                    // name. If the name is in the loaded seed and not already locked to
                    // another client, lock that world to this connection and push the
                    // byte-identical v3 SeedData (base64 in a JSON envelope, since the
                    // transport is NUL-delimited and the seed bytes contain '\0').
                    if (json.value("type", std::string()) == "start_multiworld_save") {
                        std::string reqName;
                        if (json.contains("playerName") && json["playerName"].is_string())
                            reqName = SanitizeName(json["playerName"].get<std::string>());
                        if (reqName.empty()) reqName = conn.userName;  // fall back to handshake name

                        const std::string& who0 = conn.userName.empty() ? conn.label : conn.userName;
                        std::string wire;
                        int world = -1;
                        LockResult res = TryLockWorld(reqName, conn.id, wire, world);
                        if (res == LockResult::Granted) {
                            conn.lockedName = reqName;
                            nlohmann::json resp;
                            resp["type"] = "multiworld_seed";
                            resp["version"] = (int)SeedFile::kVersion;
                            resp["worldId"] = world;
                            resp["playerName"] = reqName;
                            resp["data"] = Base64Encode(wire);
                            std::string out = resp.dump();
                            SDLNet_TCP_Send(conn.socket, out.c_str(), static_cast<int>(out.size()) + 1);
                            Log("[SERVER] Locked world " + std::to_string(world + 1) + " ('" + reqName +
                                "') to " + who0 + "; sent v3 seed (" + std::to_string(wire.size()) + " bytes)");
                        } else {
                            const char* reason = res == LockResult::DeniedLocked      ? "locked"
                                               : res == LockResult::DeniedUnknownName ? "unknown_name"
                                                                                      : "no_seed";
                            nlohmann::json resp;
                            resp["type"] = "multiworld_seed_denied";
                            resp["playerName"] = reqName;
                            resp["reason"] = reason;
                            std::string out = resp.dump();
                            SDLNet_TCP_Send(conn.socket, out.c_str(), static_cast<int>(out.size()) + 1);
                            Log("[SERVER] Refused 'Start Multiworld Save' for '" + reqName + "' from " +
                                who0 + ": " + reason);
                        }
                    }

                    // Reset baseline: the multiworld routing engine is gone, so
                    // incoming hook packets are simply logged (below), not routed.
                    summary = json.dump();
                } catch (const std::exception&) {
                    summary = "(non-JSON) " + packet;
                }
                // Messages relayed from a connected player are tagged with their
                // name (falling back to the address until the name is known).
                const std::string& who = conn.userName.empty() ? conn.label : conn.userName;
                Log("[" + who + "] " + summary);

                delim = conn.buffer.find('\0');
            }
            ++i;
        }
    }

    // Shutdown: close everything cleanly.
    for (ClientConn& conn : clients) {
        SDLNet_TCP_DelSocket(socketSet, conn.socket);
        SDLNet_TCP_Close(conn.socket);
    }
    clients.clear();
    mClientCount.store(0);
    {
        // Session over: drop all world locks (durable persistence is the later F-039).
        std::lock_guard<std::mutex> lock(mSeedMutex);
        mWorldLocks.clear();
    }

    SDLNet_TCP_DelSocket(socketSet, serverSocket);
    SDLNet_TCP_Close(serverSocket);
    SDLNet_FreeSocketSet(socketSet);

    Log("[SERVER] Stopped");
}

void Server::Log(const std::string& line) {
    spdlog::info("{}", line);
    std::lock_guard<std::mutex> lock(mLogMutex);
    mLog.push_back(line);
    while (mLog.size() > MAX_LOG_LINES) {
        mLog.pop_front();
    }
}

std::vector<std::string> Server::LogSnapshot() const {
    std::lock_guard<std::mutex> lock(mLogMutex);
    return std::vector<std::string>(mLog.begin(), mLog.end());
}

void Server::ClearLog() {
    std::lock_guard<std::mutex> lock(mLogMutex);
    mLog.clear();
}

void Server::BroadcastToClients(const std::string& payload) {
    // Just queue it; the server thread delivers it on its next loop iteration.
    std::lock_guard<std::mutex> lock(mOutMutex);
    mPendingBroadcasts.push_back(payload);
}

void Server::UnicastToClient(const std::string& clientName, const std::string& payload) {
    // Just queue it; the server thread delivers it on its next loop iteration.
    std::lock_guard<std::mutex> lock(mOutMutex);
    mPendingUnicasts.emplace_back(clientName, payload);
}

void Server::SetSeed(const SeedFile::SeedData& seed) {
    // Serialize outside the lock — the wire bytes are the EXACT v3 file bytes
    // (file == wire), so a granted request can be answered straight from the cache.
    std::string wire = SeedFile::SerializeToBytes(seed);
    {
        std::lock_guard<std::mutex> lock(mSeedMutex);
        mSeedId = seed.seedId;
        mSeedPlayers = seed.players;
        mSeedWireV3 = std::move(wire);
        mHasSeed = true;
        mWorldLocks.clear();  // a new seed starts a fresh session: drop any prior locks
    }
    // Push the (new) world-name list to any clients already connected when the seed
    // was loaded — the accept path only covers clients that connect afterwards.
    std::string info = BuildSeedInfoJson();
    if (!info.empty()) {
        BroadcastToClients(info);
    }
}

void Server::ClearSeed() {
    std::lock_guard<std::mutex> lock(mSeedMutex);
    mHasSeed = false;
    mSeedId.clear();
    mSeedPlayers.clear();
    mSeedWireV3.clear();
    mWorldLocks.clear();
}

std::string Server::BuildSeedInfoJson() {
    std::lock_guard<std::mutex> lock(mSeedMutex);
    if (!mHasSeed) {
        return std::string();
    }
    nlohmann::json info;
    info["type"] = "multiworld_seed_info";
    info["seedId"] = mSeedId;
    info["players"] = mSeedPlayers;  // index = world id
    return info.dump();
}

Server::LockResult Server::TryLockWorld(const std::string& name, uint64_t connId,
                                        std::string& outWire, int& outWorld) {
    std::lock_guard<std::mutex> lock(mSeedMutex);
    if (!mHasSeed) {
        return LockResult::DeniedNoSeed;
    }
    int world = -1;
    for (size_t i = 0; i < mSeedPlayers.size(); ++i) {
        if (mSeedPlayers[i] == name) { world = static_cast<int>(i); break; }
    }
    if (world < 0) {
        return LockResult::DeniedUnknownName;
    }
    auto it = mWorldLocks.find(name);
    if (it != mWorldLocks.end() && it->second != connId) {
        return LockResult::DeniedLocked;  // held by a different client
    }
    mWorldLocks[name] = connId;  // claim it (or re-affirm for the same connection)
    outWire = mSeedWireV3;
    outWorld = world;
    return LockResult::Granted;
}

void Server::ReleaseLocksFor(uint64_t connId) {
    std::lock_guard<std::mutex> lock(mSeedMutex);
    for (auto it = mWorldLocks.begin(); it != mWorldLocks.end();) {
        if (it->second == connId) {
            it = mWorldLocks.erase(it);
        } else {
            ++it;
        }
    }
}