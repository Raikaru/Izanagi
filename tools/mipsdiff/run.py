"""Differential check of a translated function against the retail asm.

For each function a scenario builds a random memory image and arguments. The retail asm runs
in mips.py; the override's C (compiled -m32, the same way the port compiles it) runs in
harness.c under WSL on the identical image. Return value (when the function has one), the
hook log and every touched page must match.

Run with a Windows Python that can start `wsl -d Debian` (the harness is compiled and run there).
Environment: P4_ASM_ROOT (the decomp's asm/nonmatchings tree, default decomp/asm/nonmatchings) and P4_RETAIL
(your SLUS_217.82, default decomp/orig/SLUS_217.82).
"""
import glob
import os
import random
import re
import struct
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mips  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ASM_ROOT = os.environ.get("P4_ASM_ROOT", os.path.join(ROOT, "decomp", "asm", "nonmatchings"))

ARENA = 0x30000000
# Pure float callees (sinf/cosf stand-ins): the C stub returns x*m + b, the interpreter hook the
# same in single precision ($f12 in, $f0 out). They are not logged.
FLOAT_STUBS = {0x44B7B0: (0.5, 1.0), 0x44B610: (0.25, -1.0)}

# Callees whose second argument is a pointer to a word; the stub and the interpreter log the word.
DEREF = {}

FRET = set()            # function addresses that return a float: the asm side's result is the bits of $f0 (the C wrapper returns the bits)
ALT_NAMES = {}          # callee address -> a second symbol name the C side calls it by (cases.py fills it)

HOOK_BASE = 0x00500000
POOL_BASE = 0x38000000      # bump-allocator region the alloc hooks hand out (hooks 2 and 3)          # fake retail addresses for indirect callees seen by the interpreter

_asm_index = None
RETAIL = os.environ.get("P4_RETAIL", os.path.join(ROOT, "decomp", "orig", "SLUS_217.82"))
_elf = None


def retail_page(addr):
    """Initial bytes of a retail page (code and data of the ELF image), for the interpreter only."""
    global _elf
    if _elf is None:
        data = open(RETAIL, "rb").read()
        phoff, = struct.unpack_from("<I", data, 0x1C)
        phentsize, phnum = struct.unpack_from("<HH", data, 0x2A)
        segs = []
        for i in range(phnum):
            t, off, va, pa, fsz, msz, fl, al = struct.unpack_from("<8I", data, phoff + i * phentsize)
            if t == 1:
                segs.append((off, va, fsz, msz))
        _elf = (data, segs)
    data, segs = _elf
    if addr < 0x1000:
        # PS2 low RAM, which retail code reads through null pointers: the image of include/p4pc/ps2_low_ram.h (interpreter only; the C side uses the same words from that header)
        words = [int(w, 16) for w in re.findall(r"0x[0-9a-f]{8}", open(os.path.join(ROOT, "include", "p4pc", "ps2_low_ram.h")).read().split("p4_ps2_low_ram_words[64]")[1].split("};")[0])]
        return struct.pack("<64I", *words).ljust(4096, b"\0")
    out = bytearray(4096)
    hit = False
    for off, va, fsz, msz in segs:
        lo = max(addr, va)
        hi = min(addr + 4096, va + fsz)
        if lo < hi:
            out[lo - addr:hi - addr] = data[off + lo - va:off + hi - va]
            hit = True
    return bytes(out) if hit else None


def find_asm(addr):
    global _asm_index
    if _asm_index is None:
        _asm_index = {}
        for f in glob.glob(ASM_ROOT + "/*/*.s"):
            with open(f, encoding="utf8", errors="replace") as fh:
                t = fh.read()
            m = re.search(r"/\* \w+ ([0-9A-F]{8}) ", t)
            if m:
                _asm_index[int(m.group(1), 16)] = f
    if addr not in _asm_index:
        return _flat_asm(addr)
    return open(_asm_index[addr], encoding="utf8").read()


_flat = None
_func_starts = None


def _flat_asm(addr):
    """The asm of the function at addr in the Sony asm syntax mips.parse_asm reads, built from build/all.s (the whole retail text, `python tools/audit_live_args.py --make-asm`)
    when the decomp's per-function asm tree is not there. Function ends: the next symbol of decomp/config/symbol_addrs.txt."""
    global _flat, _func_starts
    path = os.environ.get("P4_ALL_ASM", os.path.join(ROOT, "build", "all.s"))
    if _flat is None:
        pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \t(\S+)\s*(.*)$")
        _flat = {}
        if os.path.isfile(path):
            for line in open(path, errors="replace"):
                m = pat.match(line)
                if m:
                    _flat[int(m.group(1), 16)] = (m.group(2), m.group(3).split("<")[0].strip())
        starts = []
        sp = os.path.join(ROOT, "decomp", "config", "symbol_addrs.txt")
        for line in open(sp, errors="replace"):
            m = re.match(r"\s*\w+\s*=\s*0x([0-9A-Fa-f]{8});.*type:func", line)
            if m:
                starts.append(int(m.group(1), 16))
        _func_starts = sorted(set(starts))
    if addr not in _flat:
        raise KeyError(addr)
    import bisect
    i = bisect.bisect_right(_func_starts, addr)
    end = _func_starts[i] if i < len(_func_starts) else addr + 0x400
    rows = [(a, _flat.get(a, ("nop", ""))) for a in range(addr, end, 4)]      # objdump folds runs of zero words into `...`
    targets = set()
    for a, (mn, ops) in rows:
        m = re.search(r"0x([0-9a-f]+)$", ops)
        if m and (mn.startswith("b") or mn in ("j",)):
            tg = int(m.group(1), 16)
            if addr <= tg < end:
                targets.add(tg)
    out = ["glabel func_%08x" % addr]
    for a, (mn, ops) in rows:
        if a in targets:
            out.append(".L%08X:" % a)
        if mn == "move":
            d, s = [x.strip() for x in ops.split(",")]
            mn, ops = "daddu", "%s, %s, $0" % (d, s)
        elif mn == "li":
            d, v = [x.strip() for x in ops.split(",")]
            iv = int(v, 0)
            mn, ops = ("addiu", "%s, $0, %s" % (d, v)) if -0x8000 <= iv < 0x8000 else ("ori", "%s, $0, %s" % (d, v))
        elif mn == "negu":
            d, s = [x.strip() for x in ops.split(",")]
            mn, ops = "subu", "%s, $0, %s" % (d, s)
        else:
            m = re.search(r"0x([0-9a-f]+)$", ops)
            if m and mn in ("jal", "j"):
                tg = int(m.group(1), 16)
                ops = ops[:m.start()] + (".L%08X" % tg if (mn == "j" and addr <= tg < end) else "func_%08x" % tg)
            elif m and mn.startswith("b"):
                ops = ops[:m.start()] + ".L%08X" % int(m.group(1), 16)
            ops = ", ".join(x.strip() for x in ops.split(",")) if ops else ""
        out.append("/* 0 %08X 00000000 */  %s %s" % (a, mn, ops))
    return "\n".join(out) + "\n"


def wsl_path(p):
    p = os.path.abspath(p).replace("\\", "/")
    m = re.match(r"^([A-Za-z]):(.*)$", p)
    return "/mnt/%s%s" % (m.group(1).lower(), m.group(2)) if m else p


class Img:
    """Memory image under construction: a bump arena plus explicit retail-address pages."""

    def __init__(self, rng):
        self.mem = mips.Mem()
        self.rng = rng
        self.top = ARENA
        self.fixups = []        # (slot address, hook index)
        self.callee_ret = {}    # callee address -> value its stub returns

    def alloc(self, size, fill_random=True, align=16):
        self.top = (self.top + align - 1) & ~(align - 1)
        a = self.top
        self.top += (size + 15) & ~15
        for pg in range(a >> 12, ((a + size - 1) >> 12) + 1):
            self.mem._page(pg << 12)
        if fill_random:
            for i in range(size):
                self.mem.wb(a + i, self.rng.randrange(256))
        return a

    def touch(self, addr):
        self.mem._page(addr)

    def w32(self, a, v):
        self.mem.wn(a, 4, v & 0xFFFFFFFF)

    def r32(self, a):
        return self.mem.rn(a, 4)

    def wf(self, a, f):
        self.mem.wn(a, 4, mips.f2b(f))

    def rf(self, a):
        return mips.b2f(self.mem.rn(a, 4))

    def rand_f(self, lo=-100.0, hi=100.0):
        return mips.b2f(mips.f2b(self.rng.uniform(lo, hi)))

    def fill_f(self, a, n, lo=-100.0, hi=100.0):
        for i in range(n):
            self.wf(a + 4 * i, self.rand_f(lo, hi))

    def pool(self, pages=16):
        """Map the allocator region used by hooks 2 (alloc), 3 (freelist alloc) and 4 (free)."""
        for i in range(pages):
            self.mem._page(POOL_BASE + 0x1000 * i)

    def fnslot(self, slot, addr):
        """A word holding the address of the callee `addr`: the interpreter sees the retail address, the C harness the stub's."""
        self.touch(slot)
        self.fixups.append((slot, 0x10000 + addr))
        self.w32(slot, addr)

    def hook(self, slot, index):
        """Indirect callee: the interpreter sees HOOK_BASE+index, the C harness a native stub."""
        self.touch(slot)
        self.fixups.append((slot, index))
        self.w32(slot, HOOK_BASE + index)


def build(entries, callees=(), extra=(), c_impls=None):
    """entries: list of (symbol, override file stem); callees: PS2 addresses of functions the
    overrides call (replaced by logging stubs, id = index). Returns the path of the harness binary."""
    d = tempfile.mkdtemp(prefix="mipsdiff_")
    table = ["#include <stddef.h>", "typedef struct { const char *name; void *fn; } Entry;"]
    for sym, _ in entries:
        table.append("extern void %s(void);" % sym)
    table.append("const Entry p4_table[] = {")
    for sym, _ in entries:
        table.append('    { "%s", (void *)%s },' % (sym, sym))
    table.append("};")
    table.append("const int p4_table_len = %d;" % len(entries))
    table.append("#include <string.h>")
    table.append("void *func_0043f810(void *d, const void *s, unsigned n) { return memcpy(d, s, n); }")
    table.append("extern unsigned p4_record(unsigned, unsigned, unsigned, unsigned, unsigned);")
    table.append("extern unsigned p4_ncalls[], p4_case_id, p4_ret[];")
    c_impls = c_impls or {}
    for i, a in enumerate(callees):
        if a in ALT_NAMES:
            table.append("unsigned func_%08x(unsigned, unsigned, unsigned, unsigned); unsigned %s() __attribute__((alias(\"func_%08x\")));" % (a, ALT_NAMES[a], a))
        if a in c_impls:
            if c_impls[a] == "":
                # a function the C side runs for real (libc or the file itself): only the interpreter hooks it; a weak trap keeps the link whole
                table.append("__attribute__((weak)) void func_%08x(void) { __builtin_trap(); }" % a)
            table.append(c_impls[a].replace("{ID}", str(i)))
            continue
        if a in FLOAT_STUBS:
            m_, b_ = FLOAT_STUBS[a]
            table.append("float func_%08x(float x) { return x * %rf + %rf; }" % (a, m_, b_))
            continue
        table.append("unsigned func_%08x(unsigned a, unsigned b, unsigned c, unsigned d) { return p4_record(%d, a, b, c, d); }" % (a, i))
    fn_rows = []
    for a in callees:
        if a in c_impls and ("func_%08x" % a) not in c_impls[a]:
            continue
        fn_rows.append("    { 0x%xu, (void *)func_%08x }," % (a, a))
    table.append("typedef struct { unsigned a; void *f; } FnEntry;")
    table.append("const FnEntry p4_fn_table[] = {\n" + "\n".join(fn_rows) + "\n    { 0, 0 } };")
    # a port source (src/...) names many retail functions besides the ones under test: give every other one a weak trapping stub so it links
    import re as _re
    stubbed = set("func_%08x" % a for a in callees) | set(ALT_NAMES.get(a, "") for a in callees) | {"func_0043f810"} | ({"sceRead"} if 0x427338 in callees else set())
    for _, stem in list(entries) + [(None, s) for s in extra]:
        if stem.startswith(".."):
            txt = open(os.path.join(ROOT, "overrides", stem + ".c"), encoding="utf8", errors="ignore").read()
            defined = set(_re.findall(r"^[A-Za-z_][\w \t\*]*[ \t\*](func_[0-9a-f]{8})\s*\([^;{]*\)\s*\{", txt, _re.M))      # the file's own definitions need no stub
            for nm in sorted(set(_re.findall(r"\b(func_[0-9a-f]{8}|sceRead|dvCi\w+|mfCi\w+|htCi\w+)\b", txt)) - stubbed - defined):
                table.append("__attribute__((weak)) void %s(void) { __builtin_trap(); }" % nm)
                stubbed.add(nm)
    with open(os.path.join(d, "table.c"), "w") as f:
        f.write("\n".join(table) + "\n")
    srcs = [os.path.join(d, "table.c"), os.path.join(HERE, "harness.c")]
    for _, stem in list(entries) + [(None, s) for s in extra]:
        srcs.append(os.path.join(ROOT, "overrides", stem + ".c"))
    sh = ["cd \"%s\"" % wsl_path(ROOT), "set -e", "objs=''", "i=0"]
    for s in srcs:
        is_override = os.path.normpath(s).startswith(os.path.normpath(os.path.join(ROOT, "overrides")))
        # a port source (src/...) holds many functions: it is built interposable so a call to another function of the same file reaches the logging stub
        inc = "-Idecomp/include -Iinclude -include type.h " if is_override else "-fPIC -fsemantic-interposition " if "/src/" in s.replace("\\", "/") or ".." in s else ""
        sh.append("clang --target=i386-linux-gnu -m32 -O1 -DP4_PC=1 -mfpmath=sse -msse2 -ffp-contract=off "
                  "-std=gnu89 -Wno-int-conversion -Wno-incompatible-function-pointer-types -w %s-c \"%s\" -o \"%s/o$i.o\""
                  % (inc, wsl_path(s), wsl_path(d)))
        sh.append("objs=\"$objs '%s/o$i.o'\"; i=$((i+1))" % wsl_path(d))
    sh.append("eval clang --target=i386-linux-gnu -m32 $objs -Wl,--allow-multiple-definition -o \"%s/harness\" -lm" % wsl_path(d))
    script = os.path.join(d, "build.sh")
    with open(script, "w", newline="\n") as f:
        f.write("\n".join(sh) + "\n")
    r = subprocess.run(["wsl", "-d", "Debian", "--", "sh", wsl_path(script)], capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError("build failed:\n" + r.stdout + r.stderr)
    return os.path.join(d, "harness")


class Case:
    def __init__(self, args, img, ret=True):
        self.args = list(args) + [0] * (9 - len(args))
        self.img = img
        self.ret = ret
        self.cargs = None       # argument words for the C side when its parameter order differs (a float declared before pointers)


def run_cases(harness, index, cases, callees=()):
    """Runs one function over `cases`; returns a list of (ok, message)."""
    d = os.path.dirname(harness)
    # --- C side
    blob = bytearray(struct.pack("<I", len(cases)))
    befores = []
    for c in cases:
        pages = sorted(c.img.mem.pages)
        befores.append({p: bytes(c.img.mem.pages[p]) for p in pages})
        blob += struct.pack("<I", index) + struct.pack("<9I", *[a & 0xFFFFFFFF for a in ((c.cargs or c.args) + [0] * 9)[:9]])
        blob += struct.pack("<I", len(pages))
        for p in pages:
            blob += struct.pack("<I", p << 12) + befores[-1][p]
        blob += struct.pack("<I", len(c.img.fixups))
        for slot, h in c.img.fixups:
            blob += struct.pack("<II", slot, h)
        rets = [c.img.callee_ret.get(a, 0) for a in callees] + [0] * (128 - len(callees)) + [c.img.callee_ret.get(HOOK_BASE + 0, 0), c.img.callee_ret.get(HOOK_BASE + 1, 0), 0, 0, 0, 0, 0, c.img.callee_ret.get(HOOK_BASE + 7, 0), 0, 0, c.img.callee_ret.get(HOOK_BASE + 10, 0)]
        blob += struct.pack("<I", len(rets)) + struct.pack("<%dI" % len(rets), *rets)
        drf = [DEREF.get(a, 0) for a in callees] + [0] * (130 - len(callees))
        blob += struct.pack("<I", len(drf)) + struct.pack("<%dI" % len(drf), *drf)
    inp = os.path.join(d, "cases.bin")
    outp = os.path.join(d, "result.bin")
    with open(inp, "wb") as f:
        f.write(blob)
    r = subprocess.run(["wsl", "-d", "Debian", "--", wsl_path(harness), wsl_path(inp), wsl_path(outp)],
                       capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError("harness failed rc=%d\n%s%s" % (r.returncode, r.stdout, r.stderr))
    res = open(outp, "rb").read()
    return res, befores


_ARITY = {}


def live_arity(addr, fn=None):
    """Integer argument registers the retail function at addr reads (tools/audit_live_args.py), None when it has no asm. With fn (the function under test) only the ones
    fn sets or leaves incoming at its call to addr. An env P4_NO_LIVE_ARITY=1 turns it off."""
    if os.environ.get("P4_NO_LIVE_ARITY"):
        return None
    if (addr, fn) not in _ARITY:
        sys.path.insert(0, os.path.join(HERE, ".."))
        import audit_live_args
        _ARITY[(addr, fn)] = audit_live_args.callee_int_arity(addr) if fn is None else audit_live_args.site_int_arity(fn, addr)
    return _ARITY[(addr, fn)]


def check(addr, sym, cases, harness, index, callees=(), callee_nargs=None, hook_args=(True, True), skip_slots=(), impls=None, fuzzy=False, entry=None):
    """Interpreter vs C for every case. Returns the list of failure strings."""
    text = find_asm(addr)
    insns, labels, start = mips.parse_asm(text)
    if entry:
        start = entry
    res, befores = run_cases(harness, index, cases, callees)
    fails = []
    pos = 0
    for n, c in enumerate(cases):
        pages = sorted(befores[n])
        c_ret, = struct.unpack_from("<I", res, pos)
        pos += 4
        logn, = struct.unpack_from("<I", res, pos)
        pos += 4
        clog = list(struct.unpack_from("<1280I", res, pos))[:logn]
        pos += 5120
        cpages = {}
        for p in pages:
            cpages[p] = res[pos:pos + 4096]
            pos += 4096
        # interpreter side
        m = mips.Mem(retail_page)
        for p in pages:
            m.pages[p] = bytearray(befores[n][p])
        ilog = []
        nargs = callee_nargs or {}

        ncalls = {}

        def mk(cid, ret):
            def hook(mach, cid=cid, ret=ret):
                n = ncalls.get(cid, 0)
                ncalls[cid] = n + 1
                av = [mach.g32(4), mach.g32(5), mach.g32(6), mach.g32(7)]
                dk = DEREF.get(callees[cid]) if cid < len(callees) else 0
                if dk == 9 and av[1]:
                    w = [mach.mem.rn(av[1] + 4 * i, 4) for i in range(5)]
                    av[1] = (w[0] + 3 * w[1] + 5 * w[2] + 7 * w[3] + 11 * w[4]) & 0xFFFFFFFF
                elif dk and av[dk]:
                    av[dk] = mach.mem.rn(av[dk], 4)
                ilog.extend([cid] + av)
                if (ret >> 24) == 0xFD:
                    return (ret >> n) & 1
                if (ret >> 24) == 0xFE:
                    return 1 if n < (ret & 0xFFFF) else 0
                return ret
            return hook
        hooks = {HOOK_BASE + 0: mk(128, c.img.callee_ret.get(HOOK_BASE + 0, 0)),
                 HOOK_BASE + 1: mk(129, c.img.callee_ret.get(HOOK_BASE + 1, 0))}
        bump = [POOL_BASE]

        def alloc_hook(mach):
            r = bump[0]
            ilog.extend([130, mach.g32(4), mach.g32(5), mach.g32(6), mach.g32(7)])
            bump[0] += (mach.g32(4) + 15) & ~15
            return r

        def pool_hook(mach):
            r = bump[0]
            ilog.extend([131, mach.g32(4), mach.g32(5), mach.g32(6), mach.g32(7)])
            bump[0] += 0x100
            return r
        hooks[HOOK_BASE + 2] = alloc_hook
        hooks[HOOK_BASE + 3] = pool_hook
        hooks[HOOK_BASE + 4] = mk(132, 0)

        def writer_hook(mach):
            ilog.extend([133, mach.g32(4), mach.g32(5), mach.g32(6), mach.g32(7)])
            a = mach.g32(4)
            d = mach.g32(7)
            if a:
                for k, sh in ((0x10, 0), (0x18, 1), (0x20, 2), (0x28, 3)):
                    mach.mem.wn(a + k, 4, (d >> sh) & 1)
            return 0
        hooks[HOOK_BASE + 5] = writer_hook

        def strlen_hook(mach):
            ilog.extend([134, mach.g32(4), 0, 0, 0])
            p = mach.g32(4)
            n = 0
            while mach.mem.rb(p + n):
                n += 1
            return n
        hooks[HOOK_BASE + 6] = strlen_hook

        def hit_cb_hook(mach):
            n = ncalls.get(135, 0)
            ncalls[135] = n + 1
            hp = mach.g32(5)
            h = 0
            for i in range(12):
                wv = mach.mem.rn(hp + 4 * i, 4)
                if i != 10 and not (7 <= i <= 9 and wv >= 0x80000000):     # pointers into a stack frame differ between the sides
                    h = (h * 31 + wv) & 0xFFFFFFFF
            ilog.extend([135, mach.g32(4), h, mach.g32(6), mach.f[12]])
            ret = c.img.callee_ret.get(HOOK_BASE + 7, 0)
            if (ret >> 24) == 0xFD:
                return (ret >> n) & 1
            if (ret >> 24) == 0xFE:
                return 1 if n < (ret & 0xFFFF) else 0
            return ret
        hooks[HOOK_BASE + 7] = hit_cb_hook

        def ray_cb_hook(mach):
            n = ncalls.get(138, 0)
            ncalls[138] = n + 1
            hp = mach.g32(6)
            h = 0
            for i in range(12):
                wv = mach.mem.rn(hp + 4 * i, 4)
                if i != 10 and not (7 <= i <= 9 and wv >= 0x80000000):
                    h = (h * 31 + wv) & 0xFFFFFFFF
            ilog.extend([138, (mach.g32(4) + 31 * mach.g32(5)) & 0xFFFFFFFF, h, mach.g32(7), mach.f[12]])
            ret = c.img.callee_ret.get(HOOK_BASE + 10, 0)
            if (ret >> 24) == 0xFD:
                return (ret >> n) & 1
            if (ret >> 24) == 0xFE:
                return 1 if n < (ret & 0xFFFF) else 0
            return ret
        hooks[HOOK_BASE + 10] = ray_cb_hook

        def adv_hook(mach):
            n = ncalls.get(139, 0)
            ncalls[139] = n + 1
            a = mach.g32(4)
            ilog.extend([139, a, mach.mem.rn(mach.mem.rn(a + 4, 4) + 0x18, 4), 0, 0])
            return 0
        hooks[HOOK_BASE + 11] = adv_hook

        def key_hook(mach):
            o, k = mach.g32(4), mach.g32(5)
            h = 0
            for i in range(12):
                h = (h * 31 + mach.mem.rn(k + 4 * i, 4)) & 0xFFFFFFFF
            ilog.extend([136, 0, h, 0, 0])
            for i in range(16):
                mach.mem.wn(o + 4 * i, 4, (mach.mem.rn(k + 4 * (i % 12), 4) + i * 7) & 0xFFFFFFFF)
            return 0
        hooks[HOOK_BASE + 8] = key_hook

        def getreg_hook(mach):
            n = ncalls.get(137, 0)
            ncalls[137] = n + 1
            reg, outp = mach.g32(4), mach.g32(5)
            ilog.extend([137, reg, 0, 0, 0])
            mach.mem.wn(outp, 4, (reg * 0x101 + n) & 0xFFFFFFFF)
            return 0
        hooks[HOOK_BASE + 9] = getreg_hook
        for ci, ca in enumerate(callees):
            hooks[ca] = mk(ci, c.img.callee_ret.get(ca, 0))
            if ca in FLOAT_STUBS:
                def fhook(mach, ca=ca):
                    mu, ad = FLOAT_STUBS[ca]
                    x = mips.b2f(mach.f[12])
                    t = mips.b2f(mips.f2b(x * mips.b2f(mips.f2b(mu))))
                    mach.f[0] = mips.f2b(t + mips.b2f(mips.f2b(ad)))
                hooks[ca] = fhook
        for ca, fn in (impls or {}).items():
            hooks[ca] = fn
        mach = mips.Machine(m, insns, labels, hooks)
        mach.selfcall = {start}
        mach.ilog = ilog
        mach.ncalls = ncalls
        mach.cret = c.img.callee_ret
        mach.callees = list(callees)
        mach.set32(28, 0x007690F0)   # gp
        for i, a in enumerate(c.args[:8]):
            mach.set32(4 + i, a)
        if c.args[8]:
            mach.mem.wn(0, 4, c.args[8])     # ninth argument: the caller's stack word at sp + 0 (sp starts at 0)
        for fr, bits in getattr(c.img, "fregs", {}).items():
            mach.f[fr] = bits
        try:
            mach.execute(start)
        except Exception as e:  # noqa: BLE001
            fails.append("case %d: interpreter: %r" % (n, e))
            continue
        ok = True
        msg = []
        asm_ret = mach.f[0] if addr in FRET else mach.g32(2)
        if c.ret and asm_ret != c_ret:
            ok = False
            msg.append("ret asm=%08x c=%08x" % (asm_ret, c_ret))
        def mask(lg):
            out = []
            for k in range(0, len(lg), 5):
                cid = lg[k]
                if cid < len(callees):
                    n = nargs.get(callees[cid], 4)
                    ar = live_arity(callees[cid], addr)       # a stub always compares every argument register the retail callee reads
                    if isinstance(n, tuple):
                        out.append(cid)
                        out.extend([lg[k + 1 + i] if n[i] else 0 for i in range(4)])
                    else:
                        n0 = n
                        if ar is not None:
                            n = max(n, ar)
                        # the arguments added by the arity: a stack address (a local's pointer, different in the interpreter and in C) is not compared
                        extra = [(v if v < 0xFF000000 else 0) for v in lg[k + 1 + n0:k + 1 + n]]
                        out.extend(lg[k:k + 1 + n0] + extra + [0] * (4 - n))
                else:
                    ha = hook_args.get(cid, hook_args.get(None, (True, True))) if isinstance(hook_args, dict) else hook_args
                    if cid in (135, 138) and not (isinstance(hook_args, dict) and cid in hook_args):
                        ha = (True, True, True, True)        # the hit callbacks: compare the context and the float as well
                    out.append(cid)
                    out.extend([lg[k + 1 + i] if i < len(ha) and ha[i] else 0 for i in range(4)])
            return out
        ilog = mask(ilog)
        clog = mask(clog)
        if ilog != clog:
            ok = False
            msg.append("hook log asm=%s c=%s" % ([hex(x) for x in ilog], [hex(x) for x in clog]))
        skip = set(skip_slots) | {s for s, _ in c.img.fixups}
        for p in pages:
            a = bytes(m.pages[p])
            b = bytes(cpages[p])
            if a != b:
                diffs = [i for i in range(4096) if a[i] != b[i]
                         and ((p << 12) + (i & ~3)) not in skip]
                # a word that holds the retail address of a callee (a stored function pointer) is a native address on the C side
                cset = set(callees)
                diffs = [i for i in diffs if struct.unpack("<I", a[i & ~3:(i & ~3) + 4])[0] not in cset]
                if fuzzy and diffs:
                    # float words equal within a relative tolerance (the override uses host libm where retail inlines polynomials)
                    keep = []
                    for i in diffs:
                        w = i & ~3
                        x = struct.unpack("<f", a[w:w + 4])[0]
                        y = struct.unpack("<f", b[w:w + 4])[0]
                        if not (x == x and y == y and abs(x - y) <= 3e-5 * max(1.0, abs(x), abs(y))):
                            keep.append(i)
                    diffs = keep
                if diffs:
                    ok = False
                    i = diffs[0]
                    msg.append("page %08x differs at +%03x (asm %02x c %02x), %d bytes" % (p << 12, i, a[i], b[i], len(diffs)))
                    if os.environ.get("P4_DUMP"):
                        msg.append("asm %s c %s" % (a[max(0, i - 16):i + 48].hex(), b[max(0, i - 16):i + 48].hex()))
                    if fuzzy:
                        w = i & ~3
                        msg.append("floats asm %r c %r" % (struct.unpack("<f", a[w:w + 4])[0], struct.unpack("<f", b[w:w + 4])[0]))
        if not ok:
            fails.append("case %d: %s" % (n, "; ".join(msg)))
    return fails
