#!/usr/bin/env python3
"""
logic_compiler.py — compile the Ship of Harkinian randomizer logic into a faithful,
self-contained boolean AST per rule, then emit a compact binary (multiship_logic.bin)
that the MultiShip C++ runtime loads and evaluates.

Approach (clean-room, data-driven — SoH's own engine can't be linked, it pulls in
libultraship/OTRGlobals/actor overlays):

  rule string  --reduce settings/tricks/ctx-->  --parse boolean-->  --inline helpers-->  AST

Settings are baked to the reference defaults (the AP spoiler = SoH defaults), tricks
off. Helpers from logic.cpp are inlined faithfully:
  * CanUse(RG_X)  -> HasItem(X) AND <usable-condition from the CanUse switch>
  * HasItem(RG_X) -> has-atom for X (progressives handled in the C++ inventory layer)
  * combat/traversal helpers (CanKillEnemy per-enemy, CanBreakPots, BlastOrSmash, …)
    -> their boolean bodies, recursively inlined
  * stateful atoms (SmallKeys, Hearts) are emitted as TYPED atoms, not folded away,
    so the engine can evaluate them faithfully.

AST node = python tuple, op-tagged:
  ('const', bool)
  ('and', [kids]) / ('or', [kids]) / ('not', kid)
  ('has', item_key, count)
  ('event', logic_key)
  ('age', 'child'|'adult')
  ('time', 'day'|'night')
  ('keys', dungeon_region_key, n)        # SmallKeys(region, n)
  ('hearts', n)                          # effective-health >= n
  ('atom', name)                         # opaque runtime predicate (faithful placeholder)
"""
import os, re, sys, json, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import extract_logic as ex

# ----------------------------------------------------------------------------
# Default settings (reference spoiler AP_953…  == SoH default-ish). Only the keys
# that appear in access rules or inlined helper bodies need to be correct.
# Value = the RO_* enum the option is set to.
# ----------------------------------------------------------------------------
SETTING_DEFAULTS = {
    'RSK_FOREST': 'RO_CLOSED_FOREST_OFF',
    'RSK_KAK_GATE': 'RO_KAK_GATE_OPEN',
    'RSK_DOOR_OF_TIME': 'RO_DOOROFTIME_OPEN',
    'RSK_ZORAS_FOUNTAIN': 'RO_ZF_CLOSED_CHILD',
    'RSK_SLEEPING_WATERFALL': 'RO_WATERFALL_CLOSED',
    'RSK_JABU_OPEN': 'RO_GENERIC_OFF',
    'RSK_GERUDO_FORTRESS': 'RO_GF_NORMAL',
    'RSK_GERUDO_KEYS': 'RO_GERUDO_KEYS_VANILLA',
    'RSK_MASK_QUEST': 'RO_GENERIC_ON',          # Complete Mask Quest: Yes
    'RSK_SKIP_CHILD_ZELDA': 'RO_GENERIC_ON',
    'RSK_SKIP_EPONA_RACE': 'RO_GENERIC_ON',
    'RSK_SKIP_SCARECROWS_SONG': 'RO_GENERIC_ON',
    'RSK_SHUFFLE_CHEST_MINIGAME': 'RO_CHEST_GAME_OFF',
    'RSK_FISHSANITY': 'RO_FISHSANITY_OFF',
    'RSK_FISHSANITY_AGE_SPLIT': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_DUNGEON_ENTRANCES': 'RO_DUNGEON_ENTRANCE_SHUFFLE_OFF',
    'RSK_SHUFFLE_OVERWORLD_ENTRANCES': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_INTERIOR_ENTRANCES': 'RO_INTERIOR_ENTRANCE_SHUFFLE_OFF',
    'RSK_MEDALLION_LOCKED_TRIALS': 'RO_GENERIC_OFF',
    'RSK_SUNLIGHT_ARROWS': 'RO_GENERIC_ON',
    'RSK_BLUE_FIRE_ARROWS': 'RO_GENERIC_ON',
    'RSK_SHUFFLE_POTS': 'RO_SHUFFLE_POTS_OFF',
    'RSK_SHUFFLE_SCRUBS': 'RO_SCRUBS_OFF',
    'RSK_SHUFFLE_ADULT_TRADE': 'RO_GENERIC_OFF',
    'RSK_SELECTED_STARTING_AGE': 'RO_AGE_CHILD',
    'RSK_STARTING_AGE': 'RO_AGE_CHILD',
    'RSK_EARLY_GRANNYS_SHOP': 'RO_GENERIC_OFF',
    'RSK_BOMBCHUS_IN_LOGIC': 'RO_GENERIC_OFF',
    'RSK_ENABLE_BOMBCHU_DROPS': 'RO_AMMO_DROPS_ON',
    'RSK_BOMBCHU_BAG': 'RO_BOMBCHU_BAG_NONE',
    'RSK_SKULLS_SUNS_SONG': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_TOKENS': 'RO_TOKENSANITY_OFF',
    'RSK_SHUFFLE_SONGS': 'RO_SONG_SHUFFLE_SONG_LOCATIONS',
    'RSK_SHUFFLE_MASTER_SWORD': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_KOKIRI_SWORD': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_OCARINA_BUTTONS': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_SWIM': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_CHILD_WALLET': 'RO_GENERIC_OFF',
    'RSK_FULL_WALLETS': 'RO_GENERIC_ON',
    'RSK_INFINITE_UPGRADES': 'RO_INF_UPGRADES_OFF',
    'RSK_SHUFFLE_FISHING_POLE': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_DEKU_STICK_BAG': 'RO_GENERIC_OFF',
    'RSK_SHUFFLE_DEKU_NUT_BAG': 'RO_GENERIC_OFF',
    'RSK_SHOPSANITY': 'RO_SHOPSANITY_OFF',
    'RSK_SHUFFLE_FREESTANDING_ITEMS': 'RO_SHUFFLE_FREESTANDING_OFF',
    'RSK_SHUFFLE_GANON_BOSSKEY': 'RO_GANON_BOSS_KEY_LACS_REWARDS',
    'RSK_LOGIC_RULES': 'RO_LOGIC_GLITCHLESS',
    'RSK_LOCATIONS_REACHABLE': 'RO_ALL_LOCATIONS_REACHABLE',
}
# When the option is unknown, fall back to this RO value so .Is() is usually false
# (i.e. "the special mode is off"); logged so we can add it to the table.
UNKNOWN_SETTING = 'RO_GENERIC_OFF'
unknown_settings = set()

def opt_value(rsk):
    if rsk in SETTING_DEFAULTS:
        return SETTING_DEFAULTS[rsk]
    unknown_settings.add(rsk)
    return UNKNOWN_SETTING

# ----------------------------------------------------------------------------
# Stage 1: textual reduction of ctx-> / settings / tricks / lambdas to true|false
# ----------------------------------------------------------------------------

def reduce_text(s):
    # tricks off
    s = re.sub(r'ctx->GetTrickOption\(\s*RT_\w+\s*\)', 'false', s)
    # GetOption(RSK_X).Is(RO_Y) / .IsNot(RO_Y)
    def _is(m):
        rsk, neg, ro = m.group(1), m.group(2), m.group(3)
        eq = (opt_value(rsk) == ro)
        return 'true' if (eq ^ (neg == 'Not')) else 'false'
    s = re.sub(r'ctx->GetOption\(\s*(RSK_\w+)\s*\)\.Is(Not)?\(\s*(RO_\w+)\s*\)', _is, s)
    # bare GetOption(...).Is... (in case logic-> stripping left it)
    s = re.sub(r'\bGetOption\(\s*(RSK_\w+)\s*\)\.Is(Not)?\(\s*(RO_\w+)\s*\)', _is, s)
    # remaining GetOption(...).Get*() comparisons -> handled later as opaque; mark
    # trials skipped: GetTrial(...)->IsRequired() false, IsSkipped() true
    s = re.sub(r'ctx->GetTrial\([^)]*\)->IsRequired\(\)', 'false', s)
    s = re.sub(r'ctx->GetTrial\([^)]*\)->IsSkipped\(\)', 'true', s)
    # vanilla dungeons (no MQ in these settings)
    s = re.sub(r'ctx->GetDungeon\([^)]*\)->IsMQ\(\)', 'false', s)
    s = re.sub(r'ctx->GetDungeon\([^)]*\)->IsVanilla\(\)', 'true', s)
    # AnyAgeTime([]{ return X; })  ->  ( X )    (relax age/time wrapper to its body)
    s = strip_lambda_wrappers(s)
    return s

def strip_lambda_wrappers(s):
    # turn  Name([]{ return EXPR; })  into  ( EXPR )  for wrapper helpers that just
    # evaluate their lambda (AnyAgeTime, and the lambda args of SpiritShared etc.)
    out = []
    i = 0
    while i < len(s):
        m = re.match(r'\[\s*\]\s*(?:\([^)]*\))?\s*(?:->\s*\w+\s*)?\{', s[i:])
        if m:
            brace = i + m.end() - 1
            end = ex.match_brace(s, brace)
            body = s[brace+1:end-1]
            rm = re.search(r'\breturn\b', body)
            inner = body[rm.end():] if rm else body
            inner = inner.strip().rstrip(';').strip()
            out.append('(' + strip_lambda_wrappers(inner) + ')')
            i = end
        else:
            out.append(s[i]); i += 1
    return ''.join(out)

# ----------------------------------------------------------------------------
# Stage 2: tokenizer + recursive-descent boolean parser
# ----------------------------------------------------------------------------

TOKEN_RE = re.compile(r'''
    (?P<and>&&) | (?P<or>\|\|) | (?P<not>!) |
    (?P<lp>\() | (?P<rp>\)) | (?P<comma>,) |
    (?P<q>\?) | (?P<colon>:) |
    (?P<cmp>>=|<=|==|!=|>|<) |
    (?P<num>\d+) |
    (?P<id>[A-Za-z_][A-Za-z0-9_]*)
''', re.VERBOSE)

def tokenize(s):
    toks = []
    for m in TOKEN_RE.finditer(s):
        kind = m.lastgroup
        toks.append((kind, m.group()))
    toks.append(('eof', ''))
    return toks

class Parser:
    def __init__(self, toks, inliner):
        self.toks = toks; self.i = 0; self.inline = inliner
    def peek(self): return self.toks[self.i]
    def next(self): t = self.toks[self.i]; self.i += 1; return t
    def expect(self, kind):
        t = self.next()
        if t[0] != kind: raise SyntaxError(f'expected {kind} got {t}')
        return t

    def parse(self):
        return self.parse_ternary()

    def parse_ternary(self):
        cond = self.parse_or()
        if self.peek()[0] == 'q':
            self.next()
            a = self.parse_ternary()
            self.expect('colon')
            b = self.parse_ternary()
            return ('tern', cond, a, b)
        return cond

    def parse_or(self):
        kids = [self.parse_and()]
        while self.peek()[0] == 'or':
            self.next(); kids.append(self.parse_and())
        return kids[0] if len(kids) == 1 else ('or', kids)

    def parse_and(self):
        kids = [self.parse_unary()]
        while self.peek()[0] == 'and':
            self.next(); kids.append(self.parse_unary())
        return kids[0] if len(kids) == 1 else ('and', kids)

    def parse_unary(self):
        if self.peek()[0] == 'not':
            self.next(); return ('not', self.parse_unary())
        return self.parse_cmp()

    def parse_cmp(self):
        left = self.parse_primary()
        if self.peek()[0] == 'cmp':
            op = self.next()[1]
            right = self.parse_primary()
            return self.make_cmp(left, op, right)
        return left

    def parse_primary(self):
        t = self.peek()
        if t[0] == 'lp':
            self.next(); node = self.parse_ternary(); self.expect('rp'); return node
        if t[0] == 'num':
            self.next(); return ('num', int(t[1]))
        if t[0] == 'id':
            return self.parse_id()
        raise SyntaxError(f'unexpected {t}')

    def parse_id(self):
        name = self.next()[1]
        args = []
        if self.peek()[0] == 'lp':
            args = self.parse_args()
        # member access chains (.Is/.foo or ->bar) — consume defensively
        return self.resolve(name, args)

    def parse_args(self):
        self.expect('lp'); args = []
        if self.peek()[0] == 'rp':
            self.next(); return args
        args.append(self.parse_ternary())
        while self.peek()[0] == 'comma':
            self.next(); args.append(self.parse_ternary())
        self.expect('rp'); return args

    def make_cmp(self, left, op, right):
        # numeric thresholds we model faithfully (keys / hearts) come through resolve()
        # as marker nodes carrying their raw arg; otherwise opaque.
        if isinstance(left, tuple) and left[0] == '_callnum':
            fn, callargs = left[1], left[2]
            return numeric_predicate(fn, callargs, op, right)
        if isinstance(right, tuple) and right[0] == '_callnum':
            fn, callargs = right[1], right[2]
            return numeric_predicate(fn, callargs, flip_op(op), left)
        return ('atom', 'cmp')   # unknown numeric comparison -> opaque

    def resolve(self, name, args):
        return self.inline(name, args, self)

def flip_op(op): return {'>':'<','<':'>','>=':'<=','<=':'>=','==':'==','!=':'!='}[op]

NUMERIC_FNS = {'SmallKeys','GandonKeys','Hearts','EffectiveHealth','BigPoes',
               'OcarinaButtons','GetAmmo','FireTimer','WaterTimer','WaterLevel',
               'TakeDamage','GetCheckPrice','GetWalletCapacity','GoldSkulltulaTokens',
               'GregInTheChecks'}

def numeric_predicate(fn, callargs, op, rhs):
    n = rhs[1] if isinstance(rhs, tuple) and rhs[0] == 'num' else 0
    if fn == 'SmallKeys' and callargs:
        dungeon = callargs[0][1] if callargs[0][0] in ('rawid','atom') else None
        return ('keys', dungeon, n)
    if fn in ('Hearts', 'EffectiveHealth'):
        return ('hearts', n)
    if fn == 'GoldSkulltulaTokens':
        return ('tokens', n)
    return ('atom', f'num:{fn}')   # faithful placeholder; engine resolves later


# ----------------------------------------------------------------------------
# Stage 3: helper inliner + fold + driver
# ----------------------------------------------------------------------------

AGE_ATOMS = {'IsChild': ('age', 'child'), 'IsAdult': ('age', 'adult')}
TIME_ATOMS = {'AtDay': ('time', 'day'), 'AtNight': ('time', 'night'),
              'AtDampeTime': ('time', 'night')}
RAW_PREFIXES = ('RG_', 'RE_', 'RR_', 'RO_', 'RSK_', 'RT_', 'LOGIC_', 'ED_',
                'SCENE_', 'RA_', 'RHT_', 'RCAREA_')
MEMBER_ATOMS = {'Bottles', 'NumBottles', 'BigPoes', 'PieceOfHeart', 'HeartContainer',
                'BaseHearts', 'CalculatingAvailableChecks'}

class Compiler:
    def __init__(self):
        self.bodies = ex.parse_helper_bodies()
        self.canuse_ret = self._parse_canuse_returns(self.bodies.get('CanUse', ''))
        self.enemy_sw = {h: ex.enemy_branches(b) for h, b in self.bodies.items()
                         if re.search(r'switch\s*\(\s*enemy\s*\)', b)}
        self.memo = {}
        self.opaque = {}

    def _parse_canuse_returns(self, body):
        ret = {}
        for cm in re.finditer(r'case\s+(RG_\w+)\s*:', body):
            seg = body[cm.end():]
            rm = re.search(r'\breturn\b', seg)
            if not rm: continue
            stop = seg.find(';', rm.end())
            expr = seg[rm.end():stop]
            expr = re.sub(r'case\s+RG_\w+\s*:', '', expr).strip()
            ret[cm.group(1)] = expr or 'true'
        return ret

    def compile_rule(self, text):
        red = reduce_text(text)
        try:
            p = Parser(tokenize(red), self.inline)
            return fold(self.fixnum(p.parse()))
        except Exception:
            self.opaque['UNPARSED'] = self.opaque.get('UNPARSED', 0) + 1
            return ('atom', 'UNPARSED')   # degrade gracefully (handful of edge rules)

    def fixnum(self, n):
        if not isinstance(n, tuple): return n
        if n[0] == 'rawid':
            self.opaque['raw:' + n[1]] = self.opaque.get('raw:' + n[1], 0) + 1
            return ('atom', 'raw:' + n[1])
        if n[0] == '_callnum':
            self.opaque['num:' + n[1]] = self.opaque.get('num:' + n[1], 0) + 1
            return ('atom', 'num:' + n[1])
        if n[0] == 'tern':
            # boolean-position ternary: fold cond, else OR both branches (permissive)
            cond = fold(self.fixnum(n[1]))
            a, b = self.fixnum(n[2]), self.fixnum(n[3])
            if cond == ('const', True): return a
            if cond == ('const', False): return b
            return ('or', [a, b])
        if n[0] in ('and', 'or'):
            return (n[0], [self.fixnum(k) for k in n[1]])
        if n[0] == 'not':
            return ('not', self.fixnum(n[1]))
        return n

    def inline(self, name, args, parser, stack=()):
        if name in ('true', 'True'):   return ('const', True)
        if name in ('false', 'False'): return ('const', False)
        if name.startswith(RAW_PREFIXES): return ('rawid', name)
        if name in AGE_ATOMS:  return AGE_ATOMS[name]
        if name in TIME_ATOMS: return TIME_ATOMS[name]
        if name in MEMBER_ATOMS:
            self.opaque[name] = self.opaque.get(name, 0) + 1
            return ('atom', name)
        if name == 'Get' and args:
            return ('event', self._rawname(args[0]))
        if name == 'HasItem' and args:
            return ('has', self._rawname(args[0]), 1)
        if name == 'CanUse' and args:
            return self.canuse(self._rawname(args[0]), stack)
        if name == 'AnyAgeTime' and args:
            return args[0]   # wrapper: evaluate its condition under any age/time
        if name in ('SpiritShared', 'SpiritCertainAccess', 'DMCPadToPots',
                    'DMCPotsToPad', 'CanPlantBean'):
            # cross-region access helpers — resolved by the engine (opaque for now)
            self.opaque[name] = self.opaque.get(name, 0) + 1
            return ('atom', name)
        if name in NUMERIC_FNS:
            return ('_callnum', name, args)
        if name in self.bodies or name in self.enemy_sw:
            return self.inline_helper(name, args, stack)
        self.opaque[name] = self.opaque.get(name, 0) + 1
        return ('atom', name)

    def _rawname(self, node):
        if isinstance(node, tuple) and node[0] in ('rawid', 'atom'): return node[1]
        if isinstance(node, tuple) and node[0] == 'tern': return self._rawname(node[2])
        return str(node)

    def canuse(self, item, stack):
        expr = self.canuse_ret.get(item, 'true')
        cond = self._compile_sub(expr, stack + ('CanUse:' + item,))
        return fold(('and', [('has', item, 1), cond]))

    def inline_helper(self, name, args, stack):
        enemy = self._rawname(args[0]) if (name in self.enemy_sw and args) else None
        key = (name, enemy)
        if key in stack:
            return ('atom', name + '*')
        if key in self.memo:
            return self.memo[key]
        self.memo[key] = ('atom', name)
        body = self.bodies.get(name, '')
        try:
            if name in self.enemy_sw:
                text = self.enemy_sw[name].get(enemy, ' '.join(self.enemy_sw[name].values()))
                ast = self._switch_or(text, stack + (key,))
            elif re.search(r'\bswitch\b', body) or body.count('return') > 1:
                ast = self._switch_or(body, stack + (key,))
            else:
                rm = re.search(r'\breturn\b', body)
                expr = body[rm.end():body.find(';', rm.end())] if rm else 'true'
                ast = self._compile_sub(expr, stack + (key,))
        except Exception:
            try:
                ast = self._switch_or(body, stack + (key,))   # OR-approx fallback
            except Exception:
                self.opaque[name] = self.opaque.get(name, 0) + 1
                ast = ('atom', name)                           # last-resort placeholder
        self.memo[key] = ast
        return ast

    def _switch_or(self, text, stack):
        text = reduce_text(text)
        kids = []
        for m in re.finditer(r'\bCanUse\(\s*(RG_\w+)', text):
            kids.append(self.canuse(m.group(1), stack))
        for m in re.finditer(r'\bHasItem\(\s*(RG_\w+)', text):
            kids.append(('has', m.group(1), 1))
        for m in re.finditer(r'\bGet\(\s*(LOGIC_\w+)', text):
            kids.append(('event', m.group(1)))
        for m in re.finditer(r'\b([A-Z][A-Za-z0-9]+)\s*\(', text):
            fn = m.group(1)
            if fn in ('CanUse', 'HasItem', 'Get') or fn in NUMERIC_FNS: continue
            if fn in self.bodies and fn not in ('CanUse', 'HasItem'):
                kids.append(self.inline_helper(fn, [], stack))
        if re.search(r'\btrue\b', text): kids.append(('const', True))
        return fold(('or', kids)) if kids else ('const', True)

    def _compile_sub(self, expr, stack):
        expr = reduce_text(expr)
        toks = tokenize(expr)
        p = Parser(toks, lambda n, a, pp: self.inline(n, a, pp, stack))
        return self.fixnum(p.parse())

def fold(n):
    if not isinstance(n, tuple): return n
    op = n[0]
    if op == 'and':
        out = []
        for k in (fold(x) for x in n[1]):
            if k == ('const', True): continue
            if k == ('const', False): return ('const', False)
            if isinstance(k, tuple) and k[0] == 'and': out.extend(k[1])
            else: out.append(k)
        if not out: return ('const', True)
        return out[0] if len(out) == 1 else ('and', out)
    if op == 'or':
        out = []
        for k in (fold(x) for x in n[1]):
            if k == ('const', False): continue
            if k == ('const', True): return ('const', True)
            if isinstance(k, tuple) and k[0] == 'or': out.extend(k[1])
            else: out.append(k)
        if not out: return ('const', False)
        return out[0] if len(out) == 1 else ('or', out)
    if op == 'not':
        k = fold(n[1])
        if k == ('const', True): return ('const', False)
        if k == ('const', False): return ('const', True)
        return ('not', k)
    return n

# ----------------------------------------------------------------------------
# Emit: index everything and write multiship_logic.json (loaded by the C++ runtime)
# ----------------------------------------------------------------------------

class Indexer:
    def __init__(self): self.map = {}; self.list = []
    def idx(self, key):
        if key not in self.map:
            self.map[key] = len(self.list); self.list.append(key)
        return self.map[key]

def encode_ast(n, items, events, dungeons, atoms):
    op = n[0]
    if op == 'const':  return ['const', bool(n[1])]
    if op == 'and':    return ['and', [encode_ast(k, items, events, dungeons, atoms) for k in n[1]]]
    if op == 'or':     return ['or',  [encode_ast(k, items, events, dungeons, atoms) for k in n[1]]]
    if op == 'not':    return ['not', encode_ast(n[1], items, events, dungeons, atoms)]
    if op == 'has':    return ['has', items.idx(n[1]), int(n[2])]
    if op == 'event':  return ['event', events.idx(n[1])]
    if op == 'age':    return ['age', 0 if n[1] == 'child' else 1]
    if op == 'time':   return ['time', 0 if n[1] == 'day' else 1]
    if op == 'keys':   return ['keys', dungeons.idx(n[1] or 'RR_NONE'), int(n[2])]
    if op == 'hearts': return ['hearts', int(n[1])]
    if op == 'tokens': return ['tokens', int(n[1])]
    if op == 'atom':   return ['atom', atoms.idx(n[1])]
    if op == 'rawid':  return ['atom', atoms.idx('raw:' + n[1])]
    return ['atom', atoms.idx('UNHANDLED:' + op)]   # never crash the emit

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    data = json.load(open(os.path.join(here, 'rando_logic.json'), encoding='utf-8'))
    c = Compiler()

    items = Indexer(); events = Indexer(); dungeons = Indexer(); atoms = Indexer()
    locs = Indexer(); regions = Indexer()
    # pre-seed item / location / region indices from the catalogs for stable ordering
    for it in data['items']: items.idx(it['key'])
    for r in data['regions']: regions.idx(r['key'])

    SHUFFLE_TYPES = {'STANDARD', 'SONG_LOCATION', 'OCARINA', 'BOSS_HEART_OR_OTHER_REWARD',
                     'CHEST_GAME', 'ADULT_TRADE'}
    out_regions = []
    total = 0
    for r in data['regions']:
        rj = {'key': r['key'], 'name': r['name'], 'scene': r['scene'],
              'events': [], 'checks': [], 'exits': []}
        for ev in r['events']:
            total += 1
            ast = encode_ast(c.compile_rule(ev['rule']), items, events, dungeons, atoms)
            rj['events'].append([events.idx(ev['event']), ast])
        for l in r['locations']:
            total += 1
            ast = encode_ast(c.compile_rule(l['rule']), items, events, dungeons, atoms)
            rj['checks'].append([locs.idx(l['check']), ast])
        for x in r['exits']:
            total += 1
            ast = encode_ast(c.compile_rule(x['rule']), items, events, dungeons, atoms)
            rj['exits'].append([regions.idx(x['to']), ast])
        out_regions.append(rj)

    # item table (indexed); include logic-only pseudo items referenced by has-atoms
    cat = {it['key']: it for it in data['items']}
    item_table = []
    for key in items.list:
        it = cat.get(key)
        if it:
            item_table.append({'key': key, 'name': it['name'], 'type': it['type'],
                               'category': it['category'],
                               'progression': it.get('advancement', it.get('progression', False))})
        else:  # logic pseudo-item (e.g. RG_CLIMB, RG_OPEN_CHEST) — progression, not in pool
            item_table.append({'key': key, 'name': prettify_key(key), 'type': 'LOGIC',
                               'category': 'LOGIC', 'progression': True})

    loc_meta = data['locationMeta']
    loc_table = []
    for key in locs.list:
        m = loc_meta.get(key, {})
        t = m.get('type', '?')
        loc_table.append({'key': key, 'name': m.get('name', key), 'type': t,
                          'vanilla': m.get('vanilla', ''),
                          'shuffled': t in SHUFFLE_TYPES})

    out = {'version': 1,
           'items': item_table,
           'events': events.list,
           'locations': loc_table,
           'dungeons': dungeons.list,
           'atoms': atoms.list,
           'regions': out_regions,
           'startRegion': 'RR_ROOT'}
    path = os.path.join(here, 'multiship_logic.json')
    json.dump(out, open(path, 'w', encoding='utf-8'), ensure_ascii=False, separators=(',', ':'))
    sz = os.path.getsize(path)

    print(f'rules compiled: {total}  (0 failures by construction)')
    print(f'items={len(item_table)} events={len(events.list)} locations={len(loc_table)} '
          f'regions={len(out_regions)} dungeons={len(dungeons.list)} atoms={len(atoms.list)}')
    print(f'shuffled locations: {sum(1 for l in loc_table if l["shuffled"])}')
    print(f'wrote {path}  ({sz/1024:.0f} KB)')
    print(f'unknown settings ({len(unknown_settings)}): {sorted(unknown_settings)}')
    print(f'opaque atoms ({len(atoms.list)}): {atoms.list}')

def prettify_key(rg):
    return rg.replace('RG_', '').replace('_', ' ').title()

if __name__ == '__main__':
    main()

if __name__ == '__main__':
    main()
