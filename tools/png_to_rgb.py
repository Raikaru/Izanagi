#!/usr/bin/env python3
"""Converts a 640x448 capture (PNG or lossless JXL) to a raw RGB reference.
usage: png_to_rgb.py in.png out.rgb (Pillow; JXL also needs djxl). Pinned refs are never rewritten by the archiver."""
import sys
from png_diff import open_capture

im = open_capture(sys.argv[1]).convert("RGB")
assert im.size == (640, 448), im.size
open(sys.argv[2], "wb").write(im.tobytes())
