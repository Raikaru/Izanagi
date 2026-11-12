#!/usr/bin/env python3
"""Reject 3D leakage through the settled title's GS letterbox boundary cells."""
from pathlib import Path
import argparse
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from png_diff import read_png


def edge_leaks(path):
    width, height, channels, pixels = read_png(path)
    # Frame400's top letterbox occupies42 logical rows. Stay inside its first32
    # so this also checks the presenter without judging the moving silhouettes.
    left_rows = max(1, height * 32 // 448)
    top = sum(any(pixels[x * channels:x * channels + 3]) for x in range(width))
    left = sum(any(pixels[y * width * channels:y * width * channels + 3])
               for y in range(left_rows))
    return width, height, top, left


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("images", nargs="+")
    args = parser.parse_args()
    failed = False
    for path in args.images:
        width, height, top, left = edge_leaks(path)
        ok = top == 0 and left == 0
        print(f"title_edges: {path} {width}x{height}: top_leaks={top} "
              f"left_bar_leaks={left}: {'PASS' if ok else 'FAIL'}")
        failed |= not ok
    return int(failed)


if __name__ == "__main__":
    raise SystemExit(main())
