#!/usr/bin/env python3
"""Exercise the same safe-area geometry used by the Android overlay without a device."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


class PadLayoutTest(unittest.TestCase):
    def test_safe_insets(self):
        javac, java = shutil.which("javac"), shutil.which("java")
        if not javac or not java:
            self.skipTest("JDK unavailable")
        root = Path(__file__).resolve().parent.parent
        with tempfile.TemporaryDirectory() as directory:
            subprocess.run([javac, "-d", directory,
                            str(root / "android/app/src/main/java/org/p4pc/game/PadLayout.java"),
                            str(root / "android/app/src/main/java/org/p4pc/game/PadCustomization.java"),
                            str(root / "tests/PadLayoutTest.java")], check=True)
            subprocess.run([java, "-cp", directory, "org.p4pc.game.PadLayoutTest"], check=True)


if __name__ == "__main__":
    unittest.main()
