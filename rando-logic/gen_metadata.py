#!/usr/bin/env python3
"""One-time codegen: emit src/rando/MetaData.h (location + item data tables) from the
validated rando_logic.json extraction. Pure data — consumed by the C++ fill/spoiler."""
import json, os

here = os.path.dirname(os.path.abspath(__file__))
d = json.load(open(os.path.join(here, 'rando_logic.json'), encoding='utf-8'))

def esc(s):
    return s.replace('\\', '\\\\').replace('"', '\\"')

out = [
    '// Auto-generated location + item metadata (from the SoH source extraction).',
    '// Pure data for the fill + spoiler: vanilla items, names, progression, shuffle set.',
    '#pragma once',
    '#include "randomizerEnums.h"',
    '// Per-location shuffle category. The fill decides if a location is in the pool from',
    '// this + the seed settings (see Fill::IsShuffled). LC_OTHER (incl. all Master Quest',
    '// locations) is never shuffled by the clean-room engine yet.',
    'enum LocCategory {',
    '    LC_OTHER, LC_STANDARD, LC_SONG, LC_OCARINA, LC_BOSS_HEART, LC_ADULT_TRADE,',
    '    LC_SKULL_TOKEN, LC_SCRUB, LC_SHOP, LC_COW, LC_FISH,',
    '    LC_POT, LC_CRATE, LC_FREESTANDING, LC_BEEHIVE,',
    '    LC_SMALL_KEY, LC_BOSS_KEY, LC_MAP, LC_COMPASS, LC_GANON_BOSS_KEY, LC_DUNGEON_REWARD',
    '};',
    '// Dungeon zone for restricted dungeon-item placement (keys/maps/rewards, FEAT-5).',
    '// DZ_OVERWORLD = not in a dungeon; the rest are the 12 key/reward-bearing dungeons.',
    'enum DungeonZone {',
    '    DZ_OVERWORLD, DZ_DEKU_TREE, DZ_DODONGOS_CAVERN, DZ_JABU_JABU, DZ_FOREST_TEMPLE,',
    '    DZ_FIRE_TEMPLE, DZ_WATER_TEMPLE, DZ_SPIRIT_TEMPLE, DZ_SHADOW_TEMPLE,',
    '    DZ_BOTTOM_OF_THE_WELL, DZ_ICE_CAVERN, DZ_GERUDO_TRAINING_GROUND, DZ_GANONS_CASTLE',
    '};',
    'struct LocMeta { RandomizerCheck rc; const char* name; RandomizerGet vanilla; bool shuffled;'
    ' LocCategory category; bool dungeon; uint8_t zone; };',
    'struct ItemMeta { RandomizerGet rg; const char* name; bool advancement; };',
    'inline const LocMeta kLocations[] = {',
]
# Categories that are in the pool under default settings (chests/NPC rewards, songs,
# ocarina, boss-heart/other rewards, adult trade). Off-by-default sanity types
# (GS tokens, shops, scrubs, keys, maps, …) map to settings-controlled categories or
# LC_OTHER and are added to the pool only when their setting is on.
DEFAULT_SHUFFLED = {'LC_STANDARD', 'LC_SONG', 'LC_OCARINA', 'LC_BOSS_HEART', 'LC_ADULT_TRADE'}
TYPE_TO_CAT = {
    'STANDARD': 'LC_STANDARD',
    'OCARINA': 'LC_OCARINA',
    'BOSS_HEART_OR_OTHER_REWARD': 'LC_BOSS_HEART',
    'ADULT_TRADE': 'LC_ADULT_TRADE',
    'SKULL_TOKEN': 'LC_SKULL_TOKEN',
    'SCRUB': 'LC_SCRUB',
    'SHOP': 'LC_SHOP',
    'COW': 'LC_COW',
    'FISH': 'LC_FISH',
    'POT': 'LC_POT',
    'CRATE': 'LC_CRATE',
    'SMALL_CRATE': 'LC_CRATE',
    'NLCRATE': 'LC_CRATE',
    'FREESTANDING': 'LC_FREESTANDING',
    'BEEHIVE': 'LC_BEEHIVE',
    'SMALL_KEY': 'LC_SMALL_KEY',
    'BOSS_KEY': 'LC_BOSS_KEY',
    'MAP': 'LC_MAP',
    'COMPASS': 'LC_COMPASS',
    'GANON_BOSS_KEY': 'LC_GANON_BOSS_KEY',
    'DUNGEON_REWARD': 'LC_DUNGEON_REWARD',
}
# Region scene -> dungeon zone (everything not listed = DZ_OVERWORLD). The *_BOSS scenes
# hold the dungeon reward; they belong to the same zone as the dungeon body.
SCENE_TO_ZONE = {
    'SCENE_DEKU_TREE': 'DZ_DEKU_TREE', 'SCENE_DEKU_TREE_BOSS': 'DZ_DEKU_TREE',
    'SCENE_DODONGOS_CAVERN': 'DZ_DODONGOS_CAVERN', 'SCENE_DODONGOS_CAVERN_BOSS': 'DZ_DODONGOS_CAVERN',
    'SCENE_JABU_JABU': 'DZ_JABU_JABU', 'SCENE_JABU_JABU_BOSS': 'DZ_JABU_JABU',
    'SCENE_FOREST_TEMPLE': 'DZ_FOREST_TEMPLE', 'SCENE_FOREST_TEMPLE_BOSS': 'DZ_FOREST_TEMPLE',
    'SCENE_FIRE_TEMPLE': 'DZ_FIRE_TEMPLE', 'SCENE_FIRE_TEMPLE_BOSS': 'DZ_FIRE_TEMPLE',
    'SCENE_WATER_TEMPLE': 'DZ_WATER_TEMPLE', 'SCENE_WATER_TEMPLE_BOSS': 'DZ_WATER_TEMPLE',
    'SCENE_SPIRIT_TEMPLE': 'DZ_SPIRIT_TEMPLE', 'SCENE_SPIRIT_TEMPLE_BOSS': 'DZ_SPIRIT_TEMPLE',
    'SCENE_SHADOW_TEMPLE': 'DZ_SHADOW_TEMPLE', 'SCENE_SHADOW_TEMPLE_BOSS': 'DZ_SHADOW_TEMPLE',
    'SCENE_BOTTOM_OF_THE_WELL': 'DZ_BOTTOM_OF_THE_WELL',
    'SCENE_ICE_CAVERN': 'DZ_ICE_CAVERN',
    'SCENE_GERUDO_TRAINING_GROUND': 'DZ_GERUDO_TRAINING_GROUND',
    'SCENE_INSIDE_GANONS_CASTLE': 'DZ_GANONS_CASTLE', 'SCENE_GANONS_TOWER': 'DZ_GANONS_CASTLE',
    'SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR': 'DZ_GANONS_CASTLE',
    'SCENE_GANONDORF_BOSS': 'DZ_GANONS_CASTLE', 'SCENE_GANON_BOSS': 'DZ_GANONS_CASTLE',
}
# Each location's zone = the scene of a region it appears in; prefer a dungeon zone over
# overworld when a check is referenced from regions in more than one scene.
loc_zone = {}
for _r in d['regions']:
    _z = SCENE_TO_ZONE.get(_r.get('scene', ''), 'DZ_OVERWORLD')
    for _loc in _r['locations']:
        _c = _loc['check']
        if loc_zone.get(_c, 'DZ_OVERWORLD') == 'DZ_OVERWORLD':
            loc_zone[_c] = _z
# GS tokens inside these dungeons count as "dungeon" for tokensanity dungeon/overworld.
DUNGEON_GS_AREAS = {
    'RC_BOTTOM_OF_THE_WELL', 'RC_DEKU_TREE', 'RC_DODONGOS_CAVERN', 'RC_FIRE_TEMPLE',
    'RC_FOREST_TEMPLE', 'RC_ICE_CAVERN', 'RC_JABU_JABUS_BELLY', 'RC_SHADOW_TEMPLE',
    'RC_SPIRIT_TEMPLE', 'RC_WATER_TEMPLE',
}
nloc = 0
for rc, m in d['locationMeta'].items():
    van = m.get('vanilla') or 'RG_NONE'
    typ = m.get('type', '')
    # GS-token locations use a factory with no item arg -> vanilla parsed as RG_NONE,
    # but they award a Gold Skulltula Token (needed for GetGSCount gating).
    if van == 'RG_NONE' and typ == 'SKULL_TOKEN':
        van = 'RG_GOLD_SKULLTULA_TOKEN'
    # Most fish (pond + grotto) extract with no item arg -> RG_NONE; the vanilla "reward"
    # is the fish itself (RG_FISH, junk). Give them that so fishsanity can pool them (a
    # RG_NONE location is skipped entirely by Fill). The 5 Zora's Domain fish already carry
    # RG_FISH explicitly.
    if van == 'RG_NONE' and typ == 'FISH':
        van = 'RG_FISH'
    # Master Quest locations are never reachable with vanilla dungeons -> never pooled.
    is_mq = '_MQ_' in rc
    cat = 'LC_OTHER' if is_mq else TYPE_TO_CAT.get(typ, 'LC_OTHER')
    dungeon = False
    if cat == 'LC_SKULL_TOKEN':
        area = rc[:rc.find('_GS')] if '_GS' in rc else rc
        dungeon = area in DUNGEON_GS_AREAS
    shuffled = cat in DEFAULT_SHUFFLED
    zone = loc_zone.get(rc, 'DZ_OVERWORLD')
    out.append('  {%s, "%s", %s, %s, %s, %s, %s},' % (rc, esc(m.get('name', rc)), van,
                                                  'true' if shuffled else 'false', cat,
                                                  'true' if dungeon else 'false', zone))
    nloc += 1
# Song locations are a separate SoH subsystem — absent from location_list.cpp, so add
# them here with their vanilla song as the item (shuffled into the pool).
SONG_LOCATIONS = [
    ('RC_SONG_FROM_IMPA', "Song from Impa", 'RG_ZELDAS_LULLABY'),
    ('RC_SONG_FROM_MALON', "Song from Malon", 'RG_EPONAS_SONG'),
    ('RC_SONG_FROM_SARIA', "Song from Saria", 'RG_SARIAS_SONG'),
    ('RC_SONG_FROM_ROYAL_FAMILYS_TOMB', "Song from Royal Family's Tomb", 'RG_SUNS_SONG'),
    ('RC_SONG_FROM_OCARINA_OF_TIME', "Song from Ocarina of Time", 'RG_SONG_OF_TIME'),
    ('RC_SONG_FROM_WINDMILL', "Song from Windmill", 'RG_SONG_OF_STORMS'),
    ('RC_SHEIK_IN_FOREST', "Sheik in Forest", 'RG_MINUET_OF_FOREST'),
    ('RC_SHEIK_IN_CRATER', "Sheik in Crater", 'RG_BOLERO_OF_FIRE'),
    ('RC_SHEIK_IN_ICE_CAVERN', "Sheik in Ice Cavern", 'RG_SERENADE_OF_WATER'),
    ('RC_SHEIK_AT_COLOSSUS', "Sheik at Colossus", 'RG_REQUIEM_OF_SPIRIT'),
    ('RC_SHEIK_IN_KAKARIKO', "Sheik in Kakariko", 'RG_NOCTURNE_OF_SHADOW'),
    ('RC_SHEIK_AT_TEMPLE', "Sheik at Temple", 'RG_PRELUDE_OF_LIGHT'),
]
for rc, name, song in SONG_LOCATIONS:
    out.append('  {%s, "%s", %s, true, LC_SONG, false, DZ_OVERWORLD},' % (rc, name, song))
    nloc += 1
out.append('};')
out.append('inline const int kLocationCount = %d;' % nloc)
out.append('inline const ItemMeta kItems[] = {')
nit = 0
for it in d['items']:
    out.append('  {%s, "%s", %s},' % (it['key'], esc(it['name']),
                                      'true' if it.get('advancement') else 'false'))
    nit += 1
out.append('};')
out.append('inline const int kItemCount = %d;' % nit)

open(os.path.join(here, '..', 'src', 'rando', 'MetaData.h'), 'w', encoding='utf-8').write('\n'.join(out) + '\n')
ndef = sum(1 for rc, m in d['locationMeta'].items()
           if '_MQ_' not in rc and TYPE_TO_CAT.get(m.get('type', ''), 'LC_OTHER') in DEFAULT_SHUFFLED) \
    + len(SONG_LOCATIONS)
ngs = sum(1 for rc, m in d['locationMeta'].items()
          if '_MQ_' not in rc and m.get('type', '') == 'SKULL_TOKEN')
print('wrote MetaData.h:', nloc, 'locations,', nit, 'items;', ndef, 'default-shuffled,', ngs, 'GS tokens')
