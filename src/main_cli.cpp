// MultiShip server — headless CLI variant.
//
// Runs the exact same TCP server as the GUI build (src/main.cpp), but without a
// window: the listen port is taken from the command line and all activity is
// printed to the console (via spdlog, the same sink Server::Log already uses).
// Cross-platform: Windows, Linux and macOS.
//
// Usage:
//   MultiShipCLI [PORT]
//   MultiShipCLI --port <PORT>
//   MultiShipCLI -p <PORT>
//
// PORT defaults to 43384 (the SoH MultiShip menu default) and must be in the
// range 1025-65535. Press Ctrl+C to stop.
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include <spdlog/spdlog.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include "Server.h"
#include "rando/SeedFile.h"

namespace {

constexpr int kDefaultPort = 43384;
constexpr int kMinPort = 1025;
constexpr int kMaxPort = 65535;

// Flipped by the signal handler so the main loop can exit cleanly.
std::atomic<bool> gRunning{ true };

void HandleSignal(int /*sig*/) {
    gRunning.store(false);
}

void PrintUsage(const char* exe) {
    std::printf("MultiShip server (headless)\n"
                "Usage: %s [PORT] [--seed <file.multiship>]\n"
                "       %s --port <PORT> --seed <file.multiship>\n"
                "PORT must be %d-%d (default %d).\n"
                "--seed loads an existing .multiship seed (the placement table the server\n"
                "       routes items from). Press Ctrl+C to stop.\n",
                exe, exe, kMinPort, kMaxPort, kDefaultPort);
}

// Parses a base-10 port from a complete string. Returns false unless the whole
// string is a valid integer (so "12ab" or "" are rejected).
bool ParsePort(const std::string& text, int& outPort) {
    if (text.empty()) {
        return false;
    }
    try {
        size_t consumed = 0;
        int value = std::stoi(text, &consumed);
        if (consumed != text.size()) {
            return false;
        }
        outPort = value;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace

int main(int argc, char** argv) {
    SDL_SetMainReady();

    int port = kDefaultPort;
    std::string seedPath;

    // Read the port from the command line. Supports a bare positional argument
    // ("MultiShipCLI 43384") as well as "--port N", "--port=N" and "-p N", plus
    // "--seed <path>" to load an existing .multiship seed.
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            PrintUsage(argv[0]);
            return 0;
        }

        bool ok = false;
        if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) ok = ParsePort(argv[++i], port);
        } else if (arg.rfind("--port=", 0) == 0) {
            ok = ParsePort(arg.substr(std::strlen("--port=")), port);
        } else if (arg == "--seed" || arg == "--multiship") {
            if (i + 1 < argc) { seedPath = argv[++i]; ok = true; }
        } else if (arg.rfind("--seed=", 0) == 0) {
            seedPath = arg.substr(std::strlen("--seed=")); ok = true;
        } else {
            ok = ParsePort(arg, port);
        }

        if (!ok) {
            std::fprintf(stderr, "Invalid argument: %s\n\n", argv[i]);
            PrintUsage(argv[0]);
            return 1;
        }
    }

    // Load the seed file if given (the server's placement table).
    SeedFile::Loaded loadedSeed;
    bool haveSeed = false;
    if (!seedPath.empty()) {
        std::string err;
        if (!SeedFile::ReadMultiship(seedPath, loadedSeed, err)) {
            std::fprintf(stderr, "Failed to load seed '%s': %s\n", seedPath.c_str(), err.c_str());
            return 1;
        }
        haveSeed = true;
    }

    if (port < kMinPort || port > kMaxPort) {
        std::fprintf(stderr, "Port %d is out of range (%d-%d).\n", port, kMinPort, kMaxPort);
        return 1;
    }

    if (SDLNet_Init() == -1) {
        spdlog::error("SDLNet_Init failed: {}", SDLNet_GetError());
        return 1;
    }

    Server server;
    if (!server.Start(static_cast<uint16_t>(port))) {
        // Server::Start already logged the reason it couldn't bind.
        SDLNet_Quit();
        return 1;
    }

    // Stop gracefully on Ctrl+C (SIGINT) and on termination (SIGTERM).
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    spdlog::info("[SERVER] MultiShip CLI running on port {}. Press Ctrl+C to stop.", port);
    if (haveSeed) {
        std::string who;
        for (size_t i = 0; i < loadedSeed.players.size(); ++i)
            who += (i ? ", " : "") + loadedSeed.players[i];
        spdlog::info("[SERVER] Loaded seed {} ({} worlds: {}), {} placements.", loadedSeed.seed,
                     loadedSeed.numWorlds, who, loadedSeed.placements.size());
        // Arm multiworld routing. The .session (collection history) sits next to the
        // .multiship so deliveries survive a server restart.
        std::string sessionPath = seedPath;
        const std::string ext = ".multiship";
        if (sessionPath.size() >= ext.size() &&
            sessionPath.compare(sessionPath.size() - ext.size(), ext.size(), ext) == 0)
            sessionPath = sessionPath.substr(0, sessionPath.size() - ext.size()) + ".session";
        else
            sessionPath += ".session";
        server.LoadSession(loadedSeed, sessionPath);
    }

    // The server runs on its own thread and logs activity through spdlog (which
    // goes to the console), so we simply idle until interrupted or it stops.
    while (gRunning.load() && server.IsRunning()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    spdlog::info("[SERVER] Shutting down...");
    server.Stop();
    SDLNet_Quit();
    return 0;
}
