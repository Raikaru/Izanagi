/* func_003d0540 (592 bytes): sets up one draw record. arg0 is the record (+8 the source object s, +0xC the geometry's data, +0x10 its mesh cache, +0x1C a
 * size product, +0x20 the geometry type, +0x28 to +0x2A the draw state) and arg1 the destination matrix pointer. The mesh cache comes from the slot at
 * the geometry g = s[6] + the offset word at gp - 0x48D0 (0x00764820) when g[6] is 1, else from the slot at s + the offset word at gp - 0x48D4
 * (0x0076481C); an existing cache is reused when its first word equals the halfword at g[0x54] + 4, else func_003ce050(slot, that) makes
 * one. arg0[8] = g[6][6]; below 2 the +0x1C product is skipped, otherwise it is s[0x58] * s[0x5C]. func_003cfff0(s, arg0) refreshes the dirty
 * flags, halfword g +0xC and bit 0 of s[0x4C] are cleared, *arg1 = func_003e05f0 of the matrix func_003e9700(s[1]) and the camera matrix at
 * the camera pointer (0x008872E0) + 0x20, and the bounding sphere func_003bfae0(s) is tested against the six frustum planes (0x14 bytes each at camera + the
 * offset word at gp - 0x46CC (0x00764A24) + 0x60) as in func_003a6df0 (1 outside a plane, else 2); func_003d0140(arg0, mark), +0x2A = bit 0x40 of
 * g[2] and func_003d0230(arg0) follow. Returns 1. */

extern u8 *p4c_003bfae0_003d0540(u8 *o) __asm__("func_003bfae0");
extern u32 p4c_003ce050_003d0540(u8 *o, u32 n) __asm__("func_003ce050");
extern void p4c_003cfff0_003d0540(u8 *o, u32 *r) __asm__("func_003cfff0");
extern void p4c_003d0140_003d0540(u8 *r, s32 m) __asm__("func_003d0140");
extern void p4c_003d0230_003d0540(u8 *r) __asm__("func_003d0230");
extern void p4c_003e05f0_003d0540(void *d, void *a, void *b) __asm__("func_003e05f0");
extern u8 *p4c_003e9700_003d0540(u32 o) __asm__("func_003e9700");

s32 func_003d0540(u8 *arg0, u32 **arg1)
{
    u8 *s = *(u8 **)(arg0 + 8);
    u8 *g = *(u8 **)(s + 0x18);
    u8 *camera;
    u32 **slot;
    u32 *cache;
    u32 *mat;
    f32 *sph;
    f32 *plane;
    s32 mark;
    s32 i;
    u32 v;

    *(u32 *)(arg0 + 0xC) = *(u32 *)(g + 0x54);
    if (*(s32 *)(g + 0x18) == 1) {
        slot = (u32 **)(g + *(s32 *)0x00764820);
    } else {
        slot = (u32 **)(s + *(s32 *)0x0076481C);
    }
    if (*slot != (u32 *)0 && (u32)*(u16 *)(*(u8 **)(g + 0x54) + 4) == **slot) {
        cache = *slot;
    } else {
        slot = (u32 **)p4c_003ce050_003d0540((u8 *)slot, *(u16 *)(*(u8 **)(g + 0x54) + 4));
        cache = *slot;
    }
    *(u32 **)(arg0 + 0x10) = cache;
    v = *(u32 *)(*(u8 **)(s + 0x18) + 0x18);
    *(u32 *)(arg0 + 0x20) = v;
    if ((s32)v >= 2) {
        *(f32 *)(arg0 + 0x1C) = *(f32 *)(s + 0x58) * *(f32 *)(s + 0x5C);
    }
    p4c_003cfff0_003d0540(s, (u32 *)arg0);
    *(u16 *)(*(u8 **)(s + 0x18) + 0xC) = 0;
    *(u32 *)(s + 0x4C) = *(u32 *)(s + 0x4C) & ~1u;
    camera = *(u8 **)0x008872E0;
    mat = (u32 *)p4c_003e9700_003d0540(*(u32 *)(s + 4));
    p4c_003e05f0_003d0540(*arg1, mat, camera + 0x20);
    sph = (f32 *)p4c_003bfae0_003d0540(s);
    plane = (f32 *)(camera + *(s32 *)0x00764A24 + 0x60);
    mark = 2;
    for (i = 0; i < 6; i++, plane += 5) {
        f32 acc = sph[1] * plane[1];
        f32 d;

        acc = acc + sph[0] * plane[0];
        d = (acc + sph[2] * plane[2]) - plane[3];
        if (!(d <= -sph[3])) {
            mark = 1;
            break;
        }
    }
    p4c_003d0140_003d0540(arg0, mark);
    arg0[0x2A] = (*(u32 *)(*(u8 **)(s + 0x18) + 8) & 0x40) != 0;
    p4c_003d0230_003d0540(arg0);
    return 1;
}
