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
MODES = ("good", "gated-off", "absent", "no-fingerprint", "bad-label", "order", "stride", "undeclared",
         "past-end", "length", "no-stride", "bad-revision", "unknown-producer", "unknown-input", "twice",
         "source", "source-oversize", "pairs", "pairs-open", "pairs-gap", "pairs-repeat", "pairs-long", "pairs-skill",
         "encount", "encount-short", "encount-partial", "encount-selector", "encount-formation",
         "encount-kind", "encount-enemy", "encount-item")
# Modes whose battle-table completion maps fixed guest addresses (77: unavailable).
NATIVE = ("good", "pairs", "encount")


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
                (package / "mod.ini").write_text(manifest, encoding="utf-8", newline="\n")
                image = root / "inputs.iso"
                image.write_bytes(input_disc(effective_ai=True,
                                             encount=encount_blocks() if mode.startswith("encount") else None))
                environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script",
                                   P4PC_CONFIG="none", SDL_AUDIODRIVER="dummy", P4PC_ISO=str(image))
                environment.pop("P4PC_OVERRIDE_DIR", None)
                result = subprocess.run([*DRIVER, mode, str(root)], env=environment,
                                        capture_output=True, text=True, check=False)
                if result.returncode == 77 and mode in NATIVE:
                    self.skipTest("fixed guest mapping unavailable")
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(f"game_table_derived: {mode} passed", result.stdout)


if __name__ == "__main__":
    unittest.main()
