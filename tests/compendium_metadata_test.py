import pathlib
import subprocess
import sys
import tempfile
import unittest

COMMAND = []


class CompendiumMetadata(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="p4-compendium-")
        self.root = pathlib.Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def package(self, folder="package", payload="Data/DATA/ONE.DAT", xml=None):
        directory = self.root / folder
        target = directory / payload
        target.parent.mkdir(parents=True)
        target.write_bytes(b"data")
        if xml is not None:
            (directory / "Mod.xml").write_text(xml, encoding="utf-8")
        return directory

    def inspect(self, directory):
        result = subprocess.run(
            [*COMMAND, str(directory), directory.name], capture_output=True,
            encoding="utf-8", errors="strict", timeout=20, check=True,
        )
        values = dict(line.split("=", 1) for line in result.stdout.splitlines() if "=" in line)
        return values, result.stderr

    def test_conventional_wrapper_metadata_and_no_mutation(self):
        xml = """<?xml version="1.0" encoding="utf-8"?><Mod>
<Id>35A8D87C-2CB9-4BEE-AF42-CE2E61EF3830</Id><Game>Persona4</Game>
<Title>Audio &amp; tables</Title><Version>1.2</Version><Author>Fixture</Author></Mod>"""
        directory = self.package(payload="Data/BGM/04.ADX", xml=xml)
        (directory / "prebuild.bat").write_bytes(b"")
        before = (directory / "Mod.xml").read_bytes()
        values, _ = self.inspect(directory)
        self.assertEqual(values["metadata"], "0")
        self.assertEqual(values["safe"], "0")
        self.assertEqual(values["id"], "compendium.35a8d87c-2cb9-4bee-af42-ce2e61ef3830")
        self.assertEqual(values["name"], "Audio & tables")
        self.assertEqual(values["version"], "1.2")
        self.assertEqual(pathlib.Path(values["files"]), directory / "Data")
        self.assertEqual((directory / "Mod.xml").read_bytes(), before)

    def test_direct_tree_and_stable_folder_identity(self):
        first = self.package("Same Package", "BTL/BATTLE/ONE.TBL")
        values, _ = self.inspect(first)
        self.assertEqual(values["metadata"], "0")
        self.assertEqual(values["safe"], "0")
        self.assertEqual(pathlib.Path(values["files"]), first)
        self.assertEqual(values["version"], "unversioned")
        nested = self.root / "other"
        nested.mkdir()
        second = nested / first.name
        (second / "BTL/BATTLE").mkdir(parents=True)
        (second / "BTL/BATTLE/ONE.TBL").write_bytes(b"data")
        moved, _ = self.inspect(second)
        self.assertEqual(moved["id"], values["id"])

    def test_portrait_style_mixed_case_container(self):
        directory = self.package(payload="Data/Data/BUSTUP/ONE.BIN")
        values, _ = self.inspect(directory)
        self.assertEqual(values["metadata"], "0")
        self.assertEqual(pathlib.Path(values["files"]), directory / "Data")
        self.assertEqual(values["safe"], "0")

    def test_entities_cdata_and_namespaced_metadata(self):
        xml = """<x:Mod xmlns:x="urn:fixture"><x:Game>persona4</x:Game>
<x:Title>Tools &amp; Data &#x263A; <![CDATA[<extra>]]></x:Title>
<x:Version> 2.0 </x:Version></x:Mod>"""
        values, _ = self.inspect(self.package(xml=xml))
        self.assertEqual(values["metadata"], "0")
        self.assertEqual(values["name"], "Tools & Data ☺ <extra>")
        self.assertEqual(values["version"], "2.0")

    def test_wrong_game_and_invalid_metadata_refused(self):
        documents = [
            "<Mod><Game>Persona4Golden</Game></Mod>",
            "<Mod><Title>One</Title><Title>Two</Title></Mod>",
            "<Mod><Id>not-a-guid</Id></Mod>",
            "<Mod><Title>broken</Mod>",
            "<Other/>",
            "<Mod>" + "<x>" * 33 + "</x>" * 33 + "</Mod>",
            "<Mod><Description>" + "x" * 65537 + "</Description></Mod>",
            '<!DOCTYPE Mod [<!ENTITY host SYSTEM "file:///not-a-mod-file">]><Mod><Title>&host;</Title></Mod>',
        ]
        for index, xml in enumerate(documents):
            with self.subTest(index=index):
                values, _ = self.inspect(self.package(str(index), xml=xml))
                self.assertEqual(values["metadata"], "-1")

    def test_absent_disc_tree_not_recognized(self):
        directory = self.root / "not-a-package"
        directory.mkdir()
        (directory / "README.txt").write_text("not game data", encoding="utf-8")
        values, _ = self.inspect(directory)
        self.assertEqual(values["metadata"], "1")

    def test_code_inputs_refused_with_luau_explanation(self):
        for index, name in enumerate(("cheat.pnach", "patch.IRX", "SLUS_217.82", "prebuild.bat", "native.dll")):
            with self.subTest(name=name):
                directory = self.package(str(index))
                (directory / name).write_bytes(b"not empty")
                values, error = self.inspect(directory)
                self.assertEqual(values["safe"], "-1")
                self.assertIn("rewrite this feature as a Luau mod", error)
        directory = self.package("hostfs")
        (directory / "HostFS").mkdir()
        values, error = self.inspect(directory)
        self.assertEqual(values["safe"], "-1")
        self.assertIn("rewrite this feature as a Luau mod", error)

    def test_disguised_elf_and_pe_refused(self):
        for index, data in enumerate((b"\x7fELF" + bytes(64), b"MZ" + bytes(58) + b"\x40\0\0\0PE\0\0")):
            with self.subTest(index=index):
                directory = self.package(str(index))
                (directory / "Data/DATA/ONE.DAT").write_bytes(data)
                values, error = self.inspect(directory)
                self.assertEqual(values["safe"], "-1")
                self.assertIn("rewrite this feature as a Luau mod", error)


if __name__ == "__main__":
    split = sys.argv.index("--driver-end")
    COMMAND = sys.argv[1:split]
    if not COMMAND:
        raise SystemExit("supply the native driver command before --driver-end")
    unittest.main(argv=[sys.argv[0], *sys.argv[split + 1:]])
