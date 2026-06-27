#include "rando/Fill.h"
#include "rando/Graph.h"
#include "rando/Logic.h"
#include "rando/Search.h"
#include "rando/MetaData.h"

#include <algorithm>
#include <cstdio>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>

namespace Fill {
namespace {
constexpr int RCM = (int)RC_MAX;
inline long long Key(int w, RandomizerCheck rc) { return (long long)w * RCM + (int)rc; }
struct PoolItemInternal { RandomizerGet item; int owner; };

// FEAT-5 restricted placement. A dungeon item (key/map/compass/boss-key/reward) under a
// non-anywhere mode may only be placed in locations of a given zone, in its own world.
enum ZoneRule { ZR_NONE, ZR_OWN, ZR_ANY_DUNGEON, ZR_OVERWORLD, ZR_REWARD_LOC };
struct ItemRule { ZoneRule zr; uint8_t home; };  // home = the item's own DungeonZone

// Whether a location is in the shuffled pool for the active settings. Default-on
// categories are always shuffled; settings-controlled categories (currently GS tokens
// via tokensanity) are added when their setting is on. Reads the global `ctx`, so it
// must be called after RegionTable_Init + settings overrides are applied.
bool IsShuffled(const LocMeta& L) {
    switch (L.category) {
        case LC_STANDARD:
            // Three otherwise-standard locations are gated by item-specific toggles. With
            // the toggle off the vanilla item stays put (location not pooled); with it on
            // the location joins the pool like any other standard check. The vacated item
            // is collected in-world during verify, so an Off seed stays beatable.
            if (L.vanilla == RG_KOKIRI_SWORD) return ctx->GetOption(RSK_SHUFFLE_KOKIRI_SWORD).Get() != 0;
            if (L.vanilla == RG_MASTER_SWORD) return ctx->GetOption(RSK_SHUFFLE_MASTER_SWORD).Get() != 0;
            if (L.vanilla == RG_WEIRD_EGG)    return ctx->GetOption(RSK_SHUFFLE_WEIRD_EGG).Get() != 0;
            return true;
        case LC_BOSS_HEART:
            // Boss-heart containers (and the Lens-of-Truth / Ice-Arrows checks grouped
            // here) have no toggle in the menu — always pooled, like LC_STANDARD.
            return true;
        case LC_SONG:
            // Songs have Song-Locations / Dungeon-Rewards restricted-placement modes the
            // clean-room fill can't model; it does a full (Anywhere) shuffle for any
            // non-off mode (Generate normalizes the shipped value to Anywhere). Off keeps
            // each song on its vanilla teaching location.
            return ctx->GetOption(RSK_SHUFFLE_SONGS).Get() != RO_SONG_SHUFFLE_OFF;
        case LC_OCARINA:
            // On/off toggle: pools both ocarina locations (Saria's gift + Ocarina of Time).
            return ctx->GetOption(RSK_SHUFFLE_OCARINA).Get() != 0;
        case LC_ADULT_TRADE:
            // On/off toggle: pools the adult-trade-quest checks. Off leaves the vanilla
            // trade sequence in place (the Claim Check is a normal LC_STANDARD check).
            return ctx->GetOption(RSK_SHUFFLE_ADULT_TRADE).Get() != 0;
        case LC_SKULL_TOKEN: {
            uint8_t mode = ctx->GetOption(RSK_SHUFFLE_TOKENS).Get();
            if (mode == RO_TOKENSANITY_ALL) return true;
            if (mode == RO_TOKENSANITY_DUNGEONS) return L.dungeon;
            if (mode == RO_TOKENSANITY_OVERWORLD) return !L.dungeon;
            return false;  // RO_TOKENSANITY_OFF
        }
        case LC_SCRUB:
            // Scrubsanity pools every deku-scrub location (One-Time is normalized to
            // All in Generate, so any non-off mode shuffles them all).
            return ctx->GetOption(RSK_SHUFFLE_SCRUBS).Get() != RO_SCRUBS_OFF;
        case LC_SHOP:
            // Shopsanity pools every shop slot. The Specific-Count/Random sub-counts
            // aren't modeled by the clean-room fill (it shuffles all 8 slots), so any
            // non-off mode pools them all — Generate normalizes the shipped count to match.
            return ctx->GetOption(RSK_SHOPSANITY).Get() != RO_SHOPSANITY_OFF;
        case LC_COW:
            // Cowsanity is a plain on/off toggle (0 = off); every cow is pooled when on.
            return ctx->GetOption(RSK_SHUFFLE_COWS).Get() != 0;
        case LC_FISH:
            // Fishsanity has Loach/Pond/Overworld/Both modes; the clean-room fill pools
            // ALL fish for any non-off mode (Generate normalizes the shipped value to Both
            // so client + server agree). Fish are junk (RG_FISH) so this can't strand.
            return ctx->GetOption(RSK_FISHSANITY).Get() != RO_FISHSANITY_OFF;
        case LC_POT:
            // Pots/crates/freestanding have Dungeons/Overworld/All modes; the clean-room
            // fill pools ALL of each for any non-off mode (Generate normalizes the shipped
            // value to All). All of these hold junk vanilla items, so they can't strand.
            return ctx->GetOption(RSK_SHUFFLE_POTS).Get() != RO_SHUFFLE_POTS_OFF;
        case LC_CRATE:
            return ctx->GetOption(RSK_SHUFFLE_CRATES).Get() != RO_SHUFFLE_CRATES_OFF;
        case LC_FREESTANDING:
            return ctx->GetOption(RSK_SHUFFLE_FREESTANDING).Get() != RO_SHUFFLE_FREESTANDING_OFF;
        case LC_BEEHIVE:
            // Beehives are a plain on/off toggle (0 = off).
            return ctx->GetOption(RSK_SHUFFLE_BEEHIVES).Get() != 0;
        // FEAT-5 dungeon items. The location (chest) is shuffled for any non-vanilla mode;
        // Generate folds the unsupported sub-modes (Start-With, Ganon-BK LACS/100GS) to a
        // supported one first, so "!= vanilla" here means own/any/overworld/anywhere. WHERE
        // the matching key/reward item may land is constrained by the restricted fill below.
        case LC_SMALL_KEY:
            return ctx->GetOption(RSK_KEYSANITY).Get() != RO_DUNGEON_ITEM_LOC_VANILLA;
        case LC_BOSS_KEY:
            return ctx->GetOption(RSK_BOSS_KEYSANITY).Get() != RO_DUNGEON_ITEM_LOC_VANILLA;
        case LC_MAP:
        case LC_COMPASS:
            return ctx->GetOption(RSK_SHUFFLE_MAPANDCOMPASS).Get() != RO_DUNGEON_ITEM_LOC_VANILLA;
        case LC_GANON_BOSS_KEY:
            return ctx->GetOption(RSK_GANONS_BOSS_KEY).Get() != RO_GANON_BOSS_KEY_VANILLA;
        case LC_DUNGEON_REWARD:
            return ctx->GetOption(RSK_SHUFFLE_DUNGEON_REWARDS).Get() != RO_DUNGEON_REWARDS_VANILLA;
        default:
            return false;  // LC_OTHER + sanities not yet pooled by the engine
    }
}

// Forward sphere search over the finished placement: start each world from its default
// state, repeatedly collect items at reachable locations into their owner's inventory
// (placed shuffled item, or non-shuffled vanilla item in-world) until nothing new opens.
int VerifyReachable(const std::vector<Placement>& placements, int numWorlds,
                    std::vector<std::shared_ptr<Rando::Logic>>& worlds,
                    const std::unordered_map<int, RandomizerGet>& vanillaOf,
                    const std::unordered_set<int>& shuffledSet,
                    std::unordered_set<long long>& reachedOut) {
    for (auto& w : worlds) w->Reset();
    std::unordered_map<long long, std::pair<RandomizerGet, int>> placed;
    for (const auto& p : placements) placed[Key(p.locWorld, p.loc)] = { p.item, p.itemWorld };

    std::unordered_set<long long> collected;
    bool progress = true;
    while (progress) {
        progress = false;
        for (int w = 0; w < numWorlds; ++w) {
            logic = worlds[w];
            for (RandomizerCheck rc : Search::ReachableLocations()) {
                long long k = Key(w, rc);
                if (!collected.insert(k).second) continue;
                auto it = placed.find(k);
                if (it != placed.end()) {                       // a shuffled placement
                    worlds[it->second.second]->CollectItem(it->second.first, true);
                    progress = true;
                } else if (!shuffledSet.count((int)rc)) {        // non-shuffled vanilla (in-world)
                    auto v = vanillaOf.find((int)rc);
                    if (v != vanillaOf.end()) { worlds[w]->CollectItem(v->second, true); progress = true; }
                }
            }
        }
    }
    reachedOut = collected;
    int placedReached = 0;
    for (const auto& p : placements)
        if (collected.count(Key(p.locWorld, p.loc))) placedReached++;
    return placedReached;
}
} // namespace

Result Generate(uint64_t seed, int numWorlds, const std::vector<SettingOverride>& settingOverrides,
                const ProgressFn& progress) {
    auto report = [&](float f, const char* s) { if (progress) progress(f, s); };
    report(0.01f, "Building region graph...");
    RegionTable_Init();

    // Apply caller settings on top of the baked defaults (before any logic runs).
    for (const auto& ov : settingOverrides)
        if (ov.key < (uint16_t)RSK_MAX) ctx->SetOption((RandomizerSettingKey)ov.key, ov.value);

    // Normalize settings the engine can't fully generate to the value it actually uses, so
    // the settings shipped in the .multiship match the pool that was generated (the client
    // derives its checks from these — a mismatch would leave checks unplaced). Remove an
    // entry when the engine learns to pool it.
    //
    // FEAT-5 dungeon items: the engine supports Vanilla + the placement-zone modes (Own/Any
    // Dungeon, Overworld, Anywhere) via the restricted fill below. Fold the modes it does
    // NOT model to the nearest supported one so the pool matches the shipped settings:
    // Start-With -> Vanilla (no start-grant), and the Ganon-BK LACS/100-GS win-condition
    // variants -> Own Dungeon (the boss key stays inside Ganon's Castle).
    auto foldStartWith = [&](RandomizerSettingKey k) {
        if (ctx->GetOption(k).Get() == RO_DUNGEON_ITEM_LOC_STARTWITH)
            ctx->SetOption(k, RO_DUNGEON_ITEM_LOC_VANILLA);
    };
    foldStartWith(RSK_KEYSANITY);
    foldStartWith(RSK_BOSS_KEYSANITY);
    foldStartWith(RSK_SHUFFLE_MAPANDCOMPASS);
    {
        uint8_t g = ctx->GetOption(RSK_GANONS_BOSS_KEY).Get();
        if (g == RO_GANON_BOSS_KEY_STARTWITH)
            ctx->SetOption(RSK_GANONS_BOSS_KEY, RO_GANON_BOSS_KEY_VANILLA);
        else if (g >= RO_GANON_BOSS_KEY_LACS_VANILLA)  // LACS-* + 100 GS (values 6..12)
            ctx->SetOption(RSK_GANONS_BOSS_KEY, RO_GANON_BOSS_KEY_OWN_DUNGEON);
    }
    // Scrubsanity IS supported: collapse One-Time -> All (the engine pools all scrubs).
    if (ctx->GetOption(RSK_SHUFFLE_SCRUBS).Get() != RO_SCRUBS_OFF)
        ctx->SetOption(RSK_SHUFFLE_SCRUBS, RO_SCRUBS_ALL);
    // Shopsanity IS supported, but the engine shuffles every shop slot regardless of the
    // Specific-Count/Random sub-count. Canonicalize any non-off mode to "all 8 slots" so
    // the shipped settings describe the pool the client actually receives.
    if (ctx->GetOption(RSK_SHOPSANITY).Get() != RO_SHOPSANITY_OFF) {
        ctx->SetOption(RSK_SHOPSANITY, RO_SHOPSANITY_SPECIFIC_COUNT);
        ctx->SetOption(RSK_SHOPSANITY_COUNT, RO_SHOPSANITY_COUNT_EIGHT_ITEMS);
    }
    // Fishsanity IS supported, but the engine pools every fish regardless of the
    // Loach/Pond/Overworld/Both sub-mode. Canonicalize any non-off mode to Both so the
    // shipped settings describe the pool the client receives.
    if (ctx->GetOption(RSK_FISHSANITY).Get() != RO_FISHSANITY_OFF)
        ctx->SetOption(RSK_FISHSANITY, RO_FISHSANITY_BOTH);
    // Pots/crates/freestanding are pooled in full for any non-off mode; canonicalize the
    // Dungeons/Overworld sub-modes to All so the shipped settings match the generated pool.
    if (ctx->GetOption(RSK_SHUFFLE_POTS).Get() != RO_SHUFFLE_POTS_OFF)
        ctx->SetOption(RSK_SHUFFLE_POTS, RO_SHUFFLE_POTS_ALL);
    if (ctx->GetOption(RSK_SHUFFLE_CRATES).Get() != RO_SHUFFLE_CRATES_OFF)
        ctx->SetOption(RSK_SHUFFLE_CRATES, RO_SHUFFLE_CRATES_ALL);
    if (ctx->GetOption(RSK_SHUFFLE_FREESTANDING).Get() != RO_SHUFFLE_FREESTANDING_OFF)
        ctx->SetOption(RSK_SHUFFLE_FREESTANDING, RO_SHUFFLE_FREESTANDING_ALL);
    // Cowsanity + beehives are plain toggles — no sub-mode to normalize.
    // Songs: the engine does a full (Anywhere) shuffle and can't restrict placement to
    // song-teaching locations or dungeon rewards. Canonicalize any non-off mode to
    // Anywhere so the shipped setting describes the pool the client receives (Off keeps
    // songs vanilla and is already faithful).
    if (ctx->GetOption(RSK_SHUFFLE_SONGS).Get() != RO_SONG_SHUFFLE_OFF)
        ctx->SetOption(RSK_SHUFFLE_SONGS, RO_SONG_SHUFFLE_ANYWHERE);
    // Ocarina / adult-trade / weird-egg / kokiri-sword / master-sword are plain on/off
    // toggles now read directly in IsShuffled — the shipped value already matches the
    // pool either way, so they need no normalization (like cows/beehives).
    //
    // Starting Age: SoH's user-facing RSK_STARTING_AGE (Child/Adult/Random) is normally
    // resolved by FinalizeSettings into the concrete RSK_SELECTED_STARTING_AGE that the
    // world logic + the reachability search (Search::Run) actually read. The clean-room
    // engine has no FinalizeSettings, so resolve it here: Random picks deterministically
    // from the seed (independent of the per-attempt fill salt); Child/Adult pass straight
    // through (RO_AGE_CHILD/ADULT line up with the RSK_SELECTED_STARTING_AGE choice values).
    // An adult start implies the Door of Time is already open (the Master Sword has been
    // pulled) — the model needs that so the player can time-travel back to child, so force
    // it. Collapsing Random + forcing the door keeps the shipped settings truthful (they
    // describe the world the seed was actually generated for).
    {
        uint8_t age = ctx->GetOption(RSK_STARTING_AGE).Get();
        uint8_t selected;
        if (age == RO_AGE_ADULT)      selected = RO_AGE_ADULT;
        else if (age == RO_AGE_CHILD) selected = RO_AGE_CHILD;
        else {  // RO_AGE_RANDOM (or any out-of-range value) -> deterministic per-seed pick
            std::mt19937_64 ageRng(seed ^ 0x5DA1A9E3C7B2F00DULL);
            selected = (ageRng() & 1) ? RO_AGE_ADULT : RO_AGE_CHILD;
        }
        ctx->SetOption(RSK_SELECTED_STARTING_AGE, selected);
        ctx->SetOption(RSK_STARTING_AGE, selected);  // collapse Random so the shipped value is concrete
        if (selected == RO_AGE_ADULT)
            ctx->SetOption(RSK_DOOR_OF_TIME, RO_DOOROFTIME_OPEN);
    }

    // Ganon's Trials (F-043b). The world logic gates Ganon's Tower on every NON-skipped trial
    // being cleared (ganons_castle.cpp's RR_GANONS_TOWER_ENTRYWAY), but trials default to
    // all-skipped (Context::ResetToDefaults), so without this nothing requires them. Resolve the
    // required COUNT from the setting and mark that many trials required so placement assumes the
    // real gate (Light Arrows reachable, etc.). Skip -> 0; Set Number -> RSK_TRIAL_COUNT; Random
    // Number -> a deterministic per-seed count. Which SPECIFIC trials are required does not change
    // reachability (Medallion-locked trials is off), but we choose a deterministic seed-shuffled
    // subset so the seed is reproducible, and we collapse the setting to a concrete (Set Number,
    // resolved count) which the write-back ships — the client mirrors the same count.
    {
        uint8_t trialMode = ctx->GetOption(RSK_GANONS_TRIALS).Get();
        int count;
        if (trialMode == RO_GANONS_TRIALS_SKIP) {
            count = 0;
        } else if (trialMode == RO_GANONS_TRIALS_RANDOM_NUMBER) {
            std::mt19937_64 trialRng(seed ^ 0x71A1C0FFEE5EED01ULL);
            count = (int)(trialRng() % (uint64_t)(TK_MAX + 1));  // 0..6 inclusive
        } else {  // RO_GANONS_TRIALS_SET_NUMBER (or out-of-range) -> the count slider
            count = (int)ctx->GetOption(RSK_TRIAL_COUNT).Get();
        }
        if (count < 0) count = 0;
        if (count > TK_MAX) count = TK_MAX;
        // Require the first `count` trials in canonical TrialKey order (Light, Forest, Fire, ...).
        // The SoH client mirrors this EXACT selection (savefile.cpp Randomizer_MultiShipApplyTrials),
        // so both worlds require the trials the seed was generated for. This lockstep matters:
        // different trials need different items, and the fill only guarantees the items for the
        // trials it marks required — a divergent client subset could demand an unreachable item. A
        // seed-randomized subset would need a byte-identical portable shuffle on both builds;
        // first-N is the simplest selection that stays in lockstep without that fragility.
        for (int i = 0; i < TK_MAX; ++i) ctx->GetTrial((TrialKey)i)->required = (i < count);
        // Collapse to a concrete mode + count so the shipped settings describe the real gate.
        ctx->SetOption(RSK_GANONS_TRIALS, RO_GANONS_TRIALS_SET_NUMBER);
        ctx->SetOption(RSK_TRIAL_COUNT, (uint8_t)count);
    }

    std::vector<std::shared_ptr<Rando::Logic>> worlds;
    worlds.push_back(logic);
    for (int w = 1; w < numWorlds; ++w) worlds.push_back(NewWorldLogic());

    // Location data.
    std::vector<RandomizerCheck> shuffledLocs, nonShuffledLocs;
    std::unordered_map<int, RandomizerGet> vanillaOf;
    std::unordered_set<int> shuffledSet, adv;
    for (int i = 0; i < kItemCount; ++i)
        if (kItems[i].advancement) adv.insert((int)kItems[i].rg);
    for (int i = 0; i < kLocationCount; ++i) {
        const auto& L = kLocations[i];
        if (L.rc == RC_UNKNOWN_CHECK || L.vanilla == RG_NONE) continue;
        vanillaOf[(int)L.rc] = L.vanilla;
        if (IsShuffled(L)) { shuffledLocs.push_back(L.rc); shuffledSet.insert((int)L.rc); }
        else               { nonShuffledLocs.push_back(L.rc); }
    }

    // FEAT-5: per-location dungeon zone + per-item placement restriction. A restricted
    // dungeon item may only land in locations of its allowed zone, in its own world; the
    // chest it vacates is a normal shuffled location that can receive anything.
    std::vector<uint8_t> locZone(RCM, (uint8_t)DZ_OVERWORLD);
    std::unordered_set<int> rewardLocSet;
    for (int i = 0; i < kLocationCount; ++i) {
        const auto& L = kLocations[i];
        if ((int)L.rc < RCM) locZone[(int)L.rc] = L.zone;
        if (L.category == LC_DUNGEON_REWARD) rewardLocSet.insert((int)L.rc);
    }
    auto dlocRule = [](uint8_t m) -> ZoneRule {
        switch (m) {
            case RO_DUNGEON_ITEM_LOC_OWN_DUNGEON: return ZR_OWN;
            case RO_DUNGEON_ITEM_LOC_ANY_DUNGEON: return ZR_ANY_DUNGEON;
            case RO_DUNGEON_ITEM_LOC_OVERWORLD:   return ZR_OVERWORLD;
            default:                              return ZR_NONE;  // Anywhere (Vanilla isn't pooled)
        }
    };
    auto gbkRule = [](uint8_t m) -> ZoneRule {
        switch (m) {
            case RO_GANON_BOSS_KEY_OWN_DUNGEON: return ZR_OWN;
            case RO_GANON_BOSS_KEY_ANY_DUNGEON: return ZR_ANY_DUNGEON;
            case RO_GANON_BOSS_KEY_OVERWORLD:   return ZR_OVERWORLD;
            default:                            return ZR_NONE;  // Anywhere
        }
    };
    auto rewardRule = [](uint8_t m) -> ZoneRule {
        switch (m) {
            case RO_DUNGEON_REWARDS_END_OF_DUNGEON: return ZR_REWARD_LOC;
            case RO_DUNGEON_REWARDS_OWN_DUNGEON:    return ZR_OWN;
            case RO_DUNGEON_REWARDS_ANY_DUNGEON:    return ZR_ANY_DUNGEON;
            case RO_DUNGEON_REWARDS_OVERWORLD:      return ZR_OVERWORLD;
            default:                                return ZR_NONE;  // Anywhere
        }
    };
    uint8_t mKeys = ctx->GetOption(RSK_KEYSANITY).Get();
    uint8_t mBoss = ctx->GetOption(RSK_BOSS_KEYSANITY).Get();
    uint8_t mMC   = ctx->GetOption(RSK_SHUFFLE_MAPANDCOMPASS).Get();
    uint8_t mGBK  = ctx->GetOption(RSK_GANONS_BOSS_KEY).Get();
    uint8_t mRew  = ctx->GetOption(RSK_SHUFFLE_DUNGEON_REWARDS).Get();
    std::unordered_map<int, ItemRule> itemRule;  // rg -> restriction (constrained items only)
    for (int i = 0; i < kLocationCount; ++i) {
        const auto& L = kLocations[i];
        ZoneRule zr = ZR_NONE;
        switch (L.category) {
            case LC_SMALL_KEY:            zr = dlocRule(mKeys); break;
            case LC_BOSS_KEY:             zr = dlocRule(mBoss); break;
            case LC_MAP: case LC_COMPASS: zr = dlocRule(mMC);   break;
            case LC_GANON_BOSS_KEY:       zr = gbkRule(mGBK);   break;
            case LC_DUNGEON_REWARD:       zr = rewardRule(mRew); break;
            default: continue;
        }
        if (zr != ZR_NONE) itemRule[(int)L.vanilla] = { zr, L.zone };
    }
    auto ruleFor = [&](RandomizerGet rg) -> const ItemRule* {
        auto it = itemRule.find((int)rg);
        return it == itemRule.end() ? nullptr : &it->second;
    };
    auto allowed = [&](int w, RandomizerCheck rc, const ItemRule* ir, int owner) -> bool {
        if (!ir) return true;            // unrestricted (free item, or Anywhere mode)
        if (w != owner) return false;    // restricted items stay in their own world
        uint8_t z = locZone[(int)rc];
        switch (ir->zr) {
            case ZR_OWN:         return z == ir->home;
            case ZR_ANY_DUNGEON: return z != (uint8_t)DZ_OVERWORLD;
            case ZR_OVERWORLD:   return z == (uint8_t)DZ_OVERWORLD;
            case ZR_REWARD_LOC:  return rewardLocSet.count((int)rc) != 0;
            default:             return true;
        }
    };

    // Assumed fill can occasionally strand progression (assumed-fill candidate search
    // assumes all unplaced items, so some seeds form placement cycles). Like SoH/AP, we
    // retry with a salted RNG until beatable. Same input seed => same result (the salt
    // sequence is deterministic).
    Result res;
    res.seed = seed;
    // Capture the exact settings generation runs under, so the .multiship can ship
    // them to clients (their world must match these, not their local rando menu).
    {
        const auto& opts = ctx->GetAllOptions();
        res.settings.assign(opts.begin(), opts.end());
    }
    constexpr int kMaxAttempts = 40;
    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        char stage[96];
        std::snprintf(stage, sizeof(stage), "Attempt %d - preparing item pool...", attempt + 1);
        report(0.04f, stage);
        for (auto& w : worlds) w->Reset();
        std::mt19937_64 rng(seed + (uint64_t)attempt * 0x9E3779B97F4A7C15ULL);

        std::vector<PoolItemInternal> progression, junk;
        for (int w = 0; w < numWorlds; ++w)
            for (RandomizerCheck rc : shuffledLocs) {
                RandomizerGet it = vanillaOf[(int)rc];
                (adv.count((int)it) ? progression : junk).push_back({ it, w });
            }
        // Assumed inventory per world = ALL its shuffled progression (removed as placed).
        for (const auto& p : progression) worlds[p.owner]->CollectItem(p.item, true);

        std::vector<char> empty(numWorlds * RCM, 0);
        for (int w = 0; w < numWorlds; ++w)
            for (RandomizerCheck rc : shuffledLocs) empty[Key(w, rc)] = 1;

        std::vector<Placement> placements;
        std::unordered_map<long long, std::pair<RandomizerGet, int>> placedSoFar;

        // Constraint tightness (fewer legal slots = smaller = place earlier so the scarce
        // zones aren't starved). Reward-locs are scarcest, then own-dungeon, then any/ow.
        auto tightness = [&](RandomizerGet rg) -> int {
            const ItemRule* ir = ruleFor(rg);
            if (!ir) return 4;
            switch (ir->zr) {
                case ZR_REWARD_LOC:  return 0;
                case ZR_OWN:         return 1;
                case ZR_ANY_DUNGEON: return 2;
                case ZR_OVERWORLD:   return 3;
                default:             return 4;
            }
        };
        auto byTightness = [&](const PoolItemInternal& a, const PoolItemInternal& b) {
            return tightness(a.item) < tightness(b.item);
        };

        size_t placedIdx = 0;
        const size_t progTotal = progression.size();
        // Assumed fill of one progression item: forward-sweep already-placed (cross-world to
        // owner) + non-shuffled vanilla into inventory, gather reachable empty slots the item
        // is allowed in, pick one; snapshot/restore around the sweep.
        auto placeItem = [&](const PoolItemInternal& p) {
            if ((placedIdx & 7) == 0) {
                std::snprintf(stage, sizeof(stage), "Attempt %d - placing items (%zu/%zu)",
                              attempt + 1, placedIdx, progTotal);
                report(progTotal ? 0.05f + 0.85f * ((float)placedIdx / (float)progTotal) : 0.05f, stage);
            }
            ++placedIdx;
            worlds[p.owner]->CollectItem(p.item, false);   // remove from assumed inventory

            std::vector<Rando::SaveContext> snap(numWorlds);
            for (int w = 0; w < numWorlds; ++w) snap[w] = *worlds[w]->GetSaveContext();

            std::unordered_set<long long> swept;
            bool prog = true;
            while (prog) {
                prog = false;
                for (int w = 0; w < numWorlds; ++w) {
                    logic = worlds[w];
                    for (RandomizerCheck rc : Search::ReachableLocations()) {
                        long long k = Key(w, rc);
                        if (!swept.insert(k).second) continue;
                        auto it = placedSoFar.find(k);
                        if (it != placedSoFar.end()) {
                            worlds[it->second.second]->CollectItem(it->second.first, true); prog = true;
                        } else if (!shuffledSet.count((int)rc)) {
                            auto v = vanillaOf.find((int)rc);
                            if (v != vanillaOf.end()) { worlds[w]->CollectItem(v->second, true); prog = true; }
                        }
                    }
                }
            }

            const ItemRule* ir = ruleFor(p.item);
            std::vector<std::pair<int, RandomizerCheck>> cands;
            for (int w = 0; w < numWorlds; ++w) {
                logic = worlds[w];
                for (RandomizerCheck rc : Search::ReachableLocations())
                    if (empty[Key(w, rc)] && shuffledSet.count((int)rc) && allowed(w, rc, ir, p.owner))
                        cands.push_back({ w, rc });
            }
            for (int w = 0; w < numWorlds; ++w) *worlds[w]->GetSaveContext() = snap[w];  // restore

            // Fallback: no reachable allowed slot -> any empty allowed slot (a restricted
            // item must still respect its zone so the shipped setting stays truthful).
            if (cands.empty())
                for (int w = 0; w < numWorlds; ++w)
                    for (RandomizerCheck rc : shuffledLocs)
                        if (empty[Key(w, rc)] && allowed(w, rc, ir, p.owner)) cands.push_back({ w, rc });
            if (cands.empty()) return;  // no legal slot (rare) -> leave unplaced; a retry catches the strand
            auto pick = cands[rng() % cands.size()];
            empty[Key(pick.first, pick.second)] = 0;
            placements.push_back({ pick.second, pick.first, p.item, p.owner });
            placedSoFar[Key(pick.first, pick.second)] = { p.item, p.owner };
        };

        // Restricted progression first (tightest constraint first: rewards before keys, so
        // the scarce reward-locs aren't taken by an own-dungeon key/map), then free.
        std::shuffle(progression.begin(), progression.end(), rng);
        std::vector<PoolItemInternal> restrictedProg, freeProg;
        for (const auto& p : progression) (ruleFor(p.item) ? restrictedProg : freeProg).push_back(p);
        std::stable_sort(restrictedProg.begin(), restrictedProg.end(), byTightness);
        for (const auto& p : restrictedProg) placeItem(p);

        // Reserve restricted junk (e.g. own-dungeon maps/compasses) AFTER restricted
        // progression (reward-locs etc. already claimed) but BEFORE free items (so they
        // can't steal its zone). Junk needs no reachability. Each restricted item's own
        // vacated chest is a slot in its zone, so a legal slot always exists.
        std::shuffle(junk.begin(), junk.end(), rng);
        std::vector<PoolItemInternal> freeJunk, restrictedJunk;
        for (const auto& j : junk) (ruleFor(j.item) ? restrictedJunk : freeJunk).push_back(j);
        std::stable_sort(restrictedJunk.begin(), restrictedJunk.end(), byTightness);
        for (const auto& j : restrictedJunk) {
            const ItemRule* ir = ruleFor(j.item);
            std::vector<std::pair<int, RandomizerCheck>> slots;
            for (int w = 0; w < numWorlds; ++w)
                for (RandomizerCheck rc : shuffledLocs)
                    if (empty[Key(w, rc)] && allowed(w, rc, ir, j.owner)) slots.push_back({ w, rc });
            if (slots.empty()) continue;  // no room in zone (shouldn't happen for balanced pools)
            auto pick = slots[rng() % slots.size()];
            empty[Key(pick.first, pick.second)] = 0;
            placements.push_back({ pick.second, pick.first, j.item, j.owner });
        }

        // Free progression via assumed fill (restricted items already placed/removed).
        for (const auto& p : freeProg) placeItem(p);

        // Free junk fill: drop into any remaining empty slot.
        size_t ji = 0;
        for (int w = 0; w < numWorlds && ji < freeJunk.size(); ++w)
            for (RandomizerCheck rc : shuffledLocs) {
                if (!empty[Key(w, rc)]) continue;
                if (ji >= freeJunk.size()) break;
                placements.push_back({ rc, w, freeJunk[ji].item, freeJunk[ji].owner });
                empty[Key(w, rc)] = 0;
                ++ji;
            }

        // verify: every ADVANCEMENT item reachable? (unreachable junk is harmless)
        std::snprintf(stage, sizeof(stage), "Attempt %d - verifying reachability...", attempt + 1);
        report(0.92f, stage);
        std::unordered_set<long long> reachedKeys;
        int reached = VerifyReachable(placements, numWorlds, worlds, vanillaOf, shuffledSet, reachedKeys);
        int stranded = 0;
        for (const auto& pl : placements)
            if (adv.count((int)pl.item) && !reachedKeys.count(Key(pl.locWorld, pl.loc))) stranded++;

        if (stranded == 0 || attempt == kMaxAttempts - 1) {
            res.placements = std::move(placements);
            res.reached = reached;
            res.unreachedAdvancement = stranded;
            res.beatable = (stranded == 0);
            res.attempts = attempt + 1;
            for (const auto& pl : res.placements) if (pl.itemWorld != pl.locWorld) res.crossWorld++;
            break;
        }
    }
    report(0.98f, "Finalizing...");
    return res;
}

} // namespace Fill
