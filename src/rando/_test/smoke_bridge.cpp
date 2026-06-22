// Count-based bridge / LACS gating: a Stones/Medallions/Tokens rainbow bridge (and the
// dungeon-rewards LACS condition) of count N must be UNSATISFIABLE from the start state and
// only open once N of the gating item are collected. Guards the regression where the count
// settings defaulted to 0 (so "0 collected >= 0 required" opened the gate immediately,
// yielding trivially-beatable seeds). Drives Logic::CanBuildRainbowBridge / CanTriggerLACS
// directly — Fill always places items reachably, so the gate logic is what carries the
// requirement, not the fill.
#include "rando/Graph.h"
#include "rando/Logic.h"
#include "rando/Context.h"
#include <cstdio>
#include <vector>

using namespace Rando;

static int fail = 0;

// Collect `items` one at a time; the gate must stay closed until the final item and open
// exactly when the configured count is reached.
static void checkGate(const char* name, RandomizerSettingKey countKey,
                      const std::vector<RandomizerGet>& items,
                      bool (Logic::*gate)()) {
    logic->Reset();
    uint8_t count = ctx->GetOption(countKey).Get();
    bool startClosed = !(logic.get()->*gate)();
    std::printf("%-22s count=%u, items=%zu | start: %s",
                name, count, items.size(), startClosed ? "CLOSED" : "OPEN(!)");
    if (count == 0)            { std::printf("  FAIL: count defaulted to 0\n"); ++fail; return; }
    if ((size_t)count != items.size()) { std::printf("  FAIL: test supplies %zu items for count %u\n",
                                                     items.size(), count); ++fail; return; }
    if (!startClosed)          { std::printf("  FAIL: gate open from start state\n"); ++fail; return; }

    for (size_t i = 0; i < items.size(); ++i) {
        logic->CollectItem(items[i], true);
        bool open = (logic.get()->*gate)();
        bool wantOpen = (i + 1 >= count);
        if (open != wantOpen) {
            std::printf("  FAIL: after %zu/%u items gate=%s (want %s)\n",
                        i + 1, count, open ? "OPEN" : "CLOSED", wantOpen ? "OPEN" : "CLOSED");
            ++fail;
            return;
        }
    }
    std::printf("  -> opened at %u/%u: PASS\n", count, count);
}

int main() {
    RegionTable_Init();  // builds graph + sets global ctx/logic (ApplyDefaultSettings applied)

    const std::vector<RandomizerGet> stones  = { RG_KOKIRI_EMERALD, RG_GORON_RUBY, RG_ZORA_SAPPHIRE };
    const std::vector<RandomizerGet> medals  = { RG_FOREST_MEDALLION, RG_FIRE_MEDALLION,
                                                 RG_WATER_MEDALLION, RG_SPIRIT_MEDALLION,
                                                 RG_SHADOW_MEDALLION, RG_LIGHT_MEDALLION };
    std::vector<RandomizerGet> tokens(ctx->GetOption(RSK_RAINBOW_BRIDGE_TOKEN_COUNT).Get(),
                                      RG_GOLD_SKULLTULA_TOKEN);

    // --- Rainbow bridge, one mode per case ---
    ctx->SetOption(RSK_RAINBOW_BRIDGE, RO_BRIDGE_STONES);
    checkGate("bridge: stones", RSK_RAINBOW_BRIDGE_STONE_COUNT, stones, &Logic::CanBuildRainbowBridge);

    ctx->SetOption(RSK_RAINBOW_BRIDGE, RO_BRIDGE_MEDALLIONS);
    checkGate("bridge: medallions", RSK_RAINBOW_BRIDGE_MEDALLION_COUNT, medals, &Logic::CanBuildRainbowBridge);

    ctx->SetOption(RSK_RAINBOW_BRIDGE, RO_BRIDGE_TOKENS);
    checkGate("bridge: tokens", RSK_RAINBOW_BRIDGE_TOKEN_COUNT, tokens, &Logic::CanBuildRainbowBridge);

    // --- LACS (Ganon's Castle Boss Key). LACSCondition() is dungeon rewards = stones+medallions. ---
    std::vector<RandomizerGet> rewards = stones;
    rewards.insert(rewards.end(), medals.begin(), medals.end());  // 3 + 6 = 9 dungeon rewards
    checkGate("lacs: rewards", RSK_LACS_REWARD_COUNT, rewards, &Logic::CanTriggerLACS);

    std::printf(fail ? "\n%d BRIDGE/LACS CHECK(S) FAILED\n" : "\nALL BRIDGE/LACS CHECKS PASSED\n", fail);
    return fail ? 1 : 0;
}
