/* func_003b5650 (464 bytes): like func_003b4ff0, with an offset added to the position first.
 * arg0: +0 destination base, +4 slot stride S. arg1 and arg2 point at three floats each
 * (a, b); arg3 is the record count; arg4: +4 source base, +8 source stride (positions P),
 * +0x1C second stream base, +0x20 second stream stride (two floats u0, u1 per record), +0x68
 * and +0x6C floats c68, c6C. The offset is o = c6C*b + c68*a (per component; the retail code
 * forms it with mula.s/madd.s, a product rounded and then summed). For each record,
 * A = a*(0.5*u0) and B = b*(0.5*u1) per component, and three floats (P + o) + delta are
 * written at:
 *
 *   dst + 0*S : -A - B     dst + 1*S :  A - B
 *   dst + 2*S :  A + B     dst + 3*S : -A + B
 *
 * then dst advances by 4*S and both streams by their strides. Does nothing when
 * arg3 <= 0. Leaf function. */

void func_003b5650(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4)
{
    u8 *dst;
    u8 *src;
    u8 *ext;
    u32 stride;
    u32 srcStride;
    u32 extStride;
    f32 c68;
    f32 c6C;
    f32 ox;
    f32 oy;
    f32 oz;
    s32 i;

    src = *(u8 **)(arg4 + 4);
    srcStride = *(u32 *)(arg4 + 8);
    ext = *(u8 **)(arg4 + 0x1C);
    extStride = *(u32 *)(arg4 + 0x20);
    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    c68 = *(f32 *)(arg4 + 0x68);
    c6C = *(f32 *)(arg4 + 0x6C);
    ox = c6C * arg2[0] + c68 * arg1[0];
    oy = c6C * arg2[1] + c68 * arg1[1];
    oz = c6C * arg2[2] + c68 * arg1[2];

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
        q0[0] = (p[0] + ox) + ((-ax) - bx);
        q0[1] = (p[1] + oy) + ((-ay) - by);
        q0[2] = (p[2] + oz) + ((-az) - bz);
        q1[0] = (p[0] + ox) + (ax - bx);
        q1[1] = (p[1] + oy) + (ay - by);
        q1[2] = (p[2] + oz) + (az - bz);
        q2[0] = (p[0] + ox) + (ax + bx);
        q2[1] = (p[1] + oy) + (ay + by);
        q2[2] = (p[2] + oz) + (az + bz);
        q3[0] = (p[0] + ox) + ((-ax) + bx);
        q3[1] = (p[1] + oy) + ((-ay) + by);
        q3[2] = (p[2] + oz) + ((-az) + bz);
        dst += 4 * stride;
        src += srcStride;
    }
}
