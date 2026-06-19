#!/usr/bin/env python3
"""
Extract the Ship of Harkinian randomizer logic graph from ShipwreckCombo into
structured data (JSON) + a self-contained browsable HTML viewer.

Source of truth:
  ShipwreckCombo/soh/soh/Enhancements/randomizer/
    location_access/*.cpp   -> regions, events, locations, exits + their RULES
    location_list.cpp       -> location metadata (friendly name, type, vanilla item)
    item_list.cpp           -> items (name, type, advancement)

Requirements (rules) are emitted as the cleaned condition strings exactly as the
macros build them, so they cover ALL settings (the rule reads settings/tricks at
eval time). Nothing is resolved for a specific preset.
"""
import os, re, json, html, sys

HERE = os.path.dirname(os.path.abspath(__file__))
RANDO = os.path.normpath(os.path.join(
    HERE, "..", "..", "ShipwreckCombo", "soh", "soh", "Enhancements", "randomizer"))
ACCESS = os.path.join(RANDO, "location_access")

# ---------- comment / string aware scanner helpers ----------

def strip_comments(text):
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"' or c == "'":
            q = c; out.append(c); i += 1
            while i < n:
                out.append(text[i])
                if text[i] == '\\':
                    i += 1
                    if i < n: out.append(text[i]); i += 1
                    continue
                if text[i] == q:
                    i += 1; break
                i += 1
            continue
        if c == '/' and i + 1 < n and text[i+1] == '/':
            while i < n and text[i] != '\n': i += 1
            continue
        if c == '/' and i + 1 < n and text[i+1] == '*':
            i += 2
            while i + 1 < n and not (text[i] == '*' and text[i+1] == '/'): i += 1
            i += 2; continue
        out.append(c); i += 1
    return ''.join(out)

def match_paren(text, open_idx):
    """open_idx points at '('. Return index just past the matching ')'."""
    depth = 0; i, n = open_idx, len(text)
    while i < n:
        c = text[i]
        if c in '"\'':
            q = c; i += 1
            while i < n:
                if text[i] == '\\': i += 2; continue
                if text[i] == q: i += 1; break
                i += 1
            continue
        if c == '(': depth += 1
        elif c == ')':
            depth -= 1
            if depth == 0: return i + 1
        i += 1
    return -1

def split_top_commas(s):
    """Split on commas at brace/paren depth 0 (string aware)."""
    parts = []; depth = 0; cur = []; i, n = 0, len(s)
    while i < n:
        c = s[i]
        if c in '"\'':
            q = c; cur.append(c); i += 1
            while i < n:
                cur.append(s[i])
                if s[i] == '\\':
                    i += 1
                    if i < n: cur.append(s[i]); i += 1
                    continue
                if s[i] == q: i += 1; break
                i += 1
            continue
        if c in '([{': depth += 1
        elif c in ')]}': depth -= 1
        if c == ',' and depth == 0:
            parts.append(''.join(cur)); cur = []; i += 1; continue
        cur.append(c); i += 1
    parts.append(''.join(cur))
    return parts

def clean_rule(cond):
    cond = cond.strip()
    cond = cond.replace('logic->', '')
    cond = re.sub(r'\s+', ' ', cond)
    return cond.strip()

# ---------- parse region access files ----------

MACROS = ('LOCATION', 'ENTRANCE', 'EVENT_ACCESS')

def find_macros(text):
    """Yield (start_index, macro_name, target, rule)."""
    for m in re.finditer(r'\b(LOCATION|ENTRANCE|EVENT_ACCESS)\s*\(', text):
        name = m.group(1)
        open_idx = text.index('(', m.start())
        end = match_paren(text, open_idx)
        if end < 0: continue
        inner = text[open_idx+1:end-1]
        parts = split_top_commas(inner)
        if len(parts) < 2: continue
        target = parts[0].strip()
        rule = clean_rule(','.join(parts[1:]))
        yield m.start(), name, target, rule

REGION_RE = re.compile(
    r'areaTable\[(RR_\w+)\]\s*=\s*Region\(\s*"([^"]*)"\s*,\s*(\w+)')

def parse_access():
    regions = {}            # key -> dict
    order = []
    for root, _, files in os.walk(ACCESS):
        for fn in sorted(files):
            if not fn.endswith('.cpp'): continue
            raw = open(os.path.join(root, fn), encoding='utf-8', errors='replace').read()
            text = strip_comments(raw)
            # region boundaries
            regs = [(m.start(), m.group(1), m.group(2), m.group(3))
                    for m in REGION_RE.finditer(text)]
            for idx, (pos, key, name, scene) in enumerate(regs):
                if key not in regions:
                    regions[key] = {'key': key, 'name': name, 'scene': scene,
                                    'file': fn, 'events': [], 'locations': [], 'exits': []}
                    order.append(key)
            def region_at(p):
                cur = None
                for pos, key, name, scene in regs:
                    if pos <= p: cur = key
                    else: break
                return cur
            for p, mac, target, rule in find_macros(text):
                rk = region_at(p)
                if rk is None: continue
                r = regions[rk]
                if mac == 'LOCATION': r['locations'].append({'check': target, 'rule': rule})
                elif mac == 'ENTRANCE': r['exits'].append({'to': target, 'rule': rule})
                elif mac == 'EVENT_ACCESS': r['events'].append({'event': target, 'rule': rule})
    return [regions[k] for k in order]

# ---------- parse item_list ----------

def parse_items():
    text = strip_comments(open(os.path.join(RANDO, 'item_list.cpp'),
                               encoding='utf-8', errors='replace').read())
    items = []
    for m in re.finditer(r'itemTable\[(RG_\w+)\]\s*=\s*Item\s*\(', text):
        key = m.group(1)
        open_idx = text.index('(', m.start())
        end = match_paren(text, open_idx)
        if end < 0: continue
        inner = text[open_idx+1:end-1]
        parts = split_top_commas(inner)
        name = key
        tm = re.search(r'"([^"]*)"', parts[1]) if len(parts) > 1 else None
        if tm: name = tm.group(1)
        itype = parts[2].strip() if len(parts) > 2 else ''
        adv = ''
        for p in parts[3:6]:
            ps = p.strip()
            if ps in ('true', 'false'): adv = ps; break
        cat = ''
        cm = re.search(r'ITEM_CATEGORY_\w+', inner)
        if cm: cat = cm.group(0)
        items.append({'key': key, 'name': name,
                      'type': itype.replace('ITEMTYPE_', ''),
                      'advancement': adv == 'true',
                      'category': cat.replace('ITEM_CATEGORY_', '')})
    return items

# ---------- parse location_list metadata ----------

FACTORY_TYPE = {
    'GSToken': 'SKULL_TOKEN', 'Grass': 'GRASS', 'Bush': 'BUSH', 'Pot': 'POT',
    'Crate': 'CRATE', 'SmallCrate': 'SMALL_CRATE', 'NLCrate': 'NLCRATE',
    'Boulder': 'BOULDER', 'Rock': 'ROCK', 'Fish': 'FISH', 'GrottoFish': 'FISH',
    'Sign': 'SIGN', 'WonderItem': 'WONDER_ITEM', 'Icicle': 'ICICLE',
    'RedIce': 'RED_ICE', 'NLTree': 'NLTREE', 'Tree': 'TREE',
    'HintStone': 'GOSSIP_STONE', 'OtherHint': 'STATIC_HINT',
    'BeanFairy': 'BEAN_FAIRY', 'ButterflyFairy': 'BUTTERFLY_FAIRY',
    'FountainFairy': 'FOUNTAIN_FAIRY', 'SongFairy': 'SONG_FAIRY',
    'StoneFairy': 'STONE_FAIRY', 'Collectable': 'FREESTANDING',
}
# Types excluded from the pool under default ("checksanity off") settings.
SANITY_TYPES = {
    'GRASS', 'BUSH', 'POT', 'CRATE', 'SMALL_CRATE', 'NLCRATE', 'BOULDER', 'ROCK',
    'FREESTANDING', 'BEEHIVE', 'COW', 'FISH', 'WONDER_ITEM', 'ICICLE', 'RED_ICE',
    'NLTREE', 'TREE', 'SIGN', 'GOSSIP_STONE', 'BEAN_FAIRY', 'BUTTERFLY_FAIRY',
    'FOUNTAIN_FAIRY', 'SONG_FAIRY', 'STONE_FAIRY', 'MERCHANT', 'SHOP', 'BEGGAR',
    'STATIC_HINT', 'CHEST_GAME', 'FROG_SONG',
}

def _location_meta_files():
    """location_list.cpp (the 741 base checks — their types feed the default pool) FIRST
    as the authority (it wins on any shared RC, keeping the default pool stable), then the
    files that register the off-by-default sanity collectibles this engine can pool:
    cows, fish, freestanding, pots, crates (+ small/NL), beehives. Songs/grass/rocks/etc.
    are deliberately NOT scanned — songs would collide with gen_metadata's manual SONG
    append, and the rest have no LC_* category yet (add their file when a feature needs it)."""
    names = ['location_list.cpp', 'ShuffleCows.cpp', 'fishsanity.cpp',
             'ShuffleFreestanding.cpp', 'ShufflePots.cpp', 'ShuffleCrates.cpp',
             'ShuffleBeehives.cpp']
    return [p for p in (os.path.join(RANDO, n) for n in names) if os.path.exists(p)]

def parse_locations_meta():
    meta = {}
    for path in _location_meta_files():
        text = strip_comments(open(path, encoding='utf-8', errors='replace').read())
        for m in re.finditer(r'locationTable\[(RC_\w+)\]\s*=\s*Location::(\w+)\s*\(', text):
            key, factory = m.group(1), m.group(2)
            if key in meta:
                continue  # first file (location_list.cpp) wins — keeps the default pool stable
            open_idx = text.index('(', m.start())
            end = match_paren(text, open_idx)
            if end < 0: continue
            inner = text[open_idx+1:end-1]
            nm = re.search(r'"([^"]*)"', inner)
            name = nm.group(1) if nm else key
            rt = re.search(r'RCTYPE_(\w+)', inner)
            if rt: rtype = rt.group(1)
            else: rtype = FACTORY_TYPE.get(factory, factory.upper())
            vi = re.search(r'\bRG_\w+', inner)
            vanilla = vi.group(0) if vi else ''
            meta[key] = {'name': name, 'type': rtype, 'vanilla': vanilla,
                         'standard': rtype not in SANITY_TYPES}
    return meta

# ---------- resolve helper functions down to item sets ----------

def match_brace(text, open_idx):
    depth = 0; i, n = open_idx, len(text)
    while i < n:
        c = text[i]
        if c in '"\'':
            q = c; i += 1
            while i < n:
                if text[i] == '\\': i += 2; continue
                if text[i] == q: i += 1; break
                i += 1
            continue
        if c == '{': depth += 1
        elif c == '}':
            depth -= 1
            if depth == 0: return i + 1
        i += 1
    return -1

FREE_HELPERS = ('CanPlantBean', 'BothAges', 'ChildCanAccess', 'AdultCanAccess',
                'SpiritShared', 'SpiritCertainAccess', 'DMCPadToPots')
# Helpers treated as leaves: the item is exactly their RG_ argument (handled at the
# rule level), so we must NOT pull in the whole switch body.
LEAF_HELPERS = {'CanUse', 'HasItem'}

def parse_helper_bodies():
    bodies = {}
    logic = strip_comments(open(os.path.join(RANDO, 'logic.cpp'),
                                 encoding='utf-8', errors='replace').read())
    for m in re.finditer(r'\bbool\s+Logic::(\w+)\s*\(', logic):
        name = m.group(1)
        brace = logic.find('{', m.end())
        end = match_brace(logic, brace)
        if end > 0: bodies[name] = logic[brace+1:end-1]
    acc = strip_comments(open(os.path.join(ACCESS, '..', 'location_access.cpp'),
                              encoding='utf-8', errors='replace').read()) \
        if os.path.exists(os.path.join(ACCESS, '..', 'location_access.cpp')) else ''
    acc2 = strip_comments(open(os.path.join(RANDO, 'location_access.cpp'),
                               encoding='utf-8', errors='replace').read())
    for fn in FREE_HELPERS:
        m = re.search(r'\bbool\s+' + fn + r'\s*\(', acc2)
        if not m: continue
        brace = acc2.find('{', m.end())
        end = match_brace(acc2, brace)
        if end > 0: bodies[fn] = acc2[brace+1:end-1]
    return bodies

def build_canuse_extra(body):
    """For each `case RG_X:` in CanUse, the extra items its branch references."""
    extra = {}
    for cm in re.finditer(r'case\s+(RG_\w+)\s*:', body):
        rg = cm.group(1)
        seg = body[cm.end():]
        rm = re.search(r'\breturn\b', seg)
        if rm:
            stop = seg.find(';', rm.end())
            seg = seg[:stop if stop >= 0 else 200]
        else:
            seg = seg[:120]
        # drop sibling fall-through `case RG_X:` labels — only the branch body counts
        seg = re.sub(r'case\s+RG_\w+\s*:', '', seg)
        items = set(re.findall(r'RG_\w+', seg)) - {rg}
        extra[rg] = items
    return extra

def calls_in(body):
    return set(re.findall(r'\b([A-Z][A-Za-z0-9]+)\s*\(', body))

def enemy_branches(body):
    """For a `switch (enemy)` helper, map each RE_ enemy to its branch text
    (label up to its terminating `return …;`), so we resolve per-argument."""
    branches = {}
    for cm in re.finditer(r'case\s+(RE_\w+)\s*:', body):
        seg = body[cm.end():]
        rm = re.search(r'\breturn\b', seg)
        if not rm: continue
        stop = seg.find(';', rm.end())
        branches[cm.group(1)] = seg[:stop if stop >= 0 else 300]
    return branches

def build_item_resolver():
    bodies = parse_helper_bodies()
    names = set(bodies)
    canuse_extra = build_canuse_extra(bodies.get('CanUse', ''))
    enemy_sw = {h: enemy_branches(b) for h, b in bodies.items()
                if re.search(r'switch\s*\(\s*enemy\s*\)', b)}

    def items_in_text(text, visited):
        items = set(re.findall(r'RG_\w+', text))
        for cm in re.finditer(r'CanUse\(\s*(RG_\w+)', text):
            items |= canuse_extra.get(cm.group(1), set())
        for cm in re.finditer(r'\b([A-Z][A-Za-z0-9]+)\s*\(', text):
            c = cm.group(1)
            if c in LEAF_HELPERS or c not in names: continue
            if c in enemy_sw:
                am = re.match(r'\s*(RE_\w+)', text[cm.end():])
                enemy = am.group(1) if am else None
                items |= helper_items(c, enemy, visited)
            else:
                items |= helper_items(c, None, visited)
        return items

    memo = {}
    def helper_items(name, enemy, visited):
        key = (name, enemy)
        if key in memo: return memo[key]
        if key in visited: return set()
        visited = visited | {key}
        if name in enemy_sw:
            text = enemy_sw[name].get(enemy, '') if enemy else ' '.join(enemy_sw[name].values())
        else:
            text = bodies.get(name, '')
        res = items_in_text(text, visited)
        memo[key] = res
        return res

    def resolve(rule):
        return items_in_text(rule, set())
    return resolve

# RG_ tokens that are not real progression items / are noise to hide from the item view
ITEM_HIDE = {'RG_NONE'}

def prettify(rg):
    return rg.replace('RG_', '').replace('_', ' ').title()

# ---------- main ----------

def main():
    print('Parsing region access files...'); regions = parse_access()
    print('Parsing items...'); items = parse_items()
    print('Parsing location metadata...'); locmeta = parse_locations_meta()
    print('Building item resolver from logic.cpp...'); resolve = build_item_resolver()

    rg2name = {it['key']: it['name'] for it in items}
    def items_for(rule):
        rgs = sorted(resolve(rule) - ITEM_HIDE)
        return [{'key': rg, 'name': rg2name.get(rg, prettify(rg))} for rg in rgs]
    def ages(rule):
        a = []
        if re.search(r'\bIsChild\b', rule): a.append('child')
        if re.search(r'\bIsAdult\b', rule): a.append('adult')
        return a

    # attach metadata + resolved items to each rule
    for r in regions:
        for loc in r['locations']:
            md = locmeta.get(loc['check'])
            if md:
                loc['name'] = md['name']; loc['type'] = md['type']
                loc['standard'] = md['standard']
            else:
                loc['name'] = loc['check']; loc['type'] = '?'; loc['standard'] = True
            loc['items'] = items_for(loc['rule']); loc['ages'] = ages(loc['rule'])
        for x in r['exits']:
            x['items'] = items_for(x['rule']); x['ages'] = ages(x['rule'])
        for e in r['events']:
            e['items'] = items_for(e['rule']); e['ages'] = ages(e['rule'])

    # aggregate item union per unique check across every region it appears in
    agg = {}
    for r in regions:
        for loc in r['locations']:
            agg.setdefault(loc['check'], set()).update(i['key'] for i in loc['items'])
    for check, md in locmeta.items():
        keys = sorted(agg.get(check, set()))
        md['items'] = [{'key': k, 'name': rg2name.get(k, prettify(k))} for k in keys]

    n_loc_refs = sum(len(r['locations']) for r in regions)
    n_exits = sum(len(r['exits']) for r in regions)
    n_events = sum(len(r['events']) for r in regions)
    data = {'regions': regions, 'items': items, 'locationMeta': locmeta,
            'stats': {'regions': len(regions), 'locationRefs': n_loc_refs,
                      'uniqueLocations': len(locmeta), 'exits': n_exits,
                      'events': n_events, 'items': len(items)}}

    out_json = os.path.join(HERE, 'rando_logic.json')
    json.dump(data, open(out_json, 'w', encoding='utf-8'), ensure_ascii=False, indent=1)
    print('Wrote', out_json)
    print('Stats:', json.dumps(data['stats']))

    write_html(data)

def write_html(data):
    payload = json.dumps(data, ensure_ascii=False)
    htmldoc = HTML_TEMPLATE.replace('/*__DATA__*/', payload)
    out = os.path.join(HERE, 'rando_logic.html')
    open(out, 'w', encoding='utf-8').write(htmldoc)
    print('Wrote', out)

HTML_TEMPLATE = r"""<!DOCTYPE html><html lang="en"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SoH Randomizer Logic</title>
<style>
:root{--bg:#14110e;--card:#1e1a15;--line:#3a3228;--ink:#ece4d6;--mut:#a89a84;--acc:#d9a441;--rule:#7fb3d5;--ok:#8bc34a}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--ink);font:14px/1.5 system-ui,Segoe UI,sans-serif}
header{position:sticky;top:0;background:var(--bg);border-bottom:1px solid var(--line);padding:10px 16px;z-index:5}
h1{font-size:16px;margin:0 0 8px;font-weight:600}
.stats{color:var(--mut);font-size:12px;margin-bottom:8px}
.controls{display:flex;gap:8px;flex-wrap:wrap;align-items:center}
input[type=search]{flex:1;min-width:200px;background:var(--card);border:1px solid var(--line);color:var(--ink);padding:7px 10px;border-radius:6px;font-size:13px}
.tabs button,.toggle{background:var(--card);border:1px solid var(--line);color:var(--ink);padding:6px 12px;border-radius:6px;cursor:pointer;font-size:13px}
.tabs button.on{background:var(--acc);color:#1a1206;border-color:var(--acc);font-weight:600}
.toggle.on{border-color:var(--ok);color:var(--ok)}
main{padding:12px 16px;max-width:1100px;margin:0 auto}
.region{border:1px solid var(--line);border-radius:8px;margin:8px 0;background:var(--card);overflow:hidden}
.region>summary{cursor:pointer;padding:9px 12px;font-weight:600;list-style:none;display:flex;justify-content:space-between;gap:10px}
.region>summary::-webkit-details-marker{display:none}
.region>summary:hover{background:#241f19}
.rkey{color:var(--mut);font-weight:400;font-size:11px}
.body{padding:4px 12px 12px}
.sec{margin-top:8px}
.sec h3{font-size:12px;color:var(--acc);text-transform:uppercase;letter-spacing:.04em;margin:8px 0 4px}
.row{display:grid;grid-template-columns:minmax(180px,300px) 1fr;gap:12px;padding:4px 0;border-top:1px solid #2a241d}
.row .nm{font-weight:500}
.row .nm small{display:block;color:var(--mut);font-weight:400;font-size:11px}
.rule{color:var(--rule);font-family:ui-monospace,Consolas,monospace;font-size:12px;white-space:pre-wrap;word-break:break-word}
.rule.t{color:var(--ok)}
.items{margin-top:5px;display:flex;flex-wrap:wrap;gap:4px;align-items:center}
.items .lbl{font-size:10px;color:var(--mut);text-transform:uppercase;letter-spacing:.04em;margin-right:2px}
.chip{font-size:11px;background:#2a2114;border:1px solid #4a3a1c;color:#f0d9a8;border-radius:10px;padding:1px 8px}
.age{font-size:10px;border-radius:10px;padding:1px 7px;border:1px solid var(--line);color:var(--mut)}
.age.child{color:#9ccc65;border-color:#4a5a2c}.age.adult{color:#90caf9;border-color:#2c485a}
.none{font-size:11px;color:var(--mut);font-style:italic}
.tag{font-size:10px;color:var(--mut);border:1px solid var(--line);border-radius:4px;padding:0 5px;margin-left:6px}
table{width:100%;border-collapse:collapse;font-size:13px}
th,td{text-align:left;padding:6px 8px;border-bottom:1px solid #2a241d;vertical-align:top}
th{color:var(--acc);position:sticky;top:0;background:var(--card)}
.hide{display:none}
.adv{color:var(--ok)}
code{color:var(--mut)}
</style></head><body>
<header>
<h1>Ship of Harkinian — Randomizer Logic <span class="rkey" id="st"></span></h1>
<div class="stats" id="stats"></div>
<div class="controls">
<div class="tabs">
<button data-v="regions" class="on">Regions</button>
<button data-v="locations">Locations</button>
<button data-v="items">Items</button>
</div>
<input type="search" id="q" placeholder="filter (name / key / rule)…">
<button class="toggle on" id="stdtog">checksanity off</button>
</div>
<div class="stats" style="margin-top:6px">Chips = every item the rule can reference (a <b>union</b>, resolved through the logic helpers). The rule's AND/OR structure decides which combination is actually needed — read the rule above the chips for the exact condition. Songs expand to ocarina + buttons; "true" = always reachable.</div>
</header>
<main id="out"></main>
<script>
const DATA=/*__DATA__*/;
const out=document.getElementById('out');
let view='regions',q='',stdOnly=true;
const esc=s=>(s||'').replace(/[&<>]/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;'}[c]));
document.getElementById('stats').textContent=
 `${DATA.stats.regions} regions · ${DATA.stats.uniqueLocations} locations (${DATA.stats.locationRefs} refs) · ${DATA.stats.exits} exits · ${DATA.stats.events} events · ${DATA.stats.items} items`;
document.querySelectorAll('.tabs button').forEach(b=>b.onclick=()=>{
 view=b.dataset.v;document.querySelectorAll('.tabs button').forEach(x=>x.classList.toggle('on',x===b));render();});
document.getElementById('q').oninput=e=>{q=e.target.value.toLowerCase();render();};
document.getElementById('stdtog').onclick=e=>{stdOnly=!stdOnly;e.target.classList.toggle('on',stdOnly);
 e.target.textContent=stdOnly?'checksanity off':'all checks';render();};
function ruleHtml(r){const t=r==='true';return `<span class="rule${t?' t':''}">${esc(r)}</span>`;}
function itemsHtml(items,ages){
 let h='<div class="items">';
 h+=(ages||[]).map(a=>`<span class="age ${a}">${a}</span>`).join('');
 if(items&&items.length){h+='<span class="lbl">needs one of</span>'+items.map(i=>`<span class="chip" title="${i.key}">${esc(i.name)}</span>`).join('');}
 else if(!ages||!ages.length){h+='<span class="none">no items required</span>';}
 h+='</div>';return h;
}
function itemBlob(items){return (items||[]).map(i=>i.name+' '+i.key).join(' ');}
function matches(s){return !q||s.toLowerCase().includes(q);}
function renderRegions(){
 let h='';
 for(const reg of DATA.regions){
  const locs=reg.locations.filter(l=>(!stdOnly||l.standard));
  const blob=(reg.name+' '+reg.key+' '+reg.exits.map(e=>e.to+' '+e.rule+' '+itemBlob(e.items)).join(' ')+' '+locs.map(l=>l.name+' '+l.check+' '+l.rule+' '+itemBlob(l.items)).join(' ')).toLowerCase();
  if(q&&!blob.includes(q))continue;
  const open=q?' open':'';
  h+=`<details class="region"${open}><summary><span>${esc(reg.name)} <span class="rkey">${reg.key}</span></span><span class="rkey">${locs.length} loc · ${reg.exits.length} exit</span></summary><div class="body">`;
  if(reg.events.length){h+=`<div class="sec"><h3>Events</h3>`;for(const e of reg.events)h+=`<div class="row"><div class="nm">${e.event}</div><div>${ruleHtml(e.rule)}${itemsHtml(e.items,e.ages)}</div></div>`;h+=`</div>`;}
  if(locs.length){h+=`<div class="sec"><h3>Locations</h3>`;for(const l of locs)h+=`<div class="row"><div class="nm">${esc(l.name)}<small>${l.check} <span class="tag">${l.type}</span></small></div><div>${ruleHtml(l.rule)}${itemsHtml(l.items,l.ages)}</div></div>`;h+=`</div>`;}
  if(reg.exits.length){h+=`<div class="sec"><h3>Exits</h3>`;for(const x of reg.exits)h+=`<div class="row"><div class="nm">→ ${x.to}</div><div>${ruleHtml(x.rule)}${itemsHtml(x.items,x.ages)}</div></div>`;h+=`</div>`;}
  h+=`</div></details>`;
 }
 out.innerHTML=h||'<p>No matches.</p>';
}
function renderLocations(){
 const rows=[];
 for(const [k,m] of Object.entries(DATA.locationMeta)){
  if(stdOnly&&!m.standard)continue;
  if(!matches(k+' '+m.name+' '+m.type+' '+m.vanilla+' '+itemBlob(m.items)))continue;
  const chips=(m.items||[]).map(i=>`<span class="chip" title="${i.key}">${esc(i.name)}</span>`).join(' ')||'<span class="none">none</span>';
  rows.push(`<tr><td>${esc(m.name)}<br><code>${k}</code></td><td>${m.type}</td><td>${m.vanilla}</td><td><div class="items">${chips}</div></td></tr>`);
 }
 out.innerHTML=`<table><thead><tr><th>Location</th><th>Type</th><th>Vanilla item</th><th>Items that can satisfy it (union)</th></tr></thead><tbody>${rows.join('')}</tbody></table>`;
}
function renderItems(){
 const rows=[];
 for(const it of DATA.items){
  if(!matches(it.key+' '+it.name+' '+it.type+' '+it.category))continue;
  rows.push(`<tr><td>${esc(it.name)}<br><code>${it.key}</code></td><td>${it.type}</td><td>${it.category}</td><td class="${it.advancement?'adv':''}">${it.advancement?'yes':''}</td></tr>`);
 }
 out.innerHTML=`<table><thead><tr><th>Item</th><th>Type</th><th>Category</th><th>Progression</th></tr></thead><tbody>${rows.join('')}</tbody></table>`;
}
function render(){document.getElementById('stdtog').style.display=view==='items'?'none':'';
 if(view==='regions')renderRegions();else if(view==='locations')renderLocations();else renderItems();}
render();
</script></body></html>"""

if __name__ == '__main__':
    main()
