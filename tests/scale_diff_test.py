#!/usr/bin/env python3
"""Exact fractional/per-axis area weights and physical shift boundary semantics."""
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import scale_diff as sd


class AreaTests(unittest.TestCase):
    def test_integer_and_fractional_weights(self):
        self.assertEqual(sd.axis_weights(4, 2), (((0, 2), (1, 2)), ((2, 2), (3, 2))))
        self.assertEqual(sd.axis_weights(3, 2), (((0, 2), (1, 1)), ((1, 1), (2, 2))))
        for source, target in [(1366, 640), (768, 448), (2160, 448), (1, 640)]:
            for row in sd.axis_weights(source, target):
                self.assertEqual(sum(weight for _, weight in row), source)
                self.assertTrue(all(0 <= pixel < source and weight > 0 for pixel, weight in row))
        with self.assertRaises(ValueError):
            sd.axis_weights(0, 640)

    def test_per_axis_rgb_oracle(self):
        pixels = bytes(c for y in range(2) for x in range(3) for c in (x * 60, y * 100, 200, 255))
        self.assertEqual(sd.box_pixel(pixels, 3, 2, 4, 0, 0, 2, 1), (20, 50, 200))
        self.assertEqual(sd.box_pixel(pixels, 3, 2, 4, 1, 0, 2, 1), (100, 50, 200))
        # The optional full-frame accelerator must agree with the independent
        # integer oracle; native gates exercise the same integer rule in C.
        if sd.np is not None:
            image = sd.np.frombuffer(pixels, dtype=sd.np.uint8).reshape(2, 3, 4)[:, :, :3]
            sd.np.testing.assert_allclose(sd.box_down(image, (1, 2)), [[[20, 50, 200], [100, 50, 200]]], atol=1e-12)
            repeated = sd.np.repeat(sd.np.repeat(image, 2, axis=0), 4, axis=1)
            sd.np.testing.assert_array_equal(sd.box_down(repeated, (2, 3)), image)

    def test_shift_never_wraps_opposite_edge(self):
        if sd.np is not None:
            image = sd.np.array([[[0, 0, 0], [10, 10, 10], [20, 20, 20]]], dtype=sd.np.uint8)
            moved = sd.area_axis(image, 3, 1, 1)
            sd.np.testing.assert_array_equal(moved, [[[0, 0, 0], [0, 0, 0], [10, 10, 10]]])
            self.assertEqual(sd.compare(image, image, 1)["best_shift_physical"], [0, 0])


if __name__ == "__main__":
    unittest.main()
