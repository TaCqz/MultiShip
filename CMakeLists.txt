# MultiShip — standalone server for the ShipwreckCombo "MultiShip" network module.
#
# Build settings intentionally mirror the ShipwreckCombo (Ship of Harkinian) project:
#   - C++20, MSVC, Visual Studio generator
#   - vcpkg with the x64-windows-static triplet (bootstrapped via CMake/automate-vcpkg.cmake)
#
# It speaks the same protocol as the SoH `Network` client: NUL-delimited ('\0') JSON
# packets over a raw TCP connection, using SDL2_net (same library family as the client).
# 3.22 is the floor so the project builds with the stock CMake on common LTS
# distros (e.g. Ubuntu 22.04 ships 3.22.1). Nothing here needs anything newer.
cmake_minimum_required(VERSION 3.22 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0 CACHE STRING "" FORCE)
set(CMAKE_CXX_STANDARD 20 CACHE STRING "The C++ standard to use")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(MultiShip VERSION 0.1.0 LANGUAGES C CXX)

# vcpkg's x64-windows-static triplet links the static CRT (/MT), so every target
# in this project must do the same or the linker reports RuntimeLibrary mismatch.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Match ShipwreckCombo's MSVC compile options.
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/MP>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>)

# The desktop GUI server pulls in Dear ImGui + SDL_Renderer. Headless/server
# builds (Linux deployments, the Docker image) can turn it off and build only the
# CLI, which then needs neither ImGui nor a network fetch at configure time:
#   cmake -S . -B build -DMULTISHIP_BUILD_GUI=OFF
option(MULTISHIP_BUILD_GUI "Build the ImGui desktop GUI server in addition to the CLI" ON)

# ------------------------------------------------------------------------------
# Dependencies via vcpkg (same bootstrap helper and static triplet as the parent)
# ------------------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    if(NOT CMAKE_VS_PLATFORM_NAME)
        set(CMAKE_VS_PLATFORM_NAME "x64")
    endif()

    include(CMake/automate-vcpkg.cmake)

    set(VCPKG_TRIPLET x64-windows-static)
    set(VCPKG_TARGET_TRIPLET x64-windows-static)

    vcpkg_bootstrap()
    # NOTE: imgui is intentionally NOT taken from vcpkg. The current vcpkg port
    # only provides SDL3 bindings, whereas (matching the parent project) we build
    # against SDL2. We pull imgui via FetchContent below and compile its SDL2 +
    # SDL_Renderer backends ourselves.
    vcpkg_install_packages(sdl2 sdl2-net nlohmann-json spdlog)
endif()

find_package(SDL2 CONFIG REQUIRED)
# SDL2_net: prefer its CMake config (vcpkg, and SDL2_net >= 2.2.0). Older distro
# packages (e.g. Ubuntu 22.04's SDL2_net 2.0.1) ship no config, so fall back to
# pkg-config and expose the same SDL2_net::SDL2_net target the rest of this file
# links against.
find_package(SDL2_net CONFIG QUIET)
if(NOT TARGET SDL2_net::SDL2_net AND NOT TARGET SDL2_net::SDL2_net-static)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2_NET REQUIRED IMPORTED_TARGET SDL2_net)
    add_library(SDL2_net::SDL2_net ALIAS PkgConfig::SDL2_NET)
endif()
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Resolve the SDL2 target name (static on the vcpkg triplet, shared on most
# distros). Needed by the CLI always, and by the GUI when enabled.
if(TARGET SDL2::SDL2-static)
    set(MULTISHIP_SDL2_TARGET SDL2::SDL2-static)
elseif(TARGET SDL2::SDL2)
    set(MULTISHIP_SDL2_TARGET SDL2::SDL2)
endif()

# ------------------------------------------------------------------------------
# Rando engine — restored clean-room port of the Ship of Harkinian randomizer
# logic + multiworld assumed-fill, the FOUNDATION Generator pipeline, and the
# .multiship/spoiler IO. No SDL/ImGui dependency, so it builds standalone and the
# smoke test links only this.
# ------------------------------------------------------------------------------
file(GLOB MULTISHIP_RANDO_SOURCES CONFIGURE_DEPENDS
    src/rando/Logic.cpp
    src/rando/LogicState.cpp
    src/rando/RegionGraph.cpp
    src/rando/Search.cpp
    src/rando/Fill.cpp
    src/rando/Generator.cpp
    src/rando/SeedFile.cpp
    src/rando/location_access/*.cpp
    src/rando/location_access/*/*.cpp
)
add_library(multiship_rando STATIC ${MULTISHIP_RANDO_SOURCES})
target_include_directories(multiship_rando PUBLIC src)

# Foundation smoke test: a generated 2-world seed is jointly beatable AND the
# serialized contract round-trips (settings + placements + owners + names + seed id).
# Links only the engine (no SDL/ImGui), so it runs anywhere the library builds.
enable_testing()
add_executable(MultiShipSmoke src/rando/_test/smoke_foundation.cpp)
target_link_libraries(MultiShipSmoke PRIVATE multiship_rando)
add_test(NAME foundation_smoke COMMAND MultiShipSmoke)

# F-041 smoke test: the generator emits exactly one Link's Pocket dungeon-reward placement
# per world (same-world, owned locally), deterministically per seed.
add_executable(MultiShipLinksPocketSmoke src/rando/_test/smoke_linkspocket.cpp)
target_link_libraries(MultiShipLinksPocketSmoke PRIVATE multiship_rando)
add_test(NAME links_pocket_smoke COMMAND MultiShipLinksPocketSmoke)

# F-042 smoke test: the gCtx reset + the honored-settings override plumbing. Generating
# twice is deterministic; a temporary probe key on the allowlist changes placement (the
# override reaches the engine); and removing it reproduces the exact baseline (no cross-seed
# state leak). Guards the divergence trap precondition for the per-setting phase (F-043+).
add_executable(MultiShipOverridesSmoke src/rando/_test/smoke_overrides.cpp)
target_link_libraries(MultiShipOverridesSmoke PRIVATE multiship_rando)
add_test(NAME overrides_smoke COMMAND MultiShipOverridesSmoke)

# F-043 (a) smoke test: the seven "area access" settings are honored end-to-end. Each value is
# accepted + jointly beatable, generation stays deterministic, every value round-trips into the
# shipped settings, and a normalized key (adult start -> Door of Time open) ships its EFFECTIVE
# value (the client/engine lockstep write-back).
add_executable(MultiShipAreaAccessSmoke src/rando/_test/smoke_area_access.cpp)
target_link_libraries(MultiShipAreaAccessSmoke PRIVATE multiship_rando)
add_test(NAME area_access_smoke COMMAND MultiShipAreaAccessSmoke)

# F-043 (b) smoke test: Rainbow Bridge + Ganon's Trials. Trials resolve to the right required
# count (Set Number / Skip / Random), the engine marks exactly that many trials required, both
# bridge + trials seeds stay jointly beatable, and the resolved values round-trip into the seed.
add_executable(MultiShipBridgeTrialsSmoke src/rando/_test/smoke_bridge_trials.cpp)
target_link_libraries(MultiShipBridgeTrialsSmoke PRIVATE multiship_rando)
add_test(NAME bridge_trials_smoke COMMAND MultiShipBridgeTrialsSmoke)

# F-044 smoke test: Tab 1 section 1.1 "Logic" (start-state + child-era settings). An adult start
# resolves + ships Selected Starting Age = Adult and forces Door of Time open; Skip Child Zelda,
# Mask Quest Completed, Full Wallets, Skip Child Stealth, Skip Epona Race each ship their value and
# stay jointly beatable (alone and all together); Mask Quest Shuffle folds to Vanilla.
add_executable(MultiShipLogicSettingsSmoke src/rando/_test/smoke_logic_settings.cpp)
target_link_libraries(MultiShipLogicSettingsSmoke PRIVATE multiship_rando)
add_test(NAME logic_settings_smoke COMMAND MultiShipLogicSettingsSmoke)

# ------------------------------------------------------------------------------
# Desktop GUI server (optional) — Dear ImGui (pinned) + SDL2 / SDL_Renderer
# backends, built as a static lib. Skipped entirely when MULTISHIP_BUILD_GUI=OFF,
# so headless builds pull in neither ImGui nor a network fetch at configure time.
# ------------------------------------------------------------------------------
if(MULTISHIP_BUILD_GUI)
    include(FetchContent)
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.8
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui_sdl2 STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
    )
    target_include_directories(imgui_sdl2 PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui_sdl2 PUBLIC ${MULTISHIP_SDL2_TARGET})

    add_executable(MultiShip
        src/main.cpp
        src/CuratedSettings.cpp
        src/Server.cpp
    )
    target_include_directories(MultiShip PRIVATE src)
    target_link_libraries(MultiShip PRIVATE
        ${MULTISHIP_SDL2_TARGET}
        $<TARGET_NAME_IF_EXISTS:SDL2_net::SDL2_net>
        $<TARGET_NAME_IF_EXISTS:SDL2_net::SDL2_net-static>
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        imgui_sdl2
        multiship_rando
    )

    # Debug-only server tools (give-item, player target, region teleport). Enable with
    # `cmake -DDEBUG=1 ...`; off by default so released builds don't expose them.
    if(DEBUG)
        target_compile_definitions(MultiShip PRIVATE MULTISHIP_DEBUG=1)
        message(STATUS "MultiShip: DEBUG tools enabled (give-item, teleport)")
    endif()

    # We drive SDL_main ourselves (see SDL_MAIN_HANDLED in main.cpp), so a plain
    # console subsystem is fine and keeps stdout logging available.
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MultiShip)
endif()

# ------------------------------------------------------------------------------
# Headless CLI variant — same Server, no window. Reads the port from argv and
# logs to the console. Builds on Windows, Linux and macOS with no ImGui /
# SDL_Renderer dependency, so this is the target used for server deployments.
# ------------------------------------------------------------------------------
add_executable(MultiShipCLI
    src/main_cli.cpp
    src/Server.cpp
)

target_include_directories(MultiShipCLI PRIVATE src)

target_link_libraries(MultiShipCLI PRIVATE
    ${MULTISHIP_SDL2_TARGET}
    $<TARGET_NAME_IF_EXISTS:SDL2_net::SDL2_net>
    $<TARGET_NAME_IF_EXISTS:SDL2_net::SDL2_net-static>
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    multiship_rando
)

if(NOT MULTISHIP_BUILD_GUI)
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT MultiShipCLI)
endif()
