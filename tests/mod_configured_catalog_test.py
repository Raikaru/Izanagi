"""Exercise the real player root resolver without activating payloads or VMs."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class ConfiguredCatalog(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="p4-configured-catalog-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.ini = self.root / "fixture.ini"

    def packages(self, directory, names=("first", "second")):
        directory.mkdir()
        for name in names:
            package = directory / name
            package.mkdir()
            (package / "mod.ini").write_text(
                f"[mod]\nid=catalog.{name}\nname=Authored {name}\nversion=1.0\napi=1.1\npriority=0\norder=0\n"
                "[not_available service]\nvalue=metadata-only\n", encoding="utf-8")
            # Both would make activation refuse: catalog discovery must not
            # index this malformed payload, compile source or prepare services.
            (package / "files").write_text("not a directory", encoding="ascii")
            (package / "main.luau").write_text("local = invalid source", encoding="ascii")

    def invoke(self, *, directory=None, no_config=False, success=True, empty=False, bundled_root=None, bundled=0,
               shadowed=0):
        env = dict(os.environ, P4PC_CONFIG="none" if no_config else str(self.ini),
                   P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script",
                   P4PC_BUNDLED_MOD_DIR=str(bundled_root) if bundled_root else "")
        env.pop("P4PC_MOD_DIR", None)
        if directory is not None:
            env["P4PC_MOD_DIR"] = str(directory)
        result = subprocess.run([*DRIVER, "empty" if empty else "ok" if success else "error"], env=env,
                                capture_output=True, text=True, check=False)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn(f"configured catalog: PASS seen={2 if success and not empty else 0} activation=0 "
                      f"bundled={bundled} shadowed={shadowed}", result.stdout)
        if shadowed:
            self.assertIn("player copy shadows bundled catalog.first 1.0", result.stderr)

    def write_ini(self, root=None):
        text = "[mods]\nenabled=not.installed\n"
        if root is not None:
            text += "directory=" + str(root) + "\n"
        self.ini.write_text(text, encoding="utf-8")

    def test_default_root_is_beside_ini_even_with_selector_killed(self):
        self.packages(self.root / "mods")
        self.write_ini()
        self.invoke()

    def test_missing_default_root_is_an_empty_catalog(self):
        self.write_ini()
        self.invoke(empty=True)

    def test_missing_explicit_root_is_an_error(self):
        self.write_ini(self.root / "missing")
        self.invoke(success=False)

    def test_ini_override_uses_the_same_root_as_startup(self):
        directory = self.root / "ini override"
        self.packages(directory)
        self.write_ini(directory)
        self.invoke()

    def test_environment_root_overrides_ini_without_needing_selected_packages(self):
        directory = self.root / "environment override"
        self.packages(directory)
        self.write_ini(self.root / "must-not-scan")
        self.invoke(directory=directory)

    def test_environment_root_works_without_any_settings_file(self):
        directory = self.root / "explicit root"
        self.packages(directory)
        self.invoke(directory=directory, no_config=True)

    def test_bundled_root_lists_shipped_packages_without_a_player_root(self):
        self.packages(self.root / "shipped")
        self.write_ini()
        self.invoke(bundled_root=self.root / "shipped", bundled=2)

    def test_player_installation_hides_the_bundled_copy_of_its_id(self):
        self.packages(self.root / "mods")
        self.packages(self.root / "shipped", names=("first",))
        self.write_ini()
        self.invoke(bundled_root=self.root / "shipped", bundled=0, shadowed=1)

    def test_missing_bundled_root_is_empty(self):
        self.packages(self.root / "mods")
        self.write_ini()
        self.invoke(bundled_root=self.root / "not-shipped", bundled=0)

    def test_absent_settings_and_root_are_an_explicit_error_not_activation(self):
        self.invoke(no_config=True, success=False)


if __name__ == "__main__":
    unittest.main()
