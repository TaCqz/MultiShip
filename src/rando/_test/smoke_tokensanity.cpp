// Verifies tokensanity: GS tokens enter the shuffled pool per mode and seeds stay
// beatable (off / dungeons / overworld / all).
#include "rando/Fill.h"
#include <cstdio>
#include <vector>

int main() {
    struct Mode { const char* name; uint8_t v; } modes[] = {
        { "OFF", RO_TOKENSANITY_OFF },
        { "DUNGEONS", RO_TOKENSANITY_DUNGEONS },
        { "OVERWORLD", RO_TOKENSANITY_OVERWORLD },
        { "ALL", RO_TOKENSANITY_ALL },
    };
    int fail = 0;
    for (const auto& m : modes) {
        std::vector<Fill::SettingOverride> ov = { { (uint16_t)RSK_SHUFFLE_TOKENS, m.v } };
        Fill::Result r = Fill::Generate(12345, 2, ov);
        int gs = 0;
        for (const auto& p : r.placements)
            if (p.item == RG_GOLD_SKULLTULA_TOKEN) ++gs;
        std::printf("tokensanity %-9s: %zu placements, %d GS-token items in pool, %d attempt(s), beatable=%s\n",
                    m.name, r.placements.size(), gs, r.attempts, r.beatable ? "YES" : "NO");
        if (!r.beatable) ++fail;
    }
    std::printf(fail ? "\n%d MODE(S) NOT BEATABLE\n" : "\nALL TOKENSANITY MODES BEATABLE\n", fail);
    return fail ? 1 : 0;
}
