// F-044 verification: Tab 1 section 1.1 "Logic" (start-state + child-era settings) is honored
// end-to-end by the generator. These keys are on the production honored-settings allowlist
// (Generator.cpp): Starting Age (+ Selected Starting Age), Full Wallets, Skip Child Zelda,
// Mask Quest, Skip Child Stealth, Skip Epona Race. Asserts, with non-zero exit on any failure:
//
//   1. SHIPPED VALUES: each setting round-trips into the seed's settings, so the client receives
//      exactly what placement assumed (lockstep). Booleans + Mask Quest Completed.
//   2. STARTING AGE / WRITE-BACK: an adult start collapses Starting Age -> Selected Starting Age
//      = Adult, ships Starting Age = Adult, and forces (and ships) Door of Time = Open — the
//      effective values the client must apply, not the raw input.
//   3. BEATABLE: every individual setting, AND all of them together, stay jointly beatable — i.e.
//      Skip Child Zelda's reachability (Zelda's Letter from root, Weird Egg not required) holds,
//      and Mask Quest Completed's borrow logic doesn't strand anything.
//   4. MASK QUEST SHUFFLE FOLD: a (preset-only) Shuffle value folds to Vanilla in the shipped
//      settings — the engine has no mask item-locations to shuffle, so it never ships Shuffle.
//   5. DETERMINISM: generating the all-on seed twice is identical.
//
// Standalone: the engine is dependency-free, so this links only multiship_rando.
#include "rando/Generator.h"
#include "rando/SeedFile.h"
#include "rando/randomizerEnums.h"

#include <cstdio>
#include <vector>

namespace {

int failures = 0;
void check(bool cond, const char* what) {
    std::printf("  [%s] %s\n", cond ? "OK" : "FAIL", what);
    if (!cond) ++failures;
}

bool SamePlacements(const SeedFile::SeedData& a, const SeedFile::SeedData& b) {
    if (a.placements.size() != b.placements.size()) return false;
    for (size_t i = 0; i < a.placements.size(); ++i) {
        const auto& x = a.placements[i];
        const auto& y = b.placements[i];
        if (x.loc != y.loc || x.locWorld != y.locWorld || x.item != y.item ||
            x.ownerWorld != y.ownerWorld)
            return false;
    }
    return true;
}

// The value the seed ships for `key`, or -1 if the key is absent.
int shipped(const SeedFile::SeedData& sd, RandomizerSettingKey key) {
    for (const auto& s : sd.settings)
        if (s.key == (uint16_t)key) return (int)s.value;
    return -1;
}

Generator::Options baseOpts() {
    Generator::Options o;
    o.seed = 0xC0FFEEULL;
    o.players = { "Mori", "TaCqz" };
    return o;
}

constexpr uint16_t kOn = 1;   // OPT_BOOL "on" (curated booleans store 0/1 directly)
constexpr uint16_t kOff = 0;

} // namespace

int main() {
    // --- 1. Adult start: resolution + write-back + Door-of-Time force --------------------
    // Selected Starting Age and Door of Time must be PRESENT in the input for the overwrite-only
    // write-back to ship their effective values (the curated set always includes them; here we
    // pass them explicitly with throwaway values that the generator overwrites).
    Generator::Options adultO = baseOpts();
    adultO.settings = {
        { (uint16_t)RSK_STARTING_AGE,          (uint16_t)RO_AGE_ADULT },
        { (uint16_t)RSK_SELECTED_STARTING_AGE, (uint16_t)RO_AGE_CHILD },     // overwritten -> Adult
        { (uint16_t)RSK_DOOR_OF_TIME,          (uint16_t)RO_DOOROFTIME_CLOSED }, // overwritten -> Open
    };
    std::printf("=== adult start: resolution + write-back ===\n");
    Generator::Output adult = Generator::Generate(adultO);
    std::printf("  %zu placements, beatable=%d\n", adult.seed.placements.size(), (int)adult.beatable);
    check(adult.beatable, "adult-start seed is jointly beatable");
    check(shipped(adult.seed, RSK_STARTING_AGE) == RO_AGE_ADULT,
          "shipped Starting Age = Adult (collapsed)");
    check(shipped(adult.seed, RSK_SELECTED_STARTING_AGE) == RO_AGE_ADULT,
          "write-back ships Selected Starting Age = Adult (resolved), not the Child input");
    check(shipped(adult.seed, RSK_DOOR_OF_TIME) == RO_DOOROFTIME_OPEN,
          "write-back ships Door of Time = Open (forced by adult start), not the Closed input");

    // Child start (default) leaves Door of Time at its baked default (Open) and ships Child.
    Generator::Options childO = baseOpts();
    childO.settings = {
        { (uint16_t)RSK_STARTING_AGE,          (uint16_t)RO_AGE_CHILD },
        { (uint16_t)RSK_SELECTED_STARTING_AGE, (uint16_t)RO_AGE_ADULT },     // overwritten -> Child
    };
    Generator::Output child = Generator::Generate(childO);
    check(child.beatable, "child-start seed is jointly beatable");
    check(shipped(child.seed, RSK_SELECTED_STARTING_AGE) == RO_AGE_CHILD,
          "write-back ships Selected Starting Age = Child for a child start");

    // --- 2. Skip Child Zelda ON stays beatable (letter from root, egg not required) ------
    Generator::Options scz = baseOpts();
    scz.settings = { { (uint16_t)RSK_SKIP_CHILD_ZELDA, kOn } };
    std::printf("=== Skip Child Zelda = On ===\n");
    Generator::Output sczOut = Generator::Generate(scz);
    std::printf("  %zu placements, beatable=%d\n", sczOut.seed.placements.size(), (int)sczOut.beatable);
    check(sczOut.beatable, "Skip Child Zelda = On is jointly beatable");
    check(shipped(sczOut.seed, RSK_SKIP_CHILD_ZELDA) == kOn, "shipped Skip Child Zelda = On");

    // --- 3. Mask Quest Completed beatable; Shuffle folds to Vanilla ----------------------
    Generator::Options maskC = baseOpts();
    maskC.settings = { { (uint16_t)RSK_MASK_QUEST, (uint16_t)RO_MASK_QUEST_COMPLETED } };
    std::printf("=== Mask Quest = Completed ===\n");
    Generator::Output maskCOut = Generator::Generate(maskC);
    check(maskCOut.beatable, "Mask Quest = Completed is jointly beatable");
    check(shipped(maskCOut.seed, RSK_MASK_QUEST) == RO_MASK_QUEST_COMPLETED,
          "shipped Mask Quest = Completed");

    Generator::Options maskS = baseOpts();
    maskS.settings = { { (uint16_t)RSK_MASK_QUEST, (uint16_t)RO_MASK_QUEST_SHUFFLE } };
    Generator::Output maskSOut = Generator::Generate(maskS);
    check(maskSOut.beatable, "Mask Quest = Shuffle (folded) is jointly beatable");
    check(shipped(maskSOut.seed, RSK_MASK_QUEST) == RO_MASK_QUEST_VANILLA,
          "Mask Quest Shuffle folds to Vanilla in the shipped settings (engine cannot pool masks)");

    // --- 4. The remaining toggles ship their value + stay beatable -----------------------
    Generator::Options wallets = baseOpts();
    wallets.settings = { { (uint16_t)RSK_FULL_WALLETS, kOn } };
    Generator::Output walletsOut = Generator::Generate(wallets);
    check(walletsOut.beatable, "Full Wallets = On is jointly beatable");
    check(shipped(walletsOut.seed, RSK_FULL_WALLETS) == kOn,
          "shipped Full Wallets = On (no placement effect; client applies the rupees)");

    Generator::Options stealth = baseOpts();
    stealth.settings = { { (uint16_t)RSK_SKIP_CHILD_STEALTH, kOn } };
    Generator::Output stealthOut = Generator::Generate(stealth);
    check(stealthOut.beatable, "Skip Child Stealth = On is jointly beatable");
    check(shipped(stealthOut.seed, RSK_SKIP_CHILD_STEALTH) == kOn, "shipped Skip Child Stealth = On");

    Generator::Options epona = baseOpts();
    epona.settings = { { (uint16_t)RSK_SKIP_EPONA_RACE, kOn } };
    Generator::Output eponaOut = Generator::Generate(epona);
    check(eponaOut.beatable, "Skip Epona Race = On is jointly beatable");
    check(shipped(eponaOut.seed, RSK_SKIP_EPONA_RACE) == kOn, "shipped Skip Epona Race = On");

    // --- 5. Everything on together is beatable + ships correctly + deterministic ---------
    Generator::Options allO = baseOpts();
    allO.settings = {
        { (uint16_t)RSK_STARTING_AGE,          (uint16_t)RO_AGE_ADULT },
        { (uint16_t)RSK_SELECTED_STARTING_AGE, (uint16_t)RO_AGE_CHILD },  // -> Adult
        { (uint16_t)RSK_FULL_WALLETS,          kOn },
        { (uint16_t)RSK_SKIP_CHILD_ZELDA,      kOn },
        { (uint16_t)RSK_MASK_QUEST,            (uint16_t)RO_MASK_QUEST_COMPLETED },
        { (uint16_t)RSK_SKIP_CHILD_STEALTH,    kOn },
        { (uint16_t)RSK_SKIP_EPONA_RACE,       kOn },
    };
    std::printf("=== all F-044 settings on together ===\n");
    Generator::Output all = Generator::Generate(allO);
    std::printf("  %zu placements, beatable=%d\n", all.seed.placements.size(), (int)all.beatable);
    check(all.beatable, "all F-044 settings on together is jointly beatable");
    check(shipped(all.seed, RSK_SELECTED_STARTING_AGE) == RO_AGE_ADULT, "all: Selected Starting Age = Adult");
    check(shipped(all.seed, RSK_FULL_WALLETS) == kOn, "all: Full Wallets = On");
    check(shipped(all.seed, RSK_SKIP_CHILD_ZELDA) == kOn, "all: Skip Child Zelda = On");
    check(shipped(all.seed, RSK_MASK_QUEST) == RO_MASK_QUEST_COMPLETED, "all: Mask Quest = Completed");
    check(shipped(all.seed, RSK_SKIP_CHILD_STEALTH) == kOn, "all: Skip Child Stealth = On");
    check(shipped(all.seed, RSK_SKIP_EPONA_RACE) == kOn, "all: Skip Epona Race = On");

    Generator::Output all2 = Generator::Generate(allO);
    check(SamePlacements(all.seed, all2.seed), "all-on seed is deterministic");

    std::printf("\n%s (%d failure(s))\n", failures == 0 ? "ALL PASS" : "FAILED", failures);
    return failures == 0 ? 0 : 1;
}
