"""[table-derived] producers over real package boot and a synthetic disc only."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
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
         "delta-full", "delta-undeclared", "delta-forget", "names", "names-order", "names-text")
# Modes whose battle-table completion maps fixed guest addresses (77: unavailable).
NATIVE = ("good", "pairs", "encount", "delta", "names")
# Modes with only the delta package enabled (no producer).
DELTA_ONLY = ("delta-only", "delta-twice", "delta-range", "delta-field", "delta-missing", "delta-full",
              "delta-undeclared", "delta-forget")
# The delta package's records per mode; "delta" also runs the producer first.
DELTAS = {
    "delta": ('<skill-delta id="11" power="80" cost="0x6"/>\n<skill-delta id="2" accuracy="90" critical="20"/>\n'
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
                if mode in DELTAS:
                    deltas = root / "delta"
                    (deltas / "data").mkdir(parents=True)
                    (deltas / "mod.ini").write_text(
                        "[mod]\nid=derived.delta\nname=Synthetic deltas\nversion=1.0.0\napi=1.1\npriority=0\n"
                        "order=0\n" + DELTA_SECTION, encoding="utf-8", newline="\n")
                    (deltas / "data" / "deltas.xml").write_text(
                        f'<p4-table-deltas schema="1">\n{DELTAS[mode]}</p4-table-deltas>\n', encoding="utf-8", newline="\n")
                    enabled = "derived.delta" if mode in DELTA_ONLY else "derived.owner,derived.delta"
                image = root / "inputs.iso"
                image.write_bytes(input_disc(effective_ai=True,
                                             encount=encount_blocks() if mode.startswith("encount") else None,
                                             party=party_block() if mode.startswith("delta") else None))
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
