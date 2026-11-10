#!/usr/bin/env python3
"""Per-function counts of the EE float operations that need more than MXCSR rounding, from the retail code, and their comparison with what
tools/ee_float_cc.py rewrote in the decomp mirror.

    python3 tools/ee_float_sites.py <SLUS_217.82> [--ir report.txt] [-o sites.tsv]

Retail columns: div.s, sqrt.s, rsqrt.s, fused (madd.s + msub.s + madda.s + msuba.s; informational, plain C multiply and add is right for those), mula.s. Function extents come from
decomp/config/symbol_addrs.txt (a function ends where the next symbol starts). With --ir (the P4_EE_FLOAT_REPORT file of a decomp build: "unit function div
sqrt add" per line) the output adds the rewritten counts and a verdict per function: `same` (the counts agree with div.s and sqrt.s + rsqrt.s) or `div`. The retail ELF holds retail code: only the counts leave this script's output, nothing of the bytes.
"""
import argparse
import bisect
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SYM = re.compile(r"^\s*(\w+)\s*=\s*0x([0-9A-Fa-f]{8});\s*//\s*type:(\w+)")


def symbols():
    names = {}
    for f in ("symbol_addrs.txt", "symbols_recovered.txt"):
        p = ROOT / "decomp" / "config" / f
        if not p.exists():
            continue
        for line in p.read_text().splitlines():
            m = SYM.match(line)
            if m and m[3] == "func":
                names.setdefault(int(m[2], 16), m[1])
    return names


def text_words(path):
    """The instruction words of the retail image, address -> word, read straight from the ELF (an assembler listing decodes sqrt.s as a plain coprocessor op)."""
    data = open(path, "rb").read()
    phoff = struct.unpack_from("<I", data, 0x1C)[0]
    phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)
    for i in range(phnum):
        t, off, va, pa, fs, ms, fl, al = struct.unpack_from("<8I", data, phoff + i * phentsize)
        if t == 1 and fl & 1:
            for k in range(0, fs, 4):
                yield va + k, struct.unpack_from("<I", data, off + k)[0]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("elf")
    ap.add_argument("--ir")
    ap.add_argument("-o", dest="out")
    a = ap.parse_args()
    names = symbols()
    addrs = sorted(names)
    counts = {x: [0, 0, 0, 0, 0] for x in addrs}
    col = {3: 0, 4: 1, 22: 2, 0x1C: 3, 0x1D: 3, 0x1E: 3, 0x1F: 3, 0x1A: 4}     # funct of COP1 fmt S: div sqrt rsqrt madd msub madda msuba mula
    for pc, w in text_words(a.elf):
        if (w >> 26) == 0x11 and ((w >> 21) & 31) == 16 and (w & 63) in col:
            i = bisect.bisect_right(addrs, pc) - 1
            if i >= 0:
                counts[addrs[i]][col[w & 63]] += 1
    ir = {}
    if a.ir:
        for line in open(a.ir):
            f = line.split()
            if len(f) == 5:
                v = ir.setdefault(f[1], [0, 0])
                v[0] += int(f[2])
                v[1] += int(f[3])
    rows = []
    for x in addrs:
        c = counts[x]
        if not any(c):
            continue
        nm = names[x]
        row = [f"{x:08x}", nm] + [str(v) for v in c]
        if a.ir:
            r = ir.get(nm, [0, 0])
            row += [str(r[0]), str(r[1]), "same" if r[0] == c[0] and r[1] == c[1] + c[2] else "div"]
        rows.append("\t".join(row))
    head = "# addr\tname\tdiv.s\tsqrt.s\trsqrt.s\tfused\tmula.s" + ("\tir_div\tir_sqrt\tverdict" if a.ir else "")
    text = head + "\n" + "\n".join(rows) + "\n"
    if a.out:
        Path(a.out).write_text(text)
    else:
        sys.stdout.write(text)


if __name__ == "__main__":
    main()
