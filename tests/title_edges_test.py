#!/usr/bin/env python3
import tempfile
from pathlib import Path
import unittest

from title_edges import edge_leaks
from png_diff import write_png


class TitleEdgesTest(unittest.TestCase):
    def image(self, width, height, mutate=None):
        pixels = bytearray(width * height * 3)
        # A real 3D scene may reach the left edge below the top letterbox.
        for y in range((height * 42 + 447) // 448, height):
            pixels[y * width * 3 + 2] = 255
        if mutate:
            mutate(pixels)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "title.png"
            write_png(path, width, height, pixels)
            return edge_leaks(path)

    def test_fractional_frame_and_user_window(self):
        for width, height in ((1366, 768), (1277, 954)):
            self.assertEqual(self.image(width, height), (width, height, 0, 0))

    def test_first_physical_row_leak(self):
        result = self.image(1366, 768, lambda p: p.__setitem__((1366 - 1) * 3, 1))
        self.assertEqual(result[2:], (1, 0))

    def test_first_column_inside_letterbox_leak(self):
        result = self.image(1277, 954, lambda p: p.__setitem__((954 * 20 // 448) * 1277 * 3 + 1, 1))
        self.assertEqual(result[2:], (0, 1))


if __name__ == "__main__":
    unittest.main()
