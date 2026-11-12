/* func_003c1d40 (344 bytes): bounding sphere of a point list. arg0: +0 a descriptor whose +0x14 is the point
 * count n, +0x14 the points (12-byte float triples); arg1 receives (cx, cy, cz, r). Returns arg0.
 *
 * func_003e5110(box, points, n) fills the box minimum and maximum (two float triples). The centre is the
 * midpoint of the two. r is the float 0x3F8020C5 (1.001) times the largest distance from the centre to a point (squared
 * distances are summed y, x, z; 0 points or no distance above 0 gives r = 0 times the factor). */

extern void p4c_003e5110_003c1d40(f32 *box, u8 *pts, s32 n) __asm__("func_003e5110");

u8 *func_003c1d40(u8 *arg0, f32 *arg1)
{
    f32 box[6];
    f32 c[3];
    f32 maxd;
    s32 n;
    f32 *p;
    s32 i;
    u32 k_scale = 0x3F8020C5;

    n = *(s32 *)(*(u8 **)arg0 + 0x14);
    maxd = 0.0f;
    p4c_003e5110_003c1d40(box, *(u8 **)(arg0 + 0x14), n);
    c[0] = (box[3] + box[0]) * 0.5f;
    c[1] = (box[4] + box[1]) * 0.5f;
    c[2] = (box[5] + box[2]) * 0.5f;
    p = *(f32 **)(arg0 + 0x14);
    for (i = 0; i < n; i++) {
        f32 dy = p[1] - c[1];
        f32 dx = p[0] - c[0];
        f32 dz = p[2] - c[2];
        f32 d;

        d = dy * dy;
        d = d + dx * dx;
        d = d + dz * dz;
        if (!(d <= maxd)) {
            maxd = d;
        }
        p += 3;
    }
    if (!(maxd <= 0.0f)) {
        maxd = __builtin_sqrtf(maxd);
    }
    arg1[0] = c[0];
    arg1[1] = c[1];
    arg1[2] = c[2];
    arg1[3] = *(f32 *)&k_scale * maxd;
    return arg0;
}
