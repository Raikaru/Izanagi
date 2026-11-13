#!/usr/bin/env python3
"""Original UI art for bundled mods, drawn in Persona 4's own UI families
(docs/modding/ui_art.md). Nothing here reads, copies or traces retail pixels
into an asset: the player's disc is only *measured*, in memory.

  python tools/ui_art/ui_art.py check [PNG ...]       style-check art against its family (dependency-free);
                                                      no arguments checks every ASSETS path present
  python tools/ui_art/ui_art.py check --iso PATH ...  also compare against the native family members
                                                      measured from the player's own disc
  python tools/ui_art/ui_art.py measure [--iso PATH]  print the native families' statistics
  python tools/ui_art/ui_art.py build [--out DIR] [--only PREFIX] [--vector-only] [--fonts DIR]
                                                      write the assets; text labels need Pillow and the
                                                      pinned SIL OFL fonts in FONTS (fetched once)

Vector art is deterministic pure Python, so a mod's own generator can call
vector_png(name) and CTest can prove its committed bytes are the generated ones
(tools/physsplit.py does this for the Physical Split icons). Text labels need a
font rasterizer; they are rendered once with Pillow from the pinned fonts and
committed, and `check` keeps them inside their family."""
import argparse
import hashlib
import io
import math
import os
import struct
import sys
import tarfile
import urllib.request
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

# ---------------------------------------------------------------- families
# Alpha ramps are the 16-entry (or fewer) palettes the native atlases use, with
# the PS2's 0..0x80 alpha doubled; every family is white art tinted at draw time
# except camp-label, a two-tone sprite (dark fill, white outline) drawn untinted.
SKILL_RAMP = (0, 14, 30, 48, 66, 80, 98, 118, 140, 156, 172, 188, 206, 224, 240, 255)
HEADER_RAMP = (0, 56, 116, 158, 196, 230, 255)
CARD_SYMBOL_RAMP = (0, 18, 36, 52, 62, 82, 102, 116, 130, 148, 168, 190, 210, 226, 238, 255)
CARD_LABEL_RAMP = (0, 16, 34, 50, 68, 80, 100, 118, 130, 152, 172, 188, 204, 222, 238, 255)
CAMP_GREYS = (45, 59, 81, 101, 116, 136, 159, 192, 220, 240, 255)
CAMP_ALPHAS = (0, 32, 62, 126, 186, 255)
# Battle help-bar prompts ("△ RUSH"): a dark button disc with a coloured
# symbol, then light text shading from white down, all inside a dark outline.
PROMPT_GREYS = (24, 40, 64, 96, 128, 176, 208, 224, 240, 255)
PROMPT_ALPHAS = (0, 32, 64, 128, 192, 255)
PROMPT_ACCENT = (236, 140, 196)  # the Square symbol
MENU_DIGIT_RAMP = (0, 16, 32, 50, 66, 80, 98, 116, 132, 150, 170, 190, 208, 224, 242, 255)
MENU_DASH_RAMP = (0, 124, 174, 254, 255)

# cell: (width range, height range). native: (CVM path, PAK member or None, record indices).
# reference: coverage and median-stroke ranges measured from those records (2026-11-04),
# used when no disc is given; `measure` recomputes them from the player's own disc.
FAMILIES = {
    "element-header": dict(cell=((30, 30), (23, 23)), ramp=HEADER_RAMP, tone="white",
                           native=("init/camp.bin", "c_equip_help_02.spr", tuple(range(0x2E, 0x35))),
                           reference=dict(coverage=(0.157, 0.310), stroke=(3.0, 13.0))),
    "skill-icon": dict(cell=((34, 38), (27, 27)), ramp=SKILL_RAMP, tone="white",
                       native=("init/camp.bin", "c_equip_help_02.spr", tuple(range(0xB8, 0xC3))),
                       reference=dict(coverage=(0.153, 0.488), stroke=(2.0, 15.0))),
    "element-marker": dict(cell=((12, 20), (9, 12)), ramp=HEADER_RAMP, tone="white", native=None,
                           reference=dict(coverage=(0.20, 0.55), stroke=(1.0, 4.0))),
    "velvet-symbol": dict(cell=((72, 72), (77, 77)), ramp=CARD_SYMBOL_RAMP, tone="white",
                          native=("facility/velvet01.spr", None, (0xDF, 0xE1, 0xE3, 0xE5, 0xE7, 0xE9, 0xEB, 0xED, 0xEF)),
                          reference=dict(coverage=(0.067, 0.310), stroke=(5.0, 13.5))),
    "velvet-label": dict(cell=((72, 99), (22, 31)), ramp=CARD_LABEL_RAMP, tone="white",
                         native=("facility/velvet01.spr", None, (0xFC, 0xFD, 0xFE, 0xFF, 0x100, 0x101, 0x103, 0x104, 0x105)),
                         reference=dict(coverage=(0.130, 0.242), stroke=(3.0, 4.0))),
    "velvet-label-small": dict(cell=((38, 72), (18, 18)), ramp=CARD_LABEL_RAMP, tone="white",
                               native=("facility/velvet01.spr", None, (0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA)),
                               reference=dict(coverage=(0.235, 0.322), stroke=(3.0, 4.0))),
    "camp-label": dict(cell=((24, 200), (21, 22)), ramp=CAMP_ALPHAS, tone="camp",
                       native=("init/camp.bin", "c_top_01.spr", tuple(range(0, 7))),
                       reference=dict(coverage=(0.468, 0.608), stroke=(10.0, 29.0))),
    # Measured from the battle help bar's "△RUSH" (71x21 cell, 11 px caps,
    # 17 px disc); no sprite record is located yet, so the range stands alone.
    "battle-prompt": dict(cell=((40, 160), (21, 21)), ramp=PROMPT_ALPHAS, tone="prompt", native=None,
                          reference=dict(coverage=(0.70, 0.85), stroke=(12.0, 21.0))),
    "menu-numeral": dict(cell=((10, 18), (15, 15)), ramp=MENU_DIGIT_RAMP, tone="white",
                         native=("init/camp.bin", "c_equip_01.spr", tuple(range(9, 19))),
                         reference=dict(coverage=(0.219, 0.344), stroke=(3.0, 5.0))),
    "menu-dash": dict(cell=((10, 14), (8, 8)), ramp=MENU_DASH_RAMP, tone="white",
                      native=("init/camp.bin", "c_equip_01.spr", (60,)),
                      reference=dict(coverage=(0.25, 0.25), stroke=(8.0, 8.0))),
    # Persona panel button legend (sdkSpriteDrawFooter, c_tittle_01.spr): button
    # discs 0xA..0xD and words 0xF..0x13, dark 45 bodies inside white outlines.
    "legend-glyph": dict(cell=((23, 23), (25, 25)), ramp=CAMP_ALPHAS, tone="camp",
                         native=("init/camp.bin", "c_tittle_01.spr", (0xA, 0xB, 0xC, 0xD)),
                         reference=dict(coverage=(0.51, 0.51), stroke=(17.0, 17.0))),
    "legend-word": dict(cell=((24, 96), (23, 23)), ramp=CAMP_ALPHAS, tone="camp",
                        native=("init/camp.bin", "c_tittle_01.spr", (0xF, 0x10, 0x11, 0x12, 0x13)),
                        reference=dict(coverage=(0.47, 0.556), stroke=(10.0, 25.0))),
    # Persona panel stat labels (c_equip_help_02 0x2C "St/Ma/En"): 12-pixel caps
    # on rows 5/23/41, white on the header ramp, tinted at draw.
    "stat-strip": dict(cell=((27, 27), (58, 58)), ramp=HEADER_RAMP, tone="white",
                       native=("init/camp.bin", "c_equip_help_02.spr", (0x2C,)),
                       reference=dict(coverage=(0.278, 0.278), stroke=(3.0, 3.0))),
}

# Every asset this tool owns: repository path, family, maker and rectangle.
# "vector:<name>" is drawn by vector_png, "label:<name>" by the Pillow label
# renderer. A rectangle (x, y, w, h) places the art in a transparent
# power-of-two atlas of ATLASES[path] size, as retail sprite packs do, which
# native sprites (p4.ui.sprite) require; None is a standalone image of the
# family cell (Physical Split's element icons, drawn by element-ui).
ASSETS = [
    ("mods/p4pc.physsplit/art/slash.png", "skill-icon", "vector:slash-skill", None),
    ("mods/p4pc.physsplit/art/strike.png", "skill-icon", "vector:strike-skill", None),
    ("mods/p4pc.physsplit/art/pierce.png", "skill-icon", "vector:pierce-skill", None),
    ("mods/p4pc.physsplit/art/slash-header.png", "element-header", "vector:slash-header", None),
    ("mods/p4pc.physsplit/art/strike-header.png", "element-header", "vector:strike-header", None),
    ("mods/p4pc.physsplit/art/pierce-header.png", "element-header", "vector:pierce-header", None),
    ("mods/p4pc.hardmode/art/ko.png", "element-marker", "vector:ko", None),
    ("mods/p4pc.hardmode/art/ailment-panic.png", "element-header", "vector:ailment-panic", None),
    ("mods/p4pc.hardmode/art/ailment-fear.png", "element-header", "vector:ailment-fear", None),
    ("mods/p4pc.hardmode/art/ailment-rage.png", "element-header", "vector:ailment-rage", None),
    ("mods/p4pc.hardmode/art/ailment-silence.png", "element-header", "vector:ailment-silence", None),
    ("mods/p4pc.hardmode/art/ailment-poison.png", "element-header", "vector:ailment-poison", None),
    ("mods/p4pc.hardmode/art/ailment-exhaustion.png", "element-header", "vector:ailment-exhaustion", None),
    ("mods/p4pc.hardmode/art/ailment-enervation.png", "element-header", "vector:ailment-enervation", None),
    ("mods/p4pc.hardmode/art/ailment-freeze.png", "element-header", "vector:ailment-freeze", None),
    ("mods/p4pc.hardmode/art/ailment-shock.png", "element-header", "vector:ailment-shock", None),
    ("mods/p4pc.fusion-arts/art/weapon-fusion.png", "velvet-symbol", "vector:weapon-fusion", (0, 0, 72, 77)),
    ("mods/p4pc.fusion-arts/art/weapon-fusion.png", "velvet-label", "label:weapon-fusion", (80, 0, 99, 31)),
    ("mods/p4pc.fusion-arts/art/weapon-fusion.png", "velvet-label-small", "label:weapon-fusion-small", (80, 40, 72, 18)),
    ("mods/p4pc.hardmode/art/badge.png", "camp-label", "label:badge-hard", (0, 0, 58, 21)),
    ("mods/p4pc.challenge/art/badge.png", "camp-label", "label:badge-challenge", (0, 0, 111, 21)),
    ("mods/p4pc.randomizer/art/badge.png", "camp-label", "label:badge-randomizer", (0, 0, 130, 21)),
    ("mods/p4pc.practice/art/badge.png", "camp-label", "label:badge-practice", (0, 0, 95, 21)),
    ("tests/mod_fixtures/ui_overlay/art/badge.png", "camp-label", "label:badge-witness", (0, 0, 91, 21)),
    ("mods/p4pc.hardmode/art/prompt-ailments.png", "battle-prompt", "label:prompt-ailments", None),
    ("mods/p4pc.hardmode/art/prompt-affinities.png", "battle-prompt", "label:prompt-affinities", None),
    ("tests/mod_fixtures/ui_native_row/art/card.png", "velvet-symbol", "vector:weapon-fusion", (0, 0, 72, 77)),
    ("tests/mod_fixtures/ui_native_row/art/card.png", "velvet-label", "label:witness-card", (80, 0, 99, 31)),
    ("tests/mod_fixtures/ui_native_row/art/card.png", "velvet-label-small", "label:witness-card-small", (80, 40, 72, 18)),
    ("mods/p4pc.skill-affinity/art/mark-plus.png", "menu-numeral", "vector:plus-menu", None),
    ("mods/p4pc.skill-affinity/art/mark-minus.png", "menu-dash", "vector:minus-menu", None),
    ("mods/p4pc.skill-affinity/art/legend.png", "legend-glyph", "label:legend-r2", (0, 0, 23, 25)),
    ("mods/p4pc.skill-affinity/art/legend.png", "legend-word", "label:legend-affinity", (24, 0, 58, 23)),
    ("mods/p4pc.skill-affinity/art/legend.png", "legend-word", "label:legend-resist", (24, 32, 50, 23)),
    ("mods/p4pc.skill-affinity/art/stat-labels.png", "stat-strip", "label:stat-labels", None),
]
ATLASES = {
    "mods/p4pc.fusion-arts/art/weapon-fusion.png": (256, 128),
    "mods/p4pc.hardmode/art/badge.png": (64, 32),
    "mods/p4pc.challenge/art/badge.png": (128, 32),
    "mods/p4pc.randomizer/art/badge.png": (256, 32),
    "mods/p4pc.practice/art/badge.png": (128, 32),
    "tests/mod_fixtures/ui_overlay/art/badge.png": (128, 32),
    "tests/mod_fixtures/ui_native_row/art/card.png": (256, 128),
    "mods/p4pc.skill-affinity/art/legend.png": (128, 64),
}

# ---------------------------------------------------------------- PNG

def stored_zlib(data):
    """A zlib stream of stored (uncompressed) deflate blocks, written here so the
    bytes never depend on the zlib library's version or compression choices."""
    out = bytearray(b"\x78\x01")
    for start in range(0, max(len(data), 1), 65535):
        block = data[start:start + 65535]
        final = start + 65535 >= len(data)
        out += bytes((1 if final else 0,)) + struct.pack("<HH", len(block), len(block) ^ 0xFFFF) + block
    return bytes(out + struct.pack(">I", zlib.adler32(data) & 0xFFFFFFFF))


def png_rgba(width, height, rgba):
    """Byte-stable straight RGBA8 PNG: filter 0 and stored deflate blocks, so a
    generator's CTest check reproduces committed art on any host."""
    def chunk(kind, body):
        return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xFFFFFFFF)
    stride = width * 4
    raw = b"".join(b"\0" + bytes(rgba[y * stride:(y + 1) * stride]) for y in range(height))
    return (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)) +
            chunk(b"IDAT", stored_zlib(raw)) + chunk(b"IEND", b""))


def read_rgba(path):
    from png_diff import read_png
    width, height, channels, pixels = read_png(str(path))
    if channels != 4:
        raise ValueError("%s: art must be RGBA" % path)
    return width, height, pixels

# ---------------------------------------------------------------- vector raster

def _inside(x, y, points):
    hit = False
    for (x0, y0), (x1, y1) in zip(points, points[1:] + points[:1]):
        if (y0 > y) != (y1 > y) and x < x0 + (y - y0) * (x1 - x0) / (y1 - y0):
            hit = not hit
    return hit


def _snap(value):
    """Geometry on a 1/1024 grid: designs use sin/cos, whose last bit may differ
    between C libraries; snapped coordinates and IEEE + - * / then give the same
    coverage on every host."""
    return round(value * 1024.0) / 1024.0


class Raster:
    """Layered point-sampled coverage: the last layer containing a sample sets
    its value (0 erases), the pixel is the mean over samples x samples."""

    def __init__(self, width, height, samples=8):
        self.width, self.height, self.samples = width, height, samples
        self.layers = []

    def _add(self, box, test, value):
        self.layers.append((box, test, value))

    def polygon(self, points, value=255):
        points = tuple((_snap(x), _snap(y)) for x, y in points)
        xs, ys = [p[0] for p in points], [p[1] for p in points]
        self._add((min(xs), min(ys), max(xs), max(ys)), lambda x, y: _inside(x, y, points), value)

    def disc(self, cx, cy, r, value=255):
        cx, cy, r = _snap(cx), _snap(cy), _snap(r)
        self._add((cx - r, cy - r, cx + r, cy + r), lambda x, y: (x - cx) ** 2 + (y - cy) ** 2 <= r * r, value)

    def line(self, p0, p1, width, value=255):
        (x0, y0), (x1, y1) = p0, p1
        length = math.hypot(x1 - x0, y1 - y0)
        nx, ny = -(y1 - y0) / length * width / 2, (x1 - x0) / length * width / 2
        self.polygon([(x0 + nx, y0 + ny), (x1 + nx, y1 + ny), (x1 - nx, y1 - ny), (x0 - nx, y0 - ny)], value)

    def alpha(self):
        n = self.samples
        out = []
        for py in range(self.height):
            row_layers = [l for l in self.layers if l[0][1] <= py + 1 and l[0][3] >= py]
            for px in range(self.width):
                layers = [l for l in row_layers if l[0][0] <= px + 1 and l[0][2] >= px]
                total = 0
                if layers:
                    for sy in range(n):
                        y = py + (sy + 0.5) / n
                        for sx in range(n):
                            x = px + (sx + 0.5) / n
                            for box, test, value in reversed(layers):
                                if box[0] <= x <= box[2] and box[1] <= y <= box[3] and test(x, y):
                                    total += value
                                    break
                out.append(total / (n * n))
        return out


def quantize(value, ramp):
    return min(ramp, key=lambda level: (abs(level - value), level))


def white_rgba(alpha, ramp):
    rgba = bytearray()
    for value in alpha:
        rgba += bytes((255, 255, 255, quantize(value, ramp)))
    return rgba

# ---------------------------------------------------------------- vector designs

def _bracket(r, x0, y0, x1, y1, t):
    """The physical family's L-bracket (left and bottom bars, chamfered ends)."""
    r.polygon([(x0, y0), (x0 + t, y0 + t * 0.6), (x0 + t, y1 - t), (x1 - t * 0.6, y1 - t), (x1, y1), (x0, y1)])


def _lens(r, p0, p1, half_width, bulge=0.0, value=255, steps=24):
    """A tapered blade from p0 to p1, widest in the middle, optionally curved."""
    (x0, y0), (x1, y1) = p0, p1
    length = math.hypot(x1 - x0, y1 - y0)
    ux, uy = (x1 - x0) / length, (y1 - y0) / length
    nx, ny = -uy, ux
    left, right = [], []
    for i in range(steps + 1):
        t = i / steps
        s = math.sin(math.pi * t) ** 0.8
        bx = x0 + ux * length * t + nx * bulge * math.sin(math.pi * t)
        by = y0 + uy * length * t + ny * bulge * math.sin(math.pi * t)
        left.append((bx + nx * half_width * s, by + ny * half_width * s))
        right.append((bx - nx * half_width * s, by - ny * half_width * s))
    r.polygon(left + right[::-1], value)


def _star(r, cx, cy, outer, inner, points, rotation=0.0, value=255):
    r.polygon([(cx + (outer if i % 2 == 0 else inner) * math.cos(math.radians(rotation + 180.0 * i / points)),
                cy + (outer if i % 2 == 0 else inner) * math.sin(math.radians(rotation + 180.0 * i / points)))
               for i in range(2 * points)], value)


def _physical(kind, size):
    """Slash, Strike and Pierce share the physical family's L-bracket; the motif
    inside names the subtype: three claw cuts, an impact burst, an arrowhead."""
    header = size == "header"
    width, height = (30, 23) if header else (36, 27)
    x0, y0, x1, y1, t = (4, 2, 26, 21, 3.0) if header else (5, 1, 33, 26, 2.6)
    iw, ih = x1 - x0, y1 - y0
    r = Raster(width, height)
    _bracket(r, x0, y0, x1, y1, t)
    if kind == "slash":
        cx, cy = x0 + t + (iw - t) * 0.52, y0 + (ih - t) * 0.46
        angle = math.radians(-48)
        ux, uy = math.cos(angle), math.sin(angle)
        nx, ny = -uy, ux
        spacing = ih * (0.27 if header else 0.26)
        half_width = ih * (0.105 if header else 0.10)
        for offset, length in ((-1, 0.62), (0, 0.86), (1, 0.62)):
            px, py, half = cx + nx * offset * spacing, cy + ny * offset * spacing, ih * length / 2
            _lens(r, (px - ux * half, py - uy * half), (px + ux * half, py + uy * half), half_width, bulge=-ih * 0.07)
    elif kind == "strike":
        cx, cy = x0 + t + (iw - t) * 0.52, y0 + (ih - t) * 0.47
        _star(r, cx, cy, ih * 0.50, ih * 0.24, 6, rotation=-15)
        r.disc(cx, cy, ih * 0.27)
        if not header:
            r.disc(cx, cy, ih * 0.13, 150)
    elif kind == "pierce":
        tip, base = (x1 + 0.5, y0 - 0.5), (x0 + t + 1.5, y1 - t - 1.5)
        r.line(base, (x0 + iw * 0.62, y1 - ih * 0.66), 3.8 if header else 4.0)
        angle = math.atan2(tip[1] - base[1], tip[0] - base[0])
        bx, by = tip[0] - math.cos(angle) * ih * 0.62, tip[1] - math.sin(angle) * ih * 0.62
        nx, ny = -math.sin(angle), math.cos(angle)
        half = ih * 0.30
        r.polygon([tip, (bx + nx * half, by + ny * half), (bx - nx * half, by - ny * half)])
        if not header:
            for offset in (-5.5, 5.5):
                sx, sy = x0 + iw * 0.30 + nx * offset, y1 - ih * 0.36 + ny * offset
                r.line((sx, sy), (sx + math.cos(angle) * 6, sy + math.sin(angle) * 6), 1.8, 140)
    else:
        raise ValueError(kind)
    return r, HEADER_RAMP if header else SKILL_RAMP


def _weapon_fusion():
    """WEAPON FUSION card symbol in the Velvet cards' node-and-line vocabulary:
    a blade petal on a crossguard with end nodes, a grip and pommel node, and two
    satellite nodes joined to the blade (the materials being fused)."""
    r = Raster(72, 77)
    cx = 36
    _lens(r, (cx, 7), (cx, 50), 5.2)
    r.line((cx - 17, 50), (cx + 17, 50), 2.4)
    for x in (cx - 17, cx + 17):
        r.disc(x, 50, 5.2)
    r.line((cx, 50), (cx, 63), 2.4)
    r.disc(cx, 63, 6.0)
    for sx in (cx - 21, cx + 21):
        r.line((sx, 22), (cx, 30), 1.6)
        r.disc(sx, 22, 3.6)
    return r, CARD_SYMBOL_RAMP


def _ko():
    """"KO" marker for an element column's instant-kill state, header weight."""
    r = Raster(17, 10)
    r.polygon([(1.0, 1.0), (3.6, 1.0), (3.6, 9.0), (1.0, 9.0)])
    r.polygon([(3.2, 4.6), (6.4, 1.0), (9.4, 1.0), (5.2, 5.4)])
    r.polygon([(4.4, 4.4), (6.8, 4.0), (9.6, 9.0), (6.6, 9.0)])
    r.disc(13.2, 5.0, 3.9)
    r.disc(13.2, 5.0, 1.55, 0)
    return r, HEADER_RAMP


def _stroke(r, points, width, value=255):
    """A round-jointed polyline of the given stroke width."""
    for p, q in zip(points, points[1:]):
        r.line(p, q, width, value)
    for x, y in points:
        r.disc(x, y, width / 2, value)


def _arc(r, cx, cy, radius, start, end, width, steps=16, value=255):
    _stroke(r, [(cx + radius * math.cos(math.radians(start + (end - start) * i / steps)),
                 cy + radius * math.sin(math.radians(start + (end - start) * i / steps))) for i in range(steps + 1)],
            width, value)


def _ailment(kind):
    """Analyze ailment headers in the element-header family (30x23, solid
    white silhouettes): a spiral, a frightened face, the anger mark, a muted
    speech bubble, a poison drop with bubbles, "Zz", falling chevrons, an ice
    block and a jolted ring. None reuses an element glyph's motif."""
    r = Raster(30, 23)
    cx, cy = 15.0, 11.5
    if kind == "panic":
        points = []
        for i in range(60):
            t = i / 59 * 2.6 * math.pi
            points.append((cx + (1.2 + t * 1.05) * math.cos(t), cy + (1.2 + t * 1.05) * math.sin(t)))
        _stroke(r, points, 3.0)
    elif kind == "fear":
        _arc(r, cx, cy, 8.6, 0, 360, 2.7, steps=40)
        r.disc(11.6, 9.0, 1.7)
        r.disc(18.4, 9.0, 1.7)
        _stroke(r, [(9.6, 15.6), (11.9, 13.6), (14.1, 15.6), (16.3, 13.6), (18.6, 15.6), (20.4, 14.0)], 1.9)
    elif kind == "rage":
        for qx, qy in ((-1, -1), (1, -1), (-1, 1), (1, 1)):
            start = math.degrees(math.atan2(-qy, -qx)) - 48
            _arc(r, cx + qx * 9.0, cy + qy * 8.2, 6.2, start, start + 96, 3.3, steps=14)
    elif kind == "silence":
        r.polygon([(6, 3.5), (24, 3.5), (26, 5.5), (26, 14), (24, 16), (14, 16), (9, 20.5), (10, 16), (6, 16),
                   (4, 14), (4, 5.5)])
        r.line((5, 18.5), (25, 1.5), 2.6, 0)
        r.line((6.6, 19.6), (26.6, 2.6), 1.6)
    elif kind == "poison":
        r.disc(12.5, 14, 6.3)
        r.polygon([(12.5, 1.5), (6.7, 11.8), (18.3, 11.8)])
        r.disc(10.6, 14.6, 1.8, 0)
        r.disc(22.5, 7, 3.0)
        r.disc(24.5, 15.5, 2.0)
    elif kind == "exhaustion":
        _stroke(r, [(4, 4), (15, 4), (4, 15), (15, 15)], 3.0)
        _stroke(r, [(18, 11), (25.5, 11), (18, 18.5), (25.5, 18.5)], 2.4)
    elif kind == "enervation":
        for y in (3.0, 11.0):
            _stroke(r, [(6, y), (15, y + 7.5), (24, y)], 3.4)
    elif kind == "freeze":
        def hexagon(radius):
            return [(cx + radius * math.cos(math.radians(30 + 60 * k)), cy + radius * math.sin(math.radians(30 + 60 * k)))
                    for k in range(6)]
        r.polygon(hexagon(9.4))
        r.polygon(hexagon(5.6), 0)
        r.polygon(hexagon(4.0))
        r.line((cx, cy - 9.4), (cx, cy + 9.4), 1.6)
    elif kind == "shock":
        _arc(r, cx, cy, 8.4, 0, 360, 2.6, steps=36)
        _stroke(r, [(4, 12.5), (10, 8.5), (13, 14.5), (17, 8.0), (20, 14.0), (26, 10.0)], 2.6)
    else:
        raise ValueError(kind)
    return r, HEADER_RAMP


def _menu_sign(kind):
    """Signs for the menu numerals (c_equip_01.spr 9..18: 18x15 italic digits,
    12-pixel caps on rows 1..13, ~3-pixel stems, about 0.25 px of slant per
    row). The plus shares a digit's cell; the minus takes the cost-less dash's
    8-pixel cell (record 60) and bar rows, narrowed to a digit's width."""
    slant = 0.25
    if kind == "plus":
        r = Raster(18, 15)
        cx, cy = 9.0, 7.0
        top, bottom = 2.0, 12.0
        r.polygon([(cx - 1.5 + slant * (cy - top), top), (cx + 1.5 + slant * (cy - top), top),
                   (cx + 1.5 - slant * (bottom - cy), bottom), (cx - 1.5 - slant * (bottom - cy), bottom)])
        r.polygon([(4.0, cy - 1.5), (14.0, cy - 1.5), (14.0, cy + 1.5), (4.0, cy + 1.5)])
        return r, MENU_DIGIT_RAMP
    if kind == "minus":
        r = Raster(12, 8)
        r.polygon([(1.5, 3.0), (10.5, 3.0), (10.5, 6.0), (1.5, 6.0)])
        return r, MENU_DASH_RAMP
    raise ValueError(kind)


VECTOR = {
    "slash-skill": lambda: _physical("slash", "skill"),
    "strike-skill": lambda: _physical("strike", "skill"),
    "pierce-skill": lambda: _physical("pierce", "skill"),
    "slash-header": lambda: _physical("slash", "header"),
    "strike-header": lambda: _physical("strike", "header"),
    "pierce-header": lambda: _physical("pierce", "header"),
    "weapon-fusion": _weapon_fusion,
    "ko": _ko,
    "plus-menu": lambda: _menu_sign("plus"),
    "minus-menu": lambda: _menu_sign("minus"),
}
for _kind in ("panic", "fear", "rage", "silence", "poison", "exhaustion", "enervation", "freeze", "shock"):
    VECTOR["ailment-" + _kind] = (lambda kind: lambda: _ailment(kind))(_kind)


def vector_rgba(name):
    raster, ramp = VECTOR[name]()
    return raster.width, raster.height, white_rgba(raster.alpha(), ramp)


def vector_png(name):
    return png_rgba(*vector_rgba(name))

# ---------------------------------------------------------------- text labels (Pillow)

# SIL OFL 1.1 fonts, fetched once and pinned by SHA-256; never committed. Images
# rendered with them are not Font Software under the OFL and ship with the mods.
FONTS = {
    "liberation-sans-bold": dict(
        url="https://github.com/liberationfonts/liberation-fonts/files/7261482/liberation-fonts-ttf-2.1.5.tar.gz",
        member="liberation-fonts-ttf-2.1.5/LiberationSans-Bold.ttf",
        sha256="788abee4c806d660e8aee46689dd8540cd4bb98da03dcc9d171ce3efd99a9173"),
    "archivo": dict(
        url="https://github.com/google/fonts/raw/main/ofl/archivo/Archivo%5Bwdth%2Cwght%5D.ttf",
        member=None,
        sha256="0e094a7d3c7c4c25cf1310c4b30014f1dae9332220b1c2c88f4fa996f0b05053"),
}

# Velvet card labels: Liberation Sans Bold fitted to the native cards' cap
# height, stroke and coverage. Camp badges: Archivo at weight 900 and width 125
# with the Camp list's dark fill and white outline.
LABELS = {
    "weapon-fusion": dict(kind="velvet", lines=("Weapon", "Fusion"), cell=(99, 31), size=17.6,
                          tracking=-0.7, line_gap=14.0, embolden=0.1),
    "weapon-fusion-small": dict(kind="velvet", lines=("WEAPON",), cell=(72, 18), size=13.0,
                                tracking=-0.3, line_gap=None, embolden=0.3),
    "badge-hard": dict(kind="camp", text="Hard"),
    "badge-challenge": dict(kind="camp", text="Challenge"),
    "badge-randomizer": dict(kind="camp", text="Randomizer"),
    "badge-practice": dict(kind="camp", text="Practice"),
    "badge-witness": dict(kind="camp", text="Witness"),
    "prompt-ailments": dict(kind="prompt", button="square", text="AILMENTS"),
    "prompt-affinities": dict(kind="prompt", button="square", text="AFFINITIES"),
    "legend-r2": dict(kind="legend-button", text="R2"),
    "legend-affinity": dict(kind="legend-word", text="Affinity"),
    "legend-resist": dict(kind="legend-word", text="Resist"),
    "stat-labels": dict(kind="stat-strip", lines=("He", "Su", "Ai")),  # Heal, Support, Ailment
    "witness-card": dict(kind="velvet", lines=("Witness", "Card"), cell=(99, 31), size=17.6,
                         tracking=-0.7, line_gap=14.0, embolden=0.1),
    "witness-card-small": dict(kind="velvet", lines=("WITNESS",), cell=(72, 18), size=13.0,
                               tracking=-0.3, line_gap=None, embolden=0.3),
}
SUPERSAMPLE = 8


def font_path(name, cache):
    spec = FONTS[name]
    target = Path(cache) / (name + ".ttf")
    if not target.is_file():
        target.parent.mkdir(parents=True, exist_ok=True)
        data = urllib.request.urlopen(spec["url"], timeout=120).read()
        if spec["member"]:
            with tarfile.open(fileobj=io.BytesIO(data)) as archive:
                data = archive.extractfile(spec["member"]).read()
        target.write_bytes(data)
    digest = hashlib.sha256(target.read_bytes()).hexdigest()
    if digest != spec["sha256"]:
        raise SystemExit("ui_art: %s has SHA-256 %s, expected %s" % (target, digest, spec["sha256"]))
    return str(target)


def _velvet_label(spec, fonts):
    from PIL import Image, ImageDraw, ImageFont
    s = SUPERSAMPLE
    width, height = spec["cell"]
    big = Image.new("L", (width * s, height * s), 0)
    draw = ImageDraw.Draw(big)
    font = ImageFont.truetype(font_path("liberation-sans-bold", fonts), int(round(spec["size"] * s)))
    cap = font.getbbox("H")
    pitch = (spec["line_gap"] or spec["size"]) * s
    y = (height * s - (pitch * (len(spec["lines"]) - 1) + cap[3] - cap[1])) / 2 - cap[1]
    stroke = int(round(spec["embolden"] * s))
    for line in spec["lines"]:
        advances = [font.getlength(c) for c in line]
        x = (width * s - (sum(advances) + spec["tracking"] * s * (len(line) - 1))) / 2
        for c, advance in zip(line, advances):
            draw.text((x, y), c, font=font, fill=255, stroke_width=stroke, stroke_fill=255)
            x += advance + spec["tracking"] * s
        y += pitch
    alpha = list(big.resize((width, height), Image.BOX).tobytes())
    return width, height, white_rgba(alpha, CARD_LABEL_RAMP)


def _camp_label(spec, fonts, height=21, size=16.5, outline=2.6, tracking=-0.5, pad=1, width_axis=125):
    from PIL import Image, ImageDraw, ImageFont
    s = SUPERSAMPLE
    font = ImageFont.truetype(font_path("archivo", fonts), int(round(size * s)))
    font.set_variation_by_axes([900, width_axis])  # axes: weight, width
    text = spec["text"]
    advances = [font.getlength(c) for c in text]
    text_width = sum(advances) + tracking * s * (len(text) - 1)
    width = int(math.ceil(text_width / s + 2 * outline + 2 * pad))
    fill = Image.new("L", (width * s, height * s), 0)
    edge = Image.new("L", (width * s, height * s), 0)
    draw_fill, draw_edge = ImageDraw.Draw(fill), ImageDraw.Draw(edge)
    top, bottom = font.getbbox("l")[1], font.getbbox("g")[3]
    y = (height * s - (bottom - top)) / 2 - top
    x = (width * s - text_width) / 2
    for c, advance in zip(text, advances):
        draw_edge.text((x, y), c, font=font, fill=255, stroke_width=int(outline * s), stroke_fill=255)
        draw_fill.text((x, y), c, font=font, fill=255)
        x += advance + tracking * s
    fill_alpha = list(fill.resize((width, height), Image.BOX).tobytes())
    edge_alpha = list(edge.resize((width, height), Image.BOX).tobytes())
    rgba = bytearray()
    for f, e in zip(fill_alpha, edge_alpha):
        if e >= 230:  # opaque: white outline shading into the dark fill
            tone = 255 - (255 - 45) * min(1.0, f / max(e, 1))
            grey = quantize(tone, CAMP_GREYS)
            rgba += bytes((grey, grey, grey, 255))
        else:  # outer anti-aliasing: white at a ramp alpha
            rgba += bytes((255, 255, 255, quantize(e, CAMP_ALPHAS)))
    return width, height, rgba


def _prompt_label(spec, fonts, height=21, size=17.0, outline=2.4, tracking=-0.3, disc=17.0, gap=1.0):
    from PIL import Image, ImageDraw, ImageFont
    s = SUPERSAMPLE
    font = ImageFont.truetype(font_path("archivo", fonts), int(round(size * s)))
    font.set_variation_by_axes([600, 90])  # axes: weight, width (the help bar's condensed caps)
    text = spec["text"]
    advances = [font.getlength(c) for c in text]
    text_width = sum(advances) + tracking * s * (len(text) - 1)
    text_left = 1.0 + disc + gap + outline
    width = int(math.ceil(text_left + text_width / s + outline + 1.0))
    W, H = width * s, height * s
    dark, fill, accent = (Image.new("L", (W, H), 0) for _ in range(3))
    d_dark, d_fill, d_accent = ImageDraw.Draw(dark), ImageDraw.Draw(fill), ImageDraw.Draw(accent)
    cx, cy, r = (1.0 + disc / 2) * s, height / 2 * s, disc / 2 * s
    d_dark.ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
    if spec["button"] != "square":
        raise ValueError(spec["button"])
    half, line = 3.6 * s, 1.5 * s  # the Square symbol: an outlined square
    d_accent.rectangle((cx - half, cy - half, cx + half, cy + half), fill=255)
    d_accent.rectangle((cx - half + line, cy - half + line, cx + half - line, cy + half - line), fill=0)
    top, bottom = font.getbbox("H")[1], font.getbbox("H")[3]
    y = (H - (bottom - top)) / 2 - top
    x = text_left * s
    for c, advance in zip(text, advances):
        d_dark.text((x, y), c, font=font, fill=255, stroke_width=int(outline * s), stroke_fill=255)
        d_fill.text((x, y), c, font=font, fill=255)
        x += advance + tracking * s
    dark_a, fill_a, accent_a = (list(img.resize((width, height), Image.BOX).tobytes()) for img in (dark, fill, accent))
    cap_top, cap_bottom = (y + top) / s, (y + bottom) / s
    rgba = bytearray()
    for i, (e, f, k) in enumerate(zip(dark_a, fill_a, accent_a)):
        row = i // width
        if e < 230:  # outer anti-aliasing of the dark outline and disc
            rgba += bytes((PROMPT_GREYS[0],) * 3 + (quantize(e, PROMPT_ALPHAS),))
        elif k >= 128:
            rgba += bytes(PROMPT_ACCENT + (255,))
        else:  # text shades from white at the cap top to light grey at the base
            t = min(1.0, max(0.0, (row + 0.5 - cap_top) / max(cap_bottom - cap_top, 1.0)))
            tone = PROMPT_GREYS[0] + (255 - 47 * t - PROMPT_GREYS[0]) * (f / 255.0)
            grey = quantize(tone, PROMPT_GREYS)
            rgba += bytes((grey, grey, grey, 255))
    return width, height, rgba


def _stat_strip(spec, fonts, cap=12.0, rows=(5, 23, 41), left=4.0, embolden=0.4):
    """Three stacked panel stat labels (27x58 like c_equip_help_02 0x2C)."""
    from PIL import Image, ImageDraw, ImageFont
    s = SUPERSAMPLE
    width, height = 27, 58
    probe = ImageFont.truetype(font_path("liberation-sans-bold", fonts), 100)
    box = probe.getbbox("H")
    font = ImageFont.truetype(font_path("liberation-sans-bold", fonts), int(round(100 * cap * s / (box[3] - box[1]))))
    top = font.getbbox("H")[1]
    big = Image.new("L", (width * s, height * s), 0)
    draw = ImageDraw.Draw(big)
    for line, row in zip(spec["lines"], rows):
        draw.text((left * s, row * s - top), line, font=font, fill=255,
                  stroke_width=int(round(embolden * s)), stroke_fill=255)
    alpha = list(big.resize((width, height), Image.BOX).tobytes())
    return width, height, white_rgba(alpha, HEADER_RAMP)


def _legend_button(spec, fonts, size=9.5, disc=19.0, rim=2.0):
    """A panel-legend button disc (23x25 cell like c_tittle_01 0xA..0xD): a dark
    45 body inside a white rim, its label in white."""
    from PIL import Image, ImageDraw, ImageFont
    s = SUPERSAMPLE
    width, height = 23, 25
    W, H = width * s, height * s
    edge, body, text = (Image.new("L", (W, H), 0) for _ in range(3))
    cx, cy, r = 11.5 * s, 13.5 * s, disc / 2 * s
    ImageDraw.Draw(edge).ellipse((cx - r, cy - r, cx + r, cy + r), fill=255)
    inner = r - rim * s
    ImageDraw.Draw(body).ellipse((cx - inner, cy - inner, cx + inner, cy + inner), fill=255)
    font = ImageFont.truetype(font_path("archivo", fonts), int(round(size * s)))
    font.set_variation_by_axes([900, 100])
    box = font.getbbox(spec["text"])
    ImageDraw.Draw(text).text((cx - (box[2] - box[0]) / 2 - box[0], cy - (box[3] - box[1]) / 2 - box[1]),
                              spec["text"], font=font, fill=255)
    e_a, b_a, t_a = (list(img.resize((width, height), Image.BOX).tobytes()) for img in (edge, body, text))
    rgba = bytearray()
    for e, b, t in zip(e_a, b_a, t_a):
        if e < 230:  # outer anti-aliasing: white at a ramp alpha
            rgba += bytes((255, 255, 255, quantize(e, CAMP_ALPHAS)))
            continue
        dark = (b / 255.0) * (1.0 - t / 255.0)  # inside the rim and not under the label
        grey = quantize(255 - (255 - 45) * dark, CAMP_GREYS)
        rgba += bytes((grey, grey, grey, 255))
    return width, height, rgba


def label_rgba(name, fonts):
    spec = LABELS[name]
    maker = {"velvet": _velvet_label, "camp": _camp_label, "prompt": _prompt_label,
             "legend-button": _legend_button, "stat-strip": _stat_strip,
             "legend-word": lambda spec, fonts: _camp_label(spec, fonts, height=23, size=15.0, width_axis=85,
                                                            tracking=0.0, outline=2.0)}[spec["kind"]]
    return maker(spec, fonts)

# ---------------------------------------------------------------- the player's disc (read in place)

class Disc:
    """ISO 9660 image -> DATA.CVM (CRI ROFS: an ISO 9660 image after a 0x1800-byte
    header) -> file bytes. Read-only; nothing is extracted."""

    def __init__(self, path):
        self.stream = open(path, "rb")
        top = self._tree(0)
        lba, _ = top["DATA.CVM"]
        self.base = lba * 2048 + 0x1800
        self.files = self._tree(self.base, recursive=True)

    def _read(self, offset, size):
        self.stream.seek(offset)
        return self.stream.read(size)

    def _tree(self, base, recursive=False):
        pvd = self._read(base + 16 * 2048, 2048)
        if pvd[1:6] != b"CD001":
            raise ValueError("not an ISO 9660 volume at %#x" % base)
        root = pvd[156:190]
        out = {}
        self._walk(base, struct.unpack_from("<I", root, 2)[0], struct.unpack_from("<I", root, 10)[0], "", out, recursive)
        return out

    def _walk(self, base, lba, size, prefix, out, recursive):
        data = self._read(base + lba * 2048, size)
        i = 0
        while i < len(data):
            length = data[i]
            if not length:
                i = (i // 2048 + 1) * 2048
                continue
            record = data[i:i + length]
            name = record[33:33 + record[32]]
            if name not in (b"\0", b"\1"):
                child_lba, child_size = struct.unpack_from("<I", record, 2)[0], struct.unpack_from("<I", record, 10)[0]
                text = prefix + name.decode("latin-1").split(";")[0]
                if record[25] & 2:
                    if recursive:
                        self._walk(base, child_lba, child_size, text + "/", out, recursive)
                else:
                    out[text if not recursive else text.lower()] = (child_lba, child_size)
            i += length

    def read(self, path):
        lba, size = self.files[path]
        return self._read(self.base + lba * 2048, size)


def pak_member(data, name):
    position = 0
    while position + 0x100 <= len(data):
        member = data[position:position + 252].split(b"\0")[0].decode("latin-1")
        size = struct.unpack_from("<I", data, position + 252)[0]
        if member == name:
            return data[position + 0x100:position + 0x100 + size]
        if not member:
            break
        position += 0x100 + (size + 63) // 64 * 64
    raise KeyError(name)


def tmx_alpha(data, offset):
    """(width, height, alpha bytes) of one TMX0 raster: PSMT4/PSMT8 + CLUT or PSMCT32."""
    if data[offset + 8:offset + 12] != b"TMX0":
        raise ValueError("TMX0 expected at %#x" % offset)
    width, height = struct.unpack_from("<HH", data, offset + 0x12)
    kind = data[offset + 0x16]
    p = offset + 0x40
    if kind in (0x13, 0x14):
        colours = 256 if kind == 0x13 else 16
        palette = [data[p + i * 4 + 3] for i in range(colours)]
        if kind == 0x13:  # CSM1: entries 8..15 and 16..23 of each 32 are swapped
            palette = [palette[i + 8 if i & 0x18 == 8 else i - 8 if i & 0x18 == 0x10 else i] for i in range(256)]
        p += colours * 4 * data[offset + 0x10]
        if kind == 0x13:
            indices = data[p:p + width * height]
        else:
            packed = data[p:p + width * height // 2]
            indices = bytearray(width * height)
            indices[0::2] = bytes(b & 15 for b in packed)
            indices[1::2] = bytes(b >> 4 for b in packed)
        alpha = bytes(palette[i] for i in indices)
    elif kind == 0x00:
        alpha = data[p + 3:p + width * height * 4:4]
    else:
        raise ValueError("unsupported TMX pixel format %#x" % kind)
    return width, height, bytes(min(255, a * 2) if a <= 0x80 else a for a in alpha)


def spr_records(data, indices):
    """Alpha planes (width, height, bytes) of the given SPR0 records' rectangles."""
    rasters, records = struct.unpack_from("<HH", data, 0x14)
    raster_offsets = [struct.unpack_from("<I", data, 0x24 + 8 * i)[0] for i in range(rasters)]
    record_offsets = [struct.unpack_from("<I", data, 0x24 + 8 * (rasters + i))[0] for i in range(records)]
    decoded = {}
    out = []
    for index in indices:
        record = record_offsets[index]
        raster = struct.unpack_from("<i", data, record + 0x14)[0]
        left, top, right, bottom = struct.unpack_from("<4i", data, record + 0x54)
        if raster not in decoded:
            decoded[raster] = tmx_alpha(data, raster_offsets[raster])
        width, _, alpha = decoded[raster]
        out.append((right - left, bottom - top,
                    b"".join(alpha[y * width + left:y * width + right] for y in range(top, bottom))))
    return out


def default_iso(environ=None):
    """The player's disc when no --iso is given: P4PC_LOCAL_ISO, then P4PC_ISO, then
    $P4PC_LOCAL_DIR/retail/p4.iso, but only when P4PC_LOCAL_DIR is set and that file exists.
    None otherwise: there is no host-specific default location."""
    environ = os.environ if environ is None else environ
    for name in ("P4PC_LOCAL_ISO", "P4PC_ISO"):
        if environ.get(name):
            return environ[name]
    local = environ.get("P4PC_LOCAL_DIR")
    if not local:
        return None
    candidate = Path(local) / "retail" / "p4.iso"
    return str(candidate) if candidate.is_file() else None


def native_planes(disc, family):
    path, member, indices = FAMILIES[family]["native"]
    data = disc.read(path)
    if member:
        data = pak_member(data, member)
    return spr_records(data, indices)

# ---------------------------------------------------------------- metrics and checks

def metrics(width, height, alpha):
    covered = sum(1 for a in alpha if a >= 128)
    runs = []
    for y in range(height):
        run = 0
        for x in range(width):
            if alpha[y * width + x] >= 128:
                run += 1
            elif run:
                runs.append(run)
                run = 0
        if run:
            runs.append(run)
    runs.sort()
    stroke = 0.0 if not runs else (runs[len(runs) // 2] if len(runs) % 2 else (runs[len(runs) // 2 - 1] + runs[len(runs) // 2]) / 2)
    ink = [(x, y) for y in range(height) for x in range(width) if alpha[y * width + x] >= 64]
    box = None if not ink else (min(p[0] for p in ink), min(p[1] for p in ink),
                                max(p[0] for p in ink) + 1, max(p[1] for p in ink) + 1)
    return dict(size=(width, height), coverage=covered / float(width * height), stroke=float(stroke),
                ink=box, levels=sorted(set(alpha)))


def native_reference(family, disc):
    if disc is None or FAMILIES[family]["native"] is None:
        return FAMILIES[family]["reference"]
    stats = [metrics(w, h, a) for w, h, a in native_planes(disc, family)]
    return dict(coverage=(min(s["coverage"] for s in stats), max(s["coverage"] for s in stats)),
                stroke=(min(s["stroke"] for s in stats), max(s["stroke"] for s in stats)))


def crop(width, pixels, rect):
    x, y, w, h = rect
    return b"".join(pixels[(row * width + x) * 4:(row * width + x + w) * 4] for row in range(y, y + h))


def check_member(path, family, rect, disc=None):
    """Problems (strings) with one asset (an atlas member when rect is given)
    against its family; empty when it fits."""
    spec = FAMILIES[family]
    width, height, pixels = read_rgba(path)
    problems = []
    if rect is not None:
        expected = ATLASES[family_key(path)]
        if (width, height) != expected:
            return ["atlas %dx%d is not the declared %dx%d" % ((width, height) + expected)], metrics(1, 1, b"\0")
        covered = [False] * (width * height)
        for _, _, _, other in members(family_key(path)):
            ox, oy, ow, oh = other
            for row in range(oy, oy + oh):
                covered[row * width + ox:row * width + ox + ow] = [True] * ow
        if any(pixels[4 * i + 3] for i in range(width * height) if not covered[i]):
            problems.append("ink outside the atlas members' rectangles")
        pixels = crop(width, pixels, rect)
        width, height = rect[2], rect[3]
    (wmin, wmax), (hmin, hmax) = spec["cell"]
    if not (wmin <= width <= wmax and hmin <= height <= hmax):
        problems.append("cell %dx%d outside the family's %d..%dx%d..%d" % (width, height, wmin, wmax, hmin, hmax))
    alpha = pixels[3::4]
    for i in range(width * height):
        r, g, b, a = pixels[4 * i:4 * i + 4]
        if not a:
            continue
        if spec["tone"] == "prompt":
            ok = ((r, g, b) == (PROMPT_GREYS[0],) * 3 and a in PROMPT_ALPHAS) if a < 255 else \
                ((r == g == b and r in PROMPT_GREYS) or (r, g, b) == PROMPT_ACCENT)
            if not ok:
                problems.append("pixel %d,%d is %s, not a battle-prompt tone" % (i % width, i // width, (r, g, b, a)))
                break
            continue
        if spec["tone"] == "white" or a < 255:
            ramp = spec["ramp"] if spec["tone"] == "white" else CAMP_ALPHAS
            if (r, g, b) != (255, 255, 255) or a not in ramp:
                problems.append("pixel %d,%d is %s, not white at a family ramp alpha" % (i % width, i // width, (r, g, b, a)))
                break
        elif not (r == g == b and r in CAMP_GREYS):
            problems.append("opaque pixel %d,%d is %s, not a Camp label grey" % (i % width, i // width, (r, g, b)))
            break
    m = metrics(width, height, alpha)
    reference = native_reference(family, disc)
    low, high = reference["coverage"]
    if not (low * 0.75 <= m["coverage"] <= high * 1.25):
        problems.append("coverage %.3f outside the native %.3f..%.3f (+-25%%)" % (m["coverage"], low, high))
    low, high = reference["stroke"]
    if not (low - 1 <= m["stroke"] <= high + 1):
        problems.append("median stroke %.1f px outside the native %.1f..%.1f (+-1)" % (m["stroke"], low, high))
    if m["ink"] is None:
        problems.append("no ink")
    return problems, m


def family_key(path):
    """The ASSETS path that the given file path is or ends with."""
    text = Path(path).as_posix()
    for asset, _, _, _ in ASSETS:
        if text == asset or text.endswith("/" + asset):
            return asset
    return None


def members(asset):
    return [entry for entry in ASSETS if entry[0] == asset]


def cmd_check(args):
    disc = Disc(args.iso) if args.iso else None
    root = Path(args.root) if args.root else ROOT
    targets = [Path(p) for p in args.paths] or sorted({root / path for path, _, _, _ in ASSETS if (root / path).is_file()})
    failed = 0
    for target in targets:
        asset = family_key(target)
        if not asset:
            print("ui_art: %s is not an ASSETS path" % target)
            failed += 1
            continue
        for _, family, maker, rect in members(asset):
            problems, m = check_member(target, family, rect, disc)
            print("ui_art: %s %s %s [%s] %dx%d coverage %.3f stroke %.1f levels %d" % (
                "ok" if not problems else "FAIL", target.as_posix(), maker, family, m["size"][0], m["size"][1],
                m["coverage"], m["stroke"], len(m["levels"])))
            for problem in problems:
                print("  " + problem)
            failed += bool(problems)
    return 1 if failed else 0


def cmd_measure(args):
    iso = args.iso or default_iso()
    if not iso:
        raise SystemExit("ui_art: pass --iso, or set P4PC_LOCAL_ISO, P4PC_ISO or P4PC_LOCAL_DIR")
    disc = Disc(iso)
    for family, spec in FAMILIES.items():
        if not spec["native"]:
            continue
        stats = [metrics(w, h, a) for w, h, a in native_planes(disc, family)]
        sizes = sorted(set(s["size"] for s in stats))
        levels = sorted(set(l for s in stats for l in s["levels"]))
        print("%s: %d records, cells %s" % (family, len(stats), " ".join("%dx%d" % size for size in sizes)))
        print("  coverage %.3f..%.3f  median stroke %.1f..%.1f px  alpha levels %s" % (
            min(s["coverage"] for s in stats), max(s["coverage"] for s in stats),
            min(s["stroke"] for s in stats), max(s["stroke"] for s in stats), levels))
    return 0


def _write(out, path, data):
    target = Path(out) / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_bytes(data)
    print("ui_art: wrote", target)


def render(asset, fonts):
    """PNG bytes of one ASSETS path: a standalone image or its whole atlas."""
    parts = []
    for _, _, maker, rect in members(asset):
        kind, name = maker.split(":", 1)
        width, height, rgba = vector_rgba(name) if kind == "vector" else label_rgba(name, fonts)
        if rect is None:
            return png_rgba(width, height, rgba)
        if (width, height) != rect[2:]:
            raise SystemExit("ui_art: %s renders %dx%d, not its declared %dx%d" % ((maker, width, height) + rect[2:]))
        parts.append((rect, rgba))
    atlas_width, atlas_height = ATLASES[asset]
    atlas = bytearray(atlas_width * atlas_height * 4)
    for (x, y, w, h), rgba in parts:
        for row in range(h):
            start = ((y + row) * atlas_width + x) * 4
            atlas[start:start + w * 4] = rgba[row * w * 4:(row + 1) * w * 4]
    return png_rgba(atlas_width, atlas_height, atlas)


def cmd_build(args):
    fonts = args.fonts or str(Path(os.environ.get("P4PC_LOCAL_DIR", Path.home() / ".cache")) / "ui-art-fonts")
    for asset in dict.fromkeys(path for path, _, _, _ in ASSETS):
        vector_only = all(maker.startswith("vector:") for _, _, maker, _ in members(asset))
        if args.only and not any(asset.startswith(prefix) for prefix in args.only):
            continue
        if args.vector_only and not vector_only:
            continue
        _write(args.out, asset, render(asset, fonts))
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    check = commands.add_parser("check")
    check.add_argument("paths", nargs="*")
    check.add_argument("--family", choices=sorted(FAMILIES))
    check.add_argument("--iso")
    check.add_argument("--root", help="check the ASSETS paths under this directory instead of the repository")
    measure = commands.add_parser("measure")
    measure.add_argument("--iso")
    build = commands.add_parser("build")
    build.add_argument("--out", default=str(ROOT))
    build.add_argument("--fonts")
    build.add_argument("--only", action="append", help="only ASSETS paths with this prefix (repeatable)")
    build.add_argument("--vector-only", action="store_true", help="only dependency-free vector assets")
    args = parser.parse_args()
    return {"check": cmd_check, "measure": cmd_measure, "build": cmd_build}[args.command](args)


if __name__ == "__main__":
    sys.exit(main())
