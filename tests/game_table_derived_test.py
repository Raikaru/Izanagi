"""[table-derived] producers over real package boot and a synthetic disc only."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
from game_elements_inputs_test import input_disc

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]

SECTION = "[table-derived {key}]\nproducer={producer}\nrevision={revision}\ninputs={inputs}\n"
DELTA_SECTION = "[table-delta rebalance]\nfile=data/deltas.xml\nrevision=1\n"
MODES = ("good", "gated-off", "absent", "no-fingerprint", "bad-label", "order", "stride", "undeclared",
         "past-end", "length", "no-stride", "bad-revision", "unknown-producer", "unknown-input", "twice",
         "source", "source-oversize", "pairs", "pairs-open", "pairs-gap", "pairs-repeat", "pairs-long", "pairs-skill",
         "encount", "encount-short", "encount-partial", "encount-selector", "encount-formation",
         "encount-kind", "encount-enemy", "encount-item",
         "delta", "delta-only", "delta-conflict", "delta-twice", "delta-range", "delta-field", "delta-missing",
         "delta-full", "delta-undeclared", "delta-forget", "names", "names-order", "names-text",
         "delta-hit-range", "delta-hits", "delta-split",
         "delta-weapon", "delta-ailment", "delta-ailment-range", "delta-heal-on", "delta-heal-off",
         "delta-cost-bounds", "delta-cost-zero", "delta-cost-over", "delta-percent-on", "delta-percent-off",
         "delta-percent-bounds", "delta-percent-kind", "delta-percent-zero", "delta-percent-over",
         "delta-percent-power", "delta-percent-split", "delta-rider-retail-on", "delta-rider-retail-off",
         "delta-rider-produced-on", "delta-rider-produced-off",
         "delta-combined-retail-on", "delta-combined-retail-off",
         "delta-combined-produced-on", "delta-combined-produced-off")
# Modes whose battle-table completion maps fixed guest addresses (77: unavailable).
NATIVE = ("good", "pairs", "encount", "delta", "names", "delta-combined-retail-on", "delta-combined-retail-off",
          "delta-combined-produced-on", "delta-combined-produced-off")
# Modes with only the delta package enabled (no producer).
DELTA_ONLY = ("delta-only", "delta-twice", "delta-range", "delta-field", "delta-missing", "delta-full",
              "delta-undeclared", "delta-forget", "delta-hit-range", "delta-hits", "delta-split", "delta-ailment",
              "delta-ailment-range", "delta-cost-bounds", "delta-cost-zero", "delta-cost-over", "delta-percent-kind",
              "delta-percent-bounds", "delta-percent-zero", "delta-percent-over", "delta-percent-power", "delta-percent-split")
# The delta package's records per mode; "delta" also runs the producer first.
DELTAS = {
    "delta": ('<skill-delta id="11" power="80" cost="0x6"/>\n<skill-delta id="2" accuracy="90" critical="20"/>\n'
              '<skill-delta id="12" min-hits="2" max-hits="4" split="1" ailment-rate="45"/>\n'
              '<learn-delta persona="0xC0" skill="12" level="3"/>\n'
              '<learn-delta persona="0xC6" skill="21" replace="30"/>\n'
              '<learn-delta persona="0xC6" add="31" level="8"/>\n'
              '<learn-delta persona="0xC3" add="62" level="5"/>\n'),
    "delta-only": '<skill-delta id="11" power="80"/>\n<learn-delta persona="0xC0" skill="11" level="1"/>\n',
    "delta-conflict": '<skill-delta id="11" power="80"/>\n',
    "delta-twice": '<skill-delta id="11" power="80"/>\n<skill-delta id="11" power="81"/>\n',
    "delta-range": '<skill-delta id="11" power="10000"/>\n',
    "delta-field": '<skill-delta id="11" power="80" speed="3"/>\n',
    "delta-missing": '<learn-delta persona="0xC0" skill="99" level="3"/>\n',
    "delta-full": '<learn-delta persona="0xC1" add="40" level="9"/>\n',
    "delta-undeclared": '<learn-delta persona="0xC0" add="1500" level="9"/>\n',
    "delta-forget": '<learn-delta persona="0xC2" skill="51" level="7"/>\n',
    "delta-hit-range": '<skill-delta id="2" max-hits="9"/>\n',
    "delta-hits": '<skill-delta id="2" min-hits="3" max-hits="2"/>\n',
    "delta-split": '<skill-delta id="2" split="1"/>\n',
    "delta-weapon": '<skill-delta id="11" max-hits="3"/>\n',
    "delta-ailment": '<skill-delta id="2" ailment-rate="45"/>\n',
    "delta-ailment-range": '<skill-delta id="2" ailment-rate="100"/>\n',
    "delta-cost-bounds": '<skill-delta id="0xC2" cost="1"/>\n<skill-delta id="0xC5" cost="999"/>\n',
    "delta-cost-zero": '<skill-delta id="0xC2" cost="0"/>\n',
    "delta-cost-over": '<skill-delta id="0xC5" cost="1000"/>\n',
    "delta-percent-bounds": '<skill-delta id="0xC1" max-heal="1"/>\n<skill-delta id="0xC4" max-heal="100"/>\n',
    "delta-percent-kind": '<skill-delta id="0xC0" max-heal="25"/>\n',
    "delta-percent-zero": '<skill-delta id="0xC0" max-heal="0"/>\n',
    "delta-percent-over": '<skill-delta id="0xC0" max-heal="101"/>\n',
    "delta-percent-power": '<skill-delta id="0xC0" max-heal="25" power="30"/>\n',
    "delta-percent-split": '<skill-delta id="0xC0" max-heal="25" split="1"/>\n',
}
REFUSALS = {
    "delta-conflict": "both change skill 11 power",
    "delta-twice": "names skill 11 power twice",
    "delta-range": "power 10000 of skill 11 is outside 1..9999",
    "delta-field": "unknown table-delta attribute",
    "delta-missing": "Persona 0xc0 learns no skill 99",
    "delta-full": "Persona 0xc1 has no free learn event for skill 40",
    "delta-undeclared": "learns skill 1500, which no package declares",
    "delta-forget": "Persona 0xc2 forgets skill 51; a learn delta cannot name it",
    "names-order": "item name IDs must be strictly ascending",
    "names-text": "item 9: a name is 1 to 31 printable ASCII characters",
    "delta-hit-range": "max-hits 9 of skill 2 is outside 1..8",
    "delta-hits": "skill 2 min-hits 3 exceeds max-hits 2",
    "delta-split": "skill 2 split needs formula HP damage (effect kind 1 or 14), not kind 0",
    "delta-weapon": "skill 11 takes its hits from the wielder's weapon",
    "delta-ailment": "skill 2 ailment-rate needs a rolled ailment (status mode 1 or 3 and a nonzero mask without "
                     "instant KO), not mode 0 mask 0",
    "delta-ailment-range": "ailment-rate 100 of skill 2 is outside 1..99",
    "delta-cost-zero": "cost 0 of skill 194 is outside 1..999",
    "delta-cost-over": "cost 1000 of skill 197 is outside 1..999",
    "delta-percent-kind": "skill 192 max-heal needs formula HP healing (effect kind 2), not kind 0",
    "delta-percent-zero": "max-heal 0 of skill 192 is outside 1..100",
    "delta-percent-over": "max-heal 101 of skill 192 is outside 1..100",
    "delta-percent-power": "names skill 192 max-heal twice",
    "delta-percent-split": "names skill 192 max-heal twice",
}


def party_block():
    """24 party rows of 622 bytes: 0xC0 learns 10, 11, 12 at 0, 5, 9; 0xC1's 32
    events are all used; 0xC2 learns 51 at 5 and forgets it at 79 (a Golden-style
    upgrade pair); 0xC3 learns 60 at 0, then a stale event past its terminator;
    0xC6 learns 20, 21, 22 at 0, 7, 8."""
    block = bytearray(24 * 622)
    def events(row, learned, kind=1, first=0):
        for index, (level, skill) in enumerate(learned):
            struct.pack_into("<BbH", block, row * 622 + 4 + (first + index) * 4, level, kind, skill)
    events(0, ((0, 10), (5, 11), (9, 12)))
    events(1, tuple((level, 100 + level) for level in range(32)))
    events(2, ((5, 51),))
    events(2, ((79, 51),), kind=2, first=1)
    events(3, ((0, 60),))
    events(3, ((9, 61),), first=2)
    events(6, ((0, 20), (7, 21), (8, 22)))
    return bytes(block)


def encount_blocks():
    """PS2-layout ENCOUNT rows: 4 formations, 2 selectors, the 200-byte block,
    2 weighted groups and 2 record groups, each naming existing rows."""
    formations = b"".join(bytes(8) + struct.pack("<5H", 1 + row, 0, 0, 0, 0) + bytes(6) for row in range(4))
    selectors = bytes(10) + struct.pack("<HBBBBHH", 1, 0, 0, 0, 0, 1, 0)
    weighted = bytes(248) + bytes(8) + struct.pack("<HH", 2, 10) + bytes(236)
    records = bytes(348) + struct.pack("<HH", 5, 7) + bytes(344)
    return (formations, selectors, bytes(200), weighted, records)


def heal_metadata():
    """Synthetic full-heal rows plus an untouched neighbor; no retail data."""
    metadata = bytearray(440 * 40)
    metadata[0] = 2
    metadata[11 * 40 + 0x1e] = 8
    for skill, cost in ((0xc2, 18), (0xc3, 7), (0xc5, 30)):
        start = skill * 40
        metadata[start:start + 40] = bytes((skill + i) & 0xff for i in range(40))
        struct.pack_into("<H", metadata, start + 4, cost)
        metadata[start + 0x11] = 11
        struct.pack_into("<h", metadata, start + 0x12, 100)
    return metadata


def full_heal_deltas():
    """Exercise the actual authored cost-only records, not a copied fixture."""
    source = Path(__file__).resolve().parents[1] / "mods/p4pc.hardmode/data/rebalance.xml"
    root = ET.parse(source).getroot()
    wanted = {"0xC2": "12", "0xC5": "36"}
    rows = [element for element in root.findall("skill-delta") if element.get("id") in wanted]
    assert len(rows) == len(wanted)
    assert {row.get("id"): row.get("cost") for row in rows} == wanted
    assert all(set(row.attrib) == {"id", "cost"} for row in rows)
    return "".join(ET.tostring(row, encoding="unicode") for row in rows)


def percent_metadata():
    metadata = heal_metadata()
    for skill, cost, power in ((0xc0, 3, 40), (0xc1, 6, 100), (0xc3, 7, 50), (0xc4, 12, 140)):
        start = skill * 40
        metadata[start:start + 40] = bytes((skill + i) & 0xff for i in range(40))
        struct.pack_into("<H", metadata, start + 4, cost)
        metadata[start + 0x11] = 2
        struct.pack_into("<h", metadata, start + 0x12, power)
    metadata[0xc0 * 40 + 0x11] = 1  # producer replaces this with formula healing
    return metadata


def percent_deltas():
    source = Path(__file__).resolve().parents[1] / "mods/p4pc.hardmode/data/rebalance.xml"
    root = ET.parse(source).getroot()
    wanted = {
        "0xC0": {"max-heal": "25"}, "0xC1": {"max-heal": "50"},
        "0xC3": {"max-heal": "22", "cost": "12"}, "0xC4": {"max-heal": "42", "cost": "24"},
    }
    rows = [element for element in root.findall("skill-delta") if element.get("id") in wanted]
    assert len(rows) == len(wanted)
    assert all(row.attrib == {"id": row.get("id"), **wanted[row.get("id")]} for row in rows)
    return "".join(ET.tostring(row, encoding="unicode") for row in rows)


def rider_metadata():
    """Synthetic all-target riders: distinguish every byte, with a rolled status."""
    metadata = bytearray(440 * 40)
    metadata[0] = 2
    metadata[11 * 40 + 0x1e] = 8
    for skill in (0x99, 0x9a, 0x9c, 0x9e, 0xa0, 0xa4, 0xa6):
        start = skill * 40
        metadata[start:start + 40] = bytes((skill + i) & 0xff for i in range(40))
        struct.pack_into("<H", metadata, start + 4, 9)
        metadata[start + 0x11] = 1
        struct.pack_into("<h", metadata, start + 0x12, 123)
        metadata[start + 0x18] = 1
        metadata[start + 0x19] = 20
        struct.pack_into("<I", metadata, start + 0x1c, 0x10)
    return metadata


def rider_deltas():
    """Exercise the seven approved XML rows, retaining their other fields."""
    source = Path(__file__).resolve().parents[1] / "mods/p4pc.hardmode/data/rebalance.xml"
    root = ET.parse(source).getroot()
    wanted = {"0x99", "0x9A", "0x9C", "0x9E", "0xA0", "0xA4", "0xA6"}
    rows = [element for element in root.findall("skill-delta") if element.get("id") in wanted]
    return "".join(ET.tostring(row, encoding="unicode") for row in rows)


def combined_metadata():
    """Both approved families share one synthetic SKILL input."""
    metadata = percent_metadata()
    riders = rider_metadata()
    for skill in (0x99, 0x9a, 0x9c, 0x9e, 0xa0, 0xa4, 0xa6):
        metadata[skill * 40:(skill + 1) * 40] = riders[skill * 40:(skill + 1) * 40]
    return metadata


class TableDerivedBoot(unittest.TestCase):
    def test_derived_source_lifecycle(self):
        for mode in MODES:
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="p4-table-derived-") as temporary:
                root = Path(temporary)
                package = root / "owner"
                package.mkdir()
                gate = mode == "gated-off"
                manifest = ("[mod]\nid=derived.owner\nname=Synthetic derived\nversion=1.0.0\n"
                            f"api={'1.2' if gate else '1.1'}\npriority=0\norder=0\n")
                if gate:
                    manifest += "[setting golden]\nkind=toggle\nlabel=GOLDEN\ndefault=off\napply=restart\n"
                manifest += SECTION.format(
                    key="golden",
                    producer="test:other" if mode == "unknown-producer" else "test:golden",
                    revision="2" if mode == "bad-revision" else "3",
                    inputs="skill bogus" if mode == "unknown-input" else
                    "skill, item msg persona" if mode.startswith("pairs") else
                    "skill, item msg encount" if mode.startswith("encount") else "skill, item msg")
                if gate:
                    manifest += "when=golden\n"
                if mode == "twice":
                    manifest += SECTION.format(key="second", producer="test:golden", revision="3", inputs="skill")
                if mode == "delta-conflict":
                    manifest += DELTA_SECTION
                    (package / "data").mkdir()
                    (package / "data" / "deltas.xml").write_text(
                        '<p4-table-deltas schema="1">\n<skill-delta id="11" power="70"/>\n</p4-table-deltas>\n',
                        encoding="utf-8", newline="\n")
                (package / "mod.ini").write_text(manifest, encoding="utf-8", newline="\n")
                enabled = "derived.owner"
                if (mode in DELTAS or mode.startswith(("delta-heal-", "delta-rider-", "delta-combined-")) or
                        mode in ("delta-percent-on", "delta-percent-off")):
                    deltas = root / "delta"
                    (deltas / "data").mkdir(parents=True)
                    (deltas / "mod.ini").write_text(
                        "[mod]\nid=derived.delta\nname=Synthetic deltas\nversion=1.0.0\napi=1.1\npriority=0\n"
                        "order=0\n" + DELTA_SECTION, encoding="utf-8", newline="\n")
                    records = (full_heal_deltas() + percent_deltas() + rider_deltas()
                               if mode.startswith("delta-combined-") else
                               full_heal_deltas() if mode.startswith("delta-heal-") else
                               rider_deltas() if mode.startswith("delta-rider-") else
                               percent_deltas() if mode in ("delta-percent-on", "delta-percent-off") else DELTAS[mode])
                    (deltas / "data" / "deltas.xml").write_text(
                        f'<p4-table-deltas schema="1">\n{records}</p4-table-deltas>\n', encoding="utf-8", newline="\n")
                    if mode not in ("delta-heal-off", "delta-percent-off") and not mode.endswith("-off"):
                        enabled = "derived.delta" if mode in DELTA_ONLY else "derived.owner,derived.delta"
                image = root / "inputs.iso"
                image.write_bytes(input_disc(effective_ai=True,
                                             encount=encount_blocks() if mode.startswith("encount") else None,
                                             party=party_block() if mode.startswith("delta") else None,
                                             skill_metadata=(combined_metadata() if mode.startswith("delta-combined-") else
                                                             heal_metadata() if mode.startswith("delta-heal-") else
                                                             rider_metadata() if mode.startswith("delta-rider-") else
                                                             percent_metadata() if mode in ("delta-percent-on", "delta-percent-off",
                                                                                            "delta-percent-bounds")
                                                             else None)))
                environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script",
                                   P4PC_CONFIG="none", SDL_AUDIODRIVER="dummy", P4PC_ISO=str(image))
                environment.pop("P4PC_OVERRIDE_DIR", None)
                result = subprocess.run([*DRIVER, mode, str(root), enabled], env=environment,
                                        capture_output=True, text=True, check=False)
                if result.returncode == 77 and mode in NATIVE:
                    self.skipTest("fixed guest mapping unavailable")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(f"game_table_derived: {mode} passed", result.stdout)
                if mode in REFUSALS:
                    self.assertIn(REFUSALS[mode], result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
