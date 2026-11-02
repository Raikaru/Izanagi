#!/usr/bin/env python3
"""Real, separately admitted MOD_WITNESS battle/save proof for an expanded
element cohort (Physical Split development baseline); not a retail comparison
and not shipping tuning.

From the user's own disc it privately generates a package with three physical
leaves (Slash 19, Strike 20, Pierce 21), each with synthetic icon art, the
nine-column [element-ui] layout, an alias plus three side rows for every retail
enemy (ENEMY,e,e for 0..335) and persona record (PERSONA,p,0 for 1..255) that
copy the entity's decoded retail Phys word, and weapon damage bindings by
equipped item: Yu, Yosuke and Teddie weapons Slash, Chie and Kanji Strike,
Yukiko and Naoto Pierce; and the design's skill samples Cleave (Slash), Bash
(Strike), Skewer and Black Spot (Pierce). The castle 5F training walk then
fights with those weapons and saves; the next checkpoint reloads that save with
the package; a Camp menu-harness leg captures the hero's skill list and persona
panel with the nine-column layout (evidence images stay in the work directory)."""
import argparse
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.path.insert(0, str(Path(__file__).resolve().parent))
import mod_witness
import retail_tables

PACKAGE = "p4pc.physsplit"
LEAVES = ((19, "slash", "Slash"), (20, "strike", "Strike"), (21, "pierce", "Pierce"))
# Unit IDs (ELF unit-name table 0x005E4480): 1 Protagonist, 2 Yosuke, 3 Chie,
# 4 Yukiko, 5 Rise (no weapon), 6 Kanji, 7 Naoto, 8 Teddie.
UNIT_LEAF = {1: 19, 2: 19, 3: 20, 4: 21, 6: 20, 7: 21, 8: 19}
# Retail skill IDs (MSG.TBL skill names): Cleave, Bash, Skewer, Black Spot.
SKILL_LEAF = {0x79: 19, 0x78: 20, 0x96: 21, 0xA8: 21}
CAPTURE = ("open,wait30,cross,wait110,cross,wait110,shot:skills,circle,wait60,circle,wait60,"
           "down,down,down,cross,wait110,cross,wait110,square,wait120,shot:persona_status")
FLAGS = 1 | 2 | 4 | 8  # PHYSICAL | OFFENSIVE | AFFINITY | VISIBLE
ENEMIES, PERSONAS, ITEMS, ITEM_ROW = 336, 256, 1536, 68
AFFINITY_ROW = 32


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


def png(width, height, pixel):
    """A synthetic straight-RGBA8 PNG; pixel(x, y) returns four bytes."""
    raw = b"".join(b"\0" + b"".join(pixel(x, y) for x in range(width)) for y in range(height))

    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)
    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", header) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b"")


def element_package(iso, root):
    """Write the private package; returns {weapon item id: leaf}."""
    unit = retail_tables.blocks(retail_tables.battle_files(iso, ["UNIT.TBL"])["UNIT.TBL"])
    enemy_affinity, persona_affinity = unit[1], unit[2]
    assert len(enemy_affinity) == ENEMIES * AFFINITY_ROW, "unexpected retail enemy affinity bank"
    assert len(persona_affinity) == PERSONAS * AFFINITY_ROW, "unexpected retail persona affinity bank"
    items = retail_tables.data_file(iso, "INIT/ITEMTBL.BIN")
    header = len(items) - ITEMS * ITEM_ROW
    assert header >= 4 and struct.unpack_from("<I", items, 0)[0] == ITEMS, "unexpected retail item table"
    rows = ['<p4-tables schema="1">']
    for leaf, key, name in LEAVES:
        rows.append(f'<element id="{leaf}" flags="{FLAGS}" family="0" order="{leaf}" key="{PACKAGE}:{key}" '
                    f'display-name="{name}" short-name="{name}" icon-owner="{PACKAGE}" '
                    f'icon-key="{PACKAGE}:{key}-icon" icon-revision="1"/>')

    def entity(kind, entity_id, native, profile, knowledge, bank, label):
        phys = struct.unpack_from("<H", bank, entity_id * AFFINITY_ROW)[0]
        value = decode(phys)
        rows.append(f'<alias kind="{kind}" entity="{entity_id}" native-profile="{native}" profile="{profile}" '
                    f'knowledge="{knowledge}" key="{PACKAGE}:{label}-{entity_id}"/>')
        for leaf, _, _ in LEAVES:
            rows.append(f'<affinity kind="{kind}" entity="{entity_id}" profile="{profile}" element="{leaf}" '
                        f'channel="0" value="{value}" scalar="{value & 0xFFFF}"/>')
    for enemy in range(ENEMIES):
        entity(1, enemy, enemy, enemy, enemy, enemy_affinity, "enemy")
    for persona in range(1, PERSONAS):
        entity(0, persona, 0, 0x1000 + persona, 0, persona_affinity, "persona")
    weapons = {}
    for item in range(ITEMS):
        offset = header + item * ITEM_ROW
        kind, mask = struct.unpack_from("<I", items, offset)[0], struct.unpack_from("<I", items, offset + 0x30)[0]
        units = [unit_id for unit_id in range(1, 11) if mask & (1 << (unit_id + 4))]
        if kind != 0 or len(units) != 1 or units[0] not in UNIT_LEAF:
            continue
        weapons[item] = UNIT_LEAF[units[0]]
        rows.append(f'<action source-kind="1" source-id="{item}" component="0" element="{weapons[item]}" '
                    f'channel="0" key="{PACKAGE}:weapon-{item}"/>')
    for skill, leaf in SKILL_LEAF.items():
        rows.append(f'<action source-kind="0" source-id="{skill}" component="0" element="{leaf}" '
                    f'channel="0" key="{PACKAGE}:skill-{skill}"/>')
    rows.append("</p4-tables>")
    # Only weapon bindings make the three leaves battle-required, which the
    # element-ui completion needs for every new-leaf column.
    assert set(weapons.values()) == {leaf for leaf, _, _ in LEAVES}, "a leaf has no weapon binding"
    package = root / PACKAGE
    (package / "data").mkdir(parents=True)
    (package / "art").mkdir()
    (package / "data/elements.xml").write_text("\n".join(rows) + "\n", encoding="ascii")
    sections = ["[mod]\nid = %s\nname = Physical Split witness\nversion = 1.0.0\napi = 1.1\npriority = 0\norder = 0\n" % PACKAGE,
                "[table-source %s:split]\nfile = data/elements.xml\nformat = p4-tables-xml\nschema = 1\nrevision = 1\n" % PACKAGE]
    tints = {19: (0xE0, 0x40, 0x40), 20: (0x40, 0xC0, 0x40), 21: (0x40, 0x60, 0xE0)}
    for leaf, key, _ in LEAVES:
        red, green, blue = tints[leaf]
        (package / "art" / (key + ".png")).write_bytes(png(32, 32, lambda x, y: bytes(
            (red, green, blue, 0xFF if 4 <= x < 28 and 4 <= y < 28 else 0))))
        sections.append("[resource-image %s:%s-icon]\nfile = art/%s.png\nformat = png\nschema = 1\nrevision = 1\n"
                        % (PACKAGE, key, key))
    order = [f"{PACKAGE}:{key}" for _, key, _ in LEAVES] + \
        ["p4:fire", "p4:ice", "p4:electric", "p4:wind", "p4:light", "p4:dark"]
    sections.append("[element-ui]\nschema = 1\nrevision = 1\norder = %s\n" % ", ".join(order))
    for _, key, _ in LEAVES:
        sections.append("[element-column %s:%s]\ncomponent = damage\nchannel = p4:damage\n" % (PACKAGE, key))
    for key, legacy in (("fire", "fire"), ("ice", "ice"), ("electric", "elec"), ("wind", "wind")):
        sections.append("[element-column p4:%s]\ncomponent = damage\nchannel = p4:damage\nlegacy-primary = %s\n"
                        % (key, legacy))
    for key in ("light", "dark"):
        sections.append("[element-column p4:%s]\ncomponent = instant-ko\nchannel = p4:instant_ko\n"
                        "legacy-primary = %s-ko\n" % (key, key))
    (package / "mod.ini").write_text("\n".join(sections), encoding="ascii")
    return weapons


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--driver", nargs=argparse.REMAINDER, required=True)
    args = parser.parse_args()
    if not args.driver:
        parser.error("--driver must name the native player (and optional Wine prefix)")
    if os.environ.get("P4PC_INPUT") != "script" or os.environ.get("P4PC_MOD_WITNESS") != "1":
        raise ValueError("expanded-element proof requires explicit P4PC_INPUT=script and P4PC_MOD_WITNESS=1")
    if os.environ.get("P4PC_MODS") == "0" or os.environ.get("P4PC_PACKS") != "0":
        raise ValueError("expanded-element proof requires mods permitted and packs disabled")
    elf = Path(os.environ.get("P4PC_ELF", ""))
    iso = Path(os.environ.get("P4PC_ISO", ""))
    seed = Path(os.environ.get("P4PC_ELEMENT_SEED", str(elf.parent / "saves/story0420_5f")))
    if not elf.is_file() or not iso.is_file() or not (seed / "BASLUS-21782Save0").is_dir():
        print("mod_element: retail ELF/ISO or private story0420_5f seed unavailable", file=sys.stderr)
        return 77
    args.out.mkdir(parents=True, exist_ok=True)
    work = Path(tempfile.mkdtemp(prefix="mod-element-", dir=args.out)).resolve()
    mod_witness.begin(work)
    outcome = "failed"
    try:
        weapons = element_package(iso, work / "mods")
        card = work / "card" / "mc0"
        shutil.copytree(seed, card)
        config = work / "p4pc.ini"
        config.write_text("[mods]\nenabled=%s\n" % PACKAGE, encoding="utf-8")
        common = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(work / "mods"),
                      P4PC_MODS="1", P4PC_PACKS="0", P4PC_INPUT="script", P4PC_MOD_WITNESS="1",
                      P4PC_ISO=str(iso.resolve()), P4PC_SAVE_DIR=str(card.parent),
                      P4PC_SCENE_ARTIFACTS=str(work), P4PC_HIDDEN_WINDOW="1", P4PC_OFFSCREEN="1",
                      SDL_AUDIODRIVER="dummy", P4PC_TRAP_CONTINUE="1", P4PC_TURBO="2",
                      P4PC_VIRTUAL_CLOCK="1", P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_LOAD="0")
        if os.name == "nt" or any("wine" in Path(part).name.lower() for part in args.driver[:-1]):
            common.pop("SDL_VIDEODRIVER", None)
        else:
            common["SDL_VIDEODRIVER"] = "offscreen"
        for key in ("P4PC_REF_RGB", "P4PC_REF_MAX", "P4PC_CAPTURE_FRAMES", "P4PC_BOARD_SEED",
                    "P4PC_DUMP_RAM", "P4PC_DUMP_RAM_AT", "P4PC_MOD_WITNESS_OUT"):
            common.pop(key, None)
        cases = (("battle", "save0420_5f_to_training.walk", "38000,20:1,4/20"),
                 ("reload", "entrance_sleep_to_school.walk", "13000,6:6,4/21"))
        for label, walk, gate in cases:
            env = dict(common, P4PC_WALK_SCRIPT=str(ROOT / "tests/walks" / walk),
                       P4PC_NEWGAME_GATE=gate)
            log = work / (label + ".log")
            with log.open("wb") as output:
                result = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env,
                                        stdout=output, stderr=subprocess.STDOUT, timeout=2400, check=False,
                                        creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
            text = log.read_text(encoding="utf-8", errors="replace")
            assert result.returncode == 0, f"{label} exited {result.returncode}; see {log}"
            assert "run_mode=mod-witness" in text and "reference_eligible=false" in text
            assert "walk script done" in text and "SDKMEMCARD_SAVE OK" in text, f"{label} walk/save incomplete"
            # Layout/coordinator refusals name the element-ui owner; success never does.
            assert "element-ui" not in text, f"{label} refused the element-ui layout or participant"
            resolved = [(int(leaf), kind, int(source)) for leaf, kind, source in
                        re.findall(r"p4pc: element leaf (\d+) resolves (weapon|skill) (\d+)", text)]
            if label == "battle":
                assert resolved, "no authored physical leaf resolved a weapon or skill in battle"
                for leaf, kind, source in resolved:
                    expected = (weapons if kind == "weapon" else SKILL_LEAF).get(source)
                    assert expected == leaf, f"{kind} {source} resolved leaf {leaf}"
            sidecar = card / "BASLUS-21782Save0" / ".p4mods"
            assert sidecar.is_file() and PACKAGE.encode() in sidecar.read_bytes(), \
                f"{label} save does not require {PACKAGE}"
            (work / (label + ".p4mods-copy")).write_bytes(sidecar.read_bytes())
            print(f"mod_element_game: {label} resolved={resolved}")
        # Camp captures from the menu harness's new-game state. A harness exit
        # never reaches the interactive title, so it starts from a fresh config.
        config.write_text("[mods]\nenabled=%s\n" % PACKAGE, encoding="utf-8")
        env = dict(common, P4PC_START_MENU="7:2", P4PC_MENU_EXIT="1", P4PC_DRAW_WINDOW="10",
                   P4PC_MENU_SCRIPT=CAPTURE)
        log = work / "capture.log"
        with log.open("wb") as output:
            result = subprocess.run([*args.driver, str(elf.resolve())], cwd=work, env=env,
                                    stdout=output, stderr=subprocess.STDOUT, timeout=900, check=False,
                                    creationflags=subprocess.IDLE_PRIORITY_CLASS if os.name == "nt" else 0)
        text = log.read_text(encoding="utf-8", errors="replace")
        assert result.returncode == 0, f"capture exited {result.returncode}; see {log}"
        assert "element-ui" not in text, "capture refused the element-ui layout or participant"
        assert "script finished: camp opened, 2 shots, 0 dark, 0 failed writes" in text, "capture incomplete"
        for shot in ("skills", "persona_status"):
            assert (work / f"menu_{shot}.png").is_file(), f"capture {shot} missing"
        outcome = "pass"
        print(f"mod_element_game: PASS mod_witness=true reference_eligible=false weapons={len(weapons)} evidence={work}")
        return 0
    finally:
        mod_witness.finish(work, 0 if outcome == "pass" else 1, outcome=outcome)


if __name__ == "__main__":
    sys.exit(main())
