#ifndef MULTISHIP_ICETRAP_H
#define MULTISHIP_ICETRAP_H

#include <cstdint>
#include <string>

// Ice-trap presets mirrored from Ship of Harkinian so the MultiShip server owns
// the disguise models and messages itself (and, later, when it generates a
// randomized world). Kept independent of the SoH source on purpose.
namespace IceTrap {

// Preset disguise models (a RandomizerGet the ice trap can look like).
int ModelCount();
const char* ModelDisplayName(int index); // human-readable label for the picker
const char* ModelRgName(int index);       // RandomizerGet enum name to send

// Preset ice-trap textbox messages (SoH's, with format markers intact).
int TextCount();
const char* TextLabel(int index);

// Final iceTrapText to send for a chosen model + text preset: the message with
// its ${itemName} placeholder filled from the model's display name.
std::string ComposeText(int modelIndex, int textIndex);

// A rolled ice trap, ready to send: model = RandomizerGet enum name,
// text = the composed (${itemName}-substituted) message.
struct Selection {
    std::string model;
    std::string text;
};

// Random selection. `state` is an optional seed-state: pass a pointer to a
// uint64_t seed for DETERMINISTIC, reproducible results (e.g. world generation
// placing ice traps at locations) — it is advanced on each call; pass nullptr
// for ad-hoc randomness (the server UI's "Random" option).
int RandomModelIndex(uint64_t* state = nullptr);
int RandomTextIndex(uint64_t* state = nullptr);
Selection GenerateRandom(uint64_t* state = nullptr);

} // namespace IceTrap

#endif // MULTISHIP_ICETRAP_H
