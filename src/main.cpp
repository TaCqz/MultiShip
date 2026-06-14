// MultiShip server — minimal ImGui front-end: pick a port, hit "Start Server",
// and accept connections from the ShipwreckCombo MultiShip module.
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_net.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_sdlrenderer2.h>

#include <spdlog/spdlog.h>

#include <cstdio>

#include "Server.h"

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
