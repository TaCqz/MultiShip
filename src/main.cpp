// MultiShip server — ImGui front-end. First choose to CREATE a new (empty) seed
// or START the server with an existing .multiship seed, then run the server that the
// ShipwreckCombo MultiShip module connects to.
//
// Reset baseline: the randomization engine is gone. "Create a new seed" writes an
// empty .multiship (header + seed + player names, zero placements/settings); the
// Create screen offers only player names + presets. The server is a relay; under
// MULTISHIP_DEBUG it also exposes a manual give-item picker and a region teleport.
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <vector>

#include "Server.h"
#include "rando/SeedFile.h"
#include "rando/ItemNames.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#pragma comment(lib, "comdlg32.lib")   // GetOpenFileNameA
#pragma comment(lib, "shell32.lib")    // ShellExecuteA
#endif

// The folder scanned for preset templates (and the default place new presets land).
static const char* kPresetDir = "presets";

// Native open-file dialog for a .multiship seed. Returns "" if cancelled/unsupported.
// (Win32 GetOpenFileName — dependency-free; swap for tinyfiledialogs for cross-platform.)
static std::string PickMultishipFile() {
#ifdef _WIN32
    char path[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "MultiShip Seed (*.multiship)\0*.multiship\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) return std::string(path);
#endif
    return std::string();
}

// Native save / open dialogs for a settings-preset JSON. Return "" if cancelled/unsupported.
static std::string PickSavePresetFile() {
#ifdef _WIN32
    char path[MAX_PATH] = "preset.mspreset.json";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "MultiShip Preset (*.mspreset.json)\0*.mspreset.json\0All Files\0*.*\0";
    ofn.lpstrDefExt = "json";
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn)) return std::string(path);
#endif
    return std::string();
}
static std::string PickOpenPresetFile() {
#ifdef _WIN32
    char path[MAX_PATH] = "";
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "MultiShip Preset (*.mspreset.json)\0*.mspreset.json\0All Files\0*.*\0";
    ofn.lpstrFile = path;
    ofn.nMaxFile = sizeof(path);
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) return std::string(path);
#endif
    return std::string();
}

// --- Settings presets ----------------------------------------------------------------
// With the randomizer settings gone, a preset effectively round-trips the player
// names. The JSON shape/plumbing is kept (version + players + an empty settings
// object, keyed by RSK enum name when settings return), so existing tooling and a
// future settings block stay compatible.
static bool SavePreset(const std::string& path, const char playerNames[2][64], std::string& err) {
    nlohmann::json j;
    j["multiship_preset_version"] = 1;
    j["players"] = { std::string(playerNames[0]), std::string(playerNames[1]) };
    j["settings"] = nlohmann::json::object();  // none in the reset baseline

    const std::string text = j.dump(2);
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) { err = "could not open file for writing"; return false; }
    size_t n = std::fwrite(text.data(), 1, text.size(), f);
    std::fclose(f);
    if (n != text.size()) { err = "short write"; return false; }
    return true;
}

// Restores whichever player names the preset carries. Returns false only on
// read/parse failure (a preset with no "players" array is still a success).
static bool LoadPreset(const std::string& path, char playerNames[2][64], std::string& err) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) { err = "could not open file"; return false; }
    std::string text;
    char buf[4096];
    size_t n;
    while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) text.append(buf, n);
    std::fclose(f);

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(text);
    } catch (const std::exception& e) {
        err = std::string("invalid JSON: ") + e.what();
        return false;
    }

    if (j.contains("players") && j["players"].is_array()) {
        const auto& p = j["players"];
        for (size_t k = 0; k < 2 && k < p.size(); ++k)
            if (p[k].is_string())
                std::snprintf(playerNames[k], 64, "%s", p[k].get<std::string>().c_str());
    }
    return true;
}

// Lists *.mspreset.json files in the preset folder (full paths), for the template
// dropdown. Missing folder -> empty list (not an error).
static std::vector<std::string> ScanPresetTemplates() {
    std::vector<std::string> out;
    std::error_code ec;
    if (!std::filesystem::is_directory(kPresetDir, ec)) return out;
    for (const auto& entry : std::filesystem::directory_iterator(kPresetDir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        const std::string name = entry.path().filename().string();
        if (name.size() >= 14 &&
            name.compare(name.size() - 14, 14, ".mspreset.json") == 0)
            out.push_back(entry.path().string());
    }
    std::sort(out.begin(), out.end());
    return out;
}

// Opens the OS file browser at `path` with that file selected (Windows only).
static void OpenFileLocation(const std::string& path) {
#ifdef _WIN32
    char full[MAX_PATH];
    if (GetFullPathNameA(path.c_str(), MAX_PATH, full, nullptr)) {
        std::string arg = "/select,\"" + std::string(full) + "\"";
        ShellExecuteA(nullptr, "open", "explorer.exe", arg.c_str(), nullptr, SW_SHOWNORMAL);
    }
#endif
}

enum class Screen { Menu, Create, ServerView };

#ifdef MULTISHIP_DEBUG
// Debug teleport destinations. Each maps a human label to a SoH entrance index (the
// argument the client's `entrance <hex>` console command takes). Indices are the primary
// overworld entrance for each region, from ShipwreckCombo soh/include/tables/entrance_table.h.
// Age-swapped scenes resolve per the player's current age — e.g. "Castle Grounds" (0x138,
// SCENE_HYRULE_CASTLE) lands an adult at Outside Ganon's Castle, where the rainbow bridge is.
struct WarpDest { const char* label; int entrance; };
static const WarpDest kWarpDests[] = {
    { "Kokiri Forest",                   0x0EE },
    { "Lost Woods",                      0x11E },
    { "Sacred Forest Meadow",            0x0FC },
    { "Hyrule Field",                    0x0CD },
    { "Lon Lon Ranch",                   0x157 },
    { "Market (Castle Town)",            0x0B1 },
    { "Temple of Time",                  0x053 },
    { "Castle Grounds / Ganon's Castle", 0x138 },  // adult -> Outside Ganon's Castle (bridge)
    { "Kakariko Village",                0x0DB },
    { "Graveyard",                       0x0E4 },
    { "Death Mountain Trail",            0x13D },
    { "Death Mountain Crater",           0x147 },
    { "Goron City",                      0x14D },
    { "Zora's River",                    0x0EA },
    { "Zora's Domain",                   0x108 },
    { "Zora's Fountain",                 0x10E },
    { "Lake Hylia",                      0x102 },
    { "Gerudo Valley",                   0x117 },
    { "Gerudo's Fortress",               0x129 },
    { "Haunted Wasteland",               0x369 },
    { "Desert Colossus",                 0x123 },
};
#endif

int main(int, char**) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        spdlog::error("SDL_Init failed: {}", SDL_GetError());
        return 1;
    }
    if (SDLNet_Init() == -1) {
        spdlog::error("SDLNet_Init failed: {}", SDLNet_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("MultiShip Server", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          720, 540, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        spdlog::error("SDL window/renderer failed: {}", SDL_GetError());
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    Server server;
    Screen screen = Screen::Menu;

    // --- menu state ---
    std::string menuError;         // shown in red on the menu (e.g. failed seed load)

    // --- create-seed state (2 players for now; designed to extend to N) ---
    char playerNames[2][64] = { "Player 1", "Player 2" };
    std::string createStatus;
    std::string presetStatus;      // feedback for Save/Load Preset

    // Preset templates discovered in kPresetDir, refreshed on demand.
    std::vector<std::string> presetTemplates = ScanPresetTemplates();
    int presetTemplateIdx = -1;

    bool seedReady = false;        // a seed has been created or loaded
    uint64_t activeSeed = 0;
    std::vector<std::string> activePlayers;
    int activePlacements = 0;
    std::string activeSource;      // where it came from (created file / loaded path)

    // --- server-view state ---
    int port = 43384;
    bool autoScroll = true;
#ifdef MULTISHIP_DEBUG
    int itemIdx = -1;             // index into ItemCatalog::Items() (-1 = none chosen)
    char playerName[64] = "";
    int warpDestIdx = 7;          // default to "Castle Grounds / Ganon's Castle" (bridge test)
#endif

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                running = false;
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("MultiShip", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        // ===================================================================== MENU
        if (screen == Screen::Menu) {
            ImGui::TextUnformatted("MultiShip");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped("Choose what to do:");
            ImGui::Spacing();
            if (ImGui::Button("Create a new seed", ImVec2(260, 40))) {
                createStatus.clear();
                presetStatus.clear();
                menuError.clear();
                presetTemplates = ScanPresetTemplates();
                presetTemplateIdx = -1;
                screen = Screen::Create;
            }
            ImGui::Spacing();
            if (ImGui::Button("Start server with existing seed", ImVec2(260, 40))) {
                menuError.clear();
                std::string path = PickMultishipFile();
                if (!path.empty()) {
                    SeedFile::Loaded ld;
                    std::string err;
                    if (SeedFile::ReadMultiship(path, ld, err)) {
                        seedReady = true;
                        activeSeed = ld.seed;
                        activePlayers = ld.players;
                        activePlacements = (int)ld.placements.size();
                        activeSource = path;
                        screen = Screen::ServerView;
                    } else {
                        menuError = "Failed to load: " + err;
                    }
                }
            }
            if (!menuError.empty()) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.9f, 0.4f, 0.3f, 1.0f), "%s", menuError.c_str());
            }
        }

        // =================================================================== CREATE
        else if (screen == Screen::Create) {
            ImGui::TextUnformatted("Create a new seed");
            ImGui::Separator();
            ImGui::Spacing();

            // --- Presets (top): pick a template, or save/load a preset file --------
            ImGui::SeparatorText("Presets");
            {
                const char* preview = (presetTemplateIdx >= 0 && presetTemplateIdx < (int)presetTemplates.size())
                                          ? presetTemplates[presetTemplateIdx].c_str()
                                          : "(select a template)";
                ImGui::SetNextItemWidth(360.0f);
                if (ImGui::BeginCombo("Template", preview)) {
                    for (int i = 0; i < (int)presetTemplates.size(); ++i) {
                        const std::string label =
                            std::filesystem::path(presetTemplates[i]).filename().string();
                        const bool sel = (presetTemplateIdx == i);
                        ImGui::PushID(i);
                        if (ImGui::Selectable(label.c_str(), sel)) {
                            presetTemplateIdx = i;
                            std::string err;
                            if (LoadPreset(presetTemplates[i], playerNames, err))
                                presetStatus = "Loaded template " + label;
                            else
                                presetStatus = "Load failed: " + err;
                        }
                        if (sel) ImGui::SetItemDefaultFocus();
                        ImGui::PopID();
                    }
                    if (presetTemplates.empty())
                        ImGui::TextDisabled("(no *.mspreset.json in '%s')", kPresetDir);
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                if (ImGui::Button("Refresh")) {
                    presetTemplates = ScanPresetTemplates();
                    presetTemplateIdx = -1;
                }
                ImGui::SameLine();
                if (ImGui::Button("Save Preset")) {
                    std::string path = PickSavePresetFile();
                    if (!path.empty()) {
                        std::string err;
                        if (SavePreset(path, playerNames, err)) {
                            presetStatus = "Saved preset -> " + path;
                            presetTemplates = ScanPresetTemplates();
                        } else {
                            presetStatus = "Save failed: " + err;
                        }
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Load Preset")) {
                    std::string path = PickOpenPresetFile();
                    if (!path.empty()) {
                        std::string err;
                        if (LoadPreset(path, playerNames, err))
                            presetStatus = "Loaded preset from " + path;
                        else
                            presetStatus = "Load failed: " + err;
                    }
                }
                if (!presetStatus.empty())
                    ImGui::TextDisabled("%s", presetStatus.c_str());
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Players");
            ImGui::TextWrapped("Player names (2 worlds):");
            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputText("Player 1", playerNames[0], sizeof(playerNames[0]));
            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputText("Player 2", playerNames[1], sizeof(playerNames[1]));

            ImGui::Spacing();
            ImGui::Separator();
            if (ImGui::Button("Create Seed", ImVec2(160, 32))) {
                createStatus.clear();
                seedReady = false;
                std::random_device rd;
                uint64_t seed = ((uint64_t)rd() << 32) ^ rd();
                std::vector<std::string> players = { playerNames[0], playerNames[1] };
                // Build the file name as "<player1>_<player2>_<seed>" inside a "seeds/"
                // subfolder. Player names are sanitized to safe filename characters
                // (alphanumerics, '-', '.'); anything else is dropped, and an empty name
                // falls back to "Player".
                auto sanitize = [](const std::string& s) {
                    std::string out;
                    for (char c : s) {
                        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                            c == '-' || c == '.') {
                            out += c;
                        }
                    }
                    return out.empty() ? std::string("Player") : out;
                };
                std::string namePart = sanitize(playerNames[0]) + "_" + sanitize(playerNames[1]);
                std::error_code mkdirEc;
                std::filesystem::create_directories("seeds", mkdirEc);
                std::string base =
                    (std::filesystem::path("seeds") / (namePart + "_" + std::to_string(seed))).string();
                std::string err;
                bool okBin = SeedFile::WriteMultiship(base + ".multiship", seed, players, err);
                bool okTxt = SeedFile::WriteSpoiler(base + "_spoiler.txt", seed, players, err);
                if (okBin && okTxt) {
                    // Read the file back so the Server view reflects exactly what was written.
                    SeedFile::Loaded ld;
                    std::string lerr;
                    if (SeedFile::ReadMultiship(base + ".multiship", ld, lerr)) {
                        seedReady = true;
                        activeSeed = ld.seed;
                        activePlayers = ld.players;
                        activePlacements = (int)ld.placements.size();
                        activeSource = base + ".multiship";
                        createStatus = "Created seed " + base + "  ->  " + base +
                                       ".multiship  +  _spoiler.txt";
                    } else {
                        createStatus = "Created files but failed to read back: " + lerr;
                    }
                } else {
                    createStatus = "Write failed: " + err;
                }
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(!seedReady);
            if (ImGui::Button("Continue to server", ImVec2(180, 32))) {
                screen = Screen::ServerView;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Back", ImVec2(80, 32))) screen = Screen::Menu;

            if (!createStatus.empty()) {
                ImGui::Spacing();
                ImGui::TextWrapped("%s", createStatus.c_str());
                if (seedReady) {
                    if (ImGui::Button("Open file location")) OpenFileLocation(activeSource);
                }
            }
        }

        // =============================================================== SERVER VIEW
        else if (screen == Screen::ServerView) {
            ImGui::TextUnformatted("MultiShip Server");
            ImGui::Separator();
            if (seedReady) {
                ImGui::Text("Seed %llu", (unsigned long long)activeSeed);
                std::string who;
                for (size_t i = 0; i < activePlayers.size(); ++i)
                    who += (i ? ", " : "") + activePlayers[i];
                ImGui::Text("Players: %s   |   %d placements", who.c_str(), activePlacements);
                ImGui::TextDisabled("%s", activeSource.c_str());
                if (!activeSource.empty()) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Open file location")) OpenFileLocation(activeSource);
                }
                ImGui::Separator();
            }

            const bool isRunning = server.IsRunning();
            ImGui::BeginDisabled(isRunning);
            ImGui::SetNextItemWidth(120.0f);
            ImGui::InputInt("Port", &port);
            if (port < 1025) port = 1025;
            if (port > 65535) port = 65535;
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (!isRunning) {
                if (ImGui::Button("Start Server")) server.Start(static_cast<uint16_t>(port));
            } else {
                if (ImGui::Button("Stop Server")) server.Stop();
            }
            if (isRunning) {
                ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Running on port %d  -  clients: %d", port,
                                   server.ClientCount());
            } else {
                ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Stopped");
            }

#ifdef MULTISHIP_DEBUG
            // Debug-only tools (built with -DDEBUG=1): manual give-item, the player target,
            // and the region teleport. Hidden in normal builds so players can't self-serve.
            const auto& items = ItemCatalog::Items();
            ImGui::BeginDisabled(!isRunning || server.ClientCount() == 0);
            const char* itemPreview = (itemIdx >= 0 && itemIdx < (int)items.size())
                                          ? items[itemIdx].name : "(select item)";
            ImGui::SetNextItemWidth(260.0f);
            if (ImGui::BeginCombo("Item", itemPreview)) {
                for (int i = 0; i < (int)items.size(); ++i) {
                    if (items[i].rg == RG_NONE || items[i].rg == RG_MAX) continue;
                    const bool sel = (itemIdx == i);
                    ImGui::PushID(i);
                    if (ImGui::Selectable(items[i].name, sel)) itemIdx = i;
                    if (sel) ImGui::SetItemDefaultFocus();
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            ImGui::InputText("Player (empty = everyone)", playerName, sizeof(playerName));
            ImGui::BeginDisabled(itemIdx < 0);
            if (ImGui::Button("Send Item")) {
                nlohmann::json payload;
                payload["type"] = "command";
                payload["command"] = std::string("give_item randomizer ") + items[itemIdx].name;
                if (playerName[0] != '\0') server.UnicastToClient(playerName, payload.dump());
                else server.BroadcastToClients(payload.dump());
            }
            ImGui::EndDisabled();
            ImGui::EndDisabled();

            // Teleport: warp the targeted player (the "Player" field above; empty = everyone)
            // to a region. Sends the client's `entrance <hex>` console command. The player must
            // be in-game (a loaded save) for the warp to take — it no-ops on the title/menu.
            ImGui::Separator();
            ImGui::BeginDisabled(!isRunning || server.ClientCount() == 0);
            ImGui::SetNextItemWidth(260.0f);
            if (ImGui::BeginCombo("Teleport region", kWarpDests[warpDestIdx].label)) {
                for (int i = 0; i < (int)(sizeof(kWarpDests) / sizeof(kWarpDests[0])); ++i) {
                    if (ImGui::Selectable(kWarpDests[i].label, warpDestIdx == i)) warpDestIdx = i;
                    if (warpDestIdx == i) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            ImGui::SameLine();
            if (ImGui::Button("Teleport")) {
                char cmd[32];
                std::snprintf(cmd, sizeof(cmd), "entrance %X", kWarpDests[warpDestIdx].entrance);
                nlohmann::json payload;
                payload["type"] = "command";
                payload["command"] = cmd;
                if (playerName[0] != '\0') server.UnicastToClient(playerName, payload.dump());
                else server.BroadcastToClients(payload.dump());
            }
            ImGui::EndDisabled();
#endif // MULTISHIP_DEBUG

            ImGui::Separator();
            ImGui::Checkbox("Auto-scroll", &autoScroll);
            ImGui::SameLine();
            if (ImGui::Button("Clear log")) server.ClearLog();
            ImGui::SameLine();
            ImGui::BeginDisabled(isRunning);
            if (ImGui::Button("Back to menu")) screen = Screen::Menu;
            ImGui::EndDisabled();

            ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
            for (const std::string& line : server.LogSnapshot()) ImGui::TextUnformatted(line.c_str());
            if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
        }

        ImGui::End();

        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    server.Stop();
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDLNet_Quit();
    SDL_Quit();
    return 0;
}
