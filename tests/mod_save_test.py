"""Actual C codec/package fixture; no retail input, renderer or parity claim."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class NativeSaveCodec(unittest.TestCase):
    def test_owned_codec_and_exact_package_requirements(self):
        with tempfile.TemporaryDirectory(prefix="p4-mod-save-codec-") as temporary:
            root = Path(temporary)
            package = root / "dependency"
            (package / "files" / "DATA").mkdir(parents=True)
            (package / "files" / "DATA" / "CODEC.BIN").write_bytes(b"authored fixture data")
            (package / "mod.ini").write_text(
                "[mod]\nid=save.dep\nname=Authored save dependency\n"
                "version=1.2.3\napi=1.1\npriority=0\norder=0\n", encoding="utf-8")
            other = root / "other"
            (other / "files" / "DATA").mkdir(parents=True)
            (other / "files" / "DATA" / "OTHER.BIN").write_bytes(b"second authored dependency")
            (other / "mod.ini").write_text(
                "[mod]\nid=save.other\nname=Second authored save dependency\n"
                "version=9.8.7\napi=1.1\npriority=1\norder=1\n", encoding="utf-8")
            needs = root / "needs"
            needs.mkdir()
            (needs / "mod.ini").write_text(
                "[mod]\nid=save.needs\nname=Needs a ghost\nversion=1.0\napi=1.6\npriority=2\norder=0\n"
                "[require save.ghost]\nversion=1.0\n", encoding="utf-8")
            ghost = root / "ghost"
            ghost.mkdir()
            (ghost / "mod.ini").write_text(
                "[mod]\nid=save.ghost\nname=Ghost\nversion=1.0\napi=1.0\npriority=3\norder=0\n", encoding="utf-8")
            environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0",
                               P4PC_INPUT="script", P4PC_CONFIG="none",
                               SDL_AUDIODRIVER="dummy")
            environment.pop("P4PC_MOD_WITNESS", None)
            result = subprocess.run([*DRIVER, str(root)], env=environment,
                                    capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("native sidecar codec: synthetic fixture complete", result.stdout)
            result = subprocess.run([*DRIVER, str(root), "history"], env=environment,
                                    capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("native sidecar history: synthetic fixture complete", result.stdout)
            result = subprocess.run([*DRIVER, str(root), "refused"], env=environment,
                                    capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("native sidecar refused owner: synthetic fixture complete", result.stdout)
            sidecar = root / "requirement.p4mods"
            for mode in ("requirement-capture", "requirement-load"):
                result = subprocess.run([*DRIVER, str(root), mode, str(sidecar)], env=environment,
                                        capture_output=True, text=True, check=False)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn(f"native sidecar {mode}: synthetic fixture complete", result.stdout)


if __name__ == "__main__":
    unittest.main()
