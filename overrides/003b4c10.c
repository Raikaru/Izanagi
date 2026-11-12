/* func_003b4c10 (416 bytes): like func_003b4ab0, with an offset added to every corner.
 * arg0: +0 destination base, +4 slot stride S. arg1 and arg2 point at three floats each
 * (a, b); arg3 is the record count; arg4: +4 source base, +8 source stride, +0x68, +0x6C and
 * +0x70 floats c68, c6C, s. With h = 0.5*s: A = a*h, B = b*h (per component) and the offset
 * o = c6C*b + c68*a (per component; the retail code forms it with mula.s/madd.s, a product
 * rounded and then summed). Record i at source position P (three floats) is written as
 * three floats P + delta at:
 *
 *   dst + 0*S : o + (-A - B)     dst + 1*S : o + (A - B)
 *   dst + 2*S : o + (A + B)      dst + 3*S : o + (-A + B)
 *
 * then dst advances by 4*S and the source by its stride. Does nothing when arg3 <= 0.
 * Leaf function. */

void func_003b4c10(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4)
{
    u8 *dst;
    u8 *src;
    u32 stride;
    u32 srcStride;
    f32 c68;
    f32 c6C;
    f32 h;
    f32 ox, oy, oz;
    f32 ax, ay, az;
    f32 bx, by, bz;
    f32 d0x, d0y, d0z;
    f32 d1x, d1y, d1z;
    f32 d2x, d2y, d2z;
    f32 d3x, d3y, d3z;
    s32 i;

    src = *(u8 **)(arg4 + 4);
    srcStride = *(u32 *)(arg4 + 8);
    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    c68 = *(f32 *)(arg4 + 0x68);
    c6C = *(f32 *)(arg4 + 0x6C);
    h = 0.5f * *(f32 *)(arg4 + 0x70);

    ox = c6C * arg2[0] + c68 * arg1[0];
    oy = c6C * arg2[1] + c68 * arg1[1];
    oz = c6C * arg2[2] + c68 * arg1[2];
    ax = arg1[0] * h;
    ay = arg1[1] * h;
    az = arg1[2] * h;
    bx = arg2[0] * h;
    by = arg2[1] * h;
    bz = arg2[2] * h;

    d0x = ox + ((-ax) - bx);
    d0y = oy + ((-ay) - by);
    d0z = oz + ((-az) - bz);
    d1x = ox + (ax - bx);
    d1y = oy + (ay - by);
    d1z = oz + (az - bz);
    d2x = ox + (ax + bx);
    d2y = oy + (ay + by);
    d2z = oz + (az + bz);
    d3x = ox + ((-ax) + bx);
    d3y = oy + ((-ay) + by);
    d3z = oz + ((-az) + bz);

    for (i = 0; i < arg3; i++) {
        f32 *p = (f32 *)src;
        f32 *q0 = (f32 *)dst;
        f32 *q1 = (f32 *)(dst + stride);
        f32 *q2 = (f32 *)(dst + 2 * stride);
        f32 *q3 = (f32 *)(dst + 3 * stride);

        q0[0] = p[0] + d0x;
        q0[1] = p[1] + d0y;
        q0[2] = p[2] + d0z;
        q1[0] = p[0] + d1x;
        q1[1] = p[1] + d1y;
        q1[2] = p[2] + d1z;
        q2[0] = p[0] + d2x;
        q2[1] = p[1] + d2y;
        q2[2] = p[2] + d2z;
        q3[0] = p[0] + d3x;
        q3[1] = p[1] + d3y;
        q3[2] = p[2] + d3z;
        dst += 4 * stride;
        src += srcStride;
    }
}
