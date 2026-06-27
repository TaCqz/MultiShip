# MultiShip — gCtx reset + the honored-settings allowlist (F-042)

The precondition for the per-setting phase (F-043+). Two engine-side guarantees so that,
once curated settings start branching placement, generation stays **clean** (no state
leaks between seeds) and **safe** (a seed never assumes a world-state the client doesn't
enforce yet). Neither changes the v3 output today — the allowlist is empty, so generation
is byte-for-byte the foundation baseline (`docs/multiship-generator-foundation.md`).

## 1. gCtx reset — clean state every generation

The clean-room engine keeps its settings in a **singleton** `Rando::Context`
(`gCtx` in `src/rando/RegionGraph.cpp`), created once and reused for every
`Fill::Generate` call in the process. `RegionTable_Init` rebuilds the region graph,
`grottoEvents`, and resets each world's `Logic` every call — but it used to leave `gCtx`
untouched after the first creation. So any `ctx->SetOption(...)` from a prior generation
survived into the next one:

- the per-seed **overrides** (once the allowlist is non-empty), and
- the **normalizations** `Fill::Generate` always applies (e.g. resolving Starting Age
  Random → Child/Adult, folding songs/shops/scrubs to the value the engine can honor).

Harmless while everything ran at baseline (a second generation re-applied the identical
no-op normalizations), but a real cross-seed leak the moment settings branch placement:
generating seed A with setting X on, then seed B with X off, would silently generate B
with X *still on*.

**Fix.** `RegionTable_Init` now calls `gCtx->ResetToDefaults()` on every init
(`src/rando/Context.h`). `ResetToDefaults` zeroes the whole options array (plus the
Master-Quest dungeon flags and trial flags) and then re-applies `ApplyDefaultSettings`.
Zeroing first is essential: `ApplyDefaultSettings` only *writes the non-zero defaults* (it
assumes a freshly constructed, zeroed array), so without the zero pass a key a prior seed
set non-zero would survive the "reset". On the very first call this is a no-op (the
constructor already applied defaults); on every later call it re-baselines the singleton.

Net effect: each `Generator::Generate` / `Fill::Generate` starts from the exact baked
defaults, independent of any generation that ran before it in the same process.

## 2. The honored-settings allowlist — the divergence guard

Curated settings reach the engine as `Fill::SettingOverride`s, but only for keys on a
single central allowlist. **`Generator::HonoredSettings()`** (`src/rando/Generator.cpp`)
is the one source of truth: a list of `RandomizerSettingKey` values whose curated option
is fed into `Fill::Generate` as a real override. Every curated setting **not** on the list
is still written to the v3 output verbatim (so labels/spoiler/UI stay truthful) but is held
at the engine's baked default **for placement**.

Why a gate at all? This is the **divergence trap**. The engine can technically branch on
far more settings than the SoH client currently enforces end-to-end (delivery, save
application, area access, etc.). If a seed's *placement* assumed a world-state the client
doesn't actually set up, the seed could be unbeatable or desync. The allowlist makes the
honored set explicit and conservative: a setting only branches placement once its whole
path is implemented.

`HonoredSettings()` is **empty** in the foundation phase, so `Generator::Generate` builds
an empty override list and placement is the pure baseline.

### How to extend it (the per-setting workflow, F-043+)

Each per-setting ticket flips exactly one entry onto the allowlist once the client honors
it end-to-end:

1. Confirm the engine already models the setting in `Fill::IsShuffled` / the restricted
   fill / the logic (most pool toggles and dungeon-item modes already are), and that
   `Fill::Generate`'s normalization block maps any sub-modes the engine can't fully model
   to the value it does use — so the shipped setting matches the generated pool.
2. Implement the SoH client side (apply the setting, deliver/grant correctly).
3. Add the `RSK_*` key to the `gHonoredSettings` initializer in `src/rando/Generator.cpp`.
4. Extend a smoke test to assert the seed is correct + beatable with that setting on.

That's the only edit needed here — the override plumbing (`opts.settings` → filter by
`HonoredSettings()` → `Fill::SettingOverride`) is already in place.

## Smoke test

`MultiShipOverridesSmoke` (CMake target; `ctest -R overrides_smoke`,
`src/rando/_test/smoke_overrides.cpp`) asserts:

1. **Determinism (empty allowlist)** — generating twice with identical inputs yields
   identical placements + seed id (reinforces `foundation_smoke`).
2. **Override plumbing** — with a **temporary** probe key (`RSK_SHUFFLE_COWS`, which pools
   the 9 junk cow checks per world) placed on the allowlist via the
   `Generator::SetHonoredSettingsForTest` test seam, the placement table changes (354 →
   372 placements for the test seed) and the seed stays jointly beatable. This proves the
   curated value actually reaches the engine's pool/placement.
3. **Determinism (with the override)** — generating twice under the probe is identical too.
4. **No cross-seed leak (the gCtx reset)** — after restoring the empty allowlist, the exact
   baseline is reproduced. Without the `RegionTable_Init` reset this assertion fails (the
   probe's `RSK_SHUFFLE_COWS=On` survives in the singleton and the final seed still pools
   cows) — verified by temporarily disabling the reset.

`SetHonoredSettingsForTest` is a test-only seam (declared in `Generator.h`, never called by
production). The production allowlist stays empty until a per-setting ticket adds a key.
