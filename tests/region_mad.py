#!/usr/bin/env python3
"""Mean absolute difference of one box of a captured frame from the same box of a raw 640x448 RGB reference, judged against a limit.

usage: region_mad.py <frame.png> <reference.rgb> <x0,y0,x1,y1> <limit percent>

The box is in reference pixels, x1 and y1 exclusive. A whole-frame MAD dilutes a small local defect (the field's ground shadow is 1/300 of the
frame); a gate on its own box sees it. Exits 0 within the limit, 1 above it, 2 when a file or the box is unusable.
"""
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from png_diff import read_png

WIDTH, HEIGHT = 640, 448


def parse_box(text):
    values = [int(part) for part in text.split(",")]
    if len(values) != 4:
        raise ValueError("a box is x0,y0,x1,y1")
    x0, y0, x1, y1 = values
    if not (0 <= x0 < x1 <= WIDTH and 0 <= y0 < y1 <= HEIGHT):
        raise ValueError("box %s is empty or outside %dx%d" % (text, WIDTH, HEIGHT))
    return x0, y0, x1, y1


def region_mad(pixels, channels, reference, box):
    """Percent mean absolute difference over the box; `pixels` and `reference` are 640x448 row-major, `channels` bytes per frame pixel."""
    x0, y0, x1, y1 = box
    total = 0
    for y in range(y0, y1):
        frame_row = (y * WIDTH + x0) * channels
        reference_row = (y * WIDTH + x0) * 3
        for k in range((x1 - x0) * 3):
            pixel, channel = divmod(k, 3)
            total += abs(pixels[frame_row + pixel * channels + channel] - reference[reference_row + k])
    return total / ((x1 - x0) * (y1 - y0) * 3 * 255.0) * 100.0


def main(argv):
    if len(argv) != 5:
        print(__doc__)
        return 2
    try:
        box = parse_box(argv[3])
        limit = float(argv[4])
        width, height, channels, pixels = read_png(argv[1])
        reference = Path(argv[2]).read_bytes()
        if (width, height) != (WIDTH, HEIGHT) or len(reference) != WIDTH * HEIGHT * 3 or channels < 3:
            raise ValueError("frame %dx%d with %d channels, reference %d bytes: both must be %dx%d RGB" % (width, height, channels, len(reference), WIDTH, HEIGHT))
    except (OSError, ValueError) as error:
        print("region_mad: %s" % error)
        return 2
    mad = region_mad(pixels, channels, reference, box)
    ok = mad <= limit
    print("region_mad: %s box %s: %.4f%% (limit %s%%): %s" % (argv[1], argv[3], mad, argv[4], "PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
