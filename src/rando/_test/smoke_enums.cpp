// Smoke test: the vendored randomizer enums must compile standalone (no game deps).
#include "../randomizerEnums.h"
#include <cstdio>

int main() {
    std::printf("RR_KOKIRI_FOREST       = %d\n", (int)RR_KOKIRI_FOREST);
    std::printf("RG_KOKIRI_SWORD        = %d\n", (int)RG_KOKIRI_SWORD);
    std::printf("RC_KF_KOKIRI_SWORD_CHEST = %d\n", (int)RC_KF_KOKIRI_SWORD_CHEST);
    std::printf("RSK_FOREST             = %d\n", (int)RSK_FOREST);
    std::printf("RO_CLOSED_FOREST_OFF   = %d\n", (int)RO_CLOSED_FOREST_OFF);
    std::printf("LOGIC_FOREST_TEMPLE_CLEAR = %d\n", (int)LOGIC_FOREST_TEMPLE_CLEAR);
    std::printf("RR_MAX=%d  RG_MAX=%d  RC_MAX=%d\n", (int)RR_MAX, (int)RG_MAX, static_cast<int>(RC_MAX));
    return 0;
}
