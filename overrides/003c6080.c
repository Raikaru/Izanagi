/* func_003c6080 (512 bytes): three-way comparison of two triangle records for sorting; arg0 and arg1 point at pointers to the
 * records. Each record p has a material pointer m = p[2]; equal materials give 0 at once. A material contributes 0x10 when its first pointer's object
 * has a texture kind (byte 0x23 of *m[0], bits 8 to 11 of the byte shifted left by 8) of 0x100, 0x300 or 0x500, and when its byte +7 is not 0xFF. For each of the halfword
 * fields +0x10 (8), +0x12 (4), +0xE (2) and +0xC (1) of the records the one with the lower value gets the weight added to its key (the
 * keys of p and q start with the material flags). Returns key(p) - key(q). */

static u32 p4_matflags_003c6080(u32 *m)
{
    u32 f = 0;
    u32 x;
    u32 k;

    if (m == (u32 *)0) {
        return 0;
    }
    x = m[0];
    if (x != 0) {
        k = ((u32) * (u8 *)(*(u32 *)x + 0x23) << 8) & 0xF00;
        if (k == 0x100 || k == 0x300 || k == 0x500) {
            f |= 0x10;
        }
    }
    if (*(u8 *)((u8 *)m + 7) != 0xFF) {
        f |= 0x10;
    }
    return f;
}

s32 func_003c6080(u8 **arg0, u8 **arg1)
{
    u8 *p = *arg0;
    u8 *q = *arg1;
    u32 *mp = *(u32 **)(p + 8);
    u32 *mq = *(u32 **)(q + 8);
    u32 a;
    u32 b;
    u32 pv;
    u32 qv;

    if (mp == mq) {
        return 0;
    }
    a = p4_matflags_003c6080(mp);
    b = p4_matflags_003c6080(mq);
    pv = *(u16 *)(p + 0x10);
    qv = *(u16 *)(q + 0x10);
    if (qv < pv) {
        a |= 8;
    }
    if (pv < qv) {
        b |= 8;
    }
    pv = *(u16 *)(p + 0x12);
    qv = *(u16 *)(q + 0x12);
    if (qv < pv) {
        a |= 4;
    }
    if (pv < qv) {
        b |= 4;
    }
    pv = *(u16 *)(p + 0xE);
    qv = *(u16 *)(q + 0xE);
    if (qv < pv) {
        a |= 2;
    }
    if (pv < qv) {
        b |= 2;
    }
    pv = *(u16 *)(p + 0xC);
    qv = *(u16 *)(q + 0xC);
    if (qv < pv) {
        a |= 1;
    }
    if (pv < qv) {
        b |= 1;
    }
    return (s32)(a - b);
}
