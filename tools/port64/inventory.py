#!/usr/bin/env python3
"""64-bit host inventory of the decomp mirror.

Compiles every unit of an existing i386 build tree (a cmake build dir that has run `p4_mirror`) for
--target=i386-linux-gnu -malign-double (what ships today) and --target=x86_64-linux-gnu, and writes to <out>:

  warnings.tsv   x86_64 diagnostics, one row per source site (file:line:col, category, text), deduplicated
                 across units (a header site is counted once)
  layouts.json   per record: i386 and x86_64 size, align and depth-1 fields (offset, "type name")
  unit_errors.tsv  units that do not compile for x86_64 at all, with the first error
  summary.txt    the numbers docs/port64.md quotes

    inventory.py <build-dir> <out-dir> [--jobs N] [--limit N]

Read-only: nothing is built or linked. Both targets read the same mirror (<build>/gen/decomp); on x86_64
include/p4pc/compat.h pre-empts the decomp's type.h (its 32-bit size_t/intptr_t/uintptr_t clash with the
host's on LP64; that clash is itself a finding, docs/port64.md "Inventory")."""
import argparse, collections, concurrent.futures as cf, json, os, re, shutil, subprocess, sys

T32 = ["--target=i386-linux-gnu", "-m32", "-malign-double", "-mfpmath=sse", "-msse2"]
T64 = ["--target=x86_64-linux-gnu"]
WARN = ["-Wpointer-to-int-cast", "-Wint-to-pointer-cast", "-Wint-conversion", "-Wpointer-sign",
        "-Wincompatible-pointer-types", "-Wformat", "-Wvarargs", "-Wshorten-64-to-32", "-Wcast-function-type"]


def cmd(root, mirror, path, flags, target, extra):
    return (["clang"] + target + ["-DP4_PC=1", "-ffp-contract=off", "-O0", "-fno-strict-aliasing", "-fwrapv",
            "-include", "p4pc/compat.h", "-DP4_DECOMP_UNIT=1", "-std=gnu89", "-fno-builtin", "-ferror-limit=0",
            "-DINCLUDE_ASM(FOLDER,NAME)=", "-DINCLUDE_RODATA(FOLDER,NAME)=",
            "-Wno-error=int-conversion", "-Wno-error=incompatible-pointer-types", "-Wno-error=incompatible-function-pointer-types",
            "-Wno-error=return-mismatch", "-Wno-error=implicit-function-declaration", "-Wno-error=implicit-int",
            f"-I{root}/include", f"-I{mirror}", f"-I{mirror}/include", f"-I{mirror}/src"] + flags + extra + [path])


def run(a):
    c, = a
    p = subprocess.run(c, capture_output=True, text=True, errors="replace")
    return p.returncode, p.stdout, p.stderr


DIAG = re.compile(r"^(?P<file>/[^:]+):(?P<line>\d+):(?P<col>\d+): (?P<kind>warning|error): (?P<msg>.*?)(?: \[(?P<cat>-W[\w-]+)\])?$")


def parse_diags(text):
    for ln in text.splitlines():
        m = DIAG.match(ln)
        if m:
            yield m.group("file"), int(m.group("line")), int(m.group("col")), m.group("kind"), m.group("cat") or "", m.group("msg")


def parse_layout(text):
    recs, cur = {}, None
    for ln in text.splitlines():
        if ln.startswith("*** Dumping AST Record Layout"):
            cur = None
            continue
        m = re.match(r"^\s*\d+ \| ((?:struct|union) .*)$", ln)
        if m and cur is None:
            cur = {"size": None, "align": None, "fields": []}
            recs[m.group(1)] = cur
            continue
        if cur is None:
            continue
        m = re.match(r"^\s+\| \[sizeof=(\d+), (?:dsize=\d+, )?align=(\d+)", ln)
        if m:
            cur["size"], cur["align"] = int(m.group(1)), int(m.group(2))
            cur = None
            continue
        m = re.match(r"^\s*(\d+)(?::\d+-\d+)? \|   (\S.*)$", ln)
        if m:
            cur["fields"].append((int(m.group(1)), m.group(2)))
    return recs


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("build")
    ap.add_argument("out")
    ap.add_argument("--jobs", type=int, default=os.cpu_count())
    ap.add_argument("--limit", type=int, default=0)
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    m32 = os.path.join(a.build, "gen/decomp")
    m64 = m32
    units = []
    for ln in open(os.path.join(a.build, "gen/units.txt")):
        path, _, fl = ln.rstrip("\n").partition("|")
        flags = [x for x in fl.split("\t") if x and not x.startswith("-DINCLUDE")
                 and x not in ("-std=gnu89", "-fno-builtin", "-ferror-limit=0")
                 and not re.match(r"-I.*/gen/decomp(/include|/src)?$", x)]
        units.append((path, flags))
    if a.limit:
        units = units[:a.limit]
    u64 = [(p.replace(m32, m64), [x.replace(m32, m64) for x in f]) for p, f in units]
    print(f"{len(units)} decomp units", file=sys.stderr)

    sites, unit_err, per_unit = {}, {}, collections.Counter()
    jobs = [(cmd(a.root, m64, p, f, T64, ["-fsyntax-only", "-Wno-everything"] + WARN),) for p, f in u64]
    with cf.ThreadPoolExecutor(a.jobs) as ex:
        for (p, f), (rc, _o, err) in zip(u64, ex.map(run, jobs)):
            first = None
            for file, line, col, kind, cat, msg in parse_diags(err):
                if kind == "error":
                    first = first or f"{file}:{line}: {msg}"
                    sites[(file, line, col, "error")] = msg
                else:
                    sites[(file, line, col, cat)] = msg
                    per_unit[p] += 1
            if rc:
                unit_err[p] = first or "(no parsed error)"
    cats = collections.Counter(k[3] for k in sites)
    with open(os.path.join(a.out, "warnings.tsv"), "w") as w:
        for (file, line, col, cat), msg in sorted(sites.items()):
            w.write(f"{file}:{line}:{col}\t{cat}\t{msg}\n")
    with open(os.path.join(a.out, "unit_errors.tsv"), "w") as w:
        for p, e in sorted(unit_err.items()):
            w.write(f"{p}\t{e}\n")

    # port-owned code (src/): same flags as the port build, no -malign-double
    psrc = sorted(os.path.join(dp, f) for dp, _d, fs in os.walk(os.path.join(a.root, "src")) for f in fs if f.endswith(".c"))
    pinc = [f"-I{a.root}/include", f"-I{a.root}/src/movie", f"-I{a.root}/src/render", f"-I{a.root}/src/platform/files", f"-I{a.root}/src/cri",
            f"-I{a.root}/src/viewer", "-I/usr/include/i386-linux-gnu"]
    pbase = ["-DP4_PC=1", "-D_FILE_OFFSET_BITS=64", "-O0", "-fsyntax-only", "-Wno-everything", "-include", "p4pc/compat.h", "-std=gnu11"] + pinc
    psites, perr = {}, {}
    pjobs = [(["clang"] + T64 + pbase + WARN + [f],) for f in psrc]
    with cf.ThreadPoolExecutor(a.jobs) as ex:
        for f, (rc, _o, err) in zip(psrc, ex.map(run, pjobs)):
            for file, line, col, kind, cat, msg in parse_diags(err):
                if file.startswith(a.root + "/src") or file.startswith(a.root + "/include"):
                    psites[(file, line, col, cat if kind == "warning" else "error")] = msg
            if rc:
                perr[f] = err.strip().splitlines()[0] if err.strip() else "?"
    play = {}
    for tag, tgt in (("i386", ["--target=i386-linux-gnu", "-m32"]), ("x86_64", T64)):
        jobs = [(["clang"] + tgt + pbase + ["-Xclang", "-fdump-record-layouts-complete", f],) for f in psrc]
        with cf.ThreadPoolExecutor(a.jobs) as ex:
            for rc, out, err in ex.map(run, jobs):
                for name, r in parse_layout(out + err).items():
                    if a.root in name or re.search(r"\bP4\w|\bp4\w", name):
                        play.setdefault(name, {})[tag] = r
    json.dump(play, open(os.path.join(a.out, "port_layouts.json"), "w"))
    pboth = {n: r for n, r in play.items() if "i386" in r and "x86_64" in r and r["i386"]["size"] and r["x86_64"]["size"]}
    pdiff = {n: r for n, r in pboth.items() if r["i386"]["size"] != r["x86_64"]["size"]}
    pptr = {n: r for n, r in pboth.items() if any("*" in t for _o, t in r["x86_64"]["fields"])}
    pcats = collections.Counter(k[3] for k in psites)
    with open(os.path.join(a.out, "port_warnings.tsv"), "w") as w:
        for (file, line, col, cat), msg in sorted(psites.items()):
            w.write(f"{file}:{line}:{col}\t{cat}\t{msg}\n")
    with open(os.path.join(a.out, "port_errors.tsv"), "w") as w:
        for f, e in sorted(perr.items()):
            w.write(f"{f}\t{e}\n")

    lay = {}
    for tag, mirror, tgt, us in (("i386", m32, T32, units), ("x86_64", m64, T64, u64)):
        jobs = [(cmd(a.root, mirror, p, f, tgt, ["-w", "-fsyntax-only", "-Xclang", "-fdump-record-layouts-complete"]),) for p, f in us]
        with cf.ThreadPoolExecutor(a.jobs) as ex:
            for rc, out, err in ex.map(run, jobs):
                for name, r in parse_layout(out + err).items():
                    lay.setdefault(name, {})[tag] = r
    json.dump(lay, open(os.path.join(a.out, "layouts.json"), "w"))
    both = {n: r for n, r in lay.items() if "i386" in r and "x86_64" in r and r["i386"]["size"] is not None
            and r["x86_64"]["size"] is not None}
    proj = {n: r for n, r in both.items() if "/gen/decomp/" in n or "/gen64/decomp/" in n or not re.search(r"/usr/|/lib/llvm", n)}
    diff = {n: r for n, r in proj.items() if r["i386"]["size"] != r["x86_64"]["size"]
            or [o for o, _ in r["i386"]["fields"]] != [o for o, _ in r["x86_64"]["fields"]]}
    ptr = lambda t: "*" in t
    nfield = sum(len(r["x86_64"]["fields"]) for r in proj.values())
    nptr = sum(1 for r in proj.values() for _o, t in r["x86_64"]["fields"] if ptr(t))
    nfn = sum(1 for r in proj.values() for _o, t in r["x86_64"]["fields"] if re.search(r"\(\*", t))
    nlong = sum(1 for r in proj.values() for _o, t in r["x86_64"]["fields"] if re.match(r"^(unsigned )?long\b(?! long)", t))
    rec_with_ptr = sum(1 for r in proj.values() if any(ptr(t) for _o, t in r["x86_64"]["fields"]))
    with open(os.path.join(a.out, "summary.txt"), "w") as s:
        s.write(f"units: {len(units)}; failing to compile for x86_64: {len(unit_err)}\n")
        s.write(f"x86_64 diagnostics (deduplicated sites): {sum(cats.values())}\n")
        for c, n in cats.most_common():
            s.write(f"  {c or '(no category)'}: {n}\n")
        s.write(f"port src/: {len(psrc)} files; failing to compile for x86_64: {len(perr)}; diagnostics: {sum(pcats.values())}\n")
        for c, n in pcats.most_common():
            s.write(f"  {c or '(no category)'}: {n}\n")
        s.write(f"port records: {len(pboth)}; with a pointer field: {len(pptr)}; size differs i386 -> x86_64: {len(pdiff)}\n")
        s.write(f"records (project, both targets): {len(proj)}; depth-1 fields: {nfield}\n")
        s.write(f"records with a pointer field: {rec_with_ptr}; pointer fields: {nptr} (function pointers: {nfn}); `long` fields: {nlong}\n")
        s.write(f"records whose size or field offsets differ i386 -> x86_64: {len(diff)}\n")
    print(open(os.path.join(a.out, "summary.txt")).read())


if __name__ == "__main__":
    main()
