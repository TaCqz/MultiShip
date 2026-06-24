// ItemNames — a standalone RandomizerGet (RG_) -> display-name catalog.
//
// The rando engine (Fill/Logic/MetaData/...) was removed in the MultiShip reset,
// but the give_item debug tool still needs to list every item by name. This header
// derives that list directly from the kept enum (enums/RandomizerGet.h) using the
// same X-macro the enum already supports, so it stays in sync with the enum with no
// generated data file. The display name is simply the RG_ token string, which is
// exactly what the client's give_item handler expects (it resolves the RG_ name).
#pragma once
#include "randomizerEnums.h"

#include <vector>

namespace ItemCatalog {

struct Item {
    RandomizerGet rg;
    const char* name;  // the RG_ enum token, e.g. "RG_KOKIRI_SWORD"
};

// Every RandomizerGet value with its RG_ token name, in enum order. Built once.
inline const std::vector<Item>& Items() {
    static const std::vector<Item> kItems = {
// Re-expand RandomizerGet.h with our own macros. randomizerEnums.h already cleaned
// up the default macros after defining the enum type above, and RandomizerGet.h has
// no include guard (it is designed to be re-included as an X-macro), so this is safe.
#define RANDO_ENUM_BEGIN(EnumName)
#define RANDO_ENUM_ITEM(itemName, ...) Item{ itemName, #itemName },
#define RANDO_ENUM_END(EnumName)
#include "enums/RandomizerGet.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
    };
    return kItems;
}

} // namespace ItemCatalog
