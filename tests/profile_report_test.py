#!/usr/bin/env python3
"""Resolve sampled PCs against module ranges and the exact PE preferred base."""
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest

REPORT = Path(__file__).resolve().parents[1] / "tools/profile_report.py"


class ProfileReportTest(unittest.TestCase):
    def report(self, log, symbols, image_base=None, pe64=True):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "boot.log").write_text(log, encoding="utf-8")
            (root / "symbols.txt").write_text(symbols, encoding="utf-8")
            command = [sys.executable, str(REPORT), str(root / "boot.log"), str(root / "symbols.txt")]
            if image_base is not None:
                image = bytearray(128)
                image[:2] = b"MZ"
                struct.pack_into("<I", image, 60, 64)
                image[64:68] = b"PE\0\0"
                struct.pack_into("<H", image, 88, 0x20B if pe64 else 0x10B)
                struct.pack_into("<Q" if pe64 else "<I", image, 112 if pe64 else 116, image_base)
                (root / "game.exe").write_bytes(image)
                command += ["--image", str(root / "game.exe")]
            return subprocess.run(command, capture_output=True, text=True)

    def test_x64_aslr_and_external_caller(self):
        result = self.report(
            "MOD 7ff600000000 100000 p4pc64w.exe\n"
            "MOD 7ffb00000000 10000 SDL3.dll\n"
            "PROF 7ff600001010\n"
            "PROF 7ffb00000100 7ff600002010\n"
            "PROF 700000000000\n",
            "140001000 T game_update\n140002000 T draw_submit\n140003000 T last_symbol\n",
            0x140000000)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("3 samples", result.stdout)
        self.assertRegex(result.stdout, r"33\.3%  game_update")
        self.assertRegex(result.stdout, r"33\.3%  \?")
        self.assertIn("[SDL3.dll] <- draw_submit", result.stdout)
        self.assertNotIn("last_symbol", result.stdout)

    def test_historical_low_word_x64_log(self):
        result = self.report("MOD ed4c0000 100000 p4pc64w.exe\nPROF ed4c1010\n",
                             "140001000 T game_update\n", 0x140000000)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertRegex(result.stdout, r"100\.0%  game_update")

    def test_fixed_i386_image_with_guest_ram_gap(self):
        result = self.report("MOD f0000 5000000 p4pc.exe\nPROF 2100010\n",
                             "2100000 T _game_update\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertRegex(result.stdout, r"100\.0%  game_update")

    def test_pe32_relocation(self):
        result = self.report("MOD 10000000 100000 p4pc.exe\nPROF 10001010\n",
                             "401000 T _game_update\n", 0x400000, pe64=False)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertRegex(result.stdout, r"100\.0%  game_update")

    def test_linux_image_symbol(self):
        result = self.report("MOD 400000 100000 p4pc64\nPROF 401010\n",
                             "400000 R __ehdr_start\n401000 T game_update\n")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertRegex(result.stdout, r"100\.0%  game_update")

    def test_missing_relocation_input_is_not_a_false_hotspot(self):
        result = self.report("MOD 7ff600000000 100000 p4pc64w.exe\nPROF 7ff600001010\n",
                             "140001000 T game_update\n140002000 T last_symbol\n")
        self.assertEqual(result.returncode, 2)
        self.assertIn("pass --image", result.stderr)
        self.assertNotIn("100.0%", result.stdout)


if __name__ == "__main__":
    unittest.main()
