// Verifies the freestanding/pots/crates/beehives shuffles pool their locations and stay
// beatable, and that the Dungeons/Overworld sub-modes are canonicalized to All in the
// shipped settings (the clean-room fill pools all of each type for any non-off mode).
#include "rando/Fill.h"
#include "rando/MetaData.h"
#include <cstdio>
#include <unordered_set>
#include <vector>

int main() {
    int fail = 0;
    std::unordered_set<int> pot, crate, free_, hive;
    for (int i = 0; i < kLocationCount; ++i) {
        int rc = (int)kLocations[i].rc;
        switch (kLocations[i].category) {
            case LC_POT: pot.insert(rc); break;
            case LC_CRATE: crate.insert(rc); break;
            case LC_FREESTANDING: free_.insert(rc); break;
            case LC_BEEHIVE: hive.insert(rc); break;
            default: break;
        }
    }
    auto countAt = [](const Fill::Result& r, const std::unordered_set<int>& s) {
        int n = 0; for (const auto& p : r.placements) if (s.count((int)p.loc)) ++n; return n;
    };

    struct Case { const char* name; uint16_t key; uint8_t on; uint8_t allVal;
                  const std::unordered_set<int>* locs; };
    std::vector<Case> cases = {
        { "pots",         (uint16_t)RSK_SHUFFLE_POTS,         (uint8_t)RO_SHUFFLE_POTS_DUNGEONS,
          (uint8_t)RO_SHUFFLE_POTS_ALL, &pot },
        { "crates",       (uint16_t)RSK_SHUFFLE_CRATES,       (uint8_t)RO_SHUFFLE_CRATES_OVERWORLD,
          (uint8_t)RO_SHUFFLE_CRATES_ALL, &crate },
        { "freestanding", (uint16_t)RSK_SHUFFLE_FREESTANDING, (uint8_t)RO_SHUFFLE_FREESTANDING_DUNGEONS,
          (uint8_t)RO_SHUFFLE_FREESTANDING_ALL, &free_ },
        { "beehives",     (uint16_t)RSK_SHUFFLE_BEEHIVES,     (uint8_t)1, (uint8_t)1, &hive },
    };

    // Fill::Generate reuses the global ctx and doesn't reset unlisted options between
    // calls, so each case passes a FULL set of the four keys (the one under test set to
    // its value, the other three forced off) for clean isolation. Production is unaffected
    // — the GUI always sends the complete settings array.
    auto overrides = [&](const Case& test, uint8_t val) {
        std::vector<Fill::SettingOverride> ov;
        for (const auto& c : cases) ov.push_back({ c.key, c.key == test.key ? val : (uint8_t)0 });
        return ov;
    };

    // Each setting individually (off then a non-off sub-mode), others forced off.
    for (const auto& c : cases) {
        Fill::Result off = Fill::Generate(12345, 2, overrides(c, 0));
        Fill::Result on  = Fill::Generate(12345, 2, overrides(c, c.on));
        int nOff = countAt(off, *c.locs), nOn = countAt(on, *c.locs);
        std::printf("%-12s off: %zu pl (%d at), beatable=%s | on: %zu pl (%d at), beatable=%s; synced=%u\n",
                    c.name, off.placements.size(), nOff, off.beatable ? "YES" : "NO",
                    on.placements.size(), nOn, on.beatable ? "YES" : "NO", on.settings[c.key]);
        if (!off.beatable || !on.beatable) ++fail;
        if (nOff != 0) { std::printf("  FAIL: %s pooled while off\n", c.name); ++fail; }
        if (nOn == 0)  { std::printf("  FAIL: %s not pooled while on\n", c.name); ++fail; }
        if (on.settings[c.key] != c.allVal) { std::printf("  FAIL: %s not canonicalized to All\n", c.name); ++fail; }
    }

    // All four on together (heaviest pool) — must still be beatable.
    {
        std::vector<Fill::SettingOverride> ov;
        for (const auto& c : cases) ov.push_back({ c.key, c.on });
        Fill::Result r = Fill::Generate(12345, 2, ov);
        int total = countAt(r, pot) + countAt(r, crate) + countAt(r, free_) + countAt(r, hive);
        std::printf("ALL FOUR on: %zu placements (%d at collectibles), %d attempt(s), beatable=%s\n",
                    r.placements.size(), total, r.attempts, r.beatable ? "YES" : "NO");
        if (!r.beatable) ++fail;
        if (total == 0) { std::printf("  FAIL: collectibles not pooled with all on\n"); ++fail; }
    }

    std::printf(fail ? "\n%d CHECK(S) FAILED\n" : "\nALL COLLECTIBLE CHECKS PASSED\n", fail);
    return fail ? 1 : 0;
}
