/* func_003955a0 (1584 bytes): interpolates two animation key frames (the layout of func_003963c0: time at +4,
 * rotation quaternion (a, b, c, d) at +8, +0xC, +0x10, +0x14, translation at +0x18, +0x1C, +0x20) at the time arg3 and
 * stores the result in arg0. Returns nothing.
 *
 * u = (arg3 - arg1.time) / (arg2.time - arg1.time). The translation is arg1.t + u * (arg2.t - arg1.t). With the
 * quaternions' dot product d (negative d negates arg2's quaternion in memory, and d, first): for d at or above
 * 0.999 (the word 0x3F7FBE77) the rotation is the linear mix u * arg2.q + (1 - u) * arg1.q, otherwise the slerp
 * sin(u * t) / sin(t) * arg2.q + sin((1 - u) * t) / sin(t) * arg1.q with t = acos(d). The sine is retail's inlined
 * Taylor polynomial (x + x^3 * P(x^2), P from the coefficients 1/13! down to -1/3!, evaluated Horner-style), which
 * is not accurate beyond about pi/2; the acos is the host binding func_00448d10 (retail inlines fdlibm's acosf). */

extern f32 p4c_00448d10_003955a0(f32 x) __asm__("func_00448d10");

static f32 p4_sin_poly_003955a0(f32 x)
{
    u32 k13 = 0x2F2EC9D3;
    u32 k11 = 0xB2D72F34;
    u32 k9 = 0x3638EF1B;
    u32 k7 = 0xB9500D01;
    u32 k5 = 0x3C088889;
    u32 k3 = 0xBE2AAAAB;
    f32 x2 = x * x;
    f32 p;
    f32 x3;

    p = (0.0f + *(f32 *)&k11) + *(f32 *)&k13 * x2;
    p = (0.0f + *(f32 *)&k9) + x2 * p;
    p = (0.0f + *(f32 *)&k7) + x2 * p;
    p = (0.0f + *(f32 *)&k5) + x2 * p;
    p = (0.0f + *(f32 *)&k3) + x2 * p;
    x3 = x2 * x;
    return (0.0f + x) + x3 * p;
}

void func_003955a0(f32 *arg0, f32 *arg1, f32 *arg2, f32 arg3)
{
    f32 u;
    f32 dt;
    f32 dot;
    f32 acc;
    f32 t0;
    f32 w1;
    f32 w0;
    f32 th;
    f32 rcp;
    u32 k = 0x3F7FBE77;
    f32 limit = *(f32 *)&k;
    s32 i;

    dt = arg2[1] - arg1[1];
    u = (arg3 - arg1[1]) / dt;
    acc = arg1[3] * arg2[3];
    acc = acc + arg1[2] * arg2[2];
    acc = acc + arg1[4] * arg2[4];
    dot = acc + arg1[5] * arg2[5];

    t0 = 0.0f + arg1[6];
    arg0[6] = t0 + u * (arg2[6] - arg1[6]);
    t0 = 0.0f + arg1[7];
    arg0[7] = t0 + u * (arg2[7] - arg1[7]);
    t0 = 0.0f + arg1[8];
    arg0[8] = t0 + u * (arg2[8] - arg1[8]);

    if (dot < 0.0f) {
        dot = -dot;
        arg2[2] = -arg2[2];
        arg2[3] = -arg2[3];
        arg2[4] = -arg2[4];
        arg2[5] = -arg2[5];
    }
    w1 = 1.0f - u;
    w0 = u;
    if (dot < limit) {
        th = p4c_00448d10_003955a0(dot);
        rcp = 1.0f / p4_sin_poly_003955a0(th);
        w1 = p4_sin_poly_003955a0(w1 * th) * rcp;
        w0 = p4_sin_poly_003955a0(u * th) * rcp;
    }
    for (i = 2; i < 6; i++) {
        acc = w0 * arg2[i];
        arg0[i] = acc + w1 * arg1[i];
    }
}
