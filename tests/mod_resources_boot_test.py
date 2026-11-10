"""Synthetic package manifests and PNG pixels through the real boot producer."""
import binascii
import os
import pathlib
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib
from game_elements_inputs_test import input_disc

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


def png(pixels, bad_zlib=False):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", binascii.crc32(kind + data) & 0xffffffff)
    rows = b"\0" + pixels[:8] + b"\0" + pixels[8:]
    compressed = bytearray(zlib.compress(rows))
    if bad_zlib:
        compressed[-1] ^= 1  # Invalid Adler-32, with a valid enclosing chunk CRC.
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 2, 2, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", compressed) + chunk(b"IEND", b""))


class ResourceBoot(unittest.TestCase):
    def run_mode(self, mode):
        with tempfile.TemporaryDirectory(prefix="p4 resource boot ") as temporary:
            root = pathlib.Path(temporary)
            for ident in ("boot.a", "boot.b"):
                package = root / ident
                package.mkdir()
                revision = "0" if mode == "bad-revision" and ident == "boot.a" else "7"
                manifest = (f"[mod]\nid={ident}\nname=Image fixture\nversion=1.0\napi=1.1\npriority=0\norder=0\n")
                if mode != "unrelated":
                    manifest += f"[resource-image boot.a:icon]\nfile=icon.png\nformat=png\nschema=1\nrevision={revision}\n"
                if mode.startswith("item-ui") and ident == "boot.a":
                    manifest += "[table-source boot.a:tables]\nfile=tables.xml\nformat=p4-tables-xml\nschema=1\nrevision=3\n"
                    item = bytearray(68)
                    struct.pack_into("<I", item, 0, 1)
                    key = "boot.a:missing" if mode == "item-ui-missing" else "boot.a:icon"
                    icon_revision = 8 if mode == "item-ui-revision" else 7
                    xml = ('<p4-tables schema="1">'
                           f'<item id="1536" key="boot.a:item" metadata="{item.hex()}" '
                           f'icon-owner="boot.a" icon-key="{key}" icon-revision="{icon_revision}"/>'
                           '<string table="0" id="1536" role="0" bytes="4900"/>'
                           '<string table="0" id="1536" role="1" bytes="4400"/>'
                           '</p4-tables>')
                    (package / "tables.xml").write_text(xml, encoding="utf-8")
                (package / "mod.ini").write_text(manifest, encoding="utf-8")
                pixels = (bytes([255, 0, 0, 255, 0, 255, 0, 128, 0, 0, 255, 64, 255, 255, 0, 0])
                          if ident == "boot.a" else bytes(range(1, 17)))
                encoded = bytearray(png(pixels, mode == "bad-zlib" and ident == "boot.a"))
                if mode == "bad-crc" and ident == "boot.a":
                    encoded[29] ^= 1
                if mode != "missing" or ident != "boot.a":
                    (package / "icon.png").write_bytes(encoded)
            enabled = "" if mode == "empty" else "boot.a,boot.b" if mode in ("override", "override-early") else "boot.a"
            env = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script",
                       P4PC_CONFIG="none", SDL_AUDIODRIVER="dummy")
            if mode.startswith("item-ui"):
                image = root / "inputs.iso"
                image.write_bytes(input_disc(effective_ai=True))
                env["P4PC_ISO"] = str(image)
            env.pop("P4PC_MOD_WITNESS", None)
            # Wine's helper processes may retain stderr after the driver exits;
            # regular files avoid waiting for pipe EOF from unrelated helpers.
            with tempfile.TemporaryFile(mode="w+") as stdout, tempfile.TemporaryFile(mode="w+") as stderr:
                result = subprocess.run([*DRIVER, mode, str(root), enabled], env=env,
                                        stdout=stdout, stderr=stderr, text=True, check=False)
                stdout.seek(0)
                stderr.seek(0)
                out, err = stdout.read(), stderr.read()
            if result.returncode == 77:
                self.skipTest("P4PC_ELF unavailable")
            self.assertEqual(result.returncode, 0, out + err)
            if mode in ("override", "override-early"):
                self.assertIn("provider boot.b overrides provider boot.a", err)
            return out

    def test_single_provider_real_png_and_resource_lifecycle(self):
        self.assertIn("pinned drain passed", self.run_mode("single"))

    def test_early_decode_without_graphics_context(self):
        self.assertIn("early decode required no graphics context", self.run_mode("early-only"))

    def test_early_ownership_conflict_without_graphics_context(self):
        self.assertIn("early decode required no graphics context", self.run_mode("override-early"))

    def test_higher_provider_wins_but_descriptor_owner_is_preserved(self):
        self.assertIn("pinned drain passed", self.run_mode("override"))

    def test_empty_catalog_exposes_null_in_both_phases(self):
        for mode in ("empty", "unrelated"):
            with self.subTest(mode=mode):
                self.assertIn("pinned drain passed", self.run_mode(mode))

    def test_late_failure_keeps_real_selected_provider(self):
        self.assertIn("late failure retained its selected provider", self.run_mode("late-fail"))

    def test_invalid_or_changed_source_refused_before_exposure(self):
        for mode in ("bad-crc", "bad-zlib", "bad-revision", "missing", "changed"):
            with self.subTest(mode=mode):
                self.assertIn("refused before publication", self.run_mode(mode))
    def test_item_ui_real_source_and_resource_products(self):
        self.assertIn("real table and resource products resolved and drained", self.run_mode("item-ui"))

    def test_item_ui_missing_or_wrong_revision_refused_early(self):
        for mode in ("item-ui-missing", "item-ui-revision"):
            with self.subTest(mode=mode):
                self.assertIn("unresolved authored reference refused", self.run_mode(mode))


if __name__ == "__main__":
    unittest.main()
