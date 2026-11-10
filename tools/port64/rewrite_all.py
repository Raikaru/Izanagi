#!/usr/bin/env python3
"""Rewrites every unit of a mirror for the 64-bit guest-pointer model and checks that the result compiles
(docs/port64.md, M1).

    rewrite_all.py <i386 build dir> <out dir> [--jobs N] [--limit N] [--unit PATH ...] [--no-compile]

For every unit of <build>/gen/units.txt, independently (so the work is parallel and a header is never merged):
  1. preprocess the unit with its own flags (x86_64), rewrite that text (tools/port64/gptr_rewrite.py) with the i386 layout
     of the same unit as the _Static_assert oracle, write <out>/units/<unit>.64.i;
  2. compile the result for x86_64 (-fsyntax-only) and keep every diagnostic: the pointer/integer conversion warnings that
     were the inventory, and every error.

Writes summary.txt, errors.tsv (first error per failing unit), warnings.tsv and unsupported.txt. Read-only on the build tree.
M1 acceptance: summary.txt shows 0 failing units and 0 pointer/integer diagnostics."""
import argparse, collections, json, multiprocessing as mp, os, re, shlex, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, ".."))
import gptr_rewrite as gr  # noqa: E402

ARGS = {}
T32 = ["--target=i386-linux-gnu", "-m32", "-malign-double", "-mfpmath=sse", "-msse2"]
T64 = ["--target=x86_64-linux-gnu"]
# every diagnostic that is about an integer standing for a pointer or the reverse
PTRINT = ("-Wpointer-to-int-cast", "-Wint-to-pointer-cast", "-Wint-conversion", "-Wvoid-pointer-to-int-cast",
          "-Wint-to-void-pointer-cast")
WARN = list(PTRINT) + ["-Wincompatible-pointer-types", "-Wimplicit-function-declaration", "-Wincompatible-function-pointer-types"]
DIAG = re.compile(r"^(?P<file>/[^:]+):(?P<line>\d+):(?P<col>\d+): (?P<kind>warning|error): (?P<msg>.*?)(?: \[(?P<cat>-W[\w-]+)\])?$")


def init(a):
    ARGS.update(a)


def base_flags(root, fl):
    flags = [x for x in fl if x != "-ferror-limit=0"]
    return ["-DP4_PC=1", "-DP4_DECOMP_UNIT=1", "-include", "p4pc/compat.h", f"-I{root}/include"] + flags


def collect(job):
    path, fl = job
    try:
        base = base_flags(ARGS["root"], fl)
        rel = os.path.relpath(path, ARGS["mirror"])
        work = os.path.join(ARGS["out"], "units", os.path.dirname(rel))
        return path, gr.collect_abi(path, T64 + base, work, ARGS["mirror"]), None
    except BaseException as e:   # noqa: BLE001
        return path, {}, f"{type(e).__name__}: {str(e)[:300]}"


def one(job):
    path, fl = job
    t0 = time.time()
    rel = os.path.relpath(path, ARGS["mirror"])
    out_dir = os.path.join(ARGS["out"], "units", os.path.dirname(rel))
    os.makedirs(out_dir, exist_ok=True)
    try:
        base = base_flags(ARGS["root"], fl)
        rw, ipath, text = gr.process(path, T64 + base + ["-DP4_GUEST_CHECK=1"], out_dir, ARGS["mirror"], ARGS["syms"],
                                     oracle_unit=path, oracle_flags=T32 + base, abi=ARGS["abi"])
        out = os.path.join(out_dir, os.path.basename(path) + ".64.i")
        open(out, "wb").write(text.encode("latin-1"))
        os.remove(ipath)
        res = {"unsupported": rw.unsupported, "secs": time.time() - t0, "warn": [], "errors": []}
        if ARGS["compile"]:
            p = subprocess.run(["clang"] + T64 + ["-fsyntax-only", "-Wno-everything"] + WARN + gr.stage2_flags(base) +
                               ["-Wno-error=int-conversion", "-Wno-error=incompatible-pointer-types",
                                "-Wno-error=implicit-function-declaration", "-Wno-error=implicit-int", "-Wno-error=return-mismatch",
                                "-x", "cpp-output", out], capture_output=True, text=True, errors="replace")
            for ln in p.stderr.splitlines():
                m = DIAG.match(ln)
                if not m:
                    continue
                if m["kind"] == "error":
                    res["errors"].append((m["file"], m["line"], m["msg"]))
                else:
                    res["warn"].append((m["file"], m["line"], m["col"], m["cat"], m["msg"]))
        return path, res
    except BaseException as e:   # noqa: BLE001
        return path, {"fatal": f"{type(e).__name__}: {str(e)[:400]}", "secs": time.time() - t0}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("build")
    ap.add_argument("out")
    ap.add_argument("--jobs", type=int, default=6)
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--unit", action="append", default=[], help="only these units (path suffix)")
    ap.add_argument("--no-compile", action="store_true")
    ap.add_argument("--abi-cache", action="store_true", help="reuse <out>/abi.json from an earlier run")
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(HERE)))
    a = ap.parse_args()
    from pathlib import Path
    import gen_port
    mirror = os.path.join(a.build, "gen/decomp")
    os.makedirs(a.out, exist_ok=True)
    syms = gen_port.read_symbols(Path(a.root) / "decomp")
    syms.update(gen_port.read_port_symbols())
    jobs = []
    for ln in open(os.path.join(a.build, "gen/units.txt")):
        path, _, fl = ln.rstrip("\n").partition("|")
        if a.unit and not any(path.endswith(u) for u in a.unit):
            continue
        jobs.append((path, [x for x in fl.split("\t") if x and not x.startswith("-malign") and x != "-mno-implicit-float"]))
    if a.limit:
        jobs = jobs[:a.limit]
    unsup, fatal, errs, warns, secs = {}, {}, {}, {}, 0.0
    abi_path = os.path.join(a.out, "abi.json")
    abi = None
    if a.abi_cache and os.path.exists(abi_path):
        abi = json.load(open(abi_path))
    pargs = {"root": a.root, "mirror": mirror, "syms": syms, "out": a.out, "compile": not a.no_compile, "abi": abi}
    if abi is None:
        # phase 1: every unit's non-static definitions; phase 2 adapts each call to its callee's definition
        per_unit = []
        with mp.Pool(a.jobs, initializer=init, initargs=(pargs,), maxtasksperchild=25) as pool:
            for unit, defs, err in pool.imap_unordered(collect, jobs, chunksize=1):
                per_unit.append(defs)
                if err:
                    fatal[unit] = err
        abi, ambiguous = gr.merge_abi(per_unit, gr.sigs_abi(os.path.join(a.root, "tools/mipsdiff/sigs.txt")))
        json.dump(abi, open(abi_path, "w"))
        print(f"abi: {len(abi)} functions ({len(ambiguous)} names dropped as ambiguous)", file=sys.stderr)
        pargs["abi"] = abi
    with mp.Pool(a.jobs, initializer=init, initargs=(pargs,), maxtasksperchild=25) as pool:
        for i, (unit, r) in enumerate(pool.imap_unordered(one, jobs, chunksize=1), 1):
            secs += r["secs"]
            if "fatal" in r:
                fatal[unit] = r["fatal"]
                continue
            for w, l in r["unsupported"]:
                unsup[(w, l)] = 1
            if r["errors"]:
                errs[unit] = r["errors"]
            for f, line, col, cat, msg in r["warn"]:
                warns[(f, line, col, cat)] = msg
            if i % 200 == 0:
                print(f"{i}/{len(jobs)}", file=sys.stderr)
    wc = collections.Counter(k[3] for k in warns)
    ptrint = sum(n for c, n in wc.items() if c in PTRINT)
    ecat = collections.Counter(re.sub(r"'[^']*'", "'_'", e[2])[:90] for es in errs.values() for e in es)
    summary = [f"units: {len(jobs)}; rewriter crashed on {len(fatal)}; {secs:.0f} cpu-seconds",
               f"unsupported constructs left: {len(unsup)} sites"]
    if not a.no_compile:
        summary.append(f"x86_64 compile of the rewritten units: {len(errs)} of {len(jobs)} fail; "
                       f"{ptrint} pointer/integer conversion diagnostics left (deduplicated sites)")
        for c, n in wc.most_common():
            summary.append(f"  {c}: {n}")
        summary.append("most common errors:")
        for c, n in ecat.most_common(12):
            summary.append(f"  {n}  {c}")
    open(os.path.join(a.out, "summary.txt"), "w").write("\n".join(summary) + "\n")
    with open(os.path.join(a.out, "errors.tsv"), "w") as f:
        for u, e in sorted(fatal.items()):
            f.write(f"{u}\tFATAL {e}\n")
        for u, es in sorted(errs.items()):
            f.write(f"{u}\t{es[0][0]}:{es[0][1]}: {es[0][2]}\n")
    with open(os.path.join(a.out, "warnings.tsv"), "w") as f:
        for (file, line, col, cat), msg in sorted(warns.items()):
            f.write(f"{file}:{line}:{col}\t{cat}\t{msg}\n")
    with open(os.path.join(a.out, "unsupported.txt"), "w") as f:
        for (w, l) in sorted(unsup):
            f.write(f"{w} @ {l}\n")
    print("\n".join(summary))
    sys.exit(0 if not fatal else 1)


if __name__ == "__main__":
    main()
