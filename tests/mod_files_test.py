"""Synthetic mod manifests/data only: never mounts or copies a retail asset."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest

# The runtime's own API minor: release cuts change only include/p4pc/packages.h.
API_MINOR = int(re.search(r"#define P4_PACKAGE_API_MINOR (\d+)", (pathlib.Path(__file__).resolve().parents[1]
                          / "include/p4pc/packages.h").read_text(encoding="utf-8")).group(1))
# Host twins may insert Wine before the executable. Keep that command prefix
# intact rather than mistaking its second word for a unittest test selector.
separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class FileMods(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="p4-mod-files-")
        self.addCleanup(self.tmp.cleanup)
        self.root = pathlib.Path(self.tmp.name)

    def mod(self, directory, ident, priority=0, order=0, api="1.0", files=None):
        path = self.root / directory
        (path / "files").mkdir(parents=True)
        (path / "mod.ini").write_text(
            f"[mod]\nid={ident}\nname=Example {ident}\nversion=1.2\napi={api}\npriority={priority}\norder={order}\n",
            encoding="utf-8",
        )
        for key, text in (files or {}).items():
            target = path / "files" / key
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text, encoding="ascii")
        return path
    def legacy(self, directory, number, files=None, wrapped=True):
        path = self.root / directory
        path.mkdir(parents=True)
        guid = f"00000000-0000-0000-0000-{number:012x}"
        (path / "Mod.xml").write_text(
            f"<Mod><Id>{guid}</Id><Title>Legacy &amp; data</Title><Version>1.0</Version><Game>Persona4</Game></Mod>",
            encoding="utf-8",
        )
        for key, text in (files or {}).items():
            target = path / ("Data" if wrapped else "") / key
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text, encoding="ascii")
        return path, f"compendium.{guid}"


    def run_mods(self, ids, developer="-", root=None):
        result = subprocess.run(
            [*DRIVER, str(root or self.root), ids, str(developer), "cdrom0:\\data\\test.txt;1"],
            text=True, capture_output=True, check=True,
        )
        return result.stdout, result.stderr

    def test_disabled_performs_no_discovery(self):
        out, _ = self.run_mods("", root=self.root / "absent")
        self.assertIn("activation=0 count=0 conflicts=0", out)
        self.assertIn("file=missing", out)

    def test_priority_order_id_and_merged_directory(self):
        self.mod("first-on-disk", "z", priority=-2, files={"DATA/TEST.TXT": "low", "DATA/LOW.TXT": "a"})
        self.mod("second-on-disk", "b", order=2, files={"DATA/TEST.TXT": "bbbb"})
        self.mod("third-on-disk", "a", order=2, files={"DATA/TEST.TXT": "aaa"})
        self.mod("fourth-on-disk", "disabled", priority=100, files={"DATA/TEST.TXT": "disabled"})
        out, err = self.run_mods("b,z,a")
        self.assertIn("activation=0 count=3 conflicts=3", out)
        self.assertLess(out.index("mod=z"), out.index("mod=a"))
        self.assertLess(out.index("mod=a"), out.index("mod=b"))
        self.assertIn("file=bbbb size=4", out)
        self.assertEqual(out.count("entry=TEST.TXT"), 1)
        self.assertIn("entry=TEST.TXT dir=0 size=4", out)
        self.assertIn("entry=LOW.TXT dir=0 size=1", out)
        self.assertIn("a < b (winner)", err)

    def test_developer_override_wins(self):
        self.mod("mod", "example", files={"DATA/TEST.TXT": "mod"})
        dev = self.root / "developer"
        (dev / "DATA").mkdir(parents=True)
        (dev / "DATA/TEST.TXT").write_text("developer", encoding="ascii")
        out, _ = self.run_mods("example", dev)
        self.assertIn("file=developer size=9", out)
        self.assertIn("entry=TEST.TXT dir=0 size=9", out)

    def test_missing_duplicate_and_incompatible_refused(self):
        self.mod("valid", "valid", files={"DATA/TEST.TXT": "valid"})
        self.mod("future", "future", api="2.0")
        self.mod("new-minor", "new-minor", api=f"1.{API_MINOR + 1}")
        self.mod("dup-one", "duplicate")
        self.mod("dup-two", "duplicate")
        for ids in ("valid,missing", "valid,future", "new-minor", "duplicate", "valid,valid", "valid,", "../valid"):
            with self.subTest(ids=ids):
                out, _ = self.run_mods(ids)
                self.assertIn("activation=-1 count=0 conflicts=0", out)

    def test_supported_minor_uses_common_runtime(self):
        self.mod("supported", "supported", api="1.6", files={"DATA/TEST.TXT": "minor"})
        out, _ = self.run_mods("supported")
        self.assertIn("activation=0 count=1", out)
        self.assertIn("file=minor size=5", out)

    def test_two_digit_minor_is_a_number(self):
        # Compare numeric minors against the released runtime, not the
        # pre-capability planned release numbers.
        for minor in (1, 9, 10):
            api = f"1.{minor}"
            with self.subTest(api=api):
                name = "minor" + api.replace(".", "-")
                self.mod(name, name, api=api, files={"DATA/TEST.TXT": "minor"})
                out, _ = self.run_mods(name)
                expected = "activation=0 count=1" if minor <= API_MINOR else "activation=-1 count=0"
                self.assertIn(expected, out)

    def test_lowercase_refused(self):
        self.mod("lower", "lower", files={"DATA/test.txt": "bad"})
        out, _ = self.run_mods("lower")
        self.assertIn("activation=-1 count=0", out)

    def test_exposed_filesystem_links_refused(self):
        mod = self.mod("linked", "linked")
        source = self.root / "outside"
        out, _ = self.run_mods("")
        # A POSIX launcher can create file symlinks that Wine intentionally
        # hides from Win32 attributes. Use the directory-link form exposed by
        # both APIs in that cross-OS case; native POSIX/Windows retain file links.
        cross_os = "filesystem=win32" in out and os.name != "nt"
        if cross_os:
            source.mkdir()
        else:
            source.write_text("outside", encoding="ascii")
        try:
            (mod / "files/ESCAPE").symlink_to(source, target_is_directory=cross_os)
        except OSError as error:
            self.skipTest(f"host cannot create a link fixture: {error}")
        out, _ = self.run_mods("linked")
        self.assertIn("activation=-1 count=0", out)

    def test_required_and_duplicate_manifest_fields(self):
        mod = self.mod("example", "example")
        original = (mod / "mod.ini").read_text(encoding="utf-8")
        for text in (original.replace("order=0\n", ""), original + "id=example\n", original + "unknown=x\n"):
            (mod / "mod.ini").write_text(text, encoding="utf-8")
            out, _ = self.run_mods("example")
            self.assertIn("activation=-1 count=0", out)

    def test_requires_capabilities_at_admission(self):
        mod = self.mod("needs", "needs", files={"DATA/TEST.TXT": "needs"})
        original = (mod / "mod.ini").read_text(encoding="utf-8")
        for requires in ("core", "core@1, capabilities@1", "advanced@1,events.load"):
            with self.subTest(requires=requires):
                (mod / "mod.ini").write_text(original + f"requires = {requires}\n", encoding="utf-8")
                out, _ = self.run_mods("needs")
                self.assertIn("activation=0 count=1", out)
                self.assertIn("file=needs size=5", out)
        for requires, reason in (("core@999", "Requires capability core@999, but this build provides core@1"),
                                 ("made.up@1", "Requires capability made.up@1, which this build does not provide")):
            with self.subTest(requires=requires):
                (mod / "mod.ini").write_text(original + f"requires = {requires}\n", encoding="utf-8")
                out, err = self.run_mods("needs")
                self.assertNotIn("file=needs", out)
                self.assertIn(reason, err)
        for requires in ("", "core,", "Core@1", "core@0", "core@one"):
            with self.subTest(requires=requires):
                (mod / "mod.ini").write_text(original + f"requires = {requires}\n", encoding="utf-8")
                out, err = self.run_mods("needs")
                self.assertIn("activation=-1 count=0", out)
                self.assertIn("Invalid requires= list", err)


    def test_compendium_wrapper_preserves_original_host_case(self):
        _, ident = self.legacy("wrapped", 1, {"Data/test.txt": "wrapped"})
        out, _ = self.run_mods(ident)
        self.assertIn("activation=0 count=1 conflicts=0", out)
        self.assertIn("file=wrapped size=7", out)
        self.assertIn("entry=TEST.TXT dir=0 size=7", out)

    def test_compendium_direct_tree_and_native_share_one_order(self):
        _, ident = self.legacy("direct", 2, {"data/test.txt": "legacy", "data/low.txt": "a"}, wrapped=False)
        self.mod("modern", "modern", order=1, files={"DATA/TEST.TXT": "modern"})
        out, err = self.run_mods(f"modern,{ident}")
        self.assertIn("activation=0 count=2 conflicts=1", out)
        self.assertIn("file=modern size=6", out)
        self.assertIn("entry=LOW.TXT dir=0 size=1", out)
        self.assertEqual(out.count("entry=TEST.TXT"), 1)
        self.assertIn(f"file conflict DATA/TEST.TXT: {ident} < modern (winner)", err)

    def test_compendium_code_only_metadata_refused_with_luau_explanation(self):
        path, ident = self.legacy("code", 3)
        (path / "patch.pnach").write_text("code patch", encoding="ascii")
        out, err = self.run_mods(ident)
        self.assertIn("activation=-1 count=0 conflicts=0", out)
        self.assertIn("rewrite this feature as a Luau mod", err)

    def test_malformed_native_manifest_never_downgrades_to_compendium(self):
        path, ident = self.legacy("invalid-native", 4, {"Data/test.txt": "legacy"})
        (path / "mod.ini").write_text("[mod]\nid=broken\n", encoding="ascii")
        out, _ = self.run_mods(ident)
        self.assertIn("activation=-1 count=0 conflicts=0", out)

    def test_unused_native_code_inputs_are_not_silently_ignored(self):
        path = self.mod("modern", "modern", files={"DATA/TEST.TXT": "valid"})
        (path / "HostFS").mkdir()
        out, err = self.run_mods("modern")
        self.assertIn("activation=-1 count=0 conflicts=0", out)
        self.assertIn("rewrite this feature as a Luau mod", err)

    def test_empty_compendium_prebuild_is_not_executed(self):
        path, ident = self.legacy("empty-prebuild", 5, {"Data/test.txt": "valid"})
        (path / "prebuild.bat").write_bytes(b"")
        out, _ = self.run_mods(ident)
        self.assertIn("activation=0 count=1 conflicts=0", out)
        self.assertIn("file=valid size=5", out)

if __name__ == "__main__":
    unittest.main()
