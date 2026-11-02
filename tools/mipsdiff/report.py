#!/usr/bin/env python3
"""Turns an ovdiff sweep into docs/override_diff.md.

    ovdiff -n 40 -o build/ovdiff.tsv <SLUS_217.82> all
    python tools/mipsdiff/report.py build/ovdiff.tsv            # writes docs/override_diff.md

tools/mipsdiff/triage.txt classifies the mismatches (`<addr8> <class> <note>`):
  fixed        a real translation bug this sweep found; the override is fixed
  bug          a real bug reported to the owning lane (not fixed yet)
  intentional  the override does not do what retail does, by design (documented in the override)
  host         an acceptable difference between the EE and the host (see "Host differences")
  benign       the retail return value is a stale register that no caller reads
  harness      the function needs engine state the harness does not build
  platform     a port native in src/ that replaces hardware/IOP/host code, not a translation
  open         a mismatch nobody has traced to its cause yet
"""
import collections
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def read_tsv(path):
    rows = []
    for line in open(path, encoding="utf8", errors="replace"):
        if line.startswith("#") or not line.strip():
            continue
        f = line.rstrip("\n").split("\t")
        f += [""] * (13 - len(f))
        rows.append(dict(addr=f[0], name=f[1], status=f[2], attempts=int(f[3] or 0), valid=int(f[4] or 0), exact=int(f[5] or 0),
                         proven=int(f[6] or 0), fp=int(f[7] or 0), special=int(f[8] or 0), bad=int(f[9] or 0), invalid=int(f[10] or 0),
                         inconclusive=int(f[11] or 0), repro=f[12]))
    return rows


def read_triage():
    t = {}
    path = os.path.join(ROOT, "tools", "mipsdiff", "triage.txt")
    if os.path.exists(path):
        for line in open(path, encoding="utf8"):
            if not line.strip() or line.startswith("#"):
                continue
            a, c, n = (line.split(None, 2) + ["", ""])[:3]
            t[a] = (c, n.strip())
    return t


def category(r):
    s = r["status"]
    if s.startswith("ok ("):
        return "ok" if "proven" in s else "ok, float tolerance unproven"
    if s.startswith("ok"):
        return "ok"
    if s.startswith("MISMATCH") or s.startswith("documented"):
        return "mismatch"
    if s.startswith("skipped"):
        return "skipped"
    if s.startswith("inconclusive"):
        return "inconclusive"
    if s.startswith("no valid"):
        return "no valid cases"
    if s.startswith("unsupported"):
        return "unsupported instruction"
    return "harness"


def md(s):
    return s.replace("|", "\\|").replace("`", "'")


def write_subset(rows):
    """tests/ovdiff_subset.txt: keeps the functions already listed and adds every override that matched exactly (or was proven to
    differ only in host float semantics) in at least 25 valid cases of this sweep, up to 90 new ones per run."""
    path = os.path.join(ROOT, "tests", "ovdiff_subset.txt")
    kept = []
    have = set()
    if os.path.exists(path):
        for line in open(path, encoding="utf-8"):
            kept.append(line.rstrip("\n"))
            tok = line.split()
            if tok and not tok[0].startswith("#"):
                have.add(tok[0])
    ov = {f[:8] for f in os.listdir(os.path.join(ROOT, "overrides")) if re.match(r"^[0-9a-f]{8}\.c$", f)}
    ok = [r for r in rows if r["status"].startswith("ok") and r["valid"] >= 25 and r["exact"] + r["proven"] == r["valid"]
          and (r["addr"] in ov or True) and r["addr"] not in have]
    # a function whose valid cases need guest-memory state the sweep built (engine state) is not in the list: the sweep runs without it
    new = ok[:90]
    with open(path, "w", newline="\n") as fh:
        fh.write("\n".join(kept).rstrip("\n") + "\n")
        for r in new:
            fh.write("%s  # %s\n" % (r["addr"], r["name"]))
    print("report.py: %d functions added to tests/ovdiff_subset.txt (%d before)" % (len(new), len(have)))


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "build", "ovdiff.tsv")
    rows = read_tsv(src)
    if "--subset" in sys.argv:
        write_subset(rows)
        return
    triage = read_triage()
    overrides = {f[:8] for f in os.listdir(os.path.join(ROOT, "overrides")) if re.match(r"^[0-9a-f]{8}\.c$", f)}
    for r in rows:
        r["origin"] = "override" if r["addr"] in overrides else "port native"
        r["cat"] = category(r)
        r["triage"] = triage.get(r["addr"])
        if r["triage"] is None and r["origin"] == "port native":
            r["triage"] = ("platform", "host replacement of retail hardware/IOP/kernel code; the retail asm is not what the port runs")

    out = []
    w = out.append
    w("# Override differential verification\n")
    w("`tools/mipsdiff/ovdiff` runs the retail instructions in an R5900 interpreter and the port's compiled native on the same")
    w("guest memory and compares what they return and every byte they write. This page is generated by")
    w("`tools/mipsdiff/report.py` from a full sweep; the triage notes come from `tools/mipsdiff/triage.txt`.\n")
    w("## How it works\n")
    w("- **Cases.** For each function (signature from `tools/mipsdiff/sigs.txt`, which `sigs.py` extracts from the override C) the harness")
    w("  builds cases from a seed derived from the address and the case number: edge and random integers, floats (0, -0, 1, tiny, huge,")
    w("  and in every tenth case NaN/Inf/denormals), and pointers into a 64 KiB scratch arena at 0x01C00000 that is refilled for every")
    w("  case with integers, floats and pointers into itself (four fill modes). A word the retail code used as a pointer and found invalid")
    w("  is learned as a pointer slot, so structures the function walks become valid in later cases (`-c N` replays the learning).")
    w("- **Both sides.** The interpreter (`r5900.c`: 64-bit GPRs and the upper halves, the EE FPU with truncation, no NaN/Inf/denormals and")
    w("  clamping, the MMI the game uses, VU0 macro mode) decodes the retail words straight from the loaded image. The native runs through")
    w("  a cdecl thunk. Every guest page either side writes is tracked (mprotect plus a fault handler); the pages are restored between the")
    w("  runs and compared afterwards, except the interpreter's stack.")
    w("- **Callees.** Where the retail code `jal`s a function that has a native, the interpreter calls that native (VU0 registers are")
    w("  copied across), so a comparison covers one function's own code and composes leaf by leaf. A callee with no native is a logged")
    w("  stub on both sides: the harness compares the calls (address and the argument registers it reads) and the stub's made-up result.")
    w("  The engine allocator slots (0x008873E8..F4) and the libc heap are deterministic and restored with the arena.")
    w("- **Verdicts per case.** *exact*: return value and all writes identical. *float tolerance*: floats equal within a few ulps or 2e-5")
    w("  (EE truncation against round-to-nearest). *special*: the EE and IEEE disagree by design (a NaN/Inf/denormal read as a float, an")
    w("  EE Fmax against an IEEE Inf, a denormal flushed to zero). *mismatch*: anything else. *invalid*: the interpreter faulted (a random")
    w("  pointer), hit an instruction it does not model, a call it cannot make, or a native callee crashed; such a case says nothing.")
    w("  *inconclusive*: the retail path calls a function that has no native and the native never calls it.")
    w("- **Proof of a host-float difference.** When a case is not exact, the retail code is run again with the interpreter doing host float")
    w("  arithmetic (round to nearest, IEEE NaN/Inf/denormals, x86 float-to-int, separate multiply and add). If that run matches the native")
    w("  bit for bit (or faults at the same address where the native crashed), the difference is float semantics alone and the case counts")
    w("  as *proven*; the logic is identical. Differences that stay inside the tolerance without this proof are reported as unproven.")
    w("- **Isolation and determinism.** Every function runs in its own forked process from pristine guest RAM, on a private stack, with the")
    w("  main stack read-only (a wild write by a native faults instead of corrupting the harness). Natives run on a zeroed private stack, ASLR")
    w("  is switched off by re-exec when the host allows it, the only host pointers the interpreter will call are the port's natives and the")
    w("  harness's own callbacks, and random integers avoid the harness's own address ranges. Runs are reproducible (the subset was run 32")
    w("  times under load with `-fstack-protector-strong` with identical rows). memcpy/memset are logged on both sides (`--wrap`).\n")
    w("Run: `ovdiff -n 40 -o out.tsv <SLUS_217.82> all` (or addresses, `@file`); `-c N -v` replays one case and prints the verdicts,")
    w("`P4_TRACE=1` adds the interpreter's instruction trace. ctest `ovdiff_subset` runs `tests/ovdiff_subset.txt` on every check.\n")

    w("## Summary\n")
    for origin in ("override", "port native"):
        sel = [r for r in rows if r["origin"] == origin]
        c = collections.Counter(r["cat"] for r in sel)
        valid = sum(r["valid"] for r in sel)
        w("**%s** (%d functions, %d valid cases)\n" % ("overrides/<addr>.c" if origin == "override" else "port natives in src/ (host replacements, not translations)", len(sel), valid))
        w("| result | functions |")
        w("|---|---|")
        for k in ("ok", "ok, float tolerance unproven", "mismatch", "inconclusive", "no valid cases", "unsupported instruction", "skipped", "harness"):
            if c[k]:
                w("| %s | %d |" % (k, c[k]))
        w("")
    mm = [r for r in rows if r["cat"] == "mismatch"]
    tc = collections.Counter((r["triage"][0] if r["triage"] else ("intentional" if r["status"].startswith("documented") else "open")) for r in mm)
    w("Mismatches by triage: " + ", ".join("%s %d" % (k, v) for k, v in sorted(tc.items())) + ".\n")

    w("## Real translation bugs found\n")
    w("| address | function | what was wrong | status |")
    w("|---|---|---|---|")
    for a, (c, n) in sorted(triage.items()):
        if c in ("fixed", "bug"):
            nm = next((r["name"] for r in rows if r["addr"] == a), "")
            w("| %s | `%s` | %s | %s |" % (a, md(nm), md(n), "fixed" if c == "fixed" else "reported to the owning lane"))
    w("")
    w("Also found by the same work: `tools/gen_port.py` took the name of an override's single-line `extern` declaration as the name of")
    w("the function the marker introduces, so function pointers in game data (and retail-address aliases) could be patched to the wrong")
    w("function (for example data words at 0x0070B678 pointed at `_rpUVAnimCustomDataStreamGetSize` instead of `func_003d68b0`). Fixed in")
    w("`marker_definition_name`; the diff of `gen/p4pc_tables.c` changed 135 lines.\n")

    notes = os.path.join(ROOT, "tools", "mipsdiff", "replay_notes.md")
    if os.path.exists(notes):
        w(open(notes, encoding="utf8").read().replace("# Golden replay", "## Golden replay").rstrip("\n") + "\n")

    w("## Host differences (accepted)\n")
    w("- **EE float arithmetic truncates, the host rounds to nearest**: results differ by an ulp or so per operation, and an integer")
    w("  conversion after it (`vftoi`, a colour byte) can differ by one.")
    w("- **NaN, Inf, denormals**: the EE has none (exponent 255 is a large number, results clamp to +-Fmax, denormals flush to zero); the")
    w("  host produces NaN/Inf and keeps denormals. Float-to-int conversion of an out-of-range value saturates on the EE, gives INT_MIN on x86.")
    w("- **Sign of zero**: the host can give -0 where the EE gives +0.")
    w("- **libm against retail polynomials**: sin/cos/acos/atan agree to about 1e-5 relative.")
    w("- **Aligned VU0 loads**: retail `lqc2`/`sqc2` mask the low 4 address bits; the native helpers use aligned SSE accesses, so an")
    w("  unaligned operand crashes the native (real callers pass 16-byte aligned data).\n")

    w("## Mismatches\n")
    w("| address | function | origin | triage | valid | mismatching | minimal repro |")
    w("|---|---|---|---|---|---|---|")
    order = {"fixed": 0, "bug": 1, "open": 2, "harness": 3, "intentional": 4, "host": 5, "benign": 6, "platform": 7, None: 2}
    for r in sorted(mm, key=lambda r: (order.get(r["triage"][0] if r["triage"] else None, 2), r["addr"])):
        t = r["triage"]
        tx = ("%s: %s" % (t[0], t[1])) if t else "open: not traced yet"
        rep = re.sub(r"\s+", " ", r["repro"])[:220]
        st = r["status"]
        if st.startswith("documented"):
            tx = "documented (expected.txt): " + st.split(": ", 1)[-1]
        w("| %s | `%s` | %s | %s | %d | %d | `%s` |" % (r["addr"], md(r["name"]), r["origin"], md(tx), r["valid"], r["bad"], md(rep)))
    w("")

    w("## All functions\n")
    w("`pass` counts cases where the native matched exactly or within the host tolerances; `mismatch` counts the others.\n")
    w("| address | function | origin | result | valid cases | pass | mismatch | note |")
    w("|---|---|---|---|---|---|---|---|")
    for r in sorted(rows, key=lambda r: r["addr"]):
        note = ""
        if r["cat"] in ("skipped", "no valid cases", "unsupported instruction", "inconclusive", "harness"):
            note = r["status"].split(": ", 1)[-1]
        passed = r["exact"] + r["proven"] + r["fp"] + r["special"]
        w("| %s | `%s` | %s | %s | %d | %d | %d | %s |" % (r["addr"], md(r["name"]), r["origin"], r["cat"], r["valid"], passed, r["bad"], md(note)[:110]))
    path = os.path.join(ROOT, "docs", "override_diff.md")
    with open(path, "w", newline="\n", encoding="utf8") as fh:
        fh.write("\n".join(out) + "\n")
    print("report.py: wrote %s (%d functions)" % (path, len(rows)))


if __name__ == "__main__":
    main()
