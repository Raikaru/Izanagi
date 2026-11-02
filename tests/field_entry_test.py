#!/usr/bin/env python3
"""Production guest-memory field-entry and capture sampler regressions."""
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class FieldEntryTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
        if not compiler:
            raise unittest.SkipTest("a native C compiler is required for the memory sampler witness")
        cls.work = tempfile.TemporaryDirectory()
        cls.directory = Path(cls.work.name)
        binary = cls.directory / "field_entry_test"
        subprocess.run([compiler, "-std=c11", "-D_POSIX_C_SOURCE=200809L", "-include", str(ROOT / "include/p4pc/compat.h"),
                        "-I", str(ROOT / "include"), str(ROOT / "tests/field_entry_test.c"),
                        str(ROOT / "src/platform/state_dump.c"), "-o", str(binary)], check=True)
        entries = cls.directory / "entries.jsonl"
        subprocess.run([str(binary), str(entries)], check=True)
        cls.all_rows = [json.loads(line) for line in entries.read_text().splitlines()]
        cls.rows = [row for row in cls.all_rows if not row["capture"]]
        cls.capture_rows = [row for row in cls.all_rows if row["capture"]]

    @classmethod
    def tearDownClass(cls):
        cls.work.cleanup()

    def test_actual_ready_seam_and_repeated_visit(self):
        self.assertEqual([row["flip"] for row in self.rows], [4, 7, 8, 9, 11])
        self.assertEqual([(row["field"], row["room"]) for row in self.rows],
                         [(7, 2), (7, 2), (7, 3), (22, 2), (22, 2)])
        row = self.rows[0]
        self.assertEqual((row["map_state"], row["environment_state"], row["weather_condition"]), (3, 4, 2))
        self.assertEqual(row["calendar_latch_bits"], ["00000002", "00050009", "00010002"])
        self.assertEqual(row["camera_clip_bits"], ["3f000000", "45bb8000", "45600000"])
        self.assertEqual(row["event_flags"][-1], "abc000af")
        self.assertEqual(row["resources"][1], {"type": 1, "ids": [0x400, 0x401], "count": 2})
        self.assertEqual(row["resources"][12]["ids"], [0x3000])
        self.assertEqual(row["environment_transform"][-1], "3f800017")

    def test_capture_samples_loading_scene_without_completing_a_visit(self):
        self.assertEqual(len(self.all_rows), 6)
        self.assertEqual(len(self.capture_rows), 1)
        row = self.capture_rows[0]
        self.assertEqual(row["flip"], 10)
        self.assertEqual(row["event_flags"][-1], "abc0cafe")
        self.assertEqual(row["resources"][1]["ids"], [0x400, 0x402])
        self.assertEqual(self.rows[3]["event_flags"][-1], "abc000af")
        self.assertEqual(self.rows[3]["resources"][1]["ids"], [0x400, 0x401])
        self.assertEqual(self.rows[-1]["flip"], 11)
        self.assertEqual(self.rows[-1]["event_flags"][-1], "abc0cafe")


if __name__ == "__main__":
    unittest.main()
