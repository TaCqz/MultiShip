#!/usr/bin/env python3
"""Extract ALL randomizer settings from SoH's Settings::CreateOptions() into a
generated C++ table (SettingsSpecData.h) used by the MultiShip settings UI/engine.

Source of truth = ShipwreckCombo/soh/.../settings.cpp (the OPT_U8 / OPT_BOOL calls)
and RandomizerOptions.h (RO_* enum values, for resolving non-zero defaults).

Each setting becomes a SettingSpecData row: { RSK key, label, category, choices, default }.
- OPT_U8 with a {"a","b",...} list  -> combo, choices in RO-value order (value == index).
- OPT_U8 with NumOpts(min,max)       -> numeric slider [min,max].
- OPT_BOOL                           -> combo {"Off","On"} (or explicit list).
The unique RO_* token (or bare int / true|false) in a call is its default value.
OptionCategory::Toggle settings are skipped (they only drive other options' UI).
"""
import re, sys, os

HERE = os.path.dirname(os.path.abspath(__file__))
SOH = os.path.normpath(os.path.join(HERE, "..", "..", "ShipwreckCombo", "soh", "soh", "Enhancements", "randomizer"))
SETTINGS_CPP = os.path.join(SOH, "settings.cpp")
DESC_CPP = os.path.join(SOH, "option_descriptions.cpp")
RO_H = os.path.join(SOH, "randomizerEnums", "RandomizerOptions.h")
OUT = os.path.normpath(os.path.join(HERE, "..", "src", "rando", "SettingsSpecData.h"))


def parse_descriptions(path):
    """RSK_NAME -> tooltip text, from mOptionDescriptions[RSK_X] = "..." "...";

    The RHS is one or more adjacent C++ string literals. We keep each literal's
    INNER text verbatim (escapes like \\n and \\" intact) and concatenate them, so
    the result can be emitted straight back into a C++ string literal.
    """
    if not os.path.exists(path):
        return {}
    text = open(path, encoding="utf-8", errors="ignore").read()
    out = {}
    for m in re.finditer(r'mOptionDescriptions\[(RSK_\w+)\]\s*=', text):
        end = text.find(';', m.end())
        if end < 0:
            continue
        block = text[m.end():end]
        parts = re.findall(r'"((?:[^"\\]|\\.)*)"', block)
        out[m.group(1)] = "".join(parts)
    return out


def ro_values(path):
    """RO_NAME -> numeric value (each RANDO_ENUM_BEGIN group restarts at 0)."""
    vals, idx = {}, 0
    for line in open(path, encoding="utf-8", errors="ignore"):
        if "RANDO_ENUM_BEGIN" in line:
            idx = 0
            continue
        m = re.search(r"RANDO_ENUM_ITEM\((RO_\w+)\)", line)
        if m:
            vals[m.group(1)] = idx
            idx += 1
    return vals


def balanced_calls(text, macro):
    """Yield the argument text inside each `macro(...)` (balanced parens)."""
    out, i = [], 0
    needle = macro + "("
    while True:
        j = text.find(needle, i)
        if j < 0:
            break
        k = j + len(needle)
        depth, start = 1, k
        while k < len(text) and depth:
            c = text[k]
            if c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
            k += 1
        out.append(text[start:k - 1])
        i = k
    return out


def split_args(s):
    """Split top-level commas, respecting (), {}, [], and string/char literals."""
    args, depth, cur, instr, ch = [], 0, "", False, ""
    i = 0
    while i < len(s):
        c = s[i]
        if instr:
            cur += c
            if c == "\\":
                cur += s[i + 1]
                i += 2
                continue
            if c == ch:
                instr = False
        elif c in '"\'':
            instr, ch, cur = True, c, cur + c
        elif c in "({[":
            depth += 1; cur += c
        elif c in ")}]":
            depth -= 1; cur += c
        elif c == "," and depth == 0:
            args.append(cur.strip()); cur = ""
        else:
            cur += c
        i += 1
    if cur.strip():
        args.append(cur.strip())
    return args


def strings_in(s):
    return re.findall(r'"((?:[^"\\]|\\.)*)"', s)


def main():
    rovals = ro_values(RO_H)
    text = open(SETTINGS_CPP, encoding="utf-8", errors="ignore").read()
    descs = parse_descriptions(DESC_CPP)

    rows, skipped = [], []

    # Numeric (NumOpts slider) settings the clean-room engine actually honors and that the
    # Create-seed UI should expose. The rest of the dynamic-count sliders (Triforce Hunt, MQ
    # dungeon count, shopsanity counts/prices, key-ring counts, ...) gate pools the engine
    # canonicalizes at generation time, so exposing them would only mislead — keep skipping
    # them. The bridge/LACS counts, by contrast, are real end-game gates evaluated by
    # Logic::CanBuildRainbowBridge / CanTriggerLACS, so they must reach the UI as overrides.
    NUMERIC_SLIDER_ALLOW = {
        "RSK_RAINBOW_BRIDGE_STONE_COUNT", "RSK_RAINBOW_BRIDGE_MEDALLION_COUNT",
        "RSK_RAINBOW_BRIDGE_REWARD_COUNT", "RSK_RAINBOW_BRIDGE_DUNGEON_COUNT",
        "RSK_RAINBOW_BRIDGE_TOKEN_COUNT",
        "RSK_LACS_STONE_COUNT", "RSK_LACS_MEDALLION_COUNT", "RSK_LACS_REWARD_COUNT",
        "RSK_LACS_DUNGEON_COUNT", "RSK_LACS_TOKEN_COUNT",
    }

    def resolve_default(args, kind):
        # The default is the unique RO_* token, else a bare int, else true/false, else 0.
        for a in args:
            m = re.fullmatch(r"(RO_\w+)", a)
            if m and m.group(1) in rovals:
                return rovals[m.group(1)]
        for a in args:
            if re.fullmatch(r"\d+", a):
                return int(a)
            if a == "true":
                return 1
            if a == "false":
                return 0
        return 0

    def parse_options(opt_arg):
        opt_arg = opt_arg.strip()
        # NumOpts(min,max,...) is the slider form; in settings.cpp it's wrapped in braces
        # ({NumOpts(0, 4)}), so search rather than match-at-start, and check it before the
        # brace/combo branch (a combo list never contains a NumOpts call).
        m = re.search(r"NumOpts\(\s*(\d+)\s*,\s*(\d+)", opt_arg)
        if m:
            return ("numeric", None, int(m.group(1)), int(m.group(2)))
        if opt_arg.startswith("{"):
            labels = strings_in(opt_arg)
            return ("combo", labels, 0, 0) if labels else (None, None, 0, 0)
        return (None, None, 0, 0)  # variable/precomputed list -> can't extract

    for body in balanced_calls(text, "OPT_U8"):
        args = split_args(body)
        if len(args) < 3:
            continue
        key = args[0]
        if not key.startswith("RSK_"):
            continue
        labels0 = strings_in(args[1])
        name = labels0[0] if labels0 else key
        if "OptionCategory::Toggle" in body:
            skipped.append(key + " (toggle)")
            continue
        kind, choices, lo, hi = parse_options(args[2])
        if kind is None:
            skipped.append(key + " (dynamic options)")
            continue
        if kind == "numeric" and key not in NUMERIC_SLIDER_ALLOW:
            skipped.append(key + " (numeric slider, not modeled)")
            continue
        default = resolve_default(args[3:], kind)
        rows.append((key, name, kind, choices, lo, hi, default))

    for body in balanced_calls(text, "OPT_BOOL"):
        args = split_args(body)
        if len(args) < 2:
            continue
        key = args[0]
        if not key.startswith("RSK_"):
            continue
        labels0 = strings_in(args[1])
        name = labels0[0] if labels0 else key
        if "OptionCategory::Toggle" in body:
            skipped.append(key + " (toggle)")
            continue
        # Form A: 3rd arg is an explicit {"..."} option list; else implicit {Off,On}.
        choices = ["Off", "On"]
        if len(args) >= 3 and args[2].strip().startswith("{"):
            ex = strings_in(args[2])
            if ex:
                choices = ex
        default = resolve_default(args[2:], "bool")
        rows.append((key, name, "combo", choices, 0, 1, default))

    # de-dupe by key (keep first), keep source order
    seen, uniq = set(), []
    for r in rows:
        if r[0] in seen:
            continue
        seen.add(r[0]); uniq.append(r)

    # Pool-changing settings the clean-room engine doesn't generate yet are forced to a
    # vanilla default so the Create-seed UI shows what actually happens (Fill also
    # normalizes these at generation time). Drop an entry once the engine pools it.
    # All dungeon-item settings (keys / boss keys / map&compass / dungeon rewards / Ganon's
    # boss key) are now generated by the engine's dungeon-restricted fill (FEAT-5), so none
    # are force-overridden here anymore. Fill still folds the few sub-modes it doesn't model
    # (Start-With -> Vanilla, Ganon-BK LACS/100GS -> Own Dungeon) at generation time.
    DEFAULT_OVERRIDE = {}
    uniq = [(k, n, kd, c, lo, hi, DEFAULT_OVERRIDE.get(k, df)) for (k, n, kd, c, lo, hi, df) in uniq]

    # MultiShip-local display tweaks: rename a setting and/or rename+reorder its choices.
    # The (value, label) pairs keep the underlying RO_ value intact while letting us show
    # clearer wording/order than SoH's (the combo selects by value, not list position).
    NAME_OVERRIDE = {
        'RSK_FOREST': 'Kokiri Forest',
    }
    CHOICE_OVERRIDE = {
        'RSK_FOREST': [(2, 'Open'), (0, 'Closed'), (1, 'Deku only')],
    }
    # Keep the (verbatim SoH) tooltip wording consistent with renamed choices.
    TOOLTIP_REPLACE = {
        'RSK_FOREST': [("On - ", "Closed - "), ("Deku Only - ", "Deku only - "), ("Off - ", "Open - ")],
    }
    uniq = [(k, NAME_OVERRIDE.get(k, n), kd, c, lo, hi, df) for (k, n, kd, c, lo, hi, df) in uniq]

    # --- Tab assignment: flatten SoH's OptionGroup sidebars (the rando menu tabs) ---
    # mOptionGroups[RSG_X] = OptionGroup::SubGroup("Name", { &mOptions[RSK_*] | &mOptionGroups[RSG_*] ... }, TYPE)
    groups = {}  # RSG_X -> (display_name, [members]) ; members 'RSK_*' or '@RSG_*'
    for mm in re.finditer(r'mOptionGroups\[(RSG_\w+)\]\s*=\s*OptionGroup::SubGroup\(', text):
        rsg = mm.group(1)
        k, depth = mm.end(), 1
        start = k
        while k < len(text) and depth:
            if text[k] == '(':
                depth += 1
            elif text[k] == ')':
                depth -= 1
            k += 1
        body = text[start:k - 1]
        names = strings_in(body)
        members = re.findall(r'&mOptions\[(RSK_\w+)\]', body)
        members += ['@' + x for x in re.findall(r'&mOptionGroups\[(RSG_\w+)\]', body)]
        groups[rsg] = (names[0] if names else '', members)

    # Walk each sidebar (tab) -> columns -> sections -> settings, recording per RSK its
    # (tab, column index, section header) and a global walk order so the UI can lay it
    # out in columns with section dividers exactly like the base randomizer.
    rsk_meta, rank = {}, [0]

    def walk_section(grp, tab, col, section, seen):
        if grp in seen:
            return
        seen.add(grp)
        for mem in groups.get(grp, ('', []))[1]:
            if mem.startswith('@'):
                sub = mem[1:]
                subname = groups.get(sub, ('', []))[0]
                walk_section(sub, tab, col, subname or section, seen)
            elif mem not in rsk_meta:
                rsk_meta[mem] = (tab, col, section, rank[0])
                rank[0] += 1

    sidebars = [g for g in groups if g.startswith('RSG_MENU_SIDEBAR_')]
    tab_order = [groups[s][0] for s in sidebars] + ['Misc']
    for s in sidebars:
        tab, seen, col = groups[s][0], set(), 0
        for mem in groups[s][1]:
            if mem.startswith('@'):  # a column container
                walk_section(mem[1:], tab, col, '', seen)
                col += 1
            elif mem not in rsk_meta:
                rsk_meta[mem] = (tab, 0, '', rank[0])
                rank[0] += 1

    BIG = 10_000_000
    rows2 = []
    for idx, (k, n, kd, c, lo, hi, df) in enumerate(uniq):
        tab, col, sec, rk = rsk_meta.get(k, ('Misc', 0, '', BIG + idx))
        rows2.append((k, n, kd, c, lo, hi, df, tab, col, sec, rk))
    uniq = sorted(rows2, key=lambda r: r[10])  # walk order (ungrouped Misc trails)

    def cstr(s):
        return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'

    lines = [
        "// AUTO-GENERATED by rando-logic/gen_settings.py — do not edit by hand.",
        "// All randomizer settings extracted from SoH Settings::CreateOptions().",
        "#pragma once",
        "#include \"randomizerEnums.h\"",
        "#include <cstdint>",
        "#include <vector>",
        "",
        "namespace Rando {",
        "struct GenSettingChoice { uint8_t value; const char* label; };",
        "struct GenSettingData {",
        "    RandomizerSettingKey key;",
        "    const char* label;",
        "    const char* tab;         // settings menu tab (SoH sidebar), e.g. \"Shuffles\"",
        "    const char* section;     // section divider header within the column (may be \"\")",
        "    uint8_t column;          // column index within the tab",
        "    bool numeric;            // true => slider [numMin,numMax]; false => choices",
        "    uint8_t numMin, numMax;",
        "    uint8_t defaultValue;",
        "    const char* tooltip;     // hover description (from SoH option_descriptions.cpp; may be \"\")",
        "    std::vector<GenSettingChoice> choices;",
        "    RandomizerSettingKey visibleWhenKey;     // parent this setting depends on (RSK_NONE = always shown)",
        "    std::vector<uint8_t> visibleWhenValues;  // parent values that reveal it; empty => always shown",
        "};",
        "",
        "inline const std::vector<GenSettingData>& AllRandoSettings() {",
        "    static const std::vector<GenSettingData> data = {",
    ]
    # Conditional visibility, mirroring SoH's OPT_CALLBACK Hide()/Unhide() logic: each
    # count slider is only shown while its parent setting selects the matching mode (so e.g.
    # the Bridge Medallion Count is hidden unless Rainbow Bridge == Medallions). The combo
    # selection value equals the RO_* enum value (choices are emitted in RO-value order), so
    # the parent values resolve straight through rovals. Add an entry as more dependent
    # settings get exposed.
    VISIBLE_WHEN = {
        "RSK_RAINBOW_BRIDGE_STONE_COUNT":     ("RSK_RAINBOW_BRIDGE", ["RO_BRIDGE_STONES"]),
        "RSK_RAINBOW_BRIDGE_MEDALLION_COUNT": ("RSK_RAINBOW_BRIDGE", ["RO_BRIDGE_MEDALLIONS"]),
        "RSK_RAINBOW_BRIDGE_REWARD_COUNT":    ("RSK_RAINBOW_BRIDGE", ["RO_BRIDGE_DUNGEON_REWARDS"]),
        "RSK_RAINBOW_BRIDGE_DUNGEON_COUNT":   ("RSK_RAINBOW_BRIDGE", ["RO_BRIDGE_DUNGEONS"]),
        "RSK_RAINBOW_BRIDGE_TOKEN_COUNT":     ("RSK_RAINBOW_BRIDGE", ["RO_BRIDGE_TOKENS"]),
        "RSK_LACS_STONE_COUNT":     ("RSK_GANONS_BOSS_KEY", ["RO_GANON_BOSS_KEY_LACS_STONES"]),
        "RSK_LACS_MEDALLION_COUNT": ("RSK_GANONS_BOSS_KEY", ["RO_GANON_BOSS_KEY_LACS_MEDALLIONS"]),
        "RSK_LACS_REWARD_COUNT":    ("RSK_GANONS_BOSS_KEY", ["RO_GANON_BOSS_KEY_LACS_REWARDS"]),
        "RSK_LACS_DUNGEON_COUNT":   ("RSK_GANONS_BOSS_KEY", ["RO_GANON_BOSS_KEY_LACS_DUNGEONS"]),
        "RSK_LACS_TOKEN_COUNT":     ("RSK_GANONS_BOSS_KEY", ["RO_GANON_BOSS_KEY_LACS_TOKENS"]),
    }

    def vis_fields(key):
        dep = VISIBLE_WHEN.get(key)
        if dep:
            parent, ro_names = dep
            vals = [rovals[r] for r in ro_names if r in rovals]
            if vals:
                return f"{parent}, {{ {', '.join(str(v) for v in vals)} }}"
        return "RSK_NONE, {}"

    for key, name, kind, choices, lo, hi, default, tab, col, sec, _rk in uniq:
        head = f"{{ {key}, {cstr(name)}, {cstr(tab)}, {cstr(sec)}, {col},"
        tipraw = descs.get(key, "")  # inner escapes already preserved
        for find, repl in TOOLTIP_REPLACE.get(key, []):
            tipraw = tipraw.replace(find, repl)
        tip = '"' + tipraw + '"'
        vis = vis_fields(key)
        if kind == "numeric":
            lines.append(f"        {head} true, {lo}, {hi}, {default}, {tip}, {{}}, {vis} }},")
        elif key in CHOICE_OVERRIDE:
            pairs = CHOICE_OVERRIDE[key]
            ch = ", ".join(f"{{ {val}, {cstr(lbl)} }}" for val, lbl in pairs)
            dflt = default if any(val == default for val, _ in pairs) else pairs[0][0]
            lines.append(f"        {head} false, 0, 0, {dflt}, {tip}, {{ {ch} }}, {vis} }},")
        else:
            ch = ", ".join(f"{{ {i}, {cstr(lbl)} }}" for i, lbl in enumerate(choices))
            dflt = default if default < len(choices) else 0
            lines.append(f"        {head} false, 0, 0, {dflt}, {tip}, {{ {ch} }}, {vis} }},")
    lines += ["    };", "    return data;", "}", ""]
    # Tab display order (SoH sidebar order, then Misc for anything ungrouped).
    used = {r[7] for r in uniq}  # r[7] = tab
    ordered = [t for t in tab_order if t in used]
    lines.append("inline const std::vector<const char*>& SettingTabOrder() {")
    lines.append("    static const std::vector<const char*> tabs = { " +
                 ", ".join(cstr(t) for t in ordered) + " };")
    lines += ["    return tabs;", "}", "", "} // namespace Rando", ""]

    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    open(OUT, "w", encoding="utf-8").write("\n".join(lines))
    print(f"Wrote {len(uniq)} settings -> {OUT}")
    print(f"Skipped {len(skipped)}: {', '.join(skipped[:12])}{' ...' if len(skipped) > 12 else ''}")


if __name__ == "__main__":
    main()
