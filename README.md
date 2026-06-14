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
