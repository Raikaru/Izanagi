"""Enemy knowledge over the real package core and source ledger; synthetic data only."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class EnemyKnowledge(unittest.TestCase):
    def test_keys_route_participant_handler_and_save(self):
        for mode in ("expanded", "unexpanded"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory(prefix="p4-knowledge-") as temporary:
                root = Path(temporary)
                package = root / "knowledge"
                package.mkdir()
                (package / "mod.ini").write_text(
                    "[mod]\nid=knowledge.mod\nname=Knowledge fixture\nversion=1.0.0\n"
                    "api=1.1\npriority=0\norder=0\n",
                    encoding="utf-8",
                )
                environment = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0",
                                   P4PC_INPUT="script", P4PC_CONFIG="none", SDL_AUDIODRIVER="dummy")
                result = subprocess.run([*DRIVER, mode, str(root)], env=environment,
                                        capture_output=True, text=True, check=False)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                self.assertIn("game_knowledge: ", result.stdout)


if __name__ == "__main__":
    unittest.main()
