#!/usr/bin/env python3
"""Judge the fog-dream command menu's background, excluding actors, HUD and horizon."""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from png_diff import read_png
from scale_diff import box_pixel


def discontinuities(path):
    width, height, channels, pixels = read_png(path)
    def luminance(x, y):
        if (width, height) == (640, 448):
            p = (y * width + x) * channels
            r, g, b = pixels[p:p + 3]
        else:
            r, g, b = box_pixel(pixels, width, height, channels, x, y, 640, 448)
        return 0.2126 * r + 0.7152 * g + 0.0722 * b

    # Average along each reported seam to reject the animated speckle. Use nearby
    # flanks rather than absolute brightness: the genuine fog gradient is retained.
    ys = list(range(35, 80)) + list(range(105, 160))
    columns = {x: sum(luminance(x, y) for y in ys) / len(ys) for x in range(329, 347)}
    rows = {y: sum(luminance(x, y) for x in range(160, 460)) / 300 for y in range(82, 100)}

    def depression(values, centre):
        flanks = [values[k] for k in range(centre - 4, centre - 1)]
        flanks += [values[k] for k in range(centre + 2, centre + 5)]
        return sum(flanks) / len(flanks) - values[centre]

    return (max(depression(columns, x) for x in range(333, 343)),
            max(depression(rows, y) for y in range(86, 96)))


def main():
    vertical, horizontal = discontinuities(sys.argv[1])
    # Pinned retail software GS: 1.147 / 1.159 levels; corrected NVIDIA: 1.991 / 1.861.
    # The depth-poisoning regression measures 7.775 / 6.286, not a faint retail band.
    limit = 3.0
    ok = max(vertical, horizontal) <= limit
    print(f"fog_seam: vertical={vertical:.3f} horizontal={horizontal:.3f} "
          f"luminance levels, limit={limit:.1f}: {'PASS' if ok else 'FAIL'}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
