#!/usr/bin/env python3
"""Per-unit steps of the 64-bit mirror build (docs/port64.md, M1). CMake runs them as custom commands, one per unit, so
the work is parallel and incremental:

  unit64.py abi   --gen G --root R --unit U --out U.abi.json     the ABI of the non-static functions U defines
  unit64.py merge --root R --abi-files FILE --out abi.json       one table from CMake's live ABI inputs (plus sigs.txt), written
                                                                 only when it changed so Ninja does not rebuild every unit
  unit64.py build --gen G --root R --unit U --abi abi.json --out U.64.o [--opt O0] [--check]
                                                                 preprocess U, rewrite it (gptr_rewrite.py), compile it for
                                                                 x86_64; a pointer/integer conversion is an error
                                                                 --src S instead of --unit U rewrites a standalone override;
                                                                 repeated --cflag F supplies its fixture declarations
                                                                 --generate-only retains rewritten C and rule census, without compiling an object
  unit64.py report --dir D                                       what the rewriter left unsupported, from the *.unsup files

G is the build's gen/ directory (decomp/ mirror and units.txt), R the repository."""
import argparse, json, os, re, shlex, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, ".."))
import gptr_rewrite as gr  # noqa: E402
import build_cache

# the 64-bit target the units are rewritten and compiled for; P4_PORT64_TARGET overrides it (other architectures: M7), e.g.
#   "--target=aarch64-linux-gnu --gcc-install-dir=/usr/lib/gcc-cross/aarch64-linux-gnu/14 -isystem /usr/aarch64-linux-gnu/include -fsigned-char"
T64 = shlex.split(os.environ.get("P4_PORT64_TARGET", "--target=x86_64-linux-gnu"))


def default_t32(t64):
    """The i386 layout oracle's target for the 64-bit target `t64`: i386 Linux, or for an Apple target i686 macOS against the same SDK (its i386
    headers are the x86_64 ones; the oracle is only compiled to its record layouts, never linked) without Apple's blocks, as the 64-bit side."""
    if gr.symbol_prefix(t64):
        sysroot = next((t64[i:i + 2] for i, f in enumerate(t64) if f == "-isysroot"), [])
        return ["--target=i686-apple-macos10.13", *sysroot, "-fno-blocks"]
    return ["--target=i386-linux-gnu"]


# the i386 oracle of the layout assertions; P4_PORT32_TARGET overrides the target (the Android build: --target=i686-linux-android24, whose headers the NDK has)
T32 = (shlex.split(os.environ["P4_PORT32_TARGET"]) if os.environ.get("P4_PORT32_TARGET") else default_t32(T64)) + \
    ["-m32", "-malign-double", "-mfpmath=sse", "-msse2"]
# the diagnostics that mean an integer stands for a pointer or the reverse: none may be left in the rewritten mirror
PTRINT_ERRORS = ["-Werror=pointer-to-int-cast", "-Werror=int-to-pointer-cast", "-Werror=int-conversion",
                 "-Werror=void-pointer-to-int-cast", "-Werror=int-to-void-pointer-cast"]


def unit_flags(gen, unit):
    gen = os.path.abspath(gen)
    path = os.path.join(gen, "decomp", unit)
    for ln in open(os.path.join(gen, "units.txt")):
        p, _, fl = ln.rstrip("\n").partition("|")
        if os.path.normpath(p) == os.path.normpath(path):
            return path, [x for x in fl.split("\t") if x and x != "-ferror-limit=0"]
    sys.exit(f"unit64: {unit} is not in {gen}/units.txt")


def base_flags(root, uf):
    return ["-DP4_PC=1", "-DP4_DECOMP_UNIT=1", "-include", "p4pc/compat.h", f"-I{root}/include"] + uf


def write_if_changed(path, text):
    if os.path.exists(path) and open(path).read() == text:
        return
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    open(path, "w").write(text)


def cmd_abi(a):
    work = os.path.dirname(os.path.abspath(a.out))
    if a.src:
        # a port source (src/...): its non-static definitions are natives the decomp calls, and their ABI is what the decomp's call sites follow
        flags = ["-DP4_PC=1", "-include", "p4pc/compat.h", f"-I{a.root}/include"] + a.cflag
        defs = gr.collect_abi(os.path.abspath(a.src), T64 + flags, work, os.path.join(os.path.abspath(a.root), "src"), a.clang,
                              deps=(a.out + ".d", a.out))
    else:
        path, uf = unit_flags(a.gen, a.unit)
        defs = gr.collect_abi(path, T64 + base_flags(a.root, uf) + a.cflag, work, os.path.join(os.path.abspath(a.gen), "decomp"), a.clang,
                              deps=(a.out + ".d", a.out))
    write_if_changed(a.out, json.dumps(defs, sort_keys=True))


def cmd_merge(a):
    per = []
    with open(a.abi_files, encoding="utf-8") as manifest:
        for line in manifest:
            if line.strip():
                with open(line.strip(), encoding="utf-8") as source:
                    per.append(json.load(source))
    table, bad = gr.merge_abi(per, gr.sigs_abi(os.path.join(a.root, "tools/mipsdiff/sigs.txt")))
    # the host library's own functions, by the prototypes its headers give (the mirror calls strcpy, memcpy, ... as `s32` m2c calls)
    for n, d in gr.collect_host_abi(["string.h", "stdio.h", "stdlib.h", "math.h", "ctype.h", "time.h"], T64,
                                    os.path.dirname(os.path.abspath(a.out)), a.clang).items():
        table.setdefault(n, d)
    # one retail function goes by several names (the m2c `func_003e1030` for the compiled `RwFreeListCreateAndPreallocateSpace`): every
    # name that has no C definition of its own gets the ABI of the one that has
    sys.path.insert(0, os.path.join(a.root, "tools"))
    import gen_port
    from pathlib import Path
    names = gen_port.retail_function_names(Path(a.root) / "decomp", Path(os.path.abspath(a.gen)))
    by_addr = {}
    for n, addr in names.items():
        by_addr.setdefault(addr, []).append(n)
    for addr, group in sorted(by_addr.items()):
        every = [f"func_{addr:08x}", f"FUN_{addr:08X}"] + sorted(group)
        src = next((n for n in every if n in table), None)      # whichever of its names has a definition (or a sigs.txt row)
        if src:
            for alias in every:
                table.setdefault(alias, table[src])
    write_if_changed(a.out, json.dumps(table, sort_keys=True))
    write_if_changed(a.out + ".ambiguous", "\n".join(bad) + "\n")


def rewrite_outputs(path, flags, work, root, syms, oracle_flags, clang, abi, deps):
    """Cache rewritten bytes and their exact unsupported/census facts together."""
    def artifacts(rw, text):
        return {"text": text,
                "unsupported": "".join(f"{w} @ {line}\n" for w, line in sorted(set(rw.unsupported))),
                "census": census_line(rw.census)}
    if not build_cache.enabled():
        with build_cache.timed("rewrite"):
            rw, ipath, text = gr.process(path, flags, work, root, syms, oracle_unit=path,
                                         oracle_flags=oracle_flags, clang=clang, abi=abi, deps=deps)
            return ipath, artifacts(rw, text)
    ipath = os.path.join(work, os.path.basename(path) + ".i")
    # Both targets are real inputs: a header selected only by i386 must still
    # invalidate the layout facts used to rewrite its 64-bit counterpart.
    gr.preprocess(path, flags, ipath, clang, deps)
    oracle_depfile = deps[0] + ".i386" if deps else None
    try:
        layouts = gr.oracle_layouts(path, oracle_flags, clang,
                                   deps=(oracle_depfile, deps[1]) if deps else None)
        if deps:
            # CMake's GCC depfile transform merges the two rules for the same
            # output. Both conditional include trees must dirty the unit.
            with open(deps[0], "ab") as merged, open(oracle_depfile, "rb") as oracle_dependencies:
                merged.write(b"\n")
                shutil.copyfileobj(oracle_dependencies, merged)
    finally:
        if oracle_depfile and os.path.exists(oracle_depfile):
            os.remove(oracle_depfile)
    parsed = []
    def produce():
        rw, _, text = gr.rewrite_preprocessed(ipath, flags, root, syms, path, oracle_flags,
                                              clang, abi, layouts)
        value = artifacts(rw, text)
        parsed.append(value)
        return json.dumps(value, ensure_ascii=False, sort_keys=True).encode("utf-8")
    data = build_cache.run_bytes("rewrite", {
        "preprocessed": build_cache.file_digest(ipath), "input_path": os.path.abspath(ipath),
        "flags": flags, "root": os.path.abspath(root), "oracle_flags": oracle_flags,
        "layouts": layouts, "symbols": syms, "abi": abi,
        "compiler": build_cache.compiler_identity(clang),
        "versions": build_cache.versions(__file__, gr.__file__)}, produce)
    return ipath, parsed[0] if parsed else json.loads(data)


def cmd_build(a):
    os.environ["P4PC_BUILD_UNIT"] = a.src or a.unit
    gen = os.path.abspath(a.gen)
    mirror = os.path.join(gen, "decomp")
    if a.src:
        # Standalone override witnesses use exactly the mirror's layout oracle and
        # rewrite rules, with the fixture's declarations supplied in --cflag.
        path, uf = os.path.abspath(a.src), a.cflag
        mirror = os.path.dirname(path)  # port headers retain their native guest API
    else:
        path, uf = unit_flags(gen, a.unit)
        uf += a.cflag
    base = base_flags(a.root, uf)
    out_dir = os.path.dirname(os.path.abspath(a.out))
    os.makedirs(out_dir, exist_ok=True)
    abi = json.load(open(a.abi)) if a.abi else None
    syms = {}
    sys.path.insert(0, os.path.join(a.root, "tools"))
    import gen_port
    from pathlib import Path
    syms = gen_port.read_symbols(Path(a.root) / "decomp")
    syms.update(gen_port.read_port_symbols())
    extra = ["-DP4_GUEST_CHECK=1"] if a.check else []
    dependency_target = a.out + ".i" if a.generate_only else a.out
    ipath, rewritten = rewrite_outputs(path, T64 + base + extra, out_dir, mirror, syms,
                                      T32 + base, a.clang, abi, (a.out + ".d", dependency_target))
    out_i = a.out + ".i"
    open(out_i, "wb").write(rewritten["text"].encode("latin-1"))
    os.remove(ipath)
    write_if_changed(a.out + ".unsup", rewritten["unsupported"])
    write_if_changed(a.out + ".census", rewritten["census"])
    if a.generate_only:
        return
    cc = [w for w in a.launcher.split("|") if w] + [a.clang] + T64 + ["-w", "-fPIC" if os.environ.get("P4_PORT64_PIC") else "-fno-pic", "-ffp-contract=off", "-mno-implicit-float"] + PTRINT_ERRORS + \
        [f"-{a.opt}", "-fno-strict-aliasing", "-fwrapv", "-" + os.environ.get("P4_PORT64_DEBUG", "g1"), "-fno-omit-frame-pointer"] + \
        gr.stage2_flags(base) + ["-x", "cpp-output", "-c", out_i, "-o", a.out]
    # -w silences warnings, so the -Werror categories are enabled explicitly
    cc = [x for x in cc if x != "-w"] + ["-Wno-everything", "-Wpointer-to-int-cast", "-Wint-to-pointer-cast", "-Wint-conversion",
                                          "-Wvoid-pointer-to-int-cast", "-Wint-to-void-pointer-cast"]
    if a.auto_init:
        cc += [f"-ftrivial-auto-var-init={a.auto_init}", "-Wuninitialized", "-Wsometimes-uninitialized"]
    with build_cache.timed("unit.compile", source=path, output=a.out) as trace:
        p = subprocess.run(cc, capture_output=True, text=True, errors="replace")
        trace["returncode"] = p.returncode
    if a.auto_init:
        # Keep each current unit's diagnostics across incremental nightly builds.
        write_if_changed(a.out + ".uninitialized", p.stderr)
    if a.auto_init and p.stderr and not p.returncode:
        sys.stderr.write(p.stderr)  # Keep source-line candidates for the nightly logic comparison.
    if p.returncode:
        sys.stderr.write(f"$ {' '.join(shlex.quote(c) for c in cc)}\n{p.stderr}")
        sys.exit(p.returncode)
    if not a.keep_i:
        os.remove(out_i)


def census_line(census):
    """one line: the rewriter's rule counts of a unit, `rule=count` sorted, spaces in a rule's name written as %20"""
    return " ".join(f"{k.replace(' ', '%20')}={v}" for k, v in sorted(census.items()) if v) + "\n"


def clang_major(clang):
    """the major version of the compiler `clang` ('clang version 23.1.3 ...'), 0 when it cannot be run"""
    try:
        out = subprocess.run([clang, "--version"], capture_output=True, text=True).stdout
    except OSError:
        return 0
    m = re.search(r"clang version (\d+)", out)
    return int(m.group(1)) if m else 0


CENSUS_HEAD = ("# unit<TAB>rule=count ...: the rewriter's rule counts per unit (tools/port64/unit64.py report --census-refresh; review the diff)\n"
               "# The layout oracle is the compiler's own record dump, so a few counts depend on the compiler's major: the first section is the full baseline of the compiler the\n"
               "# project builds with (the llvm-mingw clang of the p4pc-ci image), a later `patch` section lists the units that differ for another supported major.\n")


def read_census_file(path):
    """[{major, patch, units: {unit: {rule: count}}}] in file order; the first is the full baseline"""
    out = []
    for ln in open(path):
        m = re.match(r"# clang-major: (\d+)(.*)", ln)
        if m:
            out.append({"major": int(m.group(1)), "patch": "patch" in m.group(2), "units": {}})
        elif ln.strip() and not ln.startswith("#") and out:
            u, _, rest = ln.rstrip("\n").partition("\t")
            out[-1]["units"][u] = {k.replace("%20", " "): int(v) for k, _, v in (w.rpartition("=") for w in rest.split())}
    return out


def write_census_file(path, sections):
    with open(path, "w", newline="\n") as f:
        f.write(CENSUS_HEAD)
        for s in sections:
            f.write(f"# clang-major: {s['major']}" + (f" patch: the units whose counts differ from the clang {sections[0]['major']} baseline above\n" if s["patch"] else "\n"))
            for u in sorted(s["units"]):
                f.write(u + "\t" + " ".join(f"{k.replace(' ', '%20')}={v}" for k, v in sorted(s["units"][u].items())) + "\n")


def read_census(dir_):
    """unit -> {rule: count} from the *.census files under dir_ (what `build` wrote beside each object)"""
    out = {}
    for dp, _d, fs in os.walk(dir_):
        for f in fs:
            if f.endswith(".census"):
                unit = os.path.relpath(os.path.join(dp, f[:-len(".64.o.census")]), dir_).replace("\\", "/")
                out[unit] = {k: int(v) for k, _, v in (w.rpartition("=") for w in open(os.path.join(dp, f)).read().split())}
    return out


def cmd_report(a):
    n, rows = 0, []
    for dp, _d, fs in os.walk(a.dir):
        for f in fs:
            if f.endswith(".unsup"):
                for ln in open(os.path.join(dp, f)):
                    if ln.strip():
                        rows.append(ln.strip())
    rows = sorted(set(rows))
    print(f"{len(rows)} constructs the rewriter does not handle")
    for r in rows:
        print("  " + r)
    if a.census_baseline or a.census_refresh:
        # The rewriter's rule counts per unit are a contract: a rule that stops firing, fires elsewhere or fires a different number of times in a unit (the
        # decomp changed under a rule, a pin moved, the rewriter changed) must be reviewed. `--census-refresh` rewrites the baseline; read its diff.
        now = {u: {k.replace("%20", " "): v for k, v in c.items()} for u, c in read_census(a.dir).items()}
        if a.active_units:
            # only the units of the current build count: a .census left by a unit that is no longer compiled must not stand in for it (and a unit without one was not built)
            active = [ln.strip() for ln in open(a.active_units) if ln.strip()]
            if not active:
                sys.exit(f"{a.active_units}: no active units")
            missing = [u for u in active if u not in now]
            if missing:
                sys.exit(f"{len(missing)} active units have no census (not built by this build?): {', '.join(missing[:5])}")
            now = {u: now[u] for u in active}
        path = a.census_baseline or a.census_refresh
        have = clang_major(a.clang)
        if a.census_refresh:
            if not have:
                sys.exit(f"cannot run {a.clang} --version: the baseline records the compiler's major")
            sections = read_census_file(path) if os.path.exists(path) else []
            if sections:
                # record-without-oracle counts records the compiler's layout dump did not cover: a rise means the oracle failed
                # on that unit (2026-11-04: a Bionic header conflict made btlEffect.c's oracle fail, and the refresh recorded 12
                # instead of 7). That is a defect to fix, never a baseline.
                old = dict(sections[0]["units"])
                if have != sections[0]["major"]:
                    old.update(next((s["units"] for s in sections[1:] if s["major"] == have), {}))
                rises = [f"{u}: record-without-oracle {old[u].get('record-without-oracle', 0)} -> {c['record-without-oracle']}"
                         for u, c in sorted(now.items()) if u in old
                         and c.get("record-without-oracle", 0) > old[u].get("record-without-oracle", 0)]
                if rises:
                    sys.exit(f"refusing to refresh {path}: the layout oracle covers fewer records in {len(rises)} units "
                             f"(fix the unit's oracle compile first):\n  " + "\n  ".join(rises[:60]))
            if sections and sections[0]["major"] != have:
                # another compiler than the baseline's: only the units whose counts differ (the layout oracle is the compiler's own record dump) are kept, as a patch
                full = sections[0]["units"]
                patch = {u: c for u, c in now.items() if full.get(u) != c}
                sections = [s for s in sections if s["major"] != have or s is sections[0]] + [{"major": have, "patch": True, "units": patch}]
                write_census_file(path, sections)
                print(f"wrote {path}: the clang {have} patch of {len(patch)} units on the clang {sections[0]['major']} baseline")
            else:
                dropped = [s["major"] for s in sections[1:]]
                write_census_file(path, [{"major": have, "patch": False, "units": now}])
                print(f"wrote {path}: {len(now)} units for clang {have}" + (f" (the clang {dropped} patches are dropped: refresh them too)" if dropped else ""))
        else:
            sections = read_census_file(path)
            if not sections:
                sys.exit(f"{path}: no census baseline in it (tools/port64/unit64.py report --census-refresh {path})")
            base = dict(sections[0]["units"])
            if have != sections[0]["major"]:
                patch = next((s for s in sections[1:] if s["major"] == have), None)
                if patch is None:
                    sys.exit(f"{path} has no baseline for this build's compiler ({a.clang} is clang {have}; the baseline is for clang {sections[0]['major']}"
                             f"{', patches for ' + ', '.join(str(s['major']) for s in sections[1:]) if sections[1:] else ''}). The layout oracle is the compiler's own record\n"
                             f"dump, so a few counts depend on the major: build with a supported compiler, or review and add this one with\n"
                             f"  python tools/port64/unit64.py report --dir <build>/gen/obj64/units --census-refresh {path} --clang {a.clang}")
                base.update(patch["units"])
            drift = []
            for u in sorted(set(base) | set(now)):
                if u not in now:
                    drift.append(f"{u}: unit missing from this build (baseline has it)")
                elif u not in base:
                    drift.append(f"{u}: unit not in the baseline")
                else:
                    for k in sorted(set(base[u]) | set(now[u])):
                        if base[u].get(k, 0) != now[u].get(k, 0):
                            drift.append(f"{u}: rule {k}: baseline {base[u].get(k, 0)}, now {now[u].get(k, 0)}")
            if drift:
                print(f"{len(drift)} rewriter rule counts differ from {path}:")
                for d in drift[:60]:
                    print("  " + d)
                if len(drift) > 60:
                    print(f"  ... and {len(drift) - 60} more")
                print("review them, then `python tools/port64/unit64.py report --dir <build>/gen/obj64/units --census-refresh " + path + " --clang " + a.clang + "` and commit the diff")
                sys.exit(1)
            print(f"rewriter rule counts equal {path} ({len(now)} units, clang {have})")
    if a.baseline:
        known = set(ln.strip() for ln in open(a.baseline) if ln.strip() and not ln.startswith("#"))
        new = [r for r in rows if r not in known]
        if new:
            print("NEW unsupported constructs (fix them, or list them in the baseline with a reason):")
            for r in new:
                print("  " + r)
            sys.exit(1)


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    for name in ("abi", "build"):
        s = sub.add_parser(name)
        s.add_argument("--gen", required=True)
        s.add_argument("--root", required=True)
        s.add_argument("--unit", default="")
        s.add_argument("--out", required=True)
        s.add_argument("--clang", default="clang")
        s.add_argument("--cflag", action="append", default=[])
        s.add_argument("--src", default="", help="a standalone source instead of a mirror unit")
        if name == "build":
            s.add_argument("--abi")
            s.add_argument("--opt", default="O0")
            s.add_argument("--auto-init", choices=("pattern", "zero"), help="initialize rewritten mirror locals too; retain uninitialized-use diagnostics")
            s.add_argument("--launcher", default="", help="command that wraps the final compile (ccache; ee_float_cc.py), words joined by '|'")
            s.add_argument("--check", action="store_true", help="P4_GUEST_CHECK: faults on a host pointer stored in guest memory")
            s.add_argument("--keep-i", action="store_true")
            s.add_argument("--generate-only", action="store_true", help="write rewritten source and census only; do not compile an object")
    s = sub.add_parser("merge")
    s.add_argument("--clang", default="clang")
    s.add_argument("--gen", required=True)
    s.add_argument("--root", required=True)
    s.add_argument("--abi-files", required=True, help="one live per-source ABI JSON path per line; stale build-directory files are not inputs")
    s.add_argument("--out", required=True)
    s = sub.add_parser("report")
    s.add_argument("--dir", required=True)
    s.add_argument("--baseline")
    s.add_argument("--census-baseline", help="fail when a unit's rewriter rule counts differ from this file (config/port64_census.txt)")
    s.add_argument("--census-refresh", help="write the baseline from the build's counts (review the diff)")
    s.add_argument("--clang", default="clang", help="the build's compiler: the census baseline of that clang major is the one checked")
    s.add_argument("--active-units", help="file listing the units of the current build, one per line: only these are compared (a stale .census of a dropped unit is ignored)")
    a = ap.parse_args()
    {"abi": cmd_abi, "merge": cmd_merge, "build": cmd_build, "report": cmd_report}[a.cmd](a)


if __name__ == "__main__":
    main()
