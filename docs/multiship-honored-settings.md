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

### Currently honored

The allowlist is no longer empty. As of the per-setting phase it holds:

- **F-043 — Area Access (9):** `RSK_FOREST`, `RSK_KAK_GATE`, `RSK_DOOR_OF_TIME`,
  `RSK_ZORAS_FOUNTAIN`, `RSK_SLEEPING_WATERFALL`, `RSK_JABU_OPEN`, `RSK_GERUDO_FORTRESS`,
  `RSK_RAINBOW_BRIDGE` (+ `RSK_RAINBOW_BRIDGE_STONE_COUNT`,
  `RSK_RAINBOW_BRIDGE_MEDALLION_COUNT`), `RSK_GANONS_TRIALS` (+ `RSK_TRIAL_COUNT`).
- **F-044 — Tab 1 §1.1 "Logic" (7):** `RSK_STARTING_AGE` (+ `RSK_SELECTED_STARTING_AGE`),
  `RSK_FULL_WALLETS`, `RSK_SKIP_CHILD_ZELDA`, `RSK_MASK_QUEST`, `RSK_SKIP_CHILD_STEALTH`,
  `RSK_SKIP_EPONA_RACE`.
- **F-045 — Tab 2 Dungeon Items + Key Rings + Dungeon Rewards (8):** `RSK_SHUFFLE_MAPANDCOMPASS`,
  `RSK_KEYSANITY`, `RSK_BOSS_KEYSANITY`, `RSK_GANONS_BOSS_KEY`, `RSK_GERUDO_KEYS`,
  `RSK_KEYRINGS` (+ `RSK_KEYRINGS_RANDOM_COUNT`), `RSK_SHUFFLE_DUNGEON_REWARDS`.

F-045 notes (placement modes + the new pool work):

- **Placement modes** (Own Dungeon / Any Dungeon / Overworld / Anywhere) were already modelled
  by `Fill.cpp`'s FEAT-5 restricted fill; F-045 just adds the keys to the allowlist + the
  effective write-back. The baked defaults in `Context.h` for the three location-mode settings
  are set to **Vanilla** explicitly (they would otherwise be `0` == Start-With), so the empty-
  override baseline keeps vanilla dungeon items exactly as before.
- **Start-With** is now real (it previously folded to Vanilla): the item is removed from the pool
  (its vacated location is filled 1:1 with junk), collected into every world's permanent
  inventory so the fill's reachability assumes it, and shipped as Start-With so the SoH client
  grants it at save init via the shared start-state path.
- **Ganon's Boss Key** uses its own option set; the LACS / 100-GS win-condition variants (which
  the engine can't model) fold to **Own Dungeon** and ship that effective value.
- **Gerudo Fortress keys** (`RSK_GERUDO_KEYS`, four modes, no Own Dungeon) govern the four
  Thieves' Hideout carpenter locations — categorized `LC_OTHER` but combat-gated, so they pool
  cleanly for any non-vanilla mode (special-cased by vanilla item in `Fill::IsShuffled` + the
  item-rule loop).
- **Key Rings** is the only genuinely new engine work. `RSK_KEYRINGS` (Off / Random / Count)
  resolves deterministically (from the seed, independent of the per-attempt RNG) to a set of
  eligible dungeons — those whose small keys are actually shuffled (a ring is pointless under
  Vanilla / Start-With keys; Gerudo Fortress is eligible only with non-vanilla fortress keys +
  Normal carpenters). For each selected dungeon the pool gets **one `RG_*_KEY_RING`** (which
  `Logic::CollectItem` already treats as 10 small keys) **plus junk** for the redundant key
  slots — a 1:1 substitution, so the placement count is unchanged. The ring inherits the
  placement zone of the small key it replaces. `RSK_KEYRINGS_RANDOM_COUNT` (Count-mode size) is
  shipped so the selection size round-trips; the client derives the actual rings from the placed
  `RG_*_KEY_RING` items, not from the setting.
- **Shuffle Dungeon Rewards** (`RSK_SHUFFLE_DUNGEON_REWARDS`, End-of-Dungeons only) shuffles the
  9 spiritual stones + medallions among the 9 dungeon-clear locations (8 boss blue-warps + Rauru),
  own-world, via `Fill.cpp`'s `ZR_REWARD_LOC`. The Rauru placement is re-homed to Link's Pocket
  (the "free" pocket reward) in `Generator.cpp` after the fill; the client suppresses the vanilla
  boss/Rauru gives and delivers each placed reward via the F-040 flag-collection flow.
  - **Closed-Door-of-Time guarantee.** With a closed Door of Time the three spiritual stones gate
    adult access (`temple_of_time.cpp` opens the door only at `StoneCount()==3`), so every stone
    must be obtainable as child — i.e. at one of the three CHILD-dungeon boss rewards (Queen Gohma
    / King Dodongo / Barinade), the only child-reachable reward locs (exactly three). Because all
    nine rewards share `ZR_REWARD_LOC`, the fill orders the **stones before the medallions** (a
    medallion is never an adult-access gate) so the stones claim the child locs deterministically;
    otherwise a medallion could steal a child loc and strand a stone behind an adult dungeon,
    leaning on the 40-attempt retry to recover (measured pre-fix: ~37/40 attempts, occasionally
    exhausting into a LOCKED seed). Guarded by `smoke_dot_closed_rewards`.

F-044 notes (the write-back + normalization details that matter):

- **Starting Age** is already modelled — `Fill.cpp` resolves `RSK_STARTING_AGE`
  (Child/Adult/Random) into `RSK_SELECTED_STARTING_AGE` (root.cpp / Logic.cpp read it) and
  forces Door of Time open for an adult start. `RSK_SELECTED_STARTING_AGE` is a **hidden
  curated setting** so it is present in `sd.settings` for the overwrite-only write-back to
  ship the *resolved* value — that is the value the SoH client applies (linkAge/spawn/Master
  Sword).
- **Full Wallets** has **no placement effect**: the engine does not model shop affordability
  (`GetCheckPrice()` returns 0, so `price <= walletCap` is always true). It is honored
  (shipped) only so the client grants the rupees; it is harmless on the allowlist.
- **Skip Child Zelda / Skip Epona Race** are pure reachability reads (`root.cpp` makes the
  Letter/Song-From-Impa/Malon checks root-reachable and frees Epona). No pool edit was
  needed beyond shipping — the seed stays beatable under each (asserted in the smoke).
- **Skip Child Stealth** has no reachability effect (Zelda is reachable either way); it is
  shipped for the client's entrance remap only.
- **Mask Quest** is honored for **Vanilla + Completed** only. The engine models masks as
  borrow *events* (`market.cpp`) with **no mask item-locations**, so it cannot pool masks for
  "Shuffle"; the curated UI drops that option and `Fill.cpp` folds a stray `Shuffle` value to
  `Vanilla` defensively.

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
