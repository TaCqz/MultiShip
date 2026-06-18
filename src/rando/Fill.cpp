#include "rando/Fill.h"
#include "rando/Graph.h"
#include "rando/Logic.h"
#include "rando/Search.h"
#include "rando/MetaData.h"

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
        if (L.shuffled) { shuffledLocs.push_back(L.rc); shuffledSet.insert((int)L.rc); }
        else            { nonShuffledLocs.push_back(L.rc); }
    }

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
        // Combined forward sweep candidate search: collect already-placed items (cross-
        // world to owner) + non-shuffled vanilla as locations are reached; snapshot/restore.
        std::unordered_map<long long, std::pair<RandomizerGet, int>> placedSoFar;
        std::shuffle(progression.begin(), progression.end(), rng);
        size_t placedIdx = 0;
        const size_t progTotal = progression.size();
        for (const auto& p : progression) {
            if ((placedIdx & 7) == 0) {
                std::snprintf(stage, sizeof(stage), "Attempt %d - placing items (%zu/%zu)",
                              attempt + 1, placedIdx, progTotal);
                // Placement is the bulk of the work: map it onto [0.05, 0.90].
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

            std::vector<std::pair<int, RandomizerCheck>> cands;
            for (int w = 0; w < numWorlds; ++w) {
                logic = worlds[w];
                for (RandomizerCheck rc : Search::ReachableLocations())
                    if (empty[Key(w, rc)] && shuffledSet.count((int)rc)) cands.push_back({ w, rc });
            }
            for (int w = 0; w < numWorlds; ++w) *worlds[w]->GetSaveContext() = snap[w];  // restore

            if (cands.empty())
                for (int w = 0; w < numWorlds; ++w)
                    for (RandomizerCheck rc : shuffledLocs)
                        if (empty[Key(w, rc)]) cands.push_back({ w, rc });
            auto pick = cands[rng() % cands.size()];
            empty[Key(pick.first, pick.second)] = 0;
            placements.push_back({ pick.second, pick.first, p.item, p.owner });
            placedSoFar[Key(pick.first, pick.second)] = { p.item, p.owner };
        }

        // junk fill
        std::shuffle(junk.begin(), junk.end(), rng);
        size_t ji = 0;
        for (int w = 0; w < numWorlds && ji < junk.size(); ++w)
            for (RandomizerCheck rc : shuffledLocs) {
                if (!empty[Key(w, rc)]) continue;
                if (ji >= junk.size()) break;
                placements.push_back({ rc, w, junk[ji].item, junk[ji].owner });
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
