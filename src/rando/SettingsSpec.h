// SettingsSpec — the randomizer settings the MultiShip engine actually implements
// (the ones Context::ApplyDefaultSettings models for logic). Shared by the GUI (to
// render editable controls) and the generator (defaults must match here). Adding a
// setting here makes it appear in the Create-seed UI and flow through generation +
// the .multiship settings block to clients.
#pragma once
#include "randomizerEnums.h"
#include <cstdint>
#include <vector>

namespace Rando {

struct SettingChoice {
    uint8_t value;       // the RO_* option value
    const char* label;   // shown in the dropdown
};

struct SettingSpec {
    RandomizerSettingKey key;
    const char* label;
    std::vector<SettingChoice> choices;  // empty => numeric setting (use [numMin, numMax])
    uint8_t defaultValue;
    uint8_t numMin = 0;
    uint8_t numMax = 0;
    bool IsNumeric() const { return choices.empty(); }
};

// The implemented settings, in display order. Defaults mirror ApplyDefaultSettings.
inline const std::vector<SettingSpec>& MultiShipSettings() {
    static const std::vector<SettingSpec> specs = {
        { RSK_FOREST, "Closed Forest",
          { { RO_CLOSED_FOREST_OFF, "Open" },
            { RO_CLOSED_FOREST_DEKU_ONLY, "Deku Tree Only" },
            { RO_CLOSED_FOREST_ON, "Closed" } },
          RO_CLOSED_FOREST_OFF },
        { RSK_KAK_GATE, "Kakariko Gate",
          { { RO_KAK_GATE_OPEN, "Open" }, { RO_KAK_GATE_CLOSED, "Closed" } },
          RO_KAK_GATE_OPEN },
        { RSK_DOOR_OF_TIME, "Door of Time",
          { { RO_DOOROFTIME_OPEN, "Open" },
            { RO_DOOROFTIME_SONGONLY, "Song Only" },
            { RO_DOOROFTIME_CLOSED, "Closed" } },
          RO_DOOROFTIME_OPEN },
        { RSK_ZORAS_FOUNTAIN, "Zora's Fountain",
          { { RO_ZF_OPEN, "Open" },
            { RO_ZF_CLOSED_CHILD, "Closed as Child" },
            { RO_ZF_CLOSED, "Closed" } },
          RO_ZF_CLOSED_CHILD },
        { RSK_SLEEPING_WATERFALL, "Sleeping Waterfall",
          { { RO_WATERFALL_OPEN, "Open" }, { RO_WATERFALL_CLOSED, "Closed" } },
          RO_WATERFALL_CLOSED },
        { RSK_GERUDO_FORTRESS, "Gerudo Fortress Carpenters",
          { { RO_GF_CARPENTERS_NORMAL, "Normal" },
            { RO_GF_CARPENTERS_FAST, "Fast" },
            { RO_GF_CARPENTERS_FREE, "Free" } },
          RO_GF_CARPENTERS_FAST },
        { RSK_SELECTED_STARTING_AGE, "Starting Age",
          { { RO_AGE_CHILD, "Child" }, { RO_AGE_ADULT, "Adult" }, { RO_AGE_RANDOM, "Random" } },
          RO_AGE_CHILD },
        { RSK_RAINBOW_BRIDGE, "Rainbow Bridge",
          { { RO_BRIDGE_VANILLA, "Vanilla" },
            { RO_BRIDGE_ALWAYS_OPEN, "Always Open" },
            { RO_BRIDGE_STONES, "Spiritual Stones" },
            { RO_BRIDGE_MEDALLIONS, "Medallions" },
            { RO_BRIDGE_DUNGEON_REWARDS, "Dungeon Rewards" },
            { RO_BRIDGE_DUNGEONS, "Dungeons" },
            { RO_BRIDGE_TOKENS, "Gold Skulltula Tokens" },
            { RO_BRIDGE_GREG, "Greg" } },
          RO_BRIDGE_GREG },
        { RSK_LACS_REWARD_COUNT, "LACS Reward Count", {}, 6, 0, 9 },
    };
    return specs;
}

} // namespace Rando
