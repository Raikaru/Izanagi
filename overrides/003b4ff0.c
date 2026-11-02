/* func_003b4ff0 (360 bytes): like func_003b4ab0, but the two half-extents come from a second
 * per-record stream. arg0: +0 destination base, +4 slot stride S. arg1 and arg2 point at
 * three floats each (a, b); arg3 is the record count; arg4: +4 source base, +8 source stride
 * (positions), +0x1C second stream base, +0x20 second stream stride (two floats u0, u1 per
 * record). For each record, A = a*(0.5*u0) and B = b*(0.5*u1) per component, and the position
 * P is written as three floats P + delta at:
 *
 *   dst + 0*S : -A - B     dst + 1*S :  A - B
 *   dst + 2*S :  A + B     dst + 3*S : -A + B
 *
 * then dst advances by 4*S and both streams by their strides. Does nothing when
 * arg3 <= 0. Leaf function. */

void func_003b4ff0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4)
{
    u8 *dst;
    u8 *src;
    u8 *ext;
    u32 stride;
    u32 srcStride;
    u32 extStride;
    s32 i;

    src = *(u8 **)(arg4 + 4);
    srcStride = *(u32 *)(arg4 + 8);
    ext = *(u8 **)(arg4 + 0x1C);
    extStride = *(u32 *)(arg4 + 0x20);
    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);

    for (i = 0; i < arg3; i++) {
        f32 *p = (f32 *)src;
        f32 *q0 = (f32 *)dst;
        f32 *q1 = (f32 *)(dst + stride);
        f32 *q2 = (f32 *)(dst + 2 * stride);
        f32 *q3 = (f32 *)(dst + 3 * stride);
        f32 h1 = 0.5f * ((f32 *)ext)[0];
        f32 h2 = 0.5f * ((f32 *)ext)[1];
        f32 ax = arg1[0] * h1;
        f32 ay = arg1[1] * h1;
        f32 az = arg1[2] * h1;
        f32 bx = arg2[0] * h2;
        f32 by = arg2[1] * h2;
        f32 bz = arg2[2] * h2;

        ext += extStride;
        q0[0] = p[0] + ((-ax) - bx);
        q0[1] = p[1] + ((-ay) - by);
        q0[2] = p[2] + ((-az) - bz);
        q1[0] = p[0] + (ax - bx);
        q1[1] = p[1] + (ay - by);
        q1[2] = p[2] + (az - bz);
        q2[0] = p[0] + (ax + bx);
        q2[1] = p[1] + (ay + by);
        q2[2] = p[2] + (az + bz);
        q3[0] = p[0] + ((-ax) + bx);
        q3[1] = p[1] + ((-ay) + by);
        q3[2] = p[2] + ((-az) + bz);
        dst += 4 * stride;
        src += srcStride;
    }
}
