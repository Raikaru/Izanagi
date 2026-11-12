/* func_003b4ab0 (344 bytes): writes four corner positions per record. arg0: +0 destination
 * base, +4 slot stride S. arg1 and arg2 each point at three floats (a = arg1[0..2],
 * b = arg2[0..2]); arg3 is the record count; arg4: +4 source base, +8 source stride,
 * +0x70 and +0x74 scale factors s and t. With sa = 0.5*s, sb = 0.5*t, A = a*sa and B = b*sb
 * (per component), record i at source position P (three floats at the source pointer) is
 * written to the destination as three floats P + delta at:
 *
 *   dst + 0*S : -A - B     dst + 1*S :  A - B
 *   dst + 2*S :  A + B     dst + 3*S : -A + B
 *
 * then dst advances by 4*S and the source by its stride. Does nothing when arg3 <= 0.
 * Single precision throughout. Leaf function. */

void func_003b4ab0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4)
{
    u8 *dst;
    u8 *src;
    u32 stride;
    u32 srcStride;
    f32 sa;
    f32 sb;
    f32 ax;
    f32 ay;
    f32 az;
    f32 bx;
    f32 by;
    f32 bz;
    f32 d0x, d0y, d0z;
    f32 d1x, d1y, d1z;
    f32 d2x, d2y, d2z;
    f32 d3x, d3y, d3z;
    s32 i;

    src = *(u8 **)(arg4 + 4);
    srcStride = *(u32 *)(arg4 + 8);
    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    sa = 0.5f * *(f32 *)(arg4 + 0x70);
    sb = 0.5f * *(f32 *)(arg4 + 0x74);
    az = arg1[2] * sa;
    ay = arg1[1] * sa;
    ax = arg1[0] * sa;
    bx = arg2[0] * sb;
    bz = arg2[2] * sb;
    by = arg2[1] * sb;
    d0x = -ax - bx;
    d0y = -ay - by;
    d0z = -az - bz;
    d1x = ax - bx;
    d1y = ay - by;
    d1z = az - bz;
    d2x = ax + bx;
    d2y = ay + by;
    d2z = az + bz;
    d3x = -ax + bx;
    d3y = -ay + by;
    d3z = -az + bz;

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
