#!/usr/bin/env python3
"""Group an `ovdiff -R` replay report by the file that defines each native.

  python tools/mipsdiff/replay_report.py build/traces/hook/replay.tsv [--seams-out FILE]

Reads the report written by an `ovdiff -R` replay run, finds the C file that defines every function (`overrides/<addr>.c`, or
the `src/` file that defines the name) and prints, per defining directory, how many functions are ok, mismatching,
documented or without valid calls, then lists every mismatching function with its first difference. `--seams-out` writes
`replay_expected.txt` lines (`<addr8> reason`) for the mismatching functions in the host-bound seam files (pad, memcard,
sound, file system, kernel, CRI middleware glue that reads host state): replacements of hardware or IOP behaviour whose result
depends on the host, so a replay against retail's has no meaning.
"""

import argparse
import collections
import glob
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))

# src/ files whose natives replace hardware/IOP/kernel behaviour with host code: replaying retail's result is not a check.
SEAM_PREFIXES = (
    "src/platform/pad/", "src/platform/memcard/", "src/platform/sound/", "src/platform/files/", "src/platform/kernel_",
    "src/platform/ps2_kernel.c", "src/platform/sce_", "src/render/",
)
SEAM_REASON = {
    "src/platform/pad/": "port seam (SDL pad): reads host input, not the IOP pad driver",
    "src/platform/memcard/": "port seam (host memory card folder): not the IOP mc server",
    "src/platform/sound/": "port seam (sound driver commands): no IOP sound server",
    "src/platform/files/": "port seam (host VFS): not the CD/ROFS drivers",
    "src/platform/kernel_": "port seam (host kernel/heap model): not the PS2 kernel",
    "src/platform/ps2_kernel.c": "port seam (host kernel model): not the PS2 kernel",
    "src/platform/sce_": "port seam (host SDK model): not the PS2 SDK",
    "src/render/": "port seam (OpenGL device layer): not the GS raster code",
}


def owner_file(addr, name, texts):
    if os.path.exists(os.path.join(ROOT, "overrides", addr + ".c")):
        return "overrides/%s.c" % addr
    pat = re.compile(r"^[\w\s\*]+\b" + re.escape(name) + r"\s*\(", re.M)
    for f, t in texts.items():
        if pat.search(t):
            return f
    return "?"


DOC_RE = re.compile(r"[^.]*\b(stale|retail leaves|undefined|port change|not modelled|does not reproduce|differs from retail|intentionally|deliberately)\b[^.]*\.",
                    re.I)


def source_note(f):
    """The sentence of the native's leading comment that says it differs from retail on purpose, or ''."""
    try:
        with open(os.path.join(ROOT, f), encoding="utf-8", errors="replace") as fh:
            head = fh.read(6000)
    except OSError:
        return ""
    end = head.find("*/")
    m = DOC_RE.search(head[: end if end > 0 else len(head)].replace("\n *", " ").replace("\n", " "))
    return " ".join(m.group(0).split())[:220] if m else ""


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("report")
    p.add_argument("--seams-out", default=None)
    a = p.parse_args()
    texts = {}
    for f in glob.glob(os.path.join(ROOT, "src", "**", "*.c"), recursive=True):
        with open(f, encoding="utf-8", errors="replace") as fh:
            texts[os.path.relpath(f, ROOT).replace("\\", "/")] = fh.read()
    rows = []
    cols = []
    with open(a.report, encoding="utf-8", errors="replace") as fh:
        for line in fh:
            if line.startswith("# addr"):
                cols = line[2:].rstrip("\n").split("\t")
            if line.startswith("#") or "\t" not in line:
                continue
            rows.append(line.rstrip("\n").split("\t"))
    by_dir = collections.defaultdict(collections.Counter)
    mism = []
    for r in rows:
        status = r[2].split(":")[0].split("(")[0].strip()
        if status in ("no recorded calls", "skipped"):
            continue
        f = owner_file(r[0], r[1], texts)
        d = "overrides" if f.startswith("overrides/") else "/".join(f.split("/")[:2]) if f.startswith("src/") else f
        by_dir[d][status] += 1
        if status == "MISMATCH":
            mism.append((f, r))
    print("| owner | ok | MISMATCH | documented | no valid calls | inconclusive |")
    print("|---|---|---|---|---|---|")
    for d, c in sorted(by_dir.items()):
        ok = c["ok"]
        print(f"| {d} | {ok} | {c['MISMATCH']} | {c['documented difference']} | {c['no valid calls']} | {c['inconclusive']} |")
    print()
    seams = []
    for f, r in sorted(mism, key=lambda m: (m[0], m[1][0])):
        seam = next((pre for pre in SEAM_PREFIXES if f.startswith(pre)), None)
        if seam:
            seams.append((r[0], SEAM_REASON[seam]))
            continue
        note = source_note(f)
        col = dict(zip(cols, r))
        print(f"{r[0]} {r[1]} [{f}] bad {col.get('mismatch')}/{col.get('valid')}: {col.get('repro', '')[:150]}")
        if note:
            print(f"    source says: {note}")
    if a.seams_out:
        with open(a.seams_out, "w", encoding="utf-8", newline="\n") as fh:
            for addr, why in seams:
                fh.write(f"{addr} {why}\n")
        print(f"{len(seams)} seam rows -> {a.seams_out}", file=sys.stderr)


if __name__ == "__main__":
    main()
