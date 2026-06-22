// MultiShipGen — standalone CLI multiworld seed generator (no SDL/ImGui).
// Usage: MultiShipGen [--seed N] [--p1 NAME] [--p2 NAME] [--out DIR]
//                     [--set KEY=VAL ...] [--list-settings]
//   --seed N         deterministic seed value (default: random)
//   --p1/--p2        player names (default Player 1 / Player 2)
//   --out DIR        output directory (default: current dir)
//   --set KEY=VAL    override a setting before generation. KEY is an RSK_ enum name
//                    (e.g. RSK_SHUFFLE_SONGS, case-insensitive) or its numeric index;
//                    VAL is the RO_ option value as an integer. Repeatable.
//   --list-settings  print every RSK_ name with its index and exit.
// Writes <out>/<seed>.multiship and <out>/<seed>_spoiler.txt. The shipped settings the
// seed was generated under are printed, so you can confirm a toggle took effect (e.g.
// Shuffle Songs folds to Anywhere when enabled).
#include "rando/Fill.h"
#include "rando/SeedFile.h"
#include "rando/randomizerEnums.h"
#include <cctype>
#include <cstdio>
#include <cstring>
#include <random>
#include <string>
#include <vector>

namespace {
// Re-include the RSK X-macro list with local macros to build a name->index table, so
// --set can take human RSK_ names instead of bare indices.
struct RskName { const char* name; int value; };
const RskName kRsk[] = {
#define RANDO_ENUM_BEGIN(n)
#define RANDO_ENUM_ITEM(name, ...) { #name, (int)name },
#define RANDO_ENUM_END(n)
#include "rando/enums/RandomizerSettingKey.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
};

bool IEq(const std::string& a, const char* b) {
    size_t i = 0;
    for (; i < a.size() && b[i]; ++i)
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    return i == a.size() && b[i] == '\0';
}

// Resolve an RSK key from a numeric index or an RSK_ name. Returns -1 if unknown.
int ResolveKey(const std::string& key) {
    if (!key.empty() && std::isdigit((unsigned char)key[0]))
        return (int)std::strtol(key.c_str(), nullptr, 10);
    for (const auto& e : kRsk)
        if (IEq(key, e.name)) return e.value;
    return -1;
}
}  // namespace

int main(int argc, char** argv) {
    uint64_t seed = 0;
    bool haveSeed = false;
    std::string p1 = "Player 1", p2 = "Player 2", out = ".";
    std::vector<Fill::SettingOverride> overrides;

    for (int i = 1; i < argc; ++i) {
        auto next = [&](const char* def) { return (i + 1 < argc) ? argv[++i] : def; };
        if (!std::strcmp(argv[i], "--seed")) { seed = std::strtoull(next("0"), nullptr, 10); haveSeed = true; }
        else if (!std::strcmp(argv[i], "--p1")) p1 = next("Player 1");
        else if (!std::strcmp(argv[i], "--p2")) p2 = next("Player 2");
        else if (!std::strcmp(argv[i], "--out")) out = next(".");
        else if (!std::strcmp(argv[i], "--list-settings")) {
            for (const auto& e : kRsk) std::printf("%4d  %s\n", e.value, e.name);
            return 0;
        }
        else if (!std::strcmp(argv[i], "--set")) {
            std::string kv = next("");
            auto eq = kv.find('=');
            if (eq == std::string::npos) { std::printf("ERROR --set expects KEY=VAL, got '%s'\n", kv.c_str()); return 2; }
            std::string key = kv.substr(0, eq), val = kv.substr(eq + 1);
            int k = ResolveKey(key);
            if (k < 0) { std::printf("ERROR unknown setting '%s' (try --list-settings)\n", key.c_str()); return 2; }
            int v = (int)std::strtol(val.c_str(), nullptr, 10);
            overrides.push_back({ (uint16_t)k, (uint8_t)v });
            std::printf("  override: %s (#%d) = %d\n", key.c_str(), k, v);
        }
        else { std::printf("ERROR unknown argument '%s'\n", argv[i]); return 2; }
    }
    if (!haveSeed) {
        std::random_device rd;
        seed = ((uint64_t)rd() << 32) ^ rd();
    }

    std::vector<std::string> players = { p1, p2 };
    std::printf("Generating MultiWorld seed %llu for [%s, %s]...\n",
                (unsigned long long)seed, p1.c_str(), p2.c_str());
    Fill::Result r = Fill::Generate(seed, 2, overrides);
    std::printf("  %zu placements, %d cross-world, %d attempt(s), beatable=%s\n",
                r.placements.size(), r.crossWorld, r.attempts, r.beatable ? "YES" : "NO");
    if (!r.beatable) { std::printf("  WARNING: seed not beatable (progression stranded)\n"); }

    // Echo the SHIPPED value of each overridden setting (post-normalization), so a fold
    // like Shuffle Songs -> Anywhere is visible in the output.
    for (const auto& ov : overrides)
        if (ov.key < r.settings.size())
            std::printf("  shipped: #%u = %u\n", ov.key, r.settings[ov.key]);

    std::string base = out + "/" + std::to_string(seed);
    std::string err;
    if (!SeedFile::WriteMultiship(base + ".multiship", r, players, err)) {
        std::printf("ERROR writing .multiship: %s\n", err.c_str()); return 1;
    }
    if (!SeedFile::WriteSpoiler(base + "_spoiler.txt", r, players, err)) {
        std::printf("ERROR writing spoiler: %s\n", err.c_str()); return 1;
    }
    std::printf("Wrote %s.multiship and %s_spoiler.txt\n", base.c_str(), base.c_str());
    return 0;
}
