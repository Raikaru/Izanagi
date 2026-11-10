#!/usr/bin/env python3
"""Physical Split mod data (mods/p4pc.physsplit, docs/modding/tables.md "Physical Split mod").

The shipped package carries authored data only: element descriptors, the
skill/weapon/enemy-attack subtype bindings, explicit authored enemy subtype
affinities and original icon art. Every retail affinity value is a
`retail-row`/`retail-element` reference that the table service reads from the
user's own disc at boot; no retail byte is committed or shipped.

  catalog --iso ISO      refresh tools/physsplit/catalog.json (IDs only)
  seed --iso ISO --fes GUIDE
                         rebuild tools/physsplit/enemies.json from the FES
                         shadow guide text and the P4 balance rules
  generate               write the package's data/split.xml and art/*.png
  check                  exit 1 unless the committed package matches generate
  report --iso ISO --out FILE
                         private per-enemy review matrix with retail values
                         and constraint results (never commit its output)
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import struct
import sys
from ui_art.ui_art import vector_png

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "tools" / "physsplit"
PACKAGE = ROOT / "mods" / "p4pc.physsplit"
OWNER = "p4pc.physsplit"
TYPES = ("slash", "strike", "pierce")
KIND_FLAGS = {"normal": 0, "null": 0x01000000, "repel": 0x02000000, "drain": 0x04000000,
              "weak": 0x08000000, "resist": 0x10000000}
DEFAULT_PERCENT = {"normal": 100, "resist": 50, "weak": 125, "null": 100, "repel": 100, "drain": 100}
ELEMENT_FLAGS = 1 | 2 | 4 | 8  # PHYSICAL | OFFENSIVE | AFFINITY | VISIBLE
ENEMIES, PERSONAS, ITEMS, ITEM_ROW = 336, 256, 1536, 68
SKILL_ACTIVE = 0x1B8
SET_ID2 = 0x2091  # AI_FUNCTION_0091 btlAICommandScriptSetId2ClearBuffs (0x001E5D90)


def load(name):
    return json.loads((DATA / name).read_text(encoding="utf-8"))


def dump(name, value):
    (DATA / name).write_text(json.dumps(value, indent=1, sort_keys=False) + "\n", encoding="utf-8", newline="\n")


# ---------------------------------------------------------------- retail disc

def retail(iso):
    sys.path.insert(0, str(ROOT / "tests"))
    import retail_tables
    battle = retail_tables.battle_files(iso, ["UNIT.TBL", "AICALC.TBL", "SKILL.TBL", "MSG.TBL"])
    blocks = {name: retail_tables.blocks(data) for name, data in battle.items()}
    items = retail_tables.data_file(iso, "INIT/ITEMTBL.BIN")
    digests = {name: hashlib.sha256(data).hexdigest() for name, data in battle.items()}
    digests["ITEMTBL.BIN"] = hashlib.sha256(items).hexdigest()
    return blocks, items, digests


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def decode(word):
    """p4_affinity_decode: flags in the high byte, the low byte in 5% steps."""
    value = ((word & 0xFF00) << 16) | ((word & 0xFF) * 5)
    if not value & 0xFFFF:
        if value & 0x08000000:
            value |= 125
        elif value & 0x27000000:
            value |= 100
        elif value & 0x10000000:
            value |= 50
    return value


def classify(value):
    flags = value & 0xFF000000
    kind = next((name for name, flag in KIND_FLAGS.items() if flag == flags), "other")
    return kind, value & 0xFFFF


def flow_code(script):
    """(pc, opcode, operand, immediate) for a FLW0 script's code section."""
    sections = {}
    for index in range(u32(script, 16)):
        kind, stride, count, offset = struct.unpack_from("<4I", script, 32 + index * 16)
        sections[kind] = (stride, count, offset)
    _, count, offset = sections[2]
    code, pc = [], 0
    while pc < count:
        opcode = u16(script, offset + pc * 4)
        operand = struct.unpack_from("<h", script, offset + pc * 4 + 2)[0]
        if opcode <= 1:  # PUSHI/PUSHF carry the next word
            code.append((pc, opcode, operand, u32(script, offset + (pc + 1) * 4)))
            pc += 2
        else:
            code.append((pc, opcode, operand, None))
            pc += 1
    _, procedures, offset = sections[0]
    starts = [u32(script, offset + i * 32 + 24) for i in range(procedures)]
    return code, starts


def enemy_phases(aicalc):
    """{enemy: sorted id2 values} from literal AI_FUNCTION_0091 arguments."""
    rows, script = aicalc[8], aicalc[10]
    code, starts = flow_code(script)
    owners = {}
    for enemy in range(ENEMIES):
        owners.setdefault(u16(rows, enemy * 164 + 2), []).append(enemy)
    phases = {}
    for index, (pc, opcode, operand, _) in enumerate(code):
        if opcode != 8 or operand != SET_ID2:
            continue
        previous = code[index - 1]
        if previous[1] == 29:      # PUSHIS
            value = previous[2] & 0xFFFF
        elif previous[1] == 0:     # PUSHI
            value = previous[3] & 0xFFFF
        else:
            raise SystemExit(f"physsplit: non-literal id2 at AI pc {pc}; declare its phases by hand")
        procedure = max(i for i, start in enumerate(starts) if start <= pc)
        for enemy in owners.get(procedure, []):
            phases.setdefault(enemy, set()).add(value)
    return {str(enemy): sorted(values) for enemy, values in sorted(phases.items())}


def catalog(args):
    blocks, items, digests = retail(args.iso)
    elements, metadata = blocks["SKILL.TBL"][0], blocks["SKILL.TBL"][1]
    fixed, weapon_derived = [], []
    for skill in range(SKILL_ACTIVE):
        if elements[skill * 2] == 0:
            (weapon_derived if metadata[skill * 40] & 2 else fixed).append(skill)
    header = len(items) - ITEMS * ITEM_ROW
    weapons = []
    for item in range(ITEMS):
        offset = header + item * ITEM_ROW
        if u32(items, offset) != 0 or u16(items, offset + 0x2C) != 0:
            continue  # Not a weapon, or an element-bearing weapon that keeps its element.
        mask = u32(items, offset + 0x30)
        weapons.append({"id": item, "units": [unit for unit in range(1, 11) if mask & (1 << (unit + 4))]})
    unit = blocks["UNIT.TBL"]
    if len(unit[0]) != ENEMIES * 60 or len(unit[1]) != ENEMIES * 32 or len(unit[2]) != PERSONAS * 32:
        raise SystemExit("physsplit: unexpected UNIT.TBL banks")
    basic_phys = [enemy for enemy in range(ENEMIES) if unit[0][enemy * 60 + 0x38] == 0]
    dump("catalog.json", {
        "schema": 1,
        "retail_sha256": digests,
        "fixed_phys_skills": fixed,
        "weapon_derived_skills": weapon_derived,
        "phys_weapons": weapons,
        "enemies": ENEMIES,
        "personas": PERSONAS,
        "basic_phys_enemies": basic_phys,
        "enemy_phases": enemy_phases(blocks["AICALC.TBL"]),
    })
    print(f"physsplit: catalog {len(fixed)} fixed and {len(weapon_derived)} weapon-derived Phys skills, "
          f"{len(weapons)} Phys weapons, {len(basic_phys)} Phys basic attackers")
    return 0


# ---------------------------------------------------------------- FES seed

FES_NAME_FIXES = {"Mercliess Maya": "Merciless Maya"}  # Guide typo; same Shadow.
IMMUNE = ("null", "repel", "drain")
NEXT = {"slash": "strike", "strike": "pierce", "pierce": "slash"}
PREVIOUS = {value: key for key, value in NEXT.items()}


def fes_guide(path):
    """{name: [{arcana, slash, strike, pierce, basic, line}]} from PennyEve's FES guide text."""
    lines = Path(path).read_text(encoding="utf-8", errors="replace").split("\n")
    entries, current = [], None
    for number, line in enumerate(lines, 1):
        header = re.match(r"^ (\S.*?):\s*$", line)
        if header and not line.startswith("     "):
            current = {"name": header.group(1).strip(), "line": number, "text": []}
            entries.append(current)
        elif current is not None and line.startswith("     "):
            current["text"].append(line.strip())
        elif current is not None and line and not line.startswith((" ", "*", "=")):
            current["text"].append(line.strip())  # Wrapped continuation.
    result = {}
    for entry in entries:
        joined = " ".join(entry["text"])
        parts = re.split(r"(Strengths?:|Weakness(?:es)?:|Attacks:|Note:)", joined)
        fields, label = {"arcana": parts[0].replace("Arcana", "").strip()}, None
        for part in parts[1:]:
            if re.fullmatch(r"(Strengths?:|Weakness(?:es)?:|Attacks:|Note:)", part):
                label = part[:4].lower()
            else:
                fields[label] = part.strip()
        facts = {kind: "normal" for kind in TYPES}
        for item in re.split(r",\s*(?![^()]*\))", fields.get("stre", "")):
            match = re.fullmatch(r"(Slash|Strike|Pierce)\s*(?:\((Null|Repel|Drain)\))?", item.strip())
            if match:
                facts[match.group(1).lower()] = (match.group(2) or "Resist").lower()
        for item in re.split(r",\s*", fields.get("weak", "")):
            if item.strip() in ("Slash", "Strike", "Pierce"):
                facts[item.strip().lower()] = "weak"
        basic = re.match(r"\s*(Slash|Strike|Pierce) ?[Aa]?ttack", fields.get("atta", ""))
        name = FES_NAME_FIXES.get(entry["name"], entry["name"])
        result.setdefault(name, []).append(dict(facts, arcana=fields["arcana"],
                                                 basic=basic.group(1).lower() if basic else None,
                                                 line=entry["line"]))
    return result


def compensate(facts, start, table):
    """The first subtype after start (in the pairing cycle) the source calls normal."""
    kind = table[start]
    for _ in range(2):
        if facts[kind] == "normal":
            return kind
        kind = table[kind]
    return None


def adapt(kind, percent, facts):
    """(final, method, reasons) under the balance rules (docs/modding/tables.md "Physical Split mod")."""
    retail_all = {subtype: "retail" for subtype in TYPES}
    if kind in IMMUNE:
        return retail_all, "locked", ["phys_immunity_lock"]
    if facts is None:
        return retail_all, "retail_equal", ["no_fes_counterpart"]
    final = dict(retail_all)
    if kind == "normal" and percent == 100:
        if all(facts[subtype] == "normal" for subtype in TYPES):
            return final, "fes_exact", []
        weak = [subtype for subtype in TYPES if facts[subtype] == "weak"]
        sturdy = [subtype for subtype in TYPES if facts[subtype] not in ("normal", "weak")]
        if len(sturdy) == 3:
            return final, "fes_adapted", ["fes_uniform_resist_exceeds_normal_budget"]
        reasons = []
        if weak:
            weak_kind = weak[0]
            others = [subtype for subtype in sturdy if subtype != weak_kind]
            resist_kind = others[0] if others else compensate(facts, weak_kind, NEXT)
            if not others:
                reasons.append("compensating_resist_pairing_v1")
        else:
            resist_kind = sturdy[0]
            weak_kind = compensate(facts, resist_kind, PREVIOUS)
            reasons.append("compensating_weak_pairing_v1")
        if weak_kind is None or resist_kind is None:
            return retail_all, "fes_adapted", ["fes_no_balanced_pair"]
        if facts[resist_kind] in IMMUNE:
            reasons.append("no_new_phys_immunity")
        if len(weak) > 1 or len(sturdy) > 1:
            reasons.append("p4_phys_normal_budget")
        final[weak_kind], final[resist_kind] = "weak", "resist"
        return final, "fes_adapted" if reasons else "fes_exact", reasons
    if kind == "normal":
        return final, "fes_adapted", ["nonstandard_percent_retained"]
    if kind == "resist":
        immune = [subtype for subtype in TYPES if facts[subtype] in IMMUNE]
        reasons = []
        if immune:
            final[immune[0]] = "null"
            if facts[immune[0]] != "null":
                reasons.append("no_new_reflect_drain")
            if len(immune) > 1:
                reasons.append("p4_phys_resist_two_resists")
        if any(facts[subtype] in ("normal", "weak") for subtype in TYPES):
            reasons.append("p4_phys_resist_budget")
        return final, "fes_adapted" if reasons else "fes_exact", reasons
    if kind == "weak":
        normal = [subtype for subtype in TYPES if facts[subtype] == "normal"]
        if normal:
            final[normal[0]] = "normal"
        return final, "fes_adapted", ["p4_phys_weak_budget"]
    return retail_all, "retail_equal", ["unclassified_retail_word"]


ARCANA = ("Fool", "Magician", "Priestess", "Empress", "Emperor", "Hierophant", "Lovers", "Chariot", "Justice",
          "Hermit", "Fortune", "Strength", "Hanged Man", "Death", "Temperance", "Devil", "Tower", "Star", "Moon",
          "Sun", "Judgement")  # UNIT0 metadata +2 holds this index + 1 (it agrees for every exact FES match).


def counterpart(guide, name, arcana):
    """(FES entry, counterpart locator, match reason) for one P4 enemy, or Nones."""
    if name in guide:
        return guide[name][0], name, "exact shadow name"
    spelled = name.replace("Judgment", "Judgement")
    if spelled in guide:
        return guide[spelled][0], spelled, "exact shadow, FES spelling"
    if "(Sub-boss) " + name in guide:
        return guide["(Sub-boss) " + name][0], "(Sub-boss) " + name, "exact shadow; FES lists only its sub-boss entry"
    family = name.split()[-1] if name else ""
    same = sorted(n for n, entries in guide.items()
                  if not n.startswith("(") and n.split()[-1] == family and entries[0]["arcana"] == arcana)
    if same and len({tuple(guide[n][0][t] for t in (*TYPES, "basic")) for n in same}) == 1:
        return guide[same[0]][0], ", ".join(same), "same family and arcana, every such FES shadow agrees (tier not compared)"
    return None, None, None


def seed(args):
    blocks, _, _ = retail(args.iso)
    names = blocks["MSG.TBL"][2]
    guide = fes_guide(args.fes)
    phases = load("catalog.json")["enemy_phases"]
    records = []
    for enemy in range(ENEMIES):
        name = names[enemy * 21:(enemy + 1) * 21].split(b"\0")[0].decode("latin1")
        kind, percent = classify(decode(u16(blocks["UNIT.TBL"][1], enemy * 32)))
        arcana_index = blocks["UNIT.TBL"][0][enemy * 60 + 2] - 1
        arcana = ARCANA[arcana_index] if 0 <= arcana_index < len(ARCANA) else None
        source, locator, match = counterpart(guide, name, arcana)
        facts = {subtype: source[subtype] for subtype in TYPES} if source else None
        final, method, reasons = adapt(kind, percent, facts)
        unused = name in ("blank", "000") or not name
        record = {
            "id": enemy,
            "status": "unused" if unused else "reviewed" if source or kind in IMMUNE else "pending",
            "method": method,
            "final": {subtype: (value if value == "retail" else {"kind": value, "percent": DEFAULT_PERCENT[value]})
                      for subtype, value in final.items()},
            "basic": (source or {}).get("basic") or "strike",
            "rule": {"id": "physsplit-balance", "revision": 1},
            "adaptation_reasons": reasons,
        }
        if source:
            record["fes_source"] = {"edition": "Persona 3 FES", "locator": f"PennyEve shadow guide line {source['line']}",
                                    "counterpart": locator, "match": match,
                                    "arcana": source["arcana"],
                                    "affinities": {subtype: source[subtype] for subtype in TYPES}}
        if str(enemy) in phases:
            record["phases"] = [{"native_profile": value, "final": "retail", "lock_reason": "scripted_affinity_phase"}
                                for value in phases[str(enemy)]]
        records.append(record)
    dump("enemies.json", {"schema": 1, "enemies": records})
    print("physsplit: seeded", len(records), "enemy records")
    return 0


# ---------------------------------------------------------------- package

def element_ids():
    authoring = load("authoring.json")
    return {element["key"]: element["id"] for element in authoring["elements"]}, authoring


def affinity(kind, entity, profile, element, value):
    """One <affinity>: an authored {kind, percent} or the entity's own retail Phys."""
    head = f'<affinity kind="{kind}" entity="{entity}" profile="{profile}" element="{element}" channel="0" '
    if value == "retail":
        return head + f'retail-row="{entity if kind == 0 else profile}" retail-element="0"/>'
    word = KIND_FLAGS[value["kind"]] | value["percent"]
    return head + f'value="{word}" scalar="{value["percent"]}"/>'


def split_xml():
    ids, authoring = element_ids()
    cat, enemies = load("catalog.json"), load("enemies.json")["enemies"]
    rows = ['<?xml version="1.0" encoding="us-ascii"?>', '<p4-tables schema="1">']
    for element in authoring["elements"]:
        key = element["key"]
        rows.append(f'<element id="{element["id"]}" flags="{ELEMENT_FLAGS}" family="0" order="{element["id"]}" '
                    f'key="{OWNER}:{key}" display-name="{element["display"]}" short-name="{element["short"]}" '
                    f'icon-owner="{OWNER}" icon-key="{OWNER}:{key}-icon" icon-revision="{element["icon_revision"]}"/>')
    # Enemies: every retail row, plus each scripted id2 phase as its own profile.
    if len(enemies) != cat["enemies"] or [record["id"] for record in enemies] != list(range(cat["enemies"])):
        raise SystemExit("physsplit: enemies.json must hold exactly one record per catalog enemy")
    for record in enemies:
        enemy = record["id"]
        rows.append(f'<alias kind="1" entity="{enemy}" native-profile="{enemy}" profile="{enemy}" '
                    f'knowledge="{enemy}" key="{OWNER}:enemy-{enemy}"/>')
        for subtype in TYPES:
            rows.append(affinity(1, enemy, enemy, ids[subtype], record["final"][subtype]))
        for phase in record.get("phases", []):
            native = phase["native_profile"]
            rows.append(f'<alias kind="1" entity="{enemy}" native-profile="{native}" profile="{native}" '
                        f'knowledge="{enemy}" key="{OWNER}:enemy-{enemy}-phase-{native}"/>')
            for subtype in TYPES:
                rows.append(affinity(1, enemy, native, ids[subtype], phase["final"]))
    overrides = {int(key): value for key, value in authoring.get("persona_overrides", {}).items()}
    for persona in range(1, cat["personas"]):
        profile = 0x1000 + persona
        rows.append(f'<alias kind="0" entity="{persona}" native-profile="0" profile="{profile}" '
                    f'knowledge="0" key="{OWNER}:persona-{persona}"/>')
        for subtype in TYPES:
            rows.append(affinity(0, persona, profile, ids[subtype], overrides.get(persona, {}).get(subtype, "retail")))
    defaults = authoring["unit_weapon_defaults"]
    weapon_overrides = {int(key): value for key, value in authoring.get("weapon_overrides", {}).items()}
    for weapon in cat["phys_weapons"]:
        units = weapon["units"]
        subtype = weapon_overrides.get(weapon["id"]) or (defaults[str(units[0])] if len(units) == 1
                                                          else authoring["shared_weapon_default"])
        rows.append(f'<action source-kind="1" source-id="{weapon["id"]}" component="0" element="{ids[subtype]}" '
                    f'channel="0" key="{OWNER}:weapon-{weapon["id"]}"/>')
    skills = {int(key, 0): value for key, value in authoring["skills"].items()}
    for skill in cat["fixed_phys_skills"]:
        subtype = skills.get(skill, authoring["fixed_phys_default"])
        rows.append(f'<action source-kind="0" source-id="{skill}" component="0" element="{ids[subtype]}" '
                    f'channel="0" key="{OWNER}:skill-{skill}"/>')
    basic = {record["id"]: record["basic"] for record in enemies}
    for enemy in cat["basic_phys_enemies"]:
        rows.append(f'<action source-kind="2" source-id="{enemy}" component="0" element="{ids[basic[enemy]]}" '
                    f'channel="0" key="{OWNER}:enemy-attack-{enemy}"/>')
    rows.append("</p4-tables>")
    return ("\n".join(rows) + "\n").encode("ascii")



def outputs():
    files = {PACKAGE / "data" / "split.xml": split_xml()}
    for element in load("authoring.json")["elements"]:
        files[PACKAGE / "art" / (element["key"] + ".png")] = vector_png(element["key"] + "-skill")
    return files


def generate(args):
    for path, data in outputs().items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        print("physsplit: wrote", path.relative_to(ROOT))
    return 0


def check(args):
    stale = [path for path, data in outputs().items() if not path.is_file() or path.read_bytes() != data]
    for path in stale:
        print("physsplit: stale generated file", path.relative_to(ROOT), "(run tools/physsplit.py generate)")
    return 1 if stale else 0


# ---------------------------------------------------------------- review report

def value_of(final, retail_value):
    if final == "retail":
        return retail_value
    return KIND_FLAGS[final["kind"]] | final["percent"]


def constraints(retail_value, values):
    """Failed design gates for one enemy profile (docs/modding/tables.md "Physical Split mod")."""
    kind, percent = classify(retail_value)
    kinds = [classify(value)[0] for value in values]
    failures = []
    if kind in IMMUNE and any(value != retail_value for value in values):
        failures.append("immunity_not_locked")
    if kind not in IMMUNE and any(k in ("repel", "drain") for k in kinds):
        failures.append("invented_reflect_drain")
    if kind == "resist" and (kinds.count("resist") < 2 or "weak" in kinds):
        failures.append("resist_rule")
    if kind == "weak" and ("weak" not in kinds or any(k in IMMUNE for k in kinds)):
        failures.append("weak_rule")
    if kind == "normal":
        new_weak = kinds.count("weak")
        if new_weak > 1 or any(k in IMMUNE for k in kinds) or (new_weak and "resist" not in kinds):
            failures.append("normal_rule")
    ordinary = [value & 0xFFFF for value, k in zip(values, kinds) if k not in IMMUNE]
    if kind not in IMMUNE and ordinary and abs(sum(ordinary) / len(ordinary) - percent) > 10.0:
        failures.append("mean_budget")
    return failures


def report(args):
    blocks, _, _ = retail(args.iso)
    names, unit = blocks["MSG.TBL"][2], blocks["UNIT.TBL"][1]
    lines = ["id\tname\tstatus\tmethod\tretail\tslash\tstrike\tpierce\tbasic\treasons\tgates"]
    totals = {}
    for record in load("enemies.json")["enemies"]:
        enemy = record["id"]
        name = names[enemy * 21:(enemy + 1) * 21].split(b"\0")[0].decode("latin1")
        profiles = [(enemy, record["final"])] + [(phase["native_profile"], {t: phase["final"] for t in TYPES})
                                                 for phase in record.get("phases", [])]
        failures = []
        for row, final in profiles:
            retail_value = decode(u16(unit, row * 32))
            failures += constraints(retail_value, [value_of(final[t], retail_value) for t in TYPES])
        retail_value = decode(u16(unit, enemy * 32))
        shown = ["%s%d" % classify(value_of(record["final"][t], retail_value)) for t in TYPES]
        lines.append("\t".join([str(enemy), name, record["status"], record["method"], "%s%d" % classify(retail_value),
                                *shown, record["basic"], ",".join(record["adaptation_reasons"]),
                                ",".join(sorted(set(failures))) or "pass"]))
        totals[record["status"]] = totals.get(record["status"], 0) + 1
        totals["gate_fail"] = totals.get("gate_fail", 0) + bool(failures)
    Path(args.out).write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("physsplit: report", args.out, json.dumps(totals, sort_keys=True))
    return 1 if totals.get("gate_fail") else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    for name in ("catalog", "seed", "report"):
        command = commands.add_parser(name)
        command.add_argument("--iso", default=os.environ.get("P4PC_ISO"), required=not os.environ.get("P4PC_ISO"))
        if name == "seed":
            command.add_argument("--fes", required=True, help="local text of PennyEve's P3 FES Shadow Guide")
        if name == "report":
            command.add_argument("--out", required=True)
    commands.add_parser("generate")
    commands.add_parser("check")
    args = parser.parse_args()
    return {"catalog": catalog, "seed": seed, "generate": generate, "check": check, "report": report}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
