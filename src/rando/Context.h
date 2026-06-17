// Clean-room port of the bits of Rando::Context the logic needs: settings (baked to
// the reference defaults), tricks (off), dungeon mode (vanilla), trials (skipped).
#pragma once

#include "randomizerEnums.h"
#include <array>
#include <cstdint>

namespace Rando {

// Mirrors option.h's OptionValue: ctx->GetOption(RSK_X).Is(RO_Y) / .IsNot(RO_Y).
class OptionValue {
  public:
    OptionValue() = default;
    explicit OptionValue(uint8_t v) : mVal(v) {}
    uint8_t Get() const { return mVal; }
    bool Is(uint32_t other) const { return mVal == other; }
    bool IsNot(uint32_t other) const { return !Is(other); }
    explicit operator bool() const { return mVal != 0; }
  private:
    uint8_t mVal = 0;
};
using Option = OptionValue;   // SoH logic refers to the value type as `Option`

typedef enum {
    DEKU_TREE, DODONGOS_CAVERN, JABU_JABUS_BELLY, FOREST_TEMPLE, FIRE_TEMPLE,
    WATER_TEMPLE, SPIRIT_TEMPLE, SHADOW_TEMPLE, BOTTOM_OF_THE_WELL, ICE_CAVERN,
    GERUDO_TRAINING_GROUND, GANONS_CASTLE
} DungeonKey;

class DungeonInfo {
  public:
    bool masterQuest = false;        // default settings: all vanilla
    bool IsMQ() const { return masterQuest; }
    bool IsVanilla() const { return !masterQuest; }
};

// TrialKey (TK_*) comes from the vendored RandomizerMiscEnums.h.
class Trial {
  public:
    bool required = false;           // Skip Ganon's Trials: Yes -> none required
    bool IsRequired() const { return required; }
    bool IsSkipped() const { return !required; }
};

class Context {
  public:
    Context() { ApplyDefaultSettings(); }

    OptionValue GetOption(RandomizerSettingKey key) const {
        return OptionValue(options[(size_t)key]);
    }
    OptionValue GetTrickOption(RandomizerTrick) const { return OptionValue(0); }  // tricks off
    // Reference seed: Ganon's Castle Boss Key = LACS dungeon rewards.
    uint8_t LACSCondition() const { return RO_LACS_REWARDS; }
    DungeonInfo* GetDungeon(DungeonKey key) { return &dungeons[(size_t)key]; }
    Trial* GetTrial(TrialKey key) { return &trials[(size_t)key]; }

    void SetOption(RandomizerSettingKey key, uint8_t value) { options[(size_t)key] = value; }

    // Baked to the reference (AP) spoiler == SoH defaults. Options default to 0 (the
    // "off/closed/vanilla/child" RO_ values); only the ones that differ are set here.
    void ApplyDefaultSettings() {
        options[(size_t)RSK_FOREST]              = RO_CLOSED_FOREST_OFF;
        options[(size_t)RSK_KAK_GATE]            = RO_KAK_GATE_OPEN;
        options[(size_t)RSK_DOOR_OF_TIME]        = RO_DOOROFTIME_OPEN;   // gates adult access
        options[(size_t)RSK_ZORAS_FOUNTAIN]      = RO_ZF_CLOSED_CHILD;
        options[(size_t)RSK_SLEEPING_WATERFALL]  = RO_WATERFALL_CLOSED;
        options[(size_t)RSK_GERUDO_FORTRESS]     = RO_GF_CARPENTERS_FAST;
        options[(size_t)RSK_SELECTED_STARTING_AGE] = RO_AGE_CHILD;
        options[(size_t)RSK_RAINBOW_BRIDGE]      = RO_BRIDGE_GREG;
        // counts (end-game gates) — see reference spoiler
        options[(size_t)RSK_LACS_REWARD_COUNT]   = 6;
        // TODO(parity): remaining count settings + verify every RSK against the spoiler.
    }

  private:
    std::array<uint8_t, RSK_MAX> options{};
    std::array<DungeonInfo, 12> dungeons{};
    std::array<Trial, 6> trials{};
};

} // namespace Rando

extern Rando::Context* ctx;
