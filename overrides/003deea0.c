/* func_003deea0 (336 bytes): reads a 12-byte packed header from the stream arg0 and stores up to four decoded values.
 * A short read reports {1, func_003df590(0x8000001A)} through func_003df4d0 and returns 0. The header words (w0, w1, w2)
 * give the record {w0, w1, v, u} with, when the high half of w2 is 0: v = w2 << 8 and u = 0, and otherwise v =
 * (((w2 >> 14) & 0x3FF00) + 0x30000) | ((w2 >> 16) & 0x3F) and u = w2 & 0xFFFF; func_003ded20(&record) normalises it, and
 * the non-null outputs arg1 = record[0], arg2 = record[1], arg3 = record[2] (v) and arg4 = record[3] (u) are stored.
 * Returns 1. */

extern u32 p4c_003ded20_003deea0(u32 *r) __asm__("func_003ded20");
extern void p4c_003df4d0_003deea0(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003deea0(s32 code, ...) __asm__("func_003df590");
extern s32 p4c_003e2910_003deea0(s32 s, void *b, s32 n) __asm__("func_003e2910");

s32 func_003deea0(s32 arg0, u32 *arg1, u32 *arg2, u32 *arg3, u32 *arg4)
{
    u32 hdr[3];
    u32 rec[4];
    s32 err[2];
    u32 w2;

    if (p4c_003e2910_003deea0(arg0, hdr, 12) != 12) {
        err[0] = 1;
        err[1] = p4c_003df590_003deea0(0x8000001A);
        p4c_003df4d0_003deea0(err);
        return 0;
    }
    w2 = hdr[2];
    rec[0] = hdr[0];
    rec[1] = hdr[1];
    if ((w2 & 0xFFFF0000) == 0) {
        rec[3] = 0;
        rec[2] = w2 << 8;
    } else {
        rec[3] = w2 & 0xFFFF;
        rec[2] = (((w2 >> 14) & 0x3FF00) + 0x30000) | ((w2 >> 16) & 0x3F);
    }
    p4c_003ded20_003deea0(rec);
    if (arg1 != (u32 *)0) {
        *arg1 = rec[0];
    }
    if (arg2 != (u32 *)0) {
        *arg2 = rec[1];
    }
    if (arg4 != (u32 *)0) {
        *arg4 = rec[3];
    }
    if (arg3 != (u32 *)0) {
        *arg3 = rec[2];
    }
    return 1;
}
