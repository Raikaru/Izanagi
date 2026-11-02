/* func_003d76d0 (192 bytes): creates an animation from a 0x20-byte record and returns it, or 0. arg0 is the record (its name); func_003d7b40(arg0, 1, 2,
 * {0, 0, 0, 0}, 0) makes the object r (0 fails). With the identity matrix I (words 0, 5 and 10 = 1.0, the others 0 except word 3 which gets 0x20003 or'ed in;
 * words 7, 11 and 15 stay unset) func_003d65e0(r, r[4], 0, &I, 0.0f) and func_003d65e0(r, r[4] + (r[0])[2], r[4], &I, 1.0f) are called. */

extern s32 p4c_003d65e0_003d76d0(u32 *a, u32 *b, u32 c, u32 *d, f32 f) __asm__("func_003d65e0");
extern u32 *p4c_003d7b40_003d76d0(u8 *n, s32 c, u32 k, u8 *d, s32 m, f32 duration) __asm__("func_003d7b40");

u32 *func_003d76d0(u8 *arg0)
{
    u32 zero = 0;
    u32 m[16];
    u32 *r;
    u32 base;
    s32 i;

    for (i = 0; i < 16; i++) {
        m[i] = 0;
    }
    r = p4c_003d7b40_003d76d0(arg0, 1, 2, (u8 *)&zero, 0, 1.0f);   /* retail: f12 = 1.0 before the jal */
    if (r == (u32 *)0) {
        return (u32 *)0;
    }
    base = r[4];
    m[0] = 0x3F800000;
    m[5] = 0x3F800000;
    m[10] = 0x3F800000;
    m[3] = m[3] | 0x20003;
    p4c_003d65e0_003d76d0(r, (u32 *)base, 0, m, 0.0f);
    p4c_003d65e0_003d76d0(r, (u32 *)(base + ((u32 *)r[0])[2]), base, m, 1.0f);
    return r;
}
