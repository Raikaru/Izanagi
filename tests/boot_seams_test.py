#!/usr/bin/env python3
"""Actual TIMER0/libpad boot seams, without the game, SDL devices or lockstep transport."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class BootSeamsTest(unittest.TestCase):
    def compile_run(self, fixture, source, include=()):
        compiler = shutil.which("cc")
        if os.name == "nt" or not compiler:
            self.skipTest("native 64-bit C witnesses run on the Linux check host")
        with tempfile.TemporaryDirectory() as directory:
            binary = str(Path(directory) / "boot-seam")
            subprocess.run([compiler, "-std=c11", "-ffunction-sections", "-fdata-sections",
                            "-I" + str(ROOT / "include"), "-include", str(ROOT / "include/p4pc/compat.h"),
                            *("-I" + str(ROOT / item) for item in include),
                            str(ROOT / "tests" / fixture), str(ROOT / source),
                            "-Wl,--gc-sections", "-o", binary], check=True)
            subprocess.run([binary], check=True)

    def test_timer0_boot_progression(self):
        self.compile_run("timer0_boot_test.c", "src/platform/kernel_init.c")

    def test_pad_boot_negotiation(self):
        self.compile_run("pad_negotiation_test.c", "src/platform/pad/ps2_pad.c", ("src/platform/pad",))


if __name__ == "__main__":
    unittest.main()
