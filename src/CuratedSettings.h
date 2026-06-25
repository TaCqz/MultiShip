// Curated randomizer-settings model for the MultiShip Create screen.
//
// MultiShip exposes a hand-picked subset of Ship of Harkinian's randomizer
// settings. This header declares the single ordered table the Create UI renders
// from and that presets serialize. Each entry is keyed by the base SoH
// `RandomizerSettingKey` enum, so the same keys can later flow into the
// .multiship file and the SoH client unchanged.
//
// Source of truth for labels / option values / defaults / tooltips:
// docs/multiship-settings.md (captured from the sibling ShipwreckCombo repo).
//
// STEP 1 scope: this only feeds the UI + preset round-trip. Nothing here touches
// the seed generator, SeedFile, the server, or the wire protocol.
#pragma once

#include <string>
#include <vector>

#include "rando/randomizerEnums.h"  // materializes the RandomizerSettingKey enum

namespace ms {

// How a curated setting is rendered.
enum class Ui {
    Checkbox,  // boolean; stored value is 0 (off) or 1 (on)
    Combo,     // pick-one; stored value is the base SoH option index (see Opt::value)
    Slider,    // integer; stored value is the option index == displayed - sliderMin
};

// One choice in a Combo. `value` is the base SoH selectedOption index for this
// choice, so option subsets keep their original indices (e.g. Overworld == 4).
// That index is exactly what will later flow into the .multiship file/client.
struct Opt {
    const char* label;
    int value;
};

// A single curated setting. The table from CuratedSettings() is the one source
// the Create UI renders from and presets serialize to/from.
struct Setting {
    RandomizerSettingKey key;       // base SoH key; its NAME is the JSON/preset key
    const char* label;              // UI label
    Ui ui;
    std::vector<Opt> options;       // Combo only
    int sliderMin = 0;              // Slider only: displayed range (inclusive)
    int sliderMax = 0;              // Slider only: displayed range (inclusive)
    int defaultValue = 0;           // stored default (option index / 0|1)
    const char* tooltip = nullptr;  // verbatim base-game tooltip, or nullptr/"" for none
    const char* tab = "";           // tab id (one of Tabs())
    const char* section = "";       // section header within the tab
    bool hidden = false;            // stored & serialized at its fixed value, never rendered
    RandomizerSettingKey greyWhenKeyOn = RSK_NONE;  // grey out while this key's value != 0
    bool greyNonDefaultOptions = false;             // Combo: disable every option but the default
};

// The curated settings, in render order (grouped by tab, then section). Built
// once on first call.
const std::vector<Setting>& CuratedSettings();

// Tab ids in display order.
const std::vector<const char*>& Tabs();

// Maps a base SoH RandomizerSettingKey to its enum NAME, e.g.
// RSK_STARTING_AGE -> "RSK_STARTING_AGE". Used as the preset/JSON key.
const char* RSKName(RandomizerSettingKey key);

} // namespace ms
