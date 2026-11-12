"""Pack root, kill switch and real VFS activation with synthetic assets only."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class PackFrontend(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="p4-packs-config-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.ini = self.root / "fixture.ini"

    def pack(self, root):
        package = root / "synthetic"
        (package / "files" / "DATA").mkdir(parents=True)
        (package / "pack.ini").write_text(
            "[pack]\nid=catalog.pack\nname=Synthetic pack\nversion=1.0\napi=1.1\npriority=0\norder=0\n",
            encoding="utf-8")
        (package / "files" / "DATA" / "HELLO.BIN").write_bytes(b"hello")

    def invoke(self, mode, *, env_root=None, config_root=None, no_config=False, switch="0", witness=False):
        text = "[packs]\nenabled=catalog.pack\n"
        if config_root is not None:
            text += "directory=" + str(config_root) + "\n"
        self.ini.write_text(text, encoding="utf-8")
        environment = dict(os.environ, P4PC_CONFIG="none" if no_config else str(self.ini),
                           P4PC_MODS="0", P4PC_PACKS=switch, P4PC_INPUT="script")
        environment.pop("P4PC_PACK_DIR", None)
        environment.pop("P4PC_MOD_WITNESS", None)
        if env_root is not None:
            environment["P4PC_PACK_DIR"] = str(env_root)
        if witness:
            environment["P4PC_MOD_WITNESS"] = "1"
        result = subprocess.run([*DRIVER, mode], env=environment, capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("pack frontend: PASS", result.stdout)

    def test_default_catalog_with_selector_disabled(self):
        self.pack(self.root / "packs")
        self.invoke("scan")

    def test_default_absent_is_empty(self):
        self.invoke("empty")

    def test_explicit_missing_or_nondirectory_is_error(self):
        self.invoke("error", config_root=self.root / "missing")
        bad = self.root / "not-a-directory"
        bad.write_text("file", encoding="ascii")
        self.invoke("error", env_root=bad)

    def test_ini_and_environment_precedence(self):
        ini_root, env_root = self.root / "ini root", self.root / "env root"
        self.pack(ini_root)
        self.invoke("scan", config_root=ini_root)
        self.pack(env_root)
        self.invoke("scan", config_root=self.root / "missing", env_root=env_root)
        self.invoke("scan", env_root=env_root, no_config=True)

    def test_no_settings_path_or_explicit_root_is_error(self):
        self.invoke("error", no_config=True)

    def test_real_activation_and_kill_switch(self):
        self.pack(self.root / "packs")
        self.invoke("disabled")
        self.invoke("active", switch="1", witness=True)

    def test_invalid_switch_is_error(self):
        self.invoke("invalid-switch", switch="bad")

    def test_scripted_selection_requires_explicit_witness(self):
        self.pack(self.root / "packs")
        self.invoke("automation", switch="1")

    def test_duplicate_pack_id_refuses_publication(self):
        root = self.root / "packs"
        self.pack(root)
        shutil.copytree(root / "synthetic", root / "duplicate")
        self.invoke("duplicate", switch="1", witness=True)


if __name__ == "__main__":
    unittest.main()
