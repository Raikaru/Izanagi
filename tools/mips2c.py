#!/usr/bin/env python3
"""A literal R5900 -> C translator for float library code (the retail libm cores), used once per function to produce a faithful C translation.

    python3 tools/mips2c.py --elf SLUS_217.82 --spec tools/mips2c/libm.spec -o src/platform/ee_libm.c [--objdump mipsel-linux-gnu-objdump]

The output is one C function per retail function, instruction for instruction: every general register and float register is a C local, branches are gotos
(delay slots and branch-likely nullification are emitted as the hardware runs them), the stack frame is a local array, the data the code reads is a static
array copied from the retail image, and every float operation is a plain C operation. The decomp mirror's compiler launcher (tools/ee_float_cc.py) gives those
operations the EE's rounding, so the translation is exact by construction, not by a reading of fdlibm: the operation order, the constants and the integer
tricks are the retail ones. Registers that hold addresses are `intptr_t`, the rest wrap to 32 bits, so the output also builds for 64-bit hosts.

Spec file: one directive per line.
    window <from> <to>               a data range (hex, retail addresses) copied into a static array; code addresses into it become host pointers
    func <addr> <ret> <args|-> <end> [name]   ret: f float, i int, v void; args: one letter each: f float, i int, p pointer (floats take f12.., ints a0.., as the EE
                                     does); end: the first address after the function
Lines starting with # are comments.
"""
import argparse
import re
import struct
import subprocess
import sys

# Every generated file starts with the licence carve-out and the fdlibm notice (NOTICE).
SPDX_HEADER = ("/* SPDX-License-Identifier: LicenseRef-reconstructed */\n"
               "/* fdlibm notice, for the code these functions translate:\n"
               " * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.\n"
               " * Developed at SunPro, a Sun Microsystems, Inc. business.\n"
               " * Permission to use, copy, modify, and distribute this software is freely granted,\n"
               " * provided that this notice is preserved. */\n")
ARG_INT = ["a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3"]
GPR = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
       "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"]

INS_RE = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]+ \t(\S+)\s*(.*)$")


def hexint(s):
    return int(s, 16)


class Func:
    def __init__(self, addr, ret, args, name):
        self.addr, self.ret, self.args, self.name = addr, ret, args, name
        self.ins = []        # (addr, mnem, [operands])
        self.end = None


def load_elf(path):
    data = open(path, "rb").read()
    phoff = struct.unpack_from("<I", data, 0x1C)[0]
    phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)
    segs = []
    for i in range(phnum):
        t, off, va, pa, fs, ms, fl, al = struct.unpack_from("<8I", data, phoff + i * phentsize)
        if t == 1:
            segs.append((va, off, fs))
    def word(a):
        for va, off, fs in segs:
            if va <= a < va + fs:
                return struct.unpack_from("<I", data, off + a - va)[0]
        raise KeyError(a)
    return word


def disassemble(objdump, elf, lo, hi):
    out = subprocess.run([objdump, "-D", "-z", "-mmips:5900", f"--start-address=0x{lo:x}", f"--stop-address=0x{hi:x}", elf],
                         capture_output=True, text=True, check=True).stdout
    ins = []
    for line in out.splitlines():
        m = INS_RE.match(line)
        if m:
            ops = [o.strip() for o in m.group(3).split("#")[0].split(",")] if m.group(3).strip() else []
            ins.append((int(m.group(1), 16), m.group(2), ops))
    return ins


class Lifter:
    def __init__(self, word, windows):
        self.word = word
        self.windows = windows           # [(lo, hi)]
        self.protos = {}

    # --- operand helpers -------------------------------------------------
    @staticmethod
    def is_reg(s):
        return s in GPR

    def lift(self, fn, ins, targets_extra=()):
        """Returns the C text of one function."""
        by_addr = {a: (m, o) for a, m, o in ins}
        addrs = [a for a, _, _ in ins]
        lo, hi = addrs[0], addrs[-1] + 4
        branches = {"b", "beq", "bne", "beqz", "bnez", "bltz", "bgez", "blez", "bgtz", "beql", "bnel", "beqzl", "bnezl", "bltzl", "bgezl", "blezl", "bgtzl",
                    "bc1t", "bc1f", "bc1tl", "bc1fl", "j", "jal", "jr", "jalr"}
        likely = {"beql", "bnel", "beqzl", "bnezl", "bltzl", "bgezl", "blezl", "bgtzl", "bc1tl", "bc1fl"}
        delay = set()
        targets = set()
        for a, m, o in ins:
            if m in branches:
                delay.add(a + 4)
                if m in ("jal", "jr", "jalr"):
                    continue
                t = int(o[-1], 16)
                if lo <= t < hi:
                    targets.add(t)
        # a branch target inside a delay slot needs its own copy of that instruction
        out = []
        emit = out.append

        def in_window(v):
            return any(lo_ <= v < hi_ for lo_, hi_ in self.windows)
        ints = [k for k in fn.args if k in "ip"]
        entry_taint = {"sp"}
        for i_, k in enumerate(ints):
            if k == "p":
                entry_taint.add(ARG_INT[i_])
        taint_ops = ("move", "addiu", "addu", "subu", "daddu", "or")

        def transfer(state, m, o):
            """state: (frozenset of registers that hold host addresses, dict reg -> known constant, frozenset of frame slots that hold a host address)."""
            tn, cs, ts = set(state[0]), dict(state[1]), set(state[2])
            if m in ("sw", "lw") and o and len(o) == 2:
                # an address spilled to the frame and reloaded is an address again (the 64-bit register is kept in shw)
                slot = re.match(r"(-?\d+|0x[0-9a-f]+)\(sp\)", o[1])
                if slot is not None:
                    off = int(slot.group(1), 0)
                    if m == "sw":
                        (ts.add if o[0] in tn else ts.discard)(off)
                    elif o[0] in GPR:
                        cs.pop(o[0], None)
                        (tn.add if off in ts else tn.discard)(o[0])
                    return (frozenset(tn), cs, frozenset(ts))
            if not o or m in ("sw", "swc1", "sd", "sb", "sh", "ctc2", "lwc1", "mtc1", "nop", "vnop", "vwaitq") or m.startswith(("b", "j", "c.")):
                return (frozenset(tn), cs, frozenset(ts))
            d = o[0]
            if d not in GPR:
                return (frozenset(tn), cs, frozenset(ts))
            srcs = [x for x in o[1:] if x in GPR]
            if m == "lui":
                cs[d] = (int(o[1], 0) << 16) & 0xFFFFFFFF
                tn.discard(d)
            elif m in ("addiu", "ori") and len(o) == 3 and o[1] in cs:
                v = (cs[o[1]] + int(o[2], 0)) & 0xFFFFFFFF if m == "addiu" else cs[o[1]] | int(o[2], 0)
                cs[d] = v
                if in_window(v):
                    tn.add(d)
                else:
                    tn.discard(d)
            else:
                cs.pop(d, None)
                if m in taint_ops and any(s in tn for s in srcs):
                    tn.add(d)
                else:
                    tn.discard(d)
            if m == "jal":
                pass
            return (frozenset(tn), cs, frozenset(ts))

        def join(a, b):
            if a is None:
                return b
            cs = {k: v for k, v in a[1].items() if b[1].get(k) == v}
            return (a[0] | b[0], cs, a[2] | b[2])

        index = {a: k for k, (a, _, _) in enumerate(ins)}
        st = {}
        work = [(addrs[0], (frozenset(entry_taint), {}, frozenset()))]
        while work:
            a, s = work.pop()
            if a not in index:
                continue
            new_s = join(st.get(a), s)
            if st.get(a) == new_s:
                continue
            st[a] = new_s
            m, o = by_addr[a]
            out_s = transfer(new_s, m, o)
            if a in delay:
                # the instruction after a branch: continues to the branch's targets
                b_addr = a - 4
                bm, bo = by_addr[b_addr]
                succ = []
                if bm in ("jr",):
                    succ = []
                elif bm == "jal" or bm == "jalr":
                    succ = [a + 4]
                elif bm == "j":
                    succ = [int(bo[0], 16)]
                elif bm == "b":
                    succ = [int(bo[-1], 16)]
                else:
                    succ = [int(bo[-1], 16), a + 4]
                for x in succ:
                    work.append((x, out_s))
            elif m in branches:
                work.append((a + 4, out_s))
            else:
                work.append((a + 4, out_s))
        def taint_at(a):
            return st.get(a, (frozenset(), {}, frozenset()))[0]
        def R(r):
            return "0" if r == "zero" else r
        def int_expr(r):
            return R(r)
        def wrap(e, dst_taint):
            return e if dst_taint else f"W({e})"
        body = []
        label_for = lambda t: f"L_{t:06x}"
        fnames = self.protos

        def reg_t(r):
            return r in taint

        def fp(x):
            return x.replace("$", "")

        def sp_slot(op):
            m = re.match(r"(-?\d+|0x[0-9a-f]+)\((\w+)\)", op)
            off = int(m.group(1), 0)
            return off if m.group(2) == "sp" and off >= 0 and off % 4 == 0 else None

        def mem_addr(op):
            m = re.match(r"(-?\d+|0x[0-9a-f]+)\((\w+)\)", op)
            off = int(m.group(1), 0)
            base = m.group(2)
            return f"(uintptr_t)({R(base)}{off:+d})" if off else f"(uintptr_t)({R(base)})"

        def stmt(a, m, o, nullified_ok=True):
            """C statement(s) (without a trailing newline) for a non-control instruction."""
            taint = taint_at(a)
            const = st.get(a, (None, {}, None))[1]
            if m == "nop" or m == "vnop" or m == "vwaitq" or m == "sync":
                return ""
            if m in ("sd", "ld", "sq", "lq"):
                return ""      # callee-saved register traffic of the prologue and epilogue
            if m == "addiu" and o[0] == "sp" and o[1] == "sp":
                return ""
            if m == "lui":
                v = (int(o[1], 0) << 16) & 0xFFFFFFFF
                return f"{o[0]} = W({v:#x}u);"
            if m in ("addiu", "ori") and len(o) == 3 and o[1] in const and o[0] != "zero":
                base = const[o[1]]
                imm = int(o[2], 0)
                v = (base + imm) & 0xFFFFFFFF if m == "addiu" else base | imm
                if in_window(v):
                    return f"{o[0]} = (intptr_t)K_{self.win_id(v)} + {v - self.win_base(v)}; /* {v:#x} */"
                return f"{o[0]} = W({v:#x}u);"
            if m in ("move",):
                d, s = o
                return f"{d} = {R(s)};"
            if m == "li":
                return f"{o[0]} = {int(o[1], 0)};"
            if m == "negu":
                return f"{o[0]} = W(-{R(o[1])});"
            if m in ("addu", "daddu"):
                d, s, t = o
                return f"{d} = " + (f"{R(s)} + {R(t)};" if (s in taint or t in taint) else f"W({R(s)} + {R(t)});")
            if m == "subu":
                d, s, t = o
                return f"{d} = " + (f"{R(s)} - {R(t)};" if (s in taint or t in taint) else f"W({R(s)} - {R(t)});")
            if m == "addiu":
                d, s, imm = o
                imm = int(imm, 0)
                return f"{d} = " + (f"{R(s)} + {imm};" if s in taint else f"W({R(s)} + {imm});")
            if m in ("and", "or", "xor", "nor"):
                d, s, t = o
                op = {"and": "&", "or": "|", "xor": "^"}.get(m)
                if m == "nor":
                    return f"{d} = W(~({R(s)} | {R(t)}));"
                return f"{d} = W({R(s)} {op} {R(t)});"
            if m in ("andi", "ori", "xori"):
                d, s, imm = o
                op = {"andi": "&", "ori": "|", "xori": "^"}[m]
                return f"{d} = W({R(s)} {op} {int(imm, 0)}u);"
            if m == "sll":
                return f"{o[0]} = W((uint32_t){R(o[1])} << {int(o[2], 0)});"
            if m == "srl":
                return f"{o[0]} = W((uint32_t){R(o[1])} >> {int(o[2], 0)});"
            if m == "sra":
                return f"{o[0]} = W((int32_t){R(o[1])} >> {int(o[2], 0)});"
            if m == "sllv":
                return f"{o[0]} = W((uint32_t){R(o[1])} << ({R(o[2])} & 31));"
            if m == "srlv":
                return f"{o[0]} = W((uint32_t){R(o[1])} >> ({R(o[2])} & 31));"
            if m == "srav":
                return f"{o[0]} = W((int32_t){R(o[1])} >> ({R(o[2])} & 31));"
            if m == "slt":
                return f"{o[0]} = (int32_t){R(o[1])} < (int32_t){R(o[2])};"
            if m == "sltu":
                return f"{o[0]} = (uint32_t){R(o[1])} < (uint32_t){R(o[2])};"
            if m == "slti":
                return f"{o[0]} = (int32_t){R(o[1])} < {int(o[2], 0)};"
            if m == "sltiu":
                return f"{o[0]} = (uint32_t){R(o[1])} < (uint32_t)({int(o[2], 0)});"
            if m == "movn":
                return f"if ({R(o[2])} != 0) {o[0]} = {R(o[1])};"
            if m == "movz":
                return f"if ({R(o[2])} == 0) {o[0]} = {R(o[1])};"
            if m == "lw":
                so = sp_slot(o[1])
                if so is not None:
                    # a word spilled to the frame may be an address (a 64-bit host pointer): the frame word keeps the low 32 bits, shw the register, and a
                    # reload that finds the word untouched gets the register back
                    return f"{{ int32_t w_ = *(int32_t *){mem_addr(o[1])}; {o[0]} = ((int32_t)shw[{so >> 2}] == w_ && shw[{so >> 2}] != 0) ? shw[{so >> 2}] : (intptr_t)w_; }}"
                return f"{o[0]} = *(int32_t *){mem_addr(o[1])};"
            if m == "sw":
                so = sp_slot(o[1])
                if so is not None:
                    return f"*(int32_t *){mem_addr(o[1])} = (int32_t){R(o[0])}; shw[{so >> 2}] = {R(o[0])};"
                return f"*(int32_t *){mem_addr(o[1])} = (int32_t){R(o[0])};"
            if m == "lwc1":
                return f"{fp(o[0])} = *(float *){mem_addr(o[1])};"
            if m == "swc1":
                return f"*(float *){mem_addr(o[1])} = {fp(o[0])};"
            if m == "mtc1":
                return f"{fp(o[1])} = float_of((uint32_t){R(o[0])});"
            if m == "mfc1":
                return f"{o[0]} = W(bits_of({fp(o[1])}));"
            if m == "ctc2":
                return f"vi22 = (uint32_t){R(o[0])};"
            if m == "cfc2":
                return f"{o[0]} = W(vi22);"
            if m in ("add.s", "sub.s", "mul.s", "div.s"):
                d, s, t = (fp(x) for x in o)
                op = {"add.s": "+", "sub.s": "-", "mul.s": "*", "div.s": "/"}[m]
                return f"{d} = {s} {op} {t};"
            if m == "neg.s":
                return f"{fp(o[0])} = -{fp(o[1])};"
            if m == "abs.s":
                return f"{fp(o[0])} = float_of(bits_of({fp(o[1])}) & 0x7fffffffu);"
            if m == "mov.s":
                return f"{fp(o[0])} = {fp(o[1])};"
            if m == "sqrt.s":
                return f"{fp(o[0])} = __builtin_sqrtf({fp(o[1])});"
            if m == "adda.s":
                return f"acc = {fp(o[0])} + {fp(o[1])};"
            if m == "suba.s":
                return f"acc = {fp(o[0])} - {fp(o[1])};"
            if m == "mula.s":
                return f"acc = {fp(o[0])} * {fp(o[1])};"
            if m == "madda.s":
                return f"acc = acc + {fp(o[0])} * {fp(o[1])};"
            if m == "msuba.s":
                return f"acc = acc - {fp(o[0])} * {fp(o[1])};"
            if m == "madd.s":
                return f"{fp(o[0])} = acc + {fp(o[1])} * {fp(o[2])};"
            if m == "msub.s":
                return f"{fp(o[0])} = acc - {fp(o[1])} * {fp(o[2])};"
            if m == "trunc.w.s":
                return f"{fp(o[0])} = float_of((uint32_t)ee_cvt_w({fp(o[1])}));"
            if m == "cvt.s.w":
                return f"{fp(o[0])} = (float)(int32_t)bits_of({fp(o[1])});"
            if m == "c.eq.s":
                return f"fcc = {fp(o[0])} == {fp(o[1])};"
            if m == "c.lt.s":
                return f"fcc = {fp(o[0])} < {fp(o[1])};"
            if m == "c.le.s":
                return f"fcc = {fp(o[0])} <= {fp(o[1])};"
            raise SystemExit(f"mips2c: {a:#x}: unsupported instruction {m} {','.join(o)}")

        def cond_expr(m, o, a):
            taint = taint_at(a)
            mm = m[:-1] if m in likely else m
            if mm == "b":
                return "1"
            if mm == "beq":
                return f"(int32_t){R(o[0])} == (int32_t){R(o[1])}" if o[0] not in taint and o[1] not in taint else f"{R(o[0])} == {R(o[1])}"
            if mm == "bne":
                return f"(int32_t){R(o[0])} != (int32_t){R(o[1])}" if o[0] not in taint and o[1] not in taint else f"{R(o[0])} != {R(o[1])}"
            if mm == "beqz":
                return f"(int32_t){R(o[0])} == 0"
            if mm == "bnez":
                return f"(int32_t){R(o[0])} != 0"
            if mm == "bltz":
                return f"(int32_t){R(o[0])} < 0"
            if mm == "bgez":
                return f"(int32_t){R(o[0])} >= 0"
            if mm == "blez":
                return f"(int32_t){R(o[0])} <= 0"
            if mm == "bgtz":
                return f"(int32_t){R(o[0])} > 0"
            if mm == "bc1t":
                return "fcc"
            if mm == "bc1f":
                return "!fcc"
            raise SystemExit(f"mips2c: cannot take the condition of {m}")

        def call_text(t):
            if t not in self.protos:
                raise SystemExit(f"mips2c: call to unknown function {t:#x}: add it to the spec")
            ret, args, name = self.protos[t]
            ai = 0
            fi = 0
            argv = []
            for k in args:
                if k == "f":
                    argv.append(f"f{12 + fi}")
                    fi += 1
                elif k == "p":
                    argv.append(f"(void *){ARG_INT[ai]}")
                    ai += 1
                else:
                    argv.append(f"(int32_t){ARG_INT[ai]}")
                    ai += 1
            call = f"{name}({', '.join(argv)})"
            if ret == "f":
                return f"f0 = {call};"
            if ret == "i":
                return f"v0 = W({call});"
            return f"{call};"

        def ret_text():
            if fn.ret == "f":
                return "return f0;"
            if fn.ret == "i":
                return "return (int32_t)v0;"
            return "return;"

        regs_used_in_cond = lambda m, o: set(x for x in o if x in GPR)

        def writes_of(m, o):
            if not o:
                return set()
            if m in ("sw", "swc1", "sd", "sb", "sh", "ctc2"):
                return set()
            if m in ("lwc1", "mtc1"):
                return set()
            return {o[0]} if o[0] in GPR else set()

        i = 0
        n = len(ins)
        emitted = {}
        text = []
        ind = "    "
        def line(s):
            text.append(ind + s)
        while i < n:
            a, m, o = ins[i]
            if a in targets:
                text.append(f"L_{a:06x}:;")
            if a in delay and a not in targets:
                i += 1
                continue
            if a in delay and a in targets:
                # entered by a jump: run the instruction, then continue at the instruction after the branch's own delay slot position
                s = stmt(a, m, o)
                if s:
                    line(s)
                line(f"goto L_{a + 4:06x};")
                targets.add(a + 4)
                i += 1
                continue
            if m in branches:
                dm, do_ = by_addr[a + 4][0], by_addr[a + 4][1]
                ds = stmt(a + 4, dm, do_)
                if m == "jr":
                    if ds:
                        line(ds)
                    line(ret_text())
                    i += 1
                    continue
                if m == "jal":
                    if ds:
                        line(ds)
                    line(call_text(int(o[0], 16)))
                    i += 1
                    continue
                if m == "j":
                    t = int(o[0], 16)
                    if ds:
                        line(ds)
                    if lo <= t < hi:
                        line(f"goto L_{t:06x};")
                    else:
                        line(call_text(t))
                        line(ret_text())
                    i += 1
                    continue
                t = int(o[-1], 16)
                cond = cond_expr(m, o, a)
                if m in likely:
                    line(f"if ({cond}) {{ {ds} goto L_{t:06x}; }}" if ds else f"if ({cond}) goto L_{t:06x};")
                elif m == "b":
                    if ds:
                        line(ds)
                    line(f"goto L_{t:06x};")
                else:
                    wr = writes_of(dm, do_)
                    if ds and (wr & regs_used_in_cond(m, o)) and cond != "fcc" and cond != "!fcc":
                        line(f"{{ int c_ = {cond};")
                        line(f"  {ds}")
                        line(f"  if (c_) goto L_{t:06x}; }}")
                    else:
                        if ds:
                            line(ds)
                        line(f"if ({cond}) goto L_{t:06x};")
                i += 1
                continue
            s = stmt(a, m, o)
            if s:
                line(s)
            i += 1
        return text, None

    # --- windows ---------------------------------------------------------
    def win_id(self, v):
        for k, (lo_, hi_) in enumerate(self.windows):
            if lo_ <= v < hi_:
                return f"{lo_:06x}"
        raise KeyError(v)

    def win_base(self, v):
        for lo_, hi_ in self.windows:
            if lo_ <= v < hi_:
                return lo_
        raise KeyError(v)


def ctype(k):
    return {"f": "float", "i": "int32_t", "p": "void *"}[k]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--elf", required=True)
    ap.add_argument("--spec", required=True)
    ap.add_argument("--objdump", default="mipsel-linux-gnu-objdump")
    ap.add_argument("-o", dest="out", required=True)
    a = ap.parse_args()
    word = load_elf(a.elf)
    windows = []
    funcs = []
    for line in open(a.spec):
        line = line.split("#")[0].strip()
        if not line:
            continue
        p = line.split()
        if p[0] == "window":
            windows.append((hexint(p[1]), hexint(p[2])))
        elif p[0] == "func":
            funcs.append((hexint(p[1]), hexint(p[2]) if False else None, p))
    L = Lifter(word, windows)
    specs = []
    for line in open(a.spec):
        line = line.split("#")[0].strip()
        p = line.split()
        if p and p[0] == "func":
            addr = hexint(p[1])
            ret, args = p[2], (p[3] if p[3] != "-" else "")
            end = hexint(p[4])
            name = p[5] if len(p) > 5 else f"func_{addr:08x}"
            specs.append((addr, end, ret, args, name))
    for addr, end, ret, args, name in specs:
        L.protos[addr] = (ret, args, name)
    out = [SPDX_HEADER]
    out.append("/* Generated by tools/mips2c.py from the retail libm cores (newlib's float libm, EE-GCC): a literal translation, see that file. Do not edit. */\n")
    out.append('#include <stdint.h>\n#include <string.h>\n\n#include "p4pc/ee_lift.h"\n\n')
    for lo_, hi_ in windows:
        words = ", ".join(f"0x{word(x):08x}u" for x in range(lo_, hi_, 4))
        out.append(f"/* retail data 0x{lo_:08x}..0x{hi_:08x} (libm constants and tables) */\nstatic const uint32_t K_{lo_:06x}[] = {{ {words} }};\n\n")
    for addr, end, ret, args, name in specs:
        params = ", ".join(f"{ctype(k)} arg{i}" for i, k in enumerate(args)) or "void"
        retc = {"f": "float", "i": "int32_t", "v": "void"}[ret]
        out.append(f"{retc} {name}({params});\n")
    out.append("\n")
    for addr, end, ret, args, name in specs:
        ins = disassemble(a.objdump, a.elf, addr, end)
        text, taint = L.lift(Func(addr, ret, args, name), ins)
        params = ", ".join(f"{ctype(k)} arg{i}" for i, k in enumerate(args)) or "void"
        retc = {"f": "float", "i": "int32_t", "v": "void"}[ret]
        # frame size from the prologue
        frame = 0
        for x, m, o in ins:
            if m == "addiu" and o[0] == "sp" and o[1] == "sp" and int(o[2], 0) < 0:
                frame = -int(o[2], 0)
                break
        out.append(f"/* retail 0x{addr:08x} ({end - addr} bytes) */\n{retc} {name}({params})\n{{\n")
        out.append("    intptr_t " + ", ".join(g for g in GPR if g != "zero") + ";\n")
        out.append("    float " + ", ".join(f"f{k}" for k in range(32)) + ";\n")
        out.append("    float acc = 0.0f;\n    int fcc = 0;\n    uint32_t vi22 = 0;\n")
        if frame:
            out.append(f"    uint8_t frame[{frame}] __attribute__((aligned(16)));\n")
            out.append(f"    intptr_t shw[{frame // 4 + 1}] = {{0}};\n")
            out.append("    (void)shw;\n")
        out.append("    (void)acc; (void)fcc; (void)vi22;\n")
        # initial register state
        init = []
        ii = fi = 0
        for k, ak in enumerate(args):
            if ak == "f":
                init.append(f"f{12 + fi} = arg{k};")
                fi += 1
            elif ak == "p":
                init.append(f"{ARG_INT[ii]} = (intptr_t)arg{k};")
                ii += 1
            else:
                init.append(f"{ARG_INT[ii]} = arg{k};")
                ii += 1
        if frame:
            init.append("sp = (intptr_t)frame;")
            init.append("memset(frame, 0, sizeof frame);")
        out.append("    " + "\n    ".join(init) + "\n" if init else "")
        out.append("\n".join(text) + "\n}\n\n")
    open(a.out, "w", newline="\n").write("".join(out))


if __name__ == "__main__":
    main()
