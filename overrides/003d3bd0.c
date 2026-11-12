/* func_003d3bd0 (652 bytes): prepares a draw record for rendering and returns 1. arg0 is the record (+8 = the object, +0xC = its
 * model header, +0x10 = the mesh slot, +0x14 = flags, +0x18 = key, +0x28/+0x29/+0x2A = draw modes), arg1 receives the camera
 * matrix pointer (*(u32 *)0x008872E0 + 0x20). The model header is read from the object word +0x78 (stored to +0xC) and its mesh slot is taken
 * from the slot at object + *(s32 *)0x00764818 (gp-0x48D8; gp is 0x007690F0): reused when it is bound to the same index (halfword +4 of the header), else
 * remade with func_003ce050. The render flags come from word +8 of **(u32 *)0x008872E4: without bit 0x1000000 the key becomes
 * (flags & 0x9D) << 16 | header halfword +6 and bit 0 of +0x14 is cleared, with +0x14 bit 8 or 4 set when the key differs from the slot's word +0x28
 * (8 when the byte at bit 16 differs). The camera frustum test func_003d3ae0(object, &result) then chooses the draw mode +0x29 and the
 * flag byte +0x28 (the byte at 0x00764A74, gp-0x467C, or'ed with 4, 0x10, 0x14 or 2, from the header's first word: bit 0 -> 4, no flags or bit 1 -> 3, bit 4 -> 4,
 * bit 3 -> 2, else 1); +0x28 gets 2 when the test result is not 2. +0x2A = bit 6 of the render flags and, with bit 5, the
 * callback func_003d3a90 is registered (func_003c96d0 / func_003c9750). */

extern s32 p4c_003c96d0_003d3bd0(void (*cb)(void), s32 a) __asm__("func_003c96d0");
extern s32 p4c_003c9750_003d3bd0(u32 obj, void (*cb)(void), s32 a) __asm__("func_003c9750");
extern u8 *p4c_003ce050_003d3bd0(u8 *slot, s32 n) __asm__("func_003ce050");
extern void p4c_003d3ae0_003d3bd0(u32 obj, s32 *out) __asm__("func_003d3ae0");

extern void p4cb_003d3a90_003d3bd0(void) __asm__("func_003d3a90");

u32 func_003d3bd0(u32 *arg0, u32 *arg1)
{
    u32 obj = arg0[2];
    u32 *slot;
    u32 *hdr;
    u32 flags;
    s32 res[1];
    u32 w;
    u32 old;
    u32 key;

    arg0[3] = *(u32 *)(obj + 0x78);
    hdr = (u32 *)arg0[3];
    slot = (u32 *)(obj + *(u32 *)0x00764818);
    if (*slot != 0 && (u32) * (u16 *)((u8 *)hdr + 4) == **(u32 **)slot) {
        arg0[4] = *slot;
    } else {
        slot = (u32 *)p4c_003ce050_003d3bd0((u8 *)slot, *(u16 *)((u8 *)hdr + 4));
        arg0[4] = *slot;
    }
    flags = ((u32 *)*(u32 *)0x008872E4)[2];
    if ((flags & 0x1000000) == 0) {
        key = (((flags & 0x9D) & 0xFF) << 16) | *(u16 *)((u8 *)arg0[3] + 6);
        arg0[6] = key;
        arg0[5] = arg0[5] & 0xFFFFFFFEu;
        old = ((u32 *)arg0[4])[1];
        if (old != 0) {
            w = ((u32 *)old)[10];
            if (arg0[6] != w) {
                if (((arg0[6] >> 16) & 0xFF) != ((w >> 16) & 0xFF)) {
                    arg0[5] = arg0[5] | 8;
                } else {
                    arg0[5] = arg0[5] | 4;
                }
            }
        }
    }
    arg1[0] = *(u32 *)0x008872E0 + 0x20;
    p4c_003d3ae0_003d3bd0(obj, res);
    w = *(u32 *)arg0[3];
    if ((w & 1) == 1) {
        *((u8 *)arg0 + 0x29) = 4;
        *((u8 *)arg0 + 0x28) = *(u8 *)0x00764A74;
    } else if ((w & 0xFF) == 0 || (w & 2) == 2) {
        *((u8 *)arg0 + 0x29) = 3;
        *((u8 *)arg0 + 0x28) = *(u8 *)0x00764A74 | 4;
    } else if ((w & 0x10) == 0x10) {
        *((u8 *)arg0 + 0x29) = 4;
        *((u8 *)arg0 + 0x28) = *(u8 *)0x00764A74;
    } else if ((w & 8) == 8) {
        *((u8 *)arg0 + 0x29) = 2;
        *((u8 *)arg0 + 0x28) = *(u8 *)0x00764A74 | 0x10;
    } else {
        *((u8 *)arg0 + 0x29) = 1;
        *((u8 *)arg0 + 0x28) = *(u8 *)0x00764A74 | 0x14;
    }
    if (res[0] != 2) {
        *((u8 *)arg0 + 0x28) = *((u8 *)arg0 + 0x28) | 2;
    }
    flags = ((u32 *)*(u32 *)0x008872E4)[2];
    *((u8 *)arg0 + 0x2A) = (flags & 0x40) != 0;
    if ((((u32 *)*(u32 *)0x008872E4)[2] & 0x20) != 0) {
        p4c_003c96d0_003d3bd0(p4cb_003d3a90_003d3bd0, 0);
        p4c_003c9750_003d3bd0(arg0[2], p4cb_003d3a90_003d3bd0, 0);
    }
    return 1;
}
