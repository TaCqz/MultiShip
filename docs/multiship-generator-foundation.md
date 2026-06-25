# MultiShip Generator — FOUNDATION pipeline (STEP 2)

This documents the minimal vanilla multiworld pipeline built on top of the restored
clean-room engine. It is the baseline the per-setting tickets extend one setting at a
time.

## What runs

`Generator::Generate(opts)` (`src/rando/Generator.{h,cpp}`):

1. **Placement** — calls the restored `Fill::Generate(seed, numWorlds)` (multiworld
   assumed-fill). Both worlds' item pools and locations are combined; any world's
   check can hold any world's item. `Fill`'s internal `VerifyReachable` confirms every
   progression item is reachable across **both** worlds before returning
   `beatable = true` (jointly beatable).
2. **Ice traps** — `Generator::kDefaultIceTraps` (currently **12**, total across both
   worlds) junk placements are rewritten to `RG_ICE_TRAP`. Only junk is ever rewritten
   (never progression, never an existing ice trap), so beatability is preserved.
   Selection is deterministic (seed-derived RNG over the junk slots in placement order).
   The ice-trap **setting** is wired in a later ticket; this is a fixed sane default.
3. **Seed id** — `Generator::MakeSeedId(seed)` derives a human-readable id from the
   numeric seed, e.g. `heron-snow-oak-d0fa` (word triplet from a 64-word list +
   4 hex digits). Deterministic, so both players can confirm they got the same seed.
4. **Output** — assembles a `SeedFile::SeedData` carrying the seed id, player names,
   the full placement table (with owners), and the curated settings verbatim.

## Baseline configuration (placement)

Placement runs the engine at its **own baked defaults**
(`Rando::Context::ApplyDefaultSettings`, `src/rando/Context.h`). The curated UI
settings are carried to the OUTPUT but **do not branch placement yet**. The baseline is:

| Setting                    | Baseline value            |
|----------------------------|---------------------------|
| Forest                     | Open (`CLOSED_FOREST_OFF`)|
| Kakariko Gate              | Open                      |
| Door of Time               | Open                      |
| Zora's Fountain            | Closed (child)            |
| Sleeping Waterfall         | Closed                    |
| Gerudo Fortress            | Fast (1 carpenter)        |
| Starting Age               | Child                     |
| Rainbow Bridge             | Greg                      |
| Bridge / LACS counts       | stones 3 / medallions 6 / rewards 9 / dungeons 8 / tokens 100 |
| Everything else (RSK)      | 0 = off / vanilla / closed |

In effect: open-forest, open-door-of-time, child start, Greg bridge, vanilla dungeon
items, and **no** extra location-pool shuffles (songs / shops / tokens / scrubs / pots
/ etc. all off). The shuffled pool is therefore the default standard checks. This
produces a clean, reliably-beatable 2-world seed (the smoke test gets it in 1 attempt).

## KNOWN milestone divergence

Because no curated setting branches placement yet, a seed's **shipped** curated settings
may not match its **placement** behavior (e.g. an area-access toggle is recorded in the
file but the world was generated open per the baseline). This is expected for the
foundation. Per-setting tickets close the gap one setting at a time, verifying both the
seed and the client view each time. The shown items and player-ownership **labels** are
already correct, because they come from the placement table + owners, which are accurate.

## Serialized contract (`.multiship`, schema v3)

`SeedFile::SeedData` is the stable, versioned contract — persisted to the `.multiship`
file today and (in a later ticket) carried over the server→client wire. Binary layout:

```
"MSHP" | version:u32(=3) | seed:u64 | seedId:str | numWorlds:u8
per world: playerName:str
placementCount:u32 | per placement: locWorld:u8, loc:u16, ownerWorld:u8, item:u16
settingCount:u16   | per setting:   rskKey:u16, value:u16
```

- `str` = u8 length prefix + bytes.
- `loc` is a `RandomizerCheck`, `item` a `RandomizerGet`.
- `rskKey` is a `RandomizerSettingKey` enum value (its NAME is
  `SeedFile::SettingKeyName(key)`); `value` is the **base SoH option index**, verbatim
  from the curated UI (no subset remapping).
- v2 files (reset-baseline empties) still load (placements/settings read as present;
  no seed id).

The contract round-trips byte-for-byte through `WriteMultiship` / `ReadMultiship`
(asserted by `src/rando/_test/smoke_foundation.cpp`).

## Determinism

Same `(seed, players, settings)` ⇒ byte-identical `SeedData`. The fill, the ice-trap
selection, and the seed id are all pure functions of `seed`.

## Smoke test

`MultiShipSmoke` (CMake target; `ctest -R foundation_smoke`) asserts: a generated
2-world seed is jointly beatable, generation is deterministic, ice traps are injected,
and the serialized form round-trips settings + placements + owners + names + seed id.
