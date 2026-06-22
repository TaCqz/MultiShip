// Verifies the settings-preset data model (Part A) + supported-list exposure (Part B)
// against the REAL generated spec. Mirrors main.cpp's SavePreset/LoadPreset logic so a
// keyName/supported codegen error would fail here. Not wired into CMake; compiled ad-hoc.
#include "rando/SettingsSpec.h"
#include <nlohmann/json.hpp>
#include <cstdio>
#include <string>
#include <unordered_set>
#include <vector>

using nlohmann::json;

// --- copies of main.cpp's preset core, operating on a values vector ---
static json SavePresetJson(const std::vector<uint8_t>& values) {
    const auto& specs = Rando::MultiShipSettings();
    json settings = json::object();
    for (size_t i = 0; i < specs.size() && i < values.size(); ++i)
        if (specs[i].supported) settings[specs[i].keyName] = (int)values[i];
    return settings;
}
static int LoadPresetJson(const json& settings, std::vector<uint8_t>& values) {
    const auto& specs = Rando::MultiShipSettings();
    int applied = 0;
    for (size_t i = 0; i < specs.size() && i < values.size(); ++i) {
        if (!specs[i].supported) continue;
        auto it = settings.find(specs[i].keyName);
        if (it != settings.end() && it->is_number_integer()) {
            int v = it->get<int>();
            if (v >= 0 && v <= 255) { values[i] = (uint8_t)v; ++applied; }
        }
    }
    return applied;
}

int main() {
    const auto& specs = Rando::MultiShipSettings();
    int fails = 0;
    auto check = [&](bool ok, const char* msg) {
        std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", msg);
        if (!ok) ++fails;
    };

    // 1) Every supported setting has a non-empty, unique keyName.
    std::unordered_set<std::string> names;
    size_t supported = 0;
    bool namesOk = true, uniqueOk = true;
    for (const auto& sp : specs) {
        if (!sp.supported) continue;
        ++supported;
        if (!sp.keyName || !sp.keyName[0]) namesOk = false;
        else if (!names.insert(sp.keyName).second) uniqueOk = false;
    }
    std::printf("supported settings: %zu / %zu total\n", supported, specs.size());
    check(namesOk, "every supported setting has a non-empty keyName");
    check(uniqueOk, "supported keyNames are unique");
    check(supported > 0 && supported < specs.size(), "some settings supported, some hidden");

    // Defaults baseline.
    std::vector<uint8_t> defaults;
    for (const auto& sp : specs) defaults.push_back(sp.defaultValue);

    // 2) Round-trip: change a handful of supported settings, save, load into fresh
    //    defaults, expect the modified values back (and unsupported untouched).
    std::vector<uint8_t> modified = defaults;
    int touched = 0;
    for (size_t i = 0; i < specs.size() && touched < 6; ++i) {
        if (!specs[i].supported) continue;
        uint8_t hi = specs[i].IsNumeric() ? specs[i].numMax
                                          : (specs[i].choices.empty() ? 0 : specs[i].choices.back().value);
        if (hi != defaults[i]) { modified[i] = hi; ++touched; }
    }
    json saved = SavePresetJson(modified);
    std::vector<uint8_t> restored = defaults;
    int applied = LoadPresetJson(saved, restored);
    check(restored == modified, "round-trip restores every saved supported value");
    check((size_t)applied == supported, "load applied exactly the supported-setting count");
    check(touched > 0, "test actually changed some settings");

    // 3) Save contains ONLY supported keys (no hidden/inert settings leak in).
    bool onlySupported = true;
    for (auto it = saved.begin(); it != saved.end(); ++it) {
        bool found = false;
        for (const auto& sp : specs)
            if (sp.supported && it.key() == sp.keyName) { found = true; break; }
        if (!found) onlySupported = false;
    }
    check(onlySupported, "saved JSON contains only supported keys");

    // 4) Forward/backward tolerance: a preset MISSING a key keeps the spec default,
    //    and an UNKNOWN key is skipped without error.
    json partial = saved;
    // drop the first key to simulate a preset made before that setting existed
    if (!partial.empty()) partial.erase(partial.begin().key());
    partial["RSK_TOTALLY_MADE_UP_FUTURE_SETTING"] = 3;  // unknown key
    std::vector<uint8_t> tol = defaults;
    LoadPresetJson(partial, tol);
    // find index of the dropped key -> must still equal its default
    bool missingKeepsDefault = true;
    {
        std::string dropped;
        for (auto it = saved.begin(); it != saved.end(); ++it)
            if (!partial.contains(it.key())) { dropped = it.key(); break; }
        for (size_t i = 0; i < specs.size(); ++i)
            if (specs[i].supported && dropped == specs[i].keyName)
                missingKeepsDefault = (tol[i] == defaults[i]);
    }
    check(missingKeepsDefault, "missing key keeps spec default");
    check(true, "unknown key skipped without crashing (reached here)");

    std::printf("\n%s (%d failure(s))\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails ? 1 : 0;
}
