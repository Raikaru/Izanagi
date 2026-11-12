/* func_003a6df0 (440 bytes): sets up one draw record. arg0 is the record (+8 the atomic, +0xC and +0x10 receive the geometry's data and its
 * mesh cache, +0x14 loses bit 0, +0x28 / +0x29 / +0x2A the draw mode bytes), arg1 points at the destination pointer that gets the 64 bytes of the camera matrix at
 * the camera (the pointer at 0x008872E0) + 0x20. The mesh cache of the geometry (slot at geometry + the offset word at gp - 0x48D0 (0x00764820)) is reused when its
 * first word equals the halfword at geometry[0x54] + 4, else func_003ce050(slot, that) makes it. The atomic's bounding sphere
 * (func_003bfae0(atomic): x, y, z, r) is tested against the six frustum planes (0x14 bytes each from camera + the offset word at gp - 0x46CC
 * (0x00764A24) + 0x60): a plane with x * p0 + y * p1 + z * p2 - p3 above -r marks the record 1 (not wholly inside), otherwise it is 2.
 * +0x29 = 4, +0x28 = the byte at gp - 0x467C (0x00764A74), or'ed with 2 unless the marker is 2, and +0x2A = 1. Returns nothing. */

extern u8 *p4c_003bfae0_003a6df0(u8 *o) __asm__("func_003bfae0");
extern u32 p4c_003ce050_003a6df0(u8 *o, u32 n) __asm__("func_003ce050");

void func_003a6df0(u8 *arg0, u32 **arg1)
{
    u8 *atomic = *(u8 **)(arg0 + 8);
    u8 *camera = *(u8 **)0x008872E0;
    u32 *src = (u32 *)(camera + 0x20);
    u8 *geo = *(u8 **)(atomic + 0x18);
    u32 **slot;
    u32 *cache;
    u32 *dst;
    f32 *sph;
    f32 *plane;
    s32 mark;
    s32 i;
    u8 mode;

    *(u32 *)(arg0 + 0xC) = *(u32 *)(geo + 0x54);
    slot = (u32 **)(geo + *(s32 *)0x00764820);
    if (*slot != (u32 *)0 && (u32)*(u16 *)(*(u8 **)(geo + 0x54) + 4) == **slot) {
        cache = *slot;
    } else {
        slot = (u32 **)p4c_003ce050_003a6df0((u8 *)slot, *(u16 *)(*(u8 **)(geo + 0x54) + 4));
        cache = *slot;
    }
    *(u32 **)(arg0 + 0x10) = cache;
    *(u32 *)(arg0 + 0x14) = *(u32 *)(arg0 + 0x14) & ~1u;
    dst = *arg1;
    for (i = 0; i < 16; i++) {
        dst[i] = src[i];
    }
    sph = (f32 *)p4c_003bfae0_003a6df0(atomic);
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
    arg0[0x29] = 4;
    mode = *(u8 *)0x00764A74;
    arg0[0x28] = mode;
    if (mark != 2) {
        arg0[0x28] = (u8)(arg0[0x28] | 2);
    }
    arg0[0x2A] = 1;
}
