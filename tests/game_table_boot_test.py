"""Actual package/table-source boot products over synthetic contained files only."""
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile
import unittest
from game_elements_inputs_test import input_disc

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class TableSourceBoot(unittest.TestCase):
    def test_declared_source_lifecycle(self):
        for mode in ("empty", "good", "families", "item-only", "skill-only", "bound-category", "unbound-category", "bad-format", "zero-revision", "bad-schema",
                     "outside", "missing", "unknown-record", "truncated", "split-dtd"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="p4-table-boot-") as temporary:
                root = Path(temporary)
                package = root / "owner"
                package.mkdir()
                format_name = "runtime-luau" if mode == "bad-format" else "p4-tables-xml"
                revision = "0" if mode == "zero-revision" else "37"
                schema = "2" if mode == "bad-schema" else "1"
                filename = "../outside.xml" if mode == "outside" else "missing.xml" if mode == "missing" else "tables.xml"
                (package / "mod.ini").write_text(
                    "[mod]\nid=source.owner\nname=Synthetic tables\nversion=1.2.3\napi=1.1\npriority=0\norder=0\n"
                    f"[table-source xml:combat]\nfile={filename}\nformat={format_name}\nschema={schema}\nrevision={revision}\n",
                    encoding="utf-8",
                )
                xml = " " * 5000 + '<p4-tables schema="1"><element id="19" flags="7" family="0" order="0" key="xml:leaf" display-name="Synthetic" short-name="Test"/></p4-tables>'
                if mode == "families":
                    metadata = (bytes([2]) + bytes(39)).hex()
                    item = bytearray(68)
                    struct.pack_into("<I", item, 0, 1)
                    struct.pack_into("<I", item, 4, 0x12345678)
                    struct.pack_into("<h", item, 0x18, -3)
                    rows = (
                        '<element id="4294967295" flags="7" family="0" order="1" key="xml:wide" display-name="Wide" short-name="Wide"/>'
                        '<skill id="1024" active="1" element="4294967295" element-aux="255" inheritance-group="18" inheritance-rate="-128" '
                        f'key="xml:skill" metadata="{metadata}" presentation="01000200"/>'
                        f'<item id="1536" key="xml:item" metadata="{item.hex()}" '
                        'icon-owner="art" icon-key="art:item" icon-revision="9" require0-id="7" require0-quantity="2"/>'
                        '<string table="0" id="1536" role="0" bytes="41808000"/>'
                        '<string table="0" id="1536" role="1" bytes="4400"/>'
                        '<string table="1" id="1024" role="0" bytes="5300"/>'
                        '<string table="1" id="1024" role="1" bytes="4400"/>'
                    )
                    xml = xml.removesuffix("</p4-tables>") + rows + "</p4-tables>"
                elif mode == "item-only":
                    item = bytearray(68)
                    struct.pack_into("<I", item, 0, 1)
                    xml = " " * 5000 + (
                        '<p4-tables schema="1">'
                        f'<item id="1536" key="xml:item" metadata="{item.hex()}" '
                        'icon-owner="art" icon-key="art:item" icon-revision="9"/>'
                        '<string table="0" id="1536" role="0" bytes="4900"/>'
                        '<string table="0" id="1536" role="1" bytes="4400"/>'
                        '</p4-tables>'
                    )
                elif mode == "skill-only":
                    metadata = (bytes([2]) + bytes(39)).hex()
                    xml = " " * 5000 + (
                        '<p4-tables schema="1">'
                        '<skill id="1024" active="1" element="2" element-aux="255" inheritance-group="18" inheritance-rate="-128" '
                        f'key="xml:skill" metadata="{metadata}" presentation="01000200"/>'
                        '<string table="1" id="1024" role="0" bytes="5300"/>'
                        '<string table="1" id="1024" role="1" bytes="4400"/>'
                        '</p4-tables>'
                    )
                elif mode in ("bound-category", "unbound-category"):
                    xml = xml.removesuffix("</p4-tables>") + (
                        '<category id="6" key="xml:cards" filter-a="64" filter-b="128" stock-policy="1" '
                        'icon-owner="art" icon-key="art:cards" icon-revision="9" '
                        'action-owner="cards" action-key="cards:apply" action-revision="7"/>'
                        '<string table="4" id="6" role="0" bytes="4300"/>'
                        '<string table="4" id="6" role="1" bytes="4400"/>'
                        '</p4-tables>'
                    )
                if mode == "unknown-record":
                    xml = '<p4-tables schema="1"><not-a-table/></p4-tables>'
                elif mode == "truncated":
                    xml = '<p4-tables schema="1"><element'
                elif mode == "split-dtd":
                    xml = " " * 4092 + '<!DOCTYPE p4-tables SYSTEM "outside"><p4-tables schema="1"/>'
                (package / "tables.xml").write_text(xml, encoding="utf-8")
                (root / "outside.xml").write_text(xml, encoding="utf-8")
                environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0",
                                   P4PC_INPUT="script", P4PC_CONFIG="none", SDL_AUDIODRIVER="dummy")
                environment.pop("P4PC_OVERRIDE_DIR", None)
                environment.pop("P4PC_ISO", None)
                if mode in ("good", "families", "item-only", "skill-only", "bound-category", "unbound-category"):
                    image = root / "inputs.iso"
                    image.write_bytes(input_disc(effective_ai=True))
                    environment["P4PC_ISO"] = str(image)
                result = subprocess.run([*DRIVER, mode, str(root)], env=environment,
                                        capture_output=True, text=True, check=False)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn("game_table_boot: real contained source plans", result.stdout)


if __name__ == "__main__":
    unittest.main()
