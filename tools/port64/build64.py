#!/usr/bin/env python3
"""Builds a 64-bit executable from mirrored decomp units and port sources (docs/port64.md).

For each --unit (a path under <gen>/decomp, as in gen/units.txt) it
  1. preprocesses the unit with its own flags and rewrites that text with tools/port64/gptr_rewrite.py, the i386 layout
     of the same unit as the assertion oracle,
  2. compiles the rewritten text for x86_64 (P4_GUEST_CHECK on),
then compiles the --src port files (x86_64, no decomp flags), and links everything into --exe.

    build64.py --gen <build>/gen --root <repo> --out <dir> --unit src/Kernel/sdkTask.c \\
               --src tests/port64_task_test.c --src src/platform/guest.c --exe <dir>/port64_task_test64

Linux x86_64 only for now; it is the prototype of the gen_port 64-bit mirror pass."""
import argparse, os, shlex, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from gptr_rewrite import stage2_flags  # noqa: E402
OPT = ["-O2", "-fno-strict-aliasing", "-fwrapv", "-g1", "-fno-omit-frame-pointer"]


def run(cmd):
    p = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
    if p.returncode:
        sys.stderr.write(f"$ {' '.join(shlex.quote(c) for c in cmd)}\n{p.stdout}{p.stderr}")
        sys.exit(p.returncode)
    return p


def unit_flags(gen, unit):
    path = os.path.join(gen, "decomp", unit)
    for ln in open(os.path.join(gen, "units.txt")):
        p, _, fl = ln.rstrip("\n").partition("|")
        if os.path.normpath(p) == os.path.normpath(path):
            return path, [x for x in fl.split("\t") if x]
    sys.exit(f"build64: {unit} is not in {gen}/units.txt")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--gen", required=True)
    ap.add_argument("--root", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--unit", action="append", default=[])
    ap.add_argument("--src", action="append", default=[])
    ap.add_argument("--exe", required=True)
    ap.add_argument("--cflag", action="append", default=[], help="extra flag for the --src files")
    ap.add_argument("--lib", action="append", default=[], help="extra link flag")
    ap.add_argument("--python", default=sys.executable)
    ap.add_argument("--clang", default="clang")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    mirror = os.path.join(a.gen, "decomp")
    inc = [f"-I{a.root}/include"]
    objs = []
    for unit in a.unit:
        path, uf = unit_flags(a.gen, unit)
        uf = [x for x in uf if x != "-ferror-limit=0"]
        base = ["-DP4_PC=1", "-DP4_DECOMP_UNIT=1", "-include", "p4pc/compat.h"] + inc + uf
        # the i386 flags of this unit: what the shipping build compiles, -malign-double included
        t32 = ["--target=i386-linux-gnu", "-m32", "-malign-double", "-mfpmath=sse", "-msse2"]
        t64 = ["--target=x86_64-linux-gnu"]
        work = os.path.join(a.out, "rw")
        out = os.path.join(work, os.path.basename(unit) + ".64.i")
        run([a.python, "-W", "ignore", os.path.join(HERE, "gptr_rewrite.py"), "--root", mirror, "--out", out, "--work", work,
             "--oracle-unit", path, "--oracle-flags", " ".join(shlex.quote(x) for x in t32 + base),
             "--symbols", os.path.join(a.root, "decomp"), "--census", os.path.join(a.out, "census_" + os.path.basename(unit) + ".json"),
             path] + t64 + base + ["-DP4_GUEST_CHECK=1"])
        obj = os.path.join(a.out, os.path.basename(unit) + ".64.o")
        run([a.clang] + t64 + ["-w", "-Wno-error"] + OPT + stage2_flags(base) + ["-x", "cpp-output", "-c", out, "-o", obj])
        objs.append(obj)
    for s in a.src:
        src = s if os.path.isabs(s) else os.path.join(a.root, s)
        obj = os.path.join(a.out, os.path.basename(s) + ".64.o")
        run([a.clang, "--target=x86_64-linux-gnu", "-DP4_PC=1", "-DP4_GUEST_CHECK=1", "-include", "p4pc/compat.h", "-Wall",
             "-Wno-unused"] + OPT + inc + a.cflag + ["-c", src, "-o", obj])
        objs.append(obj)
    run([a.clang, "--target=x86_64-linux-gnu"] + objs + a.lib + ["-o", a.exe])


if __name__ == "__main__":
    main()
