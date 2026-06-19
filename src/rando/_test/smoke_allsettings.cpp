// Beatability check for the full extracted settings table. Builds the override set
// from AllRandoSettings() defaults and generates, to confirm the clean-room engine
// produces a beatable seed under the full default config (and reports which settings
// differ from the engine's own ApplyDefaultSettings baseline).
#include "rando/SettingsSpecData.h"
#include "rando/Context.h"
#include "rando/Fill.h"
#include <cstdio>
#include <vector>

int main() {
    const auto& specs = Rando::AllRandoSettings();
    std::printf("Total settings extracted: %zu\n", specs.size());

    Rando::Context baseline;  // ApplyDefaultSettings() runs in the ctor
    const auto& base = baseline.GetAllOptions();

    std::vector<Fill::SettingOverride> overrides;
    int differ = 0;
    for (const auto& s : specs) {
        overrides.push_back({ (uint16_t)s.key, s.defaultValue });
        if (base[(size_t)s.key] != s.defaultValue) {
            std::printf("  differs from engine baseline: %-34s default=%u  baseline=%u\n",
                        s.label, s.defaultValue, base[(size_t)s.key]);
            ++differ;
        }
    }
    std::printf("%d settings differ from the engine baseline.\n\n", differ);

    for (uint64_t seed : { 12345ull, 999ull, 7ull }) {
        Fill::Result r = Fill::Generate(seed, 2, overrides);
        std::printf("seed %llu: %zu placements, %d cross, %d attempt(s), beatable=%s\n",
                    (unsigned long long)seed, r.placements.size(), r.crossWorld, r.attempts,
                    r.beatable ? "YES" : "NO");
    }
    return 0;
}
