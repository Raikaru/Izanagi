#!/usr/bin/env python3
"""region_mad.py: the box arithmetic, its refusals and its exit codes, on synthetic frames."""
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import zlib

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import region_mad  # noqa: E402


def write_png(path, rows):
    raw = b"".join(b"\x00" + row for row in rows)

    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    Path(path).write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 640, 448, 8, 2, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))


class RegionMad(unittest.TestCase):
    def setUp(self):
        self.dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.dir.cleanup)
        self.png = os.path.join(self.dir.name, "frame.png")
        self.rgb = os.path.join(self.dir.name, "ref.rgb")
        Path(self.rgb).write_bytes(bytes([100]) * (640 * 448 * 3))
        # the frame equals the reference but for a 10x10 patch 40 levels darker at (20,30)
        rows = []
        for y in range(448):
            row = bytearray([100]) * (640 * 3)
            if 30 <= y < 40:
                for x in range(20, 30):
                    row[x * 3:x * 3 + 3] = bytes([60, 60, 60])
            rows.append(bytes(row))
        write_png(self.png, rows)

    def run_tool(self, box, limit):
        return subprocess.run([sys.executable, str(HERE / "region_mad.py"), self.png, self.rgb, box, limit], capture_output=True, text=True)

    def test_patch_is_seen_in_its_box_and_not_in_the_frame(self):
        inside = self.run_tool("20,30,30,40", "15")
        self.assertEqual(inside.returncode, 1, inside.stdout)
        self.assertIn("15.6863%", inside.stdout)  # 40 / 255
        self.assertEqual(self.run_tool("20,30,30,40", "15.7").returncode, 0)
        self.assertEqual(self.run_tool("0,0,640,448", "1").returncode, 0)  # 100 * 40 / (640 * 448 * 255) per cent, far under 1
        self.assertEqual(self.run_tool("200,200,300,300", "0").returncode, 0)

    def test_unusable_inputs_fail_with_code_two(self):
        for box in ("20,30,20,40", "0,0,641,448", "1,2,3", "a,b,c,d"):
            with self.subTest(box=box):
                self.assertEqual(self.run_tool(box, "1").returncode, 2)
        Path(self.rgb).write_bytes(b"short")
        self.assertEqual(self.run_tool("0,0,10,10", "1").returncode, 2)

    def test_parse_box(self):
        self.assertEqual(region_mad.parse_box("1,2,3,4"), (1, 2, 3, 4))


if __name__ == "__main__":
    unittest.main()
