"""Real native source ledger/package/codec fixture; no retail inputs."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class NativeTableSources(unittest.TestCase):
    def test_exact_source_ownership_and_round_trip(self):
        with tempfile.TemporaryDirectory(prefix="p4-table-sources-") as temporary:
            root = Path(temporary)
            for directory, manifest, section, identity, version in (
                ("mods", "mod.ini", "mod", "source.mod", "1.2.3"),
                ("packs", "pack.ini", "pack", "source.pack", "9.8.7"),
            ):
                package = root / directory / "dependency"
                package.mkdir(parents=True)
                (package / manifest).write_text(
                    f"[{section}]\nid={identity}\nname=Authored source fixture\n"
                    f"version={version}\napi=1.1\npriority=0\norder=0\n",
                    encoding="utf-8",
                )
            environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0",
                               P4PC_INPUT="script", P4PC_CONFIG="none",
                               SDL_AUDIODRIVER="dummy")
            result = subprocess.run([*DRIVER, str(root / "mods"), str(root / "packs")],
                                    env=environment, capture_output=True, text=True, check=False)
            if result.returncode == 77:
                self.skipTest("guest address mapping unavailable")
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn("game_table_sources: exact frozen entries", result.stdout)


if __name__ == "__main__":
    unittest.main()
