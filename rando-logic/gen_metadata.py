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
    'struct LocMeta { RandomizerCheck rc; const char* name; RandomizerGet vanilla; bool shuffled; };',
    'struct ItemMeta { RandomizerGet rg; const char* name; bool advancement; };',
    'inline const LocMeta kLocations[] = {',
]
# True default-settings shuffle pool: chests/NPC rewards, songs (song locations),
# ocarina, boss-heart/other rewards, adult trade. Excludes the off-by-default sanity
# types (GS tokens, grass, pots, scrubs, shops, gossip, fairies, fish, cows, …).
SHUFFLE_TYPES = {'STANDARD', 'SONG_LOCATION', 'OCARINA', 'BOSS_HEART_OR_OTHER_REWARD', 'ADULT_TRADE'}
nloc = 0
for rc, m in d['locationMeta'].items():
    van = m.get('vanilla') or 'RG_NONE'
    # GS-token locations use a factory with no item arg -> vanilla parsed as RG_NONE,
    # but they award a Gold Skulltula Token (needed for GetGSCount gating).
    if van == 'RG_NONE' and m.get('type', '') == 'SKULL_TOKEN':
        van = 'RG_GOLD_SKULLTULA_TOKEN'
    # Exclude Master Quest locations — default settings use vanilla dungeons, so MQ
    # checks are never reachable and must not be in the pool.
    shuffled = (m.get('type', '') in SHUFFLE_TYPES) and ('_MQ_' not in rc)
    out.append('  {%s, "%s", %s, %s},' % (rc, esc(m.get('name', rc)), van,
                                          'true' if shuffled else 'false'))
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
    out.append('  {%s, "%s", %s, true},' % (rc, name, song))
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
print('wrote MetaData.h:', nloc, 'locations,', nit, 'items;',
      sum(1 for m in d['locationMeta'].values() if m.get('type','') in SHUFFLE_TYPES and '_MQ_' not in rc), 'shuffled')
