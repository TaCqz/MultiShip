#include "IceTrap.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <random>
#include <string>

namespace {

struct Model {
    const char* rgName;      // RandomizerGet enum name (must match SoH exactly)
    const char* displayName; // name substituted into ${itemName} messages
};

// Disguise models the ice trap can appear as. A subset of SoH's possible ice
// trap models; every rgName is a valid RandomizerGet enum so the client resolves
// it. Extend freely as the world generator grows.
const Model kModels[] = {
    { "RG_KOKIRI_SWORD", "Kokiri Sword" },
    { "RG_MASTER_SWORD", "Master Sword" },
    { "RG_GIANTS_KNIFE", "Giant's Knife" },
    { "RG_BIGGORON_SWORD", "Biggoron's Sword" },
    { "RG_DEKU_SHIELD", "Deku Shield" },
    { "RG_HYLIAN_SHIELD", "Hylian Shield" },
    { "RG_MIRROR_SHIELD", "Mirror Shield" },
    { "RG_GORON_TUNIC", "Goron Tunic" },
    { "RG_ZORA_TUNIC", "Zora Tunic" },
    { "RG_IRON_BOOTS", "Iron Boots" },
    { "RG_HOVER_BOOTS", "Hover Boots" },
    { "RG_BOOMERANG", "Boomerang" },
    { "RG_LENS_OF_TRUTH", "Lens of Truth" },
    { "RG_MEGATON_HAMMER", "Megaton Hammer" },
    { "RG_STONE_OF_AGONY", "Stone of Agony" },
    { "RG_DINS_FIRE", "Din's Fire" },
    { "RG_FARORES_WIND", "Farore's Wind" },
    { "RG_NAYRUS_LOVE", "Nayru's Love" },
    { "RG_FIRE_ARROWS", "Fire Arrows" },
    { "RG_ICE_ARROWS", "Ice Arrows" },
    { "RG_LIGHT_ARROWS", "Light Arrows" },
    { "RG_WEIRD_EGG", "Weird Egg" },
    { "RG_ZELDAS_LETTER", "Zelda's Letter" },
    { "RG_GERUDO_MEMBERSHIP_CARD", "Gerudo Membership Card" },
};

// SoH's English ice-trap messages, verbatim. Format markers (#...# colour,
// %r/%w red/white, @ player name, & line break) are kept as-is; the SoH client
// formats them. ${itemName} is replaced with the disguise's display name.
const char* const kMessages[] = {
    "You are a #FOOL#!",
    "You are a #FOWL#!",
    "#FOOL#!",
    "You just got #PUNKED#!",
    "Stay #frosty#, @.",
    "Take a #chill pill#, @.",
    "#Winter# is coming.",
    "#ICE# to see you, @.",
    "Feeling a little %rhot%w under the collar? #Let's fix that#.",
    "It's a #cold day# in the Evil Realm.",
    "Getting #cold feet#?",
    "Say hello to the #Zoras# for me!",
    "Can you keep a #cool head#?",
    "Ganondorf used #Ice Trap#!&It's super effective!",
    "Allow me to break the #ice#!",
    "#Cold pun#.",
    "The #Titanic# would be scared of you, @.",
    "Oh no!",
    "Uh oh!",
    "What killed the dinosaurs?&The #ICE# age!",
    "Knock knock. Who's there? Ice. Ice who? Ice see that you're a #FOOL#.",
    "Never gonna #give you up#. Never gonna #let you down#. Never gonna run around and #desert you#.",
    "Thank you #@#! But your item is in another castle!",
    "#FREEZE#! Don't move!",
    "Wouldn't it be #ice# if we were colder?",
    "Greetings from #Snowhead#! Wish you were here.",
    "Too #cool# for you?",
    "#Ice#, #ice#, baby...",
    "Time to break the #ice#.",
    "We wish that you would read this... We wish that you would read this... But we set our bar low.",
    "#Freeze# and put your hands in the air!",
    "#Ice# to meet you!",
    "Do you want to #freeze# a snowman?",
    "Isn't there a #mansion# around here?",
    "Now you know how #King Zora# feels.",
    "May the #Frost# be with you.",
    "Carpe diem. #Freeze# the day.",
    "There #snow# place like home.",
    "That'll do, #ice#. That'll do.",
    "All that is #cold# does not glitter. Not all those who wander are #frost#.",
    "I Used To Be An Adventurer Like You. Then I Took An #Icetrap# To The Knee.",
    "Would you like #ice# with that?",
    "You have obtained the #Ice# Medallion!",
    "Quick, do a #Zora# impression!",
    "One %r${itemName}%w #on the rocks#!",
    "How much does a polar bear weigh?&Enough to break the #ice#.",
    "You got Din's #Ice#!",
    "You got Nayru's #Cold#!",
    "You got Farore's #Freeze#!",
    "KEKW",
    "You just got #ICE TRAPPED#! Tag your friends to totally #ICE TRAP# them!",
    "Are you okay, @? You're being #cold# today.",
    "In a moment, your game might experience some #freezing#.",
    "Breeze? Trees? Squeeze? No, it's a #freeze#!",
    "After collecting this item, @ was assaulted in #cold# blood.",
    "Only #chill# vibes around here!",
    "Here's a #cool# gift for you!",
    "Aha! You THOUGHT.",
    "Stay hydrated and brush your teeth!",
    "Isn't it too hot here? Let's turn the #AC# on.",
    "One serving of #cold# @, coming right up!",
    "Is it #cold# in here or is that just me?",
    "Yahaha! You found me!",
    "You'd make a great #ice#-tronaut!",
    "That's just the tip of the #iceberg#!",
    "It's the triforce!&No, just kidding, it's an #ice trap#.",
    "WINNER!",
    "LOSER!",
    "Greetings from #Cold Miser#!",
    "Pardon me while I turn up the #AC#.",
    "If you can't stand the #cold#, get out of the #freezer#.",
    "Oh, goodie! #Frozen @# for the main course!",
    "You have #freeze# power!",
    "You obtained the #Ice Beam#! No wait, wrong game.",
    "Here's to another lousy millenium!",
    "You've activated my #trap card#!",
    "I love #refrigerators#!",
    "You expected an item,&BUT IT WAS I, AN #ICE TRAP#!",
    "It's dangerous to go alone! Take #this#!",
    "soh.exe has #stopped responding#.",
    "Enough! My #Ice Trap# thaws in the morning!",
    "Nobody expects the span-#ice# inquisition!",
    "This is one #cool# item!",
    "Say hello to my #little friend#!",
    "We made you an offer you #can't refuse#.",
    "Hyrule? More like #Hycool#!",
    "Ice puns are #snow# problem!",
    "This #ice# is #snow# joke!",
    "There's no business like #snow# business!",
    "No, dude.",
    "N#ice# trap ya got here!",
    "Quick do your best impression of #Zoras Domain#!",
    "Ganon used #ice beam#, it's super effective!",
    "I was #frozen# today.",
    "You're not in a #hurry#, right?",
    "It's a #trap#!",
    "At least it's not a VC crash and only Link is #frozen#!",
    "Oh no! #BRAIN FREEZE#!",
    "Looks like your game #froze#! Nope just you!",
    "PK #FREEZE#!",
    "May I interest you in some #iced# Tea?",
    "Time for some Netflix and #chill#.",
    "I know, I know... #FREEZE#!",
    "#Ice# of you to drop by!",
    "STOP!&You violated the #Thaw#!",
    "I wanted to give you a treasure, but it looks like you got #cold feet#.",
    "You told me you wanted to deliver #just ice# to Ganondorf!",
    "Time to #cool off#!",
    "The #Ice Cavern# sends its regards.",
    "Loading item, please #wait#...",
    "Mash A+B to not #die#.",
    "Sorry, your %r${itemName}%w is in another location.",
    "You only wish this was %gGreg%w.",
    "Do you want to drink a hot chocolate?",
    "The #cold# never bothered me anyway.",
    "Hope you're too school for #cool#!",
    "Be thankful this isn't #absolute zero#.",
    "Did you know the F in ZFG stands for #Freeze#?",
    "You're so #cool#, @!",
    "You found #disappointment#!",
    "You got #FOOLED#!",
};

std::string Substitute(const std::string& tmpl, const std::string& itemName) {
    static const std::string placeholder = "${itemName}";
    std::string out = tmpl;
    for (size_t pos = out.find(placeholder); pos != std::string::npos; pos = out.find(placeholder, pos)) {
        out.replace(pos, placeholder.size(), itemName);
        pos += itemName.size();
    }
    return out;
}

int Clamp(int index, int count) {
    return std::max(0, std::min(index, count - 1));
}

// splitmix64: a tiny, deterministic PRNG step. Used for the seed-state path so
// world generation can reproduce ice trap rolls from a seed.
uint64_t SplitMix64(uint64_t* state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

// Returns a random index in [0, count). Deterministic+seedable via `state`,
// otherwise drawn from a shared internal RNG.
int RandIndex(int count, uint64_t* state) {
    if (count <= 0) {
        return 0;
    }
    uint64_t r;
    if (state != nullptr) {
        r = SplitMix64(state);
    } else {
        static std::mt19937_64 rng(std::random_device{}());
        r = rng();
    }
    return static_cast<int>(r % static_cast<uint64_t>(count));
}

} // namespace

int IceTrap::ModelCount() {
    return static_cast<int>(std::size(kModels));
}

const char* IceTrap::ModelDisplayName(int index) {
    return kModels[Clamp(index, ModelCount())].displayName;
}

const char* IceTrap::ModelRgName(int index) {
    return kModels[Clamp(index, ModelCount())].rgName;
}

int IceTrap::TextCount() {
    return static_cast<int>(std::size(kMessages));
}

const char* IceTrap::TextLabel(int index) {
    return kMessages[Clamp(index, TextCount())];
}

std::string IceTrap::ComposeText(int modelIndex, int textIndex) {
    return Substitute(kMessages[Clamp(textIndex, TextCount())], kModels[Clamp(modelIndex, ModelCount())].displayName);
}

int IceTrap::RandomModelIndex(uint64_t* state) {
    return RandIndex(ModelCount(), state);
}

int IceTrap::RandomTextIndex(uint64_t* state) {
    return RandIndex(TextCount(), state);
}

IceTrap::Selection IceTrap::GenerateRandom(uint64_t* state) {
    const int modelIndex = RandomModelIndex(state);
    const int textIndex = RandomTextIndex(state);
    return { ModelRgName(modelIndex), ComposeText(modelIndex, textIndex) };
}
