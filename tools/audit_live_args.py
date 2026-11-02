#!/usr/bin/env python3
"""Live-argument audit: translated port functions and the complete compiled decomp mirror, against retail asm.

The retail asm of a function sets some of the argument registers before each `jal` (or leaves the caller's own incoming ones alone: a passthrough), and the callee reads some of
them. A C call that passes fewer arguments than the callee reads from registers the retail caller had live gives the callee garbage (on i386 the stack word that follows).
For every function with retail asm it checks, per callee T with asm:
  * the parameters of T: the argument registers ($4..$11, $f12..$f19) some path from T's entry reads before writing them (a call clobbers them), and
  * at every `jal T` of the function: the argument registers written since the previous call, or still the incoming ones when no call came before,
and reports the function when the C calls T with fewer arguments than the retail site sets that T reads.
Callees and callers without a file in the asm tree are read from build/all.s, made once with --make-asm (or P4_ALL_ASM=<path>).
Usage: python3 tools/audit_live_args.py [--src] [--mirror BUILD_GEN] [--only ADDR,ADDR] [--all]
"""
import argparse
import json
import collections
import os
import re
import sys
from pathlib import Path

import gen_port
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "mipsdiff"))
import mips  # noqa: E402
import run as rn  # noqa: E402

ROOT = rn.ROOT
STORES = {"sb", "sh", "sw", "sd", "sq", "swc1", "sdc1", "swl", "swr", "sdl", "sdr", "sqc2", "sc", "scd", "swc2", "sdc2"}
LOADS = {"lb", "lbu", "lh", "lhu", "lw", "lwu", "ld", "lq", "lwc1", "ldc1", "lwl", "lwr", "ldl", "ldr", "lqc2", "lwc2", "ldc2", "ll", "lld"}
COND2 = {"beq", "bne", "beql", "bnel"}
COND1 = {"beqz", "bnez", "blez", "bgtz", "bltz", "bgez", "beqzl", "bnezl", "blezl", "bgtzl", "bltzl", "bgezl", "bltzal", "bgezal"}
COND0 = {"b", "bc1t", "bc1f", "bc1tl", "bc1fl", "bc0t", "bc0f", "bc2t", "bc2f"}
FP_NODEST = ("c.", "adda.s", "suba.s", "mula.s", "madda.s", "msuba.s")
# Known binary-only variadic functions; compiled definitions add their own entries.
# vsprintf (00446ed8) takes a fixed third va_list argument, not an ellipsis.
VARARGS = {0x3DF590, 0x4244C8, 0x4CD3F0, 0x4CD378, 0x4E9878, 0x440B68}

REG = re.compile(r"\$(f?)(\w+)")


def regs_in(op):
    out = []
    for m in REG.finditer(op):
        k, n = m.group(1), m.group(2)
        try:
            v = int(n) if n.isdigit() else mips.REG_NAMES["$" + n]
        except KeyError:
            continue
        out.append(("f" if k else "i", v))
    return out


def arg_slot(r):
    kind, n = r
    if kind == "i" and 4 <= n <= 11:
        return r
    if kind == "f" and 12 <= n <= 19:
        return r
    return None


def effects(mn, ops):
    """(reads, writes): the register tuples an instruction reads and writes (all registers; callers filter)."""
    rd, wr = [], []
    if mn in ("nop", "sync", "sync.p", "syscall", "break", "eret"):
        return rd, wr
    if mn == "c1":
        # GNU objdump prints the EE's ft-encoded sqrt as a raw coprocessor
        # operation. Use the same encoding as mipsdiff's interpreter, rather
        # than inventing live incoming FPRs by overlooking its destination.
        code = int(ops[0], 16)
        if code & 0x3f != 4:
            raise ValueError(f"unknown COP1 register effects: {ops[0]}")
        return [("f", (code >> 16) & 31)], [("f", (code >> 6) & 31)]
    if mn in STORES:
        rd += regs_in(ops[0]) + (regs_in(ops[1]) if len(ops) > 1 else [])
        return rd, wr
    if mn in LOADS:
        wr += regs_in(ops[0])
        rd += regs_in(ops[1]) if len(ops) > 1 else []
        if mn in ("lwl", "lwr", "ldl", "ldr"):
            rd += regs_in(ops[0])
        return rd, wr
    if mn in COND2:
        return regs_in(ops[0]) + regs_in(ops[1]), wr
    if mn in COND1:
        return regs_in(ops[0]), wr
    if mn in COND0 or mn in ("j", "jal"):
        return rd, wr
    if mn in ("jr", "jalr"):
        return regs_in(ops[-1]), wr
    if mn in ("mthi", "mtlo", "mthi1", "mtlo1", "mtsa"):
        return regs_in(ops[0]), wr
    if mn in ("mfhi", "mflo", "mfhi1", "mflo1", "mfsa"):
        return rd, regs_in(ops[0])
    if mn in ("mult", "multu", "div", "divu", "madd", "maddu", "mult1", "multu1", "div1", "divu1", "madd1", "maddu1"):
        if len(ops) == 3:
            return regs_in(ops[1]) + regs_in(ops[2]), regs_in(ops[0])
        return regs_in(ops[0]) + regs_in(ops[1]), wr
    if mn in ("mtc1", "ctc1", "mtc0", "ctc2", "qmtc2"):
        return regs_in(ops[0]), regs_in(ops[1])
    if mn in ("mfc1", "cfc1", "mfc0", "cfc2", "qmfc2"):
        return regs_in(ops[1]), regs_in(ops[0])
    if mn.startswith(FP_NODEST):
        for o in ops:
            rd += regs_in(o)
        return rd, wr
    if mn in ("movz", "movn"):
        return regs_in(ops[0]) + regs_in(ops[1]) + regs_in(ops[2]), regs_in(ops[0])
    if mn in ("cache", "pref"):
        for o in ops:
            rd += regs_in(o)
        return rd, wr
    if not ops:
        return rd, wr
    wr += regs_in(ops[0])
    for o in ops[1:]:
        rd += regs_in(o)
    if mn.startswith("madd.s") or mn.startswith("msub.s"):
        pass
    return rd, wr


class _LazyFx(dict):
    def __init__(self, insns):
        super().__init__()
        self.insns = insns

    def __missing__(self, a):
        v = self[a] = effects(*self.insns[a])
        return v

    def get(self, a, default=None):
        return self[a] if a in self.insns else default


FLAT_LINE = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \t(\S+)\s*(.*)$")
_FLAT = None


def flat_asm():
    """The whole retail text as {address: (mnemonic, operands)} from `objdump -d -m mips:5900 -M reg-names=numeric` (build/all.s, made by --make-asm), or None."""
    global _FLAT
    if _FLAT is None:
        path = os.environ.get("P4_ALL_ASM", os.path.join(ROOT, "build", "all.s"))
        _FLAT = {}
        if os.path.isfile(path):
            for line in open(path, errors="replace"):
                m = FLAT_LINE.match(line)
                if m:
                    _FLAT[int(m.group(1), 16)] = (m.group(2), mips.split_ops(m.group(3).split("#")[0].split("<")[0].strip()))
    return _FLAT or None


class Func:
    def __init__(self, addr, text=None):
        self.addr = addr
        if text is None:
            flat = flat_asm()
            if flat is None or addr not in flat:
                raise KeyError(addr)
            self.insns, self.labels, self.start = flat, {}, addr
            self.order = None
            self.fx = _LazyFx(flat)
            return
        self.insns, self.labels, self.start = mips.parse_asm(text)
        self.order = sorted(self.insns)
        self.fx = {a: effects(*self.insns[a]) for a in self.order}

    def is_call(self, a):
        return self.insns[a][0] in ("jal", "jalr")

    def target(self, a):
        mn, ops = self.insns[a]
        if not ops:
            return None
        if ops[-1] in self.labels:
            return self.labels[ops[-1]]
        if ops[-1].startswith("0x"):
            return int(ops[-1], 16)
        return None

    opaque = False

    def param_slots(self):
        """Argument registers some path reads before writing them (a call clobbers them). None when a path ends in a jump through a register or a tail call."""
        out = set()
        cand = [("i", n) for n in range(4, 12)] + [("f", n) for n in range(12, 20)]
        for r in cand:
            seen = set()
            stack = [self.start]
            hit = False
            while stack and not hit:
                pc = stack.pop()
                while pc in self.insns and pc not in seen:
                    seen.add(pc)
                    mn, ops = self.insns[pc]
                    rd, wr = self.fx[pc]
                    if r in rd:
                        hit = True
                        break
                    ctl = mn in COND0 or mn in COND1 or mn in COND2 or mn in ("j", "jal", "jalr", "jr")
                    if not ctl:
                        if r in wr:
                            break
                        pc += 4
                        continue
                    drd, dwr = self.fx.get(pc + 4, ([], []))
                    if r in drd:
                        hit = True
                        break
                    if r in dwr:
                        break
                    if mn in ("jal", "jalr"):
                        break                      # the call clobbers every argument register
                    if mn == "jr":
                        if ops and ops[0] not in ("$31", "$ra"):
                            self.opaque = True      # a jump through a register (a thunk to a function pointer): the registers it passes on are unknown
                        break
                    tgt = self.target(pc)
                    if mn == "j":
                        if tgt is not None and self.order is not None:
                            stack.append(tgt)
                        else:
                            self.opaque = True          # a tail call into another function
                        break
                    if tgt is not None:
                        stack.append(tgt)
                    if mn == "b" or mn in ("bc1t", "bc1f") and False:
                        break
                    pc += 8
                    continue
            if hit:
                out.add(r)
        return None if self.opaque else out

    def succs(self, pc):
        """Control successors of the instruction (pair) at pc: (next pcs, clears registers)."""
        mn, ops = self.insns[pc]
        if mn in ("jal", "jalr"):
            return [pc + 8], True
        if mn == "jr":
            return [], False
        if mn == "j":
            tgt = self.target(pc)
            return ([tgt] if (tgt is not None and self.order is not None) else []), False
        if mn in COND0 or mn in COND1 or mn in COND2:
            tgt = self.target(pc)
            out = [tgt] if tgt is not None else []
            if mn != "b":
                out.append(pc + 8)
            return out, False
        return [pc + 4], False

    def sites(self, resolve):
        """[(pc, callee address, explicit slots, incoming slots)] for every jal to a known function: the argument registers that may hold a value the
        function wrote since its last call (explicit) or the caller's own incoming one (nothing written, no call since entry)."""
        allregs = frozenset([("i", n) for n in range(4, 12)] + [("f", n) for n in range(12, 20)])
        state = {self.start: (frozenset(), allregs)}      # pc -> (W, I)
        work = [self.start]
        sites = {}
        while work:
            pc = work.pop()
            if pc not in self.insns:
                continue
            W, I = state[pc]
            mn, ops = self.insns[pc]
            ctl = mn in COND0 or mn in COND1 or mn in COND2 or mn in ("j", "jal", "jalr", "jr")
            rd, wr = self.fx[pc]
            w2 = {arg_slot(r) for r in wr if arg_slot(r)}
            if ctl:
                dw = {arg_slot(r) for r in self.fx.get(pc + 4, ([], []))[1] if arg_slot(r)}
                W1 = W | frozenset(dw)
                I1 = I - frozenset(dw)
                if mn == "jal":
                    tgt = resolve(ops[0]) if ops else None
                    prev = sites.get(pc)
                    sites[pc] = (tgt, (prev[1] | W1) if prev else W1, (prev[2] | I1) if prev else I1)
                nxt, clears = self.succs(pc)
                if clears:
                    W1, I1 = frozenset(), frozenset()
            else:
                W1 = W | frozenset(w2)
                I1 = I - frozenset(w2)
                nxt = [pc + 4]
            for n in nxt:
                old = state.get(n)
                new = (W1, I1) if old is None else (old[0] | W1, old[1] | I1)
                if old != new:
                    state[n] = new
                    work.append(n)
        return [(pc, v[0], v[1], v[2]) for pc, v in sorted(sites.items())]


def load_func(addr):
    try:
        return Func(addr, rn.find_asm(addr))
    except (KeyError, OSError):
        pass
    try:
        return Func(addr)
    except KeyError:
        return None


def callee_int_arity(addr):
    """Number of integer argument registers ($4..$7, in order) the retail function at addr reads, or None when there is no asm for it (a native or a libc function).
    Used by tools/mipsdiff/run.py: a stub compares at least this many of the arguments it receives."""
    f = _FUNCS.get(addr, 0)
    if f == 0:
        f = load_func(addr)
        _FUNCS[addr] = f
    if f is None or addr in VARARGS:
        return None
    ps = f.param_slots()
    if ps is None:
        return None
    n = 0
    for k in range(4, 8):
        if ("i", k) in ps:
            n = k - 3
    return n


_FUNCS = {}
_SITES = {}
_SYMS = None


def site_int_arity(fn, callee):
    """Integer argument registers ($4..$7, in order) that the retail function fn sets (or leaves incoming) at its jal to callee and callee reads, or None when either has no asm.
    Used by tools/mipsdiff/run.py to decide how many arguments a stub must compare."""
    global _SYMS
    if _SYMS is None:
        _SYMS = load_symbols()

    def resolve(name):
        if name in _SYMS:
            return _SYMS[name]
        m = re.match(r"func_([0-9a-fA-F]{8})$", name)
        return int(m.group(1), 16) if m else None

    if fn not in _SITES:
        try:
            _SITES[fn] = Func(fn, rn.find_asm(fn)).sites(resolve)
        except (KeyError, OSError):
            _SITES[fn] = None
    sites = _SITES[fn]
    cf = _FUNCS.get(callee, 0)
    if cf == 0:
        cf = load_func(callee)
        _FUNCS[callee] = cf
    if sites is None or cf is None or callee in VARARGS:
        return None
    ps = cf.param_slots()
    if ps is None:
        return None
    n = 0
    for pc, tgt, expl, inc in sites:
        if tgt == callee:
            for k in range(4, 8):
                if ("i", k) in ps and (("i", k) in expl or ("i", k) in inc):
                    n = max(n, k - 3)
    return n


def slots_needed(prov, params, aggregate_return=False):
    """Explicit source arguments only: MWCC passes an aggregate result buffer in a0."""
    hi = {"i": -1, "f": -1}
    for r in prov & params:
        pos = r[1] - ((5 if aggregate_return else 4) if r[0] == "i" else 12)
        hi[r[0]] = max(hi[r[0]], pos)
    return (hi["i"] + 1) + (hi["f"] + 1), hi


def load_symbols():
    symbols = gen_port.read_symbols(Path(ROOT) / "decomp")
    symbols.update(gen_port.read_port_symbols())
    funcs = {name: addr for name, (addr, kind) in symbols.items() if kind == "func"}
    funcs.update({name: addr for addr, name in gen_port.read_native_names().items()})
    return funcs


def source_abi_metadata(files, symbols, headers=()):
    """Find ellipses and hidden aggregate-result pointers, neither of which is an omitted C argument."""
    texts = {Path(path): Path(path).read_text(encoding="utf-8", errors="replace") for path, _addr, _is_src in files}
    texts.update({Path(path): Path(path).read_text(encoding="utf-8", errors="replace") for path in headers})
    typedefs = gen_port._typedef_classes(texts.values())
    variadic = set(VARARGS)
    aggregate_returns = set()
    for name in ("printf", "fprintf", "sprintf", "snprintf", "scePrintf"):
        if name in symbols:
            variadic.add(symbols[name])
    for text in texts.values():
        for name, is_definition, ret, _params, has_ellipsis in gen_port._scan_functions(
                gen_port._strip_literals(text), typedefs):
            if not is_definition:
                continue
            generic = re.fullmatch(r"func_([0-9a-fA-F]{8})", name)
            addr = int(generic[1], 16) if generic else symbols.get(name)
            if addr is not None:
                if has_ellipsis:
                    variadic.add(addr)
                if ret == "@agg":
                    aggregate_returns.add(addr)
    return variadic, aggregate_returns


def split_args(text, i):
    """Number of top-level arguments of the call whose '(' is at text[i]; (count, end index)."""
    depth, n, seen, j = 0, 0, False, i
    while j < len(text):
        c = text[j]
        if c in "([{":
            depth += 1
            if depth > 1:
                seen = True
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                return (n + 1 if seen else 0), j
        elif c == "," and depth == 1:
            n += 1
        elif depth >= 1 and not c.isspace():
            seen = True
        j += 1
    return 0, j


def c_calls(text):
    """{local C name: [(arg count, line)]} for calls inside function bodies (indented or after a '{'), and the extern alias map name -> retail address."""
    # Preserve literal tokens as one argument, without treating their commas, braces or
    # apparent function names as C syntax. Keep original offsets and line numbers.
    def mask(match):
        value = match.group(0)
        blank = re.sub(r"[^\n]", " ", value)
        return "0" + blank[1:] if value.startswith(("\"", "'")) else blank

    t = gen_port._LITERAL_RE.sub(mask, text)
    alias_text = gen_port._LITERAL_RE.sub(
        lambda m: m.group(0) if m.group(0).startswith(("\"", "'"))
        else re.sub(r"[^\n]", " ", m.group(0)), text)
    alias = {}
    for m in re.finditer(r"(?m)^[ \t]*(?:extern[ \t]+)?[A-Za-z_][\w \t*]*?[ \t*](\w+)\s*"
                         r"\([^;{}]*\)\s*__asm__\(\"(\w+)\"\)", alias_text):
        alias[m.group(1)] = m.group(2)
    calls = collections.defaultdict(list)
    depth = 0
    for m in re.finditer(r"[{}]|\b[A-Za-z_]\w*\s*\(", t):
        s = m.group(0)
        if s == "{":
            depth += 1
        elif s == "}":
            depth -= 1
        elif depth > 0:
            name = s[:-1].strip()
            if name in ("if", "while", "for", "switch", "return", "sizeof", "defined"):
                continue
            ls = t.rfind("\n", 0, m.start()) + 1
            if re.match(r"\s*(extern|typedef)\b", t[ls:m.start()]):
                continue                                  # a block-scope declaration, not a call
            n, _ = split_args(t, m.end() - 1)
            calls[name].append((n, t.count("\n", 0, m.start()) + 1))
    return calls, alias


def functions_in(text, symbols):
    """{retail address: (opening brace, closing brace, C name)} for named/marked bodies."""
    syntax = gen_port._strip_literals(text)
    out = {}
    for m in re.finditer(r"(?m)^[A-Za-z_][^;{}()]*?\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{", syntax):
        name = m.group(1)
        generic = re.fullmatch(r"func_([0-9a-fA-F]{8})", name)
        addr = int(generic[1], 16) if generic else symbols.get(name)
        if addr is not None:
            start = m.end() - 1
            out[addr] = (start, gen_port._match_brace(text, start) - 1, name)
    # Markers also cover static retail functions and function-pointer-return syntax
    # that is not shaped like an ordinary declaration.
    for marker in gen_port.MARKER_RE.finditer(text):
        name = gen_port.marker_definition_name(text, marker.end())
        if name is None:
            continue
        start = syntax.find("{", marker.end())
        if start >= 0:
            out[int(marker[1], 16)] = (start, gen_port._match_brace(text, start) - 1, name)
    return out, text


def mirror_files(gen):
    """Only active compilation units, never excluded alternate-owner C copies."""
    manifest = os.path.join(gen, "units.txt")
    files = []
    with open(manifest, encoding="utf-8") as stream:
        for line, raw in enumerate(stream, 1):
            unit = raw.split("|", 1)[0].strip().replace("\\", "/")
            if not unit:
                continue
            _prefix, separator, relative = unit.rpartition("/decomp/")
            if not separator or not relative.startswith("src/") or ".." in relative.split("/"):
                raise ValueError(f"{manifest}:{line}: expected a compiled /decomp/src/ unit")
            path = os.path.join(gen, "decomp", *relative.split("/"))
            if not os.path.isfile(path):
                raise FileNotFoundError(path)
            files.append((path, None, True))
    return files


def model_required_args(target, constants):
    """Semantic reads, not prologue spills or intentionally unused model/packet tail words."""
    if target == 0x43c0c0:
        # SKSOUND.IRX 0xa70: kind, opcode, then one/two/three control values.
        opcode = constants[1] if len(constants) > 1 else None
        return {0x8f: 3, 0x82: 3, 0x81: 3, 0x80: 4, 0x84: 4, 0x83: 5}.get(opcode)
    if target == 0x43c230:
        # SKHSYNTH.IRX 0x830: bank setup has six words; load only reads through its limit.
        opcode = constants[0] if constants else None
        return {1: 4, 2: 4, 3: 6, 5: 4}.get(opcode)
    if target == 0x43c3b0:
        # SKSESQ.IRX 0x770: registration reads size; player creation stops at data entry.
        opcode = constants[0] if constants else None
        return {0: 4, 5: 3}.get(opcode)
    if target == 0x43c518:
        # SKSESQ.IRX 0xcfc/0xf08: release ignores seq; start consumes it.
        opcode = constants[1] if len(constants) > 1 else None
        return {2: 4, 0: 5, 7: 5}.get(opcode)
    return {
        0x43c470: 2, 0x43c5e8: 3,  # fixed packet payloads; optional incoming spills are not sent
        0x4f0a60: 3, 0x4f0c78: 4,  # RE4 SVM model deliberately has no retail debug-name field
        0x3b88c0: 0,               # native skin no-op; retail's ignored identity result is not read
        0x421580: 1, 0x421590: 1, 0x4215a0: 1, 0x4215b0: 1,  # syscall interrupt cause
        0x42b3e0: 0,               # unidentified kernel-call model ignores its argument, returns -1
        0x426cf0: 2,               # sce_file.c explicitly ignores the host-only mode formal
        0x3b6f00: 1,               # retail/native random seeder explicitly ignores its second formal
    }.get(target)


def compiled_call_inventory(report, files):
    """Only a complete Clang inventory of these active units can replace lexical call discovery."""
    units = {path.replace("\\", "/").rpartition("/decomp/")[2] for path, _addr, _src in files}
    if report.get("errors") or set(report.get("active_units", ())) != units:
        raise ValueError("compiled-call report is incomplete or belongs to different active units")
    calls = report.get("calls", ())
    if len(calls) != report.get("counts", {}).get("calls"):
        raise ValueError("compiled-call report does not contain every observed call")
    scopes = collections.defaultdict(lambda: collections.defaultdict(list))
    for call in calls:
        if call["caller"] is not None:
            scopes[call["unit"]][call["caller"]].append(call)
    return scopes


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--src", action="store_true", help="also retail-addressed definitions of src/**/*.c, including renamed functions")
    ap.add_argument("--mirror", metavar="BUILD_GEN", help="also every active C unit of BUILD_GEN/decomp, from BUILD_GEN/units.txt")
    ap.add_argument("--compiled-calls", help="complete audit_prototypes --classes JSON; use actual preprocessed calls, not inactive NON_MATCHING source")
    ap.add_argument("--only", default="")
    ap.add_argument("--all", action="store_true", help="also list calls whose callee has no asm")
    ap.add_argument("--make-asm", action="store_true", help="write build/all.s (the whole retail text, objdump, through WSL) and exit")
    a = ap.parse_args()
    if a.compiled_calls and not a.mirror:
        ap.error("--compiled-calls requires --mirror")
    if a.make_asm:
        import subprocess
        out = os.path.join(ROOT, "build", "all.s")
        os.makedirs(os.path.dirname(out), exist_ok=True)
        elf = os.path.abspath(rn.RETAIL)
        cmd = "mipsel-linux-gnu-objdump -d -m mips:5900 -M reg-names=numeric --start-address=0x100000 --stop-address=0x530000 '%s' > '%s'" % (rn.wsl_path(elf), rn.wsl_path(out))
        sys.exit(subprocess.run(["wsl", "-d", "Debian", "--", "sh", "-c", cmd]).returncode)
    sym = load_symbols()
    if a.mirror:
        sym.update(gen_port.retail_function_names(Path(ROOT) / "decomp", Path(a.mirror)))
    active_symbols = sym

    def resolve(name):
        if name in active_symbols:
            return active_symbols[name]
        m = re.match(r"func_([0-9a-fA-F]{8})$", name)
        return int(m.group(1), 16) if m else None

    asm_cache, param_cache = {}, {}

    def func(addr):
        if addr not in asm_cache:
            asm_cache[addr] = load_func(addr)
        return asm_cache[addr]

    def params(addr):
        if addr not in param_cache:
            f = _FUNCS.get(addr, 0)
            if f == 0:
                f = _FUNCS[addr] = load_func(addr)
            param_cache[addr] = f.param_slots() if f else None
        return param_cache[addr]

    files = []
    odir = os.path.join(ROOT, "overrides")
    if not a.compiled_calls:  # the compiled mirror already contains every spliced override
        for fn in sorted(os.listdir(odir)):
            m = re.match(r"^([0-9a-f]{8})\.c$", fn)
            if m:
                files.append((os.path.join(odir, fn), int(m.group(1), 16), None))
    if a.src:
        for dp, _, fns in os.walk(os.path.join(ROOT, "src")):
            for fn in fns:
                if fn.endswith(".c"):
                    files.append((os.path.join(dp, fn), None, True))
    mirrors = mirror_files(a.mirror) if a.mirror else []
    files.extend(mirrors)
    compiled = None
    if a.compiled_calls:
        compiled = compiled_call_inventory(json.loads(Path(a.compiled_calls).read_text()), mirrors)
    mirror_paths = {Path(path).resolve() for path, _addr, _src in mirrors}
    semantic_needs = {}
    headers = list((Path(ROOT) / "include").rglob("*.h"))
    if a.mirror:
        headers.extend((Path(a.mirror) / "decomp").rglob("*.h"))
    variadic, aggregate_returns = source_abi_metadata(files, sym, headers)
    only = {int(x, 16) for x in a.only.split(",") if x}
    n_checked = n_flagged = 0
    for path, addr, is_src in files:
        active_symbols = sym
        if compiled is not None and Path(path).resolve() in mirror_paths:
            unit = path.replace("\\", "/").rpartition("/decomp/")[2]
            scoped = {}
            for name, call_rows in compiled.get(unit, {}).items():
                fa = resolve(name)
                if fa is None:
                    if a.all:
                        print(f"{unit}: compiled caller {name} has no retail address")
                    continue
                cc = collections.defaultdict(list)
                for call in call_rows:
                    target_name = call["linked"] or call["callee"]
                    if target_name is None:
                        continue  # dynamic targets remain explicit in the Clang report
                    cc[target_name].append((call["passed"], call["line"]))
                    semantic_needs[fa, target_name, call["line"]] = model_required_args(
                        resolve(target_name), call["constant_args"])
                scoped[fa] = (cc, {}, 0)
            targets = scoped
        else:
            text = open(path, encoding="utf8", errors="replace").read()
            if addr is not None:
                targets = {addr: None}
                calls, alias = c_calls(text)
                scoped = {addr: (calls, alias, 0)}
            else:
                defs, t = functions_in(text, sym)
                if not defs:
                    continue
                targets = defs
                active_symbols = dict(sym)
                active_symbols.update({name: addr for addr, (_, _, name) in defs.items()})
                _, alias = c_calls(text)
                scoped = {}
                for fa, (s, e, name) in defs.items():
                    body = t[s:e + 1]
                    cc, _al = c_calls("void f()" + body)
                    scoped[fa] = (cc, alias, t.count("\n", 0, s))
        for fa in targets:
            if only and fa not in only:
                continue
            f = func(fa)
            if f is None:
                if a.all:
                    print("%08x %s: caller has no retail asm" %
                          (fa, os.path.relpath(path, ROOT).replace("\\", "/")))
                continue
            n_checked += 1
            cc, al, line_offset = scoped[fa]
            by_addr = collections.defaultdict(list)
            for name, lst in cc.items():
                tgt = resolve(al.get(name, name))
                if tgt is not None and tgt != fa:
                    by_addr[tgt] += [(n, line + line_offset,
                                      semantic_needs.get((fa, name, line + line_offset), model_required_args(tgt, ())))
                                     for n, line in lst]
            for pc, tgt, expl, inc in f.sites(resolve):
                if tgt is None or tgt not in by_addr or tgt == fa:
                    continue
                if tgt in variadic:
                    continue
                ps = params(tgt)
                if ps is None:
                    if a.all:
                        print("%08x %s: callee %08x argument flow unavailable (missing asm or opaque tail/indirect flow)" %
                              (fa, os.path.basename(path), tgt))
                    continue
                need_e, hi_e = slots_needed(expl, ps, tgt in aggregate_returns)
                need_i, hi_i = slots_needed(expl | inc, ps, tgt in aggregate_returns)
                missing = [(n, line, required if required is not None else need_i)
                           for n, line, required in by_addr[tgt] if n < (required if required is not None else need_i)]
                if missing:
                    cmin = min(n for n, _line, _need in missing)
                    required = max(need for _n, _line, need in missing)
                    n_flagged += 1
                    source_lines = ",".join(str(line) for _n, line, _need in missing)
                    print("%08x %s:%s call %08x at %08x: C passes %d, callee reads %d (%s)  [%s]" % (
                        fa, os.path.relpath(path, ROOT).replace("\\", "/"), source_lines, tgt, pc, cmin, required,
                        "set by the function" if need_e > cmin else "incoming passthrough only",
                        "/".join(sorted("%s%d" % (r[0], r[1]) for r in ((expl | inc) & ps)))))
    print("checked %d functions, %d under-passing calls" % (n_checked, n_flagged))


if __name__ == "__main__":
    main()
