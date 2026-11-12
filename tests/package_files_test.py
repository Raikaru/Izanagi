import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

COMMAND = []


class PackageFiles(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="p4-package-index-")
        self.root = pathlib.Path(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def put(self, name, data=b"data"):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        return path

    def inspect(self, flags=0, queries=(), memory=65536):
        result = subprocess.run([*COMMAND, str(self.root), str(flags), str(memory), *queries],
                                capture_output=True, encoding="utf-8", timeout=20, check=True)
        values, entries, resolved = {}, [], {}
        for line in result.stdout.splitlines():
            key, value = line.split("=", 1)
            if key == "entry":
                directory, relative = value.split("|", 1)
                entries.append((int(directory), relative))
            elif key == "resolve":
                canonical, relative = value.split("|", 1)
                resolved[canonical] = relative
            elif key == "listed":
                canonical, directory, relative = value.split("|", 2)
                values.setdefault("listed", []).append((canonical, int(directory), relative))
            elif key == "list_status":
                canonical, status = value.split("|", 1)
                values.setdefault("list_status", {})[canonical] = int(status)
            else:
                values[key] = value
        return values, entries, resolved, result.stderr

    def test_strict_sorted_names_empty_directories_and_lookup(self):
        self.put("DATA/Z.TXT")
        self.put("DATA/A.TXT")
        (self.root / "DATA/EMPTY").mkdir()
        values, entries, found, _ = self.inspect(queries=("DATA/A.TXT", "DATA/EMPTY", "DATA/MISSING", ""))
        self.assertEqual(values["index"], "0")
        self.assertEqual(entries, [(1, "DATA"), (0, "DATA/A.TXT"), (1, "DATA/EMPTY"), (0, "DATA/Z.TXT")])
        self.assertEqual(found, {"DATA/A.TXT": "DATA/A.TXT", "DATA/EMPTY": "DATA/EMPTY", "DATA/MISSING": "<missing>", "": ""})

    def test_compendium_casefold_preserves_real_host_path(self):
        self.put("Data/Bustup/One.BIN")
        values, _, found, _ = self.inspect(1, ("DATA/BUSTUP/ONE.BIN", "DATA/BUSTUP"))
        self.assertEqual(values["index"], "0")
        self.assertEqual(found["DATA/BUSTUP/ONE.BIN"], "Data/Bustup/One.BIN")
        self.assertEqual(found["DATA/BUSTUP"], "Data/Bustup")
        values, _, _, _ = self.inspect(0)
        self.assertEqual(values["index"], "-1")

    def test_disc_tree_does_not_mount_metadata_or_tools(self):
        self.put("Data/One.BIN")
        self.put("Mod.xml", b"<Mod/>")
        self.put("README.txt")
        self.put("docs/note.txt")
        self.put("prebuild.bat", b"")
        values, entries, found, _ = self.inspect(3, ("DATA/ONE.BIN", "MOD.XML", "DOCS/NOTE.TXT"))
        self.assertEqual(values["index"], "0")
        self.assertEqual(entries, [(1, "Data"), (0, "Data/One.BIN")])
        self.assertEqual(found["DATA/ONE.BIN"], "Data/One.BIN")
        self.assertEqual(found["MOD.XML"], "<missing>")
        self.assertEqual(found["DOCS/NOTE.TXT"], "<missing>")

    def test_unmounted_code_still_refused(self):
        self.put("DATA/ONE.BIN")
        self.put("docs/tools/hidden.dat", b"\x7fELF" + bytes(64))
        values, _, _, error = self.inspect(3)
        self.assertEqual(values["index"], "-1")
        self.assertIn("rewrite this feature as a Luau mod", error)

    def test_container_label_must_be_directory(self):
        self.put("DATA")
        values, _, _, _ = self.inspect(3)
        self.assertEqual(values["index"], "-1")

    def test_case_collisions_refused_where_representable(self):
        first = self.put("DATA/ONE.BIN")
        second = self.root / "DATA/one.bin"
        second.write_bytes(b"other")
        if os.path.samefile(first, second):
            self.skipTest("filesystem does not represent case-distinct aliases")
        values, _, _, error = self.inspect(1)
        self.assertEqual(values["index"], "-1")
        self.assertIn("canonical path collision", error)

    def test_index_budget_and_depth(self):
        self.put("DATA/ONE.BIN")
        values, _, _, error = self.inspect(memory=64)
        self.assertEqual(values["index"], "-1")
        self.assertIn("memory budget", error)
        self.put("/".join(["D"] * 33 + ["ONE.BIN"]))
        values, _, _, error = self.inspect(memory=1048576)
        self.assertEqual(values["index"], "-1")
        self.assertIn("depth", error)

    def test_immediate_children_skip_subtrees_without_skipping_siblings(self):
        self.put("Data/A/Deep/One.BIN")
        self.put("Data/A.TXT")
        self.put("Data/A0.TXT")
        self.put("Data/After/Kept.BIN")
        queries = ("@DATA", "@data/a", "@DATA/MISSING", "@DATA/A.TXT", "@")
        values, _, _, _ = self.inspect(1, queries)
        self.assertEqual(values["list_status"], {"DATA": 0, "data/a": 0, "DATA/MISSING": 1, "DATA/A.TXT": 1, "": 0})
        self.assertEqual(values["listed"], [
            ("DATA", 1, "Data/A"), ("DATA", 0, "Data/A.TXT"),
            ("DATA", 0, "Data/A0.TXT"), ("DATA", 1, "Data/After"),
            ("data/a", 1, "Data/A/Deep"), ("", 1, "Data"),
        ])


if __name__ == "__main__":
    split = sys.argv.index("--driver-end")
    COMMAND = sys.argv[1:split]
    if not COMMAND:
        raise SystemExit("supply native driver before --driver-end")
    unittest.main(argv=[sys.argv[0], *sys.argv[split + 1:]])
