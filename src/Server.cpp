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
    std::string label; // host:port for logging
};
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
        Log(std::string("[Error] SDLNet_ResolveHost: ") + SDLNet_GetError());
        return false;
    }
    TCPsocket test = SDLNet_TCP_Open(&probe);
    if (!test) {
        Log(std::string("[Error] Could not bind port ") + std::to_string(port) + ": " + SDLNet_GetError());
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
        Log(std::string("[Error] SDLNet_ResolveHost: ") + SDLNet_GetError());
        mRunning.store(false);
        return;
    }

    TCPsocket serverSocket = SDLNet_TCP_Open(&ip);
    if (!serverSocket) {
        Log(std::string("[Error] SDLNet_TCP_Open: ") + SDLNet_GetError());
        mRunning.store(false);
        return;
    }

    // Socket set holds the listening socket plus up to MAX_CLIENTS clients.
    SDLNet_SocketSet socketSet = SDLNet_AllocSocketSet(MAX_CLIENTS + 1);
    SDLNet_TCP_AddSocket(socketSet, serverSocket);

    std::vector<ClientConn> clients;

    Log("[Server] Listening on port " + std::to_string(port) + " (all interfaces)");

    char recvBuf[1024];
    while (mRunning.load()) {
        // Wait up to 100ms for activity so we can re-check mRunning regularly.
        int ready = SDLNet_CheckSockets(socketSet, 100);
        if (ready <= 0) {
            continue;
        }

        // New incoming connection.
        if (SDLNet_SocketReady(serverSocket)) {
            TCPsocket client = SDLNet_TCP_Accept(serverSocket);
            if (client) {
                if (static_cast<int>(clients.size()) >= MAX_CLIENTS) {
                    Log("[Server] Connection refused (client limit reached)");
                    SDLNet_TCP_Close(client);
                } else {
                    ClientConn conn;
                    conn.socket = client;

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
                    Log("[Server] Client connected: " + clients.back().label);
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
                Log("[Server] Client disconnected: " + conn.label);
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
                    summary = json.dump();
                } catch (const std::exception&) {
                    summary = "(non-JSON) " + packet;
                }
                Log("[" + conn.label + "] " + summary);

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

    SDLNet_TCP_DelSocket(socketSet, serverSocket);
    SDLNet_TCP_Close(serverSocket);
    SDLNet_FreeSocketSet(socketSet);

    Log("[Server] Stopped");
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
