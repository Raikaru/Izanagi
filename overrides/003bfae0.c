/* func_003bfae0 (352 bytes): returns the world-space bounding sphere (centre at +0x2C, radius at +0x38)
 * of an object, recomputing it when needed. arg0: +3 flag byte (bit 0 = sphere dirty), +4 the frame, +0x1C the
 * local centre, +0x28 the local radius, +0x4C flags (bit 1 = needs func_003bf930), +0x2C and +0x38 the result.
 * Returns arg0 + 0x2C.
 *
 * After func_003bf930 (when asked), the sphere is up to date unless func_003e9240(frame) is non-zero or bit 0
 * of +3 is set. Then func_003e9700(frame) gives the frame's matrix m, func_003e42a0(arg0 + 0x2C, arg0 + 0x1C,
 * m) transforms the centre and the radius is the local radius, scaled by the largest row length of m
 * (sqrt of the largest of the three row sums of squares, each summed y, x, z as in the retail code) unless (m[3] & 3) == 3 (a
 * rigid matrix), then bit 0 of +3 is cleared. */

#include "p4pc/ps2_low_ram.h"

extern void p4c_003bf930_003bfae0(u8 *o) __asm__("func_003bf930");
extern void p4c_003e42a0_003bfae0(u8 *d, u8 *s, u8 *m) __asm__("func_003e42a0");
extern s32 p4c_003e9240_003bfae0(u8 *f) __asm__("func_003e9240");
extern u8 *p4c_003e9700_003bfae0(u8 *f) __asm__("func_003e9700");

u8 *func_003bfae0(u8 *arg0)
{
    u8 *frame;
    f32 *m;

    frame = *(u8 **)(arg0 + 4);
    if (*(u32 *)(arg0 + 0x4C) & 2) {
        p4c_003bf930_003bfae0(arg0);
    }
    if (frame == (u8 *)0) {
        /* The port's null-frame model has a null root, clear sync bits and a
         * zero world matrix; no executable low-memory image is installed. */
        if ((p4_ps2_low_ram_u8(p4_ps2_low_ram_u32(0xA0) + 3) & 3) == 0 && !(*(u8 *)(arg0 + 3) & 1)) {
            return arg0 + 0x2C;
        }
        m = (f32 *)p4_ps2_low_ram_ptr(0x50);
    } else {
        if (p4c_003e9240_003bfae0(frame) == 0 && !(*(u8 *)(arg0 + 3) & 1)) {
            return arg0 + 0x2C;
        }
        m = (f32 *)p4c_003e9700_003bfae0(frame);
    }
    p4c_003e42a0_003bfae0(arg0 + 0x2C, arg0 + 0x1C, m);
    if ((*(u32 *)((u8 *)m + 0xC) & 3) == 3) {
        *(f32 *)(arg0 + 0x38) = *(f32 *)(arg0 + 0x28);
    } else {
        f32 s0 = m[1] * m[1];
        f32 s1 = m[5] * m[5];
        f32 s2 = m[9] * m[9];
        f32 mx;

        s0 = s0 + m[0] * m[0];
        s0 = s0 + m[2] * m[2];
        s1 = s1 + m[4] * m[4];
        s1 = s1 + m[6] * m[6];
        s2 = s2 + m[8] * m[8];
        s2 = s2 + m[10] * m[10];
        mx = (s1 < s2) ? s2 : s1;
        if (s0 < mx) {
            s0 = (s1 < s2) ? s2 : s1;
        }
        *(f32 *)(arg0 + 0x38) = *(f32 *)(arg0 + 0x28) * __builtin_sqrtf(s0);
    }
    *(u8 *)(arg0 + 3) = (u8)(*(u8 *)(arg0 + 3) & 0xFE);
    return arg0 + 0x2C;
}
