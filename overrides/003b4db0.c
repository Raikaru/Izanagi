/* func_003b4db0 (568 bytes): like func_003b4ff0, with the half-extents rotated by a per-record
 * angle. arg0: +0 destination base, +4 slot stride S. arg1 and arg2 point at three floats each
 * (a, b); arg3 is the record count; arg4: +4 source base, +8 source stride (positions P), +0x34
 * angle stream base, +0x38 its stride, +0x70 and +0x74 scale factors. With s = sin(angle),
 * c = cos(angle), sa = 0.5*ctx[0x70] and sb = 0.5*ctx[0x74], per component:
 *
 *   A = ((-s*b) + (c*a)) * sa        B = ((c*b) + (s*a)) * sb
 *
 * and three floats P + delta are written at dst + 0*S: -A - B, dst + 1*S: A - B,
 * dst + 2*S: A + B, dst + 3*S: -A + B. Then dst advances by 4*S, and the source and angle
 * streams by their strides. sin and cos are func_0044b7b0 and func_0044b610. Does nothing when
 * arg3 <= 0. */

float func_0044b7b0(float x);
float func_0044b610(float x);

void func_003b4db0(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4)
{
    u8 *dst;
    u8 *src;
    u8 *ang;

    u32 stride;
    u32 srcStride;
    u32 angStride;

    f32 sa;
    f32 sb;
    s32 i;

    src = *(u8 **)(arg4 + 4);
    srcStride = *(u32 *)(arg4 + 8);
    ang = *(u8 **)(arg4 + 0x34);
    angStride = *(u32 *)(arg4 + 0x38);

    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);

    sa = 0.5f * *(f32 *)(arg4 + 0x70);
    sb = 0.5f * *(f32 *)(arg4 + 0x74);

    for (i = 0; i < arg3; i++) {
        f32 s;
        f32 c;
        f32 *p = (f32 *)src;
        f32 *q0 = (f32 *)dst;
        f32 *q1 = (f32 *)(dst + stride);
        f32 *q2 = (f32 *)(dst + 2 * stride);
        f32 *q3 = (f32 *)(dst + 3 * stride);
        f32 ns;
        f32 pax, pay, paz;
        f32 pbx, pby, pbz;
        f32 ax, ay, az;
        f32 bx, by, bz;

        s = func_0044b7b0(*(f32 *)ang);
        c = func_0044b610(*(f32 *)ang);
        ns = -s;
        pax = (ns * arg2[0]) + (c * arg1[0]);
        pay = (ns * arg2[1]) + (c * arg1[1]);
        paz = (ns * arg2[2]) + (c * arg1[2]);
        pbx = (c * arg2[0]) + (s * arg1[0]);
        pby = (c * arg2[1]) + (s * arg1[1]);
        pbz = (c * arg2[2]) + (s * arg1[2]);

        ax = pax * sa;
        ay = pay * sa;
        az = paz * sa;
        bx = pbx * sb;
        by = pby * sb;
        bz = pbz * sb;
        ang += angStride;

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
