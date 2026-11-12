"""Real OS processes/directory locks; no user save roots or mocked lock API."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class DirectoryLock(unittest.TestCase):
    def probe(self, directory):
        return subprocess.run([*DRIVER, "probe", str(directory)], capture_output=True,
                              text=True, timeout=15, check=False)

    def owner_case(self, mode):
        with tempfile.TemporaryDirectory(prefix="p4 card ownership ") as temporary:
            card = Path(temporary) / "card"
            card.mkdir()
            owner = subprocess.Popen([*DRIVER, mode, str(card)], stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            try:
                self.assertEqual(owner.stdout.readline().strip(), "held")
                contender = self.probe(f"{card}/.")
                self.assertEqual(contender.returncode, 4, contender.stdout + contender.stderr)
                self.assertEqual(list(card.iterdir()), [], "ownership must not create a memory-card file")
                out, err = owner.communicate("release\n", timeout=15)
                self.assertEqual(owner.returncode, 23 if mode == "crash" else 0, out + err)
                successor = self.probe(card)
                self.assertEqual(successor.returncode, 0, successor.stdout + successor.stderr)
                self.assertEqual(list(card.iterdir()), [])
            finally:
                if owner.poll() is None:
                    owner.kill()
                    owner.communicate(timeout=15)

    def test_live_owner_is_exclusive_without_card_artifacts(self):
        self.owner_case("hold")

    def test_real_process_death_releases_ownership(self):
        self.owner_case("crash")

    def test_linked_root_is_not_owned(self):
        with tempfile.TemporaryDirectory(prefix="p4 card link ") as temporary:
            root = Path(temporary)
            target, link = root / "target", root / "link"
            target.mkdir()
            try:
                link.symlink_to(target, target_is_directory=True)
            except OSError as exc:
                self.skipTest(f"real symlink unavailable: {exc}")
            result = self.probe(link)
            self.assertEqual(result.returncode, 5, result.stdout + result.stderr)
            self.assertEqual(list(target.iterdir()), [])


if __name__ == "__main__":
    unittest.main()
