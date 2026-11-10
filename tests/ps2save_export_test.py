#!/usr/bin/env python3
"""Exercise real PSU/card writers with synthetic bytes, never retail save data."""
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import mod_witness
import ps2save


class PS2ExportTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.folder = self.root / "BASLUS-21782Save0"
        self.folder.mkdir()
        self.files = {self.folder.name: b"synthetic guest save\x00\xff", "icon.sys": b"synthetic icon", "empty": b""}
        for name, data in self.files.items():
            (self.folder / name).write_bytes(data)

    def test_ordinary_psu_and_both_real_card_formats_round_trip(self):
        folder, files = ps2save.parse(ps2save.pack_dir(self.folder))
        self.assertEqual(folder, self.folder.name)
        self.assertEqual(dict(files), self.files)
        for ecc in (False, True):
            with self.subTest(ecc=ecc):
                image = ps2save.build_card([(folder, files)], ecc=ecc)
                self.assertEqual(dict(ps2save.Card(image).saves())[folder], files)

    def test_native_pair_and_witness_metadata_are_never_ps2_files(self):
        names = (*ps2save.NATIVE_SAVE_METADATA, ".P4MODS", "save" + mod_witness.SIDECAR)
        for name in names:
            with self.subTest(name=name):
                metadata = self.folder / name
                metadata.write_bytes(b"opaque native state; not a migration")
                try:
                    with self.assertRaises(ValueError):
                        ps2save.pack_dir(self.folder)
                    with self.assertRaisesRegex(ValueError, "no approved retail-safe migration"):
                        ps2save.build_card([(self.folder.name, [(name, b"opaque")])])
                    self.assertEqual(metadata.read_bytes(), b"opaque native state; not a migration")
                finally:
                    metadata.unlink()
        self.assertEqual({name: (self.folder / name).read_bytes() for name in self.files}, self.files)

    def test_native_metadata_directory_cannot_be_silently_filtered_out(self):
        (self.folder / ".p4mods").mkdir()
        with self.assertRaisesRegex(ValueError, "no approved retail-safe migration"):
            ps2save.pack_dir(self.folder)

    def test_folder_card_nested_native_state_is_refused_before_retail_transfer(self):
        (self.folder / ".p4mods-commit").write_bytes(b"native transaction marker")
        with self.assertRaisesRegex(ValueError, "no approved retail-safe migration"):
            ps2save.refuse_ps2_export(self.root)

    def test_persisted_directory_witness_and_copied_file_taint_are_refused(self):
        environment = {mod_witness.VARIABLE: "1", "P4PC_INPUT": "script"}
        mod_witness.begin(self.folder, environment)
        self.assertTrue(mod_witness.witness_output(self.folder))
        with self.assertRaisesRegex(ValueError, "cannot be a retail reference"):
            ps2save.pack_dir(self.folder)
        destination = self.root / "copied-save"
        destination.mkdir()
        copied = destination / "save"
        copied.write_bytes(self.files[self.folder.name])
        mod_witness.copy_output_mode(self.folder / self.folder.name, copied)
        with self.assertRaises(ValueError):
            ps2save.pack_dir(destination)

    def test_every_inherited_witness_presence_refuses_even_empty_card_export(self):
        for value in ("", "0", "1", "unexpected"):
            with self.subTest(value=value), mock.patch.dict(os.environ, {mod_witness.VARIABLE: value}):
                with self.assertRaisesRegex(ValueError, "refuses P4PC_MOD_WITNESS"):
                    ps2save.pack_dir(self.folder)
                with self.assertRaisesRegex(ValueError, "refuses P4PC_MOD_WITNESS"):
                    ps2save.build_card([])

    def test_cli_refusal_never_creates_or_truncates_output(self):
        (self.folder / ".p4mods").write_bytes(b"required native state")
        for command in ("dir2psu", "mkcard"):
            for existing in (False, True):
                with self.subTest(command=command, existing=existing):
                    output = self.root / (command + str(existing))
                    if existing:
                        output.write_bytes(b"keep existing output")
                    with self.assertRaisesRegex(ValueError, "no approved retail-safe migration"):
                        ps2save.main(["ps2save.py", command, str(self.folder), str(output)])
                    if existing:
                        self.assertEqual(output.read_bytes(), b"keep existing output")
                    else:
                        self.assertFalse(output.exists())
        self.assertEqual((self.folder / ".p4mods").read_bytes(), b"required native state")

    def test_ordinary_cli_outputs_still_decode_and_card_rename_is_preserved(self):
        psu, card = self.root / "ordinary.psu", self.root / "ordinary.ps2"
        self.assertEqual(ps2save.main(["ps2save.py", "dir2psu", str(self.folder), str(psu)]), 0)
        self.assertEqual(dict(ps2save.parse(psu.read_bytes())[1]), self.files)
        name = "BASLUS-21782Save7"
        self.assertEqual(ps2save.main(["ps2save.py", "mkcard", str(self.folder), str(card), name]), 0)
        expected = {(name if key == self.folder.name else key): value for key, value in self.files.items()}
        self.assertEqual(dict(dict(ps2save.Card(card.read_bytes()).saves())[name]), expected)


if __name__ == "__main__":
    unittest.main()
