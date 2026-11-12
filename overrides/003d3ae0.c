/* func_003d3ae0 (264 bytes): frustum test of a box against six planes. arg0 holds the box as two
 * float triples at +0x60 (A) and +0x6C (B). *arg1 receives 2 (inside every plane) or 1 (outside one).
 *
 * The planes are at *(u32 *)0x008872E0 + *(s32 *)0x00764A24 (gp-0x46CC; gp is 0x007690F0) + 0x60,
 * 0x14 bytes each: normal (3 floats), distance, then three selector bytes at +0x10, +0x11, +0x12 (0 or
 * 1). Per plane the box corner chosen by the selectors (x from A when the byte at +0x10 is 1, else B,
 * and likewise y with +0x11 and z with +0x12) gives d = (y*ny + x*nx) + z*nz - distance; the first
 * plane with d > 0 (the box lies outside it) sets *arg1 = 1 and stops. Single precision, sum order y, x, z as retail. */

void func_003d3ae0(u8 *arg0, s32 *arg1)
{
    u8 *plane;
    s32 i;

    *arg1 = 2;
    plane = (u8 *)(*(u32 *)0x008872E0 + (u32)*(s32 *)0x00764A24 + 0x60);
    for (i = 0; i < 6; i++) {
        f32 nx = *(f32 *)(plane + 0);
        f32 ny = *(f32 *)(plane + 4);
        f32 nz = *(f32 *)(plane + 8);
        f32 dist = *(f32 *)(plane + 0xC);
        s32 bx = 1 - *(u8 *)(plane + 0x10);
        s32 by = 1 - *(u8 *)(plane + 0x11);
        s32 bz = 1 - *(u8 *)(plane + 0x12);
        f32 z = *(f32 *)(arg0 + 0x68 + 12 * bz);
        f32 y = *(f32 *)(arg0 + 0x64 + 12 * by);
        f32 x = *(f32 *)(arg0 + 0x60 + 12 * bx);
        f32 d;

        d = (y * ny);
        d = d + (x * nx);
        d = d + (z * nz);
        d = d - dist;
        if (d > 0.0f) {
            *arg1 = 1;
            break;
        }
        plane += 0x14;
    }
}
