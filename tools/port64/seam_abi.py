#!/usr/bin/env python3
"""Seam ABI audit (docs/port64.md M3): the port natives whose definition and whose decomp-side declaration
disagree on pointer versus integer.

A port native (a function defined under src/ or overrides/, listed in tools/mipsdiff/sigs.txt) keeps its
parameter classes: a `T *` parameter is a host pointer in the register ABI, an `int`/`u32` one a guest address.
The decomp declares most CRI, libc and RenderWare natives m2c style (`int f();`, `void f(int object, u32 v)`), so
the call sites in the 64-bit mirror must be adapted to the definition: an integer argument passed to a pointer
parameter gets P4_G2H, a pointer result used as an integer gets P4_H2G. The rewriter's ABI pass takes its
definition table from sigs.txt; this tool lists every native where the decomp's declaration differs, so the
pass's own disagreement list can be checked against it.

    seam_abi.py [--decomp decomp] [--summary]

Output, one native per line: file, name, definition (ret args), declaration(s) in the decomp (ret args).
Classes: p pointer, i any integer, f float, q 64-bit, d double, v void, - no prototype (K&R `()`: every
argument an integer), ? aggregate."""
import argparse
import collections
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools", "mipsdiff"))
import sigs  # noqa: E402

DECL = re.compile(r"(?m)^[ \t]*(?:extern\s+)?([A-Za-z_][\w \t\*]*?[ \t\*])\b(\w+)[ \t]*\(([^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)[ \t]*;")


def cls(s):
    """Reduce sigs.py's classes to the ABI classes that matter here."""
    return "".join("p" if c == "p" else "v" if c == "v" else "i" if c in "ibBhH" else c for c in s)


def natives(root):
    found = collections.defaultdict(list)
    for sub in ("src", "overrides"):
        for dp, _dn, fs in os.walk(os.path.join(root, sub)):
            for f in fs:
                if f.endswith(".c"):
                    path = os.path.join(dp, f)
                    for name, ret, args in sigs.definitions(path):
                        found[name].append((os.path.relpath(path, root).replace("\\", "/"), ret, args))
    return found


def declarations(root, decomp, names):
    decl = collections.defaultdict(set)
    for sub in ("include", "src"):
        for dp, _dn, fs in os.walk(os.path.join(root, decomp, sub)):
            for f in fs:
                if not f.endswith((".h", ".c")):
                    continue
                text = sigs.strip_comments(open(os.path.join(dp, f), encoding="utf-8", errors="replace").read())
                for m in DECL.finditer(text):
                    if m.group(2) not in names:
                        continue
                    d = sigs.DEF.search(m.group(0)[:-1] + " {")
                    r = sigs.parse_def(d) if d else None
                    if r:
                        decl[m.group(2)].add((r[1], r[2]))
    return decl


def port_declarations(root, names):
    """Prototypes of the natives in the port's own sources, headers and tests (not the decomp's): name -> {(file, ret, args)}."""
    decl = collections.defaultdict(set)
    for sub in ("include", "src", "tests", "overrides"):
        for dp, _dn, fs in os.walk(os.path.join(root, sub)):
            for f in fs:
                if not f.endswith((".h", ".c")):
                    continue
                path = os.path.join(dp, f)
                text = sigs.strip_comments(open(path, encoding="utf-8", errors="replace").read())
                for m in DECL.finditer(text):
                    if m.group(2) not in names:
                        continue
                    d = sigs.DEF.search(m.group(0)[:-1] + " {")
                    r = sigs.parse_def(d) if d else None
                    if r:
                        decl[m.group(2)].add((os.path.relpath(path, root).replace("\\", "/"), r[1], r[2]))
    return decl


def same_args(a, b):
    """Argument classes equal, a variadic `?` matching any tail."""
    if "-" in (a, b) or a == b:
        return True
    for suffix_cls, other in ((a, b), (b, a)):
        if suffix_cls.endswith("?") and other.startswith(suffix_cls[:-1]):
            return True
    return False


FNCAST = re.compile(r"\(\s*[\w \t\*]+\(\s*\*\s*\)\s*\([^()]*\)\s*\)\s*\(?\s*(\w+)\s*\)?")


def check(root):
    """The gate (no baseline): a port native whose classes disagree with a prototype of it in the port, and a
    `void *`/integer parameter of a port native that its body casts to a function pointer (it is a code address and
    must be declared as a function-pointer type, or the call sites convert it as data). The decomp's m2c
    declarations are not checked: the rewriter adapts every direct call to the definition (docs/port64.md 6.1)."""
    nat = natives(root)
    decl = port_declarations(root, set(nat))
    problems = []
    for name, defs in sorted(nat.items()):
        for f, ret, args in defs:
            for df, dret, dargs in sorted(decl.get(name, ())):
                if df == f or df.startswith("overrides/"):
                    continue   # an override's own declaration: the rewriter reads the definition
                if not same_args(cls(args), cls(dargs)) or not (cls(ret) == cls(dret) or "v" in (cls(ret), cls(dret))):
                    problems.append(f"{df}: {name} is declared `{dret} {dargs}` but defined `{ret} {args}` in {f}")
    for dp, _dn, fs in os.walk(os.path.join(root, "src")):
        for fn in fs:
            if not fn.endswith(".c"):
                continue
            path = os.path.join(dp, fn)
            text = sigs.strip_comments(open(path, encoding="utf-8", errors="replace").read())
            for m in sigs.DEF.finditer(text):
                if m.group(2) in ("if", "while", "for", "switch", "return", "sizeof") or re.search(r"\bstatic\b", m.group(1)):
                    continue
                end = text.find("\n}", m.end())
                body = text[m.end():end if end > 0 else len(text)]
                scalars = {}
                for p in m.group(3).split(","):
                    pm = re.match(r"\s*(?:const\s+)?(void\s*\*|u?int(?:32|64)_t|int|unsigned(?: int)?|u32|s32)\s*(\w+)\s*$", p)
                    if pm:
                        scalars[pm.group(2)] = pm.group(1)
                for cm in FNCAST.finditer(body):
                    if cm.group(1) in scalars:
                        problems.append(f"{os.path.relpath(path, root)}: {m.group(2)} casts its `{scalars[cm.group(1)]} {cm.group(1)}` parameter to a function pointer")
    return problems

MARKER = re.compile(r"//[ \t]*FUN_([0-9A-Fa-f]{8})")
ABI_CLS = {"ptr": "p", "int": "i", "float": "f", None: "v", "other": "?"}


def decomp_aliases(root, decomp):
    """Retail address -> the name the decomp defines it under (`// FUN_<ADDR>` marker, then the definition)."""
    alias = {}
    for dp, _dn, fs in os.walk(os.path.join(root, decomp, "src")):
        for fn in fs:
            if not fn.endswith(".c"):
                continue
            text = open(os.path.join(dp, fn), encoding="utf-8", errors="replace").read()
            if "FUN_" not in text:
                continue
            marks = [(m.group(1).lower(), m.end()) for m in MARKER.finditer(text)]
            for k, (addr, end) in enumerate(marks):
                chunk = sigs.strip_comments(text[end:marks[k + 1][1] - 20 if k + 1 < len(marks) else len(text)])
                for m in sigs.DEF.finditer(chunk):
                    if m.group(2) not in ("if", "while", "for", "switch", "return", "sizeof"):
                        alias.setdefault(addr, m.group(2))
                        break
    return alias


def mirror_check(root, abi_path, decomp="decomp"):
    """Port code calls decomp functions (the mirror) directly: their classes are the mirror's, the table
    `abi.json` that the build's merge step writes (unit64.py merge). A port prototype of a mirror function must
    agree with it on pointer versus integer for every parameter it passes (an m2c `Uint32` takes a guest address:
    pass P4_H2G, not a host pointer, which would arrive truncated), and on the pointer or integer result."""
    import json
    abi = json.load(open(abi_path))
    nat = natives(root)
    alias = decomp_aliases(root, decomp)
    problems = []
    seen = set()
    for sub in ("src", "include", "tests"):
        for dp, _dn, fs in os.walk(os.path.join(root, sub)):
            for fn in fs:
                if not fn.endswith((".h", ".c")):
                    continue
                path = os.path.join(dp, fn)
                text = sigs.strip_comments(open(path, encoding="utf-8", errors="replace").read())
                for m in DECL.finditer(text):
                    name = m.group(2)
                    if name in nat:
                        continue   # a port definition: checked against its prototypes by check()
                    tgt = name if name in abi else None
                    am = re.match(r"func_([0-9a-f]{8})$", name)
                    if am and alias.get(am.group(1)) in abi:
                        tgt = alias[am.group(1)]
                    if tgt is None or abi[tgt].get("skip"):
                        continue
                    d = sigs.DEF.search(m.group(0)[:-1] + " {")
                    r = sigs.parse_def(d) if d else None
                    if not r:
                        continue
                    want = "".join(ABI_CLS.get(p["cls"], "?") for p in abi[tgt]["params"] if p)
                    if r[2] == "-" or not want:
                        # no parameters on one side (`(void)`, or a K&R definition): the result class is still the mirror's
                        wret, gret = ABI_CLS.get(abi[tgt]["ret"], "?"), cls(r[1])
                        if not ("v" in (wret, gret) or wret == gret or (gret in "iq" and wret == "i")) and (path, name) not in seen:
                            seen.add((path, name))
                            problems.append(f"{os.path.relpath(path, root).replace(chr(92), '/')}: {name} ({tgt}) is declared to return `{r[1]}`, the mirror defines it `{wret}`")
                        continue
                    got = cls(r[2]).replace("q", "i").replace("d", "f").replace("b", "i")
                    wret, gret = ABI_CLS.get(abi[tgt]["ret"], "?"), cls(r[1])
                    # a port prototype may stop short of the m2c registers it does not use (all integers)
                    short = len(got) < len(want) and set(want[len(got):]) <= {"i"} and want.startswith(got.replace("?", ""))
                    long_ = len(got) > len(want) and got.startswith(want)   # extra trailing arguments the callee ignores (retail passes more)
                    ok_args = want == got or short or long_ or same_args(got, want)
                    ok_ret = wret == gret or "v" in (wret, gret) or (gret in "iq" and wret == "i")
                    if (not ok_args or not ok_ret) and (path, name) not in seen:
                        seen.add((path, name))
                        problems.append(f"{os.path.relpath(path, root).replace(chr(92), '/')}: {name} ({tgt}) is declared `{r[1]} {r[2]}`, the mirror defines it `{wret} {want}`")
    return problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--decomp", default="decomp")
    ap.add_argument("--summary", action="store_true")
    ap.add_argument("--check", action="store_true", help="the gate: exit 1 on a finding (see check())")
    ap.add_argument("--mirror", metavar="ABI_JSON", help="also check the port's prototypes of decomp functions against the build's abi.json (mirror_check())")
    a = ap.parse_args()
    if a.check:
        problems = check(ROOT)
        if a.mirror:
            problems += mirror_check(ROOT, a.mirror, a.decomp)
        for p in problems:
            print(p)
        print(f"seam_abi --check: {len(problems)} finding(s)", file=sys.stderr)
        sys.exit(1 if problems else 0)
    nat = natives(ROOT)
    decl = declarations(ROOT, a.decomp, set(nat))
    rows = []
    for name, defs in sorted(nat.items()):
        for f, ret, args in defs:
            for dret, dargs in sorted(decl.get(name, ())):
                if args == "-" and dargs == "-":
                    continue
                if (cls(ret), cls(args)) != (cls(dret), cls(dargs)):
                    rows.append((f, name, f"{ret} {args}", f"{dret} {dargs}"))
    seen = set()
    perfile = collections.Counter()
    for row in rows:
        if row in seen:
            continue
        seen.add(row)
        perfile[row[0]] += 1
        if not a.summary:
            print(f"{row[0]:40s} {row[1]:28s} def={row[2]:12s} decl={row[3]}")
    print(f"{len(seen)} disagreements in {len(perfile)} files; {len(nat)} natives, {len(decl)} with a decomp declaration", file=sys.stderr)
    if a.summary:
        for f, n in perfile.most_common():
            print(f"{n:4d} {f}")


if __name__ == "__main__":
    main()
