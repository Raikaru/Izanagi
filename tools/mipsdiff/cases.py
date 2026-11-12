"""Scenarios for the differential check (see run.py): one generator per translated function.

    python tools/mipsdiff/cases.py [addr ...]

builds the harness from overrides/<addr>.c, runs every scenario against the retail asm and prints
OK or the first mismatches. Each generator returns a run.Case with a random memory image; memory
that retail code reads at fixed addresses (gp-relative slots) is set up at those addresses.
"""
import os
import random
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import run as rn  # noqa: E402

N_CASES = int(os.environ.get("P4_DIFF_CASES", "150"))


def mkcases(gen, n, seed):
    rng = random.Random(seed)
    return [gen(rng) for _ in range(n)]


def setup_hdr(img, n, S=None):
    """Destination header: +0 base, +4 slot stride. Returns (hdr, stride)."""
    S = S or img.rng.choice([12, 16, 24, 32, 48])
    hdr = img.alloc(16)
    dst = img.alloc(max(1, 4 * n) * S + 32)
    img.w32(hdr, dst)
    img.w32(hdr + 4, S)
    return hdr, S


def gp_page(img):
    img.touch(0x00764000)


def strided(src_ptr_off, src_stride_off, words, ctxsize=0x100):
    def g(rng):
        img = rn.Img(rng)
        n = rng.randrange(-1, 5)
        hdr, _ = setup_hdr(img, max(n, 0), S=rng.choice([8, 16, 32]))
        ss = words * 4 + rng.choice([0, 4, 16])
        src = img.alloc(max(1, n) * ss + 16)
        ctx = img.alloc(ctxsize)
        img.w32(ctx + src_ptr_off, src)
        img.w32(ctx + src_stride_off, ss)
        return rn.Case([hdr, n, ctx], img, ret=False)
    return g


def gen_rep3(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 6)
    hdr, _ = setup_hdr(img, n)
    ss = rng.choice([12, 16, 20, 32])
    src = img.alloc(max(1, n) * ss + 16)
    ctx = img.alloc(0x100)
    img.w32(ctx + 0x14, src)
    img.w32(ctx + 0x18, ss)
    return rn.Case([hdr, n, ctx], img, ret=False)


def gen_fill(rng):
    img = rn.Img(rng)
    n = rng.randrange(-2, 24)
    hdr, _ = setup_hdr(img, max(n, 0))
    ctx = img.alloc(0x100)
    return rn.Case([hdr, n, ctx], img, ret=False)


def gen_6540(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 6)
    hdr, _ = setup_hdr(img, n)
    ctx = img.alloc(0x100)
    return rn.Case([hdr, n, ctx], img, ret=False)


def gen_1f5680(rng):
    img = rn.Img(rng)
    gp_page(img)
    o1 = img.alloc(0x1000)
    o2 = img.alloc(0x40)
    img.w32(0x0076449C, o1)
    img.w32(o1 + 0xC68, o2 if rng.random() < 0.8 else 0)
    img.mem.wn(o2 + 2, 2, rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 0xFFFF]))
    return rn.Case([], img, ret=True)


def gen_396e80(rng):
    img = rn.Img(rng)
    gp_page(img)
    tab = img.alloc(0x40)
    img.w32(0x007646C8, tab)
    off = rng.choice([0, 8, 16])
    t = img.alloc(16)
    img.w32(tab + off, rng.choice([0xFFFFFFFF, 0, 5, 0x7FFF]))
    img.w32(tab + off + 4, rng.choice([0, t]))
    img.w32(t, rng.choice([0, 1, 2, 3]))
    img.w32(t + 4, rng.randrange(0, 40))
    return rn.Case([off], img, ret=True)


def gen_3d5710(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x40)
    holder = img.alloc(16)
    img.w32(holder, obj)
    img.hook(obj + 0x28, 1)
    return rn.Case([holder], img, ret=True)


def gen_3cc250(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    obj = img.alloc(0x40)
    holder = img.alloc(16)
    img.w32(holder, obj)
    img.mem.wn(obj, 2, rng.choice([0, 0, 3, 0xFFFF]))
    img.w32(obj + 12, rng.randrange(1 << 32))
    img.hook(0x00887300, 1)
    return rn.Case([rng.randrange(1 << 32), holder], img, ret=True)


def gen_3d0790(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x40)
    fl = img.alloc(16)
    cache = img.alloc(0x40)
    cp = img.alloc(16)
    rec = img.alloc(16)
    img.w32(obj + 0xC, fl)
    img.w32(obj + 0x2C, rec)
    img.w32(obj + 0x30, cp)
    img.w32(cp, cache)
    flags = rng.choice([0, 1, 2, 3, 0x10, 0x12])
    img.w32(fl, flags)
    cnt = rng.randrange(0, 12)
    img.w32(rec + 4, cnt)
    eff = ((cnt - 2) * 3) if flags & 2 else cnt
    img.w32(cache + 0x24, eff if rng.random() < 0.6 else eff + rng.choice([1, -1, 5]))
    img.w32(cache + 0x2C, flags if rng.random() < 0.6 else flags ^ rng.choice([1, 4, 0x10]))
    img.w32(obj + 0x34, rng.choice([0, 8, 0x100, 0x108, 0x7]))
    return rn.Case([obj], img, ret=True)


def gen_cbf30(rng):
    img = rn.Img(rng)
    img.touch(0x00764000)
    img.touch(0x00887000)
    img.w32(0x00764804, 0x20)
    img.w32(0x00764808, rng.choice([0, 8, 0x10, 0x20]))
    fl = img.alloc(16)
    img.w32(0x008872E4 + img.r32(0x00764808), fl)
    light = img.alloc(0x80)
    world = img.alloc(16)
    other = img.alloc(16)
    img.w32(light + 0x20, 0xABCD)
    head = light + 0x2C
    n = rng.randrange(0, 5)
    elems = [img.alloc(0x20) for _ in range(n)]
    nodes = [e + 0xC for e in elems]
    if n == 0:
        img.w32(head, head)
        img.w32(head + 4, head)
    else:
        img.w32(head, nodes[0])
        img.w32(head + 4, nodes[-1])
        for i, nd in enumerate(nodes):
            img.w32(nd, nodes[i + 1] if i + 1 < n else head)
            img.w32(nd + 4, nodes[i - 1] if i > 0 else head)
        img.w32(other, elems[0])
        img.w32(other + 4, elems[-1])
        for i, e in enumerate(elems):
            img.w32(e, elems[i + 1] if i + 1 < n else other)
            img.w32(e + 4, elems[i - 1] if i > 0 else other)
    img.w32(light + 0x34, world)
    img.w32(light + 0x38, world)
    img.w32(world, light + 0x34)
    img.w32(world + 4, light + 0x34)
    img.hook(0x008873FC, 0)
    return rn.Case([rng.randrange(1 << 32), light], img, ret=True)


def gen_ce840(rng):
    img = rn.Img(rng)
    img.touch(0x00764000)
    img.w32(0x00764820, rng.choice([0, 4, 0x10]))
    arg = img.alloc(0x40)
    n = rng.randrange(0, 22)
    table = img.alloc(4 * (n + 2))
    img.w32(table, n)
    for i in range(1, n + 1):
        img.w32(table + 4 * i, img.alloc(0x40))
    img.w32(arg + 8, 0x01000000 if rng.random() < 0.85 else rng.choice([0, 0x800000, 0xFEFFFFFF]))
    img.w32(arg + img.r32(0x00764820), table)
    return rn.Case([arg], img, ret=True)


def gen_a910(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646D0, rng.choice([0, 4, 8]))
    img.touch(0x0064F000)
    for i in range(40):
        img.mem.wb(0x0064F280 + i, rng.randrange(256))
    arg = img.alloc(0x20)
    rec = img.alloc(0x100)
    img.w32(arg + img.r32(0x007646D0), rec)
    img.w32(rec + 0x80, rng.choice([4, 4, 0, 1, 7]))
    for off in (4, 8, 0x44, 0x48):
        img.w32(rec + off, rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 200]))
    return rn.Case([arg], img, ret=False)


def gen_2770(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x00764700, rng.choice([0, 4]))
    arg = img.alloc(0x20)
    obj = img.alloc(0x100)
    img.w32(arg + img.r32(0x00764700), obj)
    img.w32(obj + 0xA8, rng.randrange(1 << 10) if rng.random() < 0.9 else 0)
    if rng.random() < 0.5:
        img.w32(obj + 0xA4, 0)
    out = img.alloc(8)
    mask = rng.choice([1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x100, 3, 6, 0x180, 0x200])
    a3 = rng.choice([0, 0x40000000, 0x40000010, 0x80000000])
    return rn.Case([arg, out, mask, a3], img, ret=True)


def gen_5180(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x00764700, rng.choice([0, 4]))
    arg = img.alloc(0x20)
    obj = img.alloc(0x100)
    rec = img.alloc(0x400)
    img.w32(arg + img.r32(0x00764700), obj)
    img.w32(obj + 0x9C, rec)
    img.w32(rec + 0xD8, rng.choice([0, 1, 5]))
    img.w32(rec + 0xE8, rng.choice([0, 1, 2, 3]))
    img.w32(rec + 0xE4, rng.randrange(-1, 5))
    for f in range(8):
        img.w32(rec + 0x10 + 0x20 * f, rng.choice([0, 0, 7]))
    return rn.Case([arg], img, ret=True)


def gen_corner(streams):
    """streams: (pointer offset, stride offset, minimum record bytes) entries in the ctx block."""
    def g(rng):
        img = rn.Img(rng)
        n = rng.randrange(0, 6)
        hdr, _ = setup_hdr(img, n, S=rng.choice([12, 16, 24, 36]))
        ctx = img.alloc(0x100, fill_random=False)
        img.fill_f(ctx, 0x40, -8.0, 8.0)
        for ptr_off, stride_off, rec in streams:
            stride = rec + rng.choice([0, 4, 8, 16])
            base = img.alloc(max(1, n) * stride + 16, fill_random=False)
            img.fill_f(base, (max(1, n) * stride + 16) // 4, -50.0, 50.0)
            img.w32(ctx + ptr_off, base)
            img.w32(ctx + stride_off, stride)
        a = img.alloc(16, fill_random=False)
        img.fill_f(a, 3, -3, 3)
        b = img.alloc(16, fill_random=False)
        img.fill_f(b, 3, -3, 3)
        return rn.Case([hdr, a, b, n, ctx], img, ret=False)
    return g


def gen_3954b0(rng):
    img = rn.Img(rng)
    out = img.alloc(0x40)
    src = img.alloc(0x30, fill_random=False)
    img.fill_f(src, 12, -2.0, 2.0)
    return rn.Case([out, src], img, ret=False)


def gen_ab20(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646D0, rng.choice([0, 4, 8]))
    arg = img.alloc(0x20)
    rec = img.alloc(0x100)
    img.w32(arg + img.r32(0x007646D0), rec)
    img.w32(rec + 0x80, rng.choice([3, 6, 0, 1, 4]))
    return rn.Case([arg, rng.choice([1, 2, 3, 4, 5]), rng.choice([0, 1, 2, 3, 4, -1]), rng.randrange(1 << 32)], img, ret=True)


def gen_2950(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x00764700, rng.choice([0, 4]))
    arg = img.alloc(0x20)
    obj = img.alloc(0x100)
    rec = img.alloc(0x100)
    img.w32(arg + img.r32(0x00764700), obj)
    img.w32(obj + 0x9C, rec)
    return rn.Case([arg, rng.choice([0, 1, 2, 3]), rng.randrange(1 << 32)], img, ret=True)


def rw_io(slot):
    def g(rng):
        img = rn.Img(rng)
        obj = img.alloc(0x100)
        tbl = img.alloc(0x100)
        img.w32(obj + 0x50, tbl)
        img.hook(tbl + slot, 1)
        img.callee_ret[rn.HOOK_BASE + 1] = rng.randrange(1 << 32)
        return rn.Case([rng.randrange(1 << 32), rng.choice([0, 1, 2, 3, 7, 1000]), rng.randrange(0, 50), obj], img, ret=True)
    return g


def gen_de020(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x100)
    tbl = img.alloc(0x100)
    img.w32(obj + 0x50, tbl)
    img.hook(tbl + 0x28 + 0x10, 1)
    img.w32(obj + 0x30, rng.choice([0, 0, 5]))
    return rn.Case([obj, rng.randrange(-1000, 1000), rng.choice([0, 1, 2, 3, -1])], img, ret=True)


def gen_dd760(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    gp_page(img)
    img.callee_ret[0x4217E0] = rng.randrange(1 << 32)
    return rn.Case([rng.randrange(1 << 32)], img, ret=False)


def gen_b720(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 5)
    reg = img.alloc(16)
    ents = img.alloc(max(1, n) * 0x14 + 16)
    img.w32(reg, ents)
    img.w32(reg + 4, n)
    for i in range(n):
        img.w32(ents + 0x14 * i + 8, rng.randrange(1, 6))
    rec = img.alloc(0x40)
    me = img.alloc(0x40)
    img.w32(me, rec)
    img.w32(me + 4, rng.choice([0, 1, 2, 3, 4, 5]))
    img.w32(me + 8, rng.randrange(1 << 32))
    img.w32(me + 0xC, rng.choice([2, 2, 1, 0]))
    img.callee_ret[0x39BDF0] = img.alloc(0x40)
    return rn.Case([me, reg], img, ret=False)


def gen_b830(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 5)
    reg = img.alloc(16)
    ents = img.alloc(max(1, n) * 0x14 + 16)
    img.w32(reg, ents)
    img.w32(reg + 4, n)
    for i in range(n):
        img.w32(ents + 0x14 * i + 0x10, rng.choice([0, 0x1111 + i]))
    img.touch(0x00887000)
    img.hook(0x008873EC, 1)
    return rn.Case([reg], img, ret=False)


def gen_ba80(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 5)
    reg = img.alloc(16)
    ents = img.alloc(max(1, n) * 0x14 + 16)
    img.w32(reg, ents)
    img.w32(reg + 4, n)
    pipe = img.alloc(0x100)
    for i in range(n):
        img.w32(ents + 0x14 * i + 0xC, rng.choice([1, 2, 0, 1]))
        img.w32(ents + 0x14 * i + 4, rng.randrange(0, 3))
        img.w32(ents + 0x14 * i + 0x10, rng.randrange(1 << 32) if rng.random() < 0.5 else 0)
        rec = img.alloc(0x40)
        img.w32(ents + 0x14 * i, rec)
    img.callee_ret[0x39BBD0] = pipe
    return rn.Case([reg], img, ret=False)


def gen_bb70(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 6)
    reg = img.alloc(16)
    ents = img.alloc(max(1, n) * 0x14 + 16)
    img.w32(reg, ents)
    img.w32(reg + 4, n)
    vals = [rng.randrange(1, 5) for _ in range(n)]
    for i in range(n):
        img.w32(ents + 0x14 * i + 8, vals[i])
    return rn.Case([reg, rng.randrange(0, 6)], img, ret=True)


def gen_bbd0(rng):
    img = rn.Img(rng)
    d = img.alloc(0x100)
    for i in range(10):
        if rng.random() < 0.3:
            img.w32(d + 8 + 8 * i, 0)
            break
        img.w32(d + 8 + 8 * i, rng.randrange(1, 1 << 20))
    img.w32(d + 0x58, rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 9]))
    pair = img.alloc(16)
    img.w32(d + 0x6C, pair)
    for a in (0x4115D0, 0x412CA0, 0x412FB0, 0x412E90, 0x3D3A80, 0x3DF590, 0x4125E0, 0x3D38E0, 0x3D39A0):
        img.callee_ret[a] = img.alloc(0x40)
    return rn.Case([d], img, ret=True)


def gen_ptank(special_S, src_ptr_off=None, src_stride_off=None, rec_bytes=16, p_special=0.5):
    """PTank instancing helper: header (dst, S), count, ctx; the fast path is taken when S ==
    special_S (fill helpers) or when the source stride is 4*S (copy helpers)."""
    def g(rng):
        img = rn.Img(rng)
        n = rng.randrange(-1, 7)
        S = special_S if (special_S and rng.random() < p_special) else rng.choice([4, 8, 12, 16, 24])
        hdr, S = setup_hdr(img, max(n, 0), S=S)
        ctx = img.alloc(0x100)
        if src_ptr_off is not None:
            ss = 4 * S if rng.random() < p_special else rec_bytes + rng.choice([0, 4, 16])
            src = img.alloc(max(1, n) * max(ss, rec_bytes) + 32)
            img.w32(ctx + src_ptr_off, src)
            img.w32(ctx + src_stride_off, ss)
        return rn.Case([hdr, n, ctx], img, ret=False)
    return g


def gen_rot(ext, off):
    streams = [(4, 8, 12), (0x34, 0x38, 4)]
    if ext:
        streams.append((0x1C, 0x20, 8))
    inner = gen_corner(streams)

    def g(rng):
        return inner(rng)
    return g


def gen_5d20(rng):
    img = rn.Img(rng)
    n = rng.randrange(-1, 20)
    hdr, _ = setup_hdr(img, max(n, 0), S=rng.choice([4, 8, 16]))
    ss = rng.choice([4, 8, 12])
    ctx = img.alloc(0x100)
    src = img.alloc(max(1, n) * ss + 16)
    img.w32(ctx + 0x24, src)
    img.w32(ctx + 0x28, ss)
    return rn.Case([hdr, n, ctx], img, ret=False)


def gen_6890(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646C8, rng.choice([0, 8, 0x10]))
    frame = img.alloc(0x40)
    rec = frame + img.r32(0x007646C8)
    hier = 0
    if rng.random() < 0.8:
        n = rng.randrange(0, 5)
        hier = img.alloc(0x40)
        nodes = img.alloc(max(1, n) * 16 + 16)
        img.w32(hier + 4, n)
        img.w32(hier + 0x10, nodes)
        img.w32(hier + 0x14, frame if rng.random() < 0.5 else frame + 4)
    img.w32(rec + 4, hier)
    img.w32(rec, rng.randrange(1 << 32))
    return rn.Case([frame], img, ret=True)


def gen_hier_walk(arg_is_hier_first):
    def g(rng):
        img = rn.Img(rng)
        n = rng.randrange(0, 5)
        hier = img.alloc(0x40)
        nodes = img.alloc(max(1, n) * 16 + 16)
        img.w32(hier + 4, n)
        img.w32(hier + 0x10, nodes)
        keys = [rng.randrange(1, 4) for _ in range(n)]
        for i in range(n):
            img.w32(nodes + 16 * i, keys[i])
            img.w32(nodes + 16 * i + 0xC, 0)
        root = img.alloc(0x40)
        img.w32(hier + 0x14, root)
        img.callee_ret[0x397470] = rng.randrange(1, 4)
        if arg_is_hier_first:
            return rn.Case([hier], img, ret=True)
        return rn.Case([root, hier], img, ret=True)
    return g


def gen_hier_6fb0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    gp_page(img)
    n = rng.randrange(0, 6)
    a1 = img.alloc(max(1, n) * 4 + 16) if rng.random() < 0.6 else 0
    a2 = img.alloc(max(1, n) * 4 + 16) if rng.random() < 0.6 else 0
    img.w32(0x007646CC, rng.randrange(1 << 32))
    img.hook(0x008873F8, 3)
    img.hook(0x008873E8, 2)
    img.callee_ret[0x3D5790] = rng.randrange(1 << 32)
    return rn.Case([n, a1, a2, rng.choice([0, 1, 2, 3, 6]), rng.randrange(1 << 32)], img, ret=True)


def gen_6940(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646C8, rng.choice([0, 8, 0x10]))
    off = img.r32(0x007646C8)
    dstf = img.alloc(0x40)
    srcf = img.alloc(0x40)
    src = 0
    if rng.random() < 0.85:
        src = img.alloc(0x40)
        n = rng.randrange(0, 4)
        nodes = img.alloc(max(1, n) * 16 + 16)
        obj = img.alloc(0x40)
        obj2 = img.alloc(0x40)
        img.w32(src, rng.choice([0, 2, 4, 1, 3]))
        img.w32(src + 4, n)
        img.w32(src + 0x10, nodes)
        img.w32(src + 0x20, obj)
        img.w32(obj + 0x20, rng.randrange(1 << 32))
    img.w32(srcf + off + 4, src)
    img.w32(srcf + off, rng.randrange(1 << 32))
    dup = img.alloc(0x40)
    m = rng.randrange(0, 4)
    dnodes = img.alloc(max(1, m) * 16 + 16)
    img.w32(dup + 4, m)
    img.w32(dup + 0x10, dnodes)
    img.callee_ret[0x396FB0] = dup
    return rn.Case([dstf, srcf], img, ret=True)


def gen_6a40(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646C8, rng.choice([0, 8, 0x10]))
    frame = img.alloc(0x40)
    off = img.r32(0x007646C8)
    rec = 0
    if rng.random() < 0.8:
        rec = img.alloc(0x40)
        n = rng.randrange(0, 4)
        nodes = img.alloc(max(1, n) * 16 + 16)
        obj = img.alloc(0x40)
        img.w32(rec, rng.choice([0, 2, 4, 6, 1, 3]))
        img.w32(rec + 4, n)
        img.w32(rec + 0x10, nodes)
        img.w32(rec + 0x20, obj)
    img.w32(frame + off + 4, rec)
    img.callee_ret[0x3DF240] = 0xFE000000 | rng.randrange(0, 22)
    return rn.Case([rng.randrange(1 << 32), 0, frame], img, ret=True)


def gen_b2c0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    gp_page(img)
    img.w32(0x007646E0, rng.choice([0, 8, 0x10]))
    owner = img.alloc(0x40)
    hdr = owner + img.r32(0x007646E0)
    img.w32(hdr + 8, rng.choice([0, img.alloc(32)]))
    img.w32(hdr, rng.randrange(1 << 32))
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    return rn.Case([owner, rng.randrange(0, 7)], img, ret=True)


def gen_4fd0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    return rn.Case([rng.randrange(0, 9)], img, ret=True)


def gen_5100(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    tab = img.alloc(0x40)
    slots = img.alloc(0x40)
    img.w32(tab + 4, slots)
    return rn.Case([tab, rng.randrange(0, 4), rng.randrange(0, 12)], img, ret=True)


def gen_ae90(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    gp_page(img)
    img.w32(0x007646E0, rng.choice([0, 8, 0x10]))
    off = img.r32(0x007646E0)
    dst = img.alloc(0x40)
    src = img.alloc(0x40)
    n = rng.randrange(-1, 7)
    arr = img.alloc(max(1, n) * 20 + 16)
    img.w32(src + off, n)
    img.w32(src + off + 8, arr)
    img.hook(0x008873E8, 2)
    return rn.Case([dst, src], img, ret=True)


def gen_4c70(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x40)
    img.w32(obj + 0x18, rng.choice([0, 1, 2, 3]))
    draw = img.alloc(0x80)
    img.w32(draw + 4, rng.randrange(1 << 32))
    img.w32(draw + 0x18, obj)
    mtx = img.alloc(0x40, fill_random=False)
    img.fill_f(mtx, 16, -3.0, 3.0)
    img.callee_ret[0x3E9700] = mtx
    sph = img.alloc(0x10, fill_random=False)
    img.fill_f(sph, 4, -5.0, 5.0)
    return rn.Case([draw, sph, rng.randrange(1 << 32), rng.randrange(1 << 32), rng.randrange(1 << 32)], img, ret=True)


def gen_ac80(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646DC, rng.choice([0, 8, 0x10]))
    off = img.r32(0x007646DC)
    dst = img.alloc(0x100)
    src = img.alloc(0x100)
    tag = rng.randrange(1, 100)
    img.w32(src + 0x18, tag)
    rec = img.alloc(0x40, fill_random=False)
    img.fill_f(rec, 16, 0.5, 9.0)
    img.mem.wn(rec + 4, 2, rng.randrange(1 << 16))
    img.mem.wn(rec + 6, 2, rng.randrange(1 << 16))
    img.w32(src + off, rec if rng.random() < 0.8 else 0)
    img.wf(src + off + 4, img.rand_f(0.0, 5.0))
    img.w32(src + off + 8, tag if rng.random() < 0.75 else tag + 1)
    img.w32(dst + 4, img.alloc(0x20) if rng.random() < 0.7 else 0)
    img.w32(dst + 0x4C, rng.choice([0, 4, 8, 0xC, 0x40]))
    return rn.Case([dst, src], img, ret=True)


def gen_6c00(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    gp_page(img)
    img.w32(0x007646C8, rng.choice([0, 8, 0x10]))
    img.w32(0x007646CC, rng.randrange(1 << 32))
    frame = img.alloc(0x40)
    img.hook(0x008873F8, 3)
    img.hook(0x008873E8, 2)
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 40)
    img.callee_ret[0x3D5790] = rng.randrange(1 << 32)
    return rn.Case([rng.randrange(1 << 32), 0, frame], img, ret=True)


def gen_71d0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    gp_page(img)
    img.w32(0x007646CC, rng.randrange(1 << 32))
    img.hook(0x008873F8, 3)
    hier = img.alloc(0x40)
    nodes = img.alloc(64 * 16)
    depth = 0
    for i in range(64):
        if i < 20:
            f = rng.choice([0, 1, 2, 3])
            if (f & 1) and depth == 0:
                f &= ~1                 # a pop needs a matching push
            if f & 2:
                depth += 1
            if f & 1:
                depth -= 1
        else:
            f = 1
        img.w32(nodes + 16 * i + 8, f)
    img.w32(hier + 0x10, nodes)
    img.w32(hier + 8, img.alloc(64 * 64))
    img.w32(hier + 0x20, rng.randrange(1 << 32))
    img.callee_ret[0x3D5F50] = rng.randrange(1 << 32)
    return rn.Case([hier, rng.randrange(0, 10), rng.randrange(1 << 32), rng.randrange(1 << 32)], img, ret=True)


def gen_af40(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646E0, rng.choice([0, 8, 0x10]))
    off = img.r32(0x007646E0)
    owner = img.alloc(0x40)
    n = rng.randrange(-1, 6)
    arr = img.alloc(max(1, n) * 20 + 16)
    img.w32(owner + off, n)
    img.w32(owner + off + 8, arr)
    for i in range(max(n, 0)):
        img.w32(arr + 20 * i + 0x10, arr + 20 * rng.randrange(0, max(n, 1)))
    img.callee_ret[0x3DF240] = 0xFE000000 | rng.randrange(0, 8)
    img.callee_ret[0x3E2AB0] = 0xFE000000 | rng.randrange(0, 8)
    return rn.Case([rng.randrange(1 << 32), 0, owner], img, ret=True)


def gen_b080(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646E0, rng.choice([0, 8, 0x10]))
    off = img.r32(0x007646E0)
    owner = img.alloc(0x40)
    arr = img.alloc(20 * 40 + 16)
    img.w32(owner + off + 8, arr)
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 3)
    img.callee_ret[0x3E2910] = 0xFE000000 | rng.randrange(0, 8)
    img.callee_ret[0x39B2C0] = rng.choice([1, 1, 1, 0])
    return rn.Case([rng.randrange(1 << 32), 0, owner], img, ret=True)


def gen_42e0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x00764700, rng.choice([0, 4]))
    arg0 = img.alloc(0x20)
    obj = img.alloc(0x100)
    first = img.alloc(0x20)
    img.w32(first, rng.randrange(1 << 32))
    img.w32(arg0 + img.r32(0x00764700), obj)
    img.w32(obj, first)          # **(arg0+slot) path: obj[0] -> first[0]
    for off in (0x24, 0x28, 0x2C, 0x30, 0x34, 0x38):
        if rng.random() < 0.8:
            img.hook(obj + off, 5 if off == 0x24 else 1)
        else:
            img.w32(obj + off, 0)
    flags = rng.choice([0, 0x8000, 0x40000, 0x100000, 0x1E8000, 1])
    return rn.Case([arg0, rng.randrange(1 << 32), rng.randrange(1 << 32), flags], img, ret=True)


def gen_9c70(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    pipe = img.alloc(0x100)
    return rn.Case([pipe, rng.randrange(0, 6), rng.choice([0, 2, 1, 3])], img, ret=True)


def gen_b330(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x40)
    img.w32(obj + 0xC, rng.choice([1, 2, 3, 0]))
    img.w32(obj + 8, rng.randrange(0, 20))
    return rn.Case([obj], img, ret=True)


def gen_b3a0(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x40)
    img.w32(obj + 0xC, rng.choice([1, 2, 2, 3]))
    img.w32(obj + 8, rng.randrange(0, 12))
    tab = img.alloc(12 * 16, fill_random=False)
    img.fill_f(tab, 48, -5, 5)
    img.w32(obj + 0x14, tab)
    out = img.alloc(16)
    return rn.Case([obj, rng.randrange(-2, 12), out], img, ret=True)


def gen_b210(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    img.callee_ret[0x3E2910] = 0
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


def gen_bbe0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    n = rng.randrange(0, 6)
    src = img.alloc(max(1, n) * 12 + 16)
    img.callee_ret[0x3B9A40] = rng.choice([1, 1, 0])
    return rn.Case([n, rng.choice([1, 2, 2, 3]), src], img, ret=True)


def gen_71b0(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x80)
    data = img.alloc(0x40)
    img.w32(obj + 0x2C + 0xC, data)
    img.w32(obj + 0x30, rng.choice([0, 3, 5]))
    img.w32(obj + 0x34, rng.randrange(0, 7))
    img.callee_ret[0x3DF240] = 0xFE000000 | rng.randrange(0, 6)
    img.callee_ret[0x3E2AB0] = rng.choice([0, 1])
    return rn.Case([rng.randrange(1 << 32), obj], img, ret=True)


def gen_7290(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    obj = img.alloc(0x80)
    img.w32(obj, rng.randrange(0, 40))
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 6)
    img.callee_ret[0x3E2910] = rng.choice([0, 1, 7])
    return rn.Case([rng.randrange(1 << 32), obj], img, ret=True)


def _dma_chain(rng, img, conv):
    base = img.alloc(64 * 16)
    p = base
    for _ in range(rng.randrange(0, 5)):
        q = rng.randrange(0, 3)
        if rng.random() < 0.5:
            img.w32(p, (rng.randrange(0, 2) << 31) | (1 << 28) | q | (rng.randrange(0, 1 << 12) << 16 & 0x0FF00000 & 0))
            p += (q + 1) * 16
        else:
            img.w32(p, (3 << 28) | (rng.randrange(0, 1 << 12) << 16 & 0x0FFF0000 & 0) | rng.randrange(0, 1 << 16))
            img.w32(p + 4, base + 16 * rng.randrange(0, 40) if conv else rng.randrange(0, 40))
            p += 16
    img.w32(p, rng.choice([0, 6, 7]) << 28 | rng.randrange(0, 1 << 16))
    arg = img.alloc(0x40)
    img.w32(arg + 8, base)
    return arg


def gen_ce170(rng):
    img = rn.Img(rng)
    return rn.Case([_dma_chain(rng, img, True)], img, ret=True)


def gen_ce230(rng):
    img = rn.Img(rng)
    return rn.Case([_dma_chain(rng, img, False)], img, ret=True)


def gen_3ae0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    planes = img.alloc(0x200, fill_random=False)
    off = rng.choice([0, 0x10, 0x20])
    img.w32(0x008872E0, planes)
    img.w32(0x00764A24, off)
    for i in range(6):
        pl = planes + off + 0x60 + 0x14 * i
        img.fill_f(pl, 4, -3.0, 3.0)
        for k in (0x10, 0x11, 0x12):
            img.mem.wb(pl + k, rng.randrange(0, 2))
    box = img.alloc(0x80, fill_random=False)
    img.fill_f(box, 0x20, -4.0, 4.0)
    out = img.alloc(8)
    return rn.Case([box, out], img, ret=False)


def gen_0140(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.mem.wb(0x00764A74, rng.randrange(256))
    rec = img.alloc(0x40)
    fl = img.alloc(8)
    img.w32(rec + 0xC, fl)
    img.w32(fl, rng.choice([0, 1, 2, 3, 8, 0x10, 0x18, 0x20, 0x100, 0x103, rng.randrange(1 << 32)]))
    return rn.Case([rec, rng.choice([0, 1, 2, 3])], img, ret=False)


def gen_3500(rng):
    img = rn.Img(rng)
    img.touch(0x00752000)
    for off in range(0x24, 0x44, 4):
        img.w32(0x00752D00 + off, rng.randrange(1, 80))
    return rn.Case([rng.randrange(0, 200), rng.choice([rng.randrange(0, 0x200), 0x1FF])], img, ret=True)


def gen_65b0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873F8, 3)
    n = rng.randrange(0, 4)
    head = img.alloc(16)
    prev = 0
    ents = []
    for i in range(n):
        e = img.alloc(0x10)
        obj = img.alloc(0x40)
        img.mem.wn(e, 2, rng.randrange(0, 3))
        img.mem.wn(e + 2, 2, rng.randrange(0, 3))
        img.w32(e + 4, obj)
        img.w32(e + 8, rng.choice([0, 0, 0x55]))
        img.w32(e + 0xC, prev)
        prev = e
    img.w32(head, prev)
    tab = img.alloc(0x20)
    for i in range(4):
        img.w32(tab + 4 * i, img.alloc(0x40))
    pool = img.alloc(0x10)
    return rn.Case([pool, head, rng.randrange(0, 3), rng.randrange(0, 3), rng.randrange(0, 4), tab], img, ret=False)


def gen_d990(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    n = rng.randrange(0, 4)
    prev = 0
    for i in range(n):
        e = img.alloc(0x80)
        img.w32(e, prev)
        prev = e
    img.w32(0x00887180, prev)
    img.hook(0x008873C8, 1)
    img.hook(0x00887194, 0) if rng.random() < 0.7 else img.w32(0x00887194, 0)
    img.callee_ret[rn.HOOK_BASE + 1] = 0xFE000000 | rng.randrange(0, 5)
    return rn.Case([img.alloc(16)], img, ret=True)


def gen_5130(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00886000)
    n = rng.randrange(0, 6)
    img.w32(0x00764824, n)
    ids = [rng.randrange(1, 5) for _ in range(n)]
    for i in range(n):
        img.w32(0x00886E50 + 0x30 * i, ids[i])
    return rn.Case([rng.randrange(1, 6)], img, ret=True)


def gen_dca0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x00887188, rng.choice([0, 1, 1, 1]))
    n = rng.randrange(0, 4)
    prev = 0
    for i in range(n):
        e = img.alloc(0x40)
        img.w32(e, prev)
        img.w32(e + 0xC, img.alloc(16))
        prev = e
    img.w32(0x00887180, prev)
    img.hook(0x008873D4, 6)
    img.hook(0x008873D0, 1)
    if rng.random() < 0.7:
        img.hook(0x00887194, 0)
    else:
        img.w32(0x00887194, 0)
    img.w32(0x00764840, rng.choice([0, img.alloc(16)]))
    s = img.alloc(48)
    for i in range(rng.randrange(0, 20)):
        img.mem.wb(s + i, rng.choice([0x3A, 0x61, 0x62, 0x30]))
    img.mem.wb(s + 47, 0)
    for i in range(20, 47):
        img.mem.wb(s + i, 0)
    img.callee_ret[rn.HOOK_BASE + 1] = 0xFE000000 | rng.randrange(0, 4)
    return rn.Case([s], img, ret=True)


def gen_8760(rng):
    img = rn.Img(rng)
    img.pool()
    n = rng.randrange(0, 20)
    obj = img.alloc(0x40)
    fmts = [(0x20, 0x20), (0x18, 0x20), (0x10, 0x20), (0x0F, 0x20), (0x08, 0x20), (0x04, 0x20), (0x01, 0x20),
            (0x08, 0x08), (0x04, 0x08), (0x04, 0x04), (0x01, 0x08), (0x01, 0x04), (0x02, 0x20), (0x20, 0x08)]
    a3, f = rng.choice(fmts)
    stride = 4 * 24
    base = img.alloc(stride * 4 + 32)
    pal = img.alloc(256 * 4)
    src = img.alloc(100 * 4 + 64)
    img.w32(obj + 4, n)
    img.w32(obj + 0xC, f)
    img.w32(obj + 0x10, stride)
    img.w32(obj + 0x14, base)
    img.w32(obj + 0x18, pal)
    img.callee_ret[0x3DF590] = rng.randrange(1 << 32)
    return rn.Case([obj, rng.randrange(0, 3), src, a3, rng.choice([0, 1])], img, ret=True)


def gen_4a80(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873F0, 3 if False else 2)
    arr = img.alloc(0x40)
    cap = rng.randrange(0, 4)
    cnt = rng.randrange(0, cap + 3) if cap else 0
    cnt = min(cnt, cap + 1)
    data = img.alloc(max(1, cap + 1) * 4 + 16) if (cnt < cap or rng.random() < 0.6) else 0
    img.w32(arr, data)
    img.w32(arr + 4, cnt)
    img.w32(arr + 8, cap)
    item = img.alloc(0x40)
    img.mem.wn(item + 0x18, 2, rng.randrange(1 << 16))
    img.callee_ret[0x3DF590] = 0
    return rn.Case([arr, item], img, ret=True)


def gen_2a80(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873EC, 4)
    obj = img.alloc(0x80)
    img.mem.wn(obj + 0xE, 2, rng.choice([0, 1, 2, 3, 0xFFFF, 0x8000]))
    for off in (0x54, 0x58, 0x5C):
        if rng.random() < 0.3:
            img.w32(obj + off, 0)
    return rn.Case([obj], img, ret=True)


def gen_9300(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873EC, 4)

    def node(depth):
        n = img.alloc(0x100)
        kind = rng.choice([-2, -1, -1, 5]) if depth < 3 else rng.choice([-2, -1])
        img.w32(n, kind)
        if kind == -1:
            for off in (0x34, 0x8, 0xC, 0x30, 0x4, 0x78) + tuple(0x10 + 4 * i for i in range(8)):
                img.w32(n + off, rng.choice([0, 0x1000 + off]))
            for lo in (0x38, 0x40):
                k = rng.randrange(0, 3)
                ents = [img.alloc(16) for _ in range(k)]
                prev = n + lo
                chain = [n + lo] + ents + [n + lo]
                for j in range(1, len(chain) - 1):
                    img.w32(chain[j], chain[j + 1])
                img.w32(n + lo, chain[1] if k else n + lo)
        elif kind != -2:
            img.w32(n + 8, node(depth + 1) if rng.random() < 0.7 else 0)
            img.w32(n + 0xC, node(depth + 1) if rng.random() < 0.7 else 0)
        return n
    return rn.Case([node(0)], img, ret=False)


def gen_aa50(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    gp_page(img)
    img.w32(0x00764808, rng.choice([0, 4, 8]))
    img.w32(0x008872E0 + img.r32(0x00764808), img.alloc(16))
    img.hook(0x008873F8, 3)
    owner = img.alloc(0x40)

    def build(depth):
        n = img.alloc(0x80, fill_random=False)
        if depth >= 3 or rng.random() < 0.25:
            img.w32(n, rng.choice([-1, 0xFFFFFFF0]))
            img.w32(n + 0x38, n + 0x38)   # empty circular list: sentinel points at itself
            return n
        img.w32(n, rng.choice([0, 4, 8]))
        img.wf(n + 0x10, rng.uniform(-2, 2))
        img.wf(n + 0x14, rng.uniform(-2, 2))
        img.w32(n + 8, build(depth + 1))
        img.w32(n + 0xC, build(depth + 1))
        return n
    img.w32(owner + 0x1C, build(0))
    obj = img.alloc(0x80)
    img.w32(obj + 0x64, obj + 0x64)
    box = img.alloc(0x10, fill_random=False)
    img.fill_f(box, 4, -2.0, 2.0)
    img.wf(box + 12, abs(img.rf(box + 12)))
    img.callee_ret[0x3BFAE0] = box
    return rn.Case([owner, obj], img, ret=False)


def gen_3ae0b(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3E2910] = 0
    img.callee_ret[0x3C3D20] = rng.choice([0, img.alloc(0x80)])
    img.callee_ret[0x3E3110] = rng.choice([0, 1])
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


def gen_53c0(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00886000)
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    n = rng.randrange(0, 4)
    img.w32(0x00764824, n)
    ids = [rng.randrange(1, 4) for _ in range(n)]
    for i in range(n):
        img.w32(0x00886E50 + 0x30 * i, ids[i])
        img.w32(0x00886E50 + 0x30 * i + 8, rng.randrange(1, 10))
        img.w32(0x00886E50 + 0x30 * i + 0x2C, rng.choice([0, 0, 5, 12]))
        img.hook(0x00886E50 + 0x30 * i + 0x20, 1)
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 5)
    img.callee_ret[0x3DF300] = 0xFE000000 | rng.randrange(0, 3)
    img.callee_ret[0x3DF590] = rng.randrange(1 << 32)
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


def gen_5330(rng):
    img = rn.Img(rng)
    img.callee_ret[0x3E2F60] = rng.choice([0, 0x1234])
    img.callee_ret[0x3DF050] = rng.choice([0, 1])
    img.callee_ret[0x3D53C0] = rng.randrange(1 << 32)
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


def gen_5600(rng):
    img = rn.Img(rng)
    img.pool()
    tmpl = img.alloc(0x40)
    img.w32(tmpl, rng.randrange(1 << 32))
    img.hook(tmpl + 0x24, 1)
    obj = img.alloc(0x40)
    img.w32(obj, tmpl)
    img.callee_ret[0x3D5710] = rng.randrange(1 << 32)
    img.callee_ret[0x3DF240] = 0xFE000000 | rng.randrange(0, 5)
    img.callee_ret[0x3DF1A0] = rng.choice([0, 1])
    return rn.Case([obj, rng.randrange(1 << 32)], img, ret=True)


def gen_0290(rng):
    img = rn.Img(rng)
    arg = img.alloc(0x100)
    off = rng.choice([0, 0x10, 0x20])
    img.w32(arg + off, rng.choice([0, 0x5555]))
    img.callee_ret[0x38FB20] = rng.randrange(1 << 32)
    return rn.Case([arg, off], img, ret=True)


def gen_2340(rng):
    img = rn.Img(rng)
    img.callee_ret[0x3A20E0] = rng.randrange(1 << 32)
    img.callee_ret[0x3A2390] = rng.randrange(1 << 32)
    return rn.Case([rng.randrange(1 << 32), rng.randrange(1 << 32), rng.randrange(1 << 32)], img, ret=True)


def gen_7ac0(rng):
    img = rn.Img(rng)
    blk = img.alloc(0x80)
    img.callee_ret[0x3E2AB0] = rng.choice([0, 1, 1])
    img.callee_ret[0x3DF240] = rng.choice([0, 1, 1])
    return rn.Case([blk, rng.randrange(1 << 32)], img, ret=True)


def gen_7c50(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x00764838, rng.randrange(1 << 32))
    img.hook(0x008873FC, 1)
    obj = img.alloc(0x40)
    inner = img.alloc(0x80)
    img.w32(obj + 0x14, inner)
    img.w32(inner + 0x40, rng.choice([1, 1, 2, 3, 0]))
    return rn.Case([obj], img, ret=True)


def gen_60f0(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 5)
    arr = img.alloc(max(1, n) * 4 + 16)
    for i in range(n):
        img.w32(arr + 4 * i, rng.randrange(1 << 32))
    obj = img.alloc(0x40)
    cls = img.alloc(0x40)
    img.w32(obj, cls)
    img.w32(obj + 4, rng.randrange(1 << 32))
    img.hook(cls + 0x1C, 1)
    img.callee_ret[0x3DF890] = arr
    img.callee_ret[0x3DF8A0] = arr + 4 * n
    return rn.Case([obj], img, ret=True)


def gen_f0e0(rng):
    img = rn.Img(rng)
    img.touch(0x00886000)
    img.w32(0x00886498, rng.choice([0, 4, 8]))
    q = img.alloc(0x80)
    img.w32(q + 8, rng.choice([0, 0x01000000, 0x1000000 | 5, 5]))
    rec = img.alloc(0x40)
    img.w32(rec + 0x18, q)
    obj = img.alloc(0x80)
    img.w32(obj + 8, rec)
    img.w32(obj + 0x2C, rng.randrange(1 << 32))
    w = img.alloc(8)
    return rn.Case([obj, w], img, ret=True)


def gen_4220(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x007647D0, rng.choice([0, 4, 8]))
    img.w32(0x008872E0 + img.r32(0x007647D0), rng.randrange(1 << 32))
    img.hook(0x008873FC, 1)
    obj = img.alloc(0x40)
    img.mem.wn(obj + 0x18, 2, rng.choice([1, 1, 2, 0, 5, 0xFFFF]))
    return rn.Case([obj], img, ret=True)


def gen_9640(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    obj = img.alloc(0x100)
    img.mem.wn(obj + 0x84, 2, rng.choice([0, 1, 5, 0x8000]))
    img.w32(obj + 0x7C, rng.choice([0, 0x7700]))
    s = img.alloc(0x100)
    img.w32(0x008872E4, s)
    img.w32(s + 0x6C, rng.choice([0, 0x6600]))
    img.w32(0x00764AA8, rng.choice([0, 8]))
    img.w32(0x008872E0 + img.r32(0x00764AA8) + 0x40, 0x4400)
    img.callee_ret[0x4114D0] = rng.choice([0, 1])
    return rn.Case([obj], img, ret=True)


def gen_cbe0(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873EC, 4)
    base = img.alloc(0x40)
    off = rng.choice([0, 8])
    n = rng.randrange(-1, 5)
    img.w32(base + off, n)
    img.w32(base + off + 4, rng.choice([0, img.alloc(0x80)]))
    return rn.Case([base, off], img, ret=True)


def gen_49a0(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873EC, 4)
    arr = img.alloc(0x40)
    n = rng.randrange(0, 5)
    data = img.alloc(max(1, n) * 4 + 16)
    for i in range(n):
        img.w32(data + 4 * i, rng.randrange(1 << 32))
    img.w32(arr, rng.choice([0, data]))
    img.w32(arr + 4, n)
    img.w32(arr + 8, rng.randrange(1 << 32))
    img.callee_ret[0x3C4220] = 1
    return rn.Case([arr], img, ret=True)


def gen_7a20(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x00764838, rng.randrange(1 << 32))
    img.hook(0x008873F8, 3)
    img.callee_ret[0x3E2910] = rng.choice([0, 1, 1])
    img.callee_ret[0x3DF360] = rng.choice([0, 1, 1])
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


def gen_dfa0(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873EC, 4)
    n = rng.randrange(0, 5)
    tab = img.alloc(4 * (n + 2) + 8)
    img.w32(tab, n)
    for i in range(n):
        o = 0 if rng.random() < 0.2 else img.alloc(0x40)
        if o:
            img.w32(o, rng.choice([0, 0, 3]))
            img.w32(o + 4, rng.choice([0, 0, 7]))
        img.w32(tab + 4 + 4 * i, o)
    return rn.Case([tab], img, ret=False)


def _circ(img, rng, head, k, node_off=0, size=0x20):
    """Circular next-pointer list headed at `head`: returns the node addresses."""
    nodes = [img.alloc(size) + node_off for _ in range(k)]
    chain = [head] + nodes + [head]
    img.w32(head, chain[1])
    for j in range(1, len(chain) - 1):
        img.w32(chain[j], chain[j + 1])
    return nodes


def gen_e2e0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x00764820, rng.choice([0, 8, 0x10]))
    off = img.r32(0x00764820)
    obj = img.alloc(0x100)
    rec = img.alloc(0x20)
    hw = rng.randrange(0, 4)
    img.mem.wn(rec + 4, 2, hw)
    img.w32(obj + 0x54, rec)

    def table(n, first):
        t = img.alloc(4 * (n + 2))
        img.w32(t, first)
        for i in range(n):
            img.w32(t + 4 + 4 * i, rng.choice([0, 0x1000 + i]))
        return t
    n = rng.randrange(0, 4)
    t1 = table(n, rng.choice([hw, hw + 1]) if rng.random() < 0.8 else n)
    img.w32(obj + off, rng.choice([0, t1]))
    n2 = rng.randrange(0, 4)
    t2 = table(n2, n2)
    slot2 = img.alloc(8)
    img.w32(slot2, t2)
    img.callee_ret[0x3CE050] = slot2
    return rn.Case([obj], img, ret=True)


def gen_ce50(rng):
    img = rn.Img(rng)
    base = img.alloc(0x40)
    off = rng.choice([0, 8])
    n = rng.randrange(-1, 5)
    img.w32(base + off, n)
    ents = img.alloc(max(1, n) * 16 + 16)
    for i in range(max(n, 0)):
        img.w32(ents + 16 * i + 4, rng.choice([0, 1]))
    img.w32(base + off + 4, ents)
    img.callee_ret[0x3BC620] = rng.randrange(0, 100)
    return rn.Case([base, off], img, ret=True)


def gen_62e0(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x40)
    n = rng.randrange(-1, 5)
    img.w32(obj + 4, n)
    nodes = img.alloc(max(1, n) * 0x24 + 16)
    for i in range(max(n, 0)):
        img.w32(nodes + 0x24 * i, nodes + 0x24 * rng.randrange(0, max(n, 1)))
    img.w32(obj + 0x10, nodes)
    img.callee_ret[0x3DF1A0] = 0xFE000000 | rng.randrange(0, 6)
    img.callee_ret[0x3DF240] = 0xFE000000 | rng.randrange(0, 6)
    return rn.Case([obj, rng.randrange(1 << 32)], img, ret=True)


def gen_61f0(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x40)
    n = rng.randrange(-1, 5)
    img.w32(obj + 4, n)
    nodes = img.alloc(max(1, n) * 0x24 + 16)
    img.w32(obj + 0x10, nodes)
    img.callee_ret[0x3DF300] = 0xFE000000 | rng.randrange(0, 6)
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 12)
    return rn.Case([rng.randrange(1 << 32), obj], img, ret=True)


def gen_cc80(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    a0 = img.alloc(0x100)
    a1 = img.alloc(0x100)
    off = rng.choice([0, 0x10, 0x20])
    n = rng.randrange(-1, 5)
    img.w32(a1 + off, n)
    img.w32(a1 + off + 4, img.alloc(max(1, n) * 16 + 16))
    return rn.Case([a0, a1, off], img, ret=True)


def gen_8dd0(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.w32(0x008872E0, rng.randrange(1 << 32))
    img.mem.wn(0x008872E8, 2, rng.randrange(0, 3))
    obj = img.alloc(0x100)
    head = obj + 0x38
    k = rng.randrange(0, 4)
    nodes = _circ(img, rng, head, k)
    for nd in nodes:
        light = img.alloc(0x80)
        img.w32(nd + 8, light)
        img.mem.wb(light + 2, rng.choice([0, 4, 4, 0xFF]))
        img.mem.wn(light + 0x60, 2, rng.randrange(0, 3))
        img.hook(light + 0x48, 1)
    img.callee_ret[0x3C8CE0] = rng.choice([0, 1, 1])
    img.callee_ret[0x3BFAE0] = rng.randrange(1 << 32)
    img.callee_ret[0x3E8200] = rng.choice([0, 1])
    return rn.Case([obj], img, ret=True)


def gen_d290(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    dev = img.alloc(0x100)
    img.hook(0x008873AC, 0)
    img.hook(dev + 0x14, 1)
    slot = img.alloc(0x80)
    img.callee_ret[rn.HOOK_BASE + 1] = slot
    img.callee_ret[0x3DD7D0] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3DD990] = rng.choice([0, 0, 1])
    img.callee_ret[0x3DD620] = rng.choice([1, 1, 0, 3])
    return rn.Case([dev, rng.randrange(-1, 4), img.alloc(16), rng.randrange(1 << 32)], img, ret=True)


def gen_ac30(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x007647FC, rng.choice([0, 8, 0x10]))
    img.w32(0x00764808, rng.choice([0, 8]))
    img.w32(0x008872E0 + img.r32(0x00764808), rng.randrange(1 << 32))
    img.hook(0x008873FC, 4)
    obj = img.alloc(0x100)
    own = obj + img.r32(0x007647FC)
    img.w32(own, rng.choice([0, img.alloc(0x40)]))
    img.hook(own + 4, 1)
    img.callee_ret[rn.HOOK_BASE + 1] = rng.choice([0, 1, 1, 1])
    head = obj + 0x64
    k = rng.randrange(0, 4)
    nodes = _circ(img, rng, head, k, node_off=0xC)
    # make each node doubly linked (prev at +4 is part of the 0x10 slot): e = node - 0xC
    for j, nd in enumerate(nodes):
        e = nd - 0xC
        img.w32(e + 8, rng.choice([0, 0x55]))
        img.w32(e + 0x14, rng.choice([0, 0x66]))
        img.w32(e + 0x10, img.alloc(16))
        img.w32(e + 0, img.alloc(16))
        img.w32(e + 4, img.alloc(16))
    img.callee_ret[0x3CAA50] = 0
    return rn.Case([obj], img, ret=True)


def gen_0520(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x100)
    img.w32(obj + 4, rng.randrange(1 << 32))
    img.w32(obj + 0x28, rng.randrange(1 << 32))
    k = rng.randrange(0, 4)
    _circ(img, rng, obj + 8, k, node_off=0x40, size=0x80)
    clone = img.alloc(0x80)
    img.callee_ret[0x3C0640] = rng.choice([0, clone, clone, clone])
    img.callee_ret[0x3E90F0] = rng.choice([0, 0x4400, 0x4400])
    img.callee_ret[0x3BEA60] = 0xFE000000 | rng.randrange(0, 5)
    return rn.Case([obj], img, ret=True)


def gen_b550(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x40)
    img.w32(obj + 8, rng.choice([0, 0x04, 0x80, 0x84, 0x01000084, 0x01000000, 0x80000100]))
    par = img.alloc(0x100, fill_random=False)
    img.fill_f(par, 0x40, 0.5, 4.0)
    img.callee_ret[0x3E89C0] = rng.randrange(1 << 32)
    img.callee_ret[0x3EF6D0] = rng.choice([0, 0x4400, 0x4400])
    return rn.Case([obj, par], img, ret=False)


def gen_0700(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x007647A4, rng.choice([0, 8]))
    img.w32(0x007647A0, rng.choice([0, 8]))
    img.w32(0x007647B0, rng.choice([0, 4]))
    img.w32(0x008872E0 + img.r32(0x007647B0) + 4, rng.randrange(1 << 32))
    img.hook(0x008873FC, 4)
    obj = img.alloc(0x100)
    img.w32(obj + 4, rng.choice([0, 0x4400]))
    _circ(img, rng, obj + 8, rng.randrange(0, 3), node_off=0x40, size=0x80)
    _circ(img, rng, obj + 0x10, rng.randrange(0, 3), node_off=4 + img.r32(0x007647A4), size=0x40)
    _circ(img, rng, obj + 0x18, rng.randrange(0, 3), node_off=4 + img.r32(0x007647A0), size=0x40)
    return rn.Case([obj], img, ret=True)


def gen_ce50b(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    slot = img.alloc(8)
    if rng.random() < 0.6:
        n = rng.randrange(0, 4)
        t = img.alloc(4 * (n + 2))
        img.w32(t, n)
        for i in range(n):
            o = 0 if rng.random() < 0.2 else img.alloc(0x40)
            if o:
                img.w32(o, rng.choice([0, 0, 3]))
                img.w32(o + 4, rng.choice([0, 0, 7]))
            img.w32(t + 4 + 4 * i, o)
        img.w32(slot, t)
    else:
        img.w32(slot, 0)
    return rn.Case([slot, rng.randrange(1, 6)], img, ret=True)


def gen_8d40(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    obj = img.alloc(0x200)
    g = img.alloc(0x40)
    img.w32(g + 8, rng.choice([0, 0x80, 0xFF, 0x7F]))
    img.w32(obj + 0x18, g)
    off = rng.choice([0x10, 0x20, 0x30])
    img.callee_ret[0x3C1B80] = off
    img.mem.wb(obj + off, rng.choice([0, 0, 1, 7]))
    img.callee_ret[0x39B6E0] = rng.randrange(1 << 32)
    return rn.Case([obj, rng.choice([0, 1, 2, 3, 4])], img, ret=True)


def gen_81d0(rng):
    img = rn.Img(rng)
    ras = img.alloc(0x20)
    img.w32(ras + 4, rng.randrange(1, 300))
    img.w32(ras + 8, rng.randrange(1, 300))
    return rn.Case([rng.randrange(1 << 32), rng.choice([1, 4, 8, 9, 16, 24, 32]), ras], img, ret=True)


def gen_ddf0(rng):
    img = rn.Img(rng)
    img.callee_ret[0x443010] = 0xFD000000 | rng.randrange(0, 32)
    img.callee_ret[0x3DDCA0] = rng.choice([0, 0x7700])
    img.callee_ret[0x3DDA50] = rng.randrange(1 << 32)
    return rn.Case([img.alloc(16), img.alloc(16)], img, ret=True)


def gen_cb40(rng):
    img = rn.Img(rng)
    n = rng.randrange(-1, 5)
    out = img.alloc(max(1, n) * 12 + 16, fill_random=False)
    src = img.alloc(max(1, n) * 12 + 16, fill_random=False)
    img.fill_f(src, max(1, n) * 3, -5, 5)
    q = img.alloc(16, fill_random=False)
    img.fill_f(q, 4, -2, 2)
    return rn.Case([out, src, n, q], img, ret=True)


def gen_c610(rng):
    img = rn.Img(rng)
    q = img.alloc(16, fill_random=False)
    m = img.alloc(0x40, fill_random=False)
    img.fill_f(m, 16, -1.0, 1.0)
    if rng.random() < 0.5:
        img.wf(m + 0x14, 0.9)
        img.wf(m, 0.8)
        img.wf(m + 0x28, 0.7)
    return rn.Case([q if rng.random() < 0.9 else 0, m if rng.random() < 0.9 else 0], img, ret=True)


def gen_fae0(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x100, fill_random=False)
    img.fill_f(obj, 0x40, 0.1, 3.0)
    img.mem.wb(obj + 3, rng.randrange(256))
    img.w32(obj + 0x4C, rng.choice([0, 2, 3, 0x12]))
    img.w32(obj + 4, 0 if rng.random() < 0.3 else rng.randrange(1 << 32))      # a null frame: retail reads PS2 low RAM (see _py_9240_low)
    m = img.alloc(0x40, fill_random=False)
    img.fill_f(m, 16, -3.0, 3.0)
    img.w32(m + 0xC, rng.choice([0, 1, 2, 3, 0x103]))
    img.callee_ret[0x3E9240] = rng.choice([0, 1])
    img.callee_ret[0x3E9700] = m
    return rn.Case([obj], img, ret=True)


def gen_1d40(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 6)
    inner = img.alloc(0x40)
    img.w32(inner + 0x14, n)
    pts = img.alloc(max(1, n) * 12 + 16, fill_random=False)
    img.fill_f(pts, max(1, n) * 3, -10.0, 10.0)
    obj = img.alloc(0x40)
    img.w32(obj, inner)
    img.w32(obj + 0x14, pts)
    out = img.alloc(16)
    return rn.Case([obj, out], img, ret=True)


def gen_8410(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    obj = img.alloc(0x100)
    for i in range(2):
        s = obj + 64 * i
        img.w32(s + 0x20, rng.choice([0, 1, 2, 3, 4, 5, 6]))
        for off in (0, 4, 8):
            img.w32(s + off, rng.choice([0, 0x5000 + 16 * i + off]))
    return rn.Case([obj], img, ret=True)


def gen_2bb0(rng):
    img = rn.Img(rng)
    img.touch(0x00708000)
    for i in range(0, 24 * 6, 4):
        img.w32(0x007083D0 + i, rng.choice([0xFFFFFFFF, 0xFFFFFFFF, rng.randrange(1 << 32), rng.randrange(1, 100)]))
    obj = img.alloc(0x100)
    tgt = img.alloc(0x100)
    img.w32(obj + 0x4C, tgt)
    img.w32(obj + 0x5C, rng.randrange(0, 11))
    img.w32(obj + 0x60, rng.randrange(0, 11))
    return rn.Case([obj], img, ret=True)


def gen_c880(rng):
    img = rn.Img(rng)
    hdr = img.alloc(16)
    n = rng.randrange(-1, 5)
    ents = img.alloc(max(1, n) * 16 + 16)
    for i in range(max(n, 0)):
        img.w32(ents + 16 * i + 4, rng.choice([0, 1]))
    img.w32(hdr, n)
    img.w32(hdr + 4, ents)
    img.callee_ret[0x3DF240] = rng.choice([0, 1, 1])
    img.callee_ret[0x3BC400] = 0xFD000000 | rng.randrange(0, 2)
    return rn.Case([rng.choice([0, hdr, hdr, hdr]), rng.randrange(1 << 32)], img, ret=True)


def gen_9d40(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x007647F0, rng.choice([0, 8, 0x10]))
    off = img.r32(0x007647F0)
    img.w32(0x008872E0 + off, rng.randrange(1 << 32))
    img.hook(0x008873FC, 4)
    img.hook(0x008873EC, 4)
    head = 0x008872E0 + off + 4
    obj = img.alloc(0x100)
    img.mem.wb(obj + 3, rng.randrange(256))
    img.w32(obj + 0x1C, rng.choice([0, 0x4400]))
    k = rng.randrange(0, 4)
    recs = [img.alloc(0x20) for _ in range(k)]
    nodes = [r + 8 for r in recs]
    chain = [head] + nodes + [head]
    img.w32(head, chain[1])
    for j in range(1, len(chain) - 1):
        img.w32(chain[j], chain[j + 1])
        img.w32(recs[j - 1], obj if rng.random() < 0.35 else img.alloc(8))
        img.w32(recs[j - 1] + 8, chain[j + 1])
        img.w32(recs[j - 1] + 0xC, chain[j - 1] if j > 1 else head)
    return rn.Case([obj], img, ret=True)


# ---- 003c0f20 (clump stream read) stand-ins
C_F20 = {
 0x3DF050: ("unsigned func_003df050(unsigned s, unsigned t, unsigned *sz, unsigned *v) { static const unsigned K[6] = {0x35000, 0x36000, 0x37002, 0x34000, 0x37003, 0x35500}; "
            "unsigned r = p4_record({ID}, s, t, 0, 0); if (!r) return 0; if (sz) *sz = 0x18; if (v) *v = K[(s >> 4) % 6]; return 1; }"),
 0x3E2910: ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; "
            "p[0] = s % 3; p[1] = (s >> 2) % 4; p[2] = (s >> 4) % 4; return n; }"),
 0x3E5DF0: ("unsigned func_003e5df0(unsigned s, unsigned *t) { unsigned r = p4_record({ID}, s, 0, 0, 0); if (!r) return 0; t[0] = 0x38200000; t[1] = 0x77; return 1; }"),
 0x3BEE80: ("unsigned func_003bee80(unsigned s, unsigned *m) { unsigned r = p4_record({ID}, s, 0, 0, 0); if (!r) return 0; m[0] = 0x38201000; m[1] = (s >> 3) % 4; return 1; }"),
 0x3BF5F0: ("unsigned func_003bf5f0(unsigned s, unsigned *t, unsigned *m) { unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, 0, 0); return r ? 0x38300000u + 0x100u * n : 0; }"),
 0x3C3AE0: ("unsigned func_003c3ae0(unsigned s) { unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, 0, 0); return r ? 0x38400000u + 0x100u * n : 0; }"),
 0x3E53B0: ("unsigned func_003e53b0(unsigned s) { unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, 0, 0); return r ? 0x38500000u + 0x100u * n : 0; }"),
}


def _ok(m, addr):
    cid = m.callees.index(addr)
    n = m.ncalls.get(cid, 0)
    m.ncalls[cid] = n + 1
    ret = m.cret.get(addr, 0)
    if (ret >> 24) == 0xFE:
        ok = 1 if n < (ret & 0xFFFF) else 0
    elif (ret >> 24) == 0xFD:
        ok = (ret >> n) & 1
    else:
        ok = ret
    return cid, n, ok


def _py_f050(m):
    cid, n, ok = _ok(m, 0x3DF050)
    m.ilog.extend([cid, m.g32(4), m.g32(5), 0, 0])
    if not ok:
        return 0
    if m.g32(6):
        m.mem.wn(m.g32(6), 4, 0x18)
    if m.g32(7):
        m.mem.wn(m.g32(7), 4, [0x35000, 0x36000, 0x37002, 0x34000, 0x37003, 0x35500][(m.g32(4) >> 4) % 6])
    return 1


def _py_f2910(m):
    cid, n, ok = _ok(m, 0x3E2910)
    m.ilog.extend([cid, m.g32(4), 0, m.g32(6), 0])
    if not ok:
        return 0
    s = m.g32(4)
    for j, v in enumerate((s % 3, (s >> 2) % 4, (s >> 4) % 4)):
        m.mem.wn(m.g32(5) + 4 * j, 4, v)
    return m.g32(6)


def _py_f5df0(m):
    cid, n, ok = _ok(m, 0x3E5DF0)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    if not ok:
        return 0
    m.mem.wn(m.g32(5), 4, 0x38200000)
    m.mem.wn(m.g32(5) + 4, 4, 0x77)
    return 1


def _py_fee80(m):
    cid, n, ok = _ok(m, 0x3BEE80)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    if not ok:
        return 0
    m.mem.wn(m.g32(5), 4, 0x38201000)
    m.mem.wn(m.g32(5) + 4, 4, (m.g32(4) >> 3) % 4)
    return 1


def _mk_py_item(addr, base):
    def impl(m):
        cid, n, ok = _ok(m, addr)
        m.ilog.extend([cid, m.g32(4), 0, 0, 0])
        return (base + 0x100 * n) if ok else 0
    return impl


C_EFD20 = ("void func_003efd20(unsigned o, unsigned t) { p4_record({ID}, o, t, 0, 0); }")


def _py_fd20(m):
    cid = m.callees.index(0x3EFD20)
    m.ilog.extend([cid, m.g32(4), m.g32(5), 0, 0])


EXTRA_F20 = {"c_impls": {**C_F20, 0x3EFD20: C_EFD20, 0x3BF330: "void func_003bf330(void) { }", 0x3BF200: "void func_003bf200(void) { }", 0x3BF240: "void func_003bf240(void) { }"},
             "impls": {0x3DF050: _py_f050, 0x3E2910: _py_f2910, 0x3E5DF0: _py_f5df0, 0x3BEE80: _py_fee80,
                       0x3BF5F0: _mk_py_item(0x3BF5F0, 0x38300000), 0x3C3AE0: _mk_py_item(0x3C3AE0, 0x38400000),
                       0x3E53B0: _mk_py_item(0x3E53B0, 0x38500000), 0x3EFD20: _py_fd20},
             "reader": {0x3DF360: "word"}}


def gen_0f20(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00887000)
    for pg in (0x38200000, 0x38201000, 0x38300000, 0x38400000, 0x38500000):
        img.touch(pg)
    for i in range(0, 4096, 4):
        img.w32(0x38200000 + i, 0x38300000 + rng.randrange(0, 0x400))
        img.w32(0x38201000 + i, 0x39000000 + 0x10 * (i // 4))
    img.w32(0x007647A4, rng.choice([0, 8]))
    img.w32(0x007647A0, rng.choice([0, 8]))
    img.w32(0x007647B0, rng.choice([0, 4]))
    img.w32(0x008872E0 + img.r32(0x007647B0) + 4, rng.randrange(1 << 32))
    img.hook(0x008873FC, 4)
    img.hook(0x008873EC, 4)
    clump = img.alloc(0x100)
    for off in (8, 0x10, 0x18):
        img.w32(clump + off, clump + off)
    img.w32(clump + 4, 0)
    img.callee_ret[0x3C0640] = rng.choice([0, clump, clump, clump])
    n = rng.randrange(0, 40)
    for a in (0x3DF050, 0x3E2910, 0x3E5DF0, 0x3BEE80, 0x3BF5F0, 0x3C3AE0, 0x3E53B0, 0x3DF360):
        img.callee_ret[a] = 0xFE000000 | rng.randrange(0, 12 if a != 0x3DF050 else 40)
    img.callee_ret[0x3E3110] = rng.choice([0, 1, 1])
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | rng.randrange(16) << 4 & 0xF0], img, ret=True)


def gen_ee80(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    out = img.alloc(16)
    img.w32(out, rng.randrange(1 << 32))
    img.w32(out + 4, rng.randrange(1 << 32))
    img.callee_ret[0x3DF050] = 0xFE000000 | rng.randrange(0, 8)
    img.callee_ret[0x3E2910] = rng.choice([4, 4, 4, 0])
    img.callee_ret[0x3C30B0] = 0xFE000000 | rng.randrange(0, 8)
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | (rng.randrange(16) << 4 & 0xF0), out], img, ret=True)


def gen_key2(rng):
    img = rn.Img(rng)
    a = img.alloc(0x30, fill_random=False)
    b = img.alloc(0x30, fill_random=False)
    img.fill_f(a, 12, -3, 3)
    img.fill_f(b, 12, -3, 3)
    return rn.Case([a, b], img, ret=False)


def gen_key3(rng):
    img = rn.Img(rng)
    d = img.alloc(0x30, fill_random=False)
    a = img.alloc(0x30, fill_random=False)
    b = img.alloc(0x30, fill_random=False)
    img.fill_f(a, 12, -3, 3)
    img.fill_f(b, 12, -3, 3)
    return rn.Case([d, a, b], img, ret=False)


C_FE90 = ("unsigned func_0038fe90(unsigned s, unsigned w, unsigned v, unsigned *box, unsigned *done) { unsigned h = 0, i; "
          "for (i = 0; i < 6; i++) h = h * 31u + box[i]; unsigned r = p4_record({ID}, s, w, v, h); *done = (s >> 3) & 1u; return r; }")


def _py_fe90(m):
    cid = m.callees.index(0x38FE90)
    box = m.g32(7)
    h = 0
    for i in range(6):
        h = (h * 31 + m.mem.rn(box + 4 * i, 4)) & 0xFFFFFFFF
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), h])
    m.mem.wn(m.g32(8), 4, (m.g32(4) >> 3) & 1)
    return m.cret.get(0x38FE90, 0)


def gen_0350(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x100, fill_random=False)
    img.mem.wb(obj, rng.choice([8, 8, 0, 3]))
    sph = img.alloc(0x20, fill_random=False)
    img.fill_f(sph, 8, -4, 4)
    img.w32(obj + 0x5C, sph)
    img.fill_f(obj + 0x60, 6, -4, 4)
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 4)
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1])
    img.callee_ret[0x38FE90] = rng.choice([0, 0x1234])
    img.callee_ret[0x38FD30] = rng.choice([0, 0x4321])
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | rng.randrange(16), rng.randrange(1 << 32), obj, rng.randrange(0, 0x40, 4) + 0x80], img, ret=True)


def gen_90a0(rng):
    img = rn.Img(rng)
    gp_page(img)
    cell = img.alloc(0x40, fill_random=False)
    img.w32(0x007646D0, 0x10)
    rec = img.alloc(0x100, fill_random=False)
    img.w32(cell + 0x10, rng.choice([0, rec, rec, rec]))
    img.w32(rec + 0x80, rng.choice([0, 1, 5]))
    for k in range(2):
        e = rec + 64 * k
        img.w32(e + 0x20, rng.choice([0, 1, 2, 3, 4, 5]))
        for off in (0, 4, 8):
            img.w32(e + off, rng.choice([0, 0x6000 + off + 16 * k]))
    img.callee_ret[0x3E6240] = rng.randrange(0, 0x1000)
    return rn.Case([cell], img, ret=True)


# ---- hash-logging stand-ins: void callees whose pointer argument's contents matter
def _mk_hash_c(addr, nargs, hp, k):
    params = ", ".join("unsigned a%d" % i for i in range(nargs))
    others = [i for i in range(1, nargs) if i != hp]
    fold = " ".join("x = x * 31u + a%d;" % i for i in others)
    return ("void func_%08x(%s) { unsigned *q = (unsigned *)a%d; unsigned h = 0, x = 0, i; for (i = 0; i < %d; i++) h = h * 31u + q[i]; %s "
            "p4_record({ID}, a0, h, x, 0); }" % (addr, params, hp, k, fold))


def _mk_hash_py(addr, nargs, hp, k):
    others = [i for i in range(1, nargs) if i != hp]

    def impl(m):
        cid = m.callees.index(addr)
        q = m.g32(4 + hp)
        h = 0
        for i in range(k):
            h = (h * 31 + m.mem.rn(q + 4 * i, 4)) & 0xFFFFFFFF
        x = 0
        for i in others:
            x = (x * 31 + m.g32(4 + i)) & 0xFFFFFFFF
        m.ilog.extend([cid, m.g32(4), h, x, 0])
    return impl


def _hash_extra(spec):
    """spec: {addr: (nargs, hash-arg index, words)}"""
    return ({a: _mk_hash_c(a, *v) for a, v in spec.items()}, {a: _mk_hash_py(a, *v) for a, v in spec.items()})


_H4 = {0x390680: (4, 1, 4), 0x391400: (4, 1, 4), 0x391C10: (5, 1, 4), 0x394C70: (5, 1, 4)}
_H4C, _H4P = _hash_extra(_H4)
_HE = {0x3941E0: (6, 2, 8), 0x392BF0: (5, 1, 8)}
_HEC, _HEP = _hash_extra(_HE)

C_E0960 = "void func_003e0960(unsigned *m, unsigned *s) { unsigned i; p4_record({ID}, 0, (unsigned)s, 0, 0); for (i = 0; i < 16; i++) m[i] = s[i] * 3u + i; }"
C_E42E0 = "void func_003e42e0(unsigned *t, unsigned *a, unsigned n, unsigned *m) { unsigned i; p4_record({ID}, 0, (unsigned)a, n, 0); for (i = 0; i < 8; i++) t[i] = (m[i] ^ a[i]) + n; }"


def _py_e0960(m):
    cid = m.callees.index(0x3E0960)
    mm, s = m.g32(4), m.g32(5)
    m.ilog.extend([cid, 0, s, 0, 0])
    for i in range(16):
        m.mem.wn(mm + 4 * i, 4, (m.mem.rn(s + 4 * i, 4) * 3 + i) & 0xFFFFFFFF)


def _py_e42e0(m):
    cid = m.callees.index(0x3E42E0)
    t, a, n, mm = m.g32(4), m.g32(5), m.g32(6), m.g32(7)
    m.ilog.extend([cid, 0, a, n, 0])
    for i in range(8):
        m.mem.wn(t + 4 * i, 4, (((m.mem.rn(mm + 4 * i, 4) ^ m.mem.rn(a + 4 * i, 4)) & 0xFFFFFFFF) + n) & 0xFFFFFFFF)


def gen_dispatch(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    obj = img.alloc(0x100)
    ent = img.alloc(0x100)
    sph = img.alloc(0x20, fill_random=False)
    img.fill_f(sph, 8, -4, 4)
    mat = img.alloc(0x40)
    img.w32(obj + 0x18, rng.choice([0, 1, 2, 3, 4, 5, 5, 3, 1]))
    img.w32(obj, rng.randrange(1 << 32))
    img.w32(ent + 0x18, rng.choice([0, 1, 2, 3]))
    img.callee_ret[0x3BFAE0] = sph
    img.callee_ret[0x3E9700] = mat
    return rn.Case([(rng.randrange(1 << 32)), obj, rng.randrange(1 << 32), rng.randrange(1 << 32)], img, ret=True)


def gen_dispatch2(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    rec = img.alloc(0x200)
    obj = img.alloc(0x100)
    ent = img.alloc(0x100)
    sph = img.alloc(0x20, fill_random=False)
    img.fill_f(sph, 8, -4, 4)
    mat = img.alloc(0x40)
    img.w32(obj + 0x18, rng.choice([0, 1, 2, 3, 4, 5, 5, 3, 1, 1]))
    img.w32(rec + 0x18, ent)
    img.w32(ent + 0x18, rng.choice([0, 1, 2, 3]))
    img.callee_ret[0x3BFAE0] = sph
    img.callee_ret[0x3E9700] = mat
    return rn.Case([rec, obj, rng.randrange(1 << 32), rng.randrange(1 << 32)], img, ret=True)


def gen_9bf0(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00884000)
    img.touch(0x00887000)
    img.hook(0x008873F8, 3)
    img.w32(0x007646D0, 0x10)
    img.w32(0x00884A80, rng.randrange(1 << 32))
    cell = img.alloc(0x40, fill_random=False)
    if rng.random() < 0.6:
        rec = img.alloc(0x100)
        img.w32(rec + 0x80, rng.choice([0, 1, 2, 3, 4, 5, 6, 7]))
        img.w32(cell + 0x10, rec)
    else:
        img.w32(cell + 0x10, 0)
    return rn.Case([cell, rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 1, 4])], img, ret=True)


def gen_5290(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    nslot = rng.randrange(0, 4)
    obj = img.alloc(0x40, fill_random=False)
    tab = img.alloc(0x40, fill_random=False)
    img.w32(obj + 4, tab)
    for s in range(3):
        dst = img.alloc(0x100)
        img.w32(tab + 8 * s + 4, dst)
    img.callee_ret[0x394FD0] = rng.choice([0, obj, obj])
    img.callee_ret[0x395100] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 30)
    img.callee_ret[0x3DF300] = rng.choice([0, 1, 1, 1, 1])
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


# ---- generic stub factory: logs its int args (or int arg + float bits), writes deterministic words through
# `outs` pointer args, returns the callee_ret value (int), a float, or nothing
def _mk_stub(addr, nint, ret="int", farg=False, outs=()):
    nint0 = nint
    ar = rn.live_arity(addr)
    if ar is not None and not farg and ar > nint:
        nint = min(ar, 4)        # every argument register the retail callee reads is logged (a stack address is not: it differs between the interpreter and C)
    ps = ["unsigned a%d" % i for i in range(nint)] + (["float f"] if farg else [])
    rt = {"int": "unsigned", "float": "float", "void": "void"}[ret]
    rec = ["0" if i in outs else ("a%d" % i if i < nint0 else "(a%d >= 0xFF000000u ? 0u : a%d)" % (i, i)) if i < nint else "0" for i in range(4)]
    if farg:
        rec = ["a0", "*(unsigned *)&f", "0", "0"]
    wr = " ".join("*(unsigned *)a%d = a0 * 3u + %du + n * 7u;" % (k, j) for j, k in enumerate(outs))
    body = "unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, %s); %s (void)r; " % (", ".join(rec), wr)
    if ret == "int":
        body += "return r;"
    elif ret == "float":
        body += "return (float)(a0 & 0xFFu) * 0.5f + (float)n;"
    csrc = "%s func_%08x(%s) { %s }" % (rt, addr, ", ".join(ps) if ps else "void", body)

    def impl(m):
        cid, n, ok = _ok(m, addr)
        a = [(m.g32(4 + i) if (i < nint0 or m.g32(4 + i) < 0xFF000000) else 0) if i < nint and i not in outs else 0 for i in range(4)]
        if farg:
            a = [m.g32(4), m.f[12], 0, 0]
        m.ilog.extend([cid] + a)
        for j, k in enumerate(outs):
            m.mem.wn(m.g32(4 + k), 4, (m.g32(4) * 3 + j + n * 7) & 0xFFFFFFFF)
        if ret == "float":
            import mips as _m
            m.f[0] = _m.f2b(_m.b2f(_m.f2b((m.g32(4) & 0xFF) * 0.5 + n)))
            return 0
        return ok if ret == "int" else 0
    return csrc, impl


def _stubs(spec):
    cs, im = {}, {}
    for addr, args in spec.items():
        cs[addr], im[addr] = _mk_stub(addr, *args[:1], **(args[1] if len(args) > 1 else {}))
    return cs, im


C_DF360_BIT = ("unsigned func_003df360(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; *p = (s >> 6) & 1u; return 1; }")
C_DF550 = ("void func_003df550(unsigned *e) { static const unsigned V[4] = {0x80000000u, 0x16u, 5u, 0x1234u}; unsigned n = p4_ncalls[{ID}]; p4_record({ID}, 0, 0, 0, 0); e[0] = 2; e[1] = V[n % 4]; }")
C_DF4D0E = ("void func_003df4d0(unsigned *e) { p4_record({ID}, 0, e[0], e[1], 0); }")


def _py_df360_bit(m):
    cid, n, ok = _ok(m, 0x3DF360)
    m.ilog.extend([cid, m.g32(4), 0, m.g32(6), 0])
    if not ok:
        return 0
    m.mem.wn(m.g32(5), 4, (m.g32(4) >> 6) & 1)
    return 1


def _py_df550(m):
    cid = m.callees.index(0x3DF550)
    n = m.ncalls.get(cid, 0)
    m.ncalls[cid] = n + 1
    m.ilog.extend([cid, 0, 0, 0, 0])
    e = m.g32(4)
    m.mem.wn(e, 4, 2)
    m.mem.wn(e + 4, 4, [0x80000000, 0x16, 5, 0x1234][n % 4])


def _py_df4d0e(m):
    cid = m.callees.index(0x3DF4D0)
    e = m.g32(4)
    m.ilog.extend([cid, 0, m.mem.rn(e, 4), m.mem.rn(e + 4, 4), 0])


def gen_8890(rng):
    img = rn.Img(rng)
    out = img.alloc(0x10)
    s = (rng.randrange(1 << 32) & ~0xF) | rng.randrange(16) << 4 & 0xF0
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 3)
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1])
    img.callee_ret[0x3E6430] = rng.choice([0, 0x7000, 0x7000])
    img.callee_ret[0x3DF550] = 0
    return rn.Case([s, out], img, ret=True)


def _rand_name(rng, img, base, maxlen=44):
    n = rng.randrange(0, maxlen)
    for i in range(0x40):
        img.mem.wb(base + 0x10 + i, 0)
    for i in range(n):
        img.mem.wb(base + 0x10 + i, rng.randrange(1, 256))


def gen_9470(rng):
    img = rn.Img(rng)
    out = img.alloc(0x80, fill_random=False)
    a = img.alloc(0x80, fill_random=False)
    b = img.alloc(0x80, fill_random=False)
    _rand_name(rng, img, a)
    _rand_name(rng, img, b)
    return rn.Case([out, 0 if rng.random() < 0.2 else a, b], img, ret=False)


def gen_beb0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00884000)
    img.touch(0x00887000)
    frame = img.alloc(0x40)
    holder = img.alloc(0x40)
    img.w32(holder + 4, frame)
    img.w32(0x008872E0, holder)
    stamp = rng.randrange(0, 0x10000)
    img.mem.wb(0x008872E8, stamp & 0xFF)
    img.mem.wb(0x008872E9, stamp >> 8)
    rec = img.alloc(0x40)
    img.mem.wb(rec, rng.choice([0xFF, 0xFF, 0, 3]))
    a2 = img.alloc(0x40)
    img.w32(a2 + 8, rec)
    arg0 = rng.choice([0, frame, img.alloc(0x40)])
    eff = arg0 or frame
    hit = rng.random() < 0.4
    img.w32(0x007646F4, rec if hit else rng.randrange(1 << 32))
    img.w32(0x007646F0, eff if hit else rng.randrange(1 << 32))
    img.w32(0x007646F8, stamp if rng.random() < 0.8 else rng.randrange(1 << 32))
    img.w32(0x007646FC, 0 if rng.random() < 0.9 else rng.randrange(1 << 32))
    for i in range(0, 0x40, 4):
        img.w32(0x00884AE0 + i, rng.randrange(1 << 32))
    mat = img.alloc(0x40)
    img.callee_ret[0x3E9700] = mat
    out = img.alloc(0x40, fill_random=False)
    return rn.Case([arg0, out, a2], img, ret=True)


C_E05F0 = "void func_003e05f0(unsigned *d, unsigned *a, unsigned *b) { unsigned i; p4_record({ID}, 0, (unsigned)a, 0, 0); for (i = 0; i < 16; i++) d[i] = a[i] * 5u + b[i]; }"
C_E0670 = "void func_003e0670(unsigned *d, unsigned *s) { unsigned i; p4_record({ID}, (unsigned)d, (unsigned)s, 0, 0); for (i = 0; i < 16; i++) d[i] = s[i] ^ 0x5A5A5A5Au; }"


def _py_e05f0(m):
    cid = m.callees.index(0x3E05F0)
    d, a, b = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, 0, a, 0, 0])
    for i in range(16):
        m.mem.wn(d + 4 * i, 4, (m.mem.rn(a + 4 * i, 4) * 5 + m.mem.rn(b + 4 * i, 4)) & 0xFFFFFFFF)


def _py_e0670(m):
    cid = m.callees.index(0x3E0670)
    d, s = m.g32(4), m.g32(5)
    m.ilog.extend([cid, d, s, 0, 0])
    for i in range(16):
        m.mem.wn(d + 4 * i, 4, m.mem.rn(s + 4 * i, 4) ^ 0x5A5A5A5A)


def gen_9db0(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    gp_page(img)
    img.w32(0x007646D0, 0x10)
    rec = img.alloc(0x100, fill_random=False)
    cell = img.alloc(0x40, fill_random=False)
    img.w32(cell + 0x10, rec)

    def tex(zero_word):
        q = img.alloc(0x40, fill_random=False)
        img.w32(q + 0xC, 0 if zero_word else rng.randrange(1, 100))
        t = img.alloc(0x80, fill_random=False)
        img.w32(t, q)
        img.w32(t + 0x54, rng.randrange(0, 10))
        return t

    def holder_obj():
        q = img.alloc(0x40, fill_random=False)
        img.w32(q + 0xC, rng.randrange(1, 100))
        p = img.alloc(0x40, fill_random=False)
        img.w32(p, q)
        return p
    for k in range(2):
        e = rec + 64 * k
        img.w32(e + 0x20, rng.choice([1, 1, 1, 0, 2]))
        img.w32(e + 4, rng.choice([0, tex(False)]))
        img.w32(e + 8, rng.choice([0, tex(False)]))
    img.w32(rec + 0x20, 1) if rng.random() < 0.5 else img.w32(rec + 0x60, 1)
    arg1_none = rng.random() < 0.2
    # arg0[0]: either 0 or a holder whose [0] is a struct with +0xC nonzero (zero sometimes)
    mode = rng.choice([0, 1, 1, 2])
    if arg1_none:
        mode = 1
    if mode:
        q = img.alloc(0x40, fill_random=False)
        img.w32(q + 0xC, 0 if mode == 2 else rng.randrange(1, 100))
        p = img.alloc(0x40, fill_random=False)
        img.w32(p, q)
        img.w32(cell, p)
    else:
        img.w32(cell, 0)
    arg1 = 0
    if not arg1_none:
        x = img.alloc(0x80, fill_random=False)
        q = img.alloc(0x40, fill_random=False)
        img.w32(q + 0xC, 0 if rng.random() < 0.2 else rng.randrange(1, 100))
        img.w32(x, q)
        img.w32(x + 0x54, rng.randrange(0, 5))
        arg1 = x
    tt = img.alloc(0x80, fill_random=False)
    q2 = img.alloc(0x40, fill_random=False)
    img.w32(q2 + 0xC, rng.randrange(1, 100))
    img.w32(tt, q2)
    img.w32(tt + 0x54, rng.randrange(0, 5))
    img.callee_ret[0x3EF650] = rng.choice([0, tt])
    img.callee_ret[0x39AB10] = rng.choice([0, tt, tt])
    img.callee_ret[0x3EF6D0] = rng.choice([0, 0x5000])
    return rn.Case([cell, arg1], img, ret=True)


def gen_85b0(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00884000)
    img.touch(0x00887000)
    img.w32(0x007646D0, 0x10)
    img.w32(0x00884A80, rng.randrange(1 << 32))
    img.hook(0x008873F8, 3)
    src_rec = img.alloc(0x100, fill_random=False) if rng.random() < 0.9 else 0
    if src_rec:
        img.w32(src_rec + 0x80, rng.randrange(0, 8))
        for k in range(2):
            e = src_rec + 64 * k
            img.w32(e + 0x20, rng.choice([0, 1, 2, 4, 5, 3, 1]))
            for off in (4, 8):
                if rng.random() < 0.6:
                    o = img.alloc(0x80, fill_random=False)
                    img.w32(o + 0x54, rng.randrange(0, 9))
                    img.w32(e + off, o)
                else:
                    img.w32(e + off, 0)
    dst_rec = img.alloc(0x100) if rng.random() < 0.5 else 0
    c1 = img.alloc(0x40, fill_random=False)
    c2 = img.alloc(0x40, fill_random=False)
    img.w32(c1 + 0x10, dst_rec)
    img.w32(c2 + 0x10, src_rec)
    for a in (0x39A760, 0x39A700, 0x39A630, 0x39A340, 0x39A3A0, 0x39A400, 0x39A090, 0x399BF0):
        img.callee_ret[a] = rng.choice([0, rng.randrange(1, 1 << 20)])
    return rn.Case([c1, c2], img, ret=True)


def gen_c000(rng):
    img = rn.Img(rng)
    nv = 20
    n = rng.randrange(-2, 30)
    idx = img.alloc(max(n, 1) * 2 + 16, fill_random=False)
    for i in range(max(n, 0)):
        img.mem.wb(idx + 2 * i, 0)
        img.mem.wb(idx + 2 * i + 1, 0)
        v = rng.randrange(nv)
        img.mem.wb(idx + 2 * i, v & 0xFF)
    desc = img.alloc(0x10, fill_random=False)
    img.w32(desc, idx)
    img.w32(desc + 4, n & 0xFFFFFFFF)
    verts = img.alloc(nv * 16 + 16)
    src = img.alloc(0x40, fill_random=False)
    img.w32(src + 0x18, verts)
    dst = img.alloc(max(n, 1) * 16 + 16, fill_random=False)
    return rn.Case([desc, src, dst], img, ret=False)


def gen_29f0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007649C0, rng.randrange(1 << 32))
    img.w32(0x007649C4, rng.randrange(1 << 32))
    r = img.alloc(0x100)
    a0 = img.alloc(0x80, fill_random=False)
    img.w32(a0 + 0x4C, r)
    t = img.alloc(0x40, fill_random=False)
    img.mem.wb(t + 0x23, rng.randrange(256))
    img.w32(t + 0x14, rng.randrange(0, 20))
    a1 = img.alloc(0x80, fill_random=False)
    img.w32(a1, t)
    img.mem.wb(a1 + 0x50, rng.randrange(256))
    img.callee_ret[0x40F570] = rng.choice([0, 1, 1])
    return rn.Case([a0, rng.choice([0, a1, a1, a1])], img, ret=True)


def gen_c360(rng):
    img = rn.Img(rng)
    img.touch(0x00886000)
    n = rng.randrange(1, 5)
    out = img.alloc(0x40 * 6 + 16)
    img.w32(0x0088649C, out)
    img.w32(0x00886494, 0x10)
    a0 = img.alloc(0x40, fill_random=False)
    img.w32(a0 + 4, rng.randrange(1 << 32))
    flags = rng.choice([0, 2, 0x4000, 0x4002, 1, 0x4001, 3])
    skin = rng.random() < 0.85
    if skin:
        rec = img.alloc(0x40, fill_random=False)
        img.w32(rec, flags)
        img.w32(rec + 4, n)
        S = img.alloc(n * 0x40 + 16, fill_random=False)
        img.fill_f(S, n * 16, -2, 2)
        img.w32(rec + 8, S)
        ent = img.alloc(n * 16 + 16)
        img.w32(rec + 0x10, ent)
        img.w32(a0 + 0x10, rec)
    else:
        img.w32(a0 + 0x10, 0)
    a1 = img.alloc(0x40, fill_random=False)
    img.w32(a1, rng.randrange(0, 5))
    B = img.alloc(n * 0x40 + 16, fill_random=False)
    img.fill_f(B, n * 16, -2, 2)
    img.callee_ret[0x3B8510] = B
    mat = img.alloc(0x40, fill_random=False)
    img.fill_f(mat, 16, -2, 2)
    img.callee_ret[0x3E9700] = mat
    return rn.Case([a0, a1], img, ret=True)


def gen_8970(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007646D0, 0x10)
    rec = img.alloc(0x100, fill_random=False)
    cell = img.alloc(0x40, fill_random=False)
    img.w32(cell + 0x10, rec)
    img.w32(rec + 0x80, rng.randrange(1 << 32))
    for k in range(2):
        e = rec + 64 * k
        img.w32(e + 0x20, rng.choice([0, 1, 2, 3, 4, 5, 1, 2, 4]))
        for off in (0, 4, 8, 0xC):
            img.w32(e + off, rng.choice([0, rng.randrange(1, 1 << 32)]))
    img.callee_ret[0x3DF240] = 0xFE000000 | rng.randrange(0, 14)
    img.callee_ret[0x3DF1A0] = 0xFE000000 | rng.randrange(0, 5)
    img.callee_ret[0x3E62B0] = 0xFE000000 | rng.randrange(0, 5)
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | (rng.randrange(16) << 4 & 0xF0), rng.randrange(1 << 32), cell], img, ret=True)


C_RD_TYPE = ("unsigned func_003df360(unsigned s, unsigned *p, unsigned n) { unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; "
             "if (n == 8) { p[0] = (s >> (3 * k)) + k; p[1] = k * 77u + 5u; } else p[0] = ((s >> (3 * k)) ^ k) % 6u; return 1; }")
C_RD_F = ("unsigned func_003df300(unsigned s, unsigned *p, unsigned n) { unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; *p = 0x3F800000u + 0x10000u * k; return 1; }")
C_RD_OBJ = ("unsigned func_00398890(unsigned s, unsigned *p) { unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, 0, 0); if (!r) return 0; "
            "*p = ((s >> (5 + k)) & 1u) ? 0x38600000u + 0x100u * k : 0u; return s; }")


def _py_rd_type(m):
    cid, n, ok = _ok(m, 0x3DF360)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if not ok:
        return 0
    if nn == 8:
        m.mem.wn(p, 4, ((s >> (3 * n)) + n) & 0xFFFFFFFF)
        m.mem.wn(p + 4, 4, (n * 77 + 5) & 0xFFFFFFFF)
    else:
        m.mem.wn(p, 4, (((s >> (3 * n)) ^ n) & 0xFFFFFFFF) % 6)
    return 1


def _py_rd_f(m):
    cid, n, ok = _ok(m, 0x3DF300)
    m.ilog.extend([cid, m.g32(4), 0, m.g32(6), 0])
    if not ok:
        return 0
    m.mem.wn(m.g32(5), 4, (0x3F800000 + 0x10000 * n) & 0xFFFFFFFF)
    return 1


def _py_rd_obj(m):
    cid, n, ok = _ok(m, 0x398890)
    s = m.g32(4)
    m.ilog.extend([cid, s, 0, 0, 0])
    if not ok:
        return 0
    m.mem.wn(m.g32(5), 4, (0x38600000 + 0x100 * n) if ((s >> (5 + n)) & 1) else 0)
    return s


_C98D, _P98D = _stubs({0x39A030: (1, {"ret": "void", "farg": True}), 0x39A2E0: (1, {"ret": "void", "farg": True}), 0x39A150: (2, {"ret": "void"}),
                       0x39A260: (2, {"ret": "void"}), 0x39A4C0: (2, {"ret": "void"}), 0x39A590: (3,), 0x3EF3A0: (1, {"ret": "void"}),
                       0x399DB0: (2,), 0x399BF0: (2,)})
_C98D.update({0x3DF360: C_RD_TYPE, 0x3DF300: C_RD_F, 0x398890: C_RD_OBJ})
_P98D.update({0x3DF360: _py_rd_type, 0x3DF300: _py_rd_f, 0x398890: _py_rd_obj})


def gen_8d20(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00884000)
    img.touch(0x00887000)
    img.hook(0x008873F8, 3)
    img.w32(0x007646D0, 0x10)
    img.w32(0x00884A80, rng.randrange(1 << 32))
    for j in range(8):
        base = 0x38600000 + 0x100 * j
        img.touch(base)
    img.touch(0x38700000)
    for j in range(8):
        img.w32(0x38600000 + 0x100 * j, 0x38700000 + 0x40 * j)
        img.w32(0x38700000 + 0x40 * j + 0xC, rng.randrange(1, 50))
    rec = img.alloc(0x100, fill_random=False) if rng.random() < 0.6 else 0
    cell = img.alloc(0x40, fill_random=False)
    img.w32(cell + 0x10, rec)
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 6)
    img.callee_ret[0x3DF300] = 0xFE000000 | rng.randrange(0, 4)
    img.callee_ret[0x398890] = 0xFE000000 | rng.randrange(0, 6)
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | (rng.randrange(16) << 4 & 0xF0), rng.randrange(1 << 32), cell], img, ret=True)


C_D9760 = ("unsigned func_003d9760(unsigned pr, unsigned *v0, unsigned *v1, unsigned *v2, unsigned *hit, float *dist) { unsigned k = p4_ncalls[{ID}]; "
           "unsigned r = p4_record({ID}, pr, (unsigned)v0, (unsigned)v1, (unsigned)v2); if (!r) return 0; hit[0] = v1[0]; hit[1] = v1[1]; hit[2] = v2[2]; "
           "*dist = 0.25f * (float)(k + 1); return 1; }")


def _py_d9760(m):
    cid, n, ok = _ok(m, 0x3D9760)
    import mips as _m
    v0, v1, v2, hit, dist = m.g32(5), m.g32(6), m.g32(7), m.g32(8), m.g32(9)
    m.ilog.extend([cid, m.g32(4), v0, v1, v2])
    if not ok:
        return 0
    m.mem.wn(hit, 4, m.mem.rn(v1, 4))
    m.mem.wn(hit + 4, 4, m.mem.rn(v1 + 4, 4))
    m.mem.wn(hit + 8, 4, m.mem.rn(v2 + 8, 4))
    m.mem.wn(dist, 4, _m.f2b(0.25 * (n + 1)))
    return 1


def gen_2a50(rng):
    img = rn.Img(rng)
    n = rng.randrange(1, 6)
    nv = 12
    verts = img.alloc(nv * 12 + 16, fill_random=False)
    img.fill_f(verts, nv * 3, -5, 5)
    mesh = img.alloc(0x80, fill_random=False)
    img.w32(mesh + 0x14, verts)
    idx = img.alloc(n * 8 + 16, fill_random=False)
    for i in range(n):
        for j in range(3):
            v = rng.randrange(nv)
            img.mem.wb(idx + 8 * i + 2 * j, v)
            img.mem.wb(idx + 8 * i + 2 * j + 1, 0)
    obj = img.alloc(0x80, fill_random=False)
    img.w32(obj + 0x5C, mesh)
    img.w32(obj + 0x10, n)
    img.w32(obj + 0x2C, idx)
    probe = img.alloc(0x40, fill_random=False)
    img.fill_f(probe, 16, 0.5, 3.0)
    img.callee_ret[0x3D9760] = 0xFD000000 | rng.randrange(1 << 8)
    img.callee_ret[0x500007] = 0xFE000000 | rng.randrange(0, 5)
    return rn.Case([obj, probe, rng.randrange(1 << 32), 0x500007, rng.randrange(1 << 32)], img, ret=True)


def gen_55a0(rng):
    import mips as _m
    img = rn.Img(rng)
    a1 = img.alloc(0x30, fill_random=False)
    a2 = img.alloc(0x30, fill_random=False)
    img.fill_f(a1, 12, -2, 2)
    img.fill_f(a2, 12, -2, 2)
    out = img.alloc(0x30, fill_random=False)
    t1 = rng.uniform(0, 1)
    t2 = t1 + rng.uniform(0.1, 2)
    img.w32(a1 + 4, _m.f2b(t1))
    img.w32(a2 + 4, _m.f2b(t2))

    def unit():
        qs = [rng.uniform(-1, 1) for _ in range(4)]
        n = sum(q * q for q in qs) ** 0.5 or 1.0
        return [q / n for q in qs]
    q1 = unit()
    if rng.random() < 0.3:
        q2 = [q + rng.uniform(-0.02, 0.02) for q in q1]
        n = sum(q * q for q in q2) ** 0.5
        q2 = [q / n for q in q2]
    else:
        q2 = unit()
    for j in range(4):
        img.w32(a1 + 8 + 4 * j, _m.f2b(q1[j]))
        img.w32(a2 + 8 + 4 * j, _m.f2b(q2[j]))
    tbits = _m.f2b(rng.uniform(t1 - 0.2, t2 + 0.2))
    img.fregs = {12: tbits}
    return rn.Case([out, a1, a2, tbits], img, ret=False)


def gen_5bd0(rng):
    import mips as _m
    case = gen_55a0(rng)
    case.img.fregs = {12: _m.f2b(rng.uniform(-0.2, 1.2))}
    case.args[3] = case.img.fregs[12]
    return case


C_E05F0W = ("void func_003e05f0(unsigned *d, unsigned *a, unsigned *b) { unsigned i; unsigned hb = 0, ha = 0; for (i = 0; i < 16; i++) { hb = hb * 31u + b[i]; ha = ha * 31u + a[i]; } "
            "p4_record({ID}, 0, hb, ha, 0); for (i = 0; i < 16; i++) d[i] = a[i] * 5u + b[i]; }")


def _py_e05f0w(m):
    cid = m.callees.index(0x3E05F0)
    d, a, b = m.g32(4), m.g32(5), m.g32(6)
    hb = ha = 0
    for i in range(16):
        hb = (hb * 31 + m.mem.rn(b + 4 * i, 4)) & 0xFFFFFFFF
        ha = (ha * 31 + m.mem.rn(a + 4 * i, 4)) & 0xFFFFFFFF
    m.ilog.extend([cid, 0, hb, ha, 0])
    for i in range(16):
        m.mem.wn(d + 4 * i, 4, (m.mem.rn(a + 4 * i, 4) * 5 + m.mem.rn(b + 4 * i, 4)) & 0xFFFFFFFF)


def gen_7c40(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    flags = 0
    for bit in (1, 0x1000, 0x2000, 0x4000):
        if rng.random() < 0.5:
            flags |= bit
    if rng.random() < 0.05:
        flags |= 2
    n = rng.randrange(0, 5)
    head = img.alloc(0x40)
    img.w32(0x0088739C, head)
    h = img.alloc(0x40, fill_random=False)
    parent_h = img.alloc(0x40, fill_random=False)
    pmats = img.alloc(0x100, fill_random=True)
    img.w32(parent_h + 8, pmats)

    def frame_obj():
        f = img.alloc(0x120)
        img.mem.wb(f + 3, rng.randrange(256))
        rec = img.alloc(0x40, fill_random=False)
        img.mem.wb(rec + 3, rng.choice([0, 1, 2, 3, 0, 0]))
        img.w32(f + 0xA0, rec)
        return f

    def chain_obj():
        c0 = img.alloc(0x40, fill_random=False)
        s16 = img.alloc(0x40 + 0x40 * 4, fill_random=True)
        # child list: s16[4] -> node -> node ...
        prev = 0
        for _ in range(rng.randrange(0, 3)):
            nd = img.alloc(0x60, fill_random=True)
            img.w32(nd + 4, prev)
            prev = nd
        img.w32(s16 + 4, prev)
        img.w32(c0 + 4, s16 if rng.random() < 0.85 else 0)
        return c0
    root = chain_obj() if rng.random() < 0.9 else 0
    rootframe = frame_obj()
    img.w32(h, flags)
    img.w32(h + 4, n)
    mats = img.alloc(max(n, 1) * 64 + 16, fill_random=True)
    img.w32(h + 8, mats)
    info = img.alloc(max(n, 1) * 16 + 16, fill_random=False)
    depth = 0
    for i in range(n):
        mode = rng.choice([0, 1, 2, 3])
        if mode == 1 and depth == 0:
            mode = 2
        depth += 1 if mode == 2 else (-1 if mode == 1 else 0)
        img.w32(info + 16 * i + 8, (rng.randrange(1 << 30) << 2) | mode)
        img.w32(info + 16 * i + 0xC, rng.choice([0, frame_obj(), frame_obj()]))
    img.w32(h + 0x10, info)
    # +0x14: the root frame holder: its +4 is the chain head object ... retail reads h[0x14] then [4], [0xA0]
    holder = rootframe
    img.w32(holder + 4, 0)
    if root:
        img.w32(holder + 4, rng.choice([0, root, root]))
    img.w32(h + 0x14, holder)
    img.w32(parent_h + 0x14, holder)
    img.w32(h + 0x18, parent_h)
    img.w32(h + 0x1C, rng.choice([0xFFFFFFFF, 0, 1, 2]))
    interp = img.alloc(0x4C + max(n, 1) * 0x30 + 16, fill_random=True)
    img.w32(interp + 0x24, 0x30)
    img.hook(interp + 0x3C, 8)
    img.w32(h + 0x20, interp)
    img.callee_ret[0x3E9240] = rng.choice([0, 1, 1])
    mat = img.alloc(0x40)
    img.callee_ret[0x3E9700] = mat
    img.callee_ret[0x397480] = rng.randrange(1 << 16)
    return rn.Case([h], img, ret=True)


def gen_20e0(rng):
    img = rn.Img(rng)
    v = 0
    for bit in (1, 4, 8, 0x20, 0x40, 0x80, 0x100, 0x8000, 0x20000, 0x40000, 0x80000, 0x100000, 0x10000000, 0x20000000, 0x40000000):
        if rng.random() < 0.5:
            v |= bit
    return rn.Case([v | (rng.randrange(1 << 32) if rng.random() < 0.2 else 0)], img, ret=True)


def _gen_dc(axis):
    def g(rng):
        img = rn.Img(rng)
        m = img.alloc(0x40, fill_random=False)
        img.fill_f(m, 16, -0.5, 0.5)
        import mips as _m
        img.w32(m + 4 * (5 * axis), _m.f2b(rng.uniform(0.3, 3.0)))
        out = img.alloc(0x10, fill_random=False)
        return rn.Case([out, m], img, ret=True)
    return g


def gen_d390(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873D4, 6)
    s = img.alloc(0x80, fill_random=False)
    n = rng.randrange(0, 40)
    for i in range(0x80):
        img.mem.wb(s + i, 0)
    for i in range(n):
        img.mem.wb(s + i, 0x3A if rng.random() < 0.06 else rng.randrange(1, 256) if rng.random() > 0.1 else 0x61)
    return rn.Case([s], img, ret=True)


def gen_cc500(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x007649E0, rng.randrange(1 << 32))
    img.w32(0x007649E4, rng.randrange(1 << 32))
    return rn.Case([rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 0xFFFFFFFF])], img, ret=False)


def gen_7510(rng):
    img = rn.Img(rng)
    a0 = img.alloc(0x40)
    cnt = rng.randrange(0, 6)
    arr = img.alloc(max(cnt, 1) * 16 + 16, fill_random=False)
    for i in range(cnt * 4):
        img.w32(arr + 4 * i, rng.choice([0, 1, 1, 7]))
    return rn.Case([a0, arr, cnt], img, ret=False)


def gen_5750(rng):
    img = rn.Img(rng)
    stride = rng.choice([4, 8, 12, 16, 0x20])
    n = rng.randrange(1, 6)
    head = img.alloc(stride * (n + 2) + 32, fill_random=False)
    for k in range(1, n + 1):
        img.w32(head + stride * k, head + 4 if k < n else head)
    img.w32(head, rng.choice([0, 0x1234, head]) if rng.random() < 0.2 else 0x4321)
    x = img.alloc(0x20, fill_random=False)
    img.w32(x + 8, stride)
    a0 = img.alloc(0x40, fill_random=False)
    img.w32(a0, x)
    img.w32(a0 + 0x10, head)
    return rn.Case([a0], img, ret=True)


def gen_2010(rng):
    img = rn.Img(rng)
    gp_page(img)
    for pg in (0x0070B000, 0x00763000, 0x00887000):
        img.touch(pg)
    img.w32(0x007647E0, rng.choice([0, 0x10, 0x100]))
    off = img.r32(0x007647E0)
    a = img.alloc(0x40, fill_random=False)
    img.w32(a + 0x24, rng.randrange(1 << 32))
    img.w32(a + 0x28, rng.choice([0, rng.randrange(1, 1 << 32)]))
    img.w32(a + 0x2C, rng.randrange(1 << 32))
    for i in range(0, 0x100, 4):
        img.w32(0x008872E0 + off + i, rng.randrange(1 << 32))
    b = img.alloc(0x40, fill_random=False)
    return rn.Case([a, b], img, ret=False)


def gen_8070(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x0076483C, 0x10)
    cell = img.alloc(0x80, fill_random=False)
    table = img.alloc(0x80, fill_random=False)
    img.w32(cell + 0x10, table)
    # arg0 + offset gives the table pointer ADDRESS? retail: s16 = arg0 + off (an address), slot = s16 + idx*4 + 8
    obj = img.alloc(0x80, fill_random=False)
    anim = img.alloc(0x40, fill_random=False)
    hdr = img.alloc(0x20, fill_random=False)
    img.w32(anim, hdr)
    img.w32(hdr + 4, rng.randrange(1 << 32))
    node = img.alloc(0x40, fill_random=False)
    img.w32(anim + 0x14, node)
    img.w32(node + 0x40, rng.randrange(0, 100))
    img.callee_ret[0x3D5750] = rng.randrange(0, 20)
    img.callee_ret[0x3D5790] = rng.choice([0, 0x7777])
    img.callee_ret[0x3D7490] = rng.choice([0, 1, 1])
    return rn.Case([obj - 0x10 if False else obj, anim, rng.randrange(0, 4)], img, ret=True)


def gen_bea0(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873EC, 4)
    a = img.alloc(0x40, fill_random=False)
    arr = img.alloc(0x40, fill_random=False)
    n = rng.randrange(0, 5)
    for i in range(n):
        img.w32(arr + 4 * i, rng.choice([0, rng.randrange(1, 1 << 32)]))
    img.w32(a, rng.choice([0, rng.randrange(1, 1 << 32)]))
    img.w32(a + 4, rng.choice([3, 3, 1, 0]))
    ptr = rng.choice([0, arr, arr])
    img.w32(a + 8, n if ptr else 0)
    img.w32(a + 0xC, ptr)
    return rn.Case([a], img, ret=False)


C_B190 = ("void func_003db190(unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5) { "
          "p4_record({ID}, a0, a1, ((a2 * 31u + a3) * 31u + a4) * 31u + a5, 0); }")


def _py_b190(m):
    cid = m.callees.index(0x3DB190)
    a = [m.g32(4 + i) for i in range(6)]
    m.ilog.extend([cid, a[0], a[1], (((a[2] * 31 + a[3]) * 31 + a[4]) * 31 + a[5]) & 0xFFFFFFFF, 0])


def gen_b360(rng):
    img = rn.Img(rng)
    n = rng.randrange(0, 5)
    a = img.alloc(0x100, fill_random=True)
    ents = img.alloc(max(n, 1) * 8 + 16, fill_random=True)
    for i in range(n):
        img.mem.wb(ents + 8 * i + 6, rng.randrange(4))
        img.mem.wb(ents + 8 * i + 7, 0)
    img.w32(a + 4, ents)
    img.mem.wb(a + 0x84, n)
    img.mem.wb(a + 0x85, 0)
    b = img.alloc(0x40, fill_random=False)
    b1 = img.alloc(0x40, fill_random=False)
    arr = img.alloc(0x40, fill_random=True)
    img.w32(b1 + 0x10, arr)
    img.w32(b + 4, b1)
    img.w32(b + 8, rng.randrange(1 << 32))
    img.hook(b + 0x18, 0)
    img.callee_ret[0x500000] = 0xFD000000 | rng.randrange(0, 1 << 6)
    return rn.Case([a, b], img, ret=True)


def gen_7590(rng):
    img = rn.Img(rng)
    a0 = img.alloc(0x40, fill_random=False)
    cnt = rng.randrange(0, 5)
    img.w32(a0 + 0x10, cnt)
    n = rng.randrange(0, 5)
    a1 = img.alloc(max(n, 1) * 4 + 16)
    a2 = img.alloc(max(n, 1) * 16 + 16, fill_random=False)
    for i in range(n * 4):
        img.w32(a2 + 4 * i, rng.choice([0, 0, 1, 5]))
    for i in range(n):
        img.w32(a1 + 4 * i, rng.choice([rng.randrange(1 << 32), 0x01010101 * rng.randrange(256), 0x00010203]))
    a3 = img.alloc(0x40, fill_random=False)
    a4 = img.alloc(0x10)
    return rn.Case([a0, a1, a2, a3, a4, n], img, ret=False)


def gen_66c0(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873FC, 0)
    img.hook(0x008873EC, 4)
    n = rng.randrange(0, 5)
    pools = img.alloc(0x10, fill_random=False)
    img.w32(pools, rng.randrange(1 << 32))
    img.w32(pools + 4, rng.randrange(1 << 32))
    arr = img.alloc(max(n, 1) * 4 + 16, fill_random=False)
    for i in range(n):
        img.w32(arr + 4 * i, rng.randrange(1 << 32))
    prev = 0
    for _ in range(rng.randrange(0, 4)):
        nd = img.alloc(0x20, fill_random=False)
        img.w32(nd + 0xC, prev)
        prev = nd
    return rn.Case([n, pools, prev, arr], img, ret=True)


def gen_thunk(kind):
    def g(rng):
        img = rn.Img(rng)
        gp_page(img)
        img.touch(0x00887000)
        img.w32(0x007647F8, 0x10)
        obj = img.alloc(0x60, fill_random=False)
        rec = obj + 0x10
        img.w32(0x008872E4, rng.randrange(1 << 32))
        img.mem.wb(0x008872E8, rng.randrange(256))
        img.mem.wb(0x008872E9, rng.randrange(256))
        img.w32(rec + 0xC, rng.choice([0, 1, rng.randrange(1 << 32)]))
        for off in (0x10, 0x14, 0x18):
            img.hook(rec + off, 0)
        img.w32(0x00887380, 0)
        img.hook(0x00887380, 1)
        e = img.alloc(0x20, fill_random=False)
        img.w32(e + 0xC, rng.choice([0, 1, 2]))
        img.w32(rec + 0xC, rng.choice([0, e, e]))
        img.callee_ret[0x500000] = rng.choice([0, 1, 5, 0x1234])
        return rn.Case([obj, rng.randrange(1 << 32), rng.randrange(1 << 32), rng.randrange(1 << 32)], img, ret=True)
    return g


C_STRLEN2948 = "unsigned func_00442948(unsigned char *s) { unsigned n = 0; p4_record({ID}, (unsigned)0, 0, 0, 0); while (s[n]) n++; return n; }"


def _py_strlen2948(m):
    cid = m.callees.index(0x442948)
    m.ilog.extend([cid, 0, 0, 0, 0])
    p = m.g32(4)
    n = 0
    while m.mem.rb(p + n):
        n += 1
    return n


def gen_d430(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873AC, 0)
    dst = img.alloc(0x100, fill_random=False)
    d = img.alloc(0x40, fill_random=False)
    nm = img.alloc(0x40, fill_random=False)
    for i in range(0x40):
        img.mem.wb(d + i, 0)
        img.mem.wb(nm + i, 0)
    for i in range(rng.randrange(0, 12)):
        img.mem.wb(d + i, rng.randrange(1, 128))
    for i in range(rng.randrange(0, 20)):
        img.mem.wb(nm + i, rng.choice([0x2F, 0x5C, rng.randrange(1, 256)]))
    return rn.Case([dst, rng.choice([2, 8, 20, 40, 100]), d, nm, rng.randrange(256)], img, ret=True)


def gen_55f0(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00887000)
    img.hook(0x008873F8, 3)
    img.hook(0x008873E8, 2)
    img.hook(0x008873FC, 0)
    img.w32(0x007647D8, rng.randrange(1 << 32))
    return rn.Case([rng.choice([0, 0, 1, 2, 5, 0x100])], img, ret=True)


def gen_7b40(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x0070B000)
    img.touch(0x00887000)
    img.hook(0x008873F8, 3)
    img.w32(0x0070B650, rng.randrange(1 << 32))
    img.w32(0x0070B680, rng.randrange(1 << 32))
    img.w32(0x00764838, rng.randrange(1 << 32))
    name = img.alloc(0x40, fill_random=True)
    data = img.alloc(0x100, fill_random=True)
    s = img.alloc(0x40, fill_random=False)
    img.callee_ret[0x3D51C0] = rng.choice([0, s, s])
    return rn.Case([name, rng.randrange(0, 20), rng.randrange(1 << 32), data, rng.choice([0, 1, 2, 0, 1])], img, ret=True)


def gen_bc620(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873D4, 6)
    if rng.random() < 0.1:
        return rn.Case([0], img, ret=True)
    a = img.alloc(0x40, fill_random=False)

    def cstr():
        s = img.alloc(0x40, fill_random=False)
        for i in range(0x40):
            img.mem.wb(s + i, 0)
        for i in range(rng.randrange(0, 20)):
            img.mem.wb(s + i, rng.randrange(1, 256))
        return s
    img.w32(a, rng.choice([0, cstr()]))
    t = rng.choice([0, 1, 2, 3, 3, 4])
    img.w32(a + 4, t)
    n = rng.randrange(0, 5)
    img.w32(a + 8, n)
    arr = img.alloc(0x40, fill_random=False)
    for i in range(n):
        img.w32(arr + 4 * i, rng.choice([0, cstr()]))
    img.w32(a + 0xC, arr)
    return rn.Case([a], img, ret=True)


C_E2910_HDR4 = ("unsigned func_003e2910(unsigned s, unsigned short *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); if (r >= n) { p[0] = (unsigned short)((s >> 3) % 4u); p[1] = (unsigned short)((s >> 5) & 1u); } return r; }")
C_DB630 = ("void func_003db630(unsigned s, unsigned *p) { unsigned n = p4_ncalls[{ID}]; p4_record({ID}, s, 0, 0, 0); *p = 0x1000u + n; }")


def _py_e2910_hdr4(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if ok >= nn:
        m.mem.wn(p, 2, (s >> 3) % 4)
        m.mem.wn(p + 2, 2, (s >> 5) & 1)
    return ok


def _py_db630(m):
    cid, n, ok = _ok(m, 0x3DB630)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    m.mem.wn(m.g32(5), 4, 0x1000 + n)


def gen_c370(rng):
    img = rn.Img(rng)
    img.callee_ret[0x3E2910] = rng.choice([4, 4, 4, 3, 0])
    img.callee_ret[0x3EF0D0] = rng.choice([0, 0x1111, 0x2222])
    img.callee_ret[0x3EF6D0] = rng.randrange(1, 1 << 20)
    img.callee_ret[0x3DBE40] = 0xFE000000 | rng.randrange(0, 5)
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | rng.randrange(16) << 4 & 0xF0], img, ret=True)


def gen_bc740(rng):
    img = rn.Img(rng)
    img.touch(0x00887000)
    img.hook(0x008873EC, 4)
    a = img.alloc(0x20, fill_random=False)
    n = rng.randrange(0, 4)
    ents = img.alloc(max(n, 1) * 16 + 16, fill_random=False)
    img.w32(a, n)
    img.w32(a + 4, rng.choice([0, ents, ents]))

    def cstrs(k):
        arr = img.alloc(max(k, 1) * 4 + 16, fill_random=False)
        for i in range(k):
            img.w32(arr + 4 * i, rng.choice([0, rng.randrange(1, 1 << 32)]))
        return arr
    for i in range(n):
        e = ents + 16 * i
        img.w32(e, rng.choice([0, rng.randrange(1, 1 << 32)]))
        t = rng.choice([0, 1, 3, 3])
        img.w32(e + 4, t)
        k = rng.randrange(0, 4)
        img.w32(e + 8, k)
        img.w32(e + 0xC, rng.choice([0, cstrs(k)]) if t == 3 else rng.choice([0, rng.randrange(1, 1 << 32)]))
        if t == 3 and img.r32(e + 0xC) == 0:
            img.w32(e + 8, 0)
    return rn.Case([a], img, ret=False)


def gen_51c0(rng):
    img = rn.Img(rng)
    import mips as _m
    img.pool()
    gp_page(img)
    img.touch(0x00886000)
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    ntypes = rng.randrange(0, 5)
    img.w32(0x00764824, ntypes)
    keys = []
    for i in range(6):
        k = rng.randrange(1, 50)
        keys.append(k)
        img.w32(0x00886E50 + 0x30 * i, k)
        img.w32(0x00886E50 + 0x30 * i + 8, rng.randrange(0, 40))
        img.w32(0x00886E50 + 0x30 * i + 0x2C, rng.choice([0, 5, 64, 0xFFFFFFF0]))
    key = rng.choice(keys[:max(ntypes, 1)] + [999])
    fbits = _m.f2b(rng.uniform(-3, 3))
    img.fregs = {12: fbits}
    return rn.Case([key, rng.randrange(0, 12), rng.randrange(1 << 32), fbits], img, ret=True)


def gen_fff0(rng):
    img = rn.Img(rng)
    a0 = img.alloc(0x80, fill_random=True)
    m1 = img.alloc(0x40, fill_random=True)
    img.w32(a0 + 0x18, m1)
    if rng.random() < 0.15:
        img.w32(m1 + 8, img.r32(m1 + 8) | 0x1000000)
    else:
        img.w32(m1 + 8, img.r32(m1 + 8) & ~0x1000000)
    tex = img.alloc(0x20, fill_random=True)
    stage = img.alloc(0x80, fill_random=True)
    b = img.alloc(0x40, fill_random=True)
    sp = img.alloc(0x20, fill_random=True)
    img.w32(b + 0xC, tex)
    img.w32(b + 0x10, sp)
    img.w32(sp + 4, rng.choice([0, stage, stage]))
    key = (img.r32(m1 + 0x18) & 0xFF) << 24 | (img.r32(m1 + 8) & 0x9F) << 16 | (img.r32(tex + 4) >> 16 & 0xFFFF)
    img.w32(stage + 0x28, rng.choice([key, key ^ 0x10000, key ^ 0x1000000, rng.randrange(1 << 32), key ^ 1]))
    img.w32(a0 + 0x4C, rng.randrange(1 << 32))
    if rng.random() < 0.5:
        h6 = img.mem.rn(a0 + 0x52, 2)
        h4 = img.mem.rn(a0 + 0x50, 2)
        sx = lambda v: v - 0x10000 if v & 0x8000 else v
        img.w32(stage + 0x40, sx(h6) & 0xFFFFFFFF)
        img.w32(stage + 0x3C, sx(h4) & 0xFFFFFFFF)
    return rn.Case([a0, b], img, ret=False)


C_DED20 = ("unsigned func_003ded20(unsigned *r) { unsigned h = r[0] * 31u + r[1]; h = h * 31u + r[2]; h = h * 31u + r[3]; p4_record({ID}, 0, h, 0, 0); "
           "r[0] ^= 0x55u; r[2] += 3u; return 7; }")


def _py_ded20(m):
    cid = m.callees.index(0x3DED20)
    r = m.g32(4)
    w = [m.mem.rn(r + 4 * i, 4) for i in range(4)]
    h = (w[0] * 31 + w[1]) & 0xFFFFFFFF
    h = (h * 31 + w[2]) & 0xFFFFFFFF
    h = (h * 31 + w[3]) & 0xFFFFFFFF
    m.ilog.extend([cid, 0, h, 0, 0])
    m.mem.wn(r, 4, w[0] ^ 0x55)
    m.mem.wn(r + 8, 4, (w[2] + 3) & 0xFFFFFFFF)
    return 7


C_E2910_12 = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); if (r >= n) { p[0] = s * 3u; p[1] = s ^ 0x1234u; p[2] = ((s >> 2) & 1u) ? (s * 7u) : (s & 0xFFFFu); } return r; }")


def _py_e2910_12(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if ok >= nn:
        m.mem.wn(p, 4, (s * 3) & 0xFFFFFFFF)
        m.mem.wn(p + 4, 4, s ^ 0x1234)
        m.mem.wn(p + 8, 4, ((s * 7) & 0xFFFFFFFF) if (s >> 2) & 1 else (s & 0xFFFF))
    return ok


def gen_eea0(rng):
    img = rn.Img(rng)
    img.callee_ret[0x3E2910] = rng.choice([12, 12, 12, 8])
    o = [img.alloc(0x10, fill_random=False) for _ in range(4)]
    return rn.Case([rng.randrange(1 << 32), 0 if rng.random() < 0.25 else o[0], 0 if rng.random() < 0.25 else o[1],
                    0 if rng.random() < 0.25 else o[2], 0 if rng.random() < 0.25 else o[3]], img, ret=True)


def gen_bebb0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    out = img.alloc(0x10, fill_random=True)
    a1 = img.alloc(0x40, fill_random=False)
    head = a1 + 8
    owners = [img.alloc(0x40, fill_random=False) for _ in range(3)]
    nodes = []
    nn = rng.choice([0, 1, 3, 5, 0xFFFFFFFF])
    for _ in range(rng.randrange(0, 5) if 0 < nn < 0x80000000 else 0):
        base = img.alloc(0x80, fill_random=False)
        node = base + 0x40
        img.w32(node - 0x28, rng.choice(owners))
        nodes.append(node)
    chain = [head] + nodes + [head]
    for j in range(len(chain) - 1):
        img.w32(chain[j], chain[j + 1])
    img.callee_ret[0x3BFE60] = nn
    return rn.Case([out, a1], img, ret=True)


def _py_strncpy_e110(m):
    cid = m.callees.index(0x442DE8)
    d, s, n = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, 0, s, n, 0])
    k = 0
    while k < n:
        b = m.mem.rb(s + k)
        m.mem.wb(d + k, b)
        k += 1
        if b == 0:
            break
    while k < n:
        m.mem.wb(d + k, 0)
        k += 1
    return d


C_4125D0 = ("static unsigned p4_last520; unsigned func_004125d0(unsigned a) { unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, a, 0, 0, 0); return (n == 0) ? r : p4_last520; }")
C_412520 = ("unsigned func_00412520(unsigned a, unsigned n) { unsigned r = p4_record({ID}, a, 0, n, 0); p4_last520 = r; return r; }")


def _py_4125d0(m):
    cid, n, ok = _ok(m, 0x4125D0)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    return ok if n == 0 else getattr(m, "_last520", 0)


def _py_412520(m):
    cid, n, ok = _ok(m, 0x412520)
    m.ilog.extend([cid, m.g32(4), 0, m.g32(5), 0])
    m._last520 = ok
    return ok


def gen_3780(rng):
    img = rn.Img(rng)
    rec = img.alloc(0x40, fill_random=True)
    img.w32(rec + 0x3C, rng.choice([0, 0x10, 0x1, 0x20]))
    rec2 = img.alloc(0x40, fill_random=True)
    mode = rng.choice([0, 1, 1, 2])
    img.callee_ret[0x4125D0] = rec if mode == 1 else 0
    img.callee_ret[0x412520] = rec2 if mode != 0 else rng.choice([0, rec2])
    return rn.Case([rng.choice([0, 0x5000, 0x6000]), rng.randrange(-2, 20), rng.randrange(-2, 20)], img, ret=True)


def gen_e110(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.hook(0x008873D4, 6)
    img.hook(0x008873D0, 0)
    img.hook(0x00887194, 4) if rng.random() < 0.7 else img.w32(0x00887194, 0)
    img.w32(0x00887188, rng.choice([0, 1, 2]))
    path = img.alloc(0x40, fill_random=False)
    for i in range(0x40):
        img.mem.wb(path + i, 0)
    txt = [rng.choice([0x61, 0x62, 0x3A, 0x2F]) for _ in range(rng.randrange(0, 12))]
    for i, ch in enumerate(txt):
        img.mem.wb(path + i, ch)

    def dev():
        d = img.alloc(0x80, fill_random=False)
        nm = img.alloc(0x20, fill_random=False)
        img.w32(d + 0xC, nm)
        img.hook(d + 0x4C, 1)
        return d
    nodes = [dev() for _ in range(rng.randrange(0, 4))]
    for j in range(len(nodes)):
        img.w32(nodes[j], nodes[j + 1] if j + 1 < len(nodes) else 0)
    img.w32(0x00887180, nodes[0] if nodes else 0)
    img.w32(0x00764840, rng.choice([0, dev()]))
    img.callee_ret[0x500000] = 0xFD000000 | rng.randrange(0, 1 << 8)
    img.callee_ret[0x500001] = rng.randrange(1 << 16)
    return rn.Case([path], img, ret=True)


def gen_9f40(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.hook(0x00887300, 0)
    img.hook(0x00887304, 9)
    off = 0x10
    img.w32(0x00764700, off)
    a0 = img.alloc(0x40, fill_random=False)
    holder = img.alloc(0xC0, fill_random=False)
    img.w32(a0 + off, holder)
    r = img.alloc(0x20, fill_random=True)
    img.w32(holder + 0x9C, r)
    node = img.alloc(0x20, fill_random=False)
    sub = img.alloc(0x20, fill_random=True)
    img.w32(r + 8, node)
    img.w32(node + 4, sub)
    a1 = img.alloc(0x80, fill_random=True)
    img.w32(a1 + 0x58, rng.randrange(1 << 32))
    img.w32(a1 + 0x64, rng.choice([0, 1, 5]))
    mat = img.alloc(0x60, fill_random=True)
    img.callee_ret[0x3E9700] = mat
    return rn.Case([a0, a1, rng.randrange(1 << 32)], img, ret=True)


C_RD_CNT = ("unsigned func_003df360(unsigned s, unsigned *p, unsigned n) { unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; "
            "if (n == 4) *p = (unsigned)((int)(((s >> (3 * k)) ^ k) % 5u) - 1); return 1; }")


def _py_rd_cnt(m):
    cid, n, ok = _ok(m, 0x3DF360)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if not ok:
        return 0
    if nn == 4:
        m.mem.wn(p, 4, ((((s >> (3 * n)) ^ n) & 0xFFFFFFFF) % 5 - 1) & 0xFFFFFFFF)
    return 1


def gen_c150(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    rec = img.alloc(0x20, fill_random=False)
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 12)
    img.callee_ret[0x3DF300] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3E2910] = rng.choice([0, 1, 1, 1, 1])
    return rn.Case([rec, (rng.randrange(1 << 32) & ~0xF) | (rng.randrange(16) << 4 & 0xF0)], img, ret=True)


def gen_72a0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x0076483C, 0x10)
    obj = img.alloc(0x80, fill_random=False)
    blk = obj + 0x10
    for i in range(10):
        if rng.random() < 0.6:
            o = img.alloc(0x20, fill_random=True)
            img.w32(blk + 4 * i, o)
        else:
            img.w32(blk + 4 * i, 0)
    return rn.Case([obj], img, ret=True)


C_E2910_GEO = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; "
               "if (n == 4) { unsigned m = (k + 2u) % 3u; if (k == 0) *p = (s & 0x100u) ? 5u : 4u; else if (m == 0) *p = 8u * (1u + ((s >> k) % 6u)); else *p = (s >> (k + 1u)) & 1u; } return n; }")


def _py_e2910_geo(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if not ok:
        return 0
    if nn == 4:
        mm = (n + 2) % 3
        if n == 0:
            m.mem.wn(p, 4, 5 if (s & 0x100) else 4)
        elif mm == 0:
            m.mem.wn(p, 4, 8 * (1 + ((s >> n) % 6)))
        else:
            m.mem.wn(p, 4, (s >> (n + 1)) & 1)
    return nn


def gen_e560(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.w32(0x00764820, 0x10)
    geo = img.alloc(0x60, fill_random=False)
    n = rng.randrange(0, 4)
    img.w32(geo + 0x24, n)
    cache = img.alloc(0x40, fill_random=False)
    img.w32(cache, rng.choice([n, n, n + 1]))
    img.w32(geo + 0x10, cache if rng.random() < 0.6 else 0)
    slotbox = img.alloc(0x10, fill_random=False)
    cache2 = img.alloc(0x40, fill_random=False)
    img.w32(cache2, n)
    img.w32(slotbox, cache2)
    img.callee_ret[0x3CE050] = slotbox
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1, 1, 1])
    img.callee_ret[0x3E2910] = 0xFE000000 | rng.randrange(1, 12)
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | (rng.randrange(16) << 4 & 0xF0), geo], img, ret=True)


def _geo_blocks(img, rng, n):
    cache = img.alloc(0x40, fill_random=False)
    img.w32(cache, n)
    for i in range(n):
        b = img.alloc(0x400, fill_random=False)
        pay = b + 0x1E0 + 0x10 * rng.randrange(0, 4)
        img.w32(b + 8, rng.randrange(0x230, 0x300))
        img.w32(b + 0x20, pay)
        img.w32(cache + 4 + 4 * i, b)
    return cache


def gen_e3a0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x00764820, 0x10)
    img.touch(0x00763000)
    img.w32(0x00763BA8, 4)
    geo = img.alloc(0x60, fill_random=False)
    img.w32(geo + 8, rng.choice([0, 0x1000000, 0x1000000, 0x1000000 | rng.randrange(1 << 32)]))
    n = rng.randrange(0, 4)
    img.w32(geo + 0x10, _geo_blocks(img, rng, n))
    img.callee_ret[0x3CE840] = rng.randrange(0x20, 0x400)
    img.callee_ret[0x3DEFF0] = 0xFE000000 | 1 if rng.random() < 0.8 else 0
    img.callee_ret[0x3E2AB0] = 0xFE000000 | rng.randrange(0, 11)
    img.callee_ret[0x3CE170] = rng.randrange(1 << 32)
    return rn.Case([rng.randrange(1 << 32), geo], img, ret=True)


def gen_e9e0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x00764818, 0x10)
    img.touch(0x00763000)
    img.w32(0x00763BAC, 4)
    obj = img.alloc(0x40, fill_random=False)
    img.w32(obj + 8, rng.choice([0, 0x1000000, 0x1000000, 0x1000000 | rng.randrange(1 << 32)]))
    sec = img.alloc(0x60, fill_random=False)
    n = rng.randrange(0, 4)
    img.w32(sec + 0x10, _geo_blocks(img, rng, n))
    img.callee_ret[0x3C9C20] = obj
    img.callee_ret[0x3DEFF0] = 1 if rng.random() < 0.8 else 0
    img.callee_ret[0x3E2AB0] = 0xFE000000 | rng.randrange(0, 11)
    img.callee_ret[0x3CE170] = rng.randrange(1 << 32)
    img.callee_ret[0x3CEEB0] = rng.randrange(0x20, 0x400)
    return rn.Case([rng.randrange(1 << 32), sec], img, ret=True)


def gen_eeb0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x00764818, 0x10)
    obj = img.alloc(0x40, fill_random=False)
    img.w32(obj + 8, rng.choice([0, 0x1000000, 0x1000000, 0x1000000 | rng.randrange(1 << 32)]))
    sec = img.alloc(0x60, fill_random=False)
    n = rng.randrange(0, 12)
    img.w32(sec + 0x10, _geo_blocks(img, rng, n))
    img.callee_ret[0x3C9C20] = obj
    return rn.Case([sec], img, ret=True)


C_DEFF0 = ("unsigned func_003deff0(unsigned s, unsigned t, unsigned sz, unsigned ver, unsigned x) { return p4_record({ID}, s, t, sz, ver * 31u + x); }")
C_E2AB0H = ("unsigned func_003e2ab0(unsigned s, unsigned *p, unsigned n) { unsigned h = 0, i; for (i = 0; i < n / 4; i++) h = h * 31u + p[i]; return p4_record({ID}, s, h, n, 0); }")


def _py_deff0(m):
    cid, n, ok = _ok(m, 0x3DEFF0)
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), (m.g32(7) * 31 + m.g32(8)) & 0xFFFFFFFF])
    return ok


def _py_e2ab0h(m):
    cid, n, ok = _ok(m, 0x3E2AB0)
    p, nn = m.g32(5), m.g32(6)
    h = 0
    for i in range(nn // 4):
        h = (h * 31 + m.mem.rn(p + 4 * i, 4)) & 0xFFFFFFFF
    m.ilog.extend([cid, m.g32(4), h, nn, 0])
    return ok


def gen_fb50(rng):
    img = rn.Img(rng)
    a0 = img.alloc(0x60, fill_random=True)
    n = rng.randrange(0, 4)
    img.mem.wb(a0 + 0x1E, n)
    img.mem.wb(a0 + 0x1F, 0)
    img.mem.wb(a0 + 0x1C, rng.randrange(0, 5))
    img.mem.wb(a0 + 0x1D, 0)
    ents = img.alloc(max(n, 1) * 16 + 16, fill_random=True)
    img.w32(a0 + 0x20, ents)
    img.w32(a0, img.r32(a0) | 1 if rng.random() < 0.5 else img.r32(a0) & ~1)
    arr = img.alloc(0x40, fill_random=True)
    img.w32(a0 + 0x24, arr)
    img.callee_ret[0x3DEFF0] = 0xFE000000 | rng.randrange(0, 3)
    img.callee_ret[0x3E2AB0] = 0xFE000000 | rng.randrange(0, 6)
    img.callee_ret[0x3DF270] = rng.choice([0, 1, 1])
    return rn.Case([a0, rng.randrange(1 << 32)], img, ret=True)


def gen_dda50(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x00764844, rng.randrange(1 << 32))
    if rng.random() < 0.5:
        img.hook(0x00887194, 4)
    else:
        img.w32(0x00887194, 0)
    dev = img.alloc(0x80, fill_random=False)
    slot = img.alloc(0x80, fill_random=True)
    img.w32(slot + 0x38, rng.choice([1, 1, 0, 2]))
    img.w32(slot + 0x30, rng.choice([0, 7]))
    img.w32(dev + 4, rng.randrange(0, 4))
    img.w32(dev + 0x10, rng.choice([0, 1]))
    img.hook(dev + 0x14, 0)
    img.hook(dev + 0x28, 1)
    img.callee_ret[0x500000] = slot
    img.callee_ret[0x500001] = rng.choice([1, 1, 0, 2])
    st = img.alloc(0x40, fill_random=False)
    img.callee_ret[0x3DDC10] = st
    return rn.Case([0 if rng.random() < 0.1 else dev, rng.randrange(1 << 32), rng.choice([0, 0x10, 5, 0x15]), rng.randrange(1 << 32), rng.randrange(1 << 32)], img, ret=True)


def gen_6df0(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x00764820, 0x10)
    off = 0x20
    img.w32(0x00764A24, off)
    img.mem.wb(0x00764A74, rng.randrange(256))
    cam = img.alloc(0x300, fill_random=True)
    img.w32(0x008872E0, cam)
    for pl in range(6):
        img.fill_f(cam + off + 0x60 + 0x14 * pl, 4, -2, 2)
    a0 = img.alloc(0x60, fill_random=True)
    atomic = img.alloc(0x40, fill_random=False)
    geo = img.alloc(0x80, fill_random=False)
    rec = img.alloc(0x20, fill_random=False)
    cnt = rng.randrange(0, 5)
    img.mem.wb(rec + 4, cnt)
    img.mem.wb(rec + 5, 0)
    img.w32(geo + 0x54, rec)
    img.w32(atomic + 0x18, geo)
    img.w32(a0 + 8, atomic)
    cache = img.alloc(0x20, fill_random=False)
    img.w32(cache, cnt if rng.random() < 0.6 else cnt + 1)
    img.w32(geo + 0x10, cache if rng.random() < 0.7 else 0)
    box = img.alloc(0x10, fill_random=False)
    img.w32(box, img.alloc(0x20, fill_random=False))
    img.callee_ret[0x3CE050] = box
    sph = img.alloc(0x10, fill_random=False)
    img.fill_f(sph, 4, 0.1, 2)
    img.callee_ret[0x3BFAE0] = sph
    dest = img.alloc(0x50, fill_random=False)
    cell = img.alloc(0x10, fill_random=False)
    img.w32(cell, dest)
    return rn.Case([a0, cell], img, ret=False)


def gen_20d0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x00887000)
    img.mem.wb(0x00764A74, rng.randrange(256))
    cam = img.alloc(0x100, fill_random=True)
    img.w32(0x008872E0, cam)
    a0 = img.alloc(0x60, fill_random=True)
    src = img.alloc(0x40, fill_random=True)
    img.w32(src, rng.randrange(1 << 32))
    img.w32(src + 4, rng.choice([0, img.alloc(0x40, fill_random=True)]))
    img.w32(a0 + 8, src)
    t = img.alloc(0x10, fill_random=False)
    img.w32(t, rng.choice([1, 0, 2, 0x10, 8, 0x100, 0x300, 0x20, 0x18, 0x41, 0x12]) | (rng.randrange(1 << 32) & 0x100 if rng.random() < 0.3 else 0))
    img.w32(a0 + 0xC, t)
    dstbuf = img.alloc(0x50, fill_random=True)
    cell = img.alloc(0x10, fill_random=False)
    img.w32(cell, dstbuf)
    return rn.Case([a0, cell], img, ret=False)


C_DF050B = ("unsigned func_003df050(unsigned s, unsigned x, unsigned *sz, unsigned *ver) { static const unsigned K[6] = {0x34000, 0x35000, 0x37002, 0x37003, 0x36000, 0x350FF}; "
            "unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, x, 0, 0); if (!r) return 0; if (sz) *sz = 0x18; if (ver) *ver = K[((s >> 4) + k) % 6]; return 1; }")


def _py_df050b(m):
    cid, n, ok = _ok(m, 0x3DF050)
    s = m.g32(4)
    m.ilog.extend([cid, s, m.g32(5), 0, 0])
    if not ok:
        return 0
    if m.g32(6):
        m.mem.wn(m.g32(6), 4, 0x18)
    if m.g32(7):
        m.mem.wn(m.g32(7), 4, [0x34000, 0x35000, 0x37002, 0x37003, 0x36000, 0x350FF][((s >> 4) + n) % 6])
    return 1


C_E2910_N = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); if (r >= n) *p = (s >> 3) % 5u; return r; }")


def _py_e2910_n(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if ok >= nn:
        m.mem.wn(p, 4, (s >> 3) % 5)
    return ok


# ---- generic "hashing" stub factory: args are described as "i" (int compared), ("h", n) (pointer: hash of n words),
# "s" (ignored), and the optional trailing ("f",) float arg; int return = the callee_ret code/raw value.
def _mk_hstub(addr, args, ret="int", fl=False, post=None):
    ps = ["unsigned a%d" % i for i in range(len(args))] + (["float f"] if fl else [])
    hs = []
    slots = []
    for i, a in enumerate(args):
        if a == "i":
            slots.append("a%d" % i)
        elif a == "s":
            pass
        else:
            n = a[1]
            hs.append("unsigned h%d = 0; for (j = 0; j < %d; j++) h%d = h%d * 31u + ((unsigned *)a%d)[j];" % (i, n, i, i, i))
            slots.append("h%d" % i)
    if fl:
        slots.append("*(unsigned *)&f")
    while len(slots) < 4:
        slots.append("0")
    body = "unsigned j; " + " ".join(hs) + " unsigned r = p4_record({ID}, %s); " % ", ".join(slots[:4])
    if post:
        body += post[0]
    body += "(void)j; "
    if ret == "int":
        body += "return r;"
    rt = {"int": "unsigned", "void": "void"}[ret]
    csrc = "%s func_%08x(%s) { %s }" % (rt, addr, ", ".join(ps), body)

    def impl(m):
        cid, n, ok = _ok(m, addr)
        sl = []
        for i, a in enumerate(args):
            if a == "i":
                sl.append(m.g32(4 + i))
            elif a == "s":
                pass
            else:
                p_, h = m.g32(4 + i), 0
                for j in range(a[1]):
                    h = (h * 31 + m.mem.rn(p_ + 4 * j, 4)) & 0xFFFFFFFF
                sl.append(h)
        if fl:
            sl.append(m.f[12])
        while len(sl) < 4:
            sl.append(0)
        m.ilog.extend([cid] + sl[:4])
        if post:
            post[1](m, n)
        return ok if ret == "int" else 0
    return csrc, impl


def _hstubs(spec):
    cs, im = {}, {}
    for addr, v in spec.items():
        cs[addr], im[addr] = _mk_hstub(addr, v[0], **(v[1] if len(v) > 1 else {}))
    return cs, im


C_PTRN = ("unsigned func_003d5790(unsigned n, unsigned s) { unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, n, s, 0, 0); return r ? 0x38000000u + 0x100u * k : 0u; }")


def _py_5790(m):
    cid, n, ok = _ok(m, 0x3D5790)
    m.ilog.extend([cid, m.g32(4), m.g32(5), 0, 0])
    return (0x38000000 + 0x100 * n) if ok else 0


def _post_e0c90(m, n):
    mm = m.g32(4)
    for i in range(16):
        m.mem.wn(mm + 4 * i, 4, m.mem.rn(mm + 4 * i, 4) ^ (0x0101 * (i + 1)))


def _post_e0870(m, n):
    mm = m.g32(4)
    for i in range(16):
        m.mem.wn(mm + 4 * i, 4, (m.mem.rn(mm + 4 * i, 4) + 0x10001 * (i + 1)) & 0xFFFFFFFF)


_PC0C90 = "for (j = 0; j < 16; j++) ((unsigned *)a0)[j] ^= 0x0101u * (j + 1u); "
_PC0870 = "for (j = 0; j < 16; j++) ((unsigned *)a0)[j] += 0x10001u * (j + 1u); "


def _c44b950():
    return ("float func_0044b950(float y, float x) { p4_record({ID}, *(unsigned *)&y, *(unsigned *)&x, 0, 0); return y * 0.5f + x * 0.25f; }")


def _py_44b950(m):
    import mips as _m
    cid = m.callees.index(0x44B950)
    m.ilog.extend([cid, m.f[12], m.f[13], 0, 0])
    m.f[0] = _m.f2b(_m.b2f(m.f[12]) * 0.5 + _m.b2f(m.f[13]) * 0.25)
    return 0


def gen_7490(rng):
    img = rn.Img(rng)
    blk = img.alloc(0x60, fill_random=False)
    img.w32(blk, rng.choice([0, 0x77]))
    img.w32(blk + 4, rng.choice([0, 0x78]))
    for i in range(8):
        if rng.random() < 0.6:
            s = img.alloc(0x60, fill_random=False)
            A = img.alloc(0x20, fill_random=False)
            arr = img.alloc(0x100, fill_random=False)
            img.w32(A + 0x14, arr)
            cnt = rng.randrange(0, 4)
            img.w32(s + 0x2C, cnt)
            for j in range(4):
                img.w32(arr + 0x20 + 4 * j, rng.choice([0, 1, 2, 3]))
            img.w32(s, A)
            img.w32(blk + 8 + 4 * i, s)
        else:
            img.w32(blk + 8 + 4 * i, 0)
    img.callee_ret[0x3E0F80] = rng.choice([0, 0x5555])
    return rn.Case([blk], img, ret=True)


def gen_7350(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.w32(0x0076483C, 0x10)
    dst = img.alloc(0x80, fill_random=False)
    src = img.alloc(0x80, fill_random=False)
    for i in range(8):
        if rng.random() < 0.6:
            s = img.alloc(0x20, fill_random=True)
            a = img.alloc(0x20, fill_random=False)
            hdr = img.alloc(0x20, fill_random=False)
            img.w32(a, hdr)
            img.w32(hdr + 4, rng.randrange(1 << 32))
            img.w32(s, a)
            img.w32(src + 0x10 + 8 + 4 * i, s)
        else:
            img.w32(src + 0x10 + 8 + 4 * i, 0)
    img.callee_ret[0x3D5750] = rng.randrange(0, 10)
    img.callee_ret[0x3D5790] = 0xFE000000 | rng.randrange(0, 9)
    return rn.Case([dst, src], img, ret=True)


def gen_75b0(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.w32(0x0076483C, 0x10)
    obj = img.alloc(0x80, fill_random=False)
    for i in range(8):
        if rng.random() < 0.5:
            s = img.alloc(0x20, fill_random=False)
            a = img.alloc(0x20, fill_random=False)
            img.w32(s, a)
            img.w32(obj + 0x10 + 8 + 4 * i, s)
    rec = img.alloc(0x40, fill_random=True)
    img.callee_ret[0x3D8060] = rec
    img.callee_ret[0x3DEFF0] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3DF240] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3E2AB0] = rng.choice([0, 1, 1])
    return rn.Case([rng.randrange(1 << 32), rng.randrange(1 << 32), obj], img, ret=True)


def gen_76d0(rng):
    img = rn.Img(rng)
    r = img.alloc(0x40, fill_random=False)
    hdr = img.alloc(0x20, fill_random=False)
    img.w32(hdr + 8, rng.randrange(0, 0x100))
    img.w32(r, hdr)
    img.w32(r + 0x10, img.alloc(0x200, fill_random=False))
    img.callee_ret[0x3D7B40] = rng.choice([0, r, r])
    name = img.alloc(0x40, fill_random=True)
    return rn.Case([name], img, ret=True)


def gen_65e0(rng):
    img = rn.Img(rng)
    import mips as _m
    img.touch(0x0070B000)
    v650 = rng.randrange(1, 1 << 30)
    v680 = rng.randrange(1, 1 << 30)
    img.w32(0x0070B650, v650)
    img.w32(0x0070B680, v680)
    a0 = img.alloc(0x20, fill_random=False)
    T = img.alloc(0x20, fill_random=False)
    img.w32(T, rng.choice([v650, v680, 5]))
    img.w32(a0, T)
    out = img.alloc(0x40, fill_random=True)
    mat = img.alloc(0x40, fill_random=True)
    fb = _m.f2b(rng.uniform(-3, 3))
    img.fregs = {12: fb}
    img.callee_ret[0x3D6C10] = rng.choice([0, 1])
    return rn.Case([a0, out, rng.randrange(1 << 32), mat, fb], img, ret=True)


def gen_6fb0(rng):
    img = rn.Img(rng)
    img.touch(0x0070B000)
    for i, a in enumerate((0x0070B5F8, 0x0070B5FC, 0x0070B600)):
        import mips as _m
        img.w32(a, _m.f2b(rng.uniform(-1, 1)))
    out = img.alloc(0x20, fill_random=False)
    mat = img.alloc(0x40, fill_random=False)
    img.fill_f(mat, 16, -3, 3)
    return rn.Case([out, mat], img, ret=True)


C_E2910_REC = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned i; unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; for (i = 0; i < 8; i++) p[i] = s * (i + 1u); return r; }")
C_DF360_MASK = ("unsigned func_003df360(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; *p = (s >> 3) & 0xFFu; return r; }")


def _py_e2910_rec(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if not ok:
        return 0
    for i in range(8):
        m.mem.wn(p + 4 * i, 4, (s * (i + 1)) & 0xFFFFFFFF)
    return ok


def _py_df360_mask(m):
    cid, n, ok = _ok(m, 0x3DF360)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if not ok:
        return 0
    m.mem.wn(p, 4, (s >> 3) & 0xFF)
    return ok


def gen_77f0(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.w32(0x0076483C, 0x10)
    obj = img.alloc(0x80, fill_random=False)
    anim = img.alloc(0x40, fill_random=False)
    hdr = img.alloc(0x20, fill_random=False)
    img.w32(anim, hdr)
    img.w32(hdr + 4, rng.randrange(1 << 32))
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3DF360] = rng.choice([0, 1, 1])
    img.callee_ret[0x3E2910] = 0xFE000000 | rng.randrange(0, 9)
    img.callee_ret[0x3D60D0] = rng.choice([0, 0x1234])
    img.callee_ret[0x3D6170] = rng.choice([0, anim])
    img.callee_ret[0x3D76D0] = rng.choice([0, anim, anim])
    img.callee_ret[0x3D5750] = rng.randrange(0, 10)
    img.callee_ret[0x3D5790] = 0xFE000000 | rng.randrange(0, 9)
    img.callee_ret[0x3D5840] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x3D7490] = rng.choice([0, 1, 1])
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | (rng.randrange(16) << 4 & 0xF0), rng.randrange(1 << 32), obj], img, ret=True)

_C76, _P76 = _hstubs({0x3D7B40: (["i", "i", "i", "s", "i"],), 0x3D65E0: (["i", "i", "i", ("h", 16)], {"fl": True})})
_C65, _P65 = _hstubs({0x3D6C10: (["s", ("h", 16)],)})
_C6F, _P6F = _hstubs({0x3E0C90: ([("h", 16), "i", "i"], {"ret": "void", "post": (_PC0C90, _post_e0c90)}), 0x3E0870: ([("h", 16), ("h", 3), "i"], {"ret": "void", "fl": True, "post": (_PC0870, _post_e0870)})})
_C6F[0x44B950] = _c44b950()
_P6F[0x44B950] = _py_44b950
_C77, _P77 = _hstubs({0x3D60D0: (["i"],), 0x3D6170: (["i", ("h", 8)],), 0x3D76D0: ([("h", 8)],), 0x3D7CD0: (["i"], {"ret": "void"}),
                      0x3D5750: (["i"],), 0x3D5840: (["i", "i"],), 0x3D7C50: (["i"], {"ret": "void"}), 0x3D7490: (["i"],)})
_C77.update({0x3D5790: C_PTRN, 0x3E2910: C_E2910_REC, 0x3DF360: C_DF360_MASK})
_P77.update({0x3D5790: _py_5790, 0x3E2910: _py_e2910_rec, 0x3DF360: _py_df360_mask})
_C73, _P73 = _hstubs({0x3D5750: (["i"],), 0x3D59A0: (["i", "i"], {"ret": "void"}), 0x3D7CD0: (["i"], {"ret": "void"}), 0x3D7C50: (["i"], {"ret": "void"}), 0x3D5830: (["i"], {"ret": "void"})})
_C73.update({0x3D5790: C_PTRN})
_P73.update({0x3D5790: _py_5790})


def gen_7680(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    rec = img.alloc(0x80, fill_random=True)
    x = rng.randrange(0, 4)
    s = rng.randrange(0, 40)
    n = rng.randrange(0, 5)
    src1 = img.alloc(max(s, 1) + 16, fill_random=True)
    src2 = img.alloc(n * 4 + 16, fill_random=True)
    src3 = img.alloc(n * 16 + 16, fill_random=True)
    src4 = img.alloc(max(x, 1) * 64 + 16, fill_random=True)
    return rn.Case([rec, x, s, n, rng.choice([0, src1]), rng.choice([0, src2]), rng.choice([0, src3]), rng.choice([0, src4])], img, ret=True)


C_DF360_PDS = ("unsigned func_003df360(unsigned s, unsigned *p, unsigned n) { unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; "
               "if (n == 16) return s; if (k == 0) *p = (s & 0x100u) ? 5u : 4u; else *p = ((s >> 3) % 4u) | ((((s >> 5) % 6u) * 8u) << 8) | (((s >> 9) & 7u) << 16); return 1; }")


def _py_df360_pds(m):
    cid, n, ok = _ok(m, 0x3DF360)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if not ok:
        return 0
    if nn == 16:
        return s
    if n == 0:
        m.mem.wn(p, 4, 5 if (s & 0x100) else 4)
    else:
        m.mem.wn(p, 4, ((s >> 3) % 4) | ((((s >> 5) % 6) * 8) << 8) | (((s >> 9) & 7) << 16))
    return 1


C_E2910_RET = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); return r ? n : 0u; }")


def _py_e2910_ret(m):
    cid, n, ok = _ok(m, 0x3E2910)
    nn = m.g32(6)
    m.ilog.extend([cid, m.g32(4), 0, nn, 0])
    return nn if ok else 0


def gen_8ad0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00886000)
    img.touch(0x00887000)
    img.hook(0x008873F8, 3)
    img.hook(0x008873E8, 2)
    img.w32(0x008864A8, rng.randrange(1 << 32))
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1, 1, 1])
    img.callee_ret[0x3DF360] = 0xFE000000 | rng.randrange(0, 5)
    img.callee_ret[0x3E2910] = 0xFE000000 | rng.randrange(0, 4)
    img.callee_ret[0x3B7290] = rng.choice([0, 1, 1])
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | (rng.randrange(16) << 4 & 0xF0), rng.randrange(1 << 32)], img, ret=True)


def gen_c9c0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    img.hook(0x008873D4, 6)
    img.hook(0x008873AC, 0)
    t = img.alloc(0x20, fill_random=False)
    n = rng.randrange(0, 4)
    ents = img.alloc(max(n, 1) * 16 + 16, fill_random=False)
    for i in range(n):
        img.w32(ents + 16 * i + 0xC, rng.choice([0, 0x1234]))
    img.w32(t, n)
    img.w32(t + 4, rng.choice([0, ents]) if n == 0 else ents)
    nm = img.alloc(0x40, fill_random=False)
    for i in range(0x40):
        img.mem.wb(nm + i, 0)
    for i in range(rng.randrange(0, 12)):
        img.mem.wb(nm + i, rng.randrange(1, 256))
    img.callee_ret[0x3BD110] = rng.randrange(1, 40)
    return rn.Case([t, rng.choice([0, nm]), rng.randrange(1 << 32), rng.randrange(0, 8)], img, ret=True)


def gen_0540(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00887000)
    img.w32(0x00764820, 0x10)
    img.w32(0x0076481C, 0x20)
    off = 0x20
    img.w32(0x00764A24, off)
    cam = img.alloc(0x300, fill_random=True)
    img.w32(0x008872E0, cam)
    for pl in range(6):
        img.fill_f(cam + off + 0x60 + 0x14 * pl, 4, -2, 2)
    a0 = img.alloc(0x60, fill_random=True)
    s = img.alloc(0x80, fill_random=True)
    geo = img.alloc(0x80, fill_random=False)
    rec = img.alloc(0x20, fill_random=False)
    cnt = rng.randrange(0, 5)
    img.mem.wb(rec + 4, cnt)
    img.mem.wb(rec + 5, 0)
    img.w32(geo + 0x54, rec)
    img.w32(geo + 0x18, rng.choice([1, 1, 0, 2, 3]))
    img.w32(s + 0x18, geo)
    img.w32(a0 + 8, s)
    img.fill_f(s + 0x58, 2, 0.5, 3)
    cache = img.alloc(0x20, fill_random=False)
    img.w32(cache, cnt if rng.random() < 0.6 else cnt + 1)
    img.w32(geo + 0x10, cache if rng.random() < 0.7 else 0)
    cache2 = img.alloc(0x20, fill_random=False)
    img.w32(cache2, cnt if rng.random() < 0.6 else cnt + 1)
    img.w32(s + 0x20, cache2 if rng.random() < 0.7 else 0)
    box = img.alloc(0x10, fill_random=False)
    img.w32(box, img.alloc(0x20, fill_random=False))
    img.callee_ret[0x3CE050] = box
    sph = img.alloc(0x10, fill_random=False)
    img.fill_f(sph, 4, 0.1, 2)
    img.callee_ret[0x3BFAE0] = sph
    mat = img.alloc(0x40, fill_random=True)
    img.callee_ret[0x3E9700] = mat
    dest = img.alloc(0x50, fill_random=False)
    cell = img.alloc(0x10, fill_random=False)
    img.w32(cell, dest)
    return rn.Case([a0, cell], img, ret=True)


def gen_6080(rng):
    img = rn.Img(rng)

    def mat():
        if rng.random() < 0.15:
            return 0
        m = img.alloc(0x20, fill_random=False)
        x = img.alloc(0x10, fill_random=False)
        y = img.alloc(0x40, fill_random=False)
        img.mem.wb(y + 0x23, rng.choice([0x01, 0x03, 0x05, 0x02, 0x11, 0x13, 0x15, 0x00]))
        img.w32(x, y)
        img.w32(m, rng.choice([0, x, x]))
        img.mem.wb(m + 7, rng.choice([0xFF, 0xFF, 0, 5]))
        return m
    shared = mat()
    recs = []
    for _ in range(2):
        r = img.alloc(0x40, fill_random=False)
        img.w32(r + 8, shared if rng.random() < 0.1 else mat())
        for off in (0xC, 0xE, 0x10, 0x12):
            img.mem.wn(r + off, 2, rng.choice([0, 1, 2, 0xFFFF, 7]))
        recs.append(r)
    cells = []
    for r in recs:
        cell = img.alloc(0x10, fill_random=False)
        img.w32(cell, r)
        cells.append(cell)
    return rn.Case(cells, img, ret=True)


C_65B0N = ("unsigned func_003c65b0(unsigned a0, unsigned a1, unsigned a2, unsigned a3, unsigned a4, unsigned a5) { unsigned k = p4_ncalls[{ID}]; "
           "p4_record({ID}, a0, a1, a2 * 65536u + a3, a4 * 31u + a5); return 0x5000u + k; }")


def _py_65b0n(m):
    cid = m.callees.index(0x3C65B0)
    n = m.ncalls.get(cid, 0)
    m.ncalls[cid] = n + 1
    a = [m.g32(4 + i) for i in range(6)]
    m.ilog.extend([cid, a[0], a[1], (a[2] * 65536 + a[3]) & 0xFFFFFFFF, (a[4] * 31 + a[5]) & 0xFFFFFFFF])
    return 0x5000 + n


def gen_67b0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873F8, 3)
    n = rng.randrange(0, 5)
    out = img.alloc(0x10, fill_random=False)
    tris = img.alloc(max(n, 1) * 20 + 16, fill_random=True)
    img.callee_ret[0x3E1210] = 0x7777
    return rn.Case([n, out, rng.randrange(1 << 32), tris], img, ret=True)


def gen_57d0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873F0, 0)
    lst = img.alloc(0x20, fill_random=False)
    cap = rng.randrange(0, 4)
    cnt = rng.randrange(0, 5)
    cnt = min(cnt, cap) if rng.random() < 0.5 else cnt
    items = img.alloc(max(cnt + 1, 1) * 20 + 20, fill_random=True)
    img.w32(lst, cap)
    img.w32(lst + 4, cnt)
    img.w32(lst + 8, items)
    newbuf = img.alloc((cnt + 2) * 20 + 20, fill_random=True)
    img.callee_ret[0x500000] = rng.choice([0, newbuf, newbuf])
    return rn.Case([lst, rng.randrange(1 << 32)] + [rng.randrange(1 << 16) for _ in range(7)], img, ret=True)


def gen_1ea0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873F0, 0)
    m = img.alloc(0x80, fill_random=False)
    flags = rng.choice([0, 0x10, 0x1000000, 0x1000010, 0x20])
    img.w32(m + 8, flags)
    nv = rng.randrange(0, 3)
    img.w32(m + 0x14, nv)
    old = rng.randrange(0, 4)
    img.w32(m + 0x18, old)
    add = rng.choice([0, 1, 2, 3, -1])
    stride = 0x1C if flags & 0x1000000 else (nv * 12 + 0x1C + (nv * 12 if flags & 0x10 else 0))
    tab = img.alloc(max(old, 1) * stride + 0x80, fill_random=True) if old and rng.random() < 0.8 else 0
    img.w32(m + 0x5C, tab)
    newbuf = img.alloc((old + 4) * stride + 0x100, fill_random=True)
    img.callee_ret[0x500000] = rng.choice([0, newbuf, newbuf, newbuf])
    return rn.Case([m, add & 0xFFFFFFFF], img, ret=True)


C_E2AB0N = ("unsigned func_003e2ab0(unsigned s, unsigned char *p, unsigned n) { unsigned h = 0, i; for (i = 0; i < n && i < 4096u; i++) h = h * 31u + p[i]; return p4_record({ID}, s, h, n, 0); }")


def _py_e2ab0n(m):
    cid, n, ok = _ok(m, 0x3E2AB0)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    h = 0
    for i in range(min(nn, 4096)):
        h = (h * 31 + m.mem.rn(p + i, 1)) & 0xFFFFFFFF
    m.ilog.extend([cid, s, h, nn, 0])
    return ok


def gen_8500(rng):
    img = rn.Img(rng)
    w = rng.randrange(0, 10)
    stride = rng.choice([w * 4, w * 4 + 8, 64])
    pix = img.alloc(stride * 3 + 64, fill_random=True)
    im = img.alloc(0x20, fill_random=False)
    img.w32(im + 4, w)
    img.w32(im + 0x10, stride)
    img.w32(im + 0x14, pix)
    buf = img.alloc(0x100, fill_random=False)
    img.callee_ret[0x3E2AB0] = rng.choice([0, 1, 5])
    return rn.Case([rng.randrange(1 << 32), im, rng.randrange(0, 3), rng.choice([4, 8, 24, 32]), buf], img, ret=True)


C_E4180V = ("float func_003e4180(float *v) { unsigned r = p4_record({ID}, *(unsigned *)&v[0], *(unsigned *)&v[1], *(unsigned *)&v[2], 0); return (float)(r % 7u) * 0.75f; }")


def _py_e4180v(m):
    import mips as _m
    cid, n, ok = _ok(m, 0x3E4180)
    p = m.g32(4)
    w = [m.mem.rn(p + 4 * i, 4) for i in range(3)]
    m.ilog.extend([cid, w[0], w[1], w[2], 0])
    m.f[0] = _m.f2b((ok % 7) * 0.75)
    return 0


def _py_fabs(m):
    m.f[0] = m.f[12] & 0x7FFFFFFF
    return 0


def gen_b190(rng):
    import mips as _m
    img = rn.Img(rng)

    def fl(n, lo=-2.0, hi=2.0):
        a = img.alloc(4 * n + 16, fill_random=False)
        for i in range(n):
            img.w32(a + 4 * i, _m.f2b(rng.choice([rng.uniform(lo, hi), float(rng.randrange(-3, 4))])))
        return a
    p2 = fl(8)
    p3 = fl(12)
    tex = img.alloc(0x20, fill_random=False)
    img.w32(tex + 0xC, rng.choice([1, 2, 8, 64, 128, 256]))
    img.w32(tex + 0x10, rng.choice([1, 2, 8, 64, 128, 256]))
    holder = img.alloc(0x10, fill_random=False)
    img.w32(holder, tex)
    rec = img.alloc(0x30, fill_random=False)
    img.w32(rec + 8, holder)
    img.w32(rec + 0xC, _m.f2b(rng.uniform(0, 2)))
    img.w32(rec + 0x10, rng.randrange(0, 5))
    img.w32(rec + 0x14, _m.f2b(rng.choice([rng.uniform(0, 4), 1.0])))
    img.callee_ret[0x3E4180] = rng.randrange(1 << 16)
    idx = [rng.randrange(0, 4) for _ in range(3)]
    return rn.Case([p2, p3] + idx + [rec], img, ret=False)


C_SCEREAD = ("int sceRead(int fd, unsigned char *p, int n) { unsigned i; unsigned r = p4_record({ID}, fd, 0, n, 0); if (r == 0) return -1; if (n > 0) for (i = 0; i < (unsigned)n && i < 4096u; i++) p[i] = (unsigned char)(fd * 7 + i * 3); return r == 1 ? n : n - 1; } "
             "int func_00427338(int a, unsigned char *b, int c) { return sceRead(a, b, c); }")


def _py_sceread(m):
    cid, n, ok = _ok(m, 0x427338)
    fd, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, fd, 0, nn, 0])
    if ok == 0:
        return 0xFFFFFFFF
    if 0 < nn < 1 << 31:
        for i in range(min(nn, 4096)):
            m.mem.wn(p + i, 1, (fd * 7 + i * 3) & 0xFF)
    return nn if ok == 1 else (nn - 1) & 0xFFFFFFFF


def gen_de4d0(rng):
    img = rn.Img(rng)
    s = img.alloc(0x80, fill_random=False)
    size = rng.choice([0, 10, 100, 0x1000, 0x7FFFFFF0])
    pos = rng.randrange(0, min(size, 120) + 1) if size else 0
    direct = rng.random() < 0.3
    bufsz = 0x40
    buf = img.alloc(bufsz + 16, fill_random=True)
    img.mem.wn(s, 4, size)
    img.mem.wn(s + 4, 4, 0)
    img.mem.wn(s + 0x10, 4, pos)
    img.mem.wn(s + 0x14, 4, 0)
    img.w32(s + 0x30, 1 if direct else 0)
    img.w32(s + 0x38, rng.randrange(0, 6))
    img.w32(s + 0x40, rng.randrange(0, 9))
    img.w32(s + 0x60, rng.randrange(0, 20))
    lim = rng.choice([0, 0x10, 0x30, 0x40])
    img.w32(s + 0x6C, lim)
    img.w32(s + 0x64, rng.randrange(0, lim + 1) if lim else 0)
    img.w32(s + 0x68, buf)
    dst = img.alloc(0x300, fill_random=False)
    img.callee_ret[0x427338] = rng.choice([0, 1, 1, 1, 2])
    return rn.Case([s, dst, rng.choice([0, 1, 5, 0x20, 0x50, 0x90, 0x100])], img, ret=True)


C_D3AE0 = ("void func_003d3ae0(unsigned o, int *out) { unsigned r = p4_record({ID}, o, 0, 0, 0); *out = (r & 1u) ? 2 : 1; }")
C_C96D0 = ("int func_003c96d0(unsigned cb, unsigned a) { return p4_record({ID}, 0, a, 0, 0); }")
C_C9750 = ("int func_003c9750(unsigned o, unsigned cb, unsigned a) { return p4_record({ID}, o, 0, a, 0); }")


def _py_d3ae0(m):
    cid, n, ok = _ok(m, 0x3D3AE0)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    m.mem.wn(m.g32(5), 4, 2 if ok & 1 else 1)
    return 0


def _py_c96d0(m):
    cid, n, ok = _ok(m, 0x3C96D0)
    m.ilog.extend([cid, 0, m.g32(5), 0, 0])
    return ok


def _py_c9750(m):
    cid, n, ok = _ok(m, 0x3C9750)
    m.ilog.extend([cid, m.g32(4), 0, m.g32(6), 0])
    return ok


def gen_3bd0(rng):
    img = rn.Img(rng)
    for a in (0x00764818, 0x00764A74, 0x008872E0, 0x008872E4):
        img.touch(a)
    off = 0x40
    img.w32(0x00764818, off)
    img.mem.wb(0x00764A74, rng.randrange(256))
    img.w32(0x008872E0, img.alloc(0x40, fill_random=False))
    rf = img.alloc(0x20, fill_random=False)
    img.w32(rf + 8, rng.choice([0, 0x1000000, 0x20, 0x40, 0x60, 0x1000020, 0x9D, 0xFF, 0x80, 0x5]) | rng.choice([0, 0x1000000]))
    img.w32(0x008872E4, rf)
    obj = img.alloc(0x100, fill_random=False)
    hdr = img.alloc(0x20, fill_random=False)
    img.w32(obj + 0x78, hdr)
    img.w32(hdr, rng.choice([0, 1, 2, 3, 0x10, 0x8, 0x4, 0x100, 0x18, 0x9]))
    hid = rng.randrange(0, 4)
    img.mem.wn(hdr + 4, 2, hid)
    img.mem.wn(hdr + 6, 2, rng.randrange(0, 0x100))

    def mesh():
        mm = img.alloc(0x40, fill_random=False)
        old = img.alloc(0x40, fill_random=False)
        img.w32(mm, hid if rng.random() < 0.6 else hid + 1)
        img.w32(mm + 4, old if rng.random() < 0.8 else 0)
        img.w32(old + 0x28, rng.choice([0, 0x10000, 0x20000, 0x1234, 0x4D, 0x4D0000 | hid]))
        return mm
    cell = img.alloc(0x10, fill_random=False)
    img.w32(cell, mesh())
    img.callee_ret[0x3CE050] = cell
    img.w32(obj + off, mesh() if rng.random() < 0.7 else 0)
    rec = img.alloc(0x40, fill_random=True)
    img.w32(rec + 8, obj)
    out = img.alloc(0x10, fill_random=False)
    for k in (0x3C96D0, 0x3C9750, 0x3D3AE0):
        img.callee_ret[k] = rng.randrange(256)
    return rn.Case([rec, out], img, ret=True)


C_E2910_W = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); *p = ((s >> 3) % 9u) | (((s >> 5) % 3u) << 8) | (((s >> 7) % 3u) << 12) | (((s >> 9) % 4u) << 16) | (((s >> 11) % 5u) << 20); return r; }")


def _py_e2910_w(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    m.mem.wn(p, 4, ((s >> 3) % 9) | (((s >> 5) % 3) << 8) | (((s >> 7) % 3) << 12) | (((s >> 9) % 4) << 16) | (((s >> 11) % 5) << 20))
    return ok


C_E4F60 = ("int func_003e4f60(void *r, unsigned s) { return p4_record({ID}, 0, s, 0, 0); }")
C_EF2E0 = ("unsigned func_003ef2e0(unsigned a) { return p4_record({ID}, a, 0, 0, 0); }")
C_EF470 = ("void func_003ef470(unsigned o, void *r) { p4_record({ID}, o, 0, 0, 0); }")
C_EF510 = ("void func_003ef510(unsigned o, void *r) { p4_record({ID}, o, 0, 0, 0); }")


def _mk_e4f60(addr, nargs_obj):
    def impl(m):
        cid, n, ok = _ok(m, addr)
        if addr == 0x3E4F60:
            m.ilog.extend([cid, 0, m.g32(5), 0, 0])
        elif addr == 0x3EF2E0:
            m.ilog.extend([cid, m.g32(4), 0, 0, 0])
        else:
            m.ilog.extend([cid, m.g32(4), 0, 0, 0])
        return ok
    return impl


_C_BBC0, _P_BBC0 = _stubs({0x3EF020: (0,), 0x3EF060: (0,), 0x3EF000: (1, {"ret": "void"}), 0x3EF040: (1, {"ret": "void"}), 0x3E3560: (2, {"ret": "void"})})
_C_BBC0.update({0x3E2910: C_E2910_W, 0x3DF050: C_DF050B, 0x3DF4D0: C_DF4D0E, 0x3E4F60: C_E4F60, 0x3EF2E0: C_EF2E0, 0x3EF470: C_EF470, 0x3EF510: C_EF510})
_P_BBC0.update({0x3E2910: _py_e2910_w, 0x3DF050: _py_df050b, 0x3DF4D0: _py_df4d0e, 0x3E4F60: _mk_e4f60(0x3E4F60, 0), 0x3EF2E0: _mk_e4f60(0x3EF2E0, 0),
                0x3EF470: _mk_e4f60(0x3EF470, 0), 0x3EF510: _mk_e4f60(0x3EF510, 0), 0x43F9C8: lambda m: 0})


def gen_bbc0(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x80, fill_random=False)
    img.w32(obj + 0x54, rng.choice([0, 1, 1, 2]))
    img.callee_ret[0x3EF2E0] = rng.choice([0, obj, obj, obj])
    img.callee_ret[0x3E2910] = rng.choice([0x18, 0x18, 0x18, 4])
    img.callee_ret[0x3E4F60] = 0xFE000000 | rng.randrange(0, 4)
    img.callee_ret[0x3EF020] = rng.randrange(1 << 16)
    img.callee_ret[0x3EF060] = rng.randrange(1 << 16)
    img.callee_ret[0x3DF050] = rng.choice([0xFE000000 | 1, 0xFE000000 | 1, 0xFE000000 | 0])
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


_C_AEF0, _P_AEF0 = _stubs({0x3E9700: (1,), 0x3DF590: (1,)})
_C_AEF0.update({0x3DF4D0: C_DF4D0E})
_P_AEF0.update({0x3DF4D0: _py_df4d0e})


def gen_aef0(rng):
    import mips as _m
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    for a in (0x00764804, 0x00764808, 0x008872E4 + 0x10):
        img.touch(a)
    img.w32(0x00764804, 0x40)
    img.w32(0x00764808, 0x10)
    img.w32(0x008872E4 + 0x10, 0x1234)
    obj = img.alloc(0x100, fill_random=False)
    world = img.alloc(0x40, fill_random=False)
    rec = obj + 0x40
    img.w32(rec, world if rng.random() < 0.9 else 0)
    img.hook(rec + 4, 1)
    img.callee_ret[0x500001] = rng.choice([1, 1, 1, 0])
    img.mem.wb(obj + 1, rng.choice([0x80, 0xFF, 0x90, 0x10, 0x7F]))
    frame = rng.choice([0x1000, 0x2000, 0]) if rng.random() < 0.1 else 0x1000
    img.w32(obj + 4, frame)
    img.w32(obj + 0x14, _m.f2b(rng.choice([0.1, 0.5, 1.0, 2.0])))

    def leaf():
        lf = img.alloc(0x50, fill_random=False)
        img.w32(lf, 0xFFFFFFFF)
        img.w32(lf + 0x40, lf + 0x40)
        img.w32(lf + 0x44, lf + 0x40)
        return lf

    def tree(d):
        if d == 0 or rng.random() < 0.25:
            return leaf()
        nd = img.alloc(0x20, fill_random=False)
        img.w32(nd, rng.choice([0, 4, 8]))
        img.w32(nd + 0x10, _m.f2b(rng.uniform(-2, 2)))
        img.w32(nd + 0x14, _m.f2b(rng.uniform(-2, 2)))
        img.w32(nd + 8, tree(d - 1))
        img.w32(nd + 0xC, tree(d - 1))
        return nd
    root = tree(3)
    img.w32(world + 0x1C, root)
    # existing links of the object, all in one extra sector
    sec = leaf()
    k = rng.randrange(0, 4)
    pn = [img.alloc(0x20, fill_random=False) for _ in range(k)]
    sent_o = obj + 0x2C
    for i, p_ in enumerate(pn):
        img.w32(p_, pn[i + 1] if i + 1 < k else sec + 0x40)
        img.w32(p_ + 4, pn[i - 1] if i > 0 else sec + 0x40)
        img.w32(p_ + 8, obj)
        img.w32(p_ + 0xC, pn[i + 1] + 0xC if i + 1 < k else sent_o)
        img.w32(p_ + 0x10, pn[i - 1] + 0xC if i > 0 else sent_o)
    img.w32(sec + 0x40, pn[0] if k else sec + 0x40)
    img.w32(sec + 0x44, pn[-1] if k else sec + 0x40)
    img.w32(sent_o, pn[0] + 0xC if k else sent_o)
    img.w32(sent_o + 4, pn[-1] + 0xC if k else sent_o)
    mat = img.alloc(0x60, fill_random=False)
    for i in range(12, 15):
        img.w32(mat + 4 * i, _m.f2b(rng.uniform(-2, 2)))
    img.callee_ret[0x3E9700] = mat
    img.hook(0x008873F8, 3)
    img.hook(0x008873FC, 0)
    return rn.Case([obj], img, ret=True)


_C7CF, _P7CF = _hstubs({0x3E05D0: ([("h", 16)], {"ret": "void"}), 0x3E0E20: (["i", ("h", 16), "i"], {"ret": "void"}), 0x39A700: (["i", "i", "i"], {"ret": "void"})})
_C7CF.update(_C6F)
_P7CF.update(_P6F)


def gen_7cf0(rng):
    import mips as _m
    img = rn.Img(rng)
    img.touch(0x0076483C)
    img.w32(0x0076483C, 0x40)
    for k in range(3):
        img.touch(0x0070B6B0)
        img.w32(0x0070B6B0 + 4 * k, _m.f2b(rng.uniform(-1, 1)))
    a0 = img.alloc(0x100, fill_random=False)
    tab = a0 + 0x40
    for k in range(2):
        if rng.random() < 0.85:
            mm = img.alloc(0x40, fill_random=True)
            img.w32(tab + 4 * k, mm)
    for k in range(8):
        if rng.random() < 0.3:
            continue
        cnt = rng.randrange(0, 4)
        stride = rng.choice([0, 0x10, 0x20, 0x24])
        r = img.alloc(0x4C + max(cnt, 1) * 0x24 + 0x40, fill_random=False)
        for q in range(0x4C, 0x4C + max(cnt, 1) * 0x24 + 0x40, 4):
            img.w32(r + q, _m.f2b(rng.uniform(-3, 3)))
        X = img.alloc(0x40, fill_random=False)
        Y = img.alloc(0x20 + 4 * max(cnt, 1) + 16, fill_random=False)
        img.w32(r, X)
        img.w32(X + 0x14, Y)
        for q in range(cnt):
            img.w32(Y + 0x20 + 4 * q, rng.choice([0, 1, 0, 1, 2]))
        img.w32(r + 0x24, stride)
        img.w32(r + 0x2C, cnt)
        kind = rng.choice(["a", "b", "h"])
        if kind == "a":
            img.fnslot(r + 0x3C, 0x3D6900)
        elif kind == "b":
            img.fnslot(r + 0x3C, 0x3D6C50)
        else:
            img.hook(r + 0x3C, 0)
        img.w32(tab + 8 + 4 * k, r)
    return rn.Case([a0], img, ret=True)


C_QSORTR = ("void func_00440bb8(unsigned *b, unsigned n, unsigned sz, void *cmp) { unsigned i, t; p4_record({ID}, (unsigned)b, n, sz, 0); for (i = 0; i < n / 2; i++) { t = b[i]; b[i] = b[n - 1 - i]; b[n - 1 - i] = t; } }")
C_C59F0 = ("void func_003c59f0(unsigned b, void *cb, unsigned z) { p4_record({ID}, b, 0, z, 0); }")


def _py_qsortr(m):
    cid, n, ok = _ok(m, 0x440BB8)
    b, nn, sz = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, b, nn, sz, 0])
    v = [m.mem.rn(b + 4 * i, 4) for i in range(nn)]
    for i in range(nn):
        m.mem.wn(b + 4 * i, 4, v[nn - 1 - i])
    return 0


def _py_c59f0(m):
    cid, n, ok = _ok(m, 0x3C59F0)
    m.ilog.extend([cid, m.g32(4), 0, m.g32(6), 0])
    return 0


_C_8920, _P_8920 = _stubs({0x3DF590: (2,), 0x3C54A0: (1,)})
_C_8920.update({0x3DF4D0: C_DF4D0E, 0x440BB8: C_QSORTR, 0x3C59F0: C_C59F0})
_P_8920.update({0x3DF4D0: _py_df4d0e, 0x440BB8: _py_qsortr, 0x3C59F0: _py_c59f0})


def gen_8920(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.touch(0x007647E0)
    img.w32(0x007647E0, 0x20)
    img.mem.wn(0x008872E0 + 0x20, 2, rng.randrange(0, 0x8000))
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    n = rng.randrange(1, 6)
    tr = img.alloc(n * 0x14 + 16, fill_random=True)
    mats = [rng.choice([0x10, 0x20, 0x30]) for _ in range(n)]
    for i in range(n):
        img.w32(tr + i * 0x14 + 8, mats[i])
    a0 = img.alloc(0x20, fill_random=False)
    img.w32(a0 + 4, n)
    img.w32(a0 + 8, tr)
    blk = img.alloc(0x10 + 12 * 6 + 6 * 6 + 32, fill_random=True)
    img.callee_ret[0x3C54A0] = rng.choice([blk, blk, blk, 0])
    return rn.Case([a0], img, ret=True)


C_E40B0 = ("void RwV3dNormalize(float *d, float *s) { unsigned h = 0, i; for (i = 0; i < 3; i++) h = h * 31u + ((unsigned *)s)[i]; p4_record({ID}, 0, h, 0, 0); for (i = 0; i < 3; i++) d[i] = s[i] * 0.5f + (float)(i + 1); }")


def _py_e40b0(m):
    import mips as _m
    cid, n, ok = _ok(m, 0x3E40B0)
    d, s = m.g32(4), m.g32(5)
    w = [m.mem.rn(s + 4 * i, 4) for i in range(3)]
    h = 0
    for x in w:
        h = (h * 31 + x) & 0xFFFFFFFF
    m.ilog.extend([cid, 0, h, 0, 0])
    for i in range(3):
        m.mem.wn(d + 4 * i, 4, _m.f2b(_m.b2f(w[i]) * 0.5 + (i + 1)))
    return 0


_C_C740, _P_C740 = _stubs({0x3DF590: (1,)})
_C_C740.update({0x3DF4D0: C_DF4D0E, 0x3E40B0: C_E40B0})
_P_C740.update({0x3DF4D0: _py_df4d0e, 0x3E40B0: _py_e40b0})


def gen_c740(rng):
    import mips as _m
    img = rn.Img(rng)
    q = img.alloc(0x20, fill_random=False)
    ax = img.alloc(0x20, fill_random=False)
    for i in range(4):
        img.w32(q + 4 * i, _m.f2b(rng.uniform(-1, 1)))
    for i in range(3):
        img.w32(ax + 4 * i, _m.f2b(rng.uniform(-1, 1)))
    ang = _m.f2b(rng.choice([rng.uniform(-180, 180), 90.0, 0.0]))
    img.fregs = {12: ang}
    a0 = 0 if rng.random() < 0.05 else q
    a1 = 0 if rng.random() < 0.05 else ax
    return rn.Case([a0, a1, rng.choice([0, 1, 2, 0, 1, 2, 3, 5]), ang], img, ret=True)


_C_2630, _P_2630 = _stubs({0x3DF590: (2,), 0x3C4A40: (1,), 0x3E3B70: (2, {"ret": "void"}), 0x3C49A0: (1, {"ret": "void"}), 0x3C1EA0: (2,)})
_C_2630.update({0x3DF4D0: C_DF4D0E, 0x43F9C8: "void *func_0043f9c8(void *d, int c, unsigned n) { p4_record({ID}, (unsigned)d, c, n, 0); return d; }"})
_P_2630.update({0x3DF4D0: _py_df4d0e})


def _py_memset_log(m):
    cid, n, ok = _ok(m, 0x43F9C8)
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), 0])
    return m.g32(4)


_P_2630[0x43F9C8] = _py_memset_log


def gen_2630(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.touch(0x0070AFB0)
    img.w32(0x0070AFB0, rng.choice([0x60, 0x80, 0x64]))
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    nv = rng.choice([0, 1, 5, 20, 0x10000, -1, 0xFFFF, 3]) 
    nt = rng.choice([0, 1, 7, 9, 20, -1, 3])
    fl = rng.choice([0, 0x4, 0x80, 0x8, 0x1000000, 0x1000008, 0x30000, 0x0B0000, 0x5000004, 0xFFFFFF, 0xF000084, 0x0A0008, 0x120000])
    img.callee_ret[0x3C4A40] = rng.choice([1, 1, 1, 0])
    img.callee_ret[0x3C1EA0] = rng.choice([0, 0, 5, 0xFFFFFFFF])
    return rn.Case([nv & 0xFFFFFFFF, nt & 0xFFFFFFFF, fl], img, ret=True)


C_E2910_B = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned i, r = p4_record({ID}, s, 0, n, 0); for (i = 0; i < 7; i++) p[i] = s * (i + 3u); p[3] = (s >> 2) & 1u; return r; }")
C_E3110S = ("unsigned func_003e3110(unsigned l, unsigned s, unsigned n) { unsigned r = p4_record({ID}, l, s, n, 0); *(unsigned *)0x007647CC = ((r >> 4) & 0xFu) ? 0x1000u + ((r >> 4) & 0xFu) : 0u; *(unsigned *)0x007647C8 = r * 3u; return r & 1u; }")


def _py_e2910_b(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    for i in range(7):
        m.mem.wn(p + 4 * i, 4, (s * (i + 3)) & 0xFFFFFFFF)
    m.mem.wn(p + 12, 4, (s >> 2) & 1)
    return ok


def _py_e3110s(m):
    cid, n, ok = _ok(m, 0x3E3110)
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), 0])
    m.mem.wn(0x007647CC, 4, (0x1000 + ((ok >> 4) & 0xF)) if ((ok >> 4) & 0xF) else 0)
    m.mem.wn(0x007647C8, 4, (ok * 3) & 0xFFFFFFFF)
    return ok & 1


_C_4390, _P_4390 = _stubs({0x3DF590: (1,), 0x3E3B70: (2, {"ret": "void"}), 0x3E3C20: (2, {"ret": "void"}), 0x3EF3A0: (1, {"ret": "void"}), 0x3E32F0: (4, {"ret": "void"}), 0x3E6430: (1,)})
_C_4390.update({0x3DF050: C_DF050B, 0x3DF4D0: C_DF4D0E, 0x3E2910: C_E2910_B, 0x3E3110: C_E3110S})
_P_4390.update({0x43F9C8: lambda m: 0, 0x3DF050: _py_df050b, 0x3DF4D0: _py_df4d0e, 0x3E2910: _py_e2910_b, 0x3E3110: _py_e3110s})


def gen_4390(rng):
    img = rn.Img(rng)
    img.pool()
    for a in (0x00887000, 0x007647C8, 0x007647D0, 0x008872F0, 0x0070B008):
        img.touch(a)
    img.w32(0x007647D0, 0x10)
    img.w32(0x008872F0, 0x4321)
    for k in range(3):
        img.w32(0x0070B008 + 4 * k, rng.randrange(1 << 32))
    img.hook(0x008873F8, 3)
    img.hook(0x008873FC, 0)
    img.callee_ret[0x3DF050] = rng.choice([0xFE000000 | 2, 0xFE000000 | 2, 0xFE000000 | 1, 0xFE000000 | 0, 0xFE000000 | 3])
    img.callee_ret[0x3E2910] = rng.choice([0x18, 0x18, 0x18, 4])
    img.callee_ret[0x3E3110] = rng.randrange(0, 256)
    img.callee_ret[0x3E6430] = rng.randrange(0, 4) * 0x100
    img.callee_ret[0x500000] = 0
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


def gen_7d90(rng):
    img = rn.Img(rng)
    img.pool()
    for a in (0x00887000, 0x007647E0, 0x008872F0):
        img.touch(a)
    img.w32(0x007647E0, 0x0C)
    img.w32(0x008872F0, 0x2468)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    img.hook(0x008873F8, 3)
    img.hook(0x008873FC, 0)
    k = rng.randrange(1, 5)
    nodes = []
    for _ in range(k):
        cnt = rng.randrange(2, 5)
        arr = img.alloc(cnt * 2 + 8, fill_random=False)
        for i in range(cnt):
            img.mem.wn(arr + 2 * i, 2, rng.randrange(0, 4))
        nd = img.alloc(0x10, fill_random=False)
        img.w32(nd, arr)
        img.w32(nd + 4, cnt)
        nodes.append(nd)
    for i, nd in enumerate(nodes):
        img.w32(nd + 0xC, nodes[i + 1] if i + 1 < k else 0)
    a0 = img.alloc(0x10, fill_random=False)
    img.w32(a0, nodes[0] if rng.random() < 0.95 else 0)
    return rn.Case([a0, rng.choice([0, 1, 1])], img, ret=True)


_C_FA80, _P_FA80 = _stubs({0x3E9700: (1,)})


def gen_fa80(rng):
    import mips as _m
    img = rn.Img(rng)
    img.touch(0x00764810)
    buf = img.alloc(0x200, fill_random=True)
    img.w32(0x00764814, buf)
    img.w32(0x00764810, rng.choice([0, 0, 0, 5, 0x7B, 0x7C, 0x80]))
    o = img.alloc(0x40, fill_random=True)
    img.mem.wb(o + 1, rng.choice([0, 1, 2, 0x80, 0x81, 0x82, 3, 0x83]))
    img.w32(o + 4, rng.choice([0x1000, 0x1000, 0]) if rng.random() < 0.1 else 0x1000)
    mat = img.alloc(0x80, fill_random=True)
    img.callee_ret[0x3E9700] = mat
    m1 = img.alloc(0x80, fill_random=True) if rng.random() < 0.6 else 0
    f2 = _m.f2b(rng.uniform(-3, 3))
    f3 = _m.f2b(rng.uniform(-3, 3))
    img.fregs = {12: f2, 13: f3}
    return rn.Case([o, m1, f2, f3], img, ret=False)


C_E4030V = ("float func_003e4030(float *v) { unsigned r = p4_record({ID}, *(unsigned *)&v[0], *(unsigned *)&v[1], *(unsigned *)&v[2], 0); return (float)(r % 3u) * 0.75f; }")


def _py_e4030v(m):
    import mips as _m
    cid, n, ok = _ok(m, 0x3E4030)
    p = m.g32(4)
    w = [m.mem.rn(p + 4 * i, 4) for i in range(3)]
    m.ilog.extend([cid, w[0], w[1], w[2], 0])
    m.f[0] = _m.f2b((ok % 3) * 0.75)
    return 0


_C_B5B0, _P_B5B0 = _stubs({0x3DF590: (1,)})
_C_B5B0.update({0x3DF4D0: C_DF4D0E, 0x3E4030: C_E4030V})
_P_B5B0.update({0x3DF4D0: _py_df4d0e, 0x3E4030: _py_e4030v})


def gen_b5b0(rng):
    import mips as _m
    img = rn.Img(rng)
    for k, a in enumerate((0x0076470C, 0x00764710, 0x00764714, 0x00764718, 0x0076471C, 0x00764720, 0x00764724, 0x00764728)):
        img.touch(a)
        tab = img.alloc(260 * 4, fill_random=False)
        for i in range(258):
            img.w32(tab + 4 * i, _m.f2b(rng.uniform(-1, 1)))
        img.w32(a, tab)
    n = rng.randrange(4, 9)
    cv = img.alloc(0x40 + n * 12 + 16, fill_random=False)
    img.w32(cv + 8, n)
    mode = rng.choice([0, 1, 2, 2])
    img.w32(cv + 0xC, mode)
    for i in range(n * 3):
        img.w32(cv + 0x38 + 4 * i, _m.f2b(rng.uniform(-5, 5)))
    o1 = img.alloc(0x10, fill_random=True)
    o2 = img.alloc(0x10, fill_random=True) if rng.random() < 0.7 else 0
    tv = rng.choice([rng.uniform(-3, 3), rng.uniform(0, 1), 1.0, 2.0, 0.5, 0.0, -1.0, 1.5, 3.0, rng.uniform(1, 40), 3.0e9, 2.5e9, 4.1e9])
    if mode != 2:
        tv = rng.choice([rng.uniform(-1, 1), 1.0, 0.5, 0.0, -1.0, rng.uniform(0, 1)])
    t = _m.f2b(tv)
    img.fregs = {12: t}
    img.callee_ret[0x3E4030] = rng.randrange(1 << 16)
    mode_a = rng.choice([0xA, 0xB, 0xB, 0xA, 0x5])
    case = rn.Case([cv, mode_a, o1, o2], img, ret=True)
    case.cargs = [cv, mode_a, t, o1, o2]
    return case


C_DF590_2 = ("unsigned func_003df590(unsigned a, unsigned b) { return p4_record({ID}, a, a == 0x80000013u ? b : 0u, 0, 0); }")


def _py_df590_2(m):
    cid, n, ok = _ok(m, 0x3DF590)
    a, b = m.g32(4), m.g32(5)
    m.ilog.extend([cid, a, b if a == 0x80000013 else 0, 0, 0])
    return ok


C_E2910_G = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned i, r = p4_record({ID}, s, 0, n, 0); for (i = 0; i < 16; i++) p[i] = s * (i + 3u); "
             "p[0] = (s >> 2) & 1u; p[4] = (s >> 4) % 4u; p[5] = (s >> 6) % 4u; p[6] = (s >> 8) % 3u; p[7] = (s >> 10) % 3u; p[8] = ((s >> 3) & 3u) ? 0u : 1u; "
             "{ static const unsigned F[8] = {0u, 4u, 0x80u, 0x1000000u, 0x1000008u, 0x30018u, 0x20000u, 0x1Cu}; p[9] = F[(s >> 12) & 7u]; } return r; }")


def _py_e2910_g(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    w = [(s * (i + 3)) & 0xFFFFFFFF for i in range(16)]
    w[0] = (s >> 2) & 1
    w[4] = (s >> 4) % 4
    w[5] = (s >> 6) % 4
    w[6] = (s >> 8) % 3
    w[7] = (s >> 10) % 3
    w[8] = 0 if ((s >> 3) & 3) else 1
    w[9] = [0, 4, 0x80, 0x1000000, 0x1000008, 0x30018, 0x20000, 0x1C][(s >> 12) & 7]
    for i in range(16):
        m.mem.wn(p + 4 * i, 4, w[i])
    return ok


C_C4FA0T = ("int func_003c4fa0(unsigned s, unsigned *p) { unsigned r = p4_record({ID}, s, 0, 0, 0); p[0] = *(unsigned *)0x00887100; p[1] = *(unsigned *)0x00887104; return r; }")


def _py_c4fa0t(m):
    cid, n, ok = _ok(m, 0x3C4FA0)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    p = m.g32(5)
    m.mem.wn(p, 4, m.mem.rn(0x00887100, 4))
    m.mem.wn(p + 4, 4, m.mem.rn(0x00887104, 4))
    return ok


C_E3110T = ("unsigned func_003e3110(unsigned l, unsigned s, unsigned n) { unsigned r = p4_record({ID}, l, s, n, 0); *(unsigned *)0x00764794 = ((r >> 4) & 0xFu) ? 0x1000u + ((r >> 4) & 0xFu) : 0u; *(unsigned *)0x00764790 = r * 3u; return r & 1u; }")


def _py_e3110t(m):
    cid, n, ok = _ok(m, 0x3E3110)
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), 0])
    m.mem.wn(0x00764794, 4, (0x1000 + ((ok >> 4) & 0xF)) if ((ok >> 4) & 0xF) else 0)
    m.mem.wn(0x00764790, 4, (ok * 3) & 0xFFFFFFFF)
    return ok & 1


C_BD6B0S = ("unsigned func_003bd6b0(unsigned s, unsigned *d, unsigned b, unsigned f) { return p4_record({ID}, s, 0, b, f); } unsigned func_003bdd00(unsigned s, unsigned *d, unsigned b, unsigned f) { return p4_record({ID2}, s, 0, b, f); }")


def _mk_bd(addr):
    def impl(m):
        cid, n, ok = _ok(m, addr)
        m.ilog.extend([cid, m.g32(4), 0, m.g32(6), m.g32(7)])
        return ok
    return impl


def _py_memset_heap(m):
    cid, n, ok = _ok(m, 0x43F9C8)
    if m.g32(4) < 0x80000000:        # the zeroing of a stack buffer is a plain initialiser on the C side
        m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), 0])
    return m.g32(4)


_C_E180, _P_E180 = _stubs({0x3C9940: (1, {"ret": "void"}), 0x3C9D40: (1, {"ret": "void"}), 0x3C97E0: (1, {"ret": "void"}), 0x3C9E80: (2, {"ret": "void"}), 0x3C42B0: (2, {"ret": "void"}), 0x3E3B70: (2, {"ret": "void"}), 0x3E32F0: (4, {"ret": "void"}), 0x3C9B30: (1,)})
_C_E180.update({0x43F9C8: "void *func_0043f9c8(void *d, int c, unsigned n) { p4_record({ID}, (unsigned)d, c, n, 0); return d; }", 0x3DF050: C_DF050B, 0x3DF4D0: C_DF4D0E, 0x3DF590: C_DF590_2, 0x3E2910: C_E2910_G, 0x3C4FA0: C_C4FA0T, 0x3E3110: C_E3110T,
                0x3BD6B0: "unsigned func_003bd6b0(unsigned s, unsigned *d, unsigned b, unsigned f) { return p4_record({ID}, s, 0, b, f); }",
                0x3BDD00: "unsigned func_003bdd00(unsigned s, unsigned *d, unsigned b, unsigned f) { return p4_record({ID}, s, 0, b, f); }"})
_P_E180.update({0x43F9C8: _py_memset_heap, 0x3DF050: _py_df050b, 0x3DF4D0: _py_df4d0e, 0x3DF590: _py_df590_2, 0x3E2910: _py_e2910_g, 0x3C4FA0: _py_c4fa0t, 0x3E3110: _py_e3110t,
                0x3BD6B0: _mk_bd(0x3BD6B0), 0x3BDD00: _mk_bd(0x3BDD00)})


def gen_e180(rng):
    img = rn.Img(rng)
    img.pool()
    for a in (0x00887000, 0x00764790, 0x0070B040, 0x00887100):
        img.touch(a)
    img.touch(0x0070B060)
    img.w32(0x0070B060, rng.choice([0x80, 0x60]))
    img.w32(0x0070B040, rng.choice([0x10, 0x20]))
    img.hook(0x008873E8, 2)
    ne = rng.randrange(0, 4)
    tab = img.alloc(16 + 4 * ne, fill_random=False)
    for i in range(ne):
        e = img.alloc(0x10, fill_random=False)
        img.w32(e, rng.choice([0, e]))
        img.w32(tab + 4 * i, e)
    img.w32(0x00887100, tab)
    img.w32(0x00887104, ne)
    img.callee_ret[0x3DF050] = rng.choice([0xFE000000 | 4, 0xFE000000 | 4, 0xFE000000 | 2, 0xFE000000 | 3, 0xFE000000 | 1, 0xFE000000 | 0])
    img.callee_ret[0x3E2910] = rng.choice([0x18, 0x18, 0x18, 4])
    img.callee_ret[0x3C4FA0] = rng.choice([1, 1, 1, 0])
    img.callee_ret[0x3BD6B0] = rng.choice([0, 7, 7])
    img.callee_ret[0x3BDD00] = rng.choice([0, 9, 9])
    img.callee_ret[0x3E3110] = rng.randrange(0, 256)
    img.callee_ret[0x3C9B30] = rng.choice([0, 1, 1, 1])
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


def gen_9760(rng):
    import mips as _m
    img = rn.Img(rng)

    def vec(n, lo, hi):
        a = img.alloc(16 + 4 * n, fill_random=False)
        for i in range(n):
            img.w32(a + 4 * i, _m.f2b(rng.choice([rng.uniform(lo, hi), float(rng.randrange(-2, 3))])))
        return a
    sph = vec(4, -1.5, 1.5)
    img.w32(sph + 12, _m.f2b(rng.choice([0.5, 1.0, 2.0, 0.1, rng.uniform(0.05, 3)])))
    span = rng.choice([1.0, 2.5, 5.0])
    v0, v1, v2 = vec(3, -span, span), vec(3, -span, span), vec(3, -span, span)
    n = img.alloc(0x10, fill_random=True)
    d = img.alloc(0x10, fill_random=True)
    return rn.Case([sph, v0, v1, v2, n, d], img, ret=True)


def gen_1730(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.touch(0x00752D20)
    for k in range(9):
        img.w32(0x00752D20 + 4 * k, rng.choice([4, 8, 12, 16, 0x20, 0x40]))
    img.hook(0x008873E8, 2)
    g = img.alloc(0xC0, fill_random=True)
    img.w32(g, rng.randrange(0, 50))
    return rn.Case([g, rng.choice([0, 1, 0x1FF, rng.randrange(0, 0x200), rng.randrange(0, 0x200), 0x100, 0x3FF])], img, ret=True)


_C_85E0, _P_85E0 = _stubs({0x3B83F0: (0,)})


def gen_85e0(rng):
    import mips as _m
    img = rn.Img(rng)
    n = rng.randrange(0, 6)
    a0 = img.alloc(0x40, fill_random=False)
    img.w32(a0 + 8, rng.choice([0, 0, 0x1000000, 5]))
    img.w32(a0 + 0x14, n)
    r = img.alloc(0x60, fill_random=False)
    idx = img.alloc(4 * n + 16, fill_random=True)
    w = img.alloc(16 * n + 16, fill_random=False)
    for i in range(4 * n):
        img.w32(w + 4 * i, rng.choice([0, 0x80000000, _m.f2b(rng.uniform(-2, 2)), _m.f2b(rng.uniform(0, 1)), rng.randrange(1 << 32)]))
    tab = img.alloc(0x110, fill_random=True)
    img.w32(r + 0x14, idx)
    img.w32(r + 0x18, w)
    img.w32(r + 0x30, rng.choice([0, 1]))
    img.w32(r + 0x38, tab)
    for k in range(4):
        img.w32(r + 0x1C + 4 * k, rng.randrange(0, 5))
    img.callee_ret[0x3B83F0] = r
    return rn.Case([a0], img, ret=True)


C_EC1F0 = ("void func_003ec1f0(unsigned r, unsigned k, unsigned *a, unsigned *b, unsigned *c, unsigned *d) { unsigned n = p4_ncalls[{ID}]; p4_record({ID}, r, k, 0, 0); *a = 0x20u + n; *b = 3u; *c = 0x10u; *d = (n + 1u) * 0x101u; }")
C_99470S = ("void func_00399470(unsigned *rec, unsigned a, unsigned b) { p4_record({ID}, 0, a, b, 0); }")
C_EF470S = ("void func_003ef470(unsigned o, unsigned *rec) { p4_record({ID}, o, 0, 0, 0); }")


def _py_ec1f0(m):
    cid, n, ok = _ok(m, 0x3EC1F0)
    m.ilog.extend([cid, m.g32(4), m.g32(5), 0, 0])
    m.mem.wn(m.g32(6), 4, 0x20 + n)
    m.mem.wn(m.g32(7), 4, 3)
    m.mem.wn(m.g32(8), 4, 0x10)
    m.mem.wn(m.g32(9), 4, ((n + 1) * 0x101) & 0xFFFFFFFF)
    return 0


def _py_99470s(m):
    cid, n, ok = _ok(m, 0x399470)
    m.ilog.extend([cid, 0, m.g32(5), m.g32(6), 0])
    return 0


def _py_ef470s(m):
    cid, n, ok = _ok(m, 0x3EF470)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    return 0


_C_9530, _P_9530 = _stubs({0x3EA2C0: (3,), 0x3EA3E0: (1, {"ret": "void"}), 0x3EC110: (2, {"ret": "void"}), 0x3ED0B0: (2, {"ret": "void"}), 0x3EA370: (1, {"ret": "void"}),
                           0x3EA560: (1, {"ret": "void"}), 0x3EA690: (2, {"ret": "void"}), 0x3EC590: (4,), 0x3EC180: (2, {"ret": "void"}), 0x3EF2E0: (1,)})
_C_9530.update({0x3EC1F0: C_EC1F0, 0x399470: C_99470S, 0x3EF470: C_EF470S})
_P_9530.update({0x3EC1F0: _py_ec1f0, 0x399470: _py_99470s, 0x3EF470: _py_ef470s})


def gen_9530(rng):
    img = rn.Img(rng)
    ras = img.alloc(0x40, fill_random=False)
    data = img.alloc(8 * 8 * 4 + 64, fill_random=True)
    stride = 8 * 4
    img.w32(ras + 0x10, stride)
    img.w32(ras + 0x14, data)
    obj = img.alloc(0x60, fill_random=True)

    def texture(w, h):
        r1 = img.alloc(0x30, fill_random=False)
        img.w32(r1 + 0xC, w)
        img.w32(r1 + 0x10, h)
        img.mem.wb(r1 + 0x23, rng.choice([0, 0x80, 0x7F, 0xFF]))
        t = img.alloc(0x60, fill_random=True)
        img.w32(t, r1)
        return t
    a1 = texture(rng.randrange(0, 9), rng.randrange(0, 9))
    a0 = texture(rng.randrange(0, 9), rng.randrange(0, 9)) if rng.random() < 0.6 else 0
    img.callee_ret[0x3EA2C0] = ras
    img.callee_ret[0x3EC590] = 0x1234
    img.callee_ret[0x3EF2E0] = obj
    return rn.Case([a0, a1], img, ret=True)


C_9760S = ("int func_003d9760(float *c, float *a, float *b, float *d, float *n, float *dist) { unsigned h[3] = {0, 0, 0}, i; for (i = 0; i < 3; i++) { h[0] = h[0] * 31u + ((unsigned *)a)[i]; h[1] = h[1] * 31u + ((unsigned *)b)[i]; h[2] = h[2] * 31u + ((unsigned *)d)[i]; } "
           "{ unsigned r = p4_record({ID}, h[0], h[1], h[2], (unsigned)c); n[0] = 1.0f * (float)(1u + (r & 3u)); n[1] = 2.0f; n[2] = 3.0f; *dist = 2.5f; return r != 0; } }")


def _py_9760s(m):
    import mips as _m
    cid, n, ok = _ok(m, 0x3D9760)
    hs = []
    for reg in (5, 6, 7):
        p_, h = m.g32(reg), 0
        for i in range(3):
            h = (h * 31 + m.mem.rn(p_ + 4 * i, 4)) & 0xFFFFFFFF
        hs.append(h)
    m.ilog.extend([cid, hs[0], hs[1], hs[2], m.g32(4)])
    nn = m.g32(8)
    m.mem.wn(nn, 4, _m.f2b(float(1 + (ok & 3))))
    m.mem.wn(nn + 4, 4, _m.f2b(2.0))
    m.mem.wn(nn + 8, 4, _m.f2b(3.0))
    m.mem.wn(m.g32(9), 4, _m.f2b(2.5))
    return 1 if ok else 0


def gen_4870(rng):
    import mips as _m
    img = rn.Img(rng)
    nv = 6
    arrs = []
    for _ in range(2):
        a = img.alloc(12 * nv + 16, fill_random=False)
        for i in range(3 * nv):
            img.w32(a + 4 * i, _m.f2b(rng.uniform(-2, 2)))
        arrs.append(a)
    morph = img.alloc(28 * 3 + 16, fill_random=False)
    for k in range(3):
        img.w32(morph + 28 * k + 0x14, arrs[k % 2] if k < 2 else arrs[0])
    n = rng.randrange(0, 4)
    tris = img.alloc(8 * max(n, 1) + 16, fill_random=False)
    for t in range(n):
        for k in range(3):
            img.mem.wn(tris + 8 * t + 2 * k, 2, rng.randrange(0, nv))
    g = img.alloc(0x80, fill_random=False)
    img.w32(g + 0x10, n)
    img.w32(g + 0x2C, tris)
    img.w32(g + 0x5C, morph)
    a1 = img.alloc(0x20, fill_random=False)
    img.mem.wn(a1 + 4, 2, rng.choice([0, 0, 1, 2]))
    img.mem.wn(a1 + 6, 2, rng.choice([0, 1, 1, 2]))
    img.w32(a1 + 0xC, _m.f2b(rng.uniform(0, 1)))
    img.w32(a1 + 0x10, _m.f2b(rng.choice([1.0, 0.5])))
    sph = img.alloc(0x20, fill_random=False)
    for i in range(3):
        img.w32(sph + 4 * i, _m.f2b(rng.uniform(-1, 1)))
    img.w32(sph + 12, _m.f2b(rng.choice([0.5, 1.0, 2.0])))
    img.callee_ret[0x3D9760] = 0xFD000000 | rng.randrange(0, 256) | 1
    img.callee_ret[0x500007] = 0xFD000000 | rng.randrange(0, 256) | 1
    return rn.Case([g, a1, sph, rng.randrange(1 << 32), 0x500007, rng.randrange(1 << 32)], img, ret=False)


def gen_2650(rng):
    import mips as _m
    img = rn.Img(rng)
    nv = 6
    arr = img.alloc(12 * nv + 16, fill_random=False)
    for i in range(3 * nv):
        img.w32(arr + 4 * i, _m.f2b(rng.uniform(-1.5, 1.5)))
    morph = img.alloc(28 + 16, fill_random=False)
    img.w32(morph + 0x14, arr)
    n = rng.randrange(0, 5)
    tris = img.alloc(8 * max(n, 1) + 16, fill_random=False)
    for t in range(n):
        for k in range(3):
            img.mem.wn(tris + 8 * t + 2 * k, 2, rng.randrange(0, nv))
    g = img.alloc(0x80, fill_random=False)
    img.w32(g + 0x10, n)
    img.w32(g + 0x2C, tris)
    img.w32(g + 0x5C, morph)
    ray = img.alloc(0x20, fill_random=False)
    for i in range(6):
        img.w32(ray + 4 * i, _m.f2b(rng.uniform(-2, 2)))
    img.callee_ret[0x500007] = 0xFD000000 | rng.randrange(0, 256) | 1
    return rn.Case([g, ray, rng.randrange(1 << 32), 0x500007, rng.randrange(1 << 32)], img, ret=False)


def gen_41e0(rng):
    import mips as _m
    case = gen_4870(rng)
    img = case.img
    ray = img.alloc(0x20, fill_random=False)
    for i in range(6):
        img.w32(ray + 4 * i, _m.f2b(rng.uniform(-2, 2)))
    case.args[2] = ray
    return case


def build_bsp(img, rng, nverts=8, ntris=6, bias=None, depth2=3, full2=False):
    """Two-level collision world: a kd tree of leaves, each pointing at a mesh with its own second-level tree. Returns the world record."""
    import mips as _m
    img.touch(0x007646C0)
    off = 0x18
    img.w32(0x007646C0, off)

    def fl(a, v):
        img.w32(a, _m.f2b(v))

    def make_mesh():
        verts = img.alloc(12 * nverts + 16, fill_random=False)
        for i in range(3 * nverts):
            fl(verts + 4 * i, rng.uniform(-1.5, 1.5))
        tris = img.alloc(8 * ntris + 16, fill_random=False)
        for t in range(ntris):
            for k in range(3):
                img.mem.wn(tris + 8 * t + 2 * k, 2, rng.randrange(0, nverts))
        recs = []

        def sub(depth):
            if depth == 0 or (not full2 and rng.random() < 0.35):
                return (rng.randrange(1, 4), rng.randrange(0, ntris - 2))
            idx = len(recs)
            recs.append(None)
            c0 = sub(depth - 1)
            c1 = sub(depth - 1)
            ax = rng.choice([0, 4, 8])
            if full2:
                ax = (((depth2 - depth) // 2) % 3) * 4
            pa, pb = rng.uniform(-1.2, 1.2), rng.uniform(-1.2, 1.2)
            if bias and rng.random() < 0.7:
                pa = bias[0][ax // 4] - rng.choice([0, 0, rng.uniform(0, 0.3)])
                pb = bias[1][ax // 4] + rng.choice([0, 0, rng.uniform(0, 0.3)])
                if bias[0] == bias[1] and rng.random() < 0.5:
                    pb = pa
            recs[idx] = (ax | rng.choice([0, 1, 0x10]), pa, pb, c0, c1)
            return (0xFF, idx)
        root = sub(depth2)
        base = img.alloc(16 * max(len(recs), 1) + 16, fill_random=False)
        for i, r in enumerate(recs):
            img.mem.wb(base + 16 * i, r[0])
            fl(base + 16 * i + 4, r[1])
            fl(base + 16 * i + 12, r[2])
            for j, ch in enumerate((r[3], r[4])):
                img.mem.wb(base + 16 * i + 8 * j + 1, ch[0])
                img.mem.wn(base + 16 * i + 8 * j + 2, 2, ch[1])
        mesh = img.alloc(0x40, fill_random=False)
        img.__dict__.setdefault('meshes', []).append((tris, verts, mesh))
        for j, k in enumerate((4, 8, 0xC)):
            fl(mesh + k, bias[1][j] if bias and rng.random() < 0.5 else rng.uniform(0.5, 2.5))
        for j, k in enumerate((0x10, 0x14, 0x18)):
            fl(mesh + k, bias[0][j] if bias and rng.random() < 0.5 else rng.uniform(-2.5, -0.5))
        if root[0] == 0xFF:
            img.mem.wn(mesh + 0x1E, 2, 1)
        else:
            img.mem.wn(mesh + 0x1E, 2, 0)
            img.mem.wn(mesh + 0x1C, 2, root[0])
        img.w32(mesh + 0x20, base)
        if rng.random() < 0.4:
            remap = img.alloc(2 * (ntris + 4) + 8, fill_random=False)
            for i in range(ntris + 4):
                img.mem.wn(remap + 2 * i, 2, rng.randrange(0, ntris))
            img.w32(mesh + 0x24, remap)
        return tris, verts, mesh

    def leaf():
        lf = img.alloc(0x30, fill_random=False)
        img.w32(lf, 0xFFFFFFFF)
        if rng.random() < 0.9:
            tris, verts, mesh = make_mesh()
            img.w32(lf + 4, tris)
            img.w32(lf + 8, verts)
            img.w32(lf + off, mesh)
        return lf

    def tree(d):
        if d == 0 or rng.random() < 0.3:
            return leaf()
        nd = img.alloc(0x20, fill_random=False)
        img.w32(nd, rng.choice([0, 4, 8]))
        fl(nd + 0x10, rng.uniform(-1.2, 1.2))
        fl(nd + 0x14, rng.uniform(-1.2, 1.2))
        img.w32(nd + 8, tree(d - 1))
        img.w32(nd + 0xC, tree(d - 1))
        return nd
    world = img.alloc(0x40, fill_random=False)
    img.w32(world + 0x1C, tree(3))
    return world


def gen_0680(rng):
    import mips as _m
    img = rn.Img(rng)
    world = build_bsp(img, rng)
    ray = img.alloc(0x20, fill_random=False)
    for i in range(6):
        img.w32(ray + 4 * i, _m.f2b(rng.choice([rng.uniform(-2, 2), 0.0, 1.0])))
    img.callee_ret[0x50000A] = 0xFD000000 | rng.randrange(0, 256) | 1
    return rn.Case([world, ray, 0x50000A, rng.randrange(1 << 32)], img, ret=False)


_C_1400, _P_1400 = _stubs({0x3D9F30: (4,)})


def gen_1400(rng):
    import mips as _m
    img = rn.Img(rng)
    lo = [rng.uniform(-1.5, 0.5) for _ in range(3)]
    hi = [lo[i] + rng.uniform(0.0, 1.5) for i in range(3)]
    if rng.random() < 0.6:
        hi = list(lo)
    world = build_bsp(img, rng, bias=(lo, hi), depth2=6, full2=rng.random() < 0.6)
    box = img.alloc(0x20, fill_random=False)
    for i in range(3):
        img.w32(box + 4 * i, _m.f2b(lo[i]))
        img.w32(box + 12 + 4 * i, _m.f2b(hi[i]))
    img.callee_ret[0x50000A] = 0xFD000000 | rng.randrange(0, 256) | 1
    img.callee_ret[0x3D9F30] = 0xFD000000 | rng.randrange(0, 256) | 1
    return rn.Case([world, box, 0x50000A, rng.randrange(1 << 32)], img, ret=False)


def gen_1c10(rng):
    import mips as _m
    img = rn.Img(rng)
    cen = [rng.uniform(-1.0, 1.0) for _ in range(3)]
    r = rng.choice([0.3, 0.5, 1.0, 2.0])
    h = _m.b2f(_m.f2b(0.57)) * r
    lo = [cen[i] - h for i in range(3)]
    hi = [cen[i] + h for i in range(3)]
    world = build_bsp(img, rng, bias=(lo, hi), depth2=6, full2=rng.random() < 0.6)
    sph = img.alloc(0x20, fill_random=False)
    for i in range(3):
        img.w32(sph + 4 * i, _m.f2b(cen[i]))
    img.w32(sph + 12, _m.f2b(r))
    img.callee_ret[0x50000A] = 0xFD000000 | rng.randrange(0, 256) | 1
    img.callee_ret[0x3D9760] = 0xFD000000 | rng.randrange(0, 256) | 1
    return rn.Case([world, sph, rng.randrange(1 << 32), 0x50000A, rng.randrange(1 << 32)], img, ret=False)


C_2650S = ("void func_00392650(unsigned g, unsigned ray, unsigned ctx, void *cb, unsigned ctx2) { p4_record({ID}, g, ray, ctx, ctx2); }")


def _py_2650s(m):
    cid, n, ok = _ok(m, 0x392650)
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), m.g32(8)])
    return 0


def gen_2bf0(rng):
    import mips as _m
    img = rn.Img(rng)
    img.touch(0x007646BC)
    img.w32(0x007646BC, 0x40)
    ray = img.alloc(0x20, fill_random=False)
    pts = [rng.uniform(-1.5, 1.5) for _ in range(6)]
    if rng.random() < 0.2:
        pts[3] = pts[0]
    if rng.random() < 0.2:
        pts[5] = pts[2]
    for i in range(6):
        img.w32(ray + 4 * i, _m.f2b(pts[i]))
    lo = [min(pts[i], pts[3 + i]) for i in range(3)]
    hi = [max(pts[i], pts[3 + i]) for i in range(3)]
    while not getattr(img, 'meshes', None):
        build_bsp(img, rng, bias=(lo, hi), depth2=5, full2=rng.random() < 0.5)
    tris, verts, mesh = img.meshes[0]
    for a_ in (0, 1, 2):
        if pts[a_] == pts[3 + a_] and rng.random() < 0.6:
            sh = rng.choice([0.7, -0.7, 3.0, -3.0])
            img.w32(ray + 4 * a_, _m.f2b(pts[a_] + sh))
            img.w32(ray + 4 * (3 + a_), _m.f2b(pts[a_] + sh))
    if rng.random() < 0.15:
        mesh = 0
    g = img.alloc(0x80, fill_random=False)
    img.w32(g + 0x2C, tris)
    morph = img.alloc(48, fill_random=False)
    img.w32(morph + 0x14, verts)
    img.w32(g + 0x5C, morph)
    img.w32(g + 0x40, mesh)
    img.callee_ret[0x500007] = 0xFD000000 | rng.randrange(0, 256) | 1
    return rn.Case([g, ray, rng.randrange(1 << 32), 0x500007, rng.randrange(1 << 32)], img, ret=False)


C_2A50S = ("int func_00392a50(unsigned g, unsigned s, unsigned ctx, void *cb, unsigned ctx2) { return p4_record({ID}, g, s, ctx, ctx2); }")


def _py_2a50s(m):
    cid, n, ok = _ok(m, 0x392A50)
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), m.g32(8)])
    return ok


def gen_3900(rng):
    import mips as _m
    img = rn.Img(rng)
    img.touch(0x007646BC)
    img.w32(0x007646BC, 0x40)
    cen = [rng.uniform(-1.0, 1.0) for _ in range(3)]
    r = rng.choice([0.3, 0.5, 1.0, 2.0])
    h = _m.b2f(_m.f2b(0.57)) * r
    lo = [cen[i] - h for i in range(3)]
    hi = [cen[i] + h for i in range(3)]
    while not getattr(img, 'meshes', None):
        build_bsp(img, rng, bias=(lo, hi), depth2=6, full2=rng.random() < 0.6)
    tris, verts, mesh = img.meshes[0]
    if rng.random() < 0.1:
        mesh = 0
    g = img.alloc(0x80, fill_random=False)
    img.w32(g + 0x2C, tris)
    morph = img.alloc(48, fill_random=False)
    img.w32(morph + 0x14, verts)
    img.w32(g + 0x5C, morph)
    img.w32(g + 0x40, mesh)
    sph = img.alloc(0x20, fill_random=False)
    for i in range(3):
        img.w32(sph + 4 * i, _m.f2b(cen[i]))
    img.w32(sph + 12, _m.f2b(r))
    img.callee_ret[0x500007] = 0xFD000000 | rng.randrange(0, 256) | 1
    img.callee_ret[0x3D9760] = 0xFD000000 | rng.randrange(0, 256) | 1
    img.callee_ret[0x392A50] = rng.randrange(0, 4)
    return rn.Case([g, sph, rng.randrange(1 << 32), 0x500007, rng.randrange(1 << 32)], img, ret=False)


C_E05F0H = ("void func_003e05f0(unsigned *d, unsigned *a, unsigned *b) { unsigned ha = 0, hb = 0, i; for (i = 0; i < 16; i++) { ha = ha * 31u + a[i]; hb = hb * 31u + b[i]; } p4_record({ID}, ha, hb, 0, 0); for (i = 0; i < 16; i++) d[i] = ha * 3u + hb + i * 7u; }")


def _py_e05f0h(m):
    cid, n, ok = _ok(m, 0x3E05F0)
    d, a, b = m.g32(4), m.g32(5), m.g32(6)
    ha = hb = 0
    for i in range(16):
        ha = (ha * 31 + m.mem.rn(a + 4 * i, 4)) & 0xFFFFFFFF
        hb = (hb * 31 + m.mem.rn(b + 4 * i, 4)) & 0xFFFFFFFF
    m.ilog.extend([cid, ha, hb, 0, 0])
    for i in range(16):
        m.mem.wn(d + 4 * i, 4, (ha * 3 + hb + i * 7) & 0xFFFFFFFF)
    return 0


_C_7480, _P_7480 = _stubs({0x3E9240: (1,), 0x3E9700: (1,), 0x3E9680: (1, {"ret": "void"}), 0x3954B0: (2, {"ret": "void"})})
_C_7480.update({0x3E05F0: C_E05F0H})
_P_7480.update({0x3E05F0: _py_e05f0h})


def gen_7480(rng):
    import mips as _m
    img = rn.Img(rng)
    img.touch(0x0088739C)
    head = img.alloc(0x40, fill_random=False)
    img.w32(0x0088739C, head)

    def frame(parent):
        f = img.alloc(0x100, fill_random=True)
        img.w32(f + 4, parent)
        return f
    chain = 0
    for _ in range(rng.randrange(0, 3)):
        chain = frame(chain)
    fr = frame(chain) if rng.random() < 0.85 else 0
    owner = img.alloc(0xC0, fill_random=True)
    img.w32(owner + 4, fr)
    rec = img.alloc(0x20, fill_random=False)
    img.mem.wb(rec + 3, rng.choice([0, 0, 1, 2, 0x10]))
    img.w32(owner + 0xA0, rec)
    nnodes = rng.randrange(0, 6)
    nodes = img.alloc(16 * max(nnodes, 1) + 16, fill_random=False)
    depth = 0
    for k in range(nnodes):
        nf = img.alloc(0x80, fill_random=True) if rng.random() < 0.8 else 0
        op = rng.randrange(0, 8)
        if op & 3 == 1 and depth == 0:
            op = 3
        elif op & 3 == 1:
            depth -= 1
        elif op & 3 == 2:
            depth += 1
        img.w32(nodes + 16 * k + 8, op)
        img.w32(nodes + 16 * k + 0xC, nf)
    stride = 0x30
    anim = img.alloc(0x4C + stride * max(nnodes, 1) + 16, fill_random=False)
    for q in range(0x4C, 0x4C + stride * max(nnodes, 1), 4):
        img.w32(anim + q, _m.f2b(rng.uniform(-1, 1)))
    img.w32(anim + 0x24, stride)
    if rng.random() < 0.6:
        img.fnslot(anim + 0x3C, 0x3954B0)
    else:
        img.hook(anim + 0x3C, 0)
    g = img.alloc(0x60, fill_random=False)
    img.w32(g, rng.choice([0, 0x1000, 0x2000, 0x3000, 0x1000, 0x3000]))
    img.w32(g + 4, nnodes)
    img.w32(g + 0x10, nodes)
    img.w32(g + 0x14, owner)
    img.w32(g + 0x20, anim)
    mat = img.alloc(0x60, fill_random=True)
    img.callee_ret[0x3E9700] = mat
    img.callee_ret[0x3E9240] = 0xFD000000 | rng.randrange(0, 256)
    return rn.Case([g], img, ret=True)


_C_E2F0, _P_E2F0 = _stubs({0x3DD390: (1,), 0x426F80: (1,)})
_C_E2F0.update({0x3DD430: "unsigned func_003dd430(unsigned *d, unsigned n, unsigned a, unsigned b, unsigned c) { return p4_record({ID}, n, a, b, c); }",
                0x426CF0: "int func_00426cf0(unsigned *p, unsigned f, unsigned m) { return p4_record({ID}, f, 0, 0, 0); }",
                0x4270F8: "int func_004270f8(int fd, int off, int wh) { return p4_record({ID}, fd, off, wh, 0); }"})


def _mk_e2f0(addr, kind):
    def impl(m):
        cid, n, ok = _ok(m, addr)
        if kind == 0:
            m.ilog.extend([cid, m.g32(5), m.g32(6), m.g32(7), m.g32(8)])
        elif kind == 1:
            m.ilog.extend([cid, m.g32(5), 0, 0, 0])
        else:
            m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), 0])
        return ok
    return impl


_P_E2F0.update({0x3DD430: _mk_e2f0(0x3DD430, 0), 0x426CF0: _mk_e2f0(0x426CF0, 1), 0x4270F8: _mk_e2f0(0x4270F8, 2)})


def gen_e2f0(rng):
    img = rn.Img(rng)
    dev = img.alloc(0x80, fill_random=False)
    img.hook(dev + 0x1C, 0)
    h = img.alloc(0x80, fill_random=True)
    img.callee_ret[0x500000] = rng.randrange(0, 1 << 16)
    img.callee_ret[0x3DD390] = rng.randrange(1 << 16)
    img.callee_ret[0x3DD430] = rng.choice([0, 1, 1, 1])
    img.callee_ret[0x426CF0] = rng.choice([5, 7, 0xFFFFFFFF, 3, 0xFD000000 | 0b10])
    img.callee_ret[0x4270F8] = rng.choice([0, 100, 0xFFFFFFF0, 12345])
    return rn.Case([dev, h, rng.randrange(1 << 16), rng.randrange(0, 32)], img, ret=True)


C_E42A0 = ("void func_003e42a0(float *o, float *s, float *m) { unsigned i; p4_record({ID}, (unsigned)s, (unsigned)m, 0, 0); for (i = 0; i < 3; i++) o[i] = s[i] + m[12 + i]; }")
C_C900N = ("unsigned func_0039c900(unsigned buf, unsigned n) { unsigned k = p4_ncalls[{ID}]; p4_record({ID}, buf, n, 0, 0); return 0x00900000u + k * 0x2000u; }")


def _py_e42a0(m):
    import mips as _m
    cid, n, ok = _ok(m, 0x3E42A0)
    o, s, mm = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, mm, 0, 0])
    for i in range(3):
        m.mem.wn(o + 4 * i, 4, _m.f2b(_m.b2f(m.mem.rn(s + 4 * i, 4)) + _m.b2f(m.mem.rn(mm + 4 * (12 + i), 4))))
    return 0


def _py_c900n(m):
    cid, n, ok = _ok(m, 0x39C900)
    m.ilog.extend([cid, m.g32(4), m.g32(5), 0, 0])
    return 0x00900000 + n * 0x2000


_C_93B0, _P_93B0 = _stubs({0x4114D0: (3, {"ret": "void"})})
_C_93B0.update({0x3E42A0: C_E42A0, 0x39C900: C_C900N})
_P_93B0.update({0x3E42A0: _py_e42a0, 0x39C900: _py_c900n})


def gen_93b0(rng):
    import mips as _m
    img = rn.Img(rng)
    for a in (0x00884000, 0x00885000, 0x00886000, 0x00887000, 0x00900000, 0x00901000, 0x00902000, 0x00903000, 0x00904000, 0x00905000, 0x00906000, 0x00907000, 0x00908000, 0x00909000):
        img.touch(a)
    img.touch(0x00764A24)
    img.w32(0x00764A24, 0x20)
    cam = img.alloc(0x200, fill_random=False)
    img.w32(0x008872E0, cam)
    for base in (0x94, 0x20 + 0x60):
        for k in range(6):
            for j in range(3):
                img.w32(cam + base + 0x14 * k + 4 * j, _m.f2b(rng.uniform(-1, 1)))
            img.w32(cam + base + 0x14 * k + 12, _m.f2b(rng.choice([5.0, 5.0, 1.0, 0.2, rng.uniform(-1, 1)])))
    a0 = img.alloc(0x80, fill_random=False)
    for j in range(3):
        img.w32(a0 + 0x1C + 4 * j, _m.f2b(rng.uniform(-2, 2)))
    img.w32(a0 + 0x28, _m.f2b(rng.uniform(0.1, 2)))
    img.w32(a0 + 0x6C, rng.randrange(1 << 16))
    cnt = rng.choice([0, 1, 5, 47, 48, 49, 100])
    inst = img.alloc(0x40 * max(cnt, 1) + 16, fill_random=False)
    for i in range(cnt):
        for j in range(16):
            img.w32(inst + 0x40 * i + 4 * j, _m.f2b(rng.uniform(-2, 2)))
        img.w32(inst + 0x40 * i + 0xC, rng.randrange(0, 4))
    cols = img.alloc(4 * max(cnt, 1) + 16, fill_random=True)
    return rn.Case([a0, inst, cnt, cols], img, ret=True)


# ---- 003d0230: shadow receiver registration
C_CFA80H = ("void func_003cfa80(unsigned r, unsigned *m, float a, float b) { unsigned h = 0, i; for (i = 0; i < 16; i++) h += m[i] * (i + 1u); p4_record({ID}, r, h, *(unsigned *)&a, *(unsigned *)&b); }")
C_C96D0W = ("int func_003c96d0(unsigned cb, unsigned *c) { return p4_record({ID}, 0, c[0], 0, 0); }")


def _py_cfa80h(m):
    cid, n, ok = _ok(m, 0x3CFA80)
    h = 0
    for i in range(16):
        h += m.mem.rn(m.g32(5) + 4 * i, 4) * (i + 1)
    m.ilog.extend([cid, m.g32(4), h & 0xFFFFFFFF, m.f[12], m.f[13]])
    return 0


def _py_c96d0w(m):
    cid, n, ok = _ok(m, 0x3C96D0)
    m.ilog.extend([cid, 0, m.mem.rn(m.g32(5), 4), 0, 0])
    return ok


_C_0230 = {0x3CFA80: C_CFA80H, 0x3C96D0: C_C96D0W, 0x3E0960: "void func_003e0960(float *d, float *s) { int i; for (i = 0; i < 16; i++) d[i] = s[i]; }"}
_P_0230 = {0x3CFA80: _py_cfa80h, 0x3C96D0: _py_c96d0w, 0x3E0960: None}


def gen_0230(rng):
    import mips as _m
    img = rn.Img(rng)
    img.touch(0x00887000)
    rec = img.alloc(0x40, fill_random=False)
    o = img.alloc(0x100, fill_random=False)
    g = img.alloc(0x80, fill_random=False)
    base = img.alloc(0x100, fill_random=False)
    mat = img.alloc(0x40, fill_random=False)
    sph = img.alloc(0x20, fill_random=False)
    img.w32(rec + 8, o)
    img.w32(o + 4, rng.randrange(1 << 16))
    img.w32(o + 0x18, g)
    img.w32(g + 8, 0x20 if rng.random() < 0.85 else 0)
    img.w32(g + 0x54, base)
    img.w32(base + 0xC, 0x10)
    img.w32(base + 0x10 + 0x18, rng.randrange(1 << 32))
    img.fill_f(mat, 16, -2, 2)
    img.w32(mat + 0xC, rng.choice([0, 1, 2, 3, 3]))
    if rng.random() < 0.1:
        for j in (8, 9, 10):
            img.wf(mat + 4 * j, 0.0)
    img.fill_f(sph, 4, -2, 2)
    img.wf(sph + 0xC, rng.uniform(0.1, 4))
    cnt = rng.randrange(1 << 16)
    # the counter halfword lives at 0x008872EA (upper half of the word at 0x008872E8)
    img.w32(0x008872E8, (cnt << 16) | rng.randrange(1 << 16))
    nn = rng.choice([0, 1, 2, 3])
    head = o + 0x64
    prev = head
    nodes = []
    for _ in range(nn):
        nd = img.alloc(0x20, fill_random=False)
        sub = img.alloc(0x80, fill_random=False)
        img.w32(nd + 8, sub)
        nodes.append((nd, sub))
    ch = [head] + [x[0] for x in nodes] + [head]
    for i, (nd, sub) in enumerate(nodes):
        img.w32(ch[i], nd)
    img.w32(ch[nn], head)
    for nd, sub in nodes:
        ne = rng.choice([0, 1, 2, 4])
        es = []
        for _ in range(ne):
            e = img.alloc(0x10, fill_random=False)
            r = img.alloc(0x60, fill_random=False)
            img.w32(e + 8, r if rng.random() > 0.15 else 0)
            img.w32(r, rng.randrange(1 << 16) | rng.choice([0, 1, 1, 2, 3]) << 16 | rng.randrange(256) << 24)
            img.w32(r + 4, rng.randrange(1 << 16))
            img.wf(r + 0x14, rng.uniform(0.0, 4))
            nxt = ((cnt + 1) & 0xFFFF)
            img.w32(r + 0x3C, nxt if rng.random() < 0.2 else rng.randrange(1 << 16))
            es.append(e)
        sh = nd_head = sub + 0x40
        chain = [sh] + es + [sh]
        for i in range(len(es) + 1):
            img.w32(chain[i], chain[i + 1])
    img.callee_ret[0x3E9700] = mat
    img.callee_ret[0x3BFAE0] = sph
    return rn.Case([rec], img, ret=False)


# ---- 003bd6b0: geometry record stream read
C_E2910_H = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned i, r = p4_record({ID}, s, 0, n, 0); if (!r) return 0; "
             "if (n == 0x2Cu) { for (i = 0; i < 11; i++) p[i] = s * (i + 3u); p[1] = (s >> 4) & 7u; p[2] = (s >> 8) & 7u; } return n; }")
C_E3110H = ("unsigned func_003e3110(unsigned l, unsigned s, unsigned n) { unsigned r = p4_record({ID}, l, s, n, 0); *(unsigned *)0x0076478C = ((r >> 4) & 0xFu) ? 0x1000u + ((r >> 4) & 0xFu) : 0u; *(unsigned *)0x00764788 = r * 3u; return r & 1u; }")


def _py_e2910h(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, 0, nn, 0])
    if not ok:
        return 0
    if nn == 0x2C:
        w = [(s * (i + 3)) & 0xFFFFFFFF for i in range(11)]
        w[1] = (s >> 4) & 7
        w[2] = (s >> 8) & 7
        for i in range(11):
            m.mem.wn(p + 4 * i, 4, w[i])
    return nn


def _py_e3110h(m):
    cid, n, ok = _ok(m, 0x3E3110)
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), 0])
    m.mem.wn(0x0076478C, 4, (0x1000 + ((ok >> 4) & 0xF)) if ((ok >> 4) & 0xF) else 0)
    m.mem.wn(0x00764788, 4, (ok * 3) & 0xFFFFFFFF)
    return ok & 1


C_DF050V = ("unsigned func_003df050(unsigned s, unsigned x, unsigned *sz, unsigned *ver) { static const unsigned K[7] = {0x34FFF, 0x35000, 0x36001, 0x36002, 0x37002, 0x37003, 0x36000}; "
            "unsigned k = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, s, x, 0, 0); if (!r) return 0; if (sz) *sz = 0x18; if (ver) *ver = K[((s >> 4) + k) % 7]; return 1; }")


def _py_df050v(m):
    cid, n, ok = _ok(m, 0x3DF050)
    s = m.g32(4)
    m.ilog.extend([cid, s, m.g32(5), 0, 0])
    if not ok:
        return 0
    if m.g32(6):
        m.mem.wn(m.g32(6), 4, 0x18)
    if m.g32(7):
        m.mem.wn(m.g32(7), 4, [0x34FFF, 0x35000, 0x36001, 0x36002, 0x37002, 0x37003, 0x36000][((s >> 4) + n) % 7])
    return 1


_C_D6B0 = {0x43F9C8: "void *func_0043f9c8(void *d, int c, unsigned n) { p4_record({ID}, (unsigned)d, c, n, 0); return d; }", 0x3DF050: C_DF050V, 0x3DF4D0: C_DF4D0E,
           0x3DF590: C_DF590_2, 0x3E2910: C_E2910_H, 0x3E3110: C_E3110H}
_P_D6B0 = {0x43F9C8: _py_memset_heap, 0x3DF050: _py_df050v, 0x3DF4D0: _py_df4d0e, 0x3DF590: _py_df590_2, 0x3E2910: _py_e2910h, 0x3E3110: _py_e3110h}


def gen_d6b0(rng):
    img = rn.Img(rng)
    img.touch(0x0070B000)
    img.touch(0x00764000)
    img.w32(0x0070B040, rng.choice([0x90, 0x94, 0xA0, 0xB0]))
    arena = img.alloc(0x1000, fill_random=True)
    cell = img.alloc(0x10, fill_random=False)
    desc = img.alloc(0x40, fill_random=True)
    img.w32(cell, 0 if rng.random() < 0.05 else arena + 4 * rng.randrange(0, 4))
    img.w32(desc + 8, rng.choice([0, 0, 0, 0, 0x1000000, 0x1000008, 0x30018]))
    img.w32(desc + 0x20, rng.choice([0, 1, 2, 3, 4, 4, 8, 0xFFFFFFFF]))
    img.callee_ret[0x3DF050] = 0xFE000000 | rng.choice([0, 1, 1, 1, 1, 1, 1, 1, 1])
    img.callee_ret[0x3E2910] = 0xFE000000 | rng.choice([0, 1, 2, 3, 4, 99, 99, 99, 99, 99])
    img.callee_ret[0x3DF300] = 0xFE000000 | rng.choice([0, 1, 2, 99, 99, 99, 99, 99])
    img.callee_ret[0x3DF2A0] = 0xFE000000 | rng.choice([0, 99, 99, 99, 99])
    img.callee_ret[0x3E3110] = rng.choice([0x31, 0x01, 0x00, 0x30, 0x21, 0x31, 0x01])
    return rn.Case([rng.randrange(1 << 32), cell, desc, rng.randrange(1 << 32)], img, ret=True)


# ---- 003c70e0: triangle stripifier
C_C67B0S = ("unsigned func_003c67b0(unsigned n, unsigned *pools, unsigned *el, unsigned t) { unsigned r = p4_record({ID}, n, 0, 0, t); pools[0] = n * 3u + 1u; pools[1] = n * 5u + 2u; *el = n * 7u + 3u; return r; }")
C_C6A40S = ("void func_003c6a40(unsigned t, unsigned *b, unsigned cur) { unsigned a; p4_record({ID}, t, 0, cur, 0); *(unsigned *)(t + 0x1C) = 1; "
            "if (cur >= 4) { *(unsigned *)(t + 0x18) = 1; a = *(unsigned char *)(t + 0x20); if (b[a] == t) { b[a] = *(unsigned *)(t + 0x10); if (b[a]) *(unsigned *)(b[a] + 0x14) = 0; } } }")
C_C6C10S = ("unsigned func_003c6c10(unsigned s, unsigned e, unsigned b, unsigned t, unsigned cur) { unsigned k = p4_ncalls[{ID}]; unsigned len = *(unsigned *)(s + 4); unsigned j, ret; "
            "unsigned short *st = *(unsigned short **)s; p4_record({ID}, s, e, t, cur); ret = (len * 3u + cur + k) & 3u; "
            "for (j = 0; j < ret; j++) st[len + j] = (unsigned short)(k * 16u + j + 0x100u * cur); *(unsigned *)(s + 4) = len + ret; return ret; }")
C_C66C0S = ("int func_003c66c0(unsigned n, unsigned *pools, unsigned el, unsigned arr) { p4_record({ID}, n, pools[0] * 3u + pools[1], el, arr); return 0; }")


def _py_c67b0s(m):
    cid, n, ok = _ok(m, 0x3C67B0)
    nn, pools, el, t = m.g32(4), m.g32(5), m.g32(6), m.g32(7)
    m.ilog.extend([cid, nn, 0, 0, t])
    m.mem.wn(pools, 4, (nn * 3 + 1) & 0xFFFFFFFF)
    m.mem.wn(pools + 4, 4, (nn * 5 + 2) & 0xFFFFFFFF)
    m.mem.wn(el, 4, (nn * 7 + 3) & 0xFFFFFFFF)
    return ok


def _py_c6a40s(m):
    cid, n, ok = _ok(m, 0x3C6A40)
    t, b, cur = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, t, 0, cur, 0])
    m.mem.wn(t + 0x1C, 4, 1)
    if cur >= 4:
        m.mem.wn(t + 0x18, 4, 1)
        a = m.mem.rb(t + 0x20)
        if m.mem.rn(b + 4 * a, 4) == t:
            nx = m.mem.rn(t + 0x10, 4)
            m.mem.wn(b + 4 * a, 4, nx)
            if nx:
                m.mem.wn(nx + 0x14, 4, 0)
    return 0


def _py_c6c10s(m):
    cid, k, ok = _ok(m, 0x3C6C10)
    s, e, t, cur = m.g32(4), m.g32(5), m.g32(7), m.g32(8)
    ln = m.mem.rn(s + 4, 4)
    st = m.mem.rn(s, 4)
    m.ilog.extend([cid, s, e, t, cur])
    ret = (ln * 3 + cur + k) & 3
    for j in range(ret):
        m.mem.wn(st + 2 * (ln + j), 2, (k * 16 + j + 0x100 * cur) & 0xFFFF)
    m.mem.wn(s + 4, 4, (ln + ret) & 0xFFFFFFFF)
    return ret


def _py_c66c0s(m):
    cid, n, ok = _ok(m, 0x3C66C0)
    nn, pools, el, arr = m.g32(4), m.g32(5), m.g32(6), m.g32(7)
    m.ilog.extend([cid, nn, (m.mem.rn(pools, 4) * 3 + m.mem.rn(pools + 4, 4)) & 0xFFFFFFFF, el, arr])
    return 0


_C_0E0 = {0x3C67B0: C_C67B0S, 0x3C6A40: C_C6A40S, 0x3C6C10: C_C6C10S, 0x3C66C0: C_C66C0S}
_P_0E0 = {0x3C67B0: _py_c67b0s, 0x3C6A40: _py_c6a40s, 0x3C6C10: _py_c6c10s, 0x3C66C0: _py_c66c0s}


def gen_70e0(rng):
    img = rn.Img(rng)
    img.pool()
    for a in (0x00887000, 0x007647E0, 0x008872F0):
        img.touch(a)
    img.w32(0x007647E0, 0x0C)
    img.w32(0x008872F0, 0x2468)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    img.hook(0x008873F8, 3)
    img.hook(0x008873FC, 0)
    n = rng.choice([0, 1, 2, 3, 4, 5, 6, 8, 10])
    tl = img.alloc(0x14 * max(n, 1), fill_random=False)
    for i in range(n):
        for j in range(3):
            img.mem.wn(tl + 0x14 * i + 2 * j, 2, rng.randrange(0, 60))
    arr = img.alloc(4 * max(n, 1) + 16, fill_random=False)
    ents = []
    for i in range(n):
        e = img.alloc(0x30, fill_random=False)
        ents.append(e)
        img.w32(arr + 4 * i, e)
    bias = rng.choice([0, 1, 2, 3])
    for i, e in enumerate(ents):
        img.w32(e, i)
        img.w32(e + 0x18, 1 if rng.random() < 0.2 else 0)
        img.w32(e + 0x1C, 1 if rng.random() < 0.2 else 0)
        ad = rng.choice([0, 1, 2, 3, 1, 2, 3][bias:] + [1, 2])
        img.mem.wb(e + 0x20, min(ad, 3))
        for k in range(3):
            ed = img.alloc(0x10, fill_random=False)
            img.w32(ed + 4, rng.choice(ents) if rng.random() < 0.7 else 0)
            img.w32(ed + 8, rng.choice(ents) if rng.random() < 0.7 else 0)
            img.w32(e + 4 + 4 * k, ed)
    img.callee_ret[0x3C67B0] = arr
    cell = img.alloc(0x10, fill_random=False)
    img.w32(cell, 0)
    return rn.Case([tl, n, cell, rng.choice([0, 1, 1])], img, ret=True)


# ---- 003d9f30: triangle vs box
def gen_9f30(rng):
    img = rn.Img(rng)
    box = img.alloc(0x20, fill_random=False)
    quant = rng.random() < 0.3
    spread = rng.choice([1.5, 2.5, 4.0])

    def q(v):
        return round(v * 2) / 2 if quant else v
    lo = [q(rng.uniform(-2, 0)) for _ in range(3)]
    hi = [q(rng.uniform(0.1, 2)) for _ in range(3)]
    hi = [h if h > l else l + 0.5 for h, l in zip(hi, lo)]
    for k in range(3):
        img.wf(box + 4 * k, hi[k])
        img.wf(box + 12 + 4 * k, lo[k])
    pts = []
    if rng.random() < 0.55:
        # a large triangle in a random plane around the box: only the plane stage can decide these
        import math
        n = [rng.uniform(-1, 1) for _ in range(3)]
        ln = math.sqrt(sum(x * x for x in n)) or 1.0
        n = [x / ln for x in n]
        t = [1.0, 0.0, 0.0] if abs(n[0]) < 0.8 else [0.0, 1.0, 0.0]
        d = sum(a * b for a, b in zip(t, n))
        t = [a - d * b for a, b in zip(t, n)]
        lt = math.sqrt(sum(x * x for x in t))
        t = [x / lt for x in t]
        b2 = [n[1] * t[2] - n[2] * t[1], n[2] * t[0] - n[0] * t[2], n[0] * t[1] - n[1] * t[0]]
        cen = [(h + l) / 2 for h, l in zip(hi, lo)]
        off = rng.uniform(-1.6, 1.6) * max(h - l for h, l in zip(hi, lo))
        R = rng.choice([4.0, 8.0, 30.0])
        ph = rng.uniform(0, 6.28)
        for k in range(3):
            a = ph + k * 2.0943951 + rng.uniform(-0.4, 0.4)
            pt = [cen[j] + n[j] * off + R * (math.cos(a) * t[j] + math.sin(a) * b2[j]) for j in range(3)]
            ad = img.alloc(0x10, fill_random=False)
            for j in range(3):
                img.wf(ad + 4 * j, pt[j])
            pts.append(ad)
        return rn.Case([box] + pts, img, ret=True)
    for _ in range(3):
        a = img.alloc(0x10, fill_random=False)
        for k in range(3):
            img.wf(a + 4 * k, q(rng.uniform(-spread, spread)))
        pts.append(a)
    return rn.Case([box] + pts, img, ret=True)


# address -> (generator, returns a value, hook args compared (a0, a1)[, callee addresses [, {callee: argc}]])
def gen_b490(rng):
    img = rn.Img(rng)
    rec = img.alloc(0x40)
    for off in (4, 8, 0xC, 0x10, 0x14):
        img.w32(rec + off, rng.randrange(1 << 32))
    return rn.Case([rng.randrange(1 << 32), rec], img, ret=True)



_C_CA320 = ("void *func_003ca320(void *p, void *cb, unsigned *rec) { unsigned u = (unsigned)p; float f = (float)((u >> 8) & 0xFFFu) * 0.5f; p4_record({ID}, u, rec[2], rec[3], rec[4]); rec[3] = *(unsigned *)&f; rec[4] = ((u >> 20) & 15u) - 3u; return p; }")
_C_B968 = ("float func_0044b968(float x) { p4_record({ID}, *(unsigned *)&x, 0, 0, 0); return x * 0.5f + 1.0f; }")


def _py_ca320(m):
    import mips as _m
    cid, n, ok = _ok(m, 0x3CA320)
    p, rec = m.g32(4), m.g32(6)
    m.ilog.extend([cid, p, m.mem.rn(rec + 8, 4), m.mem.rn(rec + 0xC, 4), m.mem.rn(rec + 0x10, 4)])
    m.mem.wn(rec + 0xC, 4, _m.f2b(((p >> 8) & 0xFFF) * 0.5))
    m.mem.wn(rec + 0x10, 4, (((p >> 20) & 15) - 3) & 0xFFFFFFFF)
    return p


def _py_b968(m):
    import mips as _m
    cid, n, ok = _ok(m, 0x44B968)
    b = m.f[12]
    m.ilog.extend([cid, b, 0, 0, 0])
    m.f[0] = _m.f2b(_m.b2f(b) * 0.5 + 1.0)
    return 0


_C3F6800, _P3F6800 = _mk_stub(0x3F6800, 1, "void", True)
_C3DB440, _P3DB440 = _mk_stub(0x3DB440, 0, "void")


def gen_cebb0(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.w32(0x00764818, 0x10)
    sec = img.alloc(0x100, fill_random=False)
    world = img.alloc(0x20, fill_random=False)
    n = rng.randrange(0, 4)
    img.w32(world + 4, n | (rng.choice([0, 0, 1, 0xBEEF]) << 16))
    img.w32(sec + 0x78, world)
    cache = img.alloc(0x40, fill_random=False)
    img.w32(cache, rng.choice([n, n, n + 1]))
    img.w32(sec + 0x10, cache if rng.random() < 0.6 else 0)
    slotbox = img.alloc(0x10, fill_random=False)
    cache2 = img.alloc(0x40, fill_random=False)
    img.w32(cache2, n)
    img.w32(slotbox, cache2)
    img.callee_ret[0x3CE050] = slotbox
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1, 1, 1])
    img.callee_ret[0x3E2910] = 0xFE000000 | rng.randrange(1, 12)
    return rn.Case([(rng.randrange(1 << 32) & ~0xF) | (rng.randrange(16) << 4 & 0xF0), sec], img, ret=True)


_C_QS_CMP = ("int func_003c6080(const char *p, const char *q) { p4_record({ID}, (unsigned)p, (unsigned)q, 0, 0); return (int)(signed char)*p - (int)(signed char)*q; } "
             "extern void func_00440bb8(char *, unsigned, unsigned, void *); "
             "unsigned p4w_qsort_440bb8(unsigned a, unsigned n, unsigned es, unsigned c) { func_00440bb8((char *)a, n, es, (void *)func_003c6080); return 0; } "
             "__asm__(\".globl p4t_qsort_440bb8\\n.set p4t_qsort_440bb8, p4w_qsort_440bb8\");")


def _py_qs_cmp(m):
    cid, n, ok = _ok(m, 0x3C6080)
    p, q = m.g32(4), m.g32(5)
    m.ilog.extend([cid, p, q, 0, 0])
    a, b = m.mem.rb(p), m.mem.rb(q)
    a = a - 256 if a >= 128 else a
    b = b - 256 if b >= 128 else b
    return (a - b) & 0xFFFFFFFF


def gen_qsort(rng):
    img = rn.Img(rng)
    big = rng.random() < 0.12
    n = rng.choice([40, 41, 41, 42, rng.randrange(41, 61)]) if big else rng.choice([0, 1, 2, 3, 5, 6, 7, 7, 8, 9, 12, 20, rng.randrange(0, 30)])
    es = rng.choice([1, 2, 3, 4, 5, 7, 8, 8, 8, 8, 12, 16, 24, 40])
    off = rng.choice([0, 0, 0, 0, 1, 4, 8])
    base = img.alloc(n * es + 64, fill_random=True)
    a = base + off
    span = 2 if big else rng.choice([2, 3, 5, 16, 200])
    mode = rng.choice(["rand", "rand", "asc", "desc", "const", "few"])
    for i in range(n):
        if mode == "asc":
            k = i
        elif mode == "desc":
            k = n - i
        elif mode == "const":
            k = 7
        else:
            k = rng.randrange(span)
        img.mem.wb(a + i * es, (k - span // 2) & 0xFF)
    return rn.Case([a, n, es, 0x3C6080], img, ret=False)


def _f32_vec(rng, img, addr, n):
    import mips as _m
    vals = [0.0, -0.0, 1.0, -1.0, 2.5, -3.75, 1e-30, 1e30, 1e-20, 0.001, 123.456, 3.0, 4.0, 1e19, -1e19]
    for i in range(n):
        img.w32(addr + 4 * i, _m.f2b(rng.choice(vals) if rng.random() < 0.6 else rng.uniform(-50, 50)))


def gen_norm(n):
    def gen(rng):
        img = rn.Img(rng)
        src = img.alloc(0x40, fill_random=False)
        _f32_vec(rng, img, src, n)
        if rng.random() < 0.12:
            for i in range(n):
                img.w32(src + 4 * i, rng.choice([0, 0x80000000]))
        dst = src if rng.random() < 0.3 else img.alloc(0x40, fill_random=True)
        img.callee_ret[0x3DF590] = rng.randrange(1 << 32)
        return rn.Case([dst, src], img, ret=True)
    return gen


_C_NORM_W = ("unsigned p4w_n%d_%s(unsigned a, unsigned b) { float r = %s((float *)a, (float *)b); return *(unsigned *)&r; } "
             "__asm__(\".globl p4t_n%d_%s\\n.set p4t_n%d_%s, p4w_n%d_%s\");")
_C_NORM_3 = C_DF4D0E + " extern float p4real_3e4030(float *, float *) __asm__(\"func_003e4030\"); " + (_C_NORM_W % (3, "3e4030", "p4real_3e4030", 3, "3e4030", 3, "3e4030", 3, "3e4030"))
_C_NORM_2 = C_DF4D0E + " extern float p4real_3e41e0(float *, float *) __asm__(\"func_003e41e0\"); " + (_C_NORM_W % (2, "3e41e0", "p4real_3e41e0", 2, "3e41e0", 2, "3e41e0", 2, "3e41e0"))
rn.FRET.update([0x3E4030, 0x3E41E0])


def gen_gpword(slot):
    def g(rng):
        img = rn.Img(rng)
        gp_page(img)
        off = rng.choice([0, 4, 8, 0x10, 0x20])
        img.w32(slot, off)
        obj = img.alloc(0x80, fill_random=False)
        img.w32(obj + off, rng.choice([0, 1, 2, 0xFFFFFFFF, 0x80000000]))
        return rn.Case([obj], img, ret=True)
    return g


def gen_a8a0(rng):
    img = rn.Img(rng)
    gp_page(img)
    off = rng.choice([0, 4, 8, 0x10])
    img.w32(0x007646D0, off)
    obj = img.alloc(0x80, fill_random=False)
    p = img.alloc(0x100, fill_random=True)
    img.w32(obj + off, p)
    img.w32(p + 0x80, rng.choice([3, 3, 0, 5]))
    img.w32(p + 0x4C, rng.choice([0, 1, 9]))
    img.w32(p + 0xC, rng.choice([0, 1, 9]))
    return rn.Case([obj], img, ret=False)


def gen_f050(rng):
    img = rn.Img(rng)
    gp_page(img)
    img.touch(0x0064F000)
    off = rng.choice([0, 4, 0x10])
    img.w32(0x007646D0, off)
    obj = img.alloc(0x40, fill_random=False)
    holder = img.alloc(0x40, fill_random=False)
    base = img.alloc(0x40, fill_random=False)
    q = img.alloc(0x100, fill_random=False)
    d = img.alloc(0x40, fill_random=True)
    img.w32(obj + 0x2C, holder)
    img.w32(holder + 8, base)
    img.w32(base + off, q if rng.random() < 0.85 else 0)
    img.w32(q + 0x80, rng.choice([0, 1, 2, 3]))
    img.w32(obj + 4, d)
    for i in range(4):
        img.hook(0x0064F2B4 + 4 * i, 11)
    return rn.Case([obj], img, ret=False)


def gen_d0850(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    obj = img.alloc(0x40, fill_random=False)
    d = img.alloc(0x10, fill_random=False)
    img.mem.wb(d, rng.choice([0xFF, 0xFF, 0xFF, 0, 1, 0xFE]))
    img.w32(obj + 8, d)
    slot = img.alloc(0x40, fill_random=True)
    st = img.alloc(0x20, fill_random=False)
    img.w32(st + 8, rng.choice([0, 0x2000000, 0x2000000, 0xFFFFFFFF, 0xFDFFFFFF, 0x1000000]))
    img.w32(0x008872E4, st)
    img.callee_ret[0x3E1FF0] = rng.randrange(1 << 32)
    img.callee_ret[0x3D0460] = rng.randrange(1 << 32)
    return rn.Case([obj, slot, rng.randrange(0, 0x200), rng.randrange(1 << 32)], img, ret=True)


_C_8500 = ("unsigned func_003d8500(unsigned a, unsigned b, unsigned c, unsigned d, unsigned e) { unsigned r = p4_record({ID}, c, d, e, a ^ b); return r; }")


def _py_8500(m):
    cid, n, ok = _ok(m, 0x3D8500)
    m.ilog.extend([cid, m.g32(6), m.g32(7), m.g32(8), m.g32(4) ^ m.g32(5)])
    return ok


def gen_d86a0(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    img.hook(0x008873EC, 4)
    rec = img.alloc(0x20, fill_random=False)
    img.w32(rec + 4, rng.randrange(1, 0x40))
    img.w32(rec + 8, rng.choice([0xFFFFFFFF, 0, 1, 2, 5, 9]))
    img.callee_ret[0x3D8500] = rng.randrange(1 << 32)
    return rn.Case([rng.randrange(1 << 32), rng.randrange(1 << 32), rec], img, ret=True)


_C_E2910_IMG = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned k = p4_ncalls[{ID}]; unsigned inheap = ((unsigned)p >> 28) == 3u; p4_record({ID}, s, inheap ? (unsigned)p : 0, n, 0); "
                "if (k == 0 && n == 16) { unsigned *T = (unsigned *)0x00887410; p[0] = T[0]; p[1] = T[1]; p[2] = T[2]; p[3] = T[3]; } "
                "else { unsigned char *q = (unsigned char *)p; unsigned i; for (i = 0; i < n; i++) q[i] = (unsigned char)(i * 3u + n + k); } return n; }")
_C_B70_IMG = ("void func_003e3b70(unsigned list, unsigned *img) { p4_record({ID}, list, img[0] + 3u * img[5] + 5u * img[6], img[1] + 7u * img[2] + 13u * img[3], (unsigned)img); }")
_C_EA3E0_IMG = ("void func_003ea3e0(unsigned *img) { unsigned *S = (unsigned *)0x00887400; p4_record({ID}, (unsigned)img, 0, 0, 0); if (img) { img[4] = S[0]; img[5] = S[1]; img[6] = S[2]; } }")


def _py_e2910_img(m):
    cid, n, ok = _ok(m, 0x3E2910)
    s, p, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, s, p if (p >> 28) == 3 else 0, nn, 0])
    if n == 0 and nn == 16:
        for i in range(4):
            m.mem.wn(p + 4 * i, 4, m.mem.rn(0x00887410 + 4 * i, 4))
    else:
        for i in range(nn):
            m.mem.wb(p + i, (i * 3 + nn + n) & 0xFF)
    return nn


def _py_b70_img(m):
    cid, n, ok = _ok(m, 0x3E3B70)
    list_, img = m.g32(4), m.g32(5)
    r = m.mem.rn
    m.ilog.extend([cid, list_, (r(img, 4) + 3 * r(img + 20, 4) + 5 * r(img + 24, 4)) & 0xFFFFFFFF,
                   (r(img + 4, 4) + 7 * r(img + 8, 4) + 13 * r(img + 12, 4)) & 0xFFFFFFFF, img])
    return 0


def _py_ea3e0_img(m):
    cid, n, ok = _ok(m, 0x3EA3E0)
    img = m.g32(4)
    m.ilog.extend([cid, img, 0, 0, 0])
    if img:
        for i in range(3):
            m.mem.wn(img + 4 * (4 + i), 4, m.mem.rn(0x00887400 + 4 * i, 4))
    return 0


def gen_ebbf0(rng):
    img = rn.Img(rng)
    img.pool()
    gp_page(img)
    img.touch(0x00887000)
    img.hook(0x008873F8, 2)
    off = rng.choice([0, 4, 8])
    img.w32(0x007648B8, off)
    img.w32(0x008872E0 + off, rng.choice([0x40, 0x100, 0x230]))
    h = rng.choice([0, 1, 2, 3, 5, 7])
    depth = rng.choice([1, 4, 8, 8, 8, 9, 16, 24, 32])
    imgstride = rng.choice([0, 4, 8, 16, 0x20, 0x40])
    fstride = rng.choice([imgstride, imgstride, imgstride, imgstride + 4, imgstride - 4 if imgstride >= 4 else 0, 0x10, 0x30])
    pix = img.alloc(0x800, fill_random=False)
    pal = img.alloc(0x500, fill_random=False)
    img.w32(0x00887400, imgstride)
    img.w32(0x00887404, pix)
    img.w32(0x00887408, pal)
    img.w32(0x00887410, rng.randrange(0, 0x400))
    img.w32(0x00887414, h)
    img.w32(0x00887418, depth)
    img.w32(0x0088741C, fstride)
    img.callee_ret[0x3DF050] = rng.choice([0, 1, 1, 1, 1])
    return rn.Case([rng.randrange(1 << 32)], img, ret=True)


def gen_d0460(rng):
    img = rn.Img(rng)
    img.pool()
    img.touch(0x00887000)
    img.hook(0x008873E8, 2)
    obj = img.alloc(0x40, fill_random=False)
    holder = img.alloc(0x40, fill_random=False)
    d = img.alloc(0x20, fill_random=False)
    img.w32(obj + 0x24, rng.choice([0, 0, 0, 1, 0x100]))
    img.w32(obj + 8, holder)
    img.w32(holder + 0x18, d)
    img.w32(d + 8, rng.choice([0, 0x2000000, 0x2000000, 0xFFFFFFFF, 0xFDFFFFFF, 0x1000000]))
    slot = img.alloc(0x40, fill_random=True)
    img.callee_ret[0x3E1FF0] = rng.randrange(1 << 32)
    return rn.Case([obj, slot, rng.randrange(0, 0x200), rng.randrange(1 << 32)], img, ret=False)


def gen_ac3_disp(slot, with_sji):
    def g(rng):
        img = rn.Img(rng)
        img.touch(0x00714000)
        adxb = img.alloc(0x100, fill_random=True)
        dec = rng.choice([0, 0, img.alloc(0x20, fill_random=False)])
        img.w32(adxb + 0xC0, dec)
        img.hook(slot, 0)
        return rn.Case([adxb, rng.randrange(1 << 32)] if with_sji else [adxb], img, ret=False)
    return g


def gen_ac3_smpl(rng):
    img = rn.Img(rng)
    adxb = img.alloc(0x100, fill_random=True)
    return rn.Case([adxb, rng.choice([0, 1, 0x5FF, 0x600, 0x601, 0x1234567, 0x7FFFFFFF, 0xFFFFFFFF, 0x80000000, 0xFFFFF9FF, rng.randrange(1 << 32)])], img, ret=False)


def _py_df590_one(m):
    cid, n, ok = _ok(m, 0x3DF590)
    a = m.g32(4)
    m.ilog.extend([cid, a, 0 if a == 1 else m.g32(5), 0, 0])
    return ok


# Retail follows a null frame into PS2 low RAM (include/p4pc/ps2_low_ram.h): func_003e9240(0) is the sync bits of byte 3 (0), func_003e9700(0) is 0x50. The overrides read the
# header's image instead of calling them, so on the interpreter side these callees answer a null frame without a log entry; the matrix argument that points at 0x50 (the image on the C side) is not compared.
_C_9240_LOW = ("unsigned func_003e9240(unsigned f) { p4_record({ID}, f, 0, 0, 0); return p4_ret[{ID}]; }")
_C_9700_LOW = ("unsigned func_003e9700(unsigned f) { p4_record({ID}, f, 0, 0, 0); return p4_ret[{ID}]; }")
_C_42A0_LOW = ("void func_003e42a0(unsigned d, unsigned s, unsigned m) { p4_record({ID}, d, s, (m >> 28) == 3u ? m : (m & 0xFFu), 0); }")


def _py_low_ret(addr):
    def impl(m):
        a = m.g32(4)
        if a == 0:
            return 0 if addr == 0x3E9240 else 0x50
        cid, n, ok = _ok(m, addr)
        m.ilog.extend([cid, a, 0, 0, 0])
        return m.cret.get(addr, 0)
    return impl


def _py_42a0_low(m):
    cid, n, ok = _ok(m, 0x3E42A0)
    mm = m.g32(6)
    m.ilog.extend([cid, m.g32(4), m.g32(5), mm if (mm >> 28) == 3 else (mm & 0xFF), 0])
    return 0


_C_05F0_LOW = ("unsigned func_003e05f0(unsigned d, unsigned a, unsigned b) { p4_record({ID}, d, a, (b >> 28) == 3u ? b : (b & 0xFFu), 0); return d; }")


def _py_05f0_low(m):
    cid, n, ok = _ok(m, 0x3E05F0)
    b = m.g32(6)
    m.ilog.extend([cid, m.g32(4), m.g32(5), b if (b >> 28) == 3 else (b & 0xFF), 0])
    return m.g32(4)


def gen_ed960(rng):
    img = rn.Img(rng)
    obj = img.alloc(0x100, fill_random=True)
    img.mem.wb(obj + 3, rng.randrange(256))
    prev = 0
    first = 0
    for _ in range(rng.randrange(0, 4)):
        ch = img.alloc(0x100, fill_random=True)
        img.mem.wb(ch + 3, rng.randrange(256))
        img.w32(ch + 4, 0 if rng.random() < 0.35 else rng.choice([obj, img.alloc(0x100, fill_random=True)]))
        img.w32(ch + 0x98, rng.choice([0, img.alloc(0x100, fill_random=True)]))
        img.w32(ch + 0x9C, 0)
        if prev:
            img.w32(prev + 0x9C, ch)
        else:
            first = ch
        prev = ch
    img.w32(obj + 0x98, first)
    return rn.Case([obj], img, ret=False)


SPECS = {
    "001f5680": (gen_1f5680, True, (True, True)),
    "00396e80": (gen_396e80, True, (True, True)),
    "003d5710": (gen_3d5710, True, (True, False)),
    "003cc250": (gen_3cc250, True, (True, True)),
    "003b5fb0": (strided(44, 48, 4), False, (True, True)),
    "003b6390": (strided(60, 64, 4), False, (True, True)),
    "003b6420": (strided(68, 72, 8), False, (True, True)),
    "003b64c0": (gen_fill, False, (True, True)),
    "003b4230": (gen_rep3, False, (True, True)),
    "003b65d0": (gen_rep3, False, (True, True)),
    "003b6540": (gen_6540, False, (True, True)),
    "003b6020": (gen_fill, False, (True, True)),
    "003b61e0": (gen_fill, False, (True, True)),
    "003d0790": (gen_3d0790, True, (True, True)),
    "003cbf30": (gen_cbf30, True, (True, True)),
    "003ce840": (gen_ce840, True, (True, True)),
    "0039a910": (gen_a910, False, (True, True)),
    "003a2770": (gen_2770, True, (True, True)),
    "003a5180": (gen_5180, True, (True, True)),
    "003b4ab0": (gen_corner([(4, 8, 12)]), False, (True, True)),
    "003b4c10": (gen_corner([(4, 8, 12)]), False, (True, True)),
    "003b4ff0": (gen_corner([(4, 8, 12), (0x1C, 0x20, 8)]), False, (True, True)),
    "003b5650": (gen_corner([(4, 8, 12), (0x1C, 0x20, 8)]), False, (True, True)),
    "003b5ac0": (gen_corner([(0xC, 0x10, 0x3C)]), False, (True, True)),
    "003b5bf0": (gen_corner([(0xC, 0x10, 0x3C)]), False, (True, True)),
    "003954b0": (gen_3954b0, False, (True, True)),
    "0039ab20": (gen_ab20, True, (True, True)),
    "003a2950": (gen_2950, True, (True, True)),
    "003ddf80": (rw_io(0x30), True, (True, True, True, False)),
    "003ddfd0": (rw_io(0x34), True, (True, True, True, False)),
    "003de020": (gen_de020, True, (False, True, True, True)),
    "003dd760": (gen_dd760, False, (True, True), [0x4217E0], "func_003dd760", {0x4217E0: 1}),
    "0039b720": (gen_b720, False, (True, True), [0x39BDF0], "func_0039b720", {0x39BDF0: 1}),
    "0039b830": (gen_b830, False, (True, False), [0x411670], "func_0039b830", {0x411670: 1}),
    "0039ba80": (gen_ba80, False, (True, True), [0x39BBD0, 0x39B720], "PDSRegistryCreatePipes", {0x39BBD0: 1, 0x39B720: 2}),
    "0039bb70": (gen_bb70, True, (True, True), [], "PDSRegistryGetPipe"),
    "003b31a0": (gen_ptank(None, 0x2C, 0x30, 16), False, (True, True), [0x3A4D50], "func_003b31a0", {0x3A4D50: 3}),
    "003b3240": (gen_ptank(4), False, (True, True), [0x3A4850], "func_003b3240", {0x3A4850: 3}),
    "003b3570": (gen_ptank(4), False, (True, True), [0x3A4270], "func_003b3570", {0x3A4270: 3}),
    "003b3f50": (gen_ptank(None, 0x44, 0x48, 32), False, (True, True), [0x3A4D50], "func_003b3f50", {0x3A4D50: 3}),
    "003b4020": (gen_ptank(0x10), False, (True, True), [0x3A4AB0], "func_003b4020", {0x3A4AB0: 3}),
    "003b4120": (gen_ptank(8), False, (True, True), [0x3A4AB0], "func_003b4120", {0x3A4AB0: 3}),
    "003b4db0": (gen_rot(False, False), False, (True, True), [0x44B7B0, 0x44B610], "func_003b4db0"),
    "003b5160": (gen_rot(True, False), False, (True, True), [0x44B7B0, 0x44B610], "func_003b5160"),
    "003b53b0": (gen_rot(False, True), False, (True, True), [0x44B7B0, 0x44B610], "func_003b53b0"),
    "003b5820": (gen_rot(True, True), False, (True, True), [0x44B7B0, 0x44B610], "func_003b5820"),
    "003b5d20": (gen_5d20, False, (True, True)),
    "00396890": (gen_6890, True, (True, True), [0x397120], "HAnimDestructor", {0x397120: 1}),
    "00396fb0": (gen_hier_6fb0, True, (True, True), [0x3D5790], "func_00396fb0", {0x3D5790: 2}),
    "00396680": (gen_hier_walk(False), True, (True, True), [0x397470, 0x3E9680, 0x3E9AF0], "func_00396680",
                 {0x397470: 1, 0x3E9680: 1, 0x3E9AF0: (True, False, True, False)}),
    "00397390": (gen_hier_walk(True), True, (True, True), [0x397470, 0x3E9680, 0x3E9AF0, 0x396680], "func_00397390",
                 {0x397470: 1, 0x3E9680: 1, 0x3E9AF0: (True, False, True, False), 0x396680: 0}),
    "00396940": (gen_6940, True, (True, True), [0x396FB0], "func_00396940", {0x396FB0: 5}),
    "00396a40": (gen_6a40, True, (True, True), [0x3DF240], "func_00396a40", {0x3DF240: 3}),
    "0039b2c0": (gen_b2c0, True, (True, False), [0x43F9C8], "func_0039b2c0", {0x43F9C8: 3}),
    "00394fd0": (gen_4fd0, True, (True, True), [0x3DF590, 0x3DF4D0], "func_00394fd0", {0x3DF590: 2, 0x3DF4D0: 0}),
    "00395100": (gen_5100, True, (True, True), [0x43F9C8, 0x3DF590, 0x3DF4D0], "func_00395100", {0x43F9C8: 3, 0x3DF590: 2, 0x3DF4D0: 0}),
    "0039ae90": (gen_ae90, True, (True, True), [], "func_0039ae90"),
    "00394c70": (gen_4c70, True, (True, True), [0x3E9700, 0x3E0960, 0x3E42E0, 0x394870, 0x393900], "func_00394c70",
                 {0x3E9700: 1, 0x3E0960: 2, 0x3E42E0: 4, 0x394870: (True, True, True, True), 0x393900: (True, True, True, True)}),
    "0039ac80": (gen_ac80, True, (True, True), [0x3E9680], "func_0039ac80", {0x3E9680: 1}),
    "00396c00": (gen_6c00, True, (True, True), [0x3DF360, 0x43F9C8, 0x3D5790], "func_00396c00", {0x3DF360: 3, 0x43F9C8: 3, 0x3D5790: 2}),
    "003971d0": (gen_71d0, True, (True, True), [0x3D5F50], "func_003971d0", {0x3D5F50: 4}),
    "0039af40": (gen_af40, True, (True, True), [0x3DF240, 0x3E2AB0], "func_0039af40", {0x3DF240: 3, 0x3E2AB0: 3}),
    "0039b080": (gen_b080, True, (True, True), [0x3DF360, 0x39B2C0, 0x3E2910], "func_0039b080", {0x3DF360: 3, 0x39B2C0: 2, 0x3E2910: 3}),
    "003b42e0": (gen_42e0, True, {None: (False, True, True, True), 129: (False, False, False, False)}, [0x43F9C8], "func_003b42e0", {0x43F9C8: (False, True, True, False)}),
    "003a9c70": (gen_9c70, True, (True, True), [0x43F9C8], "func_003a9c70", {0x43F9C8: 3}),
    "003bb330": (gen_b330, True, (True, True), [0x3DF590, 0x3DF4D0], "func_003bb330", {0x3DF590: 1, 0x3DF4D0: 0}),
    "003bb3a0": (gen_b3a0, True, (True, True), [0x3DF590, 0x3DF4D0], "func_003bb3a0", {0x3DF590: 2, 0x3DF4D0: 0}),
    "003bb210": (gen_b210, True, (True, True), [0x3E2910, 0x3DF300, 0x3BBBE0, 0x3DF590, 0x3DF4D0], "func_003bb210",
                 {0x3E2910: 3, 0x3DF300: 3, 0x3BBBE0: 3, 0x3DF590: 2, 0x3DF4D0: 0}),
    "003bbbe0": (gen_bbe0, True, (True, True), [0x3B9A40, 0x3DF590, 0x3DF4D0], "func_003bbbe0", {0x3B9A40: 2, 0x3DF590: 2, 0x3DF4D0: 0}),
    "003b71b0": (gen_71b0, True, (True, True), [0x3DF240, 0x3E2AB0], "func_003b71b0", {0x3DF240: 3, 0x3E2AB0: 3}),
    "003b7290": (gen_7290, True, (True, True), [0x3DF360, 0x3B7150, 0x43F9C8, 0x3E2910], "func_003b7290", {0x3DF360: 3, 0x3B7150: 1, 0x43F9C8: 3, 0x3E2910: 3}),
    "003ce170": (gen_ce170, True, (True, True), [], "func_003ce170"),
    "003ce230": (gen_ce230, True, (True, True), [], "func_003ce230"),
    "003d3ae0": (gen_3ae0, False, (True, True), [], "func_003d3ae0"),
    "003d0140": (gen_0140, False, (True, True), [], "func_003d0140"),
    "003a3500": (gen_3500, True, (True, True), [], "func_003a3500"),
    "003c65b0": (gen_65b0, True, (True, True), [], "func_003c65b0"),
    "003dd990": (gen_d990, True, {None: (True, True), 128: (True, False)}, [], "func_003dd990"),
    "003d5130": (gen_5130, True, (True, True), [0x3DF590, 0x3DF4D0], "func_003d5130", {0x3DF590: 1, 0x3DF4D0: 0}),
    "003ddca0": (gen_dca0, True, {None: (True, True), 128: (True, False), 129: (False, True)}, [0x442DE8], "func_003ddca0", {0x442DE8: (False, True, True, False)}),
    "003d8760": (gen_8760, True, (True, True), [0x3DF590, 0x3DF4D0], "func_003d8760", {0x3DF590: 2, 0x3DF4D0: 0}),
    "003c4a80": (gen_4a80, True, (True, True), [0x3DF590, 0x3DF4D0], "func_003c4a80", {0x3DF590: 2, 0x3DF4D0: 0}),
    "003c2a80": (gen_2a80, True, {None: (True, True), 132: (True, False)}, [0x3E1EA0, 0x3C5760, 0x3E3C20, 0x3C49A0], "func_003c2a80", {0x3E1EA0: 1, 0x3C5760: 1, 0x3E3C20: 2, 0x3C49A0: 1}),
    "003c9300": (gen_9300, False, {None: (True, True), 132: (True, False)}, [0x3E1EA0, 0x3CA9D0, 0x3CA960, 0x3E3C20, 0x3C5760], "func_003c9300", {0x3E1EA0: 1, 0x3CA9D0: 1, 0x3CA960: 1, 0x3E3C20: 2, 0x3C5760: 1}),
    "003caa50": (gen_aa50, False, (True, True), [0x3BFAE0], "func_003caa50", {0x3BFAE0: 1}),
    "003d53c0": (gen_53c0, True, {None: (True, True), 129: (True, True)}, [0x3DF360, 0x3DF300, 0x3DF590, 0x3DF4D0], "func_003d53c0", {0x3DF360: 3, 0x3DF300: 3, 0x3DF590: 1, 0x3DF4D0: 0}),
    "003d5330": (gen_5330, True, (True, True), [0x3E2F60, 0x3DF050, 0x3D53C0, 0x3E2E40], "func_003d5330", {0x3E2F60: 3, 0x3DF050: 4, 0x3D53C0: 1, 0x3E2E40: 2}),
    "003d5600": (gen_5600, True, (True, True), [0x3D5710, 0x3DEFF0, 0x3DF240, 0x3DF1A0], "func_003d5600", {0x3D5710: 1, 0x3DEFF0: 4, 0x3DF240: 3, 0x3DF1A0: 3}),
    "003c3ae0": (gen_3ae0b, True, (True, True), [0x3DF050, 0x3DF590, 0x3DF4D0, 0x43F9C8, 0x3E2910, 0x3C3D20, 0x3E3110], "func_003c3ae0",
                 {0x3DF050: 2, 0x3DF590: 1, 0x3DF4D0: 0, 0x43F9C8: (False, True, True, False), 0x3E2910: 3, 0x3C3D20: 1, 0x3E3110: 3}),
    "00390290": (gen_0290, True, (True, True), [0x38FB20], "func_00390290", {0x38FB20: 1}),
    "003a2340": (gen_2340, True, (True, True), [0x3A20E0, 0x3A2390], "func_003a2340", {0x3A20E0: 1, 0x3A2390: 4}),
    "003d7ac0": (gen_7ac0, True, (True, True), [0x3E2AB0, 0x3DF240], "func_003d7ac0", {0x3E2AB0: 3, 0x3DF240: 3}),
    "003d7c50": (gen_7c50, True, (True, True), [0x3D5300], "func_003d7c50", {0x3D5300: 1}),
    "003d60f0": (gen_60f0, True, {None: (True, True), 129: (True, False)}, [0x3DF8A0, 0x3DF890, 0x3DF7F0], "func_003d60f0", {0x3DF8A0: 1, 0x3DF890: 1, 0x3DF7F0: 1}),
    "0039f0e0": (gen_f0e0, True, (True, True), [0x3D0FA0, 0x39C000], "func_0039f0e0", {0x3D0FA0: 2, 0x39C000: 3}),
    "003c4220": (gen_4220, True, (True, True), [0x3E3C20, 0x3C42B0], "func_003c4220", {0x3E3C20: 2, 0x3C42B0: 2}),
    "003c9640": (gen_9640, True, (True, True), [0x4114D0], "func_003c9640", {0x4114D0: 3}),
    "003bcbe0": (gen_cbe0, True, {None: (True, True), 132: (True, False)}, [0x3BBEA0], "func_003bcbe0", {0x3BBEA0: 1}),
    "003c49a0": (gen_49a0, True, {None: (True, True), 132: (True, False)}, [0x3C4220], "func_003c49a0", {0x3C4220: 1}),
    "003d7a20": (gen_7a20, True, (True, True), [0x3E2910, 0x3DF360], "func_003d7a20", {0x3E2910: 3, 0x3DF360: 3}),
    "003cdfa0": (gen_dfa0, False, {None: (True, True), 132: (True, False)}, [0x3CC6E0, 0x3E1EA0], "func_003cdfa0", {0x3CC6E0: 1, 0x3E1EA0: 1}),
    "003ce2e0": (gen_e2e0, True, (True, True), [0x3CE050, 0x3E1EA0], "func_003ce2e0", {0x3CE050: 2, 0x3E1EA0: 1}),
    "003bce50": (gen_ce50, True, (True, True), [0x3BC620], "func_003bce50", {0x3BC620: 1}),
    "003962e0": (gen_62e0, True, (True, True), [0x3DF1A0, 0x3DF240], "func_003962e0", {0x3DF1A0: 3, 0x3DF240: 3}),
    "003961f0": (gen_61f0, True, (True, True), [0x3DF300, 0x3DF360], "func_003961f0", {0x3DF300: 3, 0x3DF360: 3}),
    "003bcc80": (gen_cc80, True, (True, True), [0x3BC740, 0x3BBF80], "func_003bcc80", {0x3BC740: 1, 0x3BBF80: 2}),
    "003c8dd0": (gen_8dd0, True, {None: (True, True), 129: (True, False)}, [0x3C8CE0, 0x3BFAE0, 0x3E8200], "func_003c8dd0", {0x3C8CE0: 1, 0x3BFAE0: 1, 0x3E8200: 2}),
    "003dd290": (gen_d290, True, (True, True), [0x3DD7D0, 0x3DD990, 0x3DD620], "func_003dd290", {0x3DD7D0: 0, 0x3DD990: 1, 0x3DD620: 2}),
    "003cac30": (gen_ac30, True, {None: (True, True), 129: (True, False)}, [0x3CAA50], "func_003cac30", {0x3CAA50: 2}),
    "003c0520": (gen_0520, True, (True, True), [0x3C0640, 0x3E90F0, 0x3BEA60, 0x3E9140, 0x3C0700, 0x3E3C90], "func_003c0520",
                 {0x3C0640: 1, 0x3E90F0: 1, 0x3BEA60: (True, False, False, False), 0x3E9140: 1, 0x3C0700: 1, 0x3E3C90: 3}),
    "003db490": (gen_b490, True, (True, True), [0x3CA320, 0x44B968, 0x3F6800, 0x3DB440], "func_003db490",
                 {0x3CA320: (True, True, True, True), 0x44B968: (True, False, False, False), 0x3F6800: (True, True, False, False), 0x3DB440: 0}),
    "003db550": (gen_b550, False, (True, True), [0x3E89C0, 0x3E8970, 0x3EF6D0, 0x3EF260, 0x3DB490], "func_003db550",
                 {0x3E89C0: 0, 0x3E8970: (False, True, False, False), 0x3EF6D0: 0, 0x3EF260: (True, False, True, True), 0x3DB490: 0}),
    "003c0700": (gen_0700, True, {None: (True, True), 132: (True, False)}, [0x3E3C20, 0x3C02E0, 0x3C0880, 0x3C3CC0, 0x3C08C0, 0x3E8440, 0x3E95C0], "func_003c0700",
                 {0x3E3C20: 2, 0x3C02E0: 1, 0x3C0880: 2, 0x3C3CC0: 1, 0x3C08C0: 2, 0x3E8440: 1, 0x3E95C0: 1}),
    "003ce050": (gen_ce50b, True, {None: (True, True), 132: (True, False)}, [0x3CC6E0, 0x3E1EA0, 0x43F9C8], "func_003ce050", {0x3CC6E0: 1, 0x3E1EA0: 1, 0x43F9C8: 3}),
    "003b8d40": (gen_8d40, True, (True, True), [0x3C1B80, 0x39B6E0], "func_003b8d40", {0x3C1B80: 1, 0x39B6E0: 1}),
    "003d81d0": (gen_81d0, True, (True, True), [0x3E2AB0, 0x3DF240, 0x3DF270], "func_003d81d0", {0x3E2AB0: (True, False, True, False), 0x3DF240: 3, 0x3DF270: (True, False, True, False)}),
    "003dddf0": (gen_ddf0, True, (True, True), [0x443010, 0x3DDCA0, 0x3DDA50], "func_003dddf0", {0x443010: 2, 0x3DDCA0: 1, 0x3DDA50: 5}),
    "003dcb40": (gen_cb40, True, (True, True), [], "func_003dcb40"),
    "003dc610": (gen_c610, True, (True, True), [0x3DC490, 0x3DC510, 0x3DC590], "func_003dc610", {0x3DC490: 2, 0x3DC510: 2, 0x3DC590: 2}),
    "003bfae0": (gen_fae0, True, (True, True), [0x3BF930, 0x3E9240, 0x3E9700, 0x3E42A0], "func_003bfae0", {0x3BF930: 1, 0x3E9240: 1, 0x3E9700: 1, 0x3E42A0: 4}),
    "003c1d40": (gen_1d40, True, {None: (True, True)}, [0x3E5110], "func_003c1d40", {0x3E5110: (False, True, True, False)}),
    "00398410": (gen_8410, True, (True, True), [0x3EF3A0, 0x43F9C8], "func_00398410", {0x3EF3A0: 1, 0x43F9C8: 3}),
    "003a2bb0": (gen_2bb0, True, (True, True), [], "func_003a2bb0"),
    "003bc880": (gen_c880, True, (True, True), [0x3DF240, 0x3BC400], "func_003bc880", {0x3DF240: 3, 0x3BC400: 2}),
    "003c9d40": (gen_9d40, True, {None: (True, True), 132: (True, True)}, [0x3C9A80, 0x3C49A0, 0x3C91D0, 0x3C9300, 0x3E3C20], "func_003c9d40", {0x3C9A80: 1, 0x3C49A0: 1, 0x3C91D0: 1, 0x3C9300: 1, 0x3E3C20: 2}),
    "003c0f20": (gen_0f20, True, {None: (True, True), 132: (True, False)},
                 [0x3DF050, 0x3E2910, 0x3C0640, 0x3E3C20, 0x3BFF30, 0x3C0050, 0x3BFFC0, 0x3E95C0, 0x3DF590, 0x3DF4D0, 0x3E5DF0, 0x3E59E0,
                  0x3BEE80, 0x3BF5F0, 0x3C2A80, 0x3DF360, 0x3C3AE0, 0x3EFD20, 0x3E53B0, 0x3E3110, 0x3BF330, 0x3BF200, 0x3BF240], "func_003c0f20",
                 {0x3DF050: (True, True, False, False), 0x3E2910: 3, 0x3C0640: (False, False, False, False), 0x3E3C20: 2,
                  0x3BFF30: (True, False, True, False), 0x3C0050: (True, False, True, False), 0x3BFFC0: (True, False, True, False),
                  0x3E95C0: 1, 0x3DF590: 1, 0x3DF4D0: 0, 0x3E5DF0: (True, False, False, False), 0x3E59E0: (False, False, False, False),
                  0x3BEE80: (True, False, False, False), 0x3BF5F0: (True, False, False, False), 0x3C2A80: 1, 0x3DF360: 3,
                  0x3C3AE0: 1, 0x3EFD20: 2, 0x3E53B0: 1, 0x3E3110: 3, 0x3BF330: 0, 0x3BF200: 0, 0x3BF240: 0}),
    "003bee80": (gen_ee80, True, {None: (True, True), 130: (True, True), 132: (True, False)}, [0x3DF050, 0x3E2910, 0x3C30B0, 0x3C2A80, 0x3DF590, 0x3DF4D0], "func_003bee80",
                 {0x3DF050: (True, True, False, False), 0x3E2910: 3, 0x3C30B0: 1, 0x3C2A80: 1, 0x3DF590: 2, 0x3DF4D0: 0}),
    "003963c0": (gen_key2, False, (True, True), [], "func_003963c0"),
    "00396520": (gen_key3, False, (True, True), [], "func_00396520"),
    "00390350": (gen_0350, True, (True, True), [0x3DF360, 0x3DF050, 0x38FE90, 0x38FD30], "func_00390350",
                 {0x3DF360: 3, 0x3DF050: (True, True, False, False), 0x38FE90: 5, 0x38FD30: 1}),
    "003990a0": (gen_90a0, True, (True, True), [0x3E6240], "func_003990a0", {0x3E6240: 1}),
    "00399bf0": (gen_9bf0, True, {None: (True, True), 131: (True, True)}, [0x43F9C8, 0x398410, 0x39A590, 0x39A7F0], "func_00399bf0",
                 {0x43F9C8: 3, 0x398410: 1, 0x39A590: 3, 0x39A7F0: 1}),
    "00394d70": (gen_dispatch, True, (True, True), [0x390680, 0x391400, 0x391C10, 0x3BFAE0], "func_00394d70", {0x3BFAE0: 1}),
    "00394e70": (gen_dispatch2, True, (True, True), [0x394C70, 0x3BFAE0, 0x3E9700, 0x3E0960, 0x3E42E0, 0x3941E0, 0x392BF0], "func_00394e70",
                 {0x3BFAE0: 1, 0x3E9700: 1, 0x3E0960: 0, 0x3E42E0: 0}),
    "00395290": (gen_5290, True, (True, True), [0x3DF050, 0x3DF360, 0x3DF300, 0x394FD0, 0x395100, 0x3DF590, 0x3DF4D0], "func_00395290",
                 {0x3DF050: (True, True, False, False), 0x3DF360: 3, 0x3DF300: 3, 0x394FD0: 1, 0x395100: 2, 0x3DF590: 1, 0x3DF4D0: 0}),
    "00398890": (gen_8890, True, (True, True), [0x3DF360, 0x3DF050, 0x3E6430, 0x3DF550, 0x3DF4D0], "func_00398890",
                 {0x3DF360: 3, 0x3DF050: 4, 0x3E6430: 1, 0x3DF550: 0, 0x3DF4D0: 0}),
    "00399470": (gen_9470, False, (True, True), [], "func_00399470"),
    "0039beb0": (gen_beb0, True, (True, True), [0x3E9700, 0x3E0960, 0x3E05F0, 0x3E0670], "func_0039beb0", {0x3E9700: 1, 0x3E0960: 0, 0x3E05F0: 0, 0x3E0670: 0}),
    "00399db0": (gen_9db0, True, (True, True), [0x3EF3A0, 0x399470, 0x3EF6D0, 0x3EF650, 0x39AB10, 0x3EF5B0], "func_00399db0",
                 {0x3EF3A0: 1, 0x399470: 3, 0x3EF6D0: 0, 0x3EF650: 2, 0x39AB10: 2, 0x3EF5B0: 1}),
    "003985b0": (gen_85b0, True, {None: (True, True), 131: (True, True)},
                 [0x39A760, 0x39A700, 0x39A630, 0x39A690, 0x39A4C0, 0x39A590, 0x39A340, 0x39A3A0, 0x39A460, 0x39A400, 0x39A150, 0x39A200,
                  0x39A260, 0x39A2E0, 0x39A090, 0x39A0F0, 0x399FD0, 0x39A030, 0x399BF0, 0x43F9C8], "func_003985b0", {0x43F9C8: 3}),
    "0039c000": (gen_c000, False, (True, True), [], "func_0039c000"),
    "003a29f0": (gen_29f0, True, (True, True), [0x40F570, 0x410390], "func_003a29f0", {0x40F570: 2, 0x410390: 3}),
    "0039c360": (gen_c360, True, (True, True), [0x3B8510, 0x3E9700, 0x3E0960], "func_0039c360", {0x3B8510: 1, 0x3E9700: 1, 0x3E0960: 0}),
    "00398970": (gen_8970, True, (True, True), [0x3DF240, 0x3DF1A0, 0x3E62B0], "func_00398970", {0x3DF240: 3, 0x3DF1A0: 3, 0x3E62B0: 2}),
    "00398d20": (gen_8d20, True, {None: (True, True), 131: (True, True)},
                 [0x3DF360, 0x3DF300, 0x398890, 0x399BF0, 0x39A030, 0x39A2E0, 0x39A150, 0x39A260, 0x39A4C0, 0x39A590, 0x3EF3A0, 0x399DB0, 0x43F9C8], "func_00398d20", {0x43F9C8: 3}),
    "00392a50": (gen_2a50, True, {None: (True, True), 135: (True, True, True, True)}, [0x3D9760], "func_00392a50", {0x3D9760: 4}),
    "00395bd0": (gen_5bd0, False, (True, True), [0x448D10], "func_00395bd0"),
    "003955a0": (gen_55a0, False, (True, True), [0x448D10], "func_003955a0"),
    "00397c40": (gen_7c40, True, {None: (True, True), 136: (True, True)}, [0x397480, 0x3E9240, 0x3E9700, 0x3E05F0, 0x3E9680], "func_00397c40",
                 {0x397480: 1, 0x3E9240: 1, 0x3E9700: 1, 0x3E9680: 1}),
    "003dc490": (_gen_dc(0), True, (True, True), [], "func_003dc490"),
    "003dc510": (_gen_dc(1), True, (True, True), [], "func_003dc510"),
    "003dc590": (_gen_dc(2), True, (True, True), [], "func_003dc590"),
    "003dd390": (gen_d390, True, {None: (True, True), 134: (True, True)}, [], "func_003dd390"),
    "003cc500": (gen_cc500, False, (True, True), [], "func_003cc500"),
    "003d5750": (gen_5750, True, (True, True), [], "func_003d5750"),
    "003d2010": (gen_2010, False, (True, True), [], "func_003d2010"),
    "003d8070": (gen_8070, True, (True, True), [0x3D5750, 0x3D5790, 0x3D5840, 0x3D7490, 0x3D5830], "func_003d8070",
                 {0x3D5750: 1, 0x3D5790: 2, 0x3D5840: 2, 0x3D7490: 1, 0x3D5830: 1}),
    "003bbea0": (gen_bea0, False, {None: (True, True), 132: (True, False)}, [], "func_003bbea0"),
    "003db360": (gen_b360, True, {None: (True, True), 128: (True, True)}, [0x3DB190], "func_003db360"),
    "003c66c0": (gen_66c0, True, {None: (True, True), 132: (True, False)}, [0x3E12F0], "func_003c66c0", {0x3E12F0: 1}),
    "003ca740": (gen_thunk(1), True, {None: (True, True), 128: (True, False, False, False)}, [0x3CB300], "func_003ca740", {0x3CB300: 2}),
    "003ca740@3ca780": (gen_thunk(2), True, {None: (True, True), 128: (True, True, True, True)}, [0x3CB300], "func_003ca780", {0x3CB300: 2}),
    "003ca740@3ca7a0": (gen_thunk(3), True, {None: (True, True), 128: (True, False, False, False), 129: (True, True, True, False)}, [0x3CB300], "func_003ca7a0", {0x3CB300: 2}),
    "003dd430": (gen_d430, True, {None: (True, True)}, [0x442948], "func_003dd430"),
    "003c55f0": (gen_55f0, True, {None: (True, True), 128: (True, True)}, [0x3DF590, 0x3DF4D0], "func_003c55f0", {0x3DF590: 2, 0x3DF4D0: 0}),
    "003d7b40": (gen_7b40, True, (True, True), [0x3D51C0, 0x442DE8], "func_003d7b40", {0x3D51C0: 3, 0x442DE8: 3}),
    "003bc620": (gen_bc620, True, {None: (True, True), 134: (True, True)}, [], "func_003bc620"),
    "003dc370": (gen_c370, True, (True, True), [0x3E2910, 0x3EF0D0, 0x3EF6D0, 0x3EF6B0, 0x3DBE40, 0x3DB630], "func_003dc370",
                 {0x3E2910: 3, 0x3EF0D0: 0, 0x3EF6D0: 0, 0x3EF6B0: 1, 0x3DBE40: 1, 0x3DB630: 1}),
    "003bc740": (gen_bc740, False, {None: (True, True), 132: (True, False)}, [], "func_003bc740"),
    "003d51c0": (gen_51c0, True, {None: (True, True), 130: (True, True)}, [0x3DF590, 0x3DF4D0], "func_003d51c0", {0x3DF590: 1, 0x3DF4D0: 0}),
    "003cfff0": (gen_fff0, False, (True, True), [], "func_003cfff0"),
    "003deea0": (gen_eea0, True, (True, True), [0x3E2910, 0x3DF590, 0x3DF4D0, 0x3DED20], "func_003deea0", {0x3E2910: 3, 0x3DF590: 1, 0x3DF4D0: 0, 0x3DED20: 0}),
    "003bebb0": (gen_bebb0, True, {None: (True, True), 130: (True, True)}, [0x3BFE60, 0x3DF590, 0x3DF4D0, 0x3C2A60], "func_003bebb0", {0x3BFE60: 1, 0x3DF590: 2, 0x3DF4D0: 0, 0x3C2A60: 1}),
    "003d3780": (gen_3780, True, (True, True), [0x4125D0, 0x412520], "func_003d3780"),
    "003de110": (gen_e110, True, {None: (True, True), 128: (False, True), 129: (True, True), 132: (True, False), 134: (True, True)}, [0x442DE8], "func_003de110", {0x442DE8: 3}),
    "003a9f40": (gen_9f40, True, {None: (True, True, True, True), 128: (True, True), 137: (True, True)}, [0x3A93B0, 0x3E9700, 0x3A14F0, 0x3A9880], "func_003a9f40",
                 {0x3A93B0: 4, 0x3E9700: 1, 0x3A14F0: 2, 0x3A9880: 4}),
    "003bc150": (gen_c150, True, {None: (True, True), 130: (True, True)}, [0x3DF360, 0x3E2910, 0x3DF300], "func_003bc150", {0x3DF360: 3, 0x3E2910: 3, 0x3DF300: 3}),
    "003d72a0": (gen_72a0, True, (True, True), [0x3E0F40, 0x3D7C50, 0x3D5830], "func_003d72a0", {0x3E0F40: 1, 0x3D7C50: 1, 0x3D5830: 1}),
    "00440bb8": (gen_qsort, False, (True, True, True, True), [0x3C6080], "p4t_qsort_440bb8", {0x3C6080: (True, True, False, False)}),
    "003e4030": (gen_norm(3), True, (True, True), [0x3DF4D0], "p4t_n3_3e4030", {0x3DF4D0: 0}),
    "003e41e0": (gen_norm(2), True, (True, True), [0x3DF590, 0x3DF4D0], "p4t_n2_3e41e0", {0x3DF590: 1, 0x3DF4D0: (False, True, True, False)}),
    "00399320": (gen_gpword(0x007646D4), True, (True, True), [], "func_00399320", {}),
    "00399450": (gen_gpword(0x007646D8), True, (True, True), [], "func_00399450", {}),
    "0039a8a0": (gen_a8a0, False, (True, True), [], "func_0039a8a0", {}),
    "0039f050": (gen_f050, False, {None: (True, True), 139: (True, True, False, False)}, [], "func_0039f050", {}),
    "003d0850": (gen_d0850, True, {None: (True, True), 130: (True, True)}, [0x3D0460, 0x3E1FF0], "func_003d0850", {0x3D0460: 4, 0x3E1FF0: 4}),
    "003d86a0": (gen_d86a0, True, {None: (True, True), 130: (True, True), 132: (True, False, False, False)}, [0x3D8500], "func_003d86a0", {0x3D8500: (True, True, True, True)}),
    "003ebbf0": (gen_ebbf0, True, {None: (True, True), 130: (True, True)}, [0x3DF050, 0x3E2910, 0x3E2CE0, 0x3E3B70, 0x3EA3E0], "func_003ebbf0",
                 {0x3DF050: (True, True, False, False), 0x3E2910: (True, True, True, True), 0x3E2CE0: 2, 0x3E3B70: 4, 0x3EA3E0: 1}),
    "003d0460": (gen_d0460, False, {None: (True, True), 130: (True, True)}, [0x3E1FF0], "func_003d0460", {0x3E1FF0: 4}),
    "004c1a68": (gen_ac3_disp(0x007149F8, True), False, (True, True, False, False), [], "func_004c1a68", {}),
    "004c1ab8": (gen_ac3_disp(0x007149FC, False), False, (True, False, False, False), [], "func_004c1ab8", {}),
    "004c1a98": (gen_ac3_smpl, False, (True, True), [], "ADXB_SetAc3DecSmpl", {}),
    "003ed960": (gen_ed960, False, (True, True), [0x3E05F0, 0x3ED850], "func_003ed960", {0x3E05F0: 4, 0x3ED850: 2}),
    "003cebb0": (gen_cebb0, True, {None: (True, True), 130: (True, True)}, [0x3C9C20, 0x3DF050, 0x3DF590, 0x3DF4D0, 0x3E2910, 0x3CE050, 0x3CC6E0, 0x43F9C8, 0x3CE230, 0x4220D8], "_rpWorldSectorNativeRead",
                 {0x3C9C20: 1, 0x3DF050: (True, True, False, False), 0x3DF590: 1, 0x3DF4D0: 0, 0x3E2910: 3, 0x3CE050: 2, 0x3CC6E0: 1, 0x43F9C8: (False, True, True, False), 0x3CE230: 1, 0x4220D8: 2}),
    "003ce560": (gen_e560, True, {None: (True, True), 130: (True, True)}, [0x3DF050, 0x3DF590, 0x3DF4D0, 0x3E2910, 0x3CE050, 0x3CC6E0, 0x43F9C8, 0x3CE230, 0x4220D8], "func_003ce560",
                 {0x3DF050: (True, True, False, False), 0x3DF590: 1, 0x3DF4D0: 0, 0x3E2910: 3, 0x3CE050: 2, 0x3CC6E0: 1, 0x43F9C8: (False, True, True, False), 0x3CE230: 1, 0x4220D8: 2}),
    "003ce3a0": (gen_e3a0, True, (True, True), [0x3CC6E0, 0x3DEFF0, 0x3CE840, 0x3E2AB0, 0x3CE170, 0x3CE230, 0x4220D8], "func_003ce3a0",
                 {0x3CC6E0: 1, 0x3DEFF0: 4, 0x3CE840: 1, 0x3E2AB0: 3, 0x3CE170: 1, 0x3CE230: 1, 0x4220D8: 2}),
    "003ce9e0": (gen_e9e0, True, (True, True), [0x3C9C20, 0x3CC6E0, 0x3DEFF0, 0x3CEEB0, 0x3E2AB0, 0x3CE170, 0x3CE230, 0x4220D8], "func_003ce9e0",
                 {0x3C9C20: 1, 0x3CC6E0: 1, 0x3DEFF0: 4, 0x3CEEB0: 1, 0x3E2AB0: 3, 0x3CE170: 1, 0x3CE230: 1, 0x4220D8: 2}),
    "003ceeb0": (gen_eeb0, True, (True, True), [0x3C9C20], "func_003ceeb0", {0x3C9C20: 1}),
    "0038fb50": (gen_fb50, True, (True, True), [0x3DEFF0, 0x3E2AB0, 0x3DF270], "func_0038fb50", {0x3DEFF0: 4, 0x3E2AB0: 3, 0x3DF270: 3}),
    "003dda50": (gen_dda50, True, {None: (True, True), 128: (True, True), 129: (True, True, True, True), 132: (True, False)}, [0x421820, 0x421800, 0x3DDC10], "func_003dda50", {0x421820: 1, 0x421800: 1, 0x3DDC10: 0}),
    "003a6df0": (gen_6df0, False, (True, True), [0x3CE050, 0x3BFAE0], "func_003a6df0", {0x3CE050: 2, 0x3BFAE0: 1}),
    "003d20d0": (gen_20d0, False, (True, True), [0x3D2010, 0x3E05F0], "func_003d20d0", {0x3D2010: 2}),
    "003d7490": (gen_7490, True, (True, True), [0x3E0F80, 0x3E0F40], "func_003d7490", {0x3E0F80: 0, 0x3E0F40: 1}),
    "003d7350": (gen_7350, True, (True, True), [0x3D5750, 0x3D5790, 0x3D59A0, 0x3D7CD0, 0x3D7C50, 0x3D5830], "func_003d7350"),
    "003d75b0": (gen_75b0, True, (True, True), [0x3DEFF0, 0x3DF240, 0x3E2AB0, 0x3D8060], "func_003d75b0", {0x3DEFF0: 4, 0x3DF240: 3, 0x3E2AB0: 3, 0x3D8060: 1}),
    "003d76d0": (gen_76d0, True, (True, True), [0x3D7B40, 0x3D65E0], "func_003d76d0"),
    "003d65e0": (gen_65e0, True, (True, True), [0x3D6C10, 0x3D6FB0], "func_003d65e0", {0x3D6FB0: 2}),
    "003d6fb0": (gen_6fb0, True, (True, True), [0x44B950, 0x3E0C90, 0x3E0870], "func_003d6fb0"),
    "003d77f0": (gen_77f0, True, (True, True), [0x3DF050, 0x3DF360, 0x3E2910, 0x3D60D0, 0x3D6170, 0x3D76D0, 0x3D7CD0, 0x3D5750, 0x3D5790, 0x3D5840, 0x3D7C50, 0x3D7490], "func_003d77f0",
                 {0x3DF050: (True, True, False, False)}),
    "003b8ad0": (gen_8ad0, True, {None: (True, True), 130: (True, True), 131: (True, True)},
                 [0x3DF050, 0x3DF590, 0x3DF4D0, 0x3DF360, 0x3E2910, 0x43F9C8, 0x3B7290, 0x3B8410], "func_003b8ad0",
                 {0x3DF590: 1, 0x3DF4D0: 0, 0x43F9C8: (True, True, True, False), 0x3B7290: 2, 0x3B8410: 2}),
    "003bc9c0": (gen_c9c0, True, {None: (True, True), 130: (True, True), 132: (True, False), 134: (True, True)}, [0x43F9C8, 0x3BD110], "func_003bc9c0", {0x43F9C8: 3, 0x3BD110: 1}),
    "003d0540": (gen_0540, True, (True, True), [0x3CE050, 0x3CFFF0, 0x3E9700, 0x3E05F0, 0x3BFAE0, 0x3D0140, 0x3D0230], "func_003d0540",
                 {0x3CE050: 2, 0x3CFFF0: 2, 0x3E9700: 1, 0x3BFAE0: 1, 0x3D0140: 2, 0x3D0230: 1}),
    "003c6080": (gen_6080, True, (True, True), [], "func_003c6080"),
    "003c67b0": (gen_67b0, True, {None: (True, True), 130: (True, True), 131: (True, True)}, [0x3E1210, 0x3C65B0], "func_003c67b0", {0x3E1210: 4}),
    "003c57d0": (gen_57d0, True, {None: (True, True, True, False), 130: (True, True)}, [0x3DF590, 0x3DF4D0], "func_003c57d0", {0x3DF590: 2, 0x3DF4D0: 0}),
    "003c1ea0": (gen_1ea0, True, {None: (True, True, True, False), 130: (True, True)}, [0x3DF590, 0x3DF4D0], "func_003c1ea0", {0x3DF590: 2, 0x3DF4D0: 0}),
    "003d8500": (gen_8500, True, (True, True), [0x3E2AB0], "func_003d8500"),
    "003db190": (gen_b190, False, (True, True), [0x3E4180, 0x44B6C0], "func_003db190"),
    "003de4d0": (gen_de4d0, True, (True, True), [0x427338], "func_003de4d0"),
    "003d3bd0": (gen_3bd0, True, (True, True), [0x3CE050, 0x3D3AE0, 0x3C96D0, 0x3C9750, 0x3D3A90], "func_003d3bd0", {0x3CE050: 2, 0x3D3AE0: 1, 0x3C96D0: 1, 0x3C9750: 1}),
    "003dbbc0": (gen_bbc0, True, (True, True), [0x3DF050, 0x3E2910, 0x3DF590, 0x3DF4D0, 0x43F9C8, 0x3EF020, 0x3EF060, 0x3EF000, 0x3EF040, 0x3E4F60, 0x3EF2E0, 0x3EF470, 0x3EF510, 0x3E3560], "func_003dbbc0",
                 {0x3DF050: (True, True, False, False), 0x3E2910: (True, False, True, False), 0x3DF590: 1, 0x3DF4D0: 0, 0x3EF000: 1, 0x3EF040: 1, 0x3E3560: 2, 0x3EF2E0: 1}),
    "003caef0": (gen_aef0, True, {None: (True, True), 128: (True, True), 129: (True, False), 131: (True, True)}, [0x3E9700, 0x3DF590, 0x3DF4D0], "func_003caef0", {0x3E9700: 1, 0x3DF590: 1, 0x3DF4D0: 0}),
    "003d7cf0": (gen_7cf0, True, {None: (True, True), 128: (False, True, False, False)}, [0x3E05D0, 0x3E0C90, 0x3E0870, 0x3E0E20, 0x39A700, 0x3D6900, 0x3D6C50], "func_003d7cf0"),
    "003c8920": (gen_8920, True, {None: (True, True), 132: (True, False, False, False)}, [0x3C54A0, 0x3DF590, 0x3DF4D0, 0x440BB8, 0x3C59F0, 0x3C6080, 0x3C6280], "func_003c8920", {0x3DF590: 2, 0x3DF4D0: 0, 0x3C54A0: 1}),
    "003dc740": (gen_c740, True, (True, True), [0x3DF590, 0x3DF4D0, 0x3E40B0, 0x44B7B0, 0x44B610], "func_003dc740", {0x3DF590: 1}),
    "003c2630": (gen_2630, True, {None: (True, True), 132: (True, False, False, False)}, [0x3DF590, 0x3DF4D0, 0x3C4A40, 0x3E3B70, 0x3C49A0, 0x3C1EA0, 0x43F9C8], "func_003c2630", {0x3DF590: 2, 0x3C4A40: 1, 0x3E3B70: 2, 0x3C49A0: 1, 0x3C1EA0: 2}),
    "003c4390": (gen_4390, True, (True, True), [0x3DF050, 0x3DF590, 0x3DF4D0, 0x3E2910, 0x3E3B70, 0x3E3C20, 0x3EF3A0, 0x3E3110, 0x3E32F0, 0x3E6430, 0x43F9C8], "func_003c4390",
                 {0x3DF050: (True, True, False, False), 0x3E2910: (True, False, True, False), 0x3DF590: 1, 0x3DF4D0: 0, 0x3E3B70: 2, 0x3E3C20: 2, 0x3EF3A0: 1, 0x3E3110: 3, 0x3E32F0: 4, 0x3E6430: 1}),
    "003c7d90": (gen_7d90, True, {None: (True, True), 132: (True, False, False, False)}, [], "func_003c7d90"),
    "003cfa80": (gen_fa80, False, (True, True), [0x3E9700], "func_003cfa80", {0x3E9700: 1}),
    "003bb5b0": (gen_b5b0, True, (True, True), [0x3DF590, 0x3DF4D0, 0x3E4030], "func_003bb5b0", {0x3DF590: 1}),
    "003be180": (gen_e180, True, {None: (True, True), 130: (True, True)}, [0x3DF050, 0x3DF590, 0x3DF4D0, 0x3E2910, 0x43F9C8, 0x3C9940, 0x3C9D40, 0x3C4FA0, 0x3BD6B0, 0x3BDD00, 0x3C97E0, 0x3C9E80, 0x3C42B0, 0x3E3B70, 0x3E3110, 0x3E32F0, 0x3C9B30], "func_003be180",
                 {0x3DF050: (True, True, False, False), 0x3E2910: (True, False, True, False), 0x3DF590: 2, 0x3DF4D0: 0, 0x3BD6B0: 4, 0x3BDD00: 4}),
    "003d9760": (gen_9760, True, (True, True), [], "func_003d9760"),
    "003a1730": (gen_1730, True, (True, True), [], "func_003a1730"),
    "003a1c40": (gen_1730, True, (True, True), [], "func_003a1c40"),
    "00399530": (gen_9530, True, (True, True), [0x3EA2C0, 0x3EA3E0, 0x3EC110, 0x3ED0B0, 0x3EA370, 0x3EA560, 0x3EA690, 0x3EC1F0, 0x3EC590, 0x3EC180, 0x3EF2E0, 0x399470, 0x3EF470], "func_00399530", {0x3EA2C0: 3, 0x3EA3E0: 1, 0x3EC110: 2, 0x3ED0B0: 2, 0x3EA370: 1, 0x3EA560: 1, 0x3EA690: 2, 0x3EC590: 4, 0x3EC180: 2, 0x3EF2E0: 1}),
    "00394870": (gen_4870, True, (True, True), [0x3D9760], "func_00394870"),
    "00392650": (gen_2650, False, (True, True), [], "func_00392650"),
    "003941e0": (gen_41e0, False, (True, True), [], "func_003941e0"),
    "00390680": (gen_0680, False, (True, True), [], "func_00390680"),
    "00391400": (gen_1400, False, (True, True), [0x3D9F30], "func_00391400", {0x3D9F30: 4}),
    "00391c10": (gen_1c10, False, (True, True), [0x3D9760], "func_00391c10", {0x3D9760: 6}),
    "00392bf0": (gen_2bf0, False, (True, True), [0x392650], "func_00392bf0", {0x392650: 4}),
    "00393900": (gen_3900, False, (True, True), [0x3D9760, 0x392A50], "func_00393900", {0x3D9760: 6, 0x392A50: 4}),
    "00397480": (gen_7480, True, {None: (True, True), 128: (False, True, False, False)}, [0x3E9240, 0x3E9700, 0x3E05F0, 0x3E9680, 0x3954B0], "func_00397480", {0x3E9240: 1, 0x3E9700: 1, 0x3E05F0: 0, 0x3E9680: 1, 0x3954B0: 0}),
    "003de2d0@3de2f0": (gen_e2f0, True, {None: (True, True), 128: (True, False, False, False)}, [0x3DD390, 0x3DD430, 0x426CF0, 0x426F80, 0x4270F8, 0x3DDC20], "func_003de2f0", {0x3DD390: 1, 0x3DD430: (False, True, True, True), 0x426CF0: (True, False, False, False), 0x426F80: 1, 0x4270F8: 3}),
    "003a93b0": (gen_93b0, True, (True, True), [0x3E42A0, 0x39C900, 0x4114D0], "func_003a93b0", {0x3E42A0: 2, 0x39C900: 2, 0x4114D0: 3}),
    "003d0230": (gen_0230, False, (True, True, True, True), [0x3E9700, 0x3E0960, 0x3C96D0, 0x3BFAE0, 0x3CFA80, 0x3CFFA0], "func_003d0230", {0x3E9700: 1, 0x3BFAE0: 1, 0x3CFFA0: 2}),
    "003bd6b0": (gen_d6b0, True, (True, True), [0x3DF050, 0x3DF590, 0x3DF4D0, 0x3E2910, 0x43F9C8, 0x3DF300, 0x3DF2A0, 0x3C91D0, 0x3C9300, 0x3E3B70, 0x3E3110, 0x3E32F0], "func_003bd6b0",
                 {0x3DF050: 2, 0x3DF590: 2, 0x3DF4D0: (False, True, True, False), 0x3E2910: 3, 0x43F9C8: 3, 0x3DF300: 3, 0x3DF2A0: 3, 0x3C91D0: 1, 0x3C9300: 1, 0x3E3B70: 2, 0x3E3110: 3, 0x3E32F0: 4}),
    "003c70e0": (gen_70e0, True, {None: (True, True), 128: (True, True), 132: (True, False, False, False)}, [0x3C67B0, 0x3C6A40, 0x3C6C10, 0x3C66C0], "func_003c70e0",
                 {0x3C67B0: (True, False, False, True), 0x3C6A40: (True, False, True, False), 0x3C6C10: 4, 0x3C66C0: 4}),
    "003d9f30": (gen_9f30, True, (True, True), [], "func_003d9f30"),
    "0039bbd0": (gen_bbd0, True, (True, True), [0x4115D0, 0x412CA0, 0x412FB0, 0x412E90, 0x3D3A80, 0x3D3130, 0x3D35F0, 0x3D3460, 0x3D3780, 0x3DF590, 0x3DF4D0, 0x4125E0, 0x3D38E0, 0x3D3920, 0x3D39A0], "func_0039bbd0",
                 {0x4115D0: 0, 0x412CA0: 1, 0x412FB0: 4, 0x412E90: 4, 0x3D3A80: 0, 0x3D3130: 3, 0x3D35F0: 4, 0x3D3460: 4, 0x3D3780: 3, 0x3DF590: 1, 0x3DF4D0: 0, 0x4125E0: 1, 0x3D38E0: 2, 0x3D3920: 3, 0x3D39A0: 3}),
}


# Entries that are also callees of another entry cannot share a harness; run them one per
# invocation (`cases.py 0039ba80`, `cases.py 0039bbd0`).


# Callees whose retail body is hardware (SPR DMA); the interpreter runs a plain-Python model of
# the documented behaviour while the C side links the real override.
def _py_memcpy(m):
    n = m.g32(6)
    if n and n < 1 << 31:
        m.mem.write(m.g32(4), m.mem.read(m.g32(5), n))


def _py_fill16(m):
    n = sx(m.g32(5)) // 16 if sx(m.g32(5)) >= 0 else -((-sx(m.g32(5))) // 16)
    pat = m.mem.read(m.g32(6), 16)
    for i in range(max(n, 0)):
        m.mem.write(m.g32(4) + 16 * i, pat)


def _py_fill32(m):
    n = sx(m.g32(5)) // 16 if sx(m.g32(5)) >= 0 else -((-sx(m.g32(5))) // 16)
    pat = m.mem.read(m.g32(6), 32)
    for i in range(max(n, 0)):
        m.mem.write(m.g32(4) + 16 * i, pat[16 * (i & 1):16 * (i & 1) + 16])


def _py_fillword(m):
    total = sx(m.g32(5))
    q = total // 16 if total >= 0 else -((-total) // 16)
    r = total - 16 * q
    w = m.g32(6).to_bytes(4, "little")
    dst = m.g32(4)
    for i in range(max(q, 0)):
        m.mem.write(dst + 16 * i, w * 4)
    for i in range(max(r, 0)):
        m.mem.write(dst + 16 * max(q, 0) + 4 * i, w)


def sx(v):
    return v - (1 << 32) if v & 0x80000000 else v


REAL = {0x3A4D50: ("003a4d50", _py_memcpy), 0x3A4850: ("003a4850", _py_fill16),
        0x3A4AB0: ("003a4ab0", _py_fill32), 0x3A4270: ("003a4270", _py_fillword)}

# Per-address extras: deref {callee: arg index}, c_impls {callee: C source}, impls {callee: python}.
def _py_copy64(m):
    m.mem.write(m.g32(4), m.mem.read(m.g32(5), 64))


def _py_sphere(m):
    import mips as _m
    src = m.g32(5)
    dst = m.g32(4)
    for i in range(3):
        v = _m.b2f(m.mem.rn(src + 4 * i, 4))
        m.mem.wn(dst + 4 * i, 4, _m.f2b(v * 2.0))
    m.mem.wn(dst + 12, 4, m.mem.rn(src + 12, 4))


def _py_reader_factory(addr, kind):
    """Stream reader stand-in: logs (id, stream, 0, n, 0), fails by the callee's countdown, then
    writes deterministic words (kind 'word': one word; 'rec20': five words; returns 0x14)."""
    def impl(m):
        cid = m.callees.index(addr)
        n = m.ncalls.get(cid, 0)
        m.ncalls[cid] = n + 1
        m.ilog.extend([cid, m.g32(4), 0, m.g32(6), 0])
        ret = m.cret.get(addr, 0)
        ok = (1 if n < (ret & 0xFFFF) else 0) if (ret >> 24) == 0xFE else ret
        if not ok:
            return 0
        k = n
        p = m.g32(5)
        if kind == "word":
            m.mem.wn(p, 4, 0x100 if k == 0 else (k * 2654435761) % 7 & 0xFFFFFFFF)
            return 1
        vals = [(k * 0x1111 + 1), (k % 5) - 2, k * 3, 0x3F000000 + (k & 7) * 0x100000, k % 4]
        for j, v in enumerate(vals):
            m.mem.wn(p + 4 * j, 4, v & 0xFFFFFFFF)
        return 0x14
    return impl


C_READER_WORD = ("unsigned func_003df360(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); unsigned k; "
                 "if (!r) return 0; k = p4_ncalls[{ID}] - 1; *p = (k == 0) ? 0x100u : (unsigned)(((unsigned long long)k * 2654435761ull) % 7u); return 1; }")
C_READER_REC20 = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned r = p4_record({ID}, s, 0, n, 0); unsigned k; "
                  "if (!r) return 0; k = p4_ncalls[{ID}] - 1; p[0] = k * 0x1111u + 1u; p[1] = (unsigned)((int)(k % 5u) - 2); p[2] = k * 3u; "
                  "p[3] = 0x3F000000u + (k & 7u) * 0x100000u; p[4] = k % 4u; return 0x14; }")

def _py_strncpy(m):
    d, s, n = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([m.callees.index(0x442DE8), d, s, n, 0])
    done = False
    for i in range(n):
        ch = 0 if done else m.mem.rb(s + i)
        m.mem.wb(d + i, ch)
        if ch == 0:
            done = True
    return d


def _body6_vals(k):
    f = [0x3F800000 + 0x10000 * (k % 5), 0x40000000 + (k % 2) * 0x100, 0x40000000 + (k % 2) * 0x100,
         0x40000000 + (k % 3) * 0x100, 0x3E800000 + 0x10000 * (k % 7)]
    f.append(((k * 37) % 256) << 16 | ((k * 91) & 0xFFFF))
    return f


def _py_df050(m):
    cid = m.callees.index(0x3DF050)
    n = m.ncalls.get(cid, 0)
    m.ncalls[cid] = n + 1
    m.ilog.extend([cid, m.g32(4), m.g32(5), 0, 0])
    if not m.cret.get(0x3DF050, 0):
        return 0
    m.mem.wn(m.g32(6), 4, 0x18)
    m.mem.wn(m.g32(7), 4, [0x34000, 0x35000, 0x30000 + 0x5000, 0x37002, 0x37003, 0x36000, 0x30200, 0x30300][n % 8] if False else [0x34000, 0x35000, 0x37002, 0x37003, 0x36000, 0x350FF][(m.g32(4) >> 4) % 6])
    return 1


def _py_e2910_body(m):
    cid = m.callees.index(0x3E2910)
    n = m.ncalls.get(cid, 0)
    m.ncalls[cid] = n + 1
    m.ilog.extend([cid, m.g32(4), 0, m.g32(6), 0])
    if not m.cret.get(0x3E2910, 0):
        return 0
    for j, v in enumerate(_body6_vals(m.g32(4) >> 4)):
        m.mem.wn(m.g32(5) + 4 * j, 4, v)
    return m.g32(6)


C_DF050 = ("unsigned func_003df050(unsigned s, unsigned x, unsigned *sz, unsigned *ver) { unsigned k[6] = {0x34000, 0x35000, 0x37002, 0x37003, 0x36000, 0x350FF}; "
           "p4_record({ID}, s, x, 0, 0); if (!p4_ret[{ID}]) return 0; *sz = 0x18; *ver = k[(s >> 4) % 6]; return 1; }")
C_E2910_BODY = ("unsigned func_003e2910(unsigned s, unsigned *p, unsigned n) { unsigned k = s >> 4; unsigned f[6]; unsigned j; p4_record({ID}, s, 0, n, 0); "
                "if (!p4_ret[{ID}]) return 0; "
                "f[0] = 0x3F800000u + 0x10000u * (k % 5); f[1] = 0x40000000u + (k % 2) * 0x100u; f[2] = f[1]; f[3] = 0x40000000u + (k % 3) * 0x100u; "
                "f[4] = 0x3E800000u + 0x10000u * (k % 7); f[5] = (((k * 37u) % 256u) << 16) | ((k * 91u) & 0xFFFFu); "
                "for (j = 0; j < 6; j++) p[j] = f[j]; return n; }")

C_BEA60 = ("unsigned func_003bea60(unsigned e, unsigned *st) { unsigned r = p4_record({ID}, e, 0, 0, 0); if (!r) st[2] = 0; return r; }")
C_8970 = ("void func_003e8970(int *d, unsigned h) { p4_record({ID}, 0, h, 0, 0); d[0] = 3 + (int)(h % 5u); d[1] = 2 + (int)(h % 7u); }")
C_F260 = ("void func_003ef260(unsigned a, void *cb, unsigned *rec) { p4_record({ID}, a, 0, rec[1], rec[5]); }")


def _py_bea60(m):
    cid = m.callees.index(0x3BEA60)
    n = m.ncalls.get(cid, 0)
    m.ncalls[cid] = n + 1
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    ret = m.cret.get(0x3BEA60, 0)
    ok = (1 if n < (ret & 0xFFFF) else 0) if (ret >> 24) == 0xFE else ret
    if not ok:
        m.mem.wn(m.g32(5) + 8, 4, 0)
    return ok


def _py_8970(m):
    cid = m.callees.index(0x3E8970)
    m.ilog.extend([cid, 0, m.g32(5), 0, 0])
    h = m.g32(5)
    m.mem.wn(m.g32(4), 4, 3 + h % 5)
    m.mem.wn(m.g32(4) + 4, 4, 2 + h % 7)


def _py_f260(m):
    cid = m.callees.index(0x3EF260)
    r = m.g32(6)
    m.ilog.extend([cid, m.g32(4), 0, m.mem.rn(r + 4, 4), m.mem.rn(r + 0x14, 4)])


C_2AB0 = ("unsigned func_003e2ab0(unsigned s, unsigned char *p, unsigned n) { p4_record({ID}, s, *p, n, 0); return 1; }")
C_F270 = ("unsigned func_003df270(unsigned s, unsigned short *p, unsigned n) { p4_record({ID}, s, *p, n, 0); return 1; }")
C_F240W = ("unsigned func_003df240(unsigned s, unsigned *p, unsigned n) { p4_record({ID}, s, *p, n, 0); return 1; }")


def _mk_py_w(addr, width):
    def impl(m):
        cid = m.callees.index(addr)
        m.ilog.extend([cid, m.g32(4), m.mem.rn(m.g32(5), width), m.g32(6), 0])
        return 1
    return impl


C_5110 = ("void func_003e5110(float *b, float *p, int n) { int i, k; p4_record({ID}, 0, (unsigned)p, (unsigned)n, 0); "
          "for (k = 0; k < 6; k++) b[k] = 0.0f; if (n > 0) { for (k = 0; k < 3; k++) { b[k] = p[k]; b[3 + k] = p[k]; } "
          "for (i = 1; i < n; i++) for (k = 0; k < 3; k++) { if (p[3 * i + k] < b[k]) b[k] = p[3 * i + k]; if (p[3 * i + k] > b[3 + k]) b[3 + k] = p[3 * i + k]; } } }")


def _py_5110(m):
    import mips as _m
    cid = m.callees.index(0x3E5110)
    b, p, n = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, 0, p, n, 0])
    sn = n - (1 << 32) if n & 0x80000000 else n
    for k in range(6):
        m.mem.wn(b + 4 * k, 4, 0)
    if sn > 0:
        pts = [[_m.b2f(m.mem.rn(p + 12 * i + 4 * k, 4)) for k in range(3)] for i in range(sn)]
        for k in range(3):
            lo = min(q[k] for q in pts)
            hi = max(q[k] for q in pts)
            m.mem.wn(b + 4 * k, 4, _m.f2b(lo))
            m.mem.wn(b + 12 + 4 * k, 4, _m.f2b(hi))


_C85, _P85 = _stubs({0x39A760: (3, {"outs": (1, 2)}), 0x39A700: (3,), 0x39A630: (1,), 0x39A690: (3, {"ret": "void", "outs": (1, 2)}),
                     0x39A4C0: (2, {"ret": "void"}), 0x39A590: (3,), 0x39A340: (1,), 0x39A3A0: (1,), 0x39A460: (1, {"ret": "float"}),
                     0x39A400: (1,), 0x39A150: (2, {"ret": "void"}), 0x39A200: (2, {"ret": "void"}), 0x39A260: (2, {"ret": "void"}),
                     0x39A2E0: (1, {"ret": "void", "farg": True}), 0x39A090: (1,), 0x39A0F0: (1, {"ret": "float"}),
                     0x399FD0: (2, {"ret": "void"}), 0x39A030: (1, {"ret": "void", "farg": True}), 0x399BF0: (2,)})
_C9DB, _P9DB = _stubs({0x3EF3A0: (1, {"ret": "void"}), 0x399470: (3, {"ret": "void"}), 0x3EF6D0: (0,), 0x39AB10: (2,), 0x3EF5B0: (1, {"ret": "void"})})
_C9DB[0x3EF650] = ("unsigned func_003ef650(unsigned it, unsigned *buf) { unsigned h = 0, i; for (i = 0; i < 8; i++) h = h * 31u + buf[i]; return p4_record({ID}, it, h, 0, 0); }")


def _py_ef650(m):
    cid, n, ok = _ok(m, 0x3EF650)
    b = m.g32(5)
    h = 0
    for i in range(8):
        h = (h * 31 + m.mem.rn(b + 4 * i, 4)) & 0xFFFFFFFF
    m.ilog.extend([cid, m.g32(4), h, 0, 0])
    return ok


_P9DB[0x3EF650] = _py_ef650
_C9DB[0x399470] = "void func_00399470(unsigned char *o, unsigned a, unsigned b) { unsigned i; p4_record({ID}, 0, a, b, 0); for (i = 0; i < 32; i++) o[i] = (unsigned char)(a + b + i); }"


def _py_9470stub(m):
    cid = m.callees.index(0x399470)
    o, a, b = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, 0, a, b, 0])
    for i in range(32):
        m.mem.wb(o + i, (a + b + i) & 0xFF)


_P9DB[0x399470] = _py_9470stub

EXTRA = {
    "003c70e0": {"c_impls": _C_0E0, "impls": _P_0E0},
    "003bd6b0": {"c_impls": _C_D6B0, "impls": _P_D6B0},
    "003d0230": {"c_impls": _C_0230, "impls": {**_P_0230, 0x3E0960: _py_copy64}},
    "003a93b0": {"c_impls": _C_93B0, "impls": _P_93B0},
    "003de2d0@3de2f0": {"c_impls": _C_E2F0, "impls": _P_E2F0},
    "00397480": {"c_impls": _C_7480, "impls": _P_7480},
    "00393900": {"c_impls": {0x3D9760: C_9760S, 0x392A50: C_2A50S}, "impls": {0x3D9760: _py_9760s, 0x392A50: _py_2a50s}},
    "00392bf0": {"c_impls": {0x392650: C_2650S}, "impls": {0x392650: _py_2650s}},
    "00391c10": {"c_impls": {0x3D9760: C_9760S}, "impls": {0x3D9760: _py_9760s}},
    "00391400": {"c_impls": _C_1400, "impls": _P_1400},
    "00394870": {"c_impls": {0x3D9760: C_9760S}, "impls": {0x3D9760: _py_9760s}},
    "00399530": {"c_impls": _C_9530, "impls": _P_9530},
    "003be180": {"c_impls": _C_E180, "impls": _P_E180},
    "003bb5b0": {"c_impls": _C_B5B0, "impls": _P_B5B0},
    "003cfa80": {"c_impls": _C_FA80, "impls": _P_FA80},
    "003c4390": {"c_impls": _C_4390, "impls": _P_4390},
    "003c2630": {"c_impls": _C_2630, "impls": _P_2630},
    "003dc740": {"c_impls": _C_C740, "impls": _P_C740},
    "003c8920": {"c_impls": _C_8920, "impls": _P_8920},
    "003d7cf0": {"c_impls": _C7CF, "impls": _P7CF},
    "003caef0": {"c_impls": _C_AEF0, "impls": _P_AEF0},
    "003dbbc0": {"c_impls": _C_BBC0, "impls": _P_BBC0},
    "003d3bd0": {"c_impls": {0x3D3AE0: C_D3AE0, 0x3C96D0: C_C96D0, 0x3C9750: C_C9750}, "impls": {0x3D3AE0: _py_d3ae0, 0x3C96D0: _py_c96d0, 0x3C9750: _py_c9750}},
    "003de4d0": {"c_impls": {0x427338: C_SCEREAD}, "impls": {0x427338: _py_sceread}},
    "003db190": {"c_impls": {0x3E4180: C_E4180V}, "impls": {0x3E4180: _py_e4180v, 0x44B6C0: _py_fabs}},
    "003d8500": {"c_impls": {0x3E2AB0: C_E2AB0N}, "impls": {0x3E2AB0: _py_e2ab0n}},
    "003c67b0": {"c_impls": {0x3C65B0: C_65B0N}, "impls": {0x3C65B0: _py_65b0n}},
    "003c57d0": {"c_impls": {0x3DF4D0: C_DF4D0E}, "impls": {0x3DF4D0: _py_df4d0e}},
    "003c1ea0": {"c_impls": {0x3DF4D0: C_DF4D0E}, "impls": {0x3DF4D0: _py_df4d0e}},
    "003b8ad0": {"c_impls": {0x3DF050: C_DF050B, 0x3DF4D0: C_DF4D0E, 0x3DF360: C_DF360_PDS, 0x3E2910: C_E2910_RET},
                 "impls": {0x3DF050: _py_df050b, 0x3DF4D0: _py_df4d0e, 0x3DF360: _py_df360_pds, 0x3E2910: _py_e2910_ret}},
    "003d0540": {"c_impls": {0x3E05F0: C_E05F0W}, "impls": {0x3E05F0: _py_e05f0w}},
    "003d7490": {"deref": {}},
    "003d7350": {"c_impls": _C73, "impls": _P73},
    "003d75b0": {"deref": {0x3DF240: 1}, "c_impls": {0x3DEFF0: C_DEFF0, 0x3E2AB0: C_E2AB0H}, "impls": {0x3DEFF0: _py_deff0, 0x3E2AB0: _py_e2ab0h}},
    "003d76d0": {"c_impls": _C76, "impls": _P76},
    "003d65e0": {"c_impls": _C65, "impls": _P65},
    "003d6fb0": {"c_impls": _C6F, "impls": _P6F},
    "003d77f0": {"c_impls": _C77, "impls": _P77},
    "003bee80": {"c_impls": {0x3DF050: C_DF050B, 0x3E2910: C_E2910_N}, "impls": {0x3DF050: _py_df050b, 0x3E2910: _py_e2910_n}},
    "0038fb50": {"c_impls": {0x3DEFF0: C_DEFF0, 0x3E2AB0: C_E2AB0H}, "impls": {0x3DEFF0: _py_deff0, 0x3E2AB0: _py_e2ab0h}},
    "003d20d0": {"c_impls": {0x3E05F0: C_E05F0W}, "impls": {0x3E05F0: _py_e05f0w}},
    "00440bb8": {"c_impls": {0x3C6080: _C_QS_CMP}, "impls": {0x3C6080: _py_qs_cmp}},
    "003e4030": {"c_impls": {0x3DF4D0: _C_NORM_3}, "impls": {0x3DF4D0: _py_df4d0e}},
    "003e41e0": {"c_impls": {0x3DF4D0: _C_NORM_2}, "impls": {0x3DF4D0: _py_df4d0e}},
    "003d86a0": {"c_impls": {0x3D8500: _C_8500}, "impls": {0x3D8500: _py_8500}},
    "003ebbf0": {"c_impls": {0x3DF050: C_DF050B, 0x3E2910: _C_E2910_IMG, 0x3EA3E0: _C_EA3E0_IMG, 0x3E3B70: _C_B70_IMG}, "impls": {0x3DF050: _py_df050b, 0x3E2910: _py_e2910_img, 0x3EA3E0: _py_ea3e0_img, 0x3E3B70: _py_b70_img}},
    "003bbbe0": {"c_impls": {0x3DF590: "unsigned func_003df590(unsigned a, unsigned b, unsigned c, unsigned d) { return p4_record({ID}, a, a == 1u ? 0u : b, 0, 0); }"}, "impls": {0x3DF590: _py_df590_one}},
    "003bfae0": {"c_impls": {0x3E9240: _C_9240_LOW, 0x3E9700: _C_9700_LOW, 0x3E42A0: _C_42A0_LOW}, "impls": {0x3E9240: _py_low_ret(0x3E9240), 0x3E9700: _py_low_ret(0x3E9700), 0x3E42A0: _py_42a0_low}},
    "003ed960": {"c_impls": {0x3E05F0: _C_05F0_LOW}, "impls": {0x3E05F0: _py_05f0_low}},
    "003cebb0": {"c_impls": {0x3E2910: C_E2910_GEO, 0x3DF4D0: C_DF4D0E, 0x3DF050: C_DF050B}, "impls": {0x3E2910: _py_e2910_geo, 0x3DF4D0: _py_df4d0e, 0x3DF050: _py_df050b}},
    "003ce560": {"c_impls": {0x3E2910: C_E2910_GEO, 0x3DF4D0: C_DF4D0E, 0x3DF050: C_DF050B}, "impls": {0x3E2910: _py_e2910_geo, 0x3DF4D0: _py_df4d0e, 0x3DF050: _py_df050b}},
    "003ce3a0": {"deref": {0x3E2AB0: 1}},
    "003ce9e0": {"deref": {0x3E2AB0: 1}},
    "003bc150": {"c_impls": {0x3DF360: C_RD_CNT}, "impls": {0x3DF360: _py_rd_cnt}},
    "003bebb0": {"c_impls": {0x3DF4D0: C_DF4D0E}, "impls": {0x3DF4D0: _py_df4d0e}},
    "003d3780": {"c_impls": {0x4125D0: C_4125D0, 0x412520: C_412520}, "impls": {0x4125D0: _py_4125d0, 0x412520: _py_412520}},
    "003de110": {"c_impls": {0x442DE8: "#include <string.h>\nchar *func_00442de8(char *d, const char *s, unsigned n) { p4_record({ID}, (unsigned)0, (unsigned)s, n, 0); return strncpy(d, s, n); }"}, "impls": {0x442DE8: _py_strncpy_e110}},
    "003dc370": {"c_impls": {0x3E2910: C_E2910_HDR4, 0x3DB630: C_DB630}, "impls": {0x3E2910: _py_e2910_hdr4, 0x3DB630: _py_db630}},
    "003d51c0": {"c_impls": {0x3DF4D0: C_DF4D0E}, "impls": {0x3DF4D0: _py_df4d0e}},
    "003deea0": {"c_impls": {0x3E2910: C_E2910_12, 0x3DED20: C_DED20, 0x3DF4D0: C_DF4D0E}, "impls": {0x3E2910: _py_e2910_12, 0x3DED20: _py_ded20, 0x3DF4D0: _py_df4d0e}},
    "003dd430": {"c_impls": {0x442948: C_STRLEN2948}, "impls": {0x442948: _py_strlen2948}},
    "003c55f0": {"c_impls": {0x3DF4D0: C_DF4D0E}, "impls": {0x3DF4D0: _py_df4d0e}},
    "003d7b40": {"c_impls": {0x442DE8: "#include <string.h>\nchar *func_00442de8(char *d, const char *s, unsigned n) { p4_record({ID}, (unsigned)d, (unsigned)s, n, 0); return strncpy(d, s, n); }"}, "impls": {0x442DE8: _py_strncpy}},
    "003db360": {"c_impls": {0x3DB190: C_B190}, "impls": {0x3DB190: _py_b190}},
    "00397c40": {"c_impls": {0x3E05F0: C_E05F0W}, "impls": {0x3E05F0: _py_e05f0w}},
    "00395bd0": {"fuzzy": True, "c_impls": {0x448D10: "#include <math.h>\nfloat func_00448d10(float x) { return acosf(x); }"}, "impls": {}},
    "003955a0": {"fuzzy": True, "c_impls": {0x448D10: "#include <math.h>\nfloat func_00448d10(float x) { return acosf(x); }"}, "impls": {}},
    "00392a50": {"c_impls": {0x3D9760: C_D9760}, "impls": {0x3D9760: _py_d9760}},
    "00398970": {"deref": {0x3DF240: 1, 0x3DF1A0: 1}},
    "00398d20": {"c_impls": _C98D, "impls": _P98D},
    "0039c360": {"c_impls": {0x3E0960: "void func_003e0960(float *d, float *s) { for (int i = 0; i < 16; i++) d[i] = s[i]; }"}, "impls": {0x3E0960: _py_copy64}},
    "00398890": {"c_impls": {0x3DF360: C_DF360_BIT, 0x3DF550: C_DF550, 0x3DF4D0: C_DF4D0E}, "impls": {0x3DF360: _py_df360_bit, 0x3DF550: _py_df550, 0x3DF4D0: _py_df4d0e}},
    "0039beb0": {"c_impls": {0x3E0960: C_E0960, 0x3E05F0: C_E05F0, 0x3E0670: C_E0670}, "impls": {0x3E0960: _py_e0960, 0x3E05F0: _py_e05f0, 0x3E0670: _py_e0670}},
    "00399db0": {"c_impls": _C9DB, "impls": _P9DB},
    "003985b0": {"c_impls": _C85, "impls": _P85},
    "00394d70": {"c_impls": _H4C, "impls": _H4P},
    "00394e70": {"c_impls": {**_H4C, **_HEC, 0x3E0960: C_E0960, 0x3E42E0: C_E42E0}, "impls": {**_H4P, **_HEP, 0x3E0960: _py_e0960, 0x3E42E0: _py_e42e0}},
    "00395290": {"reader": {0x3DF360: "word"}},
    "00390350": {"c_impls": {0x38FE90: C_FE90}, "impls": {0x38FE90: _py_fe90}, "reader": {0x3DF360: "word"}},
    "003c0f20": EXTRA_F20,
    "003c1d40": {"c_impls": {0x3E5110: C_5110}, "impls": {0x3E5110: _py_5110}},
    "003bc880": {"deref": {0x3DF240: 1}},
    "003d81d0": {"c_impls": {0x3E2AB0: C_2AB0, 0x3DF270: C_F270, 0x3DF240: C_F240W},
                 "impls": {0x3E2AB0: _mk_py_w(0x3E2AB0, 1), 0x3DF270: _mk_py_w(0x3DF270, 2), 0x3DF240: _mk_py_w(0x3DF240, 4)}},
    "003c0520": {"c_impls": {0x3BEA60: C_BEA60}, "impls": {0x3BEA60: _py_bea60}},
    "003db490": {"c_impls": {0x3CA320: _C_CA320, 0x44B968: _C_B968, 0x3F6800: _C3F6800, 0x3DB440: _C3DB440},
                 "impls": {0x3CA320: _py_ca320, 0x44B968: _py_b968, 0x3F6800: _P3F6800, 0x3DB440: _P3DB440}},
    "003db550": {"c_impls": {0x3E8970: C_8970, 0x3EF260: C_F260}, "impls": {0x3E8970: _py_8970, 0x3EF260: _py_f260}},
    "003962e0": {"deref": {0x3DF240: 1}},
    "003961f0": {"reader": {0x3DF360: "word"}},
    "003c3ae0": {"c_impls": {0x3DF050: C_DF050, 0x3E2910: C_E2910_BODY}, "impls": {0x3DF050: _py_df050, 0x3E2910: _py_e2910_body}},
    "003d5600": {"deref": {0x3DF240: 1}},
    "003d53c0": {"reader": {0x3DF360: "word"}},
    "003ddca0": {"c_impls": {0x442DE8: "#include <string.h>\nchar *func_00442de8(char *d, const char *s, unsigned n) { p4_record({ID}, (unsigned)d, (unsigned)s, n, 0); return strncpy(d, s, n); }"},
                 "impls": {0x442DE8: _py_strncpy}},
    "00396c00": {"reader": {0x3DF360: "word"}},
    "0039af40": {"deref": {0x3DF240: 1, 0x3E2AB0: 9}},
    "003b71b0": {"deref": {0x3DF240: 1, 0x3E2AB0: 9}},
    "003b7290": {"reader": {0x3DF360: "word"}},
    "003bb210": {"reader": {0x3E2910: "rec20"}},
    "0039b080": {"reader": {0x3DF360: "word", 0x3E2910: "rec20"}},
    "00396a40": {"deref": {0x3DF240: 1}},
    "00394c70": {
        "deref": {0x394870: 2, 0x393900: 1},
        "c_impls": {
            0x3E0960: "void func_003e0960(float *d, float *s) { for (int i = 0; i < 16; i++) d[i] = s[i]; }",
            0x3E42E0: "void func_003e42e0(float *d, float *s, int n, float *m) { d[0] = s[0] * 2.0f; d[1] = s[1] * 2.0f; d[2] = s[2] * 2.0f; d[3] = s[3]; }",
        },
        "impls": {0x3E0960: _py_copy64, 0x3E42E0: _py_sphere},
    },
}

STEMS = {"003de2d0@3de2f0": "../src/platform/cdvd_device", "003e4030": "../src/platform/rw_normalize3", "003e41e0": "../src/platform/rw_normalize2", "003ebbf0": "../src/platform/rw_image_stream", "004c1a68": "../src/cri/adx_ac3_retail", "004c1ab8": "../src/cri/adx_ac3_retail", "004c1a98": "../src/cri/adx_ac3_retail"}     # the override file when it is not named after the asm unit
# ---- CRI ci_* port sources (src/cri/ci_*.c): generic generators over a handle object, small integers and random globals
CI_GLOBALS = [0x731C3C, 0x731C44, 0x731C48, 0x731C4C, 0x731C54, 0x731C70, 0x731C74, 0x7330C0, 0x7330CC, 0x7330D0, 0x733DC4, 0x734380, 0x733F64]


def ci_gen(argspec, rets=None, extra=None):
    rets = rets or {}

    def gen(rng):
        img = rn.Img(rng)
        for pg in (0x731000, 0x733000, 0x734000, 0x738000, 0x739000, 0x73A000, 0x923000):
            img.touch(pg)
        for a in CI_GLOBALS:
            img.w32(a, rng.choice([0, 1, 2, 3, 0xFFFFFFFF, rng.randrange(1 << 32)]))
        img.w32(0x73A748, rng.choice([0, 1, 1, 2]))
        img.w32(0x73A740, rng.choice([0xFFFFFFFF, 0, 1, 3]))
        img.w32(0x73A770, rng.choice([0, 1, 2]))
        img.w32(0x733DC0, rng.randrange(1 << 32))
        args = []
        for k in argspec:
            if k in "hH":
                if k == "H" and rng.random() < 0.12:
                    args.append(0)
                    continue
                o = img.alloc(0x60, fill_random=True)
                img.mem.wb(o + 2, rng.choice([0, 1, 2, 2, 3, 9, 0xFF]))
                img.mem.wb(o, rng.choice([0, 1, 1]))
                img.mem.wb(o + 1, rng.choice([0, 1, 0, 0xFF]))
                for off in (0x24, 0x8, 0xC, 0x10, 0x14, 0x1C, 0x30):
                    img.w32(o + off, rng.choice([0, 1, 2, 5, 100, 0xFFFFFFFF, rng.randrange(1 << 12), rng.randrange(1 << 32)]))
                args.append(o)
            elif k == "i":
                args.append(rng.choice([0, 1, 2, 3, -1, 5, 0x20, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0x12B, rng.randrange(1 << 32)]) & 0xFFFFFFFF)
            elif k == "n":
                args.append(rng.choice([-5, -1, 0, 1, 2, 3, 7, 100]) & 0xFFFFFFFF)
            elif k == "p":
                args.append(img.alloc(0x100) if rng.random() > 0.1 else 0)
            elif k == "q":
                args.append(img.alloc(0x100))
            elif k == "m":
                args.append(rng.choice([0, 0, 1, 2, 3]))
            elif k == "s":
                sb = img.alloc(0x40, fill_random=False)
                txt = "".join(rng.choice("abcdefghz/\\;1.:") for _ in range(rng.randrange(0, 26)))
                for j, ch in enumerate(txt + "\0"):
                    img.mem.wb(sb + j, ord(ch))
                args.append(sb)
            elif k == "v":
                args.append(rng.choice([-0x80, -0x7F, -0x7E, -1, 0, 1, 0x7E, 0x7F, 0x80, 100, -100]) & 0xFFFFFFFF)
            elif k == "z":
                args.append(rng.choice([-1, 0, 1, 0x2ED, 0x2EE, 0x2EF, 0x1000, 0x2EE * 0x60, 0x2EE * 0x61, 100000]) & 0xFFFFFFFF)
            elif k == "N":
                args.append(rng.choice([-4, -1, 1, 2, 3, 5, 16, 100]) & 0xFFFFFFFF)
            elif k == "k":
                args.append(rng.choice([-1, 0, 1, 2, 3, 4, 5, 15, 16, 17, 22]) & 0xFFFFFFFF)
            elif k == "r":
                o = img.alloc(0x260, fill_random=True)
                img.w32(o + 0x20, rng.choice([0, 3, 14, 15, 16, 17, 100, 255]))
                img.w32(o + 0x24, rng.choice([0, 1, 5, 16, 20]))
                for e in range(16):
                    img.w32(o + 0x38 + 0x20 * e, rng.randrange(0, 8))
                args.append(o)
        for a, cands in rets.items():
            img.callee_ret[a] = rng.choice(cands)
        if extra:
            extra(rng, img, args)
        cs = rn.Case(args, img, ret=True)
        if hasattr(img, "_cargs"):
            cs.cargs = img._cargs
        return cs
    return gen


C_6EB0 = ("int func_004e6eb0(unsigned n, unsigned *rec) { unsigned i, r = p4_record({ID}, n, 0, 0, 0); rec[0] = r & 0xFFu; rec[1] = (r >> 8) & 0xFFu; "
          "for (i = 2; i < 9; i++) rec[i] = i * 0x1111u + r; return (int)((r >> 16) & 3u); }")


def _py_6eb0(m):
    cid, n, ok = _ok(m, 0x4E6EB0)
    rec = m.g32(5)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    m.mem.wn(rec, 4, ok & 0xFF)
    m.mem.wn(rec + 4, 4, (ok >> 8) & 0xFF)
    for i in range(2, 9):
        m.mem.wn(rec + 4 * i, 4, (i * 0x1111 + ok) & 0xFFFFFFFF)
    return (ok >> 16) & 3


C_48B0S = ("int func_004e48b0(int fd, int cmd, int *p) { unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, fd, cmd, 0, 0); *p = (n < ((r >> 8) & 0xFFFFu)) ? 1 : 0; return (r & 0x80u) ? -1 : 0; }")


def _py_48b0s(m):
    cid, n, ok = _ok(m, 0x4E48B0)
    m.ilog.extend([cid, m.g32(4), m.g32(5), 0, 0])
    m.mem.wn(m.g32(6), 4, 1 if n < ((ok >> 8) & 0xFFFF) else 0)
    return 0xFFFFFFFF if (ok & 0x80) else 0


C_258B0 = ("int func_004258b0(unsigned a, unsigned b, unsigned c, unsigned d, unsigned e, unsigned f, unsigned g, unsigned h, unsigned i) { unsigned k, r = p4_record({ID}, b, e, g, 0); "
           "for (k = 0; k < 16; k++) ((unsigned *)f)[k] = b * 3u + k * 5u + e + r; return 0; }")


def _py_258b0(m):
    cid, n, ok = _ok(m, 0x4258B0)
    b, e, f, g = m.g32(5), m.g32(8), m.g32(9), m.g32(10)
    m.ilog.extend([cid, b, e, g, 0])
    for k in range(16):
        m.mem.wn(f + 4 * k, 4, (b * 3 + k * 5 + e + ok) & 0xFFFFFFFF)
    return 0


C_63B0S = ("void func_004e63b0(unsigned h) { unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, h, 0, 0, 0); if (n >= (r & 0xFFu)) *(unsigned char *)(h + 2) = 0; }")


def _py_63b0s(m):
    cid, n, ok = _ok(m, 0x4E63B0)
    h = m.g32(4)
    m.ilog.extend([cid, h, 0, 0, 0])
    if n >= (ok & 0xFF):
        m.mem.wb(h + 2, 0)
    return 0


C_SCEREAD_CI = ("int sceRead(int fd, void *p, int n) { return p4_record({ID}, fd, 0, n, 0); } int func_00427338(int a, void *b, int c) { return sceRead(a, b, c); }")


def _py_sceread_ci(m):
    cid, n, ok = _ok(m, 0x427338)
    m.ilog.extend([cid, m.g32(4), 0, m.g32(6), 0])
    return ok


def _ci_tbl(rng, img, args):
    for i in range(40):
        img.mem.wb(0x733140 + 0x48 * i, rng.choice([0, 1, 1]))


# addr: (argspec, rets, callees, nargs, impls-key, entry has_ret)
CI_DV = "../src/cri/ci_dv"
_SIGN = [0xFFFFFFFF, 0, 0, 1, 5, 0x20]
_CI_DV_SPECS = {
    "004e3a10": ("h", {0x4E48B0: [0x80, 0x100, 0x200, 0x300, 0x2100, 0x3000]}, [0x4E4618, 0x4E48B0, 0x4E39B8, 0x4244C8], {0x4E4618: 1, 0x4E48B0: (True, True, False, False), 0x4E39B8: 0, 0x4244C8: 1}, False),
    "004e3c78": ("h", {0x42F4B0: [0xFFFFFFFF, 0, 0x20, 5, 0x21, 1]}, [0x4F0668, 0x4E4618, 0x42F4B0, 0x4F06E0, 0x4244C8], {0x4F0668: 0, 0x4E4618: 1, 0x42F4B0: 0, 0x4F06E0: 0, 0x4244C8: 2}, True),
    "004e4698": ("i", {0x4244C8: [0, 7, 0xFFFFFFFF]}, [0x4244C8], {0x4244C8: 1}, True),
    "004e4770": ("i", {0x4F1278: [0, 1, 1], 0x42F3E0: [0, 0, 7], 0x42F4B0: [0, 3, 0xFFFFFFFF]}, [0x4F1278, 0x4E4618, 0x42F3E0, 0x42F4B0], {0x4F1278: 1, 0x4E4618: 1, 0x42F3E0: 1, 0x42F4B0: 0}, True),
    "004e47f8": ("iii", {0x429358: _SIGN, 0x4270F8: _SIGN}, [0x4E4700, 0x429358, 0x4270F8, 0x4244C8, 0x4E4738], {0x4E4700: 0, 0x429358: 3, 0x4270F8: 3, 0x4244C8: 2, 0x4E4738: 0}, True),
    "004e48b0": ("iqi", {0x427868: _SIGN}, [0x4E4700, 0x427868, 0x4244C8, 0x4E4738], {0x4E4700: 0, 0x427868: 3, 0x4244C8: 2, 0x4E4738: 0}, True),
    "004e49e8": ("iqi", {0x427338: _SIGN}, [0x4E4700, 0x427338, 0x4244C8, 0x4E4738], {0x4E4700: 0, 0x427338: 3, 0x4244C8: 2, 0x4E4738: 0}, True),
    "004e58a0": ("iqnqn", {0x4258B0: [0, 1, 0x77]}, [0x4F0608, 0x4258B0, 0x4F0620], {0x4F0608: 0, 0x4258B0: 3, 0x4F0620: 0}, True),
    "004e5ce0": ("p", {0x4E6EB0: [0, 0x100, 0x1, 0x10100, 0x201], 0x4E5A68: [0, 1]}, [0x4E6EB0, 0x4E6640, 0x4E4618, 0x42E7C8, 0x4E5A68], {0x4E6EB0: 1, 0x4E6640: 2, 0x4E4618: 1, 0x42E7C8: 1, 0x4E5A68: (False, True, False, False)}, True),
    "004e5d68": ("p", {0x4E6EB0: [0, 0x100, 0x1, 0x10100, 0x201], 0x4E5A68: [0, 1]}, [0x4E6EB0, 0x4E6640, 0x4E4618, 0x42E7C8, 0x4E5A68, 0x4CD3F0, 0x4E5A38], {0x4E6EB0: 1, 0x4E6640: 2, 0x4E4618: 1, 0x42E7C8: 1, 0x4E5A68: (False, True, False, False), 0x4CD3F0: 2, 0x4E5A38: 2}, True),
    "004e5ff8": ("H", {}, [0x4E62C8, 0x4E5E60], {0x4E62C8: 1, 0x4E5E60: 1}, False),
    "004e6050": ("Hnm", {}, [0x4E5A38], {0x4E5A38: 2}, True),
    "004e60e8": ("H", {}, [0x4E5A38], {0x4E5A38: 2}, True),
    "004e6120": ("Hnp", {0x4E36E8: [0, 0x1234, 0x1234], 0x4E3810: [0, 1, 1], 0x4E3810: [0, 1, 1]}, [0x4E5A38, 0x4E36E8, 0x422218, 0x4E3810, 0x4E3770, 0x42E7C8, 0x4E42A0, 0x4E4190, 0x4E3AE0], {0x4E5A38: 2, 0x4E36E8: 0, 0x422218: 2, 0x4E3810: 4, 0x4E3770: 1, 0x42E7C8: 1, 0x4E42A0: 0, 0x4E4190: 0, 0x4E3AE0: 1}, True),
    "004e62c8": ("H", {0x4E63B0: [0, 1, 3]}, [0x4E5A38, 0x4E3C10, 0x4E4190, 0x4E5BD8, 0x4E63B0], {0x4E5A38: 2, 0x4E3C10: 1, 0x4E4190: 0, 0x4E5BD8: 0, 0x4E63B0: 1}, False),
    "004e63b0": ("H", {}, [0x4E5A38], {0x4E5A38: 2}, True),
    "004e63f0": ("H", {}, [0x4E5A38], {0x4E5A38: 2}, True),
    "004e6480": ("", {}, [], {}, True),
    "004e6578": ("Hi", {0x4E6528: [0, 1, 7], 0x4E6538: [0, 1, 9], 0x4E6558: [0, 3, 4], 0x4E6560: [0, 1, 5]}, [0x4E6528, 0x4E6538, 0x4E6558, 0x4E6560], {0x4E6528: 0, 0x4E6538: 0, 0x4E6558: 0, 0x4E6560: 0}, True),
    "004e7258": ("p", {0x4E6EB0: [0, 0x100, 0x1, 0x10100, 0x201]}, [0x4E6EB0], {0x4E6EB0: 1}, True),
}
_CI_IMPLS = {0x4E6EB0: (C_6EB0, _py_6eb0), 0x4258B0: (C_258B0, _py_258b0), 0x427338: (C_SCEREAD_CI, _py_sceread_ci)}
_CI_SPEC_IMPLS = {"004e3a10": {0x4E48B0: (C_48B0S, _py_48b0s)}, "004e62c8": {0x4E63B0: (C_63B0S, _py_63b0s)}}


_CI_NAMES = {"004e5d68": "dvCiGetFileSize", "004e5e80": "dvCiOpen", "004e5ff8": "dvCiClose", "004e6120": "dvCiReqRd"}


def _ci_register(specs, stem):
    for addr, (argspec, rets, callees, nargs, has_ret) in specs.items():
        ex = None
        if addr == "004e5bd8":
            ex = _ci_tbl
        if addr == "004e5e80":
            pass
        SPECS[addr] = (ci_gen(argspec, rets, ex), has_ret, (True, True), callees, _CI_NAMES.get(addr, "func_" + addr), nargs)
        STEMS[addr] = stem
        ci = {}
        cim = {}
        for a in callees:
            if a in _CI_IMPLS:
                ci[a], cim[a] = _CI_IMPLS[a]
        for a, (cc, pp) in _CI_SPEC_IMPLS.get(addr, {}).items():
            ci[a], cim[a] = cc, pp
        if ci:
            EXTRA[addr] = {"c_impls": ci, "impls": cim}



def _open_extra(rng, img, args):
    h = img.alloc(0x60, fill_random=True)
    img.callee_ret[0x4E5E28] = h if rng.random() > 0.1 else 0


_CI_DV_SPECS["004e5e80"] = ("pqm", {0x4E6EB0: [0, 0x100, 0x1, 0x10100, 0x201, 0x10001, 0x10000], 0x4E5A68: [0, 1]},
                            [0x4E5E28, 0x4E6EB0, 0x4E6640, 0x4CD3F0, 0x4E4618, 0x42E7C8, 0x4E5A68, 0x4E5A38, 0x4E5E60],
                            {0x4E5E28: 0, 0x4E6EB0: 1, 0x4E6640: 2, 0x4CD3F0: 2, 0x4E4618: 1, 0x42E7C8: 1, 0x4E5A68: (False, True, False, False), 0x4E5A38: 2, 0x4E5E60: 1}, True)
_ci_register({k: v for k, v in _CI_DV_SPECS.items() if k != "004e5e80"}, CI_DV)
_ci_register({"004e5e80": _CI_DV_SPECS["004e5e80"]}, CI_DV)
SPECS["004e5e80"] = (ci_gen("pqm", _CI_DV_SPECS["004e5e80"][1], _open_extra),) + SPECS["004e5e80"][1:]

# ---- CRI ci_ht / ci_mf / ci_pl
C_8B98 = ("int func_004e8b98(unsigned n, unsigned *rec) { unsigned r = p4_record({ID}, n, 0, 0, 0); rec[0] = r & 0xFFu; rec[1] = (r >> 8) & 0xFFu; return (int)((r >> 16) & 3u); }")


def _py_8b98(m):
    cid, n, ok = _ok(m, 0x4E8B98)
    rec = m.g32(5)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    m.mem.wn(rec, 4, ok & 0xFF)
    m.mem.wn(rec + 4, 4, (ok >> 8) & 0xFF)
    return (ok >> 16) & 3


C_9EB0 = ("unsigned char *func_004e9eb0(unsigned name, unsigned *size) { unsigned r = p4_record({ID}, name, 0, 0, 0); *size = 0x700u + ((r >> 4) & 0x3FFu); return (unsigned char *)r; }")


def _py_9eb0(m):
    cid, n, ok = _ok(m, 0x4E9EB0)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    m.mem.wn(m.g32(5), 4, 0x700 + ((ok >> 4) & 0x3FF))
    return ok


C_MEMSETR = ("void *func_0043f9c8(void *d, int c, unsigned n) { p4_record({ID}, (unsigned)d, c, n, 0); memset(d, c, n); return d; }")


def _py_memsetr(m):
    cid, n, ok = _ok(m, 0x43F9C8)
    d, cc, nn = m.g32(4), m.g32(5), m.g32(6)
    m.ilog.extend([cid, d, cc, nn, 0])
    if nn and nn < 1 << 20:
        m.mem.write(d, bytes([cc & 0xFF]) * nn)
    return d


C_7FC8S = ("int func_004e7fc8(unsigned h) { unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, h, 0, 0, 0); if (n >= (r & 0xFFu)) *(unsigned char *)(h + 2) = 0; return 0; }")


def _py_7fc8s(m):
    cid, n, ok = _ok(m, 0x4E7FC8)
    h = m.g32(4)
    m.ilog.extend([cid, h, 0, 0, 0])
    if n >= (ok & 0xFF):
        m.mem.wb(h + 2, 0)
    return 0


def _mf_extra(rng, img, args):
    h = args[0]
    if h:
        img.mem.wb(h + 1, rng.choice([0, 1, 2, 2]))
        img.w32(h + 4, rng.choice([1, 2, 16, 32]))
        ln = rng.randrange(0, 40)
        img.w32(h + 0xC, ln)
        img.w32(h + 0x10, rng.randrange(0, ln + 1))
        img.w32(h + 8, rng.randrange(0, 1400))
    if len(args) > 2 and args[2]:
        args[2] = img.alloc(0x2000, fill_random=True)
    img.callee_ret[0x4E9EB0] = img.alloc(0x2000, fill_random=True)
    img.callee_ret[0x4EA040] = img.alloc(0x60, fill_random=True) if rng.random() > 0.1 else 0


def _mf_tbl(rng, img, args):
    only = rng.random() < 0.3
    for i in range(40):
        img.mem.wb(0x738D68 + 0x38 * i, (0 if i == 39 else 1) if only else rng.choice([0, 1, 1, 0xFF]))


def _ht_tbl(rng, img, args):
    for i in range(13):
        img.mem.wb(0x733FD0 + 0x30 * i, rng.choice([0, 1, 1]))
        img.mem.wb(0x733FD0 + 0x30 * i + 2, rng.choice([0, 1, 2, 2, 3]))


def _ht_open_extra(rng, img, args):
    img.callee_ret[0x4E7A00] = img.alloc(0x60, fill_random=True) if rng.random() > 0.1 else 0


def _ht_rd_extra(rng, img, args):
    h = args[0]
    if h:
        img.mem.wb(h + 2, rng.choice([0, 1, 2, 2, 3]))
        ln = rng.randrange(0, 40)
        img.w32(h + 0x10, ln)
        img.w32(h + 0x1C, rng.randrange(0, ln + 1))
        img.w32(h + 8, rng.randrange(0, 1 << 16))
        img.w32(h + 0xC, 0)
    img.callee_ret[0x4E36E8] = rng.choice([0, 0x1234, 0x1234])


def _ind_extra(rng, img, args):
    """a0 -> +8 -> p -> +8 -> o -> +0 -> vtable with the method at +0x24 (an indirect callee, retail address 0x4FFF00)"""
    a0 = args[0]
    p = img.alloc(0x20, fill_random=False)
    o = img.alloc(0x20, fill_random=False)
    vt = img.alloc(0x40, fill_random=False)
    img.w32(a0 + 8, p)
    img.w32(p + 8, o)
    img.w32(o, vt)
    img.fnslot(vt + 0x24, 0x4FFF00)


_CI_HT_SPECS = {
    "004e7668": ("", {0x4F1278: [0, 1, 1]}, [0x4F1278, 0x4E75A0], {0x4F1278: 1, 0x4E75A0: 1}, False, _ht_tbl),
    "004e76f8": ("p", {0x4E8B98: [0, 0x100, 0x1, 0x10100, 0x201], 0x4E4928: [0xFFFFFFFF, 3, 7, 0]}, [0x4E8B98, 0x4E82A0, 0x4E4618, 0x4E4928, 0x4E4990], {0x4E8B98: 1, 0x4E82A0: 2, 0x4E4618: 1, 0x4E4928: 2, 0x4E4990: 1}, True, None),
    "004e77c0": ("p", {0x4E8B98: [0, 0x100, 0x1, 0x10100, 0x201], 0x4E4928: [0xFFFFFFFF, 0x10004, 0x13, 3, 7], 0x4E47F8: [0xFFFFFFFF, 0, 100, 5000], 0x4E4990: [0xFFFFFFFF, 0, 0]},
                 [0x4E7458, 0x4E8B98, 0x4E82A0, 0x4E4618, 0x4E4928, 0x4CD3F0, 0x4E47F8, 0x4E4990], {0x4E7458: 2, 0x4E8B98: 1, 0x4E82A0: 2, 0x4E4618: 1, 0x4E4928: 2, 0x4CD3F0: 1, 0x4E47F8: 3, 0x4E4990: 1}, True, None),
    "004e7a58": ("pqm", {0x4E8B98: [0, 0x100, 0x1, 0x10100, 0x201], 0x4E77C0: [0, 5, 0x1234, 0xFFFFFFFD], 0x4E4928: [0xFFFFFFFF, 3, 7, 0]},
                 [0x4E7458, 0x4E7A00, 0x4E8B98, 0x4E82A0, 0x4CD378, 0x4E77C0, 0x4E4618, 0x4E4928, 0x4E7A38], {0x4E7458: 2, 0x4E7A00: 0, 0x4E8B98: 1, 0x4E82A0: 2, 0x4CD378: 2, 0x4E77C0: 1, 0x4E4618: 1, 0x4E4928: 2, 0x4E7A38: 1}, True, _ht_open_extra),
    "004e7bd0": ("H", {0x4E4990: [0xFFFFFFFF, 0, 0]}, [0x4E7ED0, 0x4E4618, 0x4E4990, 0x4CD378, 0x4E7A38], {0x4E7ED0: 1, 0x4E4618: 1, 0x4E4990: 1, 0x4CD378: 1, 0x4E7A38: 1}, False, None),
    "004e7c70": ("Hnm", {}, [0x4E7458], {0x4E7458: 2}, True, None),
    "004e7d08": ("H", {}, [0x4E7458], {0x4E7458: 2}, True, None),
    "004e7d40": ("Hnp", {0x4E7488: [0, 0, 1, 2], 0x4E3900: [0, 1, 1]}, [0x4E7458, 0x4E7488, 0x4E36E8, 0x4E7540, 0x4E3900, 0x4E42A0, 0x4E4190, 0x4E3770], {0x4E7458: 2, 0x4E7488: 0, 0x4E36E8: 0, 0x4E7540: 1, 0x4E3900: 4, 0x4E42A0: 0, 0x4E4190: 0, 0x4E3770: 1}, True, _ht_rd_extra),
    "004e7ed0": ("H", {0x4E7FC8: [0, 1, 3]}, [0x4E7458, 0x4E3C10, 0x4E4190, 0x4E7668, 0x4E7FC8, 0x4E7428, 0x4CD3F0], {0x4E7458: 2, 0x4E3C10: 1, 0x4E4190: 0, 0x4E7668: 0, 0x4E7FC8: 1, 0x4E7428: 0, 0x4CD3F0: 1}, False, None),
    "004e7fc8": ("H", {}, [0x4E7458], {0x4E7458: 2}, True, None),
    "004e8008": ("H", {}, [0x4E7458], {0x4E7458: 2}, True, None),
    "004e8058": ("p", {0x4E77C0: [0, 7, 0xFFFFFFFF]}, [0x4E77C0], {0x4E77C0: 1}, True, None),
    "004e8110": ("Hi", {0x4E8070: [0, 1, 7], 0x4E8090: [0, 1, 9], 0x4E80C0: [0, 3, 4], 0x4E80E0: [0, 1, 5]}, [0x4E8070, 0x4E8090, 0x4E80C0, 0x4E80E0], {0x4E8070: 0, 0x4E8090: 0, 0x4E80C0: 0, 0x4E80E0: 0}, True, None),
}
_CI_MF_SPECS = {
    "004e9420": ("H", {}, [0x4E9878], {0x4E9878: 1}, True, None),
    "004e9480": ("rk", {}, [0x4E9878], {0x4E9878: 1}, True, None),
    "004e9510": ("rk", {}, [0x4E9878], {0x4E9878: 1}, True, None),
    "004e9640": ("rk", {}, [0x4E9878], {0x4E9878: 1}, True, None),
    "004ea128": ("pqm", {}, [0x4EA040, 0x4E9E68, 0x442830, 0x4EA0B8], {0x4EA040: 0, 0x4E9E68: 2, 0x442830: 2, 0x4EA0B8: 1}, True, _mf_extra),
    "004ea1c8": ("H", {}, [0x4EA490, 0x4EA098], {0x4EA490: 1, 0x4EA098: 1}, False, None),
    "004ea210": ("Hnm", {}, [0x4E9E68, 0x4E9E20, 0x4E9E38], {0x4E9E68: 2, 0x4E9E20: 0, 0x4E9E38: 0}, True, None),
    "004ea2d0": ("H", {}, [0x4E9E68], {0x4E9E68: 2}, True, None),
    "004ea308": ("Hnq", {}, [0x4E9E68, 0x4E9E20, 0x4E9E38, 0x4E9EB0, 0x43F9C8], {0x4E9E68: 2, 0x4E9E20: 0, 0x4E9E38: 0, 0x4E9EB0: 1, 0x43F9C8: 3}, True, _mf_extra),
    "004ea490": ("H", {}, [0x4E9E68, 0x4E9E20, 0x4E9E38], {0x4E9E68: 2, 0x4E9E20: 0, 0x4E9E38: 0}, False, None),
    "004ea4e0": ("H", {}, [0x4E9E68], {0x4E9E68: 2}, True, None),
    "004ea518": ("H", {}, [0x4E9E68], {0x4E9E68: 2}, True, None),
    "004ea550": ("HN", {}, [0x4E9E68], {0x4E9E68: 2}, False, None),
    "004ea5d0": ("H", {}, [0x4E9E68], {0x4E9E68: 2}, True, None),
    "004ea690": ("Hi", {0x4EA5D0: [0, 1, 7], 0x4EA020: [0, 1, 9]}, [0x4EA5D0, 0x4EA020], {0x4EA5D0: 1, 0x4EA020: 0}, True, None),
}
_CI_PL_SPECS = {
    "004ea838": ("p", {0x4EA728: [0, 1, 7]}, [0x4EA710, 0x4EA868, 0x4EA728], {0x4EA710: 0, 0x4EA868: 1, 0x4EA728: 0}, True, None),
    "004ea9c8": ("p", {0x4EA728: [0, 1, 7]}, [0x4EA710, 0x4EA9F8, 0x4EA728], {0x4EA710: 0, 0x4EA9F8: 1, 0x4EA728: 0}, True, None),
    "004eaa18": ("p", {0x4EA728: [0, 1, 7]}, [0x4EA710, 0x4EAA48, 0x4EA728], {0x4EA710: 0, 0x4EAA48: 1, 0x4EA728: 0}, True, None),
    "004eaa88": ("pipp", {0x4EA728: [0, 1, 7]}, [0x4EA710, 0x4EAAE8, 0x4EA728], {0x4EA710: 0, 0x4EAAE8: 4, 0x4EA728: 0}, True, None),
    "004eae58": ("ppp", {0x4EA728: [0, 1, 7]}, [0x4EA710, 0x4EAEA8, 0x4EA728], {0x4EA710: 0, 0x4EAEA8: 3, 0x4EA728: 0}, True, None),
    "004eaf58": ("pp", {0x4EA728: [0, 1, 7]}, [0x4EA710, 0x4EAF98, 0x4EA728], {0x4EA710: 0, 0x4EAF98: 2, 0x4EA728: 0}, True, None),
    "004ec068": ("", {}, [0x4E58A0], {0x4E58A0: 4}, False, None),
    "004ec140": ("qi", {}, [], {}, False, None),
    "004ecb40": ("q", {0x4FFF00: [0, 1, 0x1000, 0x4001, 0xFFFFFFF0, 0xFFFFFFF1, 0x7FFFFFFF]}, [0x4FFF00], {0x4FFF00: 2}, True, _ind_extra),
    "004ecba8": ("q", {0x4FFF00: [0, 1, 0x1000, 0x4001, 0xFFFFFFF0, 0xFFFFFFF1, 0x7FFFFFFF]}, [0x4FFF00], {0x4FFF00: 2}, True, _ind_extra),
    "004ed180": ("", {}, [], {}, True, None),
    "004ed240": ("", {}, [0x4CD3F0], {0x4CD3F0: 1}, True, None),
}
_CI_PL_NAMES = {"004eaa88": "PL2ENC_Encode"}
_CI_MF_NAMES = {"004ea128": "mfCiOpen", "004ea1c8": "mfCiClose", "004ea2d0": "dvCiTell", "004ea308": "mfCiReqRd", "004ea690": "mfCiOptFn1"}
_CI_HT_NAMES = {"004e77c0": "htCiGetFileSize", "004e7a58": "htCiOpen", "004e7d40": "htCiReqRd"}
_CI_IMPLS.update({0x4E8B98: (C_8B98, _py_8b98), 0x4E9EB0: (C_9EB0, _py_9eb0), 0x43F9C8: (C_MEMSETR, _py_memsetr)})
_CI_SPEC_IMPLS.update({"004e7ed0": {0x4E7FC8: (C_7FC8S, _py_7fc8s)}})


def _ci_register2(specs, stem, names):
    for addr, (argspec, rets, callees, nargs, has_ret, ex) in specs.items():
        _CI_NAMES[addr] = names.get(addr, "func_" + addr)
        _ci_register({addr: (argspec, rets, callees, nargs, has_ret)}, stem)
        if ex is not None:
            SPECS[addr] = (ci_gen(argspec, rets, ex),) + SPECS[addr][1:]


rn.ALT_NAMES[0x4E77C0] = "htCiGetFileSize"
_ci_register2(_CI_HT_SPECS, "../src/cri/ci_ht", _CI_HT_NAMES)
_ci_register2(_CI_MF_SPECS, "../src/cri/ci_mf", _CI_MF_NAMES)
_ci_register2(_CI_PL_SPECS, "../src/cri/ci_pl", _CI_PL_NAMES)
_CI_ALL = list(_CI_DV_SPECS) + list(_CI_HT_SPECS) + list(_CI_MF_SPECS) + list(_CI_PL_SPECS)


# ---- CRI closure of the ci_* files: the sceCd request layer, libadxe helpers, callbacks through words (indirect callees)
def _cb_extra(slot, objslot, fn_addr):
    def ex(rng, img, args):
        if rng.random() > 0.25:
            img.fnslot(slot, fn_addr)
        else:
            img.w32(slot, 0)
        img.w32(objslot, rng.randrange(1 << 32))
    return ex


def _compose(*fs):
    def ex(rng, img, args):
        for f in fs:
            f(rng, img, args)
    return ex


def _req_extra(rng, img, args):
    img.w32(0x731C00, rng.choice([0, 0x01000000, 0x01010000]))
    img.mem.wb(0x731C01, rng.choice([0, 1, 2]))
    img.mem.wb(0x731C02, rng.choice([0, 1, 2, 3, 9]))


def _req_arg_extra(rng, img, args):
    if rng.random() < 0.7:
        args[0] = 0x731C00
    img.mem.wb(0x731C02, rng.choice([0, 1, 2, 3, 9]))


def _pos_extra(rng, img, args):
    # the 64 bit arguments (position, byte count) are small non negative values: the C side gets their high words as zero
    args[2] = args[2] & 0x7FFFFFFF
    args[3] = args[3] & 0x7FFFFFFF
    img._cargs = [args[0], args[1], args[2], 0, args[3], 0, args[4]]


def _wide_extra(argidx):
    """arguments argidx are 64 bit values for the asm; the C side gets their low and (zero) high word"""
    def ex(rng, img, args):
        pass
    return ex


def _strs_extra(rng, img, args):
    for a, s in ((0x733DD0, "DIR\\SUB"), (0x75B250, "/"), (0x75B258, ";1"), (0x75B770, "/"), (0x75B320, "x")):
        img.touch(a & ~0xFFF)
        for j, ch in enumerate(s + "\0"):
            img.mem.wb(a + j, ord(ch))
    if len(args) > 0 and args[0] and rng.random() < 0.25:
        # make the name equal to the text at 0x75B320 now and then
        txt = bytes(img.mem.rb(args[0] + j) for j in range(0x40)).split(b"\0")[0]
        for j, ch in enumerate(txt + b"\0"):
            img.mem.wb(0x75B320 + j, ch)


def _base_extra(rng, img, args):
    img.touch(0x734000)
    img.touch(0x75B000)
    for a, opts in ((0x734490, ["c:", "", "root\\", "/x"]), (0x734388, ["base", "base/", "base\\", ""]), (0x75B770, ["/"])):
        s = rng.choice(opts)
        for j, ch in enumerate(s + "\0"):
            img.mem.wb(a + j, ord(ch))


def _ht_size_extra(rng, img, args):
    h = args[0]
    img.w32(h + 8, rng.choice([0, 5, 0x1000, 0x12345, 0xFFFFFFFF]))
    img.w32(h + 0xC, rng.choice([0, 0, 0, 1, 0x80000000]))
    img.w32(h + 0x28, rng.choice([0, 1, 5, 0x400, 0x200000, 0xFFFFFFF0, 0x7FFFFFFF]))


def _ht_tail_extra(rng, img, args):
    h = args[0]
    n = rng.randrange(0, 40)
    pos = rng.randrange(0, 40)
    img.w32(h + 0x24, n)
    img.w32(h + 0x1C, pos)
    sz = pos * 2048 + rng.randrange(0, n * 2048 + 2000)
    img.w32(h + 8, sz)
    img.w32(h + 0xC, 0)
    args[0] = h
    img.w32(h + 0x20, img.alloc(0x14000, fill_random=True))


def _ht_fin_extra(rng, img, args):
    h = args[0]
    img.w32(h + 0x2C, rng.choice([0, 0x1234, 0x1234]))
    img.w32(h + 0x24, rng.randrange(0, 50))
    img.w32(h + 0x1C, rng.randrange(0, 50))
    img.mem.wb(h + 2, rng.choice([0, 1, 2, 3]))


def _ht_rec_extra(rng, img, args):
    img.w32(0x734370, rng.choice([0, 0, 0x1234]))
    img.w32(0x734374, rng.randrange(1 << 32))


def _fmt_extra(rng, img, args):
    img.touch(0x721000)
    img.w32(0x7212D0, rng.choice([0, 0, 1, 2]))
    img.touch(0x7344000 >> 4 << 4) if False else None


def _mf_tail_extra(rng, img, args):
    img.touch(0x7344D0 & ~0xFFF)
    img.w32(0x7344C8, 0)
    if rng.random() > 0.3:
        img.fnslot(0x7344C8, 0x4FFF20)
    img.w32(0x7344CC, rng.randrange(1 << 32))


def _mfname_extra(rng, img, args):
    img.touch(0x739000)
    img.touch(0x75B000)
    if rng.random() < 0.75:
        s = "%08x.%08x" % (rng.randrange(1 << 32), rng.randrange(1 << 32))
    else:
        s = rng.choice(["0123.4567", "0123456789abcdef0123", "0123456x89abcdef0"])
    nb = img.alloc(0x40, fill_random=False)
    for j, ch in enumerate(s + "\0"):
        img.mem.wb(nb + j, ord(ch))
    args[0] = nb
    if rng.random() < 0.3:
        args[1] = 0
    img.w32(0x738CF4, 0)
    if rng.random() > 0.3:
        img.fnslot(0x738CF4, 0x4FFF21)


def _mf_size_extra(rng, img, args):
    h = args[0]
    img.w32(h + 8, rng.choice([0, 1, 2047, 2048, 5000, 0x12345678 // 4]))


def _pl_enc_extra(rng, img, args):
    img.touch(0x75C000)
    for i in range(16):
        img.w32(0x75C080 + 4 * i, rng.choice([0, 0x1000, 0x800, 0x7FF, 0xDEF, 0xFFFFF000, rng.randrange(1 << 14)]))
    o = args[0]
    img.w32(o + 8, img.alloc(0x400, fill_random=True))
    img.w32(o + 0xC, img.alloc(0x400, fill_random=True))
    n = rng.choice([1, 2, 5, 0x60])
    img.w32(o + 0x18, n)
    img.w32(o + 0x14, rng.randrange(0, n))
    for off in (0x1C, 0x20, 0x24, 0x28):
        img.w32(o + off, rng.randrange(0, 16))
    for k in (1, 2):
        args[k] = args[k]


def _pl_reset_extra(rng, img, args):
    o = args[0]
    img.w32(o + 8, img.alloc(0x200, fill_random=True))
    img.w32(o + 0xC, img.alloc(0x200, fill_random=True))


C_VSPRINTF_COPY = ("int func_00446ed8(char *b, const char *f, void *ap) { p4_record({ID}, (unsigned)f, 0, 0, 0); strcpy(b, f); return 0; }")
C_PRINTF_W = ("int func_004244c8(const char *s, ...) { unsigned w = 0, i; for (i = 0; i < 4 && s[i]; i++) w |= (unsigned)(unsigned char)s[i] << (8 * i); return p4_record({ID}, w, 0, 0, 0); }")
C_CBTEXT = ("void func_004fff20(unsigned obj, const char *t) { unsigned w = 0, i; for (i = 0; i < 3 && t[i]; i++) w |= (unsigned)(unsigned char)t[i] << (8 * i); p4_record({ID}, obj, w, 0, 0); }")
C_9E98 = ("int func_004e9e98(const char *s, char **e, int base) { unsigned r = p4_record({ID}, (unsigned)s, 0, base, 0); *e = (char *)s + 8; return (int)((unsigned)s[0] * 31u + (unsigned)s[1] + r); }")
C_SPRINTF_L = ("int func_00442088(char *b, const char *f, unsigned n) { p4_record({ID}, (unsigned)f, n, 0, 0); b[0] = 'E'; b[1] = 0; return 1; }")
C_39B8S = ("void func_004e39b8(void) { unsigned n = p4_ncalls[{ID}]; unsigned r = p4_record({ID}, 0, 0, 0, 0); if (n >= (r & 0xFFu)) *(unsigned *)0x731C3C = 0; }")
C_6DF0 = ("void func_004e6df0(const char *n, unsigned *a, unsigned *b) { unsigned r = p4_record({ID}, (unsigned)n, 0, 0, 0); *a = r * 3u + 1u; *b = (r >> 4) & 0xFFu; }")
C_3900C = None


def _py_vsprintf_copy(m):
    cid, n, ok = _ok(m, 0x446ED8)
    m.ilog.extend([cid, m.g32(5), 0, 0, 0])
    dst, src = m.g32(4), m.g32(5)
    j = 0
    while True:
        b = m.mem.rb(src + j)
        m.mem.wb(dst + j, b)
        if b == 0:
            break
        j += 1
    return 0


def _py_printf_w(m):
    cid, n, ok = _ok(m, 0x4244C8)
    s = m.g32(4)
    w = 0
    for i in range(4):
        b = m.mem.rb(s + i)
        if not b:
            break
        w |= b << (8 * i)
    m.ilog.extend([cid, w, 0, 0, 0])
    return ok


def _py_cbtext(m):
    cid, n, ok = _ok(m, 0x4FFF20)
    t = m.g32(5)
    w = 0
    for i in range(3):
        b = m.mem.rb(t + i)
        if not b:
            break
        w |= b << (8 * i)
    m.ilog.extend([cid, m.g32(4), w, 0, 0])
    return 0


def _py_9e98(m):
    cid, n, ok = _ok(m, 0x4E9E98)
    s = m.g32(4)
    m.ilog.extend([cid, s, 0, m.g32(6), 0])
    m.mem.wn(m.g32(5), 4, s + 8)
    return (m.mem.rb(s) * 31 + m.mem.rb(s + 1) + ok) & 0xFFFFFFFF


def _py_sprintf_l(m):
    cid, n, ok = _ok(m, 0x442088)
    m.ilog.extend([cid, m.g32(5), m.g32(6), 0, 0])
    m.mem.wb(m.g32(4), 0x45)
    m.mem.wb(m.g32(4) + 1, 0)
    return 1


def _py_39b8s(m):
    cid, n, ok = _ok(m, 0x4E39B8)
    m.ilog.extend([cid, 0, 0, 0, 0])
    if n >= (ok & 0xFF):
        m.mem.wn(0x731C3C, 4, 0)
    return 0


def _py_6df0(m):
    cid, n, ok = _ok(m, 0x4E6DF0)
    m.ilog.extend([cid, m.g32(4), 0, 0, 0])
    m.mem.wn(m.g32(5), 4, (ok * 3 + 1) & 0xFFFFFFFF)
    m.mem.wn(m.g32(6), 4, (ok >> 4) & 0xFF)
    return 0


def _py_strcpy(m):
    d, s = m.g32(4), m.g32(5)
    j = 0
    while True:
        b = m.mem.rb(s + j)
        m.mem.wb(d + j, b)
        if b == 0:
            break
        j += 1
    return d


def _py_strcat(m):
    d, s = m.g32(4), m.g32(5)
    k = 0
    while m.mem.rb(d + k):
        k += 1
    j = 0
    while True:
        b = m.mem.rb(s + j)
        m.mem.wb(d + k + j, b)
        if b == 0:
            break
        j += 1
    return d


def _py_strlen(m):
    s = m.g32(4)
    k = 0
    while m.mem.rb(s + k):
        k += 1
    return k


def _py_strcmp(m):
    a, b = m.g32(4), m.g32(5)
    k = 0
    while True:
        x, y = m.mem.rb(a + k), m.mem.rb(b + k)
        if x != y:
            return (x - y) & 0xFFFFFFFF
        if x == 0:
            return 0
        k += 1


# libc functions run for real on the C side (no stub text) and in the interpreter without a log line
_LIBC = {0x442830: _py_strcpy, 0x442428: _py_strcat, 0x442948: _py_strlen, 0x4426E8: _py_strcmp}
_CI_IMPLS.update({0x446ED8: (C_VSPRINTF_COPY, _py_vsprintf_copy), 0x4FFF20: (C_CBTEXT, _py_cbtext), 0x4E9E98: (C_9E98, _py_9e98),
                  0x442088: (C_SPRINTF_L, _py_sprintf_l), 0x4E6DF0: (C_6DF0, _py_6df0)})
for _a, _f in _LIBC.items():
    _CI_IMPLS[_a] = ("", _f)
C_3900_ST = ("unsigned func_004e3900(unsigned r, int fd, long long pos, long long bytes, unsigned buf) { return p4_record({ID}, r, fd, (unsigned)pos, (unsigned)bytes); }")


def _py_3900_st(m):
    cid, n, ok = _ok(m, 0x4E3900)
    m.ilog.extend([cid, m.g32(4), m.g32(5), m.g32(6), m.g32(7)])
    return ok


_CI_SPEC_IMPLS.update({"004cd378": {0x4244C8: (C_PRINTF_W, _py_printf_w)}, "004e3ae0": {0x4E39B8: (C_39B8S, _py_39b8s)}, "004e7d40": {0x4E3900: (C_3900_ST, _py_3900_st)}})


_ci_tbl2 = _ci_tbl


def _req4_extra(rng, img, args):
    if args[0]:
        img.w32(args[0] + 4, rng.choice([0, 1, 1, 2]))


def _words_extra(rng, img, args):
    img.w32(0x733DB0, rng.choice([0, 0x1234]))
    img.w32(0x733DB4, rng.randrange(1 << 32))


def _mf_ring_none(rng, img, args):
    pass


_CI_DV2 = {
    "004e36e8": ("", {}, [0x4F0638, 0x4E36B8, 0x4F06B0], {0x4F0638: 0, 0x4E36B8: 0, 0x4F06B0: 0}, True, _req_extra),
    "004e3770": ("q", {}, [0x4F0638, 0x4244C8, 0x4E36B8, 0x4F06B0], {0x4F0638: 0, 0x4244C8: 1, 0x4E36B8: 0, 0x4F06B0: 0}, False, _compose(_req_extra, _req_arg_extra)),
    "004e3810": ("hiiiq", {0x42E7C8: [0, 1, 1]}, [0x4E4618, 0x42E7C8, 0x4F0638, 0x4F06B0], {0x4E4618: 1, 0x42E7C8: 1, 0x4F0638: 0, 0x4F06B0: 0}, True, None),
    "004e3900": ("hiiiq", {}, [0x4F0638, 0x4F06B0], {0x4F0638: 0, 0x4F06B0: 0}, True, _pos_extra),
    "004e3ae0": ("h", {0x4E39B8: [0, 1, 3, 5]}, [0x4F0608, 0x4F0620, 0x4E39B8, 0x4E39E8, 0x4E3A10], {0x4F0608: 0, 0x4F0620: 0, 0x4E39B8: 0, 0x4E39E8: 1, 0x4E3A10: 1}, False, _req_extra),
    "004e3c10": ("H", {}, [0x4F0638, 0x4244C8, 0x4F06B0], {0x4F0638: 0, 0x4244C8: 1, 0x4F06B0: 0}, False, _req4_extra),
    "004e4700": ("", {}, [0x4F0668], {0x4F0668: 0}, False, None),
    "004e4738": ("", {}, [0x4F06E0], {0x4F06E0: 0}, False, None),
    "004e4928": ("pi", {0x426CF0: _SIGN}, [0x4E4700, 0x426CF0, 0x4244C8, 0x4E4738], {0x4E4700: 0, 0x426CF0: 2, 0x4244C8: 2, 0x4E4738: 0}, True, None),
    "004e4990": ("i", {0x426F80: _SIGN}, [0x4E4700, 0x426F80, 0x4244C8, 0x4E4738], {0x4E4700: 0, 0x426F80: 1, 0x4244C8: 2, 0x4E4738: 0}, True, None),
    "004e5a38": ("pp", {0x4FFF10: [0]}, [0x4FFF10], {0x4FFF10: 3}, False, _cb_extra(0x7330C4, 0x7330C8, 0x4FFF10)),
    "004e5a68": ("pp", {0x42E2B8: [0, 1, 5]}, [0x4E4618, 0x42E2B8], {0x4E4618: 1, 0x42E2B8: 3}, True, None),
    "004e5e28": ("", {}, [], {}, True, _ci_tbl2),
    "004e5e60": ("h", {}, [0x43F9C8], {0x43F9C8: 3}, False, None),
    "004e6528": ("h", {}, [], {}, True, None),
    "004e6538": ("h", {}, [], {}, True, None),
    "004e6560": ("h", {0x4E6490: [0, 5, 0xFFFFFFFF]}, [0x4E6490], {0x4E6490: 1}, True, None),
    "004e6640": ("sq", {}, [0x442830, 0x442428, 0x442948, 0x4426E8, 0x4E5C68], {0x4E5C68: 1}, False, _strs_extra),
    "004e6eb0": ("sq", {0x4E6DB0: [0, 1, 1, 2], 0x4E6DF0: [0, 0x10, 0x35], 0x4E6CC0: [0]}, [0x4426E8, 0x4CD3F0, 0x4E6DB0, 0x4E6DF0, 0x4E6CC0], {0x4CD3F0: 1, 0x4E6DB0: 1, 0x4E6DF0: 1, 0x4E6CC0: 4}, True, _compose(_strs_extra, _words_extra)),
}
_CI_HT2 = {
    "004e7458": ("pp", {0x4FFF11: [0]}, [0x4FFF11], {0x4FFF11: 3}, False, _cb_extra(0x733F5C, 0x733F60, 0x4FFF11)),
    "004e7488": ("", {}, [], {}, True, _ht_tbl),
    "004e7540": ("h", {}, [0x43F9C8], {0x43F9C8: 3}, False, _ht_tail_extra),
    "004e75a0": ("h", {0x4E39B0: [0, 3, 9, 1, 5, 12, 0, 3]}, [0x4F0638, 0x4E39B0, 0x4E3770, 0x4F06B0], {0x4F0638: 0, 0x4E39B0: 1, 0x4E3770: 1, 0x4F06B0: 0}, False, _ht_fin_extra),
    "004e7a00": ("", {}, [], {}, True, _ht_tbl),
    "004e7a38": ("h", {}, [0x43F9C8], {0x43F9C8: 3}, False, None),
    "004e8070": ("h", {}, [], {}, True, _ht_size_extra),
    "004e8090": ("h", {}, [], {}, True, _ht_size_extra),
    "004e80e0": ("p", {0x4E8058: [0, 7, 0xFFFFFFFF, 0x7FFFFFFF]}, [0x4E8058], {0x4E8058: 1}, True, None),
    "004e82a0": ("sq", {0x4E81D8: [0, 1, 2], 0x4E8248: [0, 1, 1, 2]}, [0x4E81D8, 0x442830, 0x4E8248, 0x442428, 0x442948], {0x4E81D8: 1, 0x4E8248: 1}, False, _base_extra),
    "004e8b98": ("sq", {}, [0x4E8AA8], {0x4E8AA8: 4}, False, _ht_rec_extra),
    "004cd378": ("s", {}, [0x446ED8, 0x4244C8], {0x446ED8: 1, 0x4244C8: 1}, False, _fmt_extra),
}
_CI_MF2 = {
    "004e9878": ("s", {}, [0x446ED8, 0x4FFF20], {0x446ED8: 1, 0x4FFF20: 2}, False, _mf_tail_extra),
    "004e9e20": ("", {}, [0x4F0608], {0x4F0608: 0}, False, None),
    "004e9e38": ("", {}, [0x4F0620], {0x4F0620: 0}, False, None),
    "004e9e68": ("pp", {0x4FFF21: [0]}, [0x4FFF21], {0x4FFF21: 3}, False, _cb_extra(0x738CF4, 0x738CF8, 0x4FFF21)),
    "004e9eb0": ("sq", {}, [0x442948, 0x442088, 0x4E9E68, 0x4E9E98], {0x442088: 2, 0x4E9E68: 2, 0x4E9E98: (True, False, True, False)}, True, _mfname_extra),
    "004ea040": ("", {}, [], {}, True, _mf_tbl),
    "004ea098": ("h", {}, [0x43F9C8], {0x43F9C8: 3}, False, None),
    "004ea0b8": ("h", {0x4EA020: [0, 1, 2047, 2048, 5000, 0x12345678 // 4, 0xFFFFF000]}, [0x4EA020], {0x4EA020: 1}, True, None),
}
_CI_PL2 = {
    "004ea868": ("", {}, [], {}, True, None),
    "004ea9f8": ("q", {}, [0x43F9C8], {0x43F9C8: 3}, False, None),
    "004eaa48": ("h", {}, [], {}, False, _pl_reset_extra),
    "004eaae8": ("hvqq", {}, [], {}, False, _pl_enc_extra),
    "004eaea8": ("Hvv", {}, [0x4EA798], {0x4EA798: 1}, False, None),
    "004eaf98": ("Hz", {}, [0x4EA798], {0x4EA798: 1}, False, None),
}
_CI_IMPLS.setdefault(0x43F9C8, (C_MEMSETR, _py_memsetr))
# updated callee argument counts of the first batch: the helpers take the handle (asm leaves a0 in $4)
_CI_DV_SPECS["004e6578"] = _CI_DV_SPECS["004e6578"][:3] + ({0x4E6528: 1, 0x4E6538: 1, 0x4E6558: 1, 0x4E6560: 1},) + _CI_DV_SPECS["004e6578"][4:]
_CI_HT_SPECS["004e8110"] = _CI_HT_SPECS["004e8110"][:3] + ({0x4E8070: 1, 0x4E8090: 1, 0x4E80C0: 1, 0x4E80E0: 1},) + _CI_HT_SPECS["004e8110"][4:]
_CI_MF_SPECS["004ea690"] = _CI_MF_SPECS["004ea690"][:3] + ({0x4EA5D0: 1, 0x4EA020: 1},) + _CI_MF_SPECS["004ea690"][4:]
for _k in ("004ea838", "004ea9c8", "004eaa18"):
    _s = _CI_PL_SPECS[_k]
    _nm = dict(_s[3])
    for _a in list(_nm):
        if _a in (0x4EA868, 0x4EA9F8, 0x4EAA48):
            _nm[_a] = 0 if _a == 0x4EA868 else 1
    _CI_PL_SPECS[_k] = _s[:3] + (_nm,) + _s[4:]
_CI_PL_SPECS["004ea838"] = _CI_PL_SPECS["004ea838"][:2] + ([0x4EA710, 0x4EA868, 0x4EA728],) + _CI_PL_SPECS["004ea838"][3:]
_CI_PL_SPECS["004ea838"][3][0x4EA868] = 0
rn.ALT_NAMES[0x4E77C0] = "htCiGetFileSize"
_ci_register2(_CI_DV2, "../src/cri/ci_dv", {})
_ci_register2(_CI_HT2, "../src/cri/ci_ht", {})
_ci_register2(_CI_MF2, "../src/cri/ci_mf", {})
_ci_register2(_CI_PL2, "../src/cri/ci_pl", {})
for _d, _st, _nm in ((_CI_HT_SPECS, "../src/cri/ci_ht", _CI_HT_NAMES), (_CI_MF_SPECS, "../src/cri/ci_mf", _CI_MF_NAMES), (_CI_PL_SPECS, "../src/cri/ci_pl", _CI_PL_NAMES)):
    _ci_register2({k: _d[k] for k in ("004e8110", "004e7d40") if k in _d} | {k: _d[k] for k in ("004ea690", "004ea838", "004ea9c8", "004eaa18", "004ea128") if k in _d}, _st, _nm)
_CI_ALL = _CI_ALL + list(_CI_DV2) + list(_CI_HT2) + list(_CI_MF2) + list(_CI_PL2)

C_STRTOL = ("int func_00444458(const char *s, char **e, int base) { unsigned r = p4_record({ID}, (unsigned)s, 0, base, 0); *e = (char *)s + 1 + (s[0] & 3); return (int)((unsigned)(unsigned char)s[0] * 31u + r); }")


def _py_strtol(m):
    cid, n, ok = _ok(m, 0x444458)
    s = m.g32(4)
    m.ilog.extend([cid, s, 0, m.g32(6), 0])
    m.mem.wn(m.g32(5), 4, s + 1 + (m.mem.rb(s) & 3))
    return (m.mem.rb(s) * 31 + ok) & 0xFFFFFFFF


def _py_sprintf_s(m):
    # sprintf(buf, fmt, str) with one %s
    buf, fmt, sarg = m.g32(4), m.g32(5), m.g32(6)
    f = bytearray()
    j = 0
    while m.mem.rb(fmt + j):
        f.append(m.mem.rb(fmt + j)); j += 1
    s = bytearray()
    j = 0
    while m.mem.rb(sarg + j):
        s.append(m.mem.rb(sarg + j)); j += 1
    out = bytes(f).replace(b"%s", bytes(s), 1) + b"\0"
    for k, b in enumerate(out):
        m.mem.wb(buf + k, b)
    return len(out) - 1


def _fd_mask(rng):
    return 0xFD000000 | rng.randrange(256)


def _search_extra(base_words):
    def ex(rng, img, args):
        img.touch(0x734000)
        img.w32(base_words, rng.choice([0, 0x1234]))
        img.w32(base_words + 4, rng.randrange(1 << 32))
        img.w32(base_words + 8, rng.randrange(0, 4))
        img.w32(base_words + 0xC, rng.randrange(0, 4))
        args[2] = img.alloc(0x400, fill_random=True)
        args[3] = rng.choice([0, 1, 2, 5, 8, 8])
        img.callee_ret[0x4E6778] = _fd_mask(rng)
        img.callee_ret[0x4E8428] = _fd_mask(rng)
    return ex


def _host_extra(rng, img, args):
    img.touch(0x75B000)
    for j, ch in enumerate("host\0"):
        img.mem.wb(0x75B768 + j, ord(ch))
    if rng.random() < 0.45:
        s = "host" + "".join(rng.choice("abc") for _ in range(rng.randrange(0, 5)))
        for j, ch in enumerate(s + "\0"):
            img.mem.wb(args[0] + j, ord(ch))


def _case_extra(rng, img, args):
    nb = args[0]
    s = "".join(rng.choice("abczXYZ/\\01@`{") for _ in range(rng.randrange(0, 12)))
    for j, ch in enumerate(s + "\0"):
        img.mem.wb(nb + j, ord(ch))


def _sprintf_fmt_extra(rng, img, args):
    _mfname_extra(rng, img, args)
    for a, s in ((0x75BE00, "E01100308:length of '%s' is not 17 bytes.(mfci_get_adr_size)"), (0x75BE40, "E01100309:illegal file name format '%s'(mfci_get_adr_size)")):
        for j, ch in enumerate(s + "\0"):
            img.mem.wb(a + j, ord(ch))


def _mfname_extra2(rng, img, args):
    if rng.random() < 0.6:
        s = "%08x.%08x" % (rng.randrange(1 << 32), rng.randrange(1 << 32))
    else:
        s = rng.choice(["0123.4567", "0123456789abcdef0123", "0123456x89abcdef0", "", "0123456789abcdefg"])
    for j, ch in enumerate(s + "\0"):
        img.mem.wb(args[0] + j, ord(ch))


_CI_IMPLS[0x444458] = (C_STRTOL, _py_strtol)
_CI_IMPLS[0x442088] = ("", _py_sprintf_s)
_CI_DV3 = {
    "004e36b8": ("", {}, [0x43F9C8], {0x43F9C8: 3}, False, _req_extra),
    "004e39e8": ("p", {0x42E7C8: [0]}, [0x4E4618, 0x42E7C8], {0x4E4618: 1, 0x42E7C8: 1}, False, None),
    "004e5c68": ("s", {}, [0x442948], {}, False, _case_extra),
    "004e6490": ("s", {0x4E6EB0: [0, 0x100, 0x1, 0x10100, 0x201], 0x4E5A68: [0, 1]}, [0x4E6EB0, 0x4E6640, 0x4E4618, 0x42E7C8, 0x4E5A68, 0x4CD3F0], {0x4E6EB0: 1, 0x4E6640: 2, 0x4E4618: 1, 0x42E7C8: 1, 0x4E5A68: (False, True, False, False), 0x4CD3F0: 2}, True, None),
    "004e6cc0": ("qsqn", {}, [0x4E6778], {0x4E6778: 2}, False, _search_extra(0x733DB0)),
    "004e6db0": ("s", {}, [0x442948], {}, True, _mfname_extra2),
    "004e6df0": ("sqq", {}, [0x444458, 0x4CD378], {0x444458: (True, False, True, False), 0x4CD378: 2}, True, None),
}
_CI_HT3 = {
    "004e81d8": ("s", {}, [0x442948], {}, True, None),
    "004e8248": ("s", {}, [0x4426E8, 0x43F9C8], {}, True, _host_extra),
    "004e8aa8": ("qsqn", {}, [0x4E8428], {0x4E8428: 2}, False, _search_extra(0x734370)),
}
_CI_MF3 = {
    "004e9e98": ("sqm", {0x444458: [0, 1, 7]}, [0x444458], {0x444458: (True, False, True, False)}, True, None),
}

_CI_MF2["004e9eb0"] = ("sq", {}, [0x442948, 0x442088, 0x4E9E68, 0x4E9E98], {0x4E9E68: 2, 0x4E9E98: (True, False, True, False)}, True, _sprintf_fmt_extra)
_ci_register2({"004e9eb0": _CI_MF2["004e9eb0"]}, "../src/cri/ci_mf", {})
_ci_register2(_CI_DV3, "../src/cri/ci_dv", {})
_CI_SPEC_IMPLS["004e8248"] = {0x43F9C8: ("", _py_memset_heap)}
_ci_register2(_CI_HT3, "../src/cri/ci_ht", {})
_ci_register2(_CI_MF3, "../src/cri/ci_mf", {})
_CI_ALL = _CI_ALL + list(_CI_DV3) + list(_CI_HT3) + list(_CI_MF3)

def _pair_extra(rng, img, args):
    s = "".join(rng.choice("ab") for _ in range(rng.randrange(0, 5)))
    for k in (0, 1):
        t = s if rng.random() < 0.6 else "".join(rng.choice("ab") for _ in range(rng.randrange(0, 5)))
        for j, ch in enumerate(t + "\0"):
            img.mem.wb(args[k] + j, ord(ch))


_CI_DV4 = {
    "004e6778": ("ss", {0x4E6720: [0xFD000000 | 0x55, 0, 0xFD000000 | 0xFF, 0xFD000000 | 0x10]}, [0x442948, 0x4E6720], {0x4E6720: 2}, True, _pair_extra),
}
_CI_HT4 = {
    "004e8428": ("ss", {0x4E83D0: [0xFD000000 | 0x55, 0, 0xFD000000 | 0xFF, 0xFD000000 | 0x10]}, [0x442948, 0x4E83D0], {0x4E83D0: 2}, True, _pair_extra),
}
_ci_register2(_CI_DV4, "../src/cri/ci_dv", {})
_ci_register2(_CI_HT4, "../src/cri/ci_ht", {})
_CI_ALL = _CI_ALL + list(_CI_DV4) + list(_CI_HT4)

def _chars_extra(rng, img, args):
    for k in range(len(args)):
        args[k] = rng.choice([0x2F, 0x5C, 0x61, 0x7A, 0x60, 0x7B, 0x41, 0x5A, 0x80, 0xFF, 0, 0x2E, rng.randrange(256)]) | (rng.randrange(1 << 24) << 8 if rng.random() < 0.3 else 0)


_CI_DV5 = {
    "004e66e8": ("v", {}, [], {}, True, _chars_extra),
    "004e6720": ("vv", {}, [0x4E66E8], {0x4E66E8: 1}, True, _chars_extra),
}
_CI_HT5 = {
    "004e83b8": ("v", {}, [], {}, True, _chars_extra),
    "004e83d0": ("vv", {}, [0x4E83B8], {0x4E83B8: 1}, True, _chars_extra),
}
_ci_register2(_CI_DV5, "../src/cri/ci_dv", {})
_ci_register2(_CI_HT5, "../src/cri/ci_ht", {})
_CI_ALL = _CI_ALL + list(_CI_DV5) + list(_CI_HT5)

def _mk_chfold(addr):
    def py(m):
        cid, n, ok = _ok(m, addr)
        v = m.g32(4)
        m.ilog.extend([cid, v, 0, 0, 0])
        sv = v - (1 << 32) if v & 0x80000000 else v
        return (sv & 0xDF) & 0xFFFFFFFF
    return py


_CI_SPEC_IMPLS["004e6720"] = {0x4E66E8: ("int func_004e66e8(int c) { p4_record({ID}, c, 0, 0, 0); return c & 0xDF; }", _mk_chfold(0x4E66E8))}
_CI_SPEC_IMPLS["004e83d0"] = {0x4E83B8: ("int func_004e83b8(int c) { p4_record({ID}, c, 0, 0, 0); return c & 0xDF; }", _mk_chfold(0x4E83B8))}
_ci_register2({"004e6720": _CI_DV5["004e6720"]}, "../src/cri/ci_dv", {})
_ci_register2({"004e83d0": _CI_HT5["004e83d0"]}, "../src/cri/ci_ht", {})

SOLO = ("0039ba80", "0039bbd0", "00396940", "003d5330", "003c49a0", "00399470", "00399bf0", "00398890", "00399db0", "003d5750", "003d8070", "003ca740", "003ca740@3ca780", "003ca740@3ca7a0", "003d7c50", "003ce170", "003ce230", "003ce050", "003ce840", "003ceeb0", "003cc6e0", "003d2010", "003d6fb0", "003d0140", "003cfff0", "003d3ae0", "003c65b0", "003c1ea0", "003c9d40", "00399470", "003d9760", "00390680", "00391400", "00391c10", "00392650", "00392a50", "003d0230", "003bd6b0", "003c70e0", "003d9f30")
SOLO = SOLO + tuple(_CI_ALL)


def main(argv):
    if not argv:
        bad = main([a for a in sorted(SPECS) if a not in SOLO])
        for a in SOLO:
            bad |= main([a])
        return bad
    addrs = argv
    # "STEM@ENTRY" runs the asm of function STEM starting at the second entry point ENTRY (several thunks in one block)
    entries = [(SPECS[a][4] if len(SPECS[a]) > 4 else "func_" + a, STEMS.get(a, a.split("@")[0])) for a in addrs]
    callees = []
    nargs = {}
    for a in addrs:
        spec = SPECS[a]
        for ca in (spec[3] if len(spec) > 3 else ()):
            if ca not in callees:
                callees.append(ca)
    extra = []
    impls = {}
    for ca in list(callees):
        if ca in REAL:
            callees.remove(ca)
            if REAL[ca][0] not in addrs:
                extra.append(REAL[ca][0])
            impls[ca] = REAL[ca][1]
    c_impls = {}
    for a in addrs:
        ex = EXTRA.get(a, {})
        rn.DEREF.update(ex.get("deref", {}))
        c_impls.update(ex.get("c_impls", {}))
        impls.update(ex.get("impls", {}))
        for raddr, kind in ex.get("reader", {}).items():
            c_impls[raddr] = C_READER_WORD if kind == "word" else C_READER_REC20
            impls[raddr] = _py_reader_factory(raddr, kind)
    impls[0x43F810] = _py_memcpy
    harness = rn.build(entries, callees, extra, c_impls)
    bad = 0
    for i, a in enumerate(addrs):
        spec = SPECS[a]
        gen, has_ret, hook_args = spec[0], spec[1], spec[2]
        cases = mkcases(gen, N_CASES, seed=int(a.split("@")[0], 16))
        for c in cases:
            c.ret = has_ret
        fails = rn.check(int(a.split("@")[0], 16), entries[i][0], cases, harness, i, callees=callees,
                         entry=(int(a.split("@")[1], 16) if "@" in a else None),
                         callee_nargs=(spec[5] if len(spec) > 5 else {}),
                         hook_args=hook_args, impls=impls,
                         fuzzy=any(EXTRA.get(x, {}).get('fuzzy') for x in [a]))
        if fails:
            bad += 1
            print(a, "FAIL", fails[:3])
        else:
            print(a, "OK (%d cases)" % len(cases))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
