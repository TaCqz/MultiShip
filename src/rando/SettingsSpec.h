// SettingsSpec — the randomizer settings exposed in the MultiShip Create-seed UI.
// The full list is AUTO-EXTRACTED from SoH's Settings::CreateOptions() into
// SettingsSpecData.h (rando-logic/gen_settings.py); this header just adapts it into
// the SettingSpec shape the GUI/generator use. Defaults mirror SoH's own defaults,
// which the clean-room engine generates beatable seeds under (verified).
//
// NOTE: every setting is exposed + synced to clients + honored at runtime, and the
// generator honors all LOGIC/world settings. Settings that change the LOCATION POOL
// (shopsanity, tokensanity, scrubsanity, etc.) are not yet added to the engine's pool
// — they default off, and enabling them won't create those checks until the engine
// supports them.
#pragma once
#include "randomizerEnums.h"
#include "rando/SettingsSpecData.h"
#include <cctype>
#include <cstdint>
#include <vector>

namespace Rando {

struct SettingChoice {
    uint8_t value;       // the RO_* option value (== index, options are value-ordered)
    const char* label;   // shown in the dropdown
};

struct SettingSpec {
    RandomizerSettingKey key;
    const char* label;
    const char* tab;                     // settings menu tab (SoH sidebar)
    const char* section;                 // section divider header within the column ("" = none)
    const char* tooltip;                 // hover description (from SoH; "" = none)
    uint8_t column;                      // column index within the tab
    std::vector<SettingChoice> choices;  // empty => numeric setting (use [numMin, numMax])
    uint8_t defaultValue;
    uint8_t numMin = 0;
    uint8_t numMax = 0;
    bool IsNumeric() const { return choices.empty(); }

    // A plain on/off toggle: exactly two choices labelled "Off" and "On" — rendered
    // as a checkbox instead of a dropdown.
    bool IsToggle() const {
        if (choices.size() != 2) return false;
        auto eq = [](const char* a, const char* b) {
            for (; *a && *b; ++a, ++b)
                if (std::tolower((unsigned char)*a) != std::tolower((unsigned char)*b)) return false;
            return *a == *b;
        };
        const char* a = choices[0].label;
        const char* b = choices[1].label;
        return (eq(a, "off") && eq(b, "on")) || (eq(a, "on") && eq(b, "off"));
    }
    // The choice value that means "on" (so checkbox state maps to the right RO_ value).
    uint8_t OnValue() const {
        for (const auto& c : choices) {
            const char* s = c.label;
            if ((std::tolower((unsigned char)s[0]) == 'o') &&
                (std::tolower((unsigned char)s[1]) == 'n') && s[2] == '\0')
                return c.value;
        }
        return 1;
    }
};

// All extracted settings, in SoH's declaration order. Built once from the generated
// table.
inline const std::vector<SettingSpec>& MultiShipSettings() {
    static const std::vector<SettingSpec> specs = [] {
        std::vector<SettingSpec> v;
        v.reserve(AllRandoSettings().size());
        for (const auto& g : AllRandoSettings()) {
            SettingSpec s;
            s.key = g.key;
            s.label = g.label;
            s.tab = g.tab;
            s.section = g.section;
            s.tooltip = g.tooltip;
            s.column = g.column;
            s.defaultValue = g.defaultValue;
            s.numMin = g.numMin;
            s.numMax = g.numMax;
            if (!g.numeric) {
                for (const auto& c : g.choices) {
                    s.choices.push_back({ c.value, c.label });
                }
            }
            v.push_back(std::move(s));
        }
        return v;
    }();
    return specs;
}

} // namespace Rando
