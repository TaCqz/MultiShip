#ifndef MULTISHIP_SERVER_H
#define MULTISHIP_SERVER_H

#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

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

  private:
    void Run(uint16_t port);
    void Log(const std::string& line);

    std::thread mThread;
    std::atomic<bool> mRunning{ false };
    std::atomic<int> mClientCount{ 0 };

    mutable std::mutex mLogMutex;
    std::deque<std::string> mLog;
    static constexpr size_t MAX_LOG_LINES = 500;
};

#endif // MULTISHIP_SERVER_H
