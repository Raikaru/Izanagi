#!/usr/bin/env python3
"""Generates p4pc.fusion-arts' forged-weapon catalogue (docs: local design §2.1).

Inputs are the user's own tables, read only to choose anchors; the outputs carry
item IDs, our own property codes and our own names, never retail bytes:

  --items    the disc's ITEMTBL rows (68 bytes each, 1536 rows; for example the
             randomizer's private ITEMS bank)
  --personas the disc's PERSONA0 rows (14 bytes each: arcana +2, base level +3)

Outputs:
  --out/forge.xml  derived item rows (C3 `derive`, docs/modding/tables.md "Derived
                   items") plus name and description strings
  --script         main.luau: its generated catalogue block (the forged rows and
                   every retail weapon ID per character, for the vessel lists)

Every row derives from an anchor: the character's second-best shop weapon that
the band's shop stage has unlocked (attack, hit, critical byte, equip mask and
model and native weapon-family icon come from the user's own row at seal).
Bands follow the consumed Persona's level. Shop stages are the ITEMTBL
availability byte (+0x38; 9 marks items no shop sells). The stage per band is provisional (F5 retunes it).
"""
import argparse
import struct
from pathlib import Path
from xml.sax.saxutils import quoteattr

ITEM_ROW = 68
PERSONA_ROW = 14
FIRST_ID = 2048          # tables.md: 2048-3327 are reserved for p4pc.fusion-arts
LAST_ID = 3327

# Playable combatants (party IDs) and their weapon family nouns (design §2.1).
CHARACTERS = [(1, "Yu", "Blade"), (2, "Yosuke", "Kunai"), (3, "Chie", "Boots"), (4, "Yukiko", "Fan"),
              (6, "Kanji", "Guard"), (7, "Naoto", "Gun"), (8, "Teddie", "Claws")]
# Physical Split family leaves by character (physsplit-design §4A; elements 19 Slash, 20 Strike, 21 Pierce).
LEAF = {1: 19, 2: 19, 8: 19, 3: 20, 6: 20, 4: 21, 7: 21}
# Persona level bands B1..B6 and the latest shop stage each band can reach.
BANDS = [(1, 14, 1), (15, 26, 3), (27, 40, 4), (41, 54, 5), (55, 69, 6), (70, 99, 8)]
ROMAN = ["I", "II", "III", "IV", "V", "VI"]

# Property word codes (datItemValue 0x001069D0, design §3.3 and the F0 rider catalogue).
HP = (0x1A, 0x1C, 0x1E)      # Max HP +10 / +30 / +50
HP100 = 0x5B
SP = (0x1F, 0x21, 0x23)      # Max SP +10 / +30 / +50
SP100 = 0x8A
CRIT = (0x3C, 0x3D, 0x3E)
PHYS = (0x5F, 0x60, 0x61)
FIRE, ICE, WIND, ELEC = (0x62, 0x63, 0x64), (0x65, 0x66, 0x67), (0x68, 0x69, 0x6A), (0x6B, 0x6C, 0x6D)
MAGIC = (0x6E, 0x6F, 0x70)
# On-hit riders: first code of each 10/20/30% triple (skill rows 0x190.., status bit).
RIDER = {"Panic": 0x24, "Poison": 0x27, "Fear": 0x2A, "Rage": 0x2D, "Dizzy": 0x30,
         "Exhaustion": 0x33, "Enervation": 0x36, "Silence": 0x39}

# Arcana 2..21 (datPersonaGetArcana numbering): short name and standalone property.
ARCANA = {
    2: ("Magus", "fire"), 3: ("Oracle", "sp"), 4: ("Queen", "ice"), 5: ("King", "elec"),
    6: ("Abbot", "magic2"), 7: ("Amour", "Panic"), 8: ("Rider", "crit"), 9: ("Just", "hp"),
    10: ("Hermit", "Poison"), 11: ("Wheel", "crit"), 12: ("Might", "might"), 13: ("Hanged", "Exhaustion"),
    14: ("Reaper", "Fear"), 15: ("Temper", "magic"), 16: ("Devil", "Rage"), 17: ("Tower", "phys"),
    18: ("Star", "wind"), 19: ("Moon", "Silence"), 20: ("Sun", "hp"), 21: ("Judge", "Enervation"),
}
ARCANA_NAMES = {2: "Magician", 3: "Priestess", 4: "Empress", 5: "Emperor", 6: "Hierophant", 7: "Lovers",
                8: "Chariot", 9: "Justice", 10: "Hermit", 11: "Fortune", 12: "Strength", 13: "Hanged Man",
                14: "Death", 15: "Temperance", 16: "Devil", 17: "Tower", 18: "Star", 19: "Moon", 20: "Sun",
                21: "Judgement"}

# Fool Personas make Wild weapons: about 35% attack, maximum hit, one fixed tier-3 bonus.
WILD = [(25, "Obariyon", HP100, "Max HP +100"), (129, "Legion", SP100, "Max SP +100"),
        (12, "Ose", CRIT[2], "Critical rate up (strong)"), (119, "Black Frost", ICE[2], "Ice damage down (strong)"),
        (65, "Decarabia", MAGIC[2], "Magic damage down (strong)"), (4, "Loki", RIDER["Panic"] + 2, "May cause Panic (30%)"),
        (67, "Yomotsu", RIDER["Fear"] + 2, "May cause Fear (30%)"), (106, "Shiki", RIDER["Silence"] + 2, "May cause Silence (30%)")]
MAX_HIT = 99
BEGIN_MARK = "-- BEGIN GENERATED CATALOG (tools/mods/fusion_arts_forge.py)"
END_MARK = "-- END GENERATED CATALOG"

# Signatures: (persona, character, name, anchor rule, value, element, extra hit, description).
# Anchor rules: ("band", b) second-best of band b; ("best", b) best of band b; ("ratio", b, r) the shop
# weapon whose attack is closest to r x band b's anchor.
SIGNATURES = [
    (63, 1, "Smith-God Blade", ("best", 1), HP[1], 0, False, "Forged by the smith god. Max HP +30."),
    (38, 1, "Ever-Keen Blade", ("band", 5), CRIT[2], 0, False, "Never dulls. Critical rate up (strong)."),
    (175, 2, "Tengu Gale Kunai", ("ratio", 3, 0.75), 0, 4, False, "Strikes with Wind."),
    (147, 2, "Hidden-Oni Kunai", ("band", 5), CRIT[2], 0, False, "Unseen edges. Critical rate up (strong)."),
    (81, 3, "Ape-Lord Boots", ("band", 3), CRIT[2], 0, False, "Leaping kicks. Critical rate up (strong)."),
    (138, 3, "Iron-Oni Boots", ("band", 4), PHYS[1], 0, False, "Iron shell. Physical damage down."),
    (100, 4, "Vermilion Fan", ("ratio", 3, 0.75), 0, 1, False, "Strikes with Fire."),
    (17, 4, "Mediator's Fan", ("band", 3), SP100, 0, False, "Binds what is torn. Max SP +100."),
    (154, 6, "Oni Iron Guard", ("band", 2), CRIT[1], 0, False, "An ogre's weight. Critical rate up."),
    (77, 6, "Storm Maul", ("ratio", 4, 0.75), 0, 3, False, "Strikes with Elec."),
    (136, 8, "Man-Lion Claws", ("band", 3), CRIT[2], 0, False, "Tears through. Critical rate up (strong)."),
    (99, 8, "White-Tiger Claw", ("ratio", 4, 0.75), 0, 2, False, "Strikes with Ice."),
    (10, 7, "War-God Pistol", ("band", 5), 0, 0, True, "Blessed aim. Hit rate maximum."),
    (28, 7, "Never-Miss Gun", ("band", 5), CRIT[2], 0, True, "Never misses. Hit maximum, critical up."),
]

PROPERTY_TEXT = {"hp": "Max HP +{}", "sp": "Max SP +{}", "crit": "Critical rate up{}", "phys": "Physical damage down{}",
                 "fire": "Fire damage down{}", "ice": "Ice damage down{}", "elec": "Elec damage down{}",
                 "wind": "Wind damage down{}", "magic": "Magic damage down{}", "magic2": "Magic damage down{}"}
STRENGTH = ["", " (medium)", " (strong)"]


def property_for(kind, band):
    """(code, description) of the arcana's standalone property at a band (0-based)."""
    tier = 0 if band < 2 else 1 if band < 4 else 2
    if kind == "might":
        return (HP100, "Max HP +100") if band == 5 else (HP[tier], f"Max HP +{(10, 30, 50)[tier]}")
    if kind == "hp":
        return HP[tier], f"Max HP +{(10, 30, 50)[tier]}"
    if kind == "sp":
        return SP[tier], f"Max SP +{(10, 30, 50)[tier]}"
    if kind in RIDER:
        return RIDER[kind] + tier, f"May cause {kind} ({(10, 20, 30)[tier]}%)"
    if kind == "magic2":
        tier = min(tier, 1)        # Hierophant: tiers 1-2 only
    table = {"crit": CRIT, "phys": PHYS, "fire": FIRE, "ice": ICE, "elec": ELEC, "wind": WIND,
             "magic": MAGIC, "magic2": MAGIC}[kind]
    return table[tier], PROPERTY_TEXT[kind].format(STRENGTH[tier])


def read_weapons(items):
    """Shop weapons per character: (id, attack, stage)."""
    if len(items) % ITEM_ROW or len(items) // ITEM_ROW < 256:
        raise SystemExit("fusion_arts_forge: --items is not an ITEMTBL row bank")
    shop = {c: [] for c, _, _ in CHARACTERS}
    for item in range(1, 256):
        row = items[item * ITEM_ROW:(item + 1) * ITEM_ROW]
        mask = struct.unpack_from("<I", row, 0x30)[0]
        attack = struct.unpack_from("<H", row, 0x08)[0]
        price = struct.unpack_from("<I", row, 0x34)[0]
        stage = struct.unpack_from("<b", row, 0x38)[0]
        owners = [c for c, _, _ in CHARACTERS if (mask >> (c + 4)) & 1]
        if len(owners) == 1 and attack > 1 and price > 1 and 1 <= stage <= 8:
            shop[owners[0]].append((item, attack, stage))
    return shop


def band_anchor(weapons, band, best=False):
    limit = BANDS[band][2]
    reachable = sorted((w for w in weapons if w[2] <= limit), key=lambda w: (-w[1], w[0]))
    if not reachable:      # a late joiner: its earliest shop stage stands in
        first = min(w[2] for w in weapons)
        reachable = sorted((w for w in weapons if w[2] == first), key=lambda w: (-w[1], w[0]))
    return reachable[0] if best or len(reachable) == 1 else reachable[1]


def ratio_anchor(weapons, band, ratio):
    target = band_anchor(weapons, band)[1] * ratio
    return min(weapons, key=lambda w: (abs(w[1] - target), w[0]))


def band_of(level):
    return next(i for i, (low, high, _) in enumerate(BANDS) if low <= level <= high)


def encode(text, limit):
    if not text or len(text) > limit or any(not (0x20 <= ord(ch) < 0x7F) for ch in text):
        raise SystemExit(f"fusion_arts_forge: not printable text within {limit} characters: {text!r}")
    return text.encode("ascii").hex() + "00"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--items", type=Path, required=True)
    parser.add_argument("--personas", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--script", type=Path, required=True)
    args = parser.parse_args()
    shop = read_weapons(args.items.read_bytes())
    personas = args.personas.read_bytes()
    level = lambda persona: personas[persona * PERSONA_ROW + 3]
    rows = []          # (id, kind, character, band, key, anchor, value, element, hit, name, description)
    item = FIRST_ID
    for character, who, noun in CHARACTERS:
        for band in range(len(BANDS)):
            anchor = band_anchor(shop[character], band)[0]
            for arcana, (short, kind) in sorted(ARCANA.items()):
                value, text = property_for(kind, band)
                name = f"{short} {noun} {ROMAN[band]}"
                rows.append((item, "generic", character, band, arcana, anchor, value, 0, False, name,
                             f"Forged from a {ARCANA_NAMES[arcana]} Persona. {text}."))
                item += 1
    for character, who, noun in CHARACTERS:
        for persona, short, value, text in WILD:
            band = band_of(level(persona))
            anchor = ratio_anchor(shop[character], band, 0.35)[0]
            rows.append((item, "wild", character, band, persona, anchor, value, 0, True, f"{short} {noun}"[:16],
                         f"A Wild weapon. Hit maximum. {text}."))
            item += 1
    for persona, character, name, rule, value, element, hit, text in SIGNATURES:
        weapons = shop[character]
        if rule[0] == "ratio":
            anchor = ratio_anchor(weapons, rule[1], rule[2])[0]
        else:
            anchor = band_anchor(weapons, rule[1], best=rule[0] == "best")[0]
        band = band_of(level(persona))
        rows.append((item, "signature", character, band, persona, anchor, value, element, hit, name, text))
        item += 1
    if item - 1 > LAST_ID:
        raise SystemExit("fusion_arts_forge: the catalogue outgrew 2048-3327")
    for row in rows:
        if len(row[9]) > 16:
            raise SystemExit(f"fusion_arts_forge: name over 16 characters: {row[9]}")
    args.out.mkdir(parents=True, exist_ok=True)
    xml = ['<?xml version="1.0" encoding="UTF-8"?>',
           '<!-- Generated by tools/mods/fusion_arts_forge.py. Each row derives from an anchor in the user\'s own',
           '     ITEMTBL; only IDs, our own property codes and our own text are authored here. -->',
           '<p4-tables schema="1">']
    tsv = ["# id\tkind\tcharacter\tband\tkey\tanchor\tvalue\telement\thit\tname"]
    for (item, kind, character, band, key, anchor, value, element, hit, name, text) in rows:
        attributes = [f'id="{item}"', f'key="p4pc.fusion-arts:forge-{item}"', f'derive="{anchor}"', 'icon-source="anchor"']
        if value:
            attributes.append(f'value="{value:#04x}"')
        if element:
            attributes.append(f'element="{element}"')
        if hit:
            attributes.append(f'secondary-value="{MAX_HIT}"')
        xml.append(f'  <item {" ".join(attributes)}/>')
        xml.append(f'  <string table="0" id="{item}" role="0" bytes="{encode(name, 16)}"/>')
        xml.append(f'  <string table="0" id="{item}" role="1" bytes="{encode(text, 120)}"/>')
        tsv.append("\t".join(str(field) for field in (item, kind, character, band + 1, key, anchor, value, element,
                                                       int(hit), name)))
    xml.append("</p4-tables>")
    (args.out / "forge.xml").write_text("\n".join(xml) + "\n", encoding="utf-8", newline="\n")
    # Physical Split (companion, only with its `split` toggle on): every physical forged weapon resolves to
    # its family's leaf, as Physical Split types its owners' retail weapons (Slash 19, Strike 20, Pierce 21).
    split = ['<?xml version="1.0" encoding="UTF-8"?>',
             '<!-- Generated by tools/mods/fusion_arts_forge.py: forged weapons on Physical Split\'s family leaves. -->',
             '<p4-tables schema="1">']
    for (item, kind, character, band, key, anchor, value, element, hit, name, text) in rows:
        if not element:
            split.append(f'  <action source-kind="1" source-id="{item}" component="0" element="{LEAF[character]}" '
                         f'channel="0" key="p4pc.fusion-arts:split-{item}"/>')
    split.append("</p4-tables>")
    (args.out / "split.xml").write_text("\n".join(split) + "\n", encoding="utf-8", newline="\n")
    # main.luau's catalogue block: the forged rows, then every retail weapon ID per character (vessels).
    items = args.items.read_bytes()
    vessels = []
    for character, _, _ in CHARACTERS:
        ids = [i for i in range(1, 256)
               if struct.unpack_from("<I", items, i * ITEM_ROW + 0x30)[0] >> (character + 4) & 1
               and struct.unpack_from("<H", items, i * ITEM_ROW + 0x08)[0] > 1]
        vessels.append("\t".join(str(v) for v in [character] + ids))
    script = args.script.read_text(encoding="utf-8")
    begin = script.index(BEGIN_MARK)
    end = script.index(END_MARK)
    block = (BEGIN_MARK + "\nlocal CATALOG = [[\n" + "\n".join(tsv[1:]) + "\n]]\nlocal VESSELS = [[\n" +
             "\n".join(vessels) + "\n]]\n")
    args.script.write_text(script[:begin] + block + script[end:], encoding="utf-8", newline="\n")
    print(f"fusion_arts_forge: {len(rows)} rows, IDs {rows[0][0]}..{rows[-1][0]}")


if __name__ == "__main__":
    main()
