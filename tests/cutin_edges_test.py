#!/usr/bin/env python3
"""Boundary tests for the production cut-in coverage oracle, without retail data."""
import tempfile
from pathlib import Path
import unittest

from cutin_edges import coverage
from png_diff import write_png


class CutinEdges(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / "frame.png"

    def image(self, pixels, width=20, height=448):
        write_png(self.path, width, height, bytes(pixels))

    def test_orange_cannot_leak_one_pixel(self):
        pixels = bytearray([224, 100, 0] * (20 * 448))
        self.image(pixels)
        self.assertTrue(coverage(self.path, "allout-intro")[0])
        pixels[-3] = 199
        self.image(pixels)
        self.assertFalse(coverage(self.path, "allout-intro")[0])

    def band(self, start=224, end=353, left=True, right=True):
        pixels = bytearray([180, 60, 0] * (20 * 448))
        for y in range(start, end):
            for x, enabled in ((0, left), (19, right)):
                if enabled:
                    p = (y * 20 + x) * 3
                    pixels[p:p + 3] = bytes([10, 60, 130])
        self.image(pixels)
        return coverage(self.path, "cast-eyes")[0]

    def test_both_edges_are_required(self):
        self.assertTrue(self.band())
        self.assertFalse(self.band(left=False))
        self.assertFalse(self.band(right=False))
        self.assertFalse(self.band(left=False, right=False))

    def test_band_height_cannot_follow_horizontal_scale(self):
        self.assertFalse(self.band(start=190, end=370))
        self.assertFalse(self.band(start=250, end=280))


if __name__ == "__main__":
    unittest.main()
