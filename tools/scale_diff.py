#!/usr/bin/env python3
"""Compare any physical rendering resolution with the same frame at1x.

  python tools/scale_diff.py frame_1x.png frame_scaled.png [--json]

Exact separable area/box integration supports independent, fractional axis ratios.
Shift search uses physical pixels and extends edge texels, never wraps the other
side of the picture. Edge coverage at a finer raster can differ from the1x point
sample; a best shift alone is not proof that the GS virtual half-pixel is wrong.
NumPy accelerates full-frame comparisons; the pixel oracle needs only stdlib.
"""
import argparse
from functools import lru_cache
import json
import math
import sys

from png_diff import read_png, write_png, capture_path
import mod_witness

try:
    import numpy as np
except ImportError:
    np = None


@lru_cache(maxsize=32)
def axis_weights(source, target):
    """Exact integer overlap numerators, with denominator target."""
    if source <= 0 or target <= 0:
        raise ValueError("image dimensions must be positive")
    return tuple(tuple((s, min((d + 1) * source, (s + 1) * target) - max(d * source, s * target))
                       for s in range(d * source // target, ((d + 1) * source + target - 1) // target))
                 for d in range(target))


def box_pixel(pixels, width, height, channels, x, y, target_width, target_height):
    """A single RGB pixel, independently matching the native integer-weight oracle."""
    result = [0, 0, 0]
    for sy, wy in axis_weights(height, target_height)[y]:
        for sx, wx in axis_weights(width, target_width)[x]:
            offset = (sy * width + sx) * channels
            weight = wx * wy
            for c in range(3):
                result[c] += pixels[offset + c] * weight
    return tuple(v / (width * height) for v in result)


def load(path):
    width, height, channels, pixels = read_png(path)
    return np.frombuffer(pixels, dtype=np.uint8).reshape(height, width, channels)[:, :, :3]


def area_axis(a, target, axis, shift=0):
    source = a.shape[axis]
    if shift == 0 and source % target == 0:
        shape = list(a.shape)
        shape[axis:axis + 1] = [target, source // target]
        return a.reshape(shape).mean(axis=axis + 1)
    shape = list(a.shape)
    shape[axis] += 1
    prefix = np.empty(shape, dtype=np.float64)
    first = [slice(None)] * a.ndim
    first[axis] = 0
    prefix[tuple(first)] = 0
    rest = [slice(None)] * a.ndim
    rest[axis] = slice(1, None)
    np.cumsum(a, axis=axis, dtype=np.float64, out=prefix[tuple(rest)])
    edges = np.arange(target + 1, dtype=np.float64) * (source / target) - shift
    bounded = edges.clip(0, source)
    indices = np.floor(bounded).astype(np.int64)
    fractions = bounded - indices
    weights_shape = [1] * a.ndim
    weights_shape[axis] = target + 1
    integrals = np.take(prefix, indices, axis=axis)
    integrals += np.take(a, np.minimum(indices, source - 1), axis=axis) * fractions.reshape(weights_shape)
    # Constant extension outside either edge, not np.roll's opposite-edge data.
    if shift != 0:
        extra = edges - bounded
        integrals += np.take(a, np.where(extra < 0, 0, source - 1), axis=axis) * extra.reshape(weights_shape)
    return np.diff(integrals, axis=axis) / (source / target)


def box_down(a, shape):
    return area_axis(area_axis(a, shape[0], 0), shape[1], 1)


def mad(a, b):
    return float(np.abs(a - b).mean()) / 255.0 * 100.0


def compare(a, b, radius=None):
    sy, sx = b.shape[0] / a.shape[0], b.shape[1] / a.shape[1]
    zero = mad(a, box_down(b, a.shape[:2]))
    best = (zero, 0, 0)
    ry, rx = (math.ceil(sy), math.ceil(sx)) if radius is None else (radius, radius)
    for dy in range(-ry, ry + 1):
        reduced_y = area_axis(b, a.shape[0], 0, dy)
        for dx in range(-rx, rx + 1):
            value = mad(a, area_axis(reduced_y, a.shape[1], 1, dx))
            if value < best[0] - 1e-10:
                best = (value, dx, dy)
    return {"base": [a.shape[1], a.shape[0]], "physical": [b.shape[1], b.shape[0]],
            "scale_x": sx, "scale_y": sy, "mad_percent": zero,
            "best_mad_percent": best[0], "best_shift_physical": [best[1], best[2]]}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("base")
    ap.add_argument("scaled")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--radius", type=int, help="physical-pixel search radius (default one virtual pixel per axis)")
    ap.add_argument("--write-downsample")
    ap.add_argument("--max-mad", type=float)
    args = ap.parse_args()
    if np is None:
        ap.error("full-frame comparison requires NumPy: python -m pip install numpy")
    if args.radius is not None and args.radius < 0:
        ap.error("radius must be nonnegative")
    a, b = load(args.base), load(args.scaled)
    result = compare(a, b, args.radius)
    mode = mod_witness.receipt_fields(sources=(capture_path(args.base), capture_path(args.scaled)))
    if mode["mod_witness"]:
        result.update(mode)
    if args.write_downsample:
        rgb = np.rint(box_down(b, a.shape[:2])).clip(0, 255).astype(np.uint8)
        write_png(args.write_downsample, a.shape[1], a.shape[0], rgb.tobytes())
        if mode["mod_witness"]:
            mod_witness.tag_output(args.write_downsample)
    if args.json:
        print(json.dumps(result))
    else:
        if mode["mod_witness"]:
            print("mod_witness=true reference_eligible=false")
        print("%.6gx%.6g: area-downsampled MAD %.4f%% (shift0,0); best %.4f%% at shift%d,%d physical px" %
              (result["scale_x"], result["scale_y"], result["mad_percent"], result["best_mad_percent"],
               *result["best_shift_physical"]))
    return int(args.max_mad is not None and result["mad_percent"] > args.max_mad)


if __name__ == "__main__":
    sys.exit(main())
