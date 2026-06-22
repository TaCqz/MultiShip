// Starting Age resolution + reachability check. RSK_STARTING_AGE (Child/Adult/Random) is
// the user-facing setting; Fill::Generate resolves it into RSK_SELECTED_STARTING_AGE (the
// value the logic + Search::Run read) and forces the Door of Time open on an adult start.
// This verifies all three modes produce a beatable seed and that the shipped settings carry
// the resolved (concrete) values.
#include "rando/SettingsSpecData.h"
#include "rando/Context.h"
#include "rando/Fill.h"
#include "rando/enums/RandomizerOptions.h"
#include <cstdio>
#include <vector>

static int failures = 0;
static void expect(bool cond, const char* what) {
    std::printf("  [%s] %s\n", cond ? "PASS" : "FAIL", what);
    if (!cond) ++failures;
}

int main() {
    const auto& specs = Rando::AllRandoSettings();

    // Full default override set (a complete key set isolates each Generate call — the engine
    // reuses the global ctx and doesn't reset unlisted options between calls).
    std::vector<Fill::SettingOverride> base;
    for (const auto& s : specs) base.push_back({ (uint16_t)s.key, s.defaultValue });

    auto withAge = [&](uint8_t age) {
        std::vector<Fill::SettingOverride> ov = base;
        for (auto& o : ov)
            if (o.key == (uint16_t)RSK_STARTING_AGE) o.value = age;
        return ov;
    };

    struct Mode { const char* name; uint8_t age; uint8_t expectSelected; bool fixedSelected; };
    const Mode modes[] = {
        { "Child",  RO_AGE_CHILD,  RO_AGE_CHILD, true  },
        { "Adult",  RO_AGE_ADULT,  RO_AGE_ADULT, true  },
        { "Random", RO_AGE_RANDOM, 0,            false },  // resolves to child or adult
    };

    for (const Mode& m : modes) {
        std::printf("Starting Age = %s\n", m.name);
        auto ov = withAge(m.age);
        for (uint64_t seed : { 12345ull, 999ull, 7ull }) {
            Fill::Result r = Fill::Generate(seed, 2, ov);
            uint8_t selected = r.settings[(size_t)RSK_SELECTED_STARTING_AGE];
            uint8_t shippedAge = r.settings[(size_t)RSK_STARTING_AGE];
            uint8_t door = r.settings[(size_t)RSK_DOOR_OF_TIME];
            std::printf("  seed %llu: %zu pl, %d attempt(s), beatable=%s, selected=%s, door=%s\n",
                        (unsigned long long)seed, r.placements.size(), r.attempts,
                        r.beatable ? "YES" : "NO",
                        selected == RO_AGE_ADULT ? "Adult" : "Child",
                        door == RO_DOOROFTIME_OPEN ? "Open" : "Closed/Song");
            expect(r.beatable, "beatable");
            // Random must collapse to a concrete value; the user-facing key ships it too.
            expect(selected == RO_AGE_CHILD || selected == RO_AGE_ADULT, "selected is concrete");
            expect(shippedAge == selected, "shipped RSK_STARTING_AGE == selected (Random collapsed)");
            if (m.fixedSelected)
                expect(selected == m.expectSelected, "selected matches the chosen age");
            // An adult start must ship the Door of Time open (so child is reachable via time travel).
            if (selected == RO_AGE_ADULT)
                expect(door == RO_DOOROFTIME_OPEN, "adult start forces Door of Time open");
        }
    }

    std::printf("\n%s (%d failure(s))\n", failures ? "SMOKE FAILED" : "SMOKE OK", failures);
    return failures ? 1 : 0;
}
