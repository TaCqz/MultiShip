// MultiShip server — ImGui front-end. First choose to CREATE a new multiworld seed
// or START the server with an existing .multiship seed, then run the server that the
// ShipwreckCombo MultiShip module connects to.
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "Server.h"
#include "IceTrap.h"
#include "rando/Fill.h"
#include "rando/SeedFile.h"
#include "rando/SettingsSpec.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#pragma comment(lib, "comdlg32.lib")   // GetOpenFileNameA
#pragma comment(lib, "shell32.lib")    // ShellExecuteA
#endif

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

// The .session file (server's collection history) sits next to the .multiship:
// "foo.multiship" -> "foo.session", anything else -> "<path>.session".
static std::string SessionPathFor(const std::string& multishipPath) {
    const std::string ext = ".multiship";
    if (multishipPath.size() >= ext.size() &&
        multishipPath.compare(multishipPath.size() - ext.size(), ext.size(), ext) == 0) {
        return multishipPath.substr(0, multishipPath.size() - ext.size()) + ".session";
    }
    return multishipPath + ".session";
}

enum class Screen { Menu, Create, ServerView };

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
    // One chosen value per implemented setting (see Rando::MultiShipSettings()),
    // initialized to each setting's default.
    std::vector<uint8_t> settingValues;
    for (const auto& spec : Rando::MultiShipSettings()) settingValues.push_back(spec.defaultValue);
    char settingsFilter[64] = "";  // live search box over the (large) settings list
    std::vector<Fill::SettingOverride> genOverrides;  // captured at Create time for the worker
    std::string createStatus;
    bool seedReady = false;        // a seed has been created or loaded
    uint64_t activeSeed = 0;
    std::vector<std::string> activePlayers;
    int activePlacements = 0;
    std::string activeSource;      // where it came from (created file / loaded path)

    // --- background generation (Fill::Generate blocks, so it runs off the UI thread) ---
    std::thread genThread;
    std::atomic<bool> genRunning{ false };   // worker active (UI shows the progress bar)
    std::atomic<bool> genFinished{ false };  // worker done; UI joins + finalizes
    std::atomic<float> genFrac{ 0.0f };
    std::mutex genStageMutex;
    std::string genStage;
    uint64_t genSeed = 0;
    std::vector<std::string> genPlayers;
    Fill::Result genResult;
    bool genWriteOk = false;
    std::string genWriteErr;
    std::string genBase;

    // --- server-view state ---
    int port = 43384;
    bool autoScroll = true;
    char itemName[64] = "";
    char playerName[64] = "";
    int iceModelIdx = -1;
    int iceTextIdx = -1;

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

        // Generation finished on the worker thread -> join and publish the result.
        if (genFinished.load() && genThread.joinable()) {
            genThread.join();
            genRunning = false;
            genFinished = false;
            genBase = std::to_string(genSeed);
            if (genWriteOk) {
                createStatus = "Created seed " + genBase +
                               (genResult.beatable ? "  (beatable)" : "  (NOT beatable!)") +
                               "  ->  " + genBase + ".multiship  +  _spoiler.txt";
                seedReady = true;
                activeSeed = genSeed;
                activePlayers = genPlayers;
                activePlacements = (int)genResult.placements.size();
                activeSource = genBase + ".multiship";

                // Arm multiworld routing for the freshly created seed.
                SeedFile::Loaded ld;
                ld.version = 1;
                ld.seed = genSeed;
                ld.numWorlds = (int)genPlayers.size();
                ld.players = genPlayers;
                ld.placements = genResult.placements;
                server.LoadSession(ld, genBase + ".session");
            } else {
                createStatus = "Write failed: " + genWriteErr;
            }
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
            ImGui::TextUnformatted("MultiShip — MultiWorld");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped("Choose what to do:");
            ImGui::Spacing();
            if (ImGui::Button("Create a new seed", ImVec2(260, 40))) {
                createStatus.clear();
                menuError.clear();
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
                        server.LoadSession(ld, SessionPathFor(path));
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
            const bool busy = genRunning.load();

            ImGui::TextUnformatted("Create a new MultiWorld seed");
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextWrapped("Player names (2 worlds):");
            ImGui::BeginDisabled(busy);
            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputText("Player 1", playerNames[0], sizeof(playerNames[0]));
            ImGui::SetNextItemWidth(260.0f);
            ImGui::InputText("Player 2", playerNames[1], sizeof(playerNames[1]));
            ImGui::EndDisabled();
            ImGui::Spacing();

            // --- Settings (full set, in tabs like the base randomizer) ------------
            ImGui::SeparatorText("Settings");
            {
                const auto& specs = Rando::MultiShipSettings();
                ImGui::SetNextItemWidth(260.0f);
                ImGui::InputTextWithHint("##settingsfilter", "Filter settings...", settingsFilter,
                                         sizeof(settingsFilter));
                ImGui::SameLine();
                ImGui::TextDisabled("(%zu settings)", specs.size());

                // Lower-cased filter for a case-insensitive substring match.
                std::string flt = settingsFilter;
                for (char& ch : flt) ch = (char)std::tolower((unsigned char)ch);

                auto passesFilter = [&](const Rando::SettingSpec& sp) {
                    if (flt.empty()) return true;
                    std::string lbl = sp.label;
                    for (char& ch : lbl) ch = (char)std::tolower((unsigned char)ch);
                    return lbl.find(flt) != std::string::npos;
                };

                // Draw one setting. On/Off toggles render as a single-line checkbox;
                // everything else as a labelled full-width slider/dropdown. The whole
                // control is grouped so its SoH description shows as a hover tooltip.
                auto renderSetting = [&](size_t i) {
                    const Rando::SettingSpec& sp = specs[i];
                    ImGui::PushID((int)i);
                    ImGui::BeginGroup();
                    if (sp.IsToggle()) {
                        const uint8_t on = sp.OnValue();
                        bool checked = (settingValues[i] == on);
                        if (ImGui::Checkbox(sp.label, &checked))
                            settingValues[i] = checked ? on : (uint8_t)(on ? 0 : 1);
                    } else {
                        ImGui::TextUnformatted(sp.label);
                        ImGui::SetNextItemWidth(-FLT_MIN);
                        if (sp.IsNumeric()) {
                            int v = settingValues[i];
                            if (ImGui::SliderInt("##w", &v, sp.numMin, sp.numMax))
                                settingValues[i] = (uint8_t)v;
                        } else {
                            const char* cur = "?";
                            for (const auto& c : sp.choices)
                                if (c.value == settingValues[i]) cur = c.label;
                            if (ImGui::BeginCombo("##w", cur)) {
                                for (const auto& c : sp.choices) {
                                    const bool sel = (c.value == settingValues[i]);
                                    if (ImGui::Selectable(c.label, sel)) settingValues[i] = c.value;
                                    if (sel) ImGui::SetItemDefaultFocus();
                                }
                                ImGui::EndCombo();
                            }
                        }
                    }
                    ImGui::EndGroup();
                    if (sp.tooltip && sp.tooltip[0] && ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 28.0f);
                        ImGui::TextUnformatted(sp.tooltip);
                        ImGui::PopTextWrapPos();
                        ImGui::EndTooltip();
                    }
                    ImGui::PopID();
                };

                // Reserve space at the bottom for the action bar, so the tabs fill the
                // rest of the window and the buttons stay anchored at the bottom.
                float footer = 52.0f;
                if (busy) footer += 50.0f;
                if (!createStatus.empty()) footer += 66.0f;
                float settingsH = ImGui::GetContentRegionAvail().y - footer;
                if (settingsH < 160.0f) settingsH = 160.0f;

                ImGui::BeginDisabled(busy);
                ImGui::BeginChild("ms_settings_area", ImVec2(0, settingsH), ImGuiChildFlags_Borders);
                if (ImGui::BeginTabBar("ms_tabs")) {
                    for (const char* tab : Rando::SettingTabOrder()) {
                        if (!ImGui::BeginTabItem(tab)) continue;
                        ImGui::BeginChild("tabscroll", ImVec2(0, 0));

                        // How many logical columns does this tab use (per the OG menu)?
                        int numCols = 1;
                        for (const auto& sp : specs)
                            if (std::strcmp(sp.tab, tab) == 0 && passesFilter(sp) &&
                                (int)sp.column + 1 > numCols)
                                numCols = (int)sp.column + 1;

                        // Responsive: shrink to fit narrow windows (each column wants ~260px).
                        float avail = ImGui::GetContentRegionAvail().x;
                        int displayCols = numCols;
                        while (displayCols > 1 && avail / displayCols < 260.0f) --displayCols;

                        if (ImGui::BeginTable("t", displayCols,
                                              ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_PadOuterX)) {
                            ImGui::TableNextRow();
                            for (int tc = 0; tc < displayCols; ++tc) {
                                ImGui::TableNextColumn();
                                // Logical columns assigned to this table cell (round-robin
                                // when the window is too narrow to show them all side by side).
                                for (int lc = tc; lc < numCols; lc += displayCols) {
                                    const char* curSection = nullptr;
                                    for (size_t i = 0; i < specs.size() && i < settingValues.size(); ++i) {
                                        const Rando::SettingSpec& sp = specs[i];
                                        if (std::strcmp(sp.tab, tab) != 0 || (int)sp.column != lc) continue;
                                        if (!passesFilter(sp)) continue;
                                        if (sp.section && sp.section[0] &&
                                            (!curSection || std::strcmp(curSection, sp.section) != 0)) {
                                            ImGui::SeparatorText(sp.section);
                                        }
                                        curSection = sp.section;
                                        renderSetting(i);
                                    }
                                }
                            }
                            ImGui::EndTable();
                        }
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndChild();
                ImGui::EndDisabled();
            }
            ImGui::Separator();

            ImGui::BeginDisabled(busy);
            if (ImGui::Button("Create Seed", ImVec2(160, 32))) {
                // Reset progress + capture inputs, then generate off the UI thread so
                // the window keeps redrawing the progress bar.
                createStatus.clear();
                genFrac = 0.0f;
                { std::lock_guard<std::mutex> lk(genStageMutex); genStage = "Starting..."; }
                std::random_device rd;
                genSeed = ((uint64_t)rd() << 32) ^ rd();
                genPlayers = { playerNames[0], playerNames[1] };
                // Snapshot the chosen settings as overrides for the worker thread.
                genOverrides.clear();
                {
                    const auto& specs = Rando::MultiShipSettings();
                    for (size_t i = 0; i < specs.size() && i < settingValues.size(); ++i)
                        genOverrides.push_back({ (uint16_t)specs[i].key, settingValues[i] });
                }
                seedReady = false;
                genFinished = false;
                genRunning = true;
                genThread = std::thread([&] {
                    Fill::Result r = Fill::Generate(genSeed, 2, genOverrides, [&](float f, const char* s) {
                        genFrac = f;
                        std::lock_guard<std::mutex> lk(genStageMutex);
                        genStage = s;
                    });
                    {
                        std::lock_guard<std::mutex> lk(genStageMutex);
                        genStage = "Writing seed files...";
                    }
                    std::string base = std::to_string(genSeed);
                    std::string err;
                    bool okBin = SeedFile::WriteMultiship(base + ".multiship", r, genPlayers, err);
                    bool okTxt = SeedFile::WriteSpoiler(base + "_spoiler.txt", r, genPlayers, err);
                    genResult = std::move(r);
                    genWriteOk = okBin && okTxt;
                    genWriteErr = err;
                    genFrac = 1.0f;
                    {
                        std::lock_guard<std::mutex> lk(genStageMutex);
                        genStage = "Done";
                    }
                    genFinished = true;
                });
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(busy || !seedReady);
            if (ImGui::Button("Continue to server", ImVec2(180, 32))) {
                screen = Screen::ServerView;
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(busy);
            if (ImGui::Button("Back", ImVec2(80, 32))) screen = Screen::Menu;
            ImGui::EndDisabled();

            // Live progress while the worker runs.
            if (busy) {
                ImGui::Spacing();
                std::string stage;
                { std::lock_guard<std::mutex> lk(genStageMutex); stage = genStage; }
                float frac = genFrac.load();
                char overlay[32];
                std::snprintf(overlay, sizeof(overlay), "%.0f%%", frac * 100.0f);
                ImGui::ProgressBar(frac, ImVec2(-FLT_MIN, 0), overlay);
                ImGui::TextDisabled("%s", stage.c_str());
            }

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

            // Manual give-item (unchanged from the original tool).
            ImGui::BeginDisabled(!isRunning || server.ClientCount() == 0);
            ImGui::InputText("Item (RandomizerGet name)", itemName, sizeof(itemName));
            ImGui::InputText("Player (empty = everyone)", playerName, sizeof(playerName));
            const bool isIceTrap = std::string(itemName) == "RG_ICE_TRAP";
            if (isIceTrap) {
                auto shortLabel = [](const char* s) {
                    std::string str = s;
                    return str.size() > 60 ? str.substr(0, 57) + "..." : str;
                };
                const char* modelPreview = iceModelIdx < 0 ? "(Random)" : IceTrap::ModelDisplayName(iceModelIdx);
                if (ImGui::BeginCombo("Ice Trap Model", modelPreview)) {
                    if (ImGui::Selectable("(Random)", iceModelIdx < 0)) iceModelIdx = -1;
                    for (int i = 0; i < IceTrap::ModelCount(); ++i) {
                        ImGui::PushID(i);
                        if (ImGui::Selectable(IceTrap::ModelDisplayName(i), iceModelIdx == i)) iceModelIdx = i;
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
                std::string textPreview = iceTextIdx < 0 ? std::string("(Random)") : shortLabel(IceTrap::TextLabel(iceTextIdx));
                if (ImGui::BeginCombo("Ice Trap Text", textPreview.c_str())) {
                    if (ImGui::Selectable("(Random)", iceTextIdx < 0)) iceTextIdx = -1;
                    for (int i = 0; i < IceTrap::TextCount(); ++i) {
                        ImGui::PushID(i);
                        if (ImGui::Selectable(shortLabel(IceTrap::TextLabel(i)).c_str(), iceTextIdx == i)) iceTextIdx = i;
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
            }
            ImGui::BeginDisabled(itemName[0] == '\0');
            if (ImGui::Button("Send Item")) {
                nlohmann::json payload;
                payload["type"] = "command";
                payload["command"] = std::string("give_item randomizer ") + itemName;
                if (isIceTrap) {
                    int modelIndex = iceModelIdx < 0 ? IceTrap::RandomModelIndex() : iceModelIdx;
                    int textIndex = iceTextIdx < 0 ? IceTrap::RandomTextIndex() : iceTextIdx;
                    payload["iceTrapModel"] = IceTrap::ModelRgName(modelIndex);
                    payload["iceTrapText"] = IceTrap::ComposeText(modelIndex, textIndex);
                }
                if (playerName[0] != '\0') server.UnicastToClient(playerName, payload.dump());
                else server.BroadcastToClients(payload.dump());
            }
            ImGui::EndDisabled();
            ImGui::EndDisabled();

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

    if (genThread.joinable()) genThread.join();   // wait out any in-flight generation
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
