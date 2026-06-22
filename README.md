# MultiShip Server

A small standalone server for the **MultiShip** network module in
[ShipwreckCombo](../ShipwreckCombo). It listens on a TCP port, accepts
connections from the ShipwreckCombo client, and logs the `'\0'`-delimited JSON
packets it sends.

This is the native C++ counterpart to the reference TypeScript [`sail`](../sail)
server — it speaks the exact same wire protocol.

## Build settings

Mirrors ShipwreckCombo:

- C++20, MSVC, Visual Studio 2022 generator
- Dependencies via **vcpkg** (`x64-windows-static` triplet), bootstrapped
  automatically by `CMake/automate-vcpkg.cmake`
- Libraries: `sdl2`, `sdl2-net`, `nlohmann-json`, `spdlog`,
  `imgui[sdl2-binding,sdl2-renderer-binding]`

## Building

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64-release
```

The first configure clones and bootstraps vcpkg and builds the static
dependencies, which can take a while. The resulting executable is under
`build/Release/MultiShip.exe`.

> If you already have a `VCPKG_ROOT` environment variable pointing at a vcpkg
> checkout, that installation is reused instead of cloning a new one.

## Usage

1. Launch `MultiShip.exe`.
2. Enter the desired port (default `43384`) and click **Start Server**.
   The server listens on every local interface (the machine's IP).
3. In ShipwreckCombo, open **Settings → Network → MultiShip**, set the host to
   this machine's IP (or `127.0.0.1` when on the same machine) and the matching
   port, then click **Connect**.

Connections and incoming packets are shown in the log pane.

## Protocol

Raw TCP. Each packet is a UTF-8 JSON object terminated by a single NUL byte
(`\0`). The client (ShipwreckCombo) currently sends `hook`-type packets; this
server logs them. See [`../sail/types.ts`](../sail/types.ts) for the full
packet schema shared with the reference server.

<!-- BACKLOG:START -->
## Roadmap & feature backlog

Tracked across the whole project (MultiShip + ShipwreckCombo). Checked items are shipped;
items get checked off here as they land. Each entry shows its id, type
(`feat`/`fix`/`opt`) and the repo(s) it touches.

**5 shipped · 21 open**

### Multiworld item presentation
- [x] **F-001** Shopsanity item labels show owning player — _feat, ShipwreckCombo_
- [x] **F-002** Shopsanity ice traps wear a disguise (model + fake name) — _feat, ShipwreckCombo/MultiShip_
- [x] **F-003** Foreign-item collection textbox: "You found <Player>'s <Item>" — _feat, ShipwreckCombo_
- [x] **F-004** Ice-trap name consistency between shop and collection textbox — _feat, ShipwreckCombo_  _(needs F-002)_

### Client runtime & item delivery
- [x] **F-023** Area-access settings (forest/gate/door of time/fountain/etc.) don't take effect in-game — _fix, ShipwreckCombo_  _(needs F-005)_
- [ ] **F-025** "Continue to Server" after creating a seed loads a broken seed — _fix, MultiShip_
- [ ] **F-027** Item lost if another arrives during the get-item (over-head) animation — _fix, ShipwreckCombo_

### Generator correctness
- [ ] **F-009** Honor or normalize Songs / Ocarina / Adult-Trade / Egg / Sword shuffles — _fix, MultiShip_
- [ ] **F-010** Wire bridge/LACS count requirements + sane defaults (count modes trivially beatable) — _fix, MultiShip_
- [ ] **F-006** Fix 2 logic-fidelity locations in the clean-room engine — _fix, MultiShip_
- [ ] **F-013** Implement 'All Locations Reachable' verification mode — _feat, MultiShip_

### Generator features (settings not yet honored)
- [ ] **F-011** Add Context::FinalizeSettings + apply starting inventory/songs/equipment to logic — _feat, MultiShip_
- [ ] **F-012** Resolve RSK_STARTING_AGE (Child/Adult/Random) into RSK_SELECTED_STARTING_AGE — _fix, MultiShip_  _(needs F-011)_
- [ ] **F-014** Item pool quantity (Plentiful/Balanced/Scarce/Minimal) + pool edits — _feat, MultiShip_
- [ ] **F-015** Additional sanities: grass/signs/fairies/merchants/etc. — _feat, MultiShip_
- [ ] **F-016** Key rings (RSK_KEYRINGS + per-dungeon) — _feat, MultiShip_
- [ ] **F-017** Triforce Hunt + Ganon's Trials + LACS sub-modes — _feat, MultiShip_  _(needs F-011)_
- [ ] **F-018** Master Quest dungeon support — _feat, MultiShip_  _(needs F-011)_
- [ ] **F-019** Entrance randomizer — _feat, MultiShip_
- [ ] **F-020** Logic rules: No-Logic mode + trick/glitch logic options — _feat, MultiShip_
- [ ] **F-024** Honor shopsanity item-count (currently forced to all 8 per shop) — _feat, MultiShip_

### UI / tooling / QoL
- [ ] **F-026** Settings presets (save/load JSON) + gate UI to supported settings — _feat, MultiShip_
- [ ] **F-022** QoL: generation-time settings-coverage guard + smoke tests for the gaps — _opt, MultiShip_
- [ ] **F-028** Remove entrance-randomizer settings from the UI (no effect yet) — _fix, MultiShip_

### Parked (low priority / contingency)
- [ ] **F-007** Own-dungeon keysanity retry reduction (CONTINGENCY) — _fix, MultiShip_
- [ ] **F-008** Restore niche dungeon-item sub-modes to full fidelity (LOW PRIORITY) — _feat, MultiShip_

### Dropped / merged
- ~~**F-005** Guard MultiShip file creation until server seed is cached~~ — CANCELLED — tried and reverted
- ~~**F-021** QoL: only expose settings the generator actually honors~~ — MERGED into F-026 (settings presets + gate-UI-to-supported are the same Create-screen work)
<!-- BACKLOG:END -->
