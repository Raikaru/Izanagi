"""Actual Fusion XML and catalogue through the CPU builder, over synthetic ITEM rows.

This never opens an installed game, renders a sprite, or admits native tables.
The catalogue is an independent consumer of the authored row IDs and anchors.
"""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
from collections import Counter

from game_elements_inputs_test import input_disc

ROOT = Path(__file__).resolve().parents[1]
MOD = ROOT / "mods" / "p4pc.fusion-arts"
MODES = ("good", "wrong-anchor", "wrong-family", "bad-kind", "bad-flags",
         "missing-icon", "conflicting-icon", "missing-name")
FAMILY = {1: 1, 2: 2, 3: 4, 4: 8, 6: 16, 7: 32, 8: 64}


def catalogue():
    script = (MOD / "main.luau").read_text(encoding="utf-8")
    block = script.split("local CATALOG = [[\n", 1)[1].split("\n]]", 1)[0]
    result = []
    for line in block.splitlines():
        item, kind, actor, band, persona, anchor, value, element, hit, name = line.split("\t")
        result.append((int(item), kind, int(actor), int(anchor), int(value), int(element), int(hit), name))
    assert len(result) == 910
    assert Counter(row[1] for row in result) == {"generic": 840, "wild": 56, "signature": 14}
    assert set(row[2] for row in result) == set(FAMILY)
    return result


def item_bank(rows, mode):
    owners = {}
    for _, _, actor, anchor, _, _, _, _ in rows:
        assert 0 < anchor < 256 and actor in FAMILY
        assert anchor not in owners or owners[anchor] == actor
        owners[anchor] = actor
    bank = bytearray(struct.pack("<4I", 1536, 0, 0, 0) + bytes(1536 * 68))
    target = rows[0][3]
    for anchor, actor in owners.items():
        offset = 16 + anchor * 68
        flags = FAMILY[actor]
        if anchor == target and mode == "wrong-family":
            flags = FAMILY[2]
        if anchor == target and mode == "bad-flags":
            flags = 0
        kind = 1 if anchor == target and mode == "bad-kind" else 0
        struct.pack_into("<IIHH", bank, offset, kind, flags, 100 + anchor, 70 + anchor % 20)
        struct.pack_into("<H", bank, offset + 0x28, 17)
        struct.pack_into("<II", bank, offset + 0x30, 1 << (actor + 4), 10000 + anchor)
    return bytes(bank)


def mutated_xml(mode, rows):
    original = (MOD / "data" / "forge.xml").read_text(encoding="utf-8")
    item, _, actor, anchor, _, _, _, _ = rows[0]
    lines = original.splitlines(keepends=True)
    item_index = next(i for i, line in enumerate(lines) if f'<item id="{item}" ' in line)
    line = lines[item_index]
    marker = f'derive="{anchor}" icon-source="anchor"'
    assert marker in line
    if mode == "wrong-anchor":
        other = next(row[3] for row in rows if row[2] == actor and row[3] != anchor)
        line = line.replace(marker, f'derive="{other}" icon-source="anchor"', 1)
    elif mode == "missing-icon":
        line = line.replace(' icon-source="anchor"', "", 1)
    elif mode == "conflicting-icon":
        line = line.replace(marker, marker + ' icon-owner="art" icon-key="art:item" icon-revision="1"', 1)
    lines[item_index] = line
    if mode == "missing-name":
        name_index = next(i for i, entry in enumerate(lines)
                          if f'<string table="0" id="{item}" role="0" ' in entry)
        del lines[name_index]
    return "".join(lines).encode("utf-8")


def main():
    separator = sys.argv.index("--driver-end")
    driver = sys.argv[1:separator]
    if not driver or sys.argv[separator + 1:]:
        raise SystemExit("supply native driver before --driver-end")
    rows = catalogue()
    with tempfile.TemporaryDirectory(prefix="p4-fusion-rows-") as temporary:
        root = Path(temporary)
        catalog = root / "catalog.txt"
        catalog.write_text("".join(f"{item} {actor} {anchor} {value} {element} {hit} {name}\n"
                                   for item, _, actor, anchor, value, element, hit, name in rows),
                           encoding="ascii")
        for mode in MODES:
            image = root / "fixture.iso"
            image.write_bytes(input_disc(effective_ai=True, item=item_bank(rows, mode)))
            xml = root / "forge.xml"
            xml.write_bytes(mutated_xml(mode, rows))
            environment = dict(os.environ, P4PC_ISO=str(image), P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script")
            environment.pop("P4PC_OVERRIDE_DIR", None)
            result = subprocess.run([*driver, "--fusion", mode, str(xml), str(catalog)], env=environment,
                                    capture_output=True, text=True, timeout=60, check=False)
            if result.returncode:
                raise AssertionError(f"{mode}: exit {result.returncode}\n{result.stdout}\n{result.stderr}")
    print("fusion_arts_rows: authored rows through synthetic CPU builder and refusal boundaries; no game witness")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
