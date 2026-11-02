#!/usr/bin/env python3
"""Find calls that omit arguments the callee takes, and check in the retail asm whether the omitted argument
registers are the caller's own incoming ones (the decomp wrote `f()` where retail jals with a0 untouched).
A caller that does not declare that parameter gains it (`@param` row); its own direct callers that omit the argument
are then checked the same way, up the chain. A chain is kept only when every site in it is verified.

For a callee whose other call sites pass more arguments than a site does, the site is looked up in the
retail disassembly: the enclosing function's address comes from its `// FUN_xxxxxxxx` marker, the n-th source
call is matched to the n-th `jal <callee>` in the function, and every omitted integer argument register a<i>
must be unwritten between the function entry and that jal, with no call in between. A verified site gets a
row for config/passthrough_calls.txt: the call's argument list with the caller's own i-th integer parameter
passed for each omitted argument i.

Usage: python3 tools/verify_passthrough.py --mirror build/gen/decomp --asm all.s [--rows out.txt] callee[:arity] ...
  all.s: `mipsel-linux-gnu-objdump -d -m mips:5900 --start-address=0x100000 --stop-address=0x530000 SLUS_217.82`
"""
import argparse
import collections
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_port as gp  # noqa: E402

STORE = re.compile(r"^(?:s[bhwdq]|s[bhwd][lr]|sq|swc\d|sdc\d|sqc\d|scd|sc)$")
NODEST = re.compile(r"^(?:b\w*|j|jr|mult\w*|div\w*|mthi|mtlo|nop|sync|break|syscall|cache|pref|tge\w*|tlt\w*|teq\w*|tne\w*)$")


def load_asm(path):
    asm = {}
    pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]+ \t(\S+)\s*(.*)$")
    for line in Path(path).read_text(errors="replace").splitlines():
        m = pat.match(line)
        if m:
            asm[int(m[1], 16)] = (m[2], m[3].split("\t")[0].strip())
    return asm


def load_symbols(root):
    funcs = {}
    for line in (root / "decomp/config/symbol_addrs.txt").read_text(errors="replace").splitlines():
        m = gp.SYMBOL_RE.match(line)
        if m and m[3] == "func":
            funcs[m[1]] = int(m[2], 16)
    return funcs


def marker_addresses(texts):
    """name -> address of functions whose C or INCLUDE_ASM carries a `// FUN_xxxxxxxx` marker."""
    out = {}
    pat = re.compile(r"// FUN_([0-9A-Fa-f]{8})[ \t]*\n(?:[^\n]*\n){0,1}?[^\n]*?(?:INCLUDE_ASM\([^)]*,\s*(\w+)\)|\b(\w+)\s*\([^;{}]*\)\s*\{)")
    for text in texts.values():
        for m in pat.finditer(text):
            out.setdefault(m[2] or m[3], int(m[1], 16))
    return out


def writes_reg(mnemonic, operands, reg):
    if STORE.match(mnemonic) or NODEST.match(mnemonic):
        return False
    first = operands.split(",")[0].strip()
    return first == reg


REG = re.compile(r"\b(a[0-3])\b")


def reads_writes(mn, ops):
    """(argument registers read, argument register written) by one instruction."""
    regs = REG.findall(ops)
    first = ops.split(",")[0].strip()
    if STORE.match(mn) or NODEST.match(mn) or mn in ("jal", "jalr") or not REG.fullmatch(first):
        return regs, None
    return regs[1:] + ([first] if re.search(rf"\({first}\)", ops) else []), first


def varargs_prologue(asm, addr):
    """True when the function spills a1, a2 and a3 to the stack at entry (a va_start save area)."""
    spilled = set()
    for ad in range(addr, addr + 4 * 16, 4):
        mn, ops = asm.get(ad, ("", ""))
        m = re.match(r"^(a[1-3]),-?\d+\(sp\)$", ops.replace(" ", ""))
        if m and STORE.match(mn):
            spilled.add(m[1])
    return spilled >= {"a1", "a2", "a3"}


_ARITY = {}


def asm_arity(asm, addr, starts):
    """How many integer argument registers a0..a3 the function at addr takes: those it reads before writing them,
    and those it hands untouched to a callee that takes them (a linear scan: a call clobbers them, a branch is not
    followed, so a register read only on a later path is missed)."""
    if addr in _ARITY:
        return _ARITY[addr]
    _ARITY[addr] = 0                     # recursion guard
    end = next((s_ for s_ in starts if s_ > addr), addr + 0x400)
    written, used = set(), set()
    ad = addr
    while ad < end:
        mn, ops = asm.get(ad, ("", ""))
        if not mn:
            break
        reads, dest = reads_writes(mn, ops)
        used.update(r for r in reads if r not in written)
        if mn in ("jal", "jalr") or (mn == "j" and ops.strip().startswith("0x") and int(ops.strip(), 16) in starts_set(starts)):
            reads, slot_dest = reads_writes(*asm.get(ad + 4, ("", "")))  # the delay slot runs before the call
            used.update(r for r in reads if r not in written)
            if slot_dest:
                written.add(slot_dest)
            if mn in ("jal", "j"):
                try:
                    target = int(ops.strip(), 16)
                except ValueError:
                    target = None
                if target and target in starts_set(starts):
                    used.update(f"a{i}" for i in range(asm_arity(asm, target, starts)) if f"a{i}" not in written)
            written.update(("a0", "a1", "a2", "a3"))
            ad += 8
            continue
        if dest:
            written.add(dest)
        ad += 4
    _ARITY[addr] = 1 + max((int(r[1]) for r in used), default=-1)
    return _ARITY[addr]


_STARTS = {}


def starts_set(starts):
    key = id(starts)
    if key not in _STARTS:
        _STARTS[key] = set(starts)
    return _STARTS[key]


_INDEX = {}


def call_sites(texts, name):
    """Every call of `name` in the units: (path, text, stripped text, start, open paren, close paren, argument count, spans)."""
    key = id(texts)
    if key not in _INDEX:
        index = collections.defaultdict(list)
        for path, text in texts.items():
            st = gp._strip_literals(text)
            for m in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", st):
                k = m.start() - 1
                while k >= 0 and st[k] in " \t\n":
                    k -= 1
                if k >= 0 and (st[k] in "*." or ((st[k].isalnum() or st[k] == "_") and
                                                  re.search(r"(\w+)$", st[:k + 1]).group(1) not in gp._CALL_KEYWORDS)):
                    continue
                spans, close = gp._split_args_at(st, m.end() - 1)
                argc = 0 if not "".join(st[x:y] for x, y in spans).strip() else len(spans)
                index[m[1]].append((path, text, st, m.start(), m.end() - 1, close, argc, spans))
        _INDEX[key] = index
    return _INDEX[key].get(name, [])


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mirror", required=True)
    ap.add_argument("--asm", required=True)
    ap.add_argument("--rows")
    ap.add_argument("--all", action="store_true", help="check every function with a retail address that the tree calls")
    ap.add_argument("callee", nargs="*", help="name[:arity]; the arity defaults to the definition's (or typed prototypes') parameter count, else the largest count any call passes")
    a = ap.parse_args()
    mirror = Path(a.mirror)
    asm = load_asm(a.asm)
    funcs = load_symbols(gp.ROOT)
    report = []
    texts = {p: p.read_text(encoding="utf-8", errors="surrogateescape") for p in sorted((mirror / "src").rglob("*.c"))}
    funcs.update({n: a_ for n, a_ in marker_addresses(texts).items() if n not in funcs})
    headers = [(h, h.read_text(encoding="utf-8", errors="surrogateescape"))
               for h in sorted(list((mirror / "include").rglob("*.h")) + list((mirror / "src").rglob("*.h")))]
    td = gp._typedef_classes(list(texts.values()) + [t for _, t in headers])
    shapes = collections.defaultdict(list)
    for path, text in list(texts.items()) + headers:
        for fn_name, is_def, ret, prm, variadic in gp._scan_functions(gp._strip_literals(text), td):
            shapes[fn_name].append((prm, variadic))
    starts = sorted(set(funcs.values()))
    site_rows = {}                         # (path, position, callee) -> (caller key, depends on the caller gaining parameters, row)
    demand = {}                            # (path, function) -> [parameters needed, declared parameters, address]
    site_fail = collections.Counter()      # callee -> sites with no row
    processed = {}
    queue = collections.deque()
    for spec in a.callee:
        name, _, ar = spec.partition(":")
        queue.append((name, int(ar) if ar else None))
    if a.all:
        call_sites(texts, "")
        for name in sorted(n for n in _INDEX[id(texts)] if n in funcs):
            queue.append((name, None))

    def process(name, ar):
        site_fail[name] = 0
        for key in [k_ for k_ in site_rows if k_[2] == name]:
            del site_rows[key]
        addr = funcs.get(name) or (int(name[5:], 16) if re.fullmatch(r"func_[0-9a-fA-F]{8}", name) else None)
        if addr is None:
            report.append(f"{name}: no retail address")
            site_fail[name] += 1
            return
        sites = call_sites(texts, name)
        counts = collections.Counter(site[6] for site in sites)
        known = {len(prm) for prm, _ in shapes.get(name, []) if prm is not None}
        expect = ar if ar else (next(iter(known)) if len(known) == 1 else max(counts, default=0))
        measured = asm_arity(asm, addr, starts)
        if not ar and varargs_prologue(asm, addr):
            if not a.all:
                report.append(f"{name}: spills a1..a3 at entry, a varargs function; skipped")
            return
        if any(v for _, v in shapes.get(name, [])):
            if not a.all:
                report.append(f"{name}: variadic, skipped")
            return
        if not ar and all(c in ("i", "p") for prm, _ in shapes.get(name, []) if prm for c in prm) and measured != expect:
            if any(site[6] < measured for site in sites):
                report.append(f"{name}: retail reads {measured} argument registers, the decomp declares or calls with {expect}; using {measured}")
            expect = measured
        if a.all and not any(site[6] < expect for site in sites):
            return
        float_pos = {i for prm, _ in shapes.get(name, []) if prm for i, c in enumerate(prm) if c not in ("i", "p")}
        ok = bad = 0
        for path, text, st, pos, open_idx, close, argc, spans in sites:
            if argc >= expect:
                continue
            marker = text.rfind("// FUN_", 0, pos)
            m = re.match(r"// FUN_([0-9A-Fa-f]{8})", text[marker:]) if marker >= 0 else None
            line = text[:pos].count("\n") + 1
            where = f"{path.relative_to(mirror).as_posix()}:{line}"

            def fail(msg):
                nonlocal bad
                report.append(f"{where} {name}: not verified: {msg}")
                bad += 1
                site_fail[name] += 1
            if not m:
                fail("no FUN marker"); continue
            fn = int(m[1], 16)
            end = next((s_ for s_ in starts if s_ > fn), fn + 0x2000)
            hdr = re.search(r"\n([A-Za-z_][\w \t\*]*?)\b([A-Za-z_]\w*)\s*\(([^;{}]*)\)\s*\{", text[marker:])
            if not hdr:
                fail("enclosing definition not found"); continue
            fname = hdr.group(2)
            params = gp._split_params(hdr.group(3)) if hdr.group(3).strip() != "void" else []
            body_start = marker + hdr.end()
            order = [mm.start() for mm in re.finditer(r"\b" + re.escape(name) + r"\s*\(", gp._strip_literals(text[body_start:gp._match_brace(text, body_start - 1)]))]
            nth = sum(1 for p_ in order if body_start + p_ < pos) + 1
            jals = [ad for ad in range(fn, end, 4) if asm.get(ad, ("",))[0] in ("jal", "j") and asm[ad][1] == f"0x{addr:x}"]   # j: a tail call
            if len(jals) != len(order):
                fail(f"{len(order)} source calls, {len(jals)} jal in retail"); continue
            if nth > len(jals) or body_start + order[nth - 1] != pos:
                fail("the call is not inside the function its FUN marker names"); continue
            jal = jals[nth - 1]
            missing = list(range(argc, expect))
            if any(i in float_pos for i in missing):
                fail("an omitted parameter is not an integer/pointer"); continue
            ints = [p_ for p_ in params if gp._abi_class(gp._param_type_text(p_), td) in ("i", "p")]
            why = None
            literal = {}
            for i in missing:
                last_write, calls_before, branch_after = None, False, False
                for ad in range(fn, jal + 8, 4):      # the instruction after the jal is its delay slot
                    mn, ops = asm.get(ad, ("", ""))
                    if writes_reg(mn, ops, f"a{i}"):
                        last_write, branch_after = ad, False
                    elif ad < jal and last_write is not None and (mn in ("jal", "jalr", "j", "jr") or re.match(r"^b", mn)):
                        branch_after = True
                    if ad < jal and mn in ("jal", "jalr"):
                        calls_before = True
                if last_write is None:
                    if calls_before:
                        why = f"call before the jal and a{i} never set"
                        break
                    continue
                mn, ops = asm[last_write]
                im = re.fullmatch(rf"a{i},(?:zero,)?(-?\d+|0x[0-9a-f]+)", ops.replace(" ", ""))
                if branch_after or not (mn in ("li", "addiu", "ori") and im and (mn == "li" or ops.replace(" ", "").startswith(f"a{i},zero,"))):
                    why = f"a{i} written at {last_write:x} ({mn} {ops})"
                    break
                literal[i] = str(int(im.group(1), 0))
            if why:
                fail(why); continue
            names = []
            for p_ in ints:
                t = gp._param_type_text(p_)
                names.append(p_.strip()[len(t):].strip() if p_.strip().startswith(t) else "")
            if any(i < len(names) and not re.fullmatch(r"[A-Za-z_]\w*", names[i]) for i in missing if i not in literal):
                fail("unnamed parameter"); continue
            used = [i for i in missing if i not in literal]
            needs = 1 + max(used) if used else 0
            args = ([text[x:y].strip() for x, y in spans] if argc else []) + \
                   [literal[i] if i in literal else (names[i] if i < len(names) else f"p4_a{i}") for i in missing]
            row = (path.relative_to(mirror).as_posix(), fname, name, nth, ", ".join(args),
                   f"retail {fn:08x} jals {name} at {jal:x}: " + ", ".join(
                       f"a{i} = {literal[i]} (immediate)" if i in literal else f"a{i} untouched since entry" for i in missing))
            caller = (path, fname)
            if needs > len(ints):
                d = demand.setdefault(caller, [0, len(params), fn])
                d[0] = max(d[0], needs)
                if len(ints) != len(params):
                    d[1] = -1          # a non-integer parameter in front: appended registers would be misplaced
            site_rows[(path, pos, name)] = (caller, needs > len(ints), row)
            ok += 1
        if ok or bad or not a.all:
            report.append(f"{name}: expected {expect} arguments, call counts {dict(counts)}: {ok} verified, {bad} not")

    while queue:
        name, ar = queue.popleft()
        if processed.get(name, 0) >= (ar or 0) and name in processed:
            continue
        processed[name] = ar or 0
        process(name, ar)
        for (path, fname), (k, nparams, fn) in list(demand.items()):
            if processed.get(fname, -1) < k:
                queue.append((fname, k))

    # a function that gains parameters is kept only when every call that omits them has a row, and each of
    # those callers either declares the parameter or gains it too
    keep = {c for c, (k, nparams, _fn) in demand.items() if nparams >= 0}
    for c, (k, nparams, _fn) in demand.items():
        if nparams < 0:
            report.append(f"{c[0].relative_to(mirror).as_posix()} {c[1]}: needs {k} integer parameters but has a non-integer one in front")
    changed = True
    while changed:
        changed = False
        for c in sorted(keep, key=lambda c: (str(c[0]), c[1])):
            bad = site_fail.get(c[1], 0)
            blockers = [r for r in site_rows.values() if r[2][2] == c[1] and r[1] and r[0] not in keep]
            if bad or blockers or processed.get(c[1], -1) < demand[c][0]:
                keep.discard(c)
                report.append(f"{c[0].relative_to(mirror).as_posix()} {c[1]}: needs {demand[c][0]} parameters, left alone "
                              f"({bad} calls to it without a verified row, {len(blockers)} callers that cannot take the parameter)")
                changed = True
    rows = []
    for (path, fname), (k, nparams, fn) in sorted(demand.items(), key=lambda kv: (str(kv[0][0]), kv[0][1])):
        if (path, fname) in keep:
            rows.append((path.relative_to(mirror).as_posix(), fname, "@param", nparams,
                         ", ".join(f"void *p4_a{i}" for i in range(nparams, k)),
                         f"retail {fn:08x} reads a{nparams}..a{k - 1} from its caller and passes them on"))
    for caller, conditional, row in site_rows.values():
        if not conditional or caller in keep:
            rows.append(row)
    print("\n".join(report))
    if a.rows:
        Path(a.rows).write_text("".join("\t".join(map(str, r)) + "\n" for r in rows), encoding="utf-8")
        print(f"{len(rows)} rows -> {a.rows}")


if __name__ == "__main__":
    main()
