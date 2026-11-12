#!/usr/bin/env python3
"""Diff two PS2 RAM dumps (P4PC_DUMP_RAM, src/viewer/p4pc_viewer.c: 0x00100000..0x02000000) word by word.

  python tools/ram_diff.py a.bin b.bin [--symbols decomp/config/symbol_addrs.txt] [--runs N] [--ulp K]

Words where either side looks like a host pointer (a function or library address patched into the image) are ignored: those differ by build. The
rest is reported as runs of differing words (gaps up to 64 bytes join), each with the nearest retail symbol below it and a few word pairs, shown as
hex and as float; the lowest runs first. Counts of differing words and of float words that differ by at most --ulp units in the last place are
printed first, since a build difference in rounding shows as many one-ulp differences.
"""
import argparse
import bisect
import re
import struct
import sys

BASE = 0x00100000


def load_symbols(path):
    addrs, names = [], []
    if not path:
        return addrs, names
    pat = re.compile(r"^\s*(\S+)\s*=\s*0x([0-9A-Fa-f]+)\s*;")
    rows = []
    for line in open(path, errors="replace"):
        m = pat.match(line)
        if m:
            rows.append((int(m.group(2), 16), m.group(1)))
    rows.sort()
    for a, n in rows:
        addrs.append(a)
        names.append(n)
    return addrs, names


def sym(addrs, names, a):
    i = bisect.bisect_right(addrs, a) - 1
    return "%s+0x%x" % (names[i], a - addrs[i]) if i >= 0 else "?"


def fl(w):
    return struct.unpack("<f", struct.pack("<I", w))[0]


def hostish(w):
    return 0x08000000 <= w < 0x0C000000 or 0xA0000000 <= w < 0xE0000000


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("--symbols", default="decomp/config/symbol_addrs.txt")
    ap.add_argument("--runs", type=int, default=40)
    ap.add_argument("--ulp", type=int, default=4)
    ap.add_argument("--near-only", action="store_true",
                    help="report only the words that differ as normal floats by at most --ulp units in the last place: a rounding difference of the "
                         "arithmetic, not a different decision (the first of them in a battle names the first value that two builds computed differently)")
    ap.add_argument("--ram-ptrs", action="store_true",
                    help="also ignore words where both sides are 4-aligned addresses in the RAM range 0x00100000..0x02000000: a 64-bit build's heap "
                         "blocks sit elsewhere than an i386 build's, so every stored heap pointer differs; what is left is data")
    args = ap.parse_args()
    da, db = open(args.a, "rb").read(), open(args.b, "rb").read()
    n = min(len(da), len(db)) // 4
    wa = struct.unpack("<%dI" % n, da[: n * 4])
    wb = struct.unpack("<%dI" % n, db[: n * 4])
    def ramptr(w):
        return args.ram_ptrs and 0x00100000 <= w < 0x02000000 and w % 4 == 0

    diffs = [i for i in range(n) if wa[i] != wb[i] and not hostish(wa[i]) and not hostish(wb[i]) and not (ramptr(wa[i]) and ramptr(wb[i]))]
    def normal(w):
        e = (w >> 23) & 0xFF
        return 0 < e < 0xFF

    if args.near_only:
        diffs = [i for i in diffs if normal(wa[i]) and normal(wb[i]) and (wa[i] ^ wb[i]) < 0x80000000 and abs(wa[i] - wb[i]) <= args.ulp]
    near = sum(1 for i in diffs if abs(wa[i] - wb[i]) <= args.ulp)
    print("%d words, %d differ (host pointers ignored), %d of those within %d ulp as floats" % (n, len(diffs), near, args.ulp))
    addrs, names = load_symbols(args.symbols)
    runs = []
    for i in diffs:
        if runs and i - runs[-1][1] <= 16:
            runs[-1][1] = i
            runs[-1][2] += 1
        else:
            runs.append([i, i, 1])
    print("%d runs" % len(runs))
    for start, end, cnt in runs[: args.runs]:
        a0 = BASE + start * 4
        print("0x%08x..0x%08x %4d words  %s" % (a0, BASE + end * 4, cnt, sym(addrs, names, a0)))
        shown = 0
        for i in range(start, end + 1):
            if wa[i] != wb[i] and shown < 4:
                print("    %08x  %08x %-14.7g | %08x %-14.7g" % (BASE + i * 4, wa[i], fl(wa[i]), wb[i], fl(wb[i])))
                shown += 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
