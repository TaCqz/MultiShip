// MultiShip server — minimal ImGui front-end: pick a port, hit "Start Server",
// and accept connections from the ShipwreckCombo MultiShip module.
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

#include <cstdio>

#include "Server.h"
#include "IceTrap.h"

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

    SDL_Window* window =
        SDL_CreateWindow("MultiShip Server", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        spdlog::error("SDL_CreateWindow failed: {}", SDL_GetError());
        SDLNet_Quit();
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        spdlog::error("SDL_CreateRenderer failed: {}", SDL_GetError());
        SDL_DestroyWindow(window);
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
    int port = 43384; // default matches the SoH Sail/MultiShip menu default
    bool autoScroll = true;
    char itemName[64] = "";   // RandomizerGet enum name to give, e.g. RG_KOKIRI_SWORD
    char playerName[64] = ""; // target player (empty = send to everyone)
    int iceModelIdx = -1;     // ice-trap-only: IceTrap model preset index, -1 = random
    int iceTextIdx = -1;      // ice-trap-only: IceTrap text preset index, -1 = random

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window)) {
                running = false;
            }
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Fill the OS window with a single main panel.
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("MultiShip", nullptr,
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImGui::TextUnformatted("MultiShip Server");
        ImGui::Separator();

        const bool isRunning = server.IsRunning();

        ImGui::BeginDisabled(isRunning);
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputInt("Port", &port);
        if (port < 1025)
            port = 1025;
        if (port > 65535)
            port = 65535;
        ImGui::EndDisabled();

        ImGui::SameLine();
        if (!isRunning) {
            if (ImGui::Button("Start Server")) {
                server.Start(static_cast<uint16_t>(port));
            }
        } else {
            if (ImGui::Button("Stop Server")) {
                server.Stop();
            }
        }

        if (isRunning) {
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.4f, 1.0f), "Running on port %d  -  clients: %d", port,
                               server.ClientCount());
        } else {
            ImGui::TextColored(ImVec4(0.9f, 0.6f, 0.3f, 1.0f), "Stopped");
        }

        // Give an item to a player. Enter the item (a RandomizerGet enum name,
        // e.g. RG_KOKIRI_SWORD) and the target player's name (leave empty to send
        // it to everyone). When the item is an ice trap, the model/text fields set
        // the disguise; leaving one empty lets the client fall back.
        ImGui::BeginDisabled(!isRunning || server.ClientCount() == 0);
        ImGui::InputText("Item (RandomizerGet name)", itemName, sizeof(itemName));
        ImGui::InputText("Player (empty = everyone)", playerName, sizeof(playerName));

        const bool isIceTrap = std::string(itemName) == "RG_ICE_TRAP";
        if (isIceTrap) {
            // Pick the disguise model and message from the presets (keys are the
            // dropdown selections), or "(Random)" to let the server roll one.
            // Long messages are shortened in the list only.
            auto shortLabel = [](const char* s) {
                std::string str = s;
                return str.size() > 60 ? str.substr(0, 57) + "..." : str;
            };
            const char* modelPreview = iceModelIdx < 0 ? "(Random)" : IceTrap::ModelDisplayName(iceModelIdx);
            if (ImGui::BeginCombo("Ice Trap Model", modelPreview)) {
                if (ImGui::Selectable("(Random)", iceModelIdx < 0)) {
                    iceModelIdx = -1;
                }
                for (int i = 0; i < IceTrap::ModelCount(); ++i) {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(IceTrap::ModelDisplayName(i), iceModelIdx == i)) {
                        iceModelIdx = i;
                    }
                    ImGui::PopID();
                }
                ImGui::EndCombo();
            }
            std::string textPreview = iceTextIdx < 0 ? std::string("(Random)") : shortLabel(IceTrap::TextLabel(iceTextIdx));
            if (ImGui::BeginCombo("Ice Trap Text", textPreview.c_str())) {
                if (ImGui::Selectable("(Random)", iceTextIdx < 0)) {
                    iceTextIdx = -1;
                }
                for (int i = 0; i < IceTrap::TextCount(); ++i) {
                    ImGui::PushID(i);
                    if (ImGui::Selectable(shortLabel(IceTrap::TextLabel(i)).c_str(), iceTextIdx == i)) {
                        iceTextIdx = i;
                    }
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
                // -1 means "(Random)" — roll one now (ad-hoc, no seed state).
                int modelIndex = iceModelIdx < 0 ? IceTrap::RandomModelIndex() : iceModelIdx;
                int textIndex = iceTextIdx < 0 ? IceTrap::RandomTextIndex() : iceTextIdx;
                payload["iceTrapModel"] = IceTrap::ModelRgName(modelIndex);
                payload["iceTrapText"] = IceTrap::ComposeText(modelIndex, textIndex);
            }
            if (playerName[0] != '\0') {
                server.UnicastToClient(playerName, payload.dump());
            } else {
                server.BroadcastToClients(payload.dump());
            }
        }
        ImGui::EndDisabled();
        ImGui::EndDisabled();

        ImGui::Separator();
        ImGui::Checkbox("Auto-scroll", &autoScroll);
        ImGui::SameLine();
        if (ImGui::Button("Clear log")) {
            server.ClearLog();
        }

        ImGui::BeginChild("log", ImVec2(0, 0), ImGuiChildFlags_Borders,
                          ImGuiWindowFlags_HorizontalScrollbar);
        for (const std::string& line : server.LogSnapshot()) {
            ImGui::TextUnformatted(line.c_str());
        }
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();

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
