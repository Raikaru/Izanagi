#!/usr/bin/env python3
"""Check the settled orange intro and both ends of the retail-height blue eye band."""
from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from png_diff import read_png


def coverage(path, surface):
    width, height, channels, pixels = read_png(path)
    if surface == "allout-intro":
        # This exact phase is opaque orange plus additive white dividers.
        # A scene pixel through any edge or interior is not acceptable.
        leaks = sum(pixels[i] < 200 for i in range(0, len(pixels), channels))
        return leaks == 0, f"scene_leaks={leaks}"
    rows = []
    for y in range(height):
        blue = []
        for x in (0, width - 1):
            p = (y * width + x) * channels
            r, g, b = pixels[p:p + 3]
            blue.append(b > r + 8 and b > g + 4)
        if all(blue):
            rows.append(y)
    span = rows[-1] - rows[0] + 1 if rows else 0
    # The band is about130/448 high; widening may not enlarge its Y aperture.
    ok = len(rows) >= height // 8 and height // 4 <= span <= height // 3
    return ok, f"both_edges_blue_rows={len(rows)} band_span={span}/{height}"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("surface", choices=("allout-intro", "cast-eyes"))
    parser.add_argument("images", nargs="+")
    args = parser.parse_args()
    failed = False
    for path in args.images:
        ok, detail = coverage(path, args.surface)
        print(f"cutin_edges: {path}: {detail}: {'PASS' if ok else 'FAIL'}")
        failed |= not ok
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
