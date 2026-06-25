# MultiShip Randomizer Settings (curated)

Reference documentation for the curated randomizer settings exposed by MultiShip.

This is a **reference-only** document — it describes which settings MultiShip exposes,
their option labels, defaults, and the verbatim base-game tooltip for each. It does **not**
describe any UI or code.

## Source of truth

All option definitions (display name, option labels, label order, base default) come from
[`settings.cpp`](../../ShipwreckCombo/soh/soh/Enhancements/randomizer/settings.cpp), and all
verbatim tooltips come from
[`option_descriptions.cpp`](../../ShipwreckCombo/soh/soh/Enhancements/randomizer/option_descriptions.cpp),
both in the sibling **ShipwreckCombo** repo (the canonical SoH source).
Tab membership for Tabs 4 & 5 comes from the `OptionGroup` definitions in `settings.cpp`
(`RSG_MENU_SIDEBAR_HINTS_TRAPS`, `RSG_MENU_SIDEBAR_STARTING_ITEMS`).

Captured **2026-06-25**.

## Conventions

- **RSK key** — the base SoH `RandomizerSettingKey` enum name.
- **Options** — listed in base label order; the zero-based index is shown in parentheses,
  e.g. `Vanilla (0)`. These are the exact strings from `settings.cpp`.
- **Base default** — the default defined in base SoH `settings.cpp`.
- **MultiShip default** — the curated default for MultiShip (the value specified in the
  curation spec). For Tabs 1–3 this is given explicitly; for Tabs 4–5 MultiShip keeps the
  base default unless noted.
- **Tooltip** — reproduced verbatim from base SoH `option_descriptions.cpp`, including
  original typos. Settings with no tooltip string in the base game are marked
  *(no base-game tooltip)*.
- ⚠️ **EXCLUDED / HIDDEN / FIXED** callouts mark where MultiShip differs from base SoH
  (subset of options, hidden+fixed value, or a default that overrides the base default).

> **Two MultiShip defaults override the base-SoH default** (everything else matches base or
> is a pure option-subset): **Skip Child Stealth** (MultiShip = *Checked*, base = *Unchecked*)
> and **Ganon's Trials** (MultiShip = *Random Number*, base = *Set Number*).

---

# Tab 1 — Logic / Access

## 1.1 Logic

### Item Pool — `RSK_ITEM_POOL`
- Options: `Plentiful (0)`, `Balanced (1)`, `Scarce (2)`, `Minimal (3)`
- Base default: **Balanced** (`RO_ITEM_POOL_BALANCED`)
- MultiShip default: **Balanced**
- ⚠️ **HIDDEN / FIXED** — not shown in the MultiShip UI; the generator always behaves as Balanced.
- Tooltip:
  > Sets how many major items appear in the item pool.
  >
  > Plentiful - Extra major items are added to the pool.
  >
  > Balanced - Original item pool.
  >
  > Scarce - Some excess items are removed, including health upgrades.
  >
  > Minimal - Most excess items are removed.

### Logic — `RSK_LOGIC_RULES`
- Options: `Glitchless (0)`, `No Logic (1)`
- Base default: **Glitchless** (`RO_LOGIC_GLITCHLESS`)
- MultiShip default: **Glitchless**
- ⚠️ **HIDDEN / FIXED** — fixed to *Glitchless* combined with *All Locations Reachable* (see below).
- Tooltip:
  > Glitchless - No glitches are required, but may require some minor tricks. Additional tricks may be enabled and disabled below.
  >
  > No logic - Item placement is completely random. MAY BE IMPOSSIBLE TO BEAT.

### All Locations Reachable — `RSK_ALL_LOCATIONS_REACHABLE`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **On** (`RO_GENERIC_ON`)
- MultiShip default: **On**
- ⚠️ **HIDDEN / FIXED** — part of the fixed "Glitchless + All Locations Reachable" logic combo.
- Tooltip:
  > When this option is enabled, the randomizer will guarantee that every item is obtainable and every location is reachable. When disabled, only required items and locations to beat the game will be guaranteed reachable.

### Starting Age — `RSK_STARTING_AGE`
- Options: `Child (0)`, `Adult (1)`, `Random (2)`
- Base default: **Child** (`RO_AGE_CHILD`)
- MultiShip default: **Child**
- MultiShip note: *Adult* ⇒ start with the Master Sword if it is not shuffled, else a random item.
  (See `RSK_SHUFFLE_MASTER_SWORD`: "Adult Link will start with a second free item instead of the Master Sword.")
- Tooltip:
  > Choose which age Link will start as.
  >
  > Starting as adult means you start with the Master Sword in your inventory.
  > The child option is forcefully set if it would conflict with other options.

### Full Wallets — `RSK_FULL_WALLETS`
- Options: `No (0)`, `Yes (1)` (checkbox)
- Base default: **No / Unchecked** (`RO_GENERIC_OFF`)
- MultiShip default: **Unchecked**
- Note: in base SoH this control lives on the Starting Items tab; MultiShip surfaces it here on the Logic tab.
- Tooltip:
  > Start with a full wallet. All wallet upgrades come filled with rupees.

### Skip Child Zelda — `RSK_SKIP_CHILD_ZELDA`
- Options: `Don't Skip (0)`, `Skip (1)` (checkbox)
- Base default: **Don't Skip / Unchecked** (`RO_GENERIC_DONT_SKIP`)
- MultiShip default: **Unchecked**
- MultiShip note: if skipped, start with Zelda's Letter + 1 extra random item from the Impa song location;
  Weird Egg is excluded from the pool; Malon no longer waits at the castle so that location holds no item.
- Tooltip:
  > Start with Zelda's Letter and the item Impa would normally give you and skip the sequence up until after meeting Zelda. Disables the ability to shuffle Weird Egg.

### Mask Quest — `RSK_MASK_QUEST`
- Options: `Vanilla (0)`, `Completed (1)`, `Shuffle (2)`
- Base default: **Vanilla** (`0`)
- MultiShip default: **Vanilla**
- Tooltip:
  > How masks are acquired.
  > Vanilla - Mask trade quest.
  >
  > Completed - Once the Happy Mask Shop is opened, all masks will be available to be borrowed.
  >
  > Shuffle - Happy Mask Shop never opens, masks are shuffled with rest of items.

### Skip Child Stealth — `RSK_SKIP_CHILD_STEALTH`
- Options: `Don't Skip (0)`, `Skip (1)` (checkbox)
- Base default: **Don't Skip / Unchecked** (`RO_GENERIC_DONT_SKIP`)
- MultiShip default: ⚠️ **Checked** — *MultiShip overrides the base default.*
- MultiShip note: skips the child section, jumps to Zelda after the crawlspace; irrelevant + greyed out when Skip Child Zelda is on.
- Tooltip:
  > The crawlspace into Hyrule Castle goes straight to Zelda, skipping the guards.

### Skip Epona Race — `RSK_SKIP_EPONA_RACE`
- Options: `Don't Skip (0)`, `Skip (1)` (checkbox)
- Base default: **Don't Skip / Unchecked** (`RO_GENERIC_DONT_SKIP`)
- MultiShip default: **Unchecked**
- Tooltip:
  > Epona can be summoned with Epona's Song without needing to race Ingo.

### Ganon's Boss Key — `RSK_GANONS_BOSS_KEY`
- Base options (full list): `Vanilla (0)`, `Own Dungeon (1)`, `Start With (2)`, `Any Dungeon (3)`,
  `Overworld (4)`, `Anywhere (5)`, `LACS-Vanilla (6)`, `LACS-Stones (7)`, `LACS-Medallions (8)`,
  `LACS-Rewards (9)`, `LACS-Dungeons (10)`, `LACS-Tokens (11)`, `100 GS Reward (12)`
- ⚠️ **EXCLUDED in MultiShip:** `LACS-Vanilla`, `LACS-Stones`, `LACS-Medallions`, `LACS-Rewards`,
  `LACS-Dungeons`, `LACS-Tokens`, `100 GS Reward`. MultiShip offers only
  `Vanilla / Own Dungeon / Start With / Any Dungeon / Overworld / Anywhere`.
- Base default: **Vanilla** (`RO_GANON_BOSS_KEY_VANILLA`)
- MultiShip default: **Vanilla**
- Tooltip:
  > Vanilla - Ganon's Boss Key will appear in the vanilla location.
  >
  > Own dungeon - Ganon's Boss Key can appear anywhere inside Ganon's Castle.
  >
  > Start with - Places Ganon's Boss Key in your starting inventory.
  > Any dungeon - Ganon's Boss Key Key can only appear inside of any dungeon.
  >
  > Overworld - Ganon's Boss Key Key can only appear outside of dungeons.
  >
  > Anywhere - Ganon's Boss Key Key can appear anywhere in the world.
  >
  > LACS - These settings put the boss key on the Light Arrow Cutscene location, from Zelda in Temple of Time as adult, with differing requirements:
  > - Vanilla: Obtain the Shadow Medallion and Spirit Medallion
  > - Stones: Obtain the specified amount of Spiritual Stones.
  > - Medallions: Obtain the specified amount of medallions.
  > - Dungeon rewards: Obtain the specified total sum of Spiritual Stones or medallions.
  > - Dungeons: Complete the specified amount of dungeons. Dungeons are considered complete after stepping in to the blue warp after the boss.
  > - Tokens: Obtain the specified amount of Skulltula tokens.
  >
  > 100 GS Reward - Ganon's Boss Key will be awarded by the cursed rich man after you collect 100 Gold Skulltula Tokens.

## 1.2 Area Access

### Closed Forest — `RSK_FOREST`
- Options: `On (0)`, `Deku Only (1)`, `Off (2)`
- Base default: **On** (`RO_CLOSED_FOREST_ON`)
- MultiShip default: **On**
- Tooltip:
  > Determines if Kokiri forest can be left for the Lost Woods bridge or the Deku Tree.
  >
  > On - Kokiri Sword & Deku Shield are required to access the Deku Tree, and completing the Deku Tree is required to access the Lost Woods Bridge Exit.
  >
  > Deku Only - Kokiri boy no longer blocks the path to the Bridge but Mido still requires the Kokiri Sword and Deku Shield to access the tree.
  >
  > Off - Mido no longer blocks the path to the Deku Tree. Kokiri boy no longer blocks the path out of the forest.

### Kakariko Gate — `RSK_KAK_GATE`
- Options: `Closed (0)`, `Open (1)`
- Base default: **Closed** (`0`)
- MultiShip default: **Closed**
- Tooltip:
  > Closed - The gate will remain closed until Zelda's Letter is shown to the guard.
  >
  > Open - The gate is always open. The Happy Mask Shop will open immediately after obtaining Zelda's Letter.

### Door of Time — `RSK_DOOR_OF_TIME`
- Options: `Closed (0)`, `Song only (1)`, `Open (2)`
- Base default: **Closed** (`0`)
- MultiShip default: **Closed**
- Tooltip:
  > Closed - The Ocarina of Time, the Song of Time and all three Spiritual Stones are required to open the Door of Time.
  >
  > Song only - Play the Song of Time in front of the Door of Time to open it.
  >
  > Open - The Door of Time is permanently open with no requirements.

### Zora's Fountain — `RSK_ZORAS_FOUNTAIN`
- Options: `Closed (0)`, `Closed as child (1)`, `Open (2)`
- Base default: **Closed** (`0`)
- MultiShip default: **Closed**
- Tooltip:
  > Closed - King Zora obstructs the way to Zora's Fountain. Ruto's Letter must be shown as child Link in order to move him in both time periods.
  >
  > Closed as child - Ruto's Letter is only required to move King Zora as child Link. Zora's Fountain starts open as adult.
  >
  > Open - King Zora has already mweeped out of the way in both time periods. Ruto's Letter is removed from the item pool.

### Sleeping Waterfall — `RSK_SLEEPING_WATERFALL`
- Options: `Closed (0)`, `Open (1)`
- Base default: **Closed** (`0`)
- MultiShip default: **Closed**
- Tooltip:
  > Closed - Sleeping Waterfall obstructs the entrance to Zora's Domain. Zelda's Lullaby must be played in order to open it (but only once; then it stays open in both time periods).
  >
  > Open - Sleeping Waterfall is always open. Link may always enter Zora's Domain.

### Jabu-Jabu — `RSK_JABU_OPEN`
- Options: `Closed (0)`, `Open (1)`
- Base default: **Closed** (`0`)
- MultiShip default: **Closed**
- Tooltip:
  > Closed - A fish is required to open Jabu-Jabu's mouth.
  >
  > Open - Jabu-Jabu's mouth opens without the need for a fish.

### Fortress Carpenters — `RSK_GERUDO_FORTRESS`
- Base options: `Normal (0)`, `Fast (1)`, `Free (2)`
- ⚠️ **EXCLUDED in MultiShip:** `Free`. MultiShip offers only `Normal / Fast`.
- Base default: **Normal** (`0`)
- MultiShip default: **Normal**
- Tooltip:
  > Sets the state of the carpenters captured by Gerudo in Gerudo Fortress, and with it the number of guards that spawn.
  >
  > Normal - All 4 carpenters are required to be saved.
  >
  > Fast - Only the bottom left carpenter requires rescuing.
  >
  > Free - Bridge is repaired from start, and Nabooru cannot spawn.
  > If the Gerudo Membership Card isn't shuffled, you start with it.
  >
  > Only "Normal" is compatible with Gerudo Fortress Key Rings.

### Rainbow Bridge — `RSK_RAINBOW_BRIDGE`
- Base options: `Vanilla (0)`, `Always open (1)`, `Stones (2)`, `Medallions (3)`,
  `Dungeon rewards (4)`, `Dungeons (5)`, `Tokens (6)`, `Greg (7)`
- ⚠️ **EXCLUDED in MultiShip:** `Dungeon rewards`, `Dungeons`, `Tokens`, `Greg`.
  MultiShip offers only `Vanilla / Always Open / Stones / Medallions`.
- Base default: **Vanilla** (`RO_BRIDGE_VANILLA`)
- MultiShip default: **Vanilla**
- Tooltip:
  > Alters the requirements to open the bridge to Ganon's Castle.
  >
  > Vanilla - Obtain the Shadow Medallion, Spirit Medallion and Light Arrows.
  >
  > Always open - No requirements.
  >
  > Stones - Obtain the specified amount of Spiritual Stones.
  >
  > Medallions - Obtain the specified amount of medallions.
  >
  > Dungeon rewards - Obtain the specified total sum of Spiritual Stones or medallions.
  >
  > Dungeons - Complete the specified amount of dungeons. Dungeons are considered complete after stepping in to the blue warp after the boss.
  >
  > Tokens - Obtain the specified amount of Skulltula tokens.
  >
  > Greg - Find Greg the Green Rupee.

### Ganon's Trials — `RSK_GANONS_TRIALS`
- Options: `Skip (0)`, `Set Number (1)`, `Random Number (2)`
- Base default: **Set Number** (`RO_GANONS_TRIALS_SET_NUMBER`)
- MultiShip default: ⚠️ **Random Number** — *MultiShip overrides the base default.*
- Tooltip:
  > Sets the number of Ganon's Trials required to dispel the barrier.
  >
  > Skip - No Trials are required and the barrier is already dispelled.
  >
  > Set Number - Select a number of trials that will be required from the slider below. Which specific trials you need to complete will be random.
  >
  > Random Number - A random number and set of trials will be required.

> Companion slider **Ganon's Trials Count** — `RSK_TRIAL_COUNT` — options `0`–`6` (slider),
> base default `6`. Tooltip: "Set the number of trials required to enter Ganon's Tower."
> (Only relevant when Ganon's Trials = *Set Number*.)

## 1.3 Entrances

⚠️ **OMITTED entirely.** MultiShip ships no entrance randomizer; none of the
`RSK_SHUFFLE_*_ENTRANCES` / `RSK_DECOUPLED_ENTRANCES` / `RSK_MIXED_ENTRANCE_POOLS` settings
are exposed.

---

# Tab 2 — Dungeons

## 2.1 Dungeon Items

### Maps & Compasses — `RSK_SHUFFLE_MAPANDCOMPASS`
- Options: `Start With (0)`, `Vanilla (1)`, `Own Dungeon (2)`, `Any Dungeon (3)`, `Overworld (4)`, `Anywhere (5)`
- Base default: **Own Dungeon** (`RO_DUNGEON_ITEM_LOC_OWN_DUNGEON`)
- MultiShip default: **Own Dungeon**
- Tooltip:
  > Start with - You will start with Maps & Compasses from all dungeons.
  >
  > Vanilla - Maps & Compasses will appear in their vanilla locations.
  >
  > Own dungeon - Maps & Compasses can only appear in their respective dungeon.
  >
  > Any dungeon - Maps & Compasses can only appear inside of any dungeon.
  >
  > Overworld - Maps & Compasses can only appear outside of dungeons.
  >
  > Anywhere - Maps & Compasses can appear anywhere in the world.

### Small Key Shuffle — `RSK_KEYSANITY`
- Options: `Start With (0)`, `Vanilla (1)`, `Own Dungeon (2)`, `Any Dungeon (3)`, `Overworld (4)`, `Anywhere (5)`
- Base default: **Own Dungeon** (`RO_DUNGEON_ITEM_LOC_OWN_DUNGEON`)
- MultiShip default: **Own Dungeon**
- Tooltip:
  > Start with - You will start with all Small Keys from all dungeons.
  >
  > Vanilla - Small Keys will appear in their vanilla locations. You start with 3 keys in Spirit Temple MQ because the vanilla key layout is not beatable in logic.
  >
  > Own dungeon - Small Keys can only appear in their respective dungeon. If Fire Temple is not a Master Quest dungeon, the door to the Boss Key chest will be unlocked.
  >
  > Any dungeon - Small Keys can only appear inside of any dungeon.
  >
  > Overworld - Small Keys can only appear outside of dungeons.
  >
  > Anywhere - Small Keys can appear anywhere in the world.

### Boss Key Shuffle — `RSK_BOSS_KEYSANITY`
- Options: `Start With (0)`, `Vanilla (1)`, `Own Dungeon (2)`, `Any Dungeon (3)`, `Overworld (4)`, `Anywhere (5)`
- Base default: **Own Dungeon** (`RO_DUNGEON_ITEM_LOC_OWN_DUNGEON`)
- MultiShip default: **Own Dungeon**
- Tooltip:
  > Start with - You will start with Boss keys from all dungeons.
  >
  > Vanilla - Boss Keys will appear in their vanilla locations.
  >
  > Own dungeon - Boss Keys can only appear in their respective dungeon.
  >
  > Any dungeon - Boss Keys can only appear inside of any dungeon.
  >
  > Overworld - Boss Keys can only appear outside of dungeons.
  >
  > Anywhere - Boss Keys can appear anywhere in the world.

### Shuffle Dungeon Rewards — `RSK_SHUFFLE_DUNGEON_REWARDS`
- Base options: `Vanilla (0)`, `End of Dungeons (1)`, `Own Dungeon (2)`, `Any Dungeon (3)`, `Overworld (4)`, `Anywhere (5)`
- ⚠️ **EXCLUDED in MultiShip:** `Own Dungeon`, `Any Dungeon`. The intended MultiShip subset is
  `Vanilla / End of Dungeons / Overworld / Anywhere`, **but every option except the default is
  greyed out for now** — effectively fixed to *End of Dungeons*.
- Base default: **End of Dungeons** (`RO_DUNGEON_REWARDS_END_OF_DUNGEON`)
- MultiShip default: **End of Dungeons**
- Tooltip:
  > Shuffles the location of Spiritual Stones and medallions.
  > Vanilla - Spiritual Stones and medallions will be given from their respective boss.
  >
  > End of dungeons - Spiritual Stones and medallions will be given as rewards for beating major dungeons. Link will always start with one stone or medallion.
  >
  > Any dungeon - Spiritual Stones and medallions can be found inside any dungeon.
  >
  > Overworld - Spiritual Stones and medallions can only be found outside of dungeons.
  >
  > Anywhere - Spiritual Stones and medallions can appear anywhere.

### Gerudo Fortress Keys — `RSK_GERUDO_KEYS`
- Options: `Vanilla (0)`, `Any Dungeon (1)`, `Overworld (2)`, `Anywhere (3)`
- Base default: **Vanilla** (`RO_GERUDO_KEYS_VANILLA`)
- MultiShip default: **Vanilla**
- Tooltip:
  > Vanilla - Thieves' Hideout Keys will appear in their vanilla locations.
  >
  > Any dungeon - Thieves' Hideout Keys can only appear inside of any dungon.
  >
  > Overworld - Thieves' Hideout Keys can only appear outside of dungeons.
  >
  > Anywhere - Thieves' Hideout Keys can appear anywhere in the world.

## 2.2 Key Rings

### Key Rings — `RSK_KEYRINGS`
- Base options: `Off (0)`, `Random (1)`, `Count (2)`, `Selection (3)`
- ⚠️ **EXCLUDED in MultiShip:** `Selection`. MultiShip offers only `Off / Random / Count`.
- Base default: **Off** (`RO_KEYRINGS_OFF`)
- MultiShip default: **Off**
- Tooltip:
  > Keyrings will replace all small keys from a particular dungeon with a single keyring that awards all keys for its associated dungeon.
  >
  > Off - No dungeons will have their keys replaced with keyrings.
  >
  > Random - A random amount of dungeons (0-8 or 9) will have their keys replaced with keyrings.
  >
  > Count - A specified amount of randomly selected dungeons will have their keys replaced with keyrings.
  >
  > Selection - Hand select which dungeons will have their keys replaced with keyrings
  > (can also be left as random, in which case each one will have a 50% chance of being a keyring).
  >
  > Selecting key ring for dungeons will have no effect if Small Keys are set to Start With or Vanilla.
  >
  > If Gerudo Fortress Carpenters is set to Normal, and Gerudo Fortress Keys is set to anything other than Vanilla, then the maximum amount of Key Rings that can be selected by Random or Count will be 9. Otherwise, the maximum amount of Key Rings will be 8.

> Companion slider **Keyring Dungeon Count** — `RSK_KEYRINGS_RANDOM_COUNT` — options `0`–`9`
> (slider), base default `8`. No base tooltip. (Only relevant when Key Rings = *Count*.)

---

# Tab 3 — Shuffles

## 3.1 Shuffle Items

### Shuffle Songs — `RSK_SHUFFLE_SONGS`
- Base options: `Off (0)`, `Song Locations (1)`, `Dungeon Rewards (2)`, `Anywhere (3)`
- ⚠️ **EXCLUDED in MultiShip:** `Dungeon Rewards`. MultiShip offers only `Off / Song Locations / Anywhere`.
- Base default: **Song Locations** (`RO_SONG_SHUFFLE_SONG_LOCATIONS`)
- MultiShip default: **Song Locations**
- Tooltip:
  > Off - Songs will appear at their vanilla locations.
  >
  > Song locations - Songs will only appear at locations that normally teach songs.
  >
  > Dungeon rewards - Songs appear after beating a major dungeon boss.
  > The 4 remaining songs are located at:
  >   - Zelda's Lullaby location
  >   - Ice Cavern's Serenade of Water location
  >   - Bottom of the Well Lens of Truth location
  >   - Gerudo Training Ground's Ice Arrows location
  >
  > Anywhere - Songs can appear at any location.

### Token Shuffle — `RSK_SHUFFLE_TOKENS`
- Options: `Off (0)`, `Dungeons (1)`, `Overworld (2)`, `All Tokens (3)`
- Base default: **Off** (`RO_TOKENSANITY_OFF`)
- MultiShip default: **Off**
- Tooltip:
  > Shuffles Golden Skulltula Tokens into the item pool. This means Golden Skulltulas can contain other items as well.
  >
  > Off - GS tokens will not be shuffled.
  >
  > Dungeons - Only shuffle GS tokens that are within dungeons.
  >
  > Overworld - Only shuffle GS tokens that are outside of dungeons.
  >
  > All Tokens - Shuffle all 100 GS tokens.

### Shuffle Kokiri Sword — `RSK_SHUFFLE_KOKIRI_SWORD`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off**
- MultiShip default: **Off**
- Tooltip:
  > Shuffles the Kokiri Sword into the item pool.
  >
  > This will require the use of sticks until the Kokiri Sword is found.

### Shuffle Master Sword — `RSK_SHUFFLE_MASTER_SWORD`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off**
- MultiShip default: **Off**
- Tooltip:
  > Shuffles the Master Sword into the item pool.
  >
  > Adult Link will start with a second free item instead of the Master Sword.
  > If you haven't found the Master Sword before facing Ganon, you won't receive it during the fight.

### Shuffle Ocarinas — `RSK_SHUFFLE_OCARINA`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off**
- MultiShip default: **Off**
- Tooltip:
  > Enabling this shuffles the Fairy Ocarina and the Ocarina of Time into the item pool.
  >
  > This will require finding an Ocarina before being able to play songs.

### Shuffle Weird Egg — `RSK_SHUFFLE_WEIRD_EGG`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off**
- MultiShip default: **Off**
- Tooltip:
  > Shuffles the Weird Egg from Malon in to the item pool. Enabling "Skip Child Zelda" disables this feature.
  >
  > The Weird Egg is required to unlock several events:
  >   - Zelda's Lullaby from Impa
  >   - Saria's Song in Sacred Forest Meadow
  >   - Epona's Song and chicken minigame at Lon Lon Ranch
  >   - Zelda's Letter for Kakariko gate (if set to closed)
  >   - Happy Mask Shop sidequest

### Shuffle Gerudo Membership Card — `RSK_SHUFFLE_GERUDO_MEMBERSHIP_CARD`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off**
- MultiShip default: **Off**
- Tooltip:
  > Shuffles the Gerudo Membership Card into the item pool.
  >
  > The Gerudo Card is required to enter the Gerudo Training Ground, opening the gate to Haunted Wasteland and the Horseback Archery minigame.

### Shuffle Frog Song Rupees — `RSK_SHUFFLE_FROG_SONG_RUPEES`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off** ✅ *(confirmed)*
- MultiShip default: **Off**
- Tooltip:
  > Shuffles 5 Purple Rupees into to the item pool, and allows
  > you to earn items by playing songs at the Frog Choir.
  >
  > This setting does not effect the item earned from playing
  > the Song of Storms and the frog song minigame.

### Shuffle Adult Trade — `RSK_SHUFFLE_ADULT_TRADE`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off** ✅ *(confirmed)*
- MultiShip default: **Off**
- Tooltip:
  > Adds all of the adult trade quest items into the pool, each of which can be traded for a unique reward.
  >
  > You will be able to choose which of your owned adult trade items is visible in the inventory by selecting the item with A and using the control stick or D-pad.
  >
  > If disabled, only the Claim Check will be found in the pool.

## 3.2 Shuffle Shops & Merchants

### Shop Shuffle — `RSK_SHOPSANITY`
- Options: `Off (0)`, `Specific Count (1)`, `Random (2)`
- Base default: **Off** (`RO_SHOPSANITY_OFF`) ✅ *(confirmed)*
- MultiShip default: **Off**
- Note: shop **prices** are **not exposed** in MultiShip; the price control
  (`RSK_SHOPSANITY_PRICES`) is fixed to its base default **Vanilla** (`RO_PRICE_VANILLA`).
- Tooltip:
  > Off - All shop items will be the same as vanilla.
  >
  > Specific Count - Vanilla shop items will be shuffled among different shops, and each shop will contain a specific number (0-7) of non-vanilla shop items.
  >
  > Random - Vanilla shop items will be shuffled among different shops, and each shop will contain a random number (1-7) of non-vanilla shop items.

## 3.3 Additional Items

### Shuffle Child's Wallet — `RSK_SHUFFLE_CHILD_WALLET`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off**
- MultiShip default: **Off**
- Tooltip:
  > Enabling this shuffles the Child's Wallet into the item pool.
  >
  > You will not be able to carry any rupees until you find a wallet.

### Include Tycoon Wallet — `RSK_INCLUDE_TYCOON_WALLET`
- Options: `Off (0)`, `On (1)` (checkbox)
- Base default: **Off**
- MultiShip default: **Off**
- Tooltip:
  > Enabling this adds an extra Progressive Wallet to the pool and adds a new 999 capacity tier after Giant's Wallet.

### Bombchu Drops — `RSK_ENABLE_BOMBCHU_DROPS`
- Options: `No (0)`, `Yes (1)`
- Base default: **Yes / On** (`RO_AMMO_DROPS_ON`)
- MultiShip default: **On**
- Tooltip:
  > Once you obtain a Bombchu Bag, refills will sometimes replace Bomb drops that would spawn.
  > If you have Bombchu Bag disabled, you will need a Bomb Bag and existing Bombchus for Bombchus to drop.

---

# Tab 4 — Hints / Traps

MultiShip takes the entire base SoH **Hints/Traps** tab (`RSG_MENU_SIDEBAR_HINTS_TRAPS`),
which is composed of the **Hints**, **Traps**, and **Static Hints** sections, **except** the
six static hints omitted below. All defaults shown are the base SoH defaults (MultiShip keeps them).

⚠️ **OMITTED static hints (6):** `RSK_LOACH_HINT` (Hyrule Loach), `RSK_MALON_HINT` (Malon),
`RSK_FISHING_POLE_HINT` (Fishing Pole), `RSK_KAK_100_SKULLS_HINT` (100 GS),
`RSK_MASK_SHOP_HINT` (Mask Shop), `RSK_MERCHANT_TEXT_HINT` (Merchant). See the
*Omitted* subsection at the end of this tab for their tooltips.

## Hints (section)

### Gossip Stone Hints — `RSK_GOSSIP_STONE_HINTS`
- Options: `No Hints (0)`, `Need Nothing (1)`, `Mask of Truth (2)`, `Stone of Agony (3)`
- Default: **Need Nothing** (`RO_GOSSIP_STONES_NEED_NOTHING`)
- Tooltip:
  > Allows Gossip Stones to provide hints on item locations. Hints mentioning "Way of the Hero" indicate a location that holds an item required to beat the seed.
  >
  > No hints - No hints will be given at all.
  >
  > Need Nothing - Hints are always available from Gossip Stones.
  >
  > Need Stone of Agony - Hints are only available after obtaining the Stone of Agony.
  >
  > Need Mask of Truth - Hints are only available whilst wearing the Mask of Truth.

### Hint Clarity — `RSK_HINT_CLARITY`
- Options: `Obscure (0)`, `Ambiguous (1)`, `Clear (2)`
- Default: **Clear** (`RO_HINT_CLARITY_CLEAR`)
- Tooltip:
  > Sets the difficulty of hints.
  >
  > Obscure - Hints are unique for each item, but the writing may be cryptic.
  > Ex: Kokiri Sword > a butter knife
  >
  > Ambiguous - Hints are clearly written, but may refer to more than one item.
  > Ex: Kokiri Sword > a sword
  >
  > Clear - Hints are clearly written and are unique for each item.
  > Ex: Kokiri Sword > the Kokiri Sword

### Hint Distribution — `RSK_HINT_DISTRIBUTION`
- Options: `Useless (0)`, `Balanced (1)`, `Strong (2)`, `Very Strong (3)`
- Default: **Balanced** (`RO_HINT_DIST_BALANCED`)
- Tooltip:
  > Sets how many hints will be useful.
  >
  > Useless - Only junk hints.
  >
  > Balanced - Recommended hint spread.
  >
  > Strong - More useful hints.
  >
  > Very Strong - Many powerful hints.

## Traps (section)

### Base Ice Traps — `RSK_BASE_ICE_TRAPS`
- Options: `Off (0)`, `On (1)`
- Default: **On** (`RO_GENERIC_ON`)
- Tooltip:
  > Sets if ice traps that exist in vanilla are shuffled into the item pool.
  > If this is on, 1 Trap will always be added to the pool,
  > an additional trap will be added if Gerudo Training Grounds
  > is NOT master quest,
  > and 4 more will be added if Ganon's Castle is NOT Master Quest.

### Additional Ice Traps — `RSK_ADDITIONAL_ICE_TRAPS`
- Options: `0`–`100` (slider)
- Default: **0**
- Tooltip:
  > Sets how many more Ice Traps will be added to item pool,
  > assuming there is enough space after placing Progression Items.
  >
  > You do not need to have base ice traps on for this setting to work.

### Ice Trap Percent — `RSK_ICE_TRAP_PERCENT`
- Options: `0`–`100` (slider)
- Default: **0**
- Tooltip:
  > If set above 0, each Junk item has that chance of being replaced with an extra Ice Trap.

## Static Hints (section)

Section tooltip (base): *"This setting adds some hints at locations other than Gossip Stones."*
All entries below are checkboxes (`Off`/`On`). MultiShip keeps these 21 (the omitted 6 are listed afterwards).

| Setting | RSK key | Default | Tooltip (verbatim) |
|---|---|---|---|
| ToT Altar Hint | `RSK_TOT_ALTAR_HINT` | **On** | Reading the Temple of Time altar as child will tell you the locations of the Spiritual Stones.<br>Reading the Temple of Time altar as adult will tell you the locations of the medallions, as well as the conditions for building the Rainbow Bridge and getting the Boss Key for Ganon's Castle. |
| Ganondorf Hint | `RSK_GANONDORF_HINT` | **On** | Talking to Ganondorf in his boss room will tell you the location of the Light Arrows and Master Sword.If this option is enabled and Ganondorf is reachable without these items, Gossip Stones will never hint the appropriate items. |
| Sheik Light Arrow Hint | `RSK_SHEIK_LA_HINT` | **On** | Talking to Sheik inside Ganon's Castle will tell you the location of the Light Arrows.If this option is enabled and Sheik is reachable without Light Arrows, Gossip Stones will never hint the Light Arrows. |
| Boss Door Hints | `RSK_BOSS_KEY_HINT` | **Off** | Navi will tell where boss key can be found when prompted at boss door. |
| Dampe's Diary Hint | `RSK_DAMPES_DIARY_HINT` | **Off** | Reading the diary of Dampé the gravekeeper as adult will tell you the location of one of the Hookshots. |
| Greg the Green Rupee Hint | `RSK_GREG_HINT` | **Off** | Talking to the chest game owner after buying a key will tell you the location of Greg the Green Rupee. |
| Saria's Hint | `RSK_SARIA_HINT` | **Off** | Talking to Saria either in person or through Saria's Song will tell you the location of a progressive magic meter. |
| Mido's Hint | `RSK_MIDO_HINT` | **Off** | Talking to Mido as child will tell you the location of the Kokiri Sword. |
| Frog Ocarina Game Hint | `RSK_FROGS_HINT` | **Off** | Standing near the pedestal for the frogs in Zora's River will tell you the reward for the frogs' Ocarina game. |
| Ocarina of Time Hint | `RSK_OOT_HINT` | **Off** | Sheik in the Temple of Time will tell you the item and song on the Ocarina of Time. |
| Biggoron's Hint | `RSK_BIGGORON_HINT` | **Off** | Talking to Biggoron will tell you the item he will give you in exchange for the Claim Check. |
| Big Poes Hint | `RSK_BIG_POES_HINT` | **Off** | Talking to the Poe Collector in the Market Guardhouse while adult will tell you what you receive for handing in Big Poes. |
| Chickens Hint | `RSK_CHICKENS_HINT` | **Off** | Talking to Anju as a child will tell you the item she will give you for delivering her cuccos to the pen. |
| Horseback Archery Hint | `RSK_HBA_HINT` | **Off** | Talking to the Horseback Archery gerudo in Gerudo Fortress, or the nearby sign, will tell you what you win for scoring 1000 and 1500 points on Horseback Archery. |
| Warp Song Hints | `RSK_WARP_SONG_HINTS` | **On** | Playing a warp song will tell you where it leads. (If warp song destinations are vanilla, this is always enabled.) |
| Scrub Hint Text | `RSK_SCRUB_TEXT_HINT` | **Off** | Business scrubs will reveal the identity of what they're selling. |
| 10 GS Hint | `RSK_KAK_10_SKULLS_HINT` | **Off** | Talking to the Cursed Resident in the Skulltula House who is saved after 10 tokens will tell you the reward. |
| 20 GS Hint | `RSK_KAK_20_SKULLS_HINT` | **Off** | Talking to the Cursed Resident in the Skulltula House who is saved after 20 tokens will tell you the reward. |
| 30 GS Hint | `RSK_KAK_30_SKULLS_HINT` | **Off** | Talking to the Cursed Resident in the Skulltula House who is saved after 30 tokens will tell you the reward. |
| 40 GS Hint | `RSK_KAK_40_SKULLS_HINT` | **Off** | Talking to the Cursed Resident in the Skulltula House who is saved after 40 tokens will tell you the reward. |
| 50 GS Hint | `RSK_KAK_50_SKULLS_HINT` | **Off** | Talking to the Cursed Resident in the Skulltula House who is saved after 50 tokens will tell you the reward. |

### Omitted static hints (documented for reference)

| Setting | RSK key | Base default | Tooltip (verbatim) |
|---|---|---|---|
| Hyrule Loach Hint | `RSK_LOACH_HINT` | Off | Talking to the fishing pond owner and asking to talk about something will tell you what's the reward for the Hyrule Loach. |
| Malon Hint | `RSK_MALON_HINT` | Off | Talking to Malon as adult will tell you the item on "Link's cow", the cow you win from beating her time on the Lon Lon Obstacle Course. |
| Fishing Pole Hint | `RSK_FISHING_POLE_HINT` | Off | Talking to the fishing pond owner without the fishing pole will tell you its location. |
| 100 GS Hint | `RSK_KAK_100_SKULLS_HINT` | Off | Talking to the Cursed Resident in the Skulltula House who is saved after 100 tokens will tell you the reward. |
| Mask Shop Hint | `RSK_MASK_SHOP_HINT` | Off | Reading the mask shop sign will tell you rewards from showing masks at the Deku Theatre. |
| Merchant Hint Text | `RSK_MERCHANT_TEXT_HINT` | Off | Merchants will reveal the identity of what they're selling (Shops are not affected by this setting). |

---

# Tab 5 — Starting Items

MultiShip takes the entire base SoH **Starting Items** tab (`RSG_MENU_SIDEBAR_STARTING_ITEMS`),
composed of the **Equips**, **Items**, **Normal Songs**, and **Warp Songs** sections, **except
Link's Pocket** (see below). All defaults are the base SoH defaults.

> ⚠️ **Important:** every kept Starting Items setting has **no tooltip string in the base game**
> (the description argument is empty). Only the omitted *Link's Pocket* controls carry tooltips.

⚠️ **OMITTED / FIXED — Link's Pocket.** The Link's Pocket control is removed from the UI and the
starting reward is fixed to **"Any Reward"**. Two base keys are involved:

- **Link's Pocket** — `RSK_LINKS_POCKET` — options `Dungeon Reward (0)`, `Advancement (1)`,
  `Anything (2)`, `Nothing (3)`; base default **Dungeon Reward** (`RO_LINKS_POCKET_DUNGEON_REWARD`).
  Tooltip:
  > Dungeon Reward - Link will start with a Spiritual Stone or Medallion, and specific options will open up
  >
  > Advancement - Link will start with a useful item.
  >
  > Anything - Link will start with a random item.
  >
  > Nothing - Link will not start with a bonus item.
- **Link's Pocket Reward Type** — `RSK_LINKS_POCKET_REWARD` — options `Any Reward (0)`,
  `Any Stone (1)`, `Any Medallion (2)`, `Light Medallion (3)`; base default **Any Reward**
  (`RO_LINKS_POCKET_ANY_REWARD`). This is the key **FIXED to "Any Reward"** in MultiShip.
  Tooltip:
  > Any Reward - Link starts with a random Spiritual Stone or Medallion
  >
  > Stone - Link starts with a random Spiritual Stone.
  >
  > Any Medallion - Link starts with a random Medallion.
  >
  > Light Medallion - Link starts with the Light Medallion.

  *(The "fixed to Any Reward" value names the `RSK_LINKS_POCKET_REWARD` option specifically;
  `RSK_LINKS_POCKET` itself is presumed to stay at its "Dungeon Reward" default so a reward is
  actually granted. Flagged because the spec only named "Any Reward".)*

## Equips (section) — kept

| Setting | RSK key | Type | Options | Default | Tooltip |
|---|---|---|---|---|---|
| Start with Kokiri Sword | `RSK_STARTING_KOKIRI_SWORD` | checkbox | Off / On | **Off** | *(no base-game tooltip)* |
| Start with Master Sword | `RSK_STARTING_MASTER_SWORD` | checkbox | Off / On | **Off** | *(no base-game tooltip)* |
| Start with Deku Shield | `RSK_STARTING_DEKU_SHIELD` | checkbox | Off / On | **Off** | *(no base-game tooltip)* |

## Items (section) — kept

| Setting | RSK key | Type | Options | Default | Tooltip |
|---|---|---|---|---|---|
| Start with Ocarina | `RSK_STARTING_OCARINA` | combobox | `Off (0)`, `Fairy Ocarina (1)`, `Ocarina of Time (2)` | **Off** (`RO_STARTING_OCARINA_OFF`) | *(no base-game tooltip)* |
| Start with Stick Ammo | `RSK_STARTING_STICKS` | checkbox | `No (0)`, `Yes (1)` | **No / Off** | *(no base-game tooltip)* |
| Start with Nut Ammo | `RSK_STARTING_NUTS` | checkbox | `No (0)`, `Yes (1)` | **No / Off** | *(no base-game tooltip)* |
| Start with Magic Beans | `RSK_STARTING_BEANS` | checkbox | `No (0)`, `Yes (1)` | **No / Off** | *(no base-game tooltip)* |
| Gold Skulltula Tokens | `RSK_STARTING_SKULLTULA_TOKEN` | slider | `0`–`100` | **0** | *(no base-game tooltip)* |
| Starting Hearts | `RSK_STARTING_HEARTS` | slider | `1`–`20` | **3** (stored option index `2`; OoT vanilla) | *(no base-game tooltip)* |

## Normal Songs (section) — kept

All checkboxes (`Off`/`On`), base default **Off**, no base-game tooltip.

| Setting | RSK key |
|---|---|
| Start with Zelda's Lullaby | `RSK_STARTING_ZELDAS_LULLABY` |
| Start with Epona's Song | `RSK_STARTING_EPONAS_SONG` |
| Start with Saria's Song | `RSK_STARTING_SARIAS_SONG` |
| Start with Sun's Song | `RSK_STARTING_SUNS_SONG` |
| Start with Song of Time | `RSK_STARTING_SONG_OF_TIME` |
| Start with Song of Storms | `RSK_STARTING_SONG_OF_STORMS` |

## Warp Songs (section) — kept

All checkboxes (`Off`/`On`), base default **Off**, no base-game tooltip.

| Setting | RSK key |
|---|---|
| Start with Minuet of Forest | `RSK_STARTING_MINUET_OF_FOREST` |
| Start with Bolero of Fire | `RSK_STARTING_BOLERO_OF_FIRE` |
| Start with Serenade of Water | `RSK_STARTING_SERENADE_OF_WATER` |
| Start with Requiem of Spirit | `RSK_STARTING_REQUIEM_OF_SPIRIT` |
| Start with Nocturne of Shadow | `RSK_STARTING_NOCTURNE_OF_SHADOW` |
| Start with Prelude of Light | `RSK_STARTING_PRELUDE_OF_LIGHT` |

---

# Verification notes & flags

- **All RSK keys and tooltips were located** in the ShipwreckCombo source. Nothing is missing.
- **Settings with no base-game tooltip** (empty description string in `settings.cpp`) — the entire
  kept Tab 5 (Starting Items): `RSK_STARTING_KOKIRI_SWORD`, `RSK_STARTING_MASTER_SWORD`,
  `RSK_STARTING_DEKU_SHIELD`, `RSK_STARTING_OCARINA`, `RSK_STARTING_STICKS`, `RSK_STARTING_NUTS`,
  `RSK_STARTING_BEANS`, `RSK_STARTING_SKULLTULA_TOKEN`, `RSK_STARTING_HEARTS`, and all 12 starting
  songs. (The companion sliders `RSK_TRIAL_COUNT` has a tooltip; `RSK_KEYRINGS_RANDOM_COUNT` does not.)
- **MultiShip defaults that override base SoH:** `RSK_SKIP_CHILD_STEALTH` (MultiShip *Checked* vs
  base *Don't Skip*) and `RSK_GANONS_TRIALS` (MultiShip *Random Number* vs base *Set Number*).
- **Verbatim quirks preserved from source:** "mweeped" (Zora's Fountain); "dungon" typo (Gerudo
  Fortress Keys); "Ganon's Boss Key Key" double word (Ganon's Boss Key); missing space in
  "Master Sword.If" (Ganondorf Hint) and "Light Arrows.If" (Sheik Light Arrow Hint);
  "does not effect" (Frog Song Rupees).
- **Label vs tooltip mismatch:** `RSK_LINKS_POCKET_REWARD` option label is `Any Stone` while its
  tooltip line reads `Stone`. Options use the UI labels from `settings.cpp`; tooltip reproduced as-is.
- **`RSK_STARTING_HEARTS` default:** the macro stores option index `2` over the `NumOpts(1, 20)`
  label list (`"1".."20"`), i.e. label `"3"` → **3 starting hearts** (OoT vanilla).
- **Open Forest naming:** base SoH internal name is *Closed Forest* with labels `On/Deku Only/Off`;
  MultiShip presents them as *Closed/Deku Only/Open* (`On` = closed, `Off` = open).
