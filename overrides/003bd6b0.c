/* func_003bd6b0 (1616 bytes): reads one geometry record (stream chunk type 1) from the stream arg0 into a new object taken from the arena whose write cursor is *arg1, and returns the
 * object (0 on failure, after the cleanup func_003c91d0 (bit 0 of the byte at arg2 + 3) / func_003c9300 of the half built object; arg2 is the block descriptor: +8 flags, bit 0x1000000
 * skips all the data, +0x20 the number of extra arrays). The chunk version (func_003df050(stream, 1, 0, &version)) must lie in [0x35000, 0x37003), else the error record {2,
 * func_003df590(0x80000004)} is reported through func_003df4d0. A 0x2C byte header (func_003e2910) gives h[0] (halfword at +0x80), h[1] the count of 8 byte entries (halfword +0x84) and h[2] the count of
 * vertices (halfword +0x82) and six floats (to +0x6C..+0x74 and +0x60..+0x68). The object has the size of the word at 0x0070B040, its first word is -1, the words +4 / +8 / +0xC / +0x30 and
 * the 0x20 bytes at +0x10 are cleared and +0x38 / +0x40 are two empty circular list heads. With h[2] != 0 12 * h[2] bytes (+8, func_003df300), 4 * h[2] bytes at +0xC when bit 0x10 of
 * arg3 is set (func_003e2910, must return the size) and 4 * h[2] bytes at +0x30 when bit 8 is set (func_003e2910, non zero) are read, then arg2[0x20] arrays of h[2] * 8 bytes at +0x10 + 4 * i
 * (func_003df300); then, with h[1] != 0, h[1] * 8 bytes at +4 (func_003df2a0), whose halfwords are rotated (a, b, c, d to b, c, d, a) when the version is below 0x36002. Every arena allocation
 * that returns 0 reports {2, func_003df590(0x80000013, size)} and fails. Finally the object is registered (func_003e3b70(0x0070B040, object), the two words at gp - 0x4964 / -0x4968
 * (0x0076478C, 0x00764788) cleared, func_003e3110(0x0070B040, stream, object) must succeed and func_003e32f0(0x0070B040, word, object, second word) follows when the first word is set). */

extern void p4c_003c91d0_003bd6b0(u8 *o) __asm__("func_003c91d0");
extern void p4c_003c9300_003bd6b0(u8 *o) __asm__("func_003c9300");
extern s32 p4c_003df050_003bd6b0(u32 s, s32 t, void *z, void *v) __asm__("func_003df050");
extern s32 p4c_003df2a0_003bd6b0(u32 s, void *b, u32 n) __asm__("func_003df2a0");
extern s32 p4c_003df300_003bd6b0(u32 s, void *b, u32 n) __asm__("func_003df300");
extern void p4c_003df4d0_003bd6b0(void *e) __asm__("func_003df4d0");
extern u32 p4c_003df590_003bd6b0(u32 c, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003bd6b0(u32 s, void *b, u32 n) __asm__("func_003e2910");
extern s32 p4c_003e3110_003bd6b0(u32 l, u32 s, u8 *o) __asm__("func_003e3110");
extern void p4c_003e32f0_003bd6b0(u32 l, u32 v, u8 *o, u32 w) __asm__("func_003e32f0");
extern void p4c_003e3b70_003bd6b0(u32 l, u8 *o) __asm__("func_003e3b70");
extern void p4c_0043f9c8_003bd6b0(void *p, s32 v, u32 n) __asm__("func_0043f9c8");

u32 func_003bd6b0(u32 arg0, u32 *arg1, u8 *arg2, u32 arg3)
{
    u32 ver = 0;
    u32 hdr[11];
    u32 err[2];
    u32 cur;
    u32 sz;
    u32 i;
    s32 ok = 1;
    u8 *o;

    for (i = 0; i < 11; i++) {
        hdr[i] = 0;
    }
    if (p4c_003df050_003bd6b0(arg0, 1, 0, &ver) == 0) {
        return 0;
    }
    if (ver < 0x35000 || ver >= 0x37003) {
        err[0] = 2;
        err[1] = p4c_003df590_003bd6b0(0x80000004);
        p4c_003df4d0_003bd6b0(err);
        return 0;
    }
    if (p4c_003e2910_003bd6b0(arg0, hdr, 0x2C) != 0x2C) {
        return 0;
    }
    cur = *arg1;
    *arg1 = cur + *(u32 *)0x0070B040;
    if (cur == 0) {
        err[0] = 2;
        err[1] = p4c_003df590_003bd6b0(0x80000013, *(u32 *)0x0070B040);
        p4c_003df4d0_003bd6b0(err);
        return 0;
    }
    o = (u8 *)cur;
    *(s32 *)o = -1;
    *(u32 *)(o + 0x6C) = hdr[3];
    *(u32 *)(o + 0x70) = hdr[4];
    *(u32 *)(o + 0x74) = hdr[5];
    *(u32 *)(o + 0x60) = hdr[6];
    *(u32 *)(o + 0x64) = hdr[7];
    *(u32 *)(o + 0x68) = hdr[8];
    *(u16 *)(o + 0x80) = hdr[0];
    *(u16 *)(o + 0x84) = hdr[1];
    *(u16 *)(o + 0x82) = hdr[2];
    *(u32 *)(o + 4) = 0;
    *(u32 *)(o + 8) = 0;
    *(u32 *)(o + 0x30) = 0;
    *(u32 *)(o + 0xC) = 0;
    p4c_0043f9c8_003bd6b0(o + 0x10, 0, 0x20);
    *(u32 *)(o + 0x7C) = 0;
    *(u32 *)(o + 0x34) = 0;
    *(u32 *)(o + 0x78) = 0;
    *(u32 *)(o + 0x38) = (u32)(o + 0x38);
    *(u32 *)(o + 0x3C) = (u32)(o + 0x38);
    *(u32 *)(o + 0x40) = (u32)(o + 0x40);
    *(u32 *)(o + 0x44) = (u32)(o + 0x40);
    if (!(*(u32 *)(arg2 + 8) & 0x01000000)) {
        if (hdr[2] != 0) {
            sz = hdr[2] * 12;
            cur = *arg1;
            *arg1 = cur + sz;
            *(u32 *)(o + 8) = cur;
            if (cur == 0) {
                err[0] = 2;
                err[1] = p4c_003df590_003bd6b0(0x80000013, sz);
                p4c_003df4d0_003bd6b0(err);
                ok = 0;
            }
            if (ok) {
                if (p4c_003df300_003bd6b0(arg0, (void *)*(u32 *)(o + 8), sz) == 0) {
                    ok = 0;
                }
            }
            if (ok && (arg3 & 0x10)) {
                sz = hdr[2] * 4;
                cur = *arg1;
                *arg1 = cur + sz;
                *(u32 *)(o + 0xC) = cur;
                if (cur == 0) {
                    err[0] = 2;
                    err[1] = p4c_003df590_003bd6b0(0x80000013, sz);
                    p4c_003df4d0_003bd6b0(err);
                    ok = 0;
                }
                if (ok) {
                    if (p4c_003e2910_003bd6b0(arg0, (void *)*(u32 *)(o + 0xC), sz) != (s32)sz) {
                        ok = 0;
                    }
                }
            }
            if (ok && (arg3 & 8)) {
                sz = hdr[2] * 4;
                cur = *arg1;
                *arg1 = cur + sz;
                *(u32 *)(o + 0x30) = cur;
                if (cur == 0) {
                    err[0] = 2;
                    err[1] = p4c_003df590_003bd6b0(0x80000013, sz);
                    p4c_003df4d0_003bd6b0(err);
                    ok = 0;
                }
                if (ok) {
                    if (p4c_003e2910_003bd6b0(arg0, (void *)*(u32 *)(o + 0x30), sz) == 0) {
                        ok = 0;
                    }
                }
            }
            if (ok && *(s32 *)(arg2 + 0x20) > 0) {
                u32 msz = (u32)*(u16 *)(o + 0x82) << 3;

                for (i = 0; i < (u32)*(s32 *)(arg2 + 0x20); i++) {
                    cur = *arg1;
                    *arg1 = cur + msz;
                    *(u32 *)(o + 0x10 + 4 * i) = cur;
                    if (cur == 0) {
                        err[0] = 2;
                        err[1] = p4c_003df590_003bd6b0(0x80000013, msz);
                        p4c_003df4d0_003bd6b0(err);
                        ok = 0;
                        break;
                    }
                    if (p4c_003df300_003bd6b0(arg0, (void *)cur, msz) == 0) {
                        ok = 0;
                        break;
                    }
                }
            }
        }
        if (ok && hdr[1] != 0) {
            sz = hdr[1] << 3;
            cur = *arg1;
            *arg1 = cur + sz;
            *(u32 *)(o + 4) = cur;
            if (cur == 0) {
                err[0] = 2;
                err[1] = p4c_003df590_003bd6b0(0x80000013, sz);
                p4c_003df4d0_003bd6b0(err);
                ok = 0;
            }
            if (ok) {
                if (p4c_003df2a0_003bd6b0(arg0, (void *)*(u32 *)(o + 4), sz) == 0) {
                    ok = 0;
                }
            }
            if (ok && ver < 0x36002 && (s32)hdr[1] > 0) {
                for (i = 0; i < hdr[1]; i++) {
                    u16 *p = (u16 *)(*(u32 *)(o + 4) + 8 * i);
                    u16 a = p[0];
                    u16 b = p[1];
                    u16 c = p[2];
                    u16 d = p[3];

                    p[0] = b;
                    p[1] = c;
                    p[2] = d;
                    p[3] = a;
                }
            }
        }
    }
    if (ok) {
        p4c_003e3b70_003bd6b0(0x0070B040, o);
        *(u32 *)0x0076478C = 0;
        *(u32 *)0x00764788 = 0;
        if (p4c_003e3110_003bd6b0(0x0070B040, arg0, o) == 0) {
            ok = 0;
        }
        if (ok && *(u32 *)0x0076478C != 0) {
            p4c_003e32f0_003bd6b0(0x0070B040, *(u32 *)0x0076478C, o, *(u32 *)0x00764788);
        }
    }
    if (ok) {
        return (u32)o;
    }
    if (*(u8 *)(arg2 + 3) & 1) {
        p4c_003c91d0_003bd6b0(o);
    } else {
        p4c_003c9300_003bd6b0(o);
    }
    return 0;
}
