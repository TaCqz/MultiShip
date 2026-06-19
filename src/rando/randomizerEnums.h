// Default expansion: real enums
#pragma once

#if !defined(RANDO_ENUM_BEGIN) && !defined(RANDO_ENUM_ITEM) && !defined(RANDO_ENUM_END)
// clang-format off
#define RANDO_ENUM_BEGIN(EnumName) typedef enum EnumName {
#define RANDO_ENUM_ITEM(name, ...) name __VA_OPT__(=) __VA_ARGS__,
#define RANDO_ENUM_END(EnumName) } EnumName;
#define RANDO_ENUM__CLEANUP
// clang-format on
#endif

#include "enums/LogicVal.h"
#include "enums/RandomizerCheck.h"
#include "enums/RandomizerGet.h"
#include "enums/RandomizerHintTextKey.h"
#include "enums/RandomizerInf.h"
#include "enums/RandomizerMiscEnums.h"
#include "enums/RandomizerOptions.h"
#include "enums/RandomizerRegion.h"
#include "enums/RandomizerSettingKey.h"
#include "enums/RandomizerTrick.h"

// Clean up only the defaults we defined.
#ifdef RANDO_ENUM__CLEANUP
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
#undef RANDO_ENUM__CLEANUP
#endif
