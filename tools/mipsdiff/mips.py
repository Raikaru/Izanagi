"""A small R5900 interpreter for the retail asm of leaf functions, used to check translations.

It reads the decomp's generated `asm/nonmatchings/**/func_XXXXXXXX.s` text (`glabel`, `.L<addr>`
labels, `/* off addr word */ mnemonic operands` lines). It models the integer core (64-bit GPRs
plus the upper 64 bits for lq/sq), the FPU in IEEE single precision (not the EE's truncating FPU:
this checks structure and offsets, not the last bit of rounding) and the EE accumulator ops
(adda.s/mula.s/madd.s/madda.s/msub.s). `jal`/`jalr` go to hooks. Anything else raises.
"""
import re
import struct

M32 = 0xFFFFFFFF
M64 = 0xFFFFFFFFFFFFFFFF
M128 = (1 << 128) - 1


def sx32(v):
    v &= M32
    return v - (1 << 32) if v & 0x80000000 else v


def s64(v):
    v &= M64
    return v - (1 << 64) if v & (1 << 63) else v


def f2b(f):
    """Round a Python float to single precision and return its bit pattern."""
    try:
        return struct.unpack("<I", struct.pack("<f", f))[0]
    except OverflowError:
        return 0x7F800000 if f > 0 else 0xFF800000


def b2f(b):
    return struct.unpack("<f", struct.pack("<I", b & M32))[0]


class Mem:
    """Sparse little-endian memory."""

    def __init__(self, backing=None):
        self.pages = {}
        self.writes = set()
        self.backing = backing      # backing(page_addr) -> 4096 bytes or None, for pages not yet touched

    def _page(self, a):
        p = a >> 12
        pg = self.pages.get(p)
        if pg is None:
            init = self.backing(p << 12) if self.backing else None
            pg = self.pages[p] = bytearray(init) if init else bytearray(4096)
        return pg

    def rb(self, a):
        return self._page(a)[a & 0xFFF]

    def wb(self, a, v):
        self._page(a)[a & 0xFFF] = v & 0xFF
        self.writes.add(a)

    def read(self, a, n):
        return bytes(self.rb(a + i) for i in range(n))

    def write(self, a, data):
        for i, b in enumerate(data):
            self.wb(a + i, b)

    def rn(self, a, n):
        return int.from_bytes(self.read(a, n), "little")

    def wn(self, a, n, v):
        self.write(a, (v & ((1 << (8 * n)) - 1)).to_bytes(n, "little"))


REG_NAMES = {}
for _i in range(32):
    REG_NAMES["$%d" % _i] = _i
for _i, _n in enumerate("zero at v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 s0 s1 s2 s3 s4 s5 s6 s7 t8 t9 k0 k1 gp sp fp ra".split()):
    REG_NAMES["$" + _n] = _i


def reg(tok):
    tok = tok.strip()
    if tok.startswith("$f"):
        return int(tok[2:])
    return REG_NAMES[tok]


LINE = re.compile(r"^\s*/\* \w+ ([0-9A-F]{8}) [0-9A-F]{8} \*/\s*(\S+)\s*(.*)$")
LABEL = re.compile(r"^\s*(\.L[0-9A-F]{8}):")


def parse_asm(text):
    """Returns (insns {addr: (mnemonic, [operands])}, labels {name: addr}, start addr)."""
    insns = {}
    labels = {}
    start = None
    for line in text.splitlines():
        m = LABEL.match(line)
        if m:
            labels[m.group(1)] = int(m.group(1)[2:], 16)
            continue
        m = LINE.match(line)
        if not m:
            continue
        addr = int(m.group(1), 16)
        if start is None:
            start = addr
        ops = m.group(3).strip()
        # strip trailing comments
        ops = ops.split("#")[0].strip()
        ops = re.sub(r"/\*.*?\*/", "", ops).strip()
        insns[addr] = (m.group(2), split_ops(ops))
    return insns, labels, start


def split_ops(s):
    out = []
    depth = 0
    cur = ""
    for ch in s:
        if ch == "(":
            depth += 1
        if ch == ")":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


KNOWN_SYMS = {"sceRead": 0x00427338}   # callees the asm refers to by their library name
SYM = re.compile(r"[A-Za-z_]\w*_([0-9A-Fa-f]{8})")


def sym_addr(tok):
    m = SYM.search(tok)
    return int(m.group(1), 16) if m else None


def eval_imm(tok):
    tok = tok.strip()
    m = re.match(r"%hi\((.*)\)$", tok)
    if m:
        a = sym_addr(m.group(1))
        return ((a + 0x8000) >> 16) & 0xFFFF
    m = re.match(r"%lo\((.*)\)$", tok)
    if m:
        a = sym_addr(m.group(1))
        v = a & 0xFFFF
        return v - 0x10000 if v & 0x8000 else v
    if tok.startswith("(") or re.match(r"^-?0x[0-9a-fA-F]+$|^-?\d+$", tok):
        return int(eval(tok, {"__builtins__": {}}, {}))
    raise ValueError("immediate %r" % tok)


MEMOP = re.compile(r"^(.*)\((\$\w+)\)$")


class Machine:
    def __init__(self, mem, insns, labels, hooks=None):
        self.mem = mem
        self.insns = insns
        self.labels = labels
        self.hooks = hooks or {}
        self.r = [0] * 32          # 128-bit values
        self.f = [0] * 32          # single-precision bit patterns
        self.acc = 0.0
        self.vf = [[0, 0, 0, 0] for _ in range(32)]   # VU0 macro-mode registers as float bit patterns
        self.vf[0][3] = f2b(1.0)
        self.vacc = [0.0, 0.0, 0.0, 0.0]
        self.fcond = False
        self.hi = 0
        self.lo = 0
        self.steps = 0
        self.ra_sentinel = 0xDEAD0000
        self.r[31] = self.ra_sentinel

    # --- register helpers
    def g(self, n):
        return self.r[n] & M64

    def gs(self, n):
        return s64(self.r[n])

    def g32(self, n):
        return self.r[n] & M32

    def setg(self, n, v):
        if n:
            self.r[n] = (self.r[n] & ~M64 & M128) | (v & M64)

    def set32(self, n, v):
        self.setg(n, sx32(v))

    def ea(self, op):
        m = MEMOP.match(op)
        off = eval_imm(m.group(1)) if m.group(1).strip() else 0
        return (self.g32(reg(m.group(2))) + off) & M32

    def target(self, tok):
        tok = tok.strip()
        if tok in self.labels:
            return self.labels[tok]
        m = re.match(r"^\.L([0-9A-F]{8})$", tok)
        if m:
            return int(m.group(1), 16)
        if tok in KNOWN_SYMS or SYM.search(tok):       # a tail jump to another function
            return KNOWN_SYMS[tok] if tok in KNOWN_SYMS else sym_addr(tok)
        raise ValueError("target %r" % tok)

    # --- execution
    def run(self, entry, maxsteps=2000000):
        pc = entry
        while True:
            self.steps += 1
            if self.steps > maxsteps:
                raise RuntimeError("step limit")
            if pc == self.ra_sentinel:
                return
            mn, ops = self.insns[pc]
            nxt = self.exec_one(pc, mn, ops)
            if nxt is None:
                pc += 4
                continue
            kind, tgt, likely_skip = nxt
            # delay slot at pc+4
            if kind == "branch_likely_not_taken":
                pc += 8
                continue
            dmn, dops = self.insns[pc + 4]
            r = self.exec_one(pc + 4, dmn, dops)
            if r is not None:
                raise RuntimeError("branch in delay slot at %08x" % (pc + 4))
            if kind == "jump":
                if tgt not in self.insns and tgt in self.hooks:
                    # tail call into a hook: run it and return to the caller
                    ret = self.hooks[tgt](self)
                    if ret is not None:
                        self.set32(2, ret)
                    raise _Done()
                pc = tgt
            elif kind == "call":
                self.call(tgt, pc)
                pc += 8
            elif kind == "fall":
                pc += 8
            else:
                raise RuntimeError(kind)

    def call(self, tgt, pc):
        if tgt in getattr(self, "selfcall", ()):
            saved = self.r[31]
            self.r[31] = self.ra_sentinel
            try:
                self.run(tgt)
            except _Done:
                pass
            self.r[31] = saved
            return
        h = self.hooks.get(tgt)
        if h is None:
            raise RuntimeError("call to %08x at %08x has no hook" % (tgt, pc))
        ret = h(self)
        if ret is not None:
            self.set32(2, ret)

    def exec_one(self, pc, mn, ops):
        g, gs, set32, setg, mem = self.g, self.gs, self.set32, self.setg, self.mem
        f = self.f
        if mn == ".word":      # raw COP1 words the disassembler could not name
            code = int(ops[0], 0)
            if code & 0xFFE0003F == 0x46000024:     # cvt.w.s fd, fs
                mn, ops = "cvt.w.s", ["$f%d" % ((code >> 6) & 31), "$f%d" % ((code >> 11) & 31)]
            else:
                raise NotImplementedError("%08x .word %s" % (pc, ops[0]))
        if mn in ("nop", "sync", "sync.p", "vnop"):
            return None
        # ---- ALU
        if mn in ("addu", "add"):
            set32(reg(ops[0]), self.g32(reg(ops[1])) + self.g32(reg(ops[2])))
        elif mn == "addi":
            set32(reg(ops[0]), self.g32(reg(ops[1])) + eval_imm(ops[2]))
        elif mn == "lqc2":
            a = self.ea(ops[1]) & ~15
            self.vf[int(ops[0][3:])] = [mem.rn(a + 4 * k, 4) for k in range(4)]
        elif mn == "sqc2":
            a = self.ea(ops[1]) & ~15
            for k in range(4):
                mem.wn(a + 4 * k, 4, self.vf[int(ops[0][3:])][k])
        elif mn.startswith("v") and re.match(r"^v(mul|madd)(a?)([xyzw])\.([xyzw]+)$", mn):
            mm = re.match(r"^v(mul|madd)(a?)([xyzw])\.([xyzw]+)$", mn)
            kind, isa, bcs, mask = mm.group(1), mm.group(2), mm.group(3), mm.group(4)
            # source operands: [$ACC | vfd], vfs, vft + broadcast letter
            s_i = int(ops[1][3:])
            tt = ops[2]
            bc = "xyzw".index(tt[-1])
            t_i = int(tt[3:-1])
            for ch in mask:
                c = "xyzw".index(ch)
                prod = b2f(f2b(b2f(self.vf[s_i][c]) * b2f(self.vf[t_i][bc])))
                val = prod if kind == "mul" else b2f(f2b(self.vacc[c] + prod))
                if isa:
                    self.vacc[c] = val
                else:
                    self.vf[int(ops[0][3:])][c] = f2b(val)
        elif mn == "subu":
            set32(reg(ops[0]), self.g32(reg(ops[1])) - self.g32(reg(ops[2])))
        elif mn == "addiu":
            set32(reg(ops[0]), self.g32(reg(ops[1])) + eval_imm(ops[2]))
        elif mn == "daddu":
            setg(reg(ops[0]), g(reg(ops[1])) + g(reg(ops[2])))
        elif mn == "daddiu":
            setg(reg(ops[0]), g(reg(ops[1])) + eval_imm(ops[2]))
        elif mn == "dsubu":
            setg(reg(ops[0]), g(reg(ops[1])) - g(reg(ops[2])))
        elif mn == "or":
            setg(reg(ops[0]), g(reg(ops[1])) | g(reg(ops[2])))
        elif mn == "and":
            setg(reg(ops[0]), g(reg(ops[1])) & g(reg(ops[2])))
        elif mn == "xor":
            setg(reg(ops[0]), g(reg(ops[1])) ^ g(reg(ops[2])))
        elif mn == "not":
            setg(reg(ops[0]), ~g(reg(ops[1])))
        elif mn == "nor":
            setg(reg(ops[0]), ~(g(reg(ops[1])) | g(reg(ops[2]))))
        elif mn == "ori":
            setg(reg(ops[0]), g(reg(ops[1])) | (eval_imm(ops[2]) & 0xFFFF))
        elif mn == "andi":
            setg(reg(ops[0]), g(reg(ops[1])) & (eval_imm(ops[2]) & 0xFFFF))
        elif mn == "xori":
            setg(reg(ops[0]), g(reg(ops[1])) ^ (eval_imm(ops[2]) & 0xFFFF))
        elif mn == "lui":
            set32(reg(ops[0]), (eval_imm(ops[1]) & 0xFFFF) << 16)
        elif mn == "sll":
            set32(reg(ops[0]), self.g32(reg(ops[1])) << eval_imm(ops[2]))
        elif mn == "srl":
            set32(reg(ops[0]), self.g32(reg(ops[1])) >> eval_imm(ops[2]))
        elif mn == "sra":
            set32(reg(ops[0]), sx32(self.g32(reg(ops[1]))) >> eval_imm(ops[2]))
        elif mn == "sllv":
            set32(reg(ops[0]), self.g32(reg(ops[1])) << (self.g32(reg(ops[2])) & 31))
        elif mn == "srlv":
            set32(reg(ops[0]), self.g32(reg(ops[1])) >> (self.g32(reg(ops[2])) & 31))
        elif mn == "srav":
            set32(reg(ops[0]), sx32(self.g32(reg(ops[1]))) >> (self.g32(reg(ops[2])) & 31))
        elif mn == "dsll":
            setg(reg(ops[0]), g(reg(ops[1])) << eval_imm(ops[2]))
        elif mn == "dsll32":
            setg(reg(ops[0]), g(reg(ops[1])) << (32 + eval_imm(ops[2])))
        elif mn == "dsrl":
            setg(reg(ops[0]), g(reg(ops[1])) >> eval_imm(ops[2]))
        elif mn == "dsrl32":
            setg(reg(ops[0]), g(reg(ops[1])) >> (32 + eval_imm(ops[2])))
        elif mn == "dsra":
            setg(reg(ops[0]), s64(g(reg(ops[1]))) >> eval_imm(ops[2]))
        elif mn == "dsra32":
            setg(reg(ops[0]), s64(g(reg(ops[1]))) >> (32 + eval_imm(ops[2])))
        elif mn == "slt":
            setg(reg(ops[0]), int(gs(reg(ops[1])) < gs(reg(ops[2]))))
        elif mn == "sltu":
            setg(reg(ops[0]), int(g(reg(ops[1])) < g(reg(ops[2]))))
        elif mn == "slti":
            setg(reg(ops[0]), int(gs(reg(ops[1])) < eval_imm(ops[2])))
        elif mn == "sltiu":
            setg(reg(ops[0]), int(g(reg(ops[1])) < (eval_imm(ops[2]) & M64)))
        elif mn == "movn":
            if g(reg(ops[2])) != 0:
                setg(reg(ops[0]), g(reg(ops[1])))
        elif mn == "movz":
            if g(reg(ops[2])) == 0:
                setg(reg(ops[0]), g(reg(ops[1])))
        elif mn in ("mult", "multu", "mult1", "multu1"):
            rd = reg(ops[0]) if len(ops) == 3 else 0
            a, b = ops[-2], ops[-1]
            if mn in ("mult", "mult1"):
                p = sx32(self.g32(reg(a))) * sx32(self.g32(reg(b)))
            else:
                p = self.g32(reg(a)) * self.g32(reg(b))
            self.lo = sx32(p)
            self.hi = sx32(p >> 32)
            if rd:
                set32(rd, self.lo)
        elif mn in ("div", "divu"):
            a, b = ops[-2], ops[-1]
            if mn == "div":
                x, y = sx32(self.g32(reg(a))), sx32(self.g32(reg(b)))
                q = 0 if y == 0 else int(x / y)
                self.lo = sx32(q) if y else (-1 if x >= 0 else 1)
                self.hi = sx32(x - q * y) if y else x
            else:
                x, y = self.g32(reg(a)), self.g32(reg(b))
                self.lo = sx32(x // y) if y else sx32(0xFFFFFFFF)
                self.hi = sx32(x % y) if y else sx32(x)
        elif mn == "mflo":
            setg(reg(ops[0]), self.lo)
        elif mn == "mfhi":
            setg(reg(ops[0]), self.hi)
        elif mn == "paddub":
            a, b = self.r[reg(ops[1])], self.r[reg(ops[2])]
            v = 0
            for i in range(16):
                v |= min(255, ((a >> (8 * i)) & 255) + ((b >> (8 * i)) & 255)) << (8 * i)
            if reg(ops[0]):
                self.r[reg(ops[0])] = v
        elif mn == "pcpyld":
            # rd = { rs[63:0], rt[63:0] }
            d = reg(ops[0])
            if d:
                self.r[d] = (g(reg(ops[1])) << 64) | g(reg(ops[2]))
        # ---- loads and stores
        elif mn == "lw":
            set32(reg(ops[0]), mem.rn(self.ea(ops[1]), 4))
        elif mn in ("lwl", "lwr", "swl", "swr"):
            # unaligned word halves (little endian): the word containing the address is read / written whole
            ea = self.ea(ops[1])
            k = ea & 3
            w = mem.rn(ea & ~3, 4)
            rt = self.g32(reg(ops[0]))
            if mn == "lwl":
                set32(reg(ops[0]), ((w << (8 * (3 - k))) & 0xFFFFFFFF) | (rt & ((1 << (8 * (3 - k))) - 1)))
            elif mn == "lwr":
                set32(reg(ops[0]), (w >> (8 * k)) | (rt & ~(0xFFFFFFFF >> (8 * k)) & 0xFFFFFFFF))
            elif mn == "swl":
                mem.wn(ea & ~3, 4, (w & ~((1 << (8 * (k + 1))) - 1) & 0xFFFFFFFF) | (rt >> (8 * (3 - k))))
            else:
                mem.wn(ea & ~3, 4, (w & ((1 << (8 * k)) - 1)) | ((rt << (8 * k)) & 0xFFFFFFFF))
        elif mn == "lwu":
            setg(reg(ops[0]), mem.rn(self.ea(ops[1]), 4))
        elif mn == "lh":
            v = mem.rn(self.ea(ops[1]), 2)
            setg(reg(ops[0]), v - 0x10000 if v & 0x8000 else v)
        elif mn == "lhu":
            setg(reg(ops[0]), mem.rn(self.ea(ops[1]), 2))
        elif mn == "lb":
            v = mem.rn(self.ea(ops[1]), 1)
            setg(reg(ops[0]), v - 0x100 if v & 0x80 else v)
        elif mn == "lbu":
            setg(reg(ops[0]), mem.rn(self.ea(ops[1]), 1))
        elif mn == "ld":
            setg(reg(ops[0]), mem.rn(self.ea(ops[1]), 8))
        elif mn == "lq":
            if reg(ops[0]):
                self.r[reg(ops[0])] = mem.rn(self.ea(ops[1]) & ~15, 16)
        elif mn == "sw":
            mem.wn(self.ea(ops[1]), 4, self.g32(reg(ops[0])))
        elif mn == "sh":
            mem.wn(self.ea(ops[1]), 2, g(reg(ops[0])))
        elif mn == "sb":
            mem.wn(self.ea(ops[1]), 1, g(reg(ops[0])))
        elif mn == "sd":
            mem.wn(self.ea(ops[1]), 8, g(reg(ops[0])))
        elif mn == "sq":
            mem.wn(self.ea(ops[1]) & ~15, 16, self.r[reg(ops[0])])
        elif mn == "lwc1":
            f[reg(ops[0])] = mem.rn(self.ea(ops[1]), 4)
        elif mn == "swc1":
            mem.wn(self.ea(ops[1]), 4, f[reg(ops[0])])
        # ---- FPU
        elif mn == "mtc1":
            f[reg(ops[1])] = self.g32(reg(ops[0]))
        elif mn == "mfc1":
            set32(reg(ops[0]), f[reg(ops[1])])
        elif mn == "add.s":
            f[reg(ops[0])] = f2b(b2f(f[reg(ops[1])]) + b2f(f[reg(ops[2])]))
        elif mn == "sub.s":
            f[reg(ops[0])] = f2b(b2f(f[reg(ops[1])]) - b2f(f[reg(ops[2])]))
        elif mn == "mul.s":
            f[reg(ops[0])] = f2b(b2f(f[reg(ops[1])]) * b2f(f[reg(ops[2])]))
        elif mn == "div.s":
            a, b = b2f(f[reg(ops[1])]), b2f(f[reg(ops[2])])
            f[reg(ops[0])] = f2b(a / b) if b != 0 else (0x7F7FFFFF if (a >= 0) == (f[reg(ops[2])] >> 31 == 0) else 0xFF7FFFFF)
        elif mn == "c1":
            code = int(ops[0], 16)
            if code & 0x3F == 4:     # sqrt.s fd, ft
                ft, fd = (code >> 16) & 31, (code >> 6) & 31
                v = b2f(f[ft])
                f[fd] = f2b(v ** 0.5) if v >= 0 else 0x7FFFFFFF
            else:
                raise NotImplementedError("c1 %s" % ops[0])
        elif mn == "neg.s":
            f[reg(ops[0])] = f[reg(ops[1])] ^ 0x80000000
        elif mn == "c1":
            code = int(ops[0], 0)
            fn = code & 0x3F
            ft = (code >> 16) & 31
            fd = (code >> 6) & 31
            if fn == 4:   # sqrt.s fd, ft
                f[fd] = f2b(abs(b2f(f[ft])) ** 0.5)
            else:
                raise NotImplementedError("c1 %s" % ops[0])
        elif mn == "abs.s":
            f[reg(ops[0])] = f[reg(ops[1])] & 0x7FFFFFFF
        elif mn == "mov.s":
            f[reg(ops[0])] = f[reg(ops[1])]
        elif mn == "sqrt.s":
            f[reg(ops[0])] = f2b(abs(b2f(f[reg(ops[1])])) ** 0.5)
        elif mn == "cvt.s.w":
            f[reg(ops[0])] = f2b(float(sx32(f[reg(ops[1])])))
        elif mn == "cvt.w.s":
            v = b2f(f[reg(ops[1])])
            f[reg(ops[0])] = int(max(min(v, 2147483647.0), -2147483648.0)) & M32
        elif mn == "adda.s":
            self.acc = b2f(f2b(b2f(f[reg(ops[0])]) + b2f(f[reg(ops[1])])))
        elif mn == "suba.s":
            self.acc = b2f(f2b(b2f(f[reg(ops[0])]) - b2f(f[reg(ops[1])])))
        elif mn == "mula.s":
            self.acc = b2f(f2b(b2f(f[reg(ops[0])]) * b2f(f[reg(ops[1])])))
        elif mn == "madda.s":
            p = b2f(f2b(b2f(f[reg(ops[0])]) * b2f(f[reg(ops[1])])))
            self.acc = b2f(f2b(self.acc + p))
        elif mn == "msuba.s":
            p = b2f(f2b(b2f(f[reg(ops[0])]) * b2f(f[reg(ops[1])])))
            self.acc = b2f(f2b(self.acc - p))
        elif mn == "madd.s":
            p = b2f(f2b(b2f(f[reg(ops[1])]) * b2f(f[reg(ops[2])])))
            self.acc = b2f(f2b(self.acc + p))
            f[reg(ops[0])] = f2b(self.acc)
        elif mn == "msub.s":
            p = b2f(f2b(b2f(f[reg(ops[1])]) * b2f(f[reg(ops[2])])))
            self.acc = b2f(f2b(self.acc - p))
            f[reg(ops[0])] = f2b(self.acc)
        elif mn == "c.lt.s":
            self.fcond = b2f(f[reg(ops[0])]) < b2f(f[reg(ops[1])])
        elif mn == "c.le.s":
            self.fcond = b2f(f[reg(ops[0])]) <= b2f(f[reg(ops[1])])
        elif mn == "c.eq.s":
            self.fcond = b2f(f[reg(ops[0])]) == b2f(f[reg(ops[1])])
        # ---- control flow
        elif mn in ("beq", "bne", "beqz", "bnez", "blez", "bgtz", "bgez", "bltz", "b",
                    "bc1t", "bc1f"):
            a = gs(reg(ops[0])) if mn not in ("b", "bc1t", "bc1f") else 0
            if mn == "beq":
                taken = gs(reg(ops[0])) == gs(reg(ops[1]))
                tgt = self.target(ops[2])
            elif mn == "bne":
                taken = gs(reg(ops[0])) != gs(reg(ops[1]))
                tgt = self.target(ops[2])
            elif mn == "b":
                taken = True
                tgt = self.target(ops[0])
            elif mn == "bc1t":
                taken = self.fcond
                tgt = self.target(ops[0])
            elif mn == "bc1f":
                taken = not self.fcond
                tgt = self.target(ops[0])
            else:
                taken = {"beqz": a == 0, "bnez": a != 0, "blez": a <= 0, "bgtz": a > 0,
                         "bgez": a >= 0, "bltz": a < 0}[mn]
                tgt = self.target(ops[1])
            return ("jump" if taken else "fall", tgt, False)
        elif mn in ("beql", "bnel", "beqzl", "bnezl", "blezl", "bgtzl", "bgezl", "bltzl", "bc1tl", "bc1fl"):
            a = gs(reg(ops[0])) if mn not in ("bc1tl", "bc1fl") else 0
            if mn == "beql":
                taken = gs(reg(ops[0])) == gs(reg(ops[1]))
                tgt = self.target(ops[2])
            elif mn == "bnel":
                taken = gs(reg(ops[0])) != gs(reg(ops[1]))
                tgt = self.target(ops[2])
            elif mn == "bc1tl":
                taken = self.fcond
                tgt = self.target(ops[0])
            elif mn == "bc1fl":
                taken = not self.fcond
                tgt = self.target(ops[0])
            else:
                taken = {"beqzl": a == 0, "bnezl": a != 0, "blezl": a <= 0, "bgtzl": a > 0,
                         "bgezl": a >= 0, "bltzl": a < 0}[mn]
                tgt = self.target(ops[1])
            return ("jump" if taken else "branch_likely_not_taken", tgt, False)
        elif mn == "j":
            return ("jump", self.target(ops[0]), False)
        elif mn == "jr":
            if reg(ops[0]) == 31 and self.g32(31) == self.ra_sentinel:
                # return: execute the delay slot, then stop
                dmn, dops = self.insns[pc + 4]
                self.exec_one(pc + 4, dmn, dops)
                self.steps += 1
                raise _Done()
            return ("jump", self.g32(reg(ops[0])), False)
        elif mn == "jal":
            tgt = KNOWN_SYMS[ops[0]] if ops[0] in KNOWN_SYMS else sym_addr(ops[0]) if SYM.search(ops[0]) else int(ops[0], 0)
            self.set32(31, self.ra_sentinel + 1)
            return ("call", tgt, False)
        elif mn == "jalr":
            tgt = self.g32(reg(ops[-1] if len(ops) == 1 else ops[1]))
            return ("call", tgt, False)
        else:
            raise NotImplementedError("%08x %s %s" % (pc, mn, ", ".join(ops)))
        return None

    def execute(self, entry):
        try:
            self.run(entry)
        except _Done:
            return


class _Done(Exception):
    pass
