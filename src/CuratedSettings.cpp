// Curated randomizer-settings table — see CuratedSettings.h.
//
// Labels, option values, defaults and tooltips are reproduced from
// docs/multiship-settings.md (verbatim base-game tooltips, including the original
// typos noted there). Two defaults intentionally override base SoH:
//   - Skip Child Stealth  -> On            (base: Off)
//   - Ganon's Trials      -> Random Number (base: Set Number)
#include "CuratedSettings.h"

namespace ms {

// Tab ids (also the visible tab labels).
static constexpr const char* TAB_LOGIC    = "Logic/Access";
static constexpr const char* TAB_DUNGEONS = "Dungeons";
static constexpr const char* TAB_SHUFFLES = "Shuffles";
static constexpr const char* TAB_HINTS    = "Hints/Traps";
static constexpr const char* TAB_START    = "Starting Items";

// Shared option lists (the six dungeon-item destinations recur verbatim).
static const std::vector<Opt> kDungeonItemDest = {
    { "Start With", 0 }, { "Vanilla", 1 }, { "Own Dungeon", 2 },
    { "Any Dungeon", 3 }, { "Overworld", 4 }, { "Anywhere", 5 },
};

const std::vector<const char*>& Tabs() {
    static const std::vector<const char*> t = {
        TAB_LOGIC, TAB_DUNGEONS, TAB_SHUFFLES, TAB_HINTS, TAB_START,
    };
    return t;
}

const std::vector<Setting>& CuratedSettings() {
    static const std::vector<Setting> table = {
        // =========================================================== Logic/Access
        // -- Logic (Item Pool / Logic / All Locations Reachable are fixed+hidden) --
        Setting{ .key = RSK_ITEM_POOL, .label = "Item Pool", .ui = Ui::Combo,
                 .defaultValue = 1 /* Balanced */, .tab = TAB_LOGIC, .section = "Logic",
                 .hidden = true },
        Setting{ .key = RSK_LOGIC_RULES, .label = "Logic", .ui = Ui::Combo,
                 .defaultValue = 0 /* Glitchless */, .tab = TAB_LOGIC, .section = "Logic",
                 .hidden = true },
        Setting{ .key = RSK_ALL_LOCATIONS_REACHABLE, .label = "All Locations Reachable",
                 .ui = Ui::Checkbox, .defaultValue = 1 /* On */, .tab = TAB_LOGIC,
                 .section = "Logic", .hidden = true },

        Setting{ .key = RSK_STARTING_AGE, .label = "Starting Age", .ui = Ui::Combo,
                 .options = { { "Child", 0 }, { "Adult", 1 } }, .defaultValue = 0,
                 .tooltip = "Choose which age Link will start as.\n\n"
                            "Starting as adult means you start with the Master Sword in your inventory.\n"
                            "The child option is forcefully set if it would conflict with other options.",
                 .tab = TAB_LOGIC, .section = "Logic" },
        Setting{ .key = RSK_FULL_WALLETS, .label = "Full Wallets", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Start with a full wallet. All wallet upgrades come filled with rupees.",
                 .tab = TAB_LOGIC, .section = "Logic" },
        Setting{ .key = RSK_SKIP_CHILD_ZELDA, .label = "Skip Child Zelda", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Start with Zelda's Letter and the item Impa would normally give you and "
                            "skip the sequence up until after meeting Zelda. Disables the ability to "
                            "shuffle Weird Egg.",
                 .tab = TAB_LOGIC, .section = "Logic" },
        Setting{ .key = RSK_MASK_QUEST, .label = "Mask Quest", .ui = Ui::Combo,
                 .options = { { "Vanilla", 0 }, { "Completed", 1 }, { "Shuffle", 2 } },
                 .defaultValue = 0,
                 .tooltip = "How masks are acquired.\n"
                            "Vanilla - Mask trade quest.\n\n"
                            "Completed - Once the Happy Mask Shop is opened, all masks will be "
                            "available to be borrowed.\n\n"
                            "Shuffle - Happy Mask Shop never opens, masks are shuffled with rest of items.",
                 .tab = TAB_LOGIC, .section = "Logic" },
        Setting{ .key = RSK_SKIP_CHILD_STEALTH, .label = "Skip Child Stealth", .ui = Ui::Checkbox,
                 .defaultValue = 1 /* MultiShip override (base: Off) */,
                 .tooltip = "The crawlspace into Hyrule Castle goes straight to Zelda, skipping the guards.",
                 .tab = TAB_LOGIC, .section = "Logic", .greyWhenKeyOn = RSK_SKIP_CHILD_ZELDA },
        Setting{ .key = RSK_SKIP_EPONA_RACE, .label = "Skip Epona Race", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Epona can be summoned with Epona's Song without needing to race Ingo.",
                 .tab = TAB_LOGIC, .section = "Logic" },
        Setting{ .key = RSK_GANONS_BOSS_KEY, .label = "Ganon's Boss Key", .ui = Ui::Combo,
                 .options = { { "Vanilla", 0 }, { "Own Dungeon", 1 }, { "Start With", 2 },
                              { "Any Dungeon", 3 }, { "Overworld", 4 }, { "Anywhere", 5 } },
                 .defaultValue = 0,
                 .tooltip = "Vanilla - Ganon's Boss Key will appear in the vanilla location.\n\n"
                            "Own dungeon - Ganon's Boss Key can appear anywhere inside Ganon's Castle.\n\n"
                            "Start with - Places Ganon's Boss Key in your starting inventory.\n"
                            "Any dungeon - Ganon's Boss Key Key can only appear inside of any dungeon.\n\n"
                            "Overworld - Ganon's Boss Key Key can only appear outside of dungeons.\n\n"
                            "Anywhere - Ganon's Boss Key Key can appear anywhere in the world.",
                 .tab = TAB_LOGIC, .section = "Logic" },

        // -- Area Access --
        Setting{ .key = RSK_FOREST, .label = "Open Forest", .ui = Ui::Combo,
                 .options = { { "Closed", 0 }, { "Deku Only", 1 }, { "Open", 2 } },
                 .defaultValue = 0,
                 .tooltip = "Determines if Kokiri forest can be left for the Lost Woods bridge or the "
                            "Deku Tree.\n\n"
                            "On - Kokiri Sword & Deku Shield are required to access the Deku Tree, and "
                            "completing the Deku Tree is required to access the Lost Woods Bridge Exit.\n\n"
                            "Deku Only - Kokiri boy no longer blocks the path to the Bridge but Mido "
                            "still requires the Kokiri Sword and Deku Shield to access the tree.\n\n"
                            "Off - Mido no longer blocks the path to the Deku Tree. Kokiri boy no longer "
                            "blocks the path out of the forest.",
                 .tab = TAB_LOGIC, .section = "Area Access" },
        Setting{ .key = RSK_KAK_GATE, .label = "Kakariko Gate", .ui = Ui::Combo,
                 .options = { { "Closed", 0 }, { "Open", 1 } }, .defaultValue = 0,
                 .tooltip = "Closed - The gate will remain closed until Zelda's Letter is shown to the guard.\n\n"
                            "Open - The gate is always open. The Happy Mask Shop will open immediately "
                            "after obtaining Zelda's Letter.",
                 .tab = TAB_LOGIC, .section = "Area Access" },
        Setting{ .key = RSK_DOOR_OF_TIME, .label = "Door of Time", .ui = Ui::Combo,
                 .options = { { "Closed", 0 }, { "Song Only", 1 }, { "Open", 2 } }, .defaultValue = 0,
                 .tooltip = "Closed - The Ocarina of Time, the Song of Time and all three Spiritual "
                            "Stones are required to open the Door of Time.\n\n"
                            "Song only - Play the Song of Time in front of the Door of Time to open it.\n\n"
                            "Open - The Door of Time is permanently open with no requirements.",
                 .tab = TAB_LOGIC, .section = "Area Access" },
        Setting{ .key = RSK_ZORAS_FOUNTAIN, .label = "Zora's Fountain", .ui = Ui::Combo,
                 .options = { { "Closed", 0 }, { "Closed as Child", 1 }, { "Open", 2 } }, .defaultValue = 0,
                 .tooltip = "Closed - King Zora obstructs the way to Zora's Fountain. Ruto's Letter must "
                            "be shown as child Link in order to move him in both time periods.\n\n"
                            "Closed as child - Ruto's Letter is only required to move King Zora as child "
                            "Link. Zora's Fountain starts open as adult.\n\n"
                            "Open - King Zora has already mweeped out of the way in both time periods. "
                            "Ruto's Letter is removed from the item pool.",
                 .tab = TAB_LOGIC, .section = "Area Access" },
        Setting{ .key = RSK_SLEEPING_WATERFALL, .label = "Sleeping Waterfall", .ui = Ui::Combo,
                 .options = { { "Closed", 0 }, { "Open", 1 } }, .defaultValue = 0,
                 .tooltip = "Closed - Sleeping Waterfall obstructs the entrance to Zora's Domain. Zelda's "
                            "Lullaby must be played in order to open it (but only once; then it stays open "
                            "in both time periods).\n\n"
                            "Open - Sleeping Waterfall is always open. Link may always enter Zora's Domain.",
                 .tab = TAB_LOGIC, .section = "Area Access" },
        Setting{ .key = RSK_JABU_OPEN, .label = "Jabu-Jabu", .ui = Ui::Combo,
                 .options = { { "Closed", 0 }, { "Open", 1 } }, .defaultValue = 0,
                 .tooltip = "Closed - A fish is required to open Jabu-Jabu's mouth.\n\n"
                            "Open - Jabu-Jabu's mouth opens without the need for a fish.",
                 .tab = TAB_LOGIC, .section = "Area Access" },
        Setting{ .key = RSK_GERUDO_FORTRESS, .label = "Fortress Carpenters", .ui = Ui::Combo,
                 .options = { { "Normal", 0 }, { "Fast", 1 } }, .defaultValue = 0,
                 .tooltip = "Sets the state of the carpenters captured by Gerudo in Gerudo Fortress, and "
                            "with it the number of guards that spawn.\n\n"
                            "Normal - All 4 carpenters are required to be saved.\n\n"
                            "Fast - Only the bottom left carpenter requires rescuing.\n\n"
                            "Free - Bridge is repaired from start, and Nabooru cannot spawn.\n"
                            "If the Gerudo Membership Card isn't shuffled, you start with it.\n\n"
                            "Only \"Normal\" is compatible with Gerudo Fortress Key Rings.",
                 .tab = TAB_LOGIC, .section = "Area Access" },
        Setting{ .key = RSK_RAINBOW_BRIDGE, .label = "Rainbow Bridge", .ui = Ui::Combo,
                 .options = { { "Vanilla", 0 }, { "Always Open", 1 }, { "Stones", 2 }, { "Medallions", 3 } },
                 .defaultValue = 0,
                 .tooltip = "Alters the requirements to open the bridge to Ganon's Castle.\n\n"
                            "Vanilla - Obtain the Shadow Medallion, Spirit Medallion and Light Arrows.\n\n"
                            "Always open - No requirements.\n\n"
                            "Stones - Obtain the specified amount of Spiritual Stones.\n\n"
                            "Medallions - Obtain the specified amount of medallions.\n\n"
                            "Dungeon rewards - Obtain the specified total sum of Spiritual Stones or "
                            "medallions.\n\n"
                            "Dungeons - Complete the specified amount of dungeons. Dungeons are considered "
                            "complete after stepping in to the blue warp after the boss.\n\n"
                            "Tokens - Obtain the specified amount of Skulltula tokens.\n\n"
                            "Greg - Find Greg the Green Rupee.",
                 .tab = TAB_LOGIC, .section = "Area Access" },
        Setting{ .key = RSK_GANONS_TRIALS, .label = "Ganon's Trials", .ui = Ui::Combo,
                 .options = { { "Skip", 0 }, { "Set Number", 1 }, { "Random Number", 2 } },
                 .defaultValue = 2 /* MultiShip override (base: Set Number) */,
                 .tooltip = "Sets the number of Ganon's Trials required to dispel the barrier.\n\n"
                            "Skip - No Trials are required and the barrier is already dispelled.\n\n"
                            "Set Number - Select a number of trials that will be required from the slider "
                            "below. Which specific trials you need to complete will be random.\n\n"
                            "Random Number - A random number and set of trials will be required.",
                 .tab = TAB_LOGIC, .section = "Area Access" },

        // ============================================================== Dungeons
        // -- Dungeon Items --
        Setting{ .key = RSK_SHUFFLE_MAPANDCOMPASS, .label = "Maps & Compasses", .ui = Ui::Combo,
                 .options = kDungeonItemDest, .defaultValue = 2 /* Own Dungeon */,
                 .tooltip = "Start with - You will start with Maps & Compasses from all dungeons.\n\n"
                            "Vanilla - Maps & Compasses will appear in their vanilla locations.\n\n"
                            "Own dungeon - Maps & Compasses can only appear in their respective dungeon.\n\n"
                            "Any dungeon - Maps & Compasses can only appear inside of any dungeon.\n\n"
                            "Overworld - Maps & Compasses can only appear outside of dungeons.\n\n"
                            "Anywhere - Maps & Compasses can appear anywhere in the world.",
                 .tab = TAB_DUNGEONS, .section = "Dungeon Items" },
        Setting{ .key = RSK_KEYSANITY, .label = "Small Key Shuffle", .ui = Ui::Combo,
                 .options = kDungeonItemDest, .defaultValue = 2 /* Own Dungeon */,
                 .tooltip = "Start with - You will start with all Small Keys from all dungeons.\n\n"
                            "Vanilla - Small Keys will appear in their vanilla locations. You start with "
                            "3 keys in Spirit Temple MQ because the vanilla key layout is not beatable in "
                            "logic.\n\n"
                            "Own dungeon - Small Keys can only appear in their respective dungeon. If Fire "
                            "Temple is not a Master Quest dungeon, the door to the Boss Key chest will be "
                            "unlocked.\n\n"
                            "Any dungeon - Small Keys can only appear inside of any dungeon.\n\n"
                            "Overworld - Small Keys can only appear outside of dungeons.\n\n"
                            "Anywhere - Small Keys can appear anywhere in the world.",
                 .tab = TAB_DUNGEONS, .section = "Dungeon Items" },
        Setting{ .key = RSK_BOSS_KEYSANITY, .label = "Boss Key Shuffle", .ui = Ui::Combo,
                 .options = kDungeonItemDest, .defaultValue = 2 /* Own Dungeon */,
                 .tooltip = "Start with - You will start with Boss keys from all dungeons.\n\n"
                            "Vanilla - Boss Keys will appear in their vanilla locations.\n\n"
                            "Own dungeon - Boss Keys can only appear in their respective dungeon.\n\n"
                            "Any dungeon - Boss Keys can only appear inside of any dungeon.\n\n"
                            "Overworld - Boss Keys can only appear outside of dungeons.\n\n"
                            "Anywhere - Boss Keys can appear anywhere in the world.",
                 .tab = TAB_DUNGEONS, .section = "Dungeon Items" },
        Setting{ .key = RSK_SHUFFLE_DUNGEON_REWARDS, .label = "Shuffle Dungeon Rewards", .ui = Ui::Combo,
                 .options = { { "Vanilla", 0 }, { "End of Dungeons", 1 }, { "Overworld", 4 },
                              { "Anywhere", 5 } },
                 .defaultValue = 1 /* End of Dungeons */,
                 .tooltip = "Shuffles the location of Spiritual Stones and medallions.\n"
                            "Vanilla - Spiritual Stones and medallions will be given from their "
                            "respective boss.\n\n"
                            "End of dungeons - Spiritual Stones and medallions will be given as rewards "
                            "for beating major dungeons. Link will always start with one stone or "
                            "medallion.\n\n"
                            "Any dungeon - Spiritual Stones and medallions can be found inside any "
                            "dungeon.\n\n"
                            "Overworld - Spiritual Stones and medallions can only be found outside of "
                            "dungeons.\n\n"
                            "Anywhere - Spiritual Stones and medallions can appear anywhere.",
                 .tab = TAB_DUNGEONS, .section = "Dungeon Items", .greyNonDefaultOptions = true },
        Setting{ .key = RSK_GERUDO_KEYS, .label = "Gerudo Fortress Keys", .ui = Ui::Combo,
                 .options = { { "Vanilla", 0 }, { "Any Dungeon", 1 }, { "Overworld", 2 },
                              { "Anywhere", 3 } },
                 .defaultValue = 0,
                 .tooltip = "Vanilla - Thieves' Hideout Keys will appear in their vanilla locations.\n\n"
                            "Any dungeon - Thieves' Hideout Keys can only appear inside of any dungon.\n\n"
                            "Overworld - Thieves' Hideout Keys can only appear outside of dungeons.\n\n"
                            "Anywhere - Thieves' Hideout Keys can appear anywhere in the world.",
                 .tab = TAB_DUNGEONS, .section = "Dungeon Items" },

        // -- Key Rings --
        Setting{ .key = RSK_KEYRINGS, .label = "Key Rings", .ui = Ui::Combo,
                 .options = { { "Off", 0 }, { "Random", 1 }, { "Count", 2 } }, .defaultValue = 0,
                 .tooltip = "Keyrings will replace all small keys from a particular dungeon with a single "
                            "keyring that awards all keys for its associated dungeon.\n\n"
                            "Off - No dungeons will have their keys replaced with keyrings.\n\n"
                            "Random - A random amount of dungeons (0-8 or 9) will have their keys replaced "
                            "with keyrings.\n\n"
                            "Count - A specified amount of randomly selected dungeons will have their keys "
                            "replaced with keyrings.\n\n"
                            "Selection - Hand select which dungeons will have their keys replaced with "
                            "keyrings\n(can also be left as random, in which case each one will have a 50% "
                            "chance of being a keyring).\n\n"
                            "Selecting key ring for dungeons will have no effect if Small Keys are set to "
                            "Start With or Vanilla.\n\n"
                            "If Gerudo Fortress Carpenters is set to Normal, and Gerudo Fortress Keys is "
                            "set to anything other than Vanilla, then the maximum amount of Key Rings that "
                            "can be selected by Random or Count will be 9. Otherwise, the maximum amount "
                            "of Key Rings will be 8.",
                 .tab = TAB_DUNGEONS, .section = "Key Rings" },

        // ============================================================== Shuffles
        // -- Shuffle Items --
        Setting{ .key = RSK_SHUFFLE_SONGS, .label = "Shuffle Songs", .ui = Ui::Combo,
                 .options = { { "Off", 0 }, { "Song Locations", 1 }, { "Anywhere", 3 } },
                 .defaultValue = 1 /* Song Locations */,
                 .tooltip = "Off - Songs will appear at their vanilla locations.\n\n"
                            "Song locations - Songs will only appear at locations that normally teach "
                            "songs.\n\n"
                            "Dungeon rewards - Songs appear after beating a major dungeon boss.\n"
                            "The 4 remaining songs are located at:\n"
                            "  - Zelda's Lullaby location\n"
                            "  - Ice Cavern's Serenade of Water location\n"
                            "  - Bottom of the Well Lens of Truth location\n"
                            "  - Gerudo Training Ground's Ice Arrows location\n\n"
                            "Anywhere - Songs can appear at any location.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },
        Setting{ .key = RSK_SHUFFLE_TOKENS, .label = "Token Shuffle", .ui = Ui::Combo,
                 .options = { { "Off", 0 }, { "Dungeons", 1 }, { "Overworld", 2 }, { "All Tokens", 3 } },
                 .defaultValue = 0,
                 .tooltip = "Shuffles Golden Skulltula Tokens into the item pool. This means Golden "
                            "Skulltulas can contain other items as well.\n\n"
                            "Off - GS tokens will not be shuffled.\n\n"
                            "Dungeons - Only shuffle GS tokens that are within dungeons.\n\n"
                            "Overworld - Only shuffle GS tokens that are outside of dungeons.\n\n"
                            "All Tokens - Shuffle all 100 GS tokens.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },
        Setting{ .key = RSK_SHUFFLE_KOKIRI_SWORD, .label = "Shuffle Kokiri Sword", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Shuffles the Kokiri Sword into the item pool.\n\n"
                            "This will require the use of sticks until the Kokiri Sword is found.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },
        Setting{ .key = RSK_SHUFFLE_MASTER_SWORD, .label = "Shuffle Master Sword", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Shuffles the Master Sword into the item pool.\n\n"
                            "Adult Link will start with a second free item instead of the Master Sword.\n"
                            "If you haven't found the Master Sword before facing Ganon, you won't receive "
                            "it during the fight.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },
        Setting{ .key = RSK_SHUFFLE_OCARINA, .label = "Shuffle Ocarinas", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Enabling this shuffles the Fairy Ocarina and the Ocarina of Time into the "
                            "item pool.\n\n"
                            "This will require finding an Ocarina before being able to play songs.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },
        Setting{ .key = RSK_SHUFFLE_WEIRD_EGG, .label = "Shuffle Weird Egg", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Shuffles the Weird Egg from Malon in to the item pool. Enabling \"Skip Child "
                            "Zelda\" disables this feature.\n\n"
                            "The Weird Egg is required to unlock several events:\n"
                            "  - Zelda's Lullaby from Impa\n"
                            "  - Saria's Song in Sacred Forest Meadow\n"
                            "  - Epona's Song and chicken minigame at Lon Lon Ranch\n"
                            "  - Zelda's Letter for Kakariko gate (if set to closed)\n"
                            "  - Happy Mask Shop sidequest",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },
        Setting{ .key = RSK_SHUFFLE_GERUDO_MEMBERSHIP_CARD, .label = "Shuffle Gerudo Membership Card",
                 .ui = Ui::Checkbox, .defaultValue = 0,
                 .tooltip = "Shuffles the Gerudo Membership Card into the item pool.\n\n"
                            "The Gerudo Card is required to enter the Gerudo Training Ground, opening the "
                            "gate to Haunted Wasteland and the Horseback Archery minigame.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },
        Setting{ .key = RSK_SHUFFLE_FROG_SONG_RUPEES, .label = "Shuffle Frog Song Rupees",
                 .ui = Ui::Checkbox, .defaultValue = 0,
                 .tooltip = "Shuffles 5 Purple Rupees into to the item pool, and allows\n"
                            "you to earn items by playing songs at the Frog Choir.\n\n"
                            "This setting does not effect the item earned from playing\n"
                            "the Song of Storms and the frog song minigame.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },
        Setting{ .key = RSK_SHUFFLE_ADULT_TRADE, .label = "Shuffle Adult Trade", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Adds all of the adult trade quest items into the pool, each of which can be "
                            "traded for a unique reward.\n\n"
                            "You will be able to choose which of your owned adult trade items is visible "
                            "in the inventory by selecting the item with A and using the control stick or "
                            "D-pad.\n\n"
                            "If disabled, only the Claim Check will be found in the pool.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Items" },

        // -- Shuffle Shops & Merchants (prices fixed Vanilla + hidden) --
        Setting{ .key = RSK_SHOPSANITY, .label = "Shop Shuffle", .ui = Ui::Combo,
                 .options = { { "Off", 0 }, { "Specific Count", 1 }, { "Random", 2 } }, .defaultValue = 0,
                 .tooltip = "Off - All shop items will be the same as vanilla.\n\n"
                            "Specific Count - Vanilla shop items will be shuffled among different shops, "
                            "and each shop will contain a specific number (0-7) of non-vanilla shop "
                            "items.\n\n"
                            "Random - Vanilla shop items will be shuffled among different shops, and each "
                            "shop will contain a random number (1-7) of non-vanilla shop items.",
                 .tab = TAB_SHUFFLES, .section = "Shuffle Shops & Merchants" },
        Setting{ .key = RSK_SHOPSANITY_PRICES, .label = "Shopsanity Prices", .ui = Ui::Combo,
                 .defaultValue = 0 /* Vanilla */, .tab = TAB_SHUFFLES,
                 .section = "Shuffle Shops & Merchants", .hidden = true },

        // -- Additional Items --
        Setting{ .key = RSK_SHUFFLE_CHILD_WALLET, .label = "Shuffle Child's Wallet", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Enabling this shuffles the Child's Wallet into the item pool.\n\n"
                            "You will not be able to carry any rupees until you find a wallet.",
                 .tab = TAB_SHUFFLES, .section = "Additional Items" },
        Setting{ .key = RSK_INCLUDE_TYCOON_WALLET, .label = "Include Tycoon Wallet", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Enabling this adds an extra Progressive Wallet to the pool and adds a new 999 "
                            "capacity tier after Giant's Wallet.",
                 .tab = TAB_SHUFFLES, .section = "Additional Items" },
        Setting{ .key = RSK_ENABLE_BOMBCHU_DROPS, .label = "Bombchu Drops", .ui = Ui::Checkbox,
                 .defaultValue = 1 /* On */,
                 .tooltip = "Once you obtain a Bombchu Bag, refills will sometimes replace Bomb drops that "
                            "would spawn.\n"
                            "If you have Bombchu Bag disabled, you will need a Bomb Bag and existing "
                            "Bombchus for Bombchus to drop.",
                 .tab = TAB_SHUFFLES, .section = "Additional Items" },

        // ============================================================ Hints/Traps
        // -- Hints --
        Setting{ .key = RSK_GOSSIP_STONE_HINTS, .label = "Gossip Stone Hints", .ui = Ui::Combo,
                 .options = { { "No Hints", 0 }, { "Need Nothing", 1 }, { "Mask of Truth", 2 },
                              { "Stone of Agony", 3 } },
                 .defaultValue = 1 /* Need Nothing */,
                 .tooltip = "Allows Gossip Stones to provide hints on item locations. Hints mentioning "
                            "\"Way of the Hero\" indicate a location that holds an item required to beat "
                            "the seed.\n\n"
                            "No hints - No hints will be given at all.\n\n"
                            "Need Nothing - Hints are always available from Gossip Stones.\n\n"
                            "Need Stone of Agony - Hints are only available after obtaining the Stone of "
                            "Agony.\n\n"
                            "Need Mask of Truth - Hints are only available whilst wearing the Mask of Truth.",
                 .tab = TAB_HINTS, .section = "Hints" },
        Setting{ .key = RSK_HINT_CLARITY, .label = "Hint Clarity", .ui = Ui::Combo,
                 .options = { { "Obscure", 0 }, { "Ambiguous", 1 }, { "Clear", 2 } },
                 .defaultValue = 2 /* Clear */,
                 .tooltip = "Sets the difficulty of hints.\n\n"
                            "Obscure - Hints are unique for each item, but the writing may be cryptic.\n"
                            "Ex: Kokiri Sword > a butter knife\n\n"
                            "Ambiguous - Hints are clearly written, but may refer to more than one item.\n"
                            "Ex: Kokiri Sword > a sword\n\n"
                            "Clear - Hints are clearly written and are unique for each item.\n"
                            "Ex: Kokiri Sword > the Kokiri Sword",
                 .tab = TAB_HINTS, .section = "Hints" },
        Setting{ .key = RSK_HINT_DISTRIBUTION, .label = "Hint Distribution", .ui = Ui::Combo,
                 .options = { { "Useless", 0 }, { "Balanced", 1 }, { "Strong", 2 }, { "Very Strong", 3 } },
                 .defaultValue = 1 /* Balanced */,
                 .tooltip = "Sets how many hints will be useful.\n\n"
                            "Useless - Only junk hints.\n\n"
                            "Balanced - Recommended hint spread.\n\n"
                            "Strong - More useful hints.\n\n"
                            "Very Strong - Many powerful hints.",
                 .tab = TAB_HINTS, .section = "Hints" },

        // -- Traps --
        Setting{ .key = RSK_BASE_ICE_TRAPS, .label = "Base Ice Traps", .ui = Ui::Checkbox,
                 .defaultValue = 1 /* On */,
                 .tooltip = "Sets if ice traps that exist in vanilla are shuffled into the item pool.\n"
                            "If this is on, 1 Trap will always be added to the pool,\n"
                            "an additional trap will be added if Gerudo Training Grounds\n"
                            "is NOT master quest,\n"
                            "and 4 more will be added if Ganon's Castle is NOT Master Quest.",
                 .tab = TAB_HINTS, .section = "Traps" },
        Setting{ .key = RSK_ADDITIONAL_ICE_TRAPS, .label = "Additional Ice Traps", .ui = Ui::Slider,
                 .sliderMin = 0, .sliderMax = 100, .defaultValue = 0,
                 .tooltip = "Sets how many more Ice Traps will be added to item pool,\n"
                            "assuming there is enough space after placing Progression Items.\n\n"
                            "You do not need to have base ice traps on for this setting to work.",
                 .tab = TAB_HINTS, .section = "Traps" },
        Setting{ .key = RSK_ICE_TRAP_PERCENT, .label = "Ice Trap Percent", .ui = Ui::Slider,
                 .sliderMin = 0, .sliderMax = 100, .defaultValue = 0,
                 .tooltip = "If set above 0, each Junk item has that chance of being replaced with an "
                            "extra Ice Trap.",
                 .tab = TAB_HINTS, .section = "Traps" },

        // -- Static Hints (all checkboxes) --
        Setting{ .key = RSK_TOT_ALTAR_HINT, .label = "ToT Altar Hint", .ui = Ui::Checkbox,
                 .defaultValue = 1,
                 .tooltip = "Reading the Temple of Time altar as child will tell you the locations of the "
                            "Spiritual Stones.\n"
                            "Reading the Temple of Time altar as adult will tell you the locations of the "
                            "medallions, as well as the conditions for building the Rainbow Bridge and "
                            "getting the Boss Key for Ganon's Castle.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_GANONDORF_HINT, .label = "Ganondorf Hint", .ui = Ui::Checkbox,
                 .defaultValue = 1,
                 .tooltip = "Talking to Ganondorf in his boss room will tell you the location of the Light "
                            "Arrows and Master Sword.If this option is enabled and Ganondorf is reachable "
                            "without these items, Gossip Stones will never hint the appropriate items.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_SHEIK_LA_HINT, .label = "Sheik Light Arrow Hint", .ui = Ui::Checkbox,
                 .defaultValue = 1,
                 .tooltip = "Talking to Sheik inside Ganon's Castle will tell you the location of the "
                            "Light Arrows.If this option is enabled and Sheik is reachable without Light "
                            "Arrows, Gossip Stones will never hint the Light Arrows.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_BOSS_KEY_HINT, .label = "Boss Door Hints", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Navi will tell where boss key can be found when prompted at boss door.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_DAMPES_DIARY_HINT, .label = "Dampe's Diary Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Reading the diary of Dampé the gravekeeper as adult will tell you the "
                            "location of one of the Hookshots.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_GREG_HINT, .label = "Greg the Green Rupee Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to the chest game owner after buying a key will tell you the location "
                            "of Greg the Green Rupee.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_SARIA_HINT, .label = "Saria's Hint", .ui = Ui::Checkbox, .defaultValue = 0,
                 .tooltip = "Talking to Saria either in person or through Saria's Song will tell you the "
                            "location of a progressive magic meter.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_MIDO_HINT, .label = "Mido's Hint", .ui = Ui::Checkbox, .defaultValue = 0,
                 .tooltip = "Talking to Mido as child will tell you the location of the Kokiri Sword.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_FROGS_HINT, .label = "Frog Ocarina Game Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Standing near the pedestal for the frogs in Zora's River will tell you the "
                            "reward for the frogs' Ocarina game.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_OOT_HINT, .label = "Ocarina of Time Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Sheik in the Temple of Time will tell you the item and song on the Ocarina "
                            "of Time.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_BIGGORON_HINT, .label = "Biggoron's Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to Biggoron will tell you the item he will give you in exchange for "
                            "the Claim Check.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_BIG_POES_HINT, .label = "Big Poes Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to the Poe Collector in the Market Guardhouse while adult will tell "
                            "you what you receive for handing in Big Poes.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_CHICKENS_HINT, .label = "Chickens Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to Anju as a child will tell you the item she will give you for "
                            "delivering her cuccos to the pen.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_HBA_HINT, .label = "Horseback Archery Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to the Horseback Archery gerudo in Gerudo Fortress, or the nearby "
                            "sign, will tell you what you win for scoring 1000 and 1500 points on "
                            "Horseback Archery.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_WARP_SONG_HINTS, .label = "Warp Song Hints", .ui = Ui::Checkbox,
                 .defaultValue = 1,
                 .tooltip = "Playing a warp song will tell you where it leads. (If warp song destinations "
                            "are vanilla, this is always enabled.)",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_SCRUB_TEXT_HINT, .label = "Scrub Hint Text", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Business scrubs will reveal the identity of what they're selling.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_KAK_10_SKULLS_HINT, .label = "10 GS Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to the Cursed Resident in the Skulltula House who is saved after 10 "
                            "tokens will tell you the reward.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_KAK_20_SKULLS_HINT, .label = "20 GS Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to the Cursed Resident in the Skulltula House who is saved after 20 "
                            "tokens will tell you the reward.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_KAK_30_SKULLS_HINT, .label = "30 GS Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to the Cursed Resident in the Skulltula House who is saved after 30 "
                            "tokens will tell you the reward.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_KAK_40_SKULLS_HINT, .label = "40 GS Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to the Cursed Resident in the Skulltula House who is saved after 40 "
                            "tokens will tell you the reward.",
                 .tab = TAB_HINTS, .section = "Static Hints" },
        Setting{ .key = RSK_KAK_50_SKULLS_HINT, .label = "50 GS Hint", .ui = Ui::Checkbox,
                 .defaultValue = 0,
                 .tooltip = "Talking to the Cursed Resident in the Skulltula House who is saved after 50 "
                            "tokens will tell you the reward.",
                 .tab = TAB_HINTS, .section = "Static Hints" },

        // ========================================================= Starting Items
        // (Every kept Starting Items setting has no base-game tooltip.)
        // -- Equips --
        Setting{ .key = RSK_STARTING_KOKIRI_SWORD, .label = "Start with Kokiri Sword",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Equips" },
        Setting{ .key = RSK_STARTING_MASTER_SWORD, .label = "Start with Master Sword",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Equips" },
        Setting{ .key = RSK_STARTING_DEKU_SHIELD, .label = "Start with Deku Shield",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Equips" },

        // -- Items --
        Setting{ .key = RSK_STARTING_OCARINA, .label = "Start with Ocarina", .ui = Ui::Combo,
                 .options = { { "Off", 0 }, { "Fairy Ocarina", 1 }, { "Ocarina of Time", 2 } },
                 .defaultValue = 0, .tab = TAB_START, .section = "Items" },
        Setting{ .key = RSK_STARTING_STICKS, .label = "Start with Stick Ammo", .ui = Ui::Checkbox,
                 .defaultValue = 0, .tab = TAB_START, .section = "Items" },
        Setting{ .key = RSK_STARTING_NUTS, .label = "Start with Nut Ammo", .ui = Ui::Checkbox,
                 .defaultValue = 0, .tab = TAB_START, .section = "Items" },
        Setting{ .key = RSK_STARTING_BEANS, .label = "Start with Magic Beans", .ui = Ui::Checkbox,
                 .defaultValue = 0, .tab = TAB_START, .section = "Items" },
        Setting{ .key = RSK_STARTING_SKULLTULA_TOKEN, .label = "Gold Skulltula Tokens", .ui = Ui::Slider,
                 .sliderMin = 0, .sliderMax = 100, .defaultValue = 0, .tab = TAB_START,
                 .section = "Items" },
        // Stored value is the option index over the "1".."20" label list, so the
        // default index 2 displays as 3 hearts (OoT vanilla).
        Setting{ .key = RSK_STARTING_HEARTS, .label = "Starting Hearts", .ui = Ui::Slider,
                 .sliderMin = 1, .sliderMax = 20, .defaultValue = 2 /* displays 3 */, .tab = TAB_START,
                 .section = "Items" },
        // Link's Pocket is removed from the UI; the reward is fixed to "Any Reward",
        // with the pocket itself left at "Dungeon Reward" so a reward is granted.
        Setting{ .key = RSK_LINKS_POCKET, .label = "Link's Pocket", .ui = Ui::Combo,
                 .defaultValue = 0 /* Dungeon Reward */, .tab = TAB_START, .section = "Items",
                 .hidden = true },
        Setting{ .key = RSK_LINKS_POCKET_REWARD, .label = "Link's Pocket Reward Type", .ui = Ui::Combo,
                 .defaultValue = 0 /* Any Reward */, .tab = TAB_START, .section = "Items",
                 .hidden = true },

        // -- Normal Songs --
        Setting{ .key = RSK_STARTING_ZELDAS_LULLABY, .label = "Start with Zelda's Lullaby",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Normal Songs" },
        Setting{ .key = RSK_STARTING_EPONAS_SONG, .label = "Start with Epona's Song",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Normal Songs" },
        Setting{ .key = RSK_STARTING_SARIAS_SONG, .label = "Start with Saria's Song",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Normal Songs" },
        Setting{ .key = RSK_STARTING_SUNS_SONG, .label = "Start with Sun's Song",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Normal Songs" },
        Setting{ .key = RSK_STARTING_SONG_OF_TIME, .label = "Start with Song of Time",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Normal Songs" },
        Setting{ .key = RSK_STARTING_SONG_OF_STORMS, .label = "Start with Song of Storms",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Normal Songs" },

        // -- Warp Songs --
        Setting{ .key = RSK_STARTING_MINUET_OF_FOREST, .label = "Start with Minuet of Forest",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Warp Songs" },
        Setting{ .key = RSK_STARTING_BOLERO_OF_FIRE, .label = "Start with Bolero of Fire",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Warp Songs" },
        Setting{ .key = RSK_STARTING_SERENADE_OF_WATER, .label = "Start with Serenade of Water",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Warp Songs" },
        Setting{ .key = RSK_STARTING_REQUIEM_OF_SPIRIT, .label = "Start with Requiem of Spirit",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Warp Songs" },
        Setting{ .key = RSK_STARTING_NOCTURNE_OF_SHADOW, .label = "Start with Nocturne of Shadow",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Warp Songs" },
        Setting{ .key = RSK_STARTING_PRELUDE_OF_LIGHT, .label = "Start with Prelude of Light",
                 .ui = Ui::Checkbox, .defaultValue = 0, .tab = TAB_START, .section = "Warp Songs" },
    };
    return table;
}

// RandomizerSettingKey -> enum NAME, generated from the same X-macro header that
// defines the enum (so the two can never drift).
const char* RSKName(RandomizerSettingKey key) {
    switch (key) {
#define RANDO_ENUM_BEGIN(EnumName)
#define RANDO_ENUM_ITEM(name, ...) case name: return #name;
#define RANDO_ENUM_END(EnumName)
#include "rando/enums/RandomizerSettingKey.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
    }
    return "RSK_NONE";
}

} // namespace ms
