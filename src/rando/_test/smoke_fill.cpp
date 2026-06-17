// Smoke test: generate a 2-world multiworld seed; check beatability, cross-world
// mixing, and determinism (same seed -> identical placement).
#include "rando/Fill.h"
#include <cstdio>

static bool SamePlacements(const Fill::Result& a, const Fill::Result& b) {
    if (a.placements.size() != b.placements.size()) return false;
    for (size_t i = 0; i < a.placements.size(); ++i) {
        const auto& x = a.placements[i]; const auto& y = b.placements[i];
        if (x.loc != y.loc || x.locWorld != y.locWorld || x.item != y.item || x.itemWorld != y.itemWorld)
            return false;
    }
    return true;
}

int main() {
    Fill::Result r = Fill::Generate(12345ULL, 2);
    std::printf("seed 12345: %zu placements, %d cross-world\n", r.placements.size(), r.crossWorld);
    std::printf("  reachable via sphere search: %d/%zu\n", r.reached, r.placements.size());
    std::printf("  unreachable PROGRESSION items: %d   ->  beatable=%s\n",
                r.unreachedAdvancement, r.beatable ? "YES" : "no");

    Fill::Result r2 = Fill::Generate(12345ULL, 2);
    std::printf("deterministic (same seed -> same seed): %s\n", SamePlacements(r, r2) ? "YES" : "no");

    Fill::Result r3 = Fill::Generate(99999ULL, 2);
    std::printf("different seed -> different placement: %s\n", !SamePlacements(r, r3) ? "YES" : "no");

    // sample a few cross-world placements
    int shown = 0;
    for (const auto& p : r.placements) {
        if (p.itemWorld != p.locWorld && shown < 5) {
            std::printf("  world%d location %d  <-  item %d (owner world%d)\n",
                        p.locWorld + 1, (int)p.loc, (int)p.item, p.itemWorld + 1);
            shown++;
        }
    }
    return 0;
}
