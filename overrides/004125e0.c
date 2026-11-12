extern s32 func_003df590();
extern u8 *func_003df4d0();
extern s32 func_00411820(s32 reg);
extern s32 func_004118c0(u8 *reg, u32 size);
extern u8 *func_004123c0(u8 *reg);
extern s32 func_004147a0(u8 *reg);
extern s32 func_00411a70(u8 *reg, u8 *reg2);
extern void *func_0043f810(void *dst, const void *src, u32 n);

/* func_004125e0: closes a registry opened with func_00412ca0. `reg` has the opened flag at +0,
 * the entry count at +4, the 40-byte entry table at +8, the working buffer at +0x20 (capacity
 * +0x24) and the index of the registry's own entry at +0x28. Errors, each {1, code} and
 * returning 0: reg == 0 -> 0x80000016; reg not open -> 52; entries present but the +0x28 index
 * not naming a live entry -> 0x24. With no entries the flag is cleared and reg is returned.
 * Otherwise the working buffer is sized from the engine limit E (*(0x00764AA8) + 0x887318),
 * func_00411820() = F, the entry count and each record's +0x34 and +0x20 words, grown through
 * func_004118c0 and handed to func_004123c0, whose result replaces reg. The 12-byte blocks
 * after the table move to the end of the buffer (entry word 7 points at each), each entry's
 * word-1 sized array is copied behind the table (entry word 2 rebased), the buffer markers go
 * to the retail words 0x00764AB8 / 0x00764ABC, func_004147a0 runs and the buffer is trimmed.
 * Then each record's use count (+0x3c) is raised from the last entry down, running its +8
 * callback on the first use and its +0x10 callback with the entry, and the +0x18 callbacks run
 * from the last entry down. If all succeed the flag is cleared and reg returned. A failure
 * tears down the records already raised (+0x14 callback, count, +0xC callback at 0) and falls
 * through to func_00411a70, returning 0. */
u8 *func_004125e0(u8 *reg)
{
    s32 err[2];
    s32 count;
    s32 f;
    s32 e;
    u32 size;
    u32 need;
    u32 a;
    u32 i;
    u32 w;
    s32 acc;
    u8 *entry;
    u8 *rec;
    s32 s0;
    s32 s5;
    u8 *dst;
    u8 *src;
    u8 *table;
    u8 *data;
    u8 *tail;
    s32 undo;
    s32 failed;
    s32 total;
    u32 off;
    u32 j;
    s32 v;
    s32 (*cb)(u8 *);
    s32 (*cb2)(u8 *, u8 *);
    void (*cb3)(u8 *);
    void (*cb4)(u8 *);

    if (reg == 0) {
        err[0] = 1;
        err[1] = func_003df590(0x80000016);
        func_003df4d0(err);
        return 0;
    }
    if (*(s32 *)reg == 0) {
        err[0] = 1;
        err[1] = func_003df590(52);
        func_003df4d0(err);
        return 0;
    }
    if (*(s32 *)(reg + 4) == 0) {
        *(s32 *)reg = 0;
        return reg;
    }

    undo = 0;
    failed = 0;
    total = 0;
    if (*(u32 *)(reg + 4) <= *(u32 *)(reg + 0x28) ||
        *(s32 *)(*(s32 *)(reg + 8) + *(s32 *)(reg + 0x28) * 40) == 0) {
        err[0] = 1;
        err[1] = func_003df590(0x24);
        func_003df4d0(err);
        return 0;
    }

    f = func_00411820((s32)reg);
    count = *(s32 *)(reg + 4);
    e = *(s32 *)(*(s32 *)0x00764AA8 + 0x887318);
    size = e * 0xB4;
    acc = count * 40;
    for (i = 0; i < (u32)count; i++) {
        acc += (*(s32 *)(*(s32 *)(reg + 8) + 4)) * 4;
    }
    a = f * 0x14 + count * f * 4 + count * (f + 1) * 4;
    entry = *(u8 **)(reg + 8);
    for (i = 0; i < (u32)count; i++) {
        rec = *(u8 **)entry;
        if (*(s32 *)(rec + 0x34) != 0) {
            a += *(s32 *)(rec + 0x34);
        }
        a += *(s32 *)(rec + 0x20) * 4;
        entry += 40;
    }
    need = e * 12 + acc + count * (f * 0x24 + 0x14) + count * f * 0x10 + a + (f - 1) * 0x1C + 0x30;
    if (size < need) {
        size = need;
    }
    if (*(u32 *)(reg + 0x24) < size && func_004118c0(reg, size) == 0) {
        return 0;
    }
    *(u32 *)0x00764AB8 = *(u32 *)(reg + 0x20) + size;
    *(u32 *)0x00764ABC = 0;
    reg = func_004123c0(reg);
    if (reg == 0) {
        return 0;
    }

    count = *(s32 *)(reg + 4);
    e = *(s32 *)(*(s32 *)0x00764AA8 + 0x887318);
    dst = *(u8 **)(reg + 0x20) + size - 12;
    data = *(u8 **)(reg + 8) + e * 40;
    src = data + e * 128 + (count - 1) * 12;
    s0 = count - 1;
    s5 = s0 * 40;
    while (s0 >= 0) {
        func_0043f810(dst, src, 12);
        s0--;
        src -= 12;
        *(u8 **)(*(u8 **)(reg + 8) + s5 + 0x1C) = dst;
        s5 -= 40;
        dst -= 12;
    }

    count = *(s32 *)(reg + 4);
    tail = *(u8 **)(reg + 8) + count * 40;
    if (count != 0) {
        off = 0;
        for (j = 0; j < (u32)*(s32 *)(reg + 4); j++) {
            u8 *slot = *(u8 **)(reg + 8) + off;
            w = *(u32 *)(slot + 4);
            if (w == 0) {
                *(u32 *)(slot + 8) = 0;
            } else {
                func_0043f810(tail, data, w * 4);
                *(u8 **)(*(u8 **)(reg + 8) + off + 8) = tail;
            }
            data += 128;
            w = *(u32 *)(*(u8 **)(reg + 8) + off + 4);
            total += w;
            tail += w * 4;
            off += 40;
        }
    }
    *(u8 **)0x00764ABC = data + total * 4;
    *(u8 **)0x00764AB8 = src;
    if (func_004147a0(reg) != 0) {
        return 0;
    }

    if (func_004118c0(reg, *(u32 *)0x00764ABC - *(u32 *)(reg + 0x20)) != 0) {
        for (j = 0; j < (u32)*(s32 *)(reg + 4); j++) {
            *(s32 *)(*(u8 **)(reg + 8) + j * 40 + 0x1C) = 0;
        }
        s0 = *(s32 *)(reg + 4) - 1;
        while (s0 >= 0 && !failed) {
            entry = *(u8 **)(reg + 8) + s0 * 40;
            rec = *(u8 **)entry;
            v = *(s32 *)(rec + 0x3C);
            *(s32 *)(rec + 0x3C) = v + 1;
            if (v == 0 && *(s32 *)(rec + 8) != 0) {
                cb = *(s32 (**)(u8 *))(rec + 8);
                if (cb(rec) == 0) {
                    failed = 1;
                    undo = (*(s32 *)(reg + 4) - 1) - s0;
                    break;
                }
            }
            if (*(s32 *)(rec + 0x10) != 0) {
                cb = *(s32 (**)(u8 *))(rec + 0x10);
                if (cb(entry) == 0) {
                    *(s32 *)(rec + 0x3C) = *(s32 *)(rec + 0x3C) - 1;
                    if (*(s32 *)(rec + 0x3C) == 0 && *(s32 *)(rec + 0xC) != 0) {
                        cb3 = *(void (**)(u8 *))(rec + 0xC);
                        cb3(rec);
                    }
                    failed = 1;
                    undo = (*(s32 *)(reg + 4) - 1) - s0;
                    break;
                }
            }
            s0--;
        }
        if (!failed) {
            s0 = *(s32 *)(reg + 4) - 1;
            while (s0 >= 0) {
                entry = *(u8 **)(reg + 8) + s0 * 40;
                rec = *(u8 **)entry;
                if (*(s32 *)(rec + 0x18) != 0) {
                    cb2 = *(s32 (**)(u8 *, u8 *))(rec + 0x18);
                    if (cb2(entry, reg) == 0) {
                        undo = *(s32 *)(reg + 4);
                        failed = 1;
                        break;
                    }
                }
                s0--;
            }
        }
        if (!failed) {
            *(s32 *)reg = 0;
            return reg;
        }
        a = *(u32 *)(reg + 4) - undo;
        for (; a < *(u32 *)(reg + 4); a++) {
            rec = *(u8 **)(*(u8 **)(reg + 8) + a * 40);
            if (*(s32 *)(rec + 0x14) != 0) {
                cb4 = *(void (**)(u8 *))(rec + 0x14);
                cb4(*(u8 **)(reg + 8) + a * 40);
            }
            *(s32 *)(rec + 0x3C) = *(s32 *)(rec + 0x3C) - 1;
            if (*(s32 *)(rec + 0x3C) == 0 && *(s32 *)(rec + 0xC) != 0) {
                cb3 = *(void (**)(u8 *))(rec + 0xC);
                cb3(rec);
            }
        }
    }
    func_00411a70(reg, reg);
    return 0;
}
