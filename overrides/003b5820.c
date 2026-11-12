/* func_003b5820 (672 bytes): func_003b53b0 with the per-record scales of func_003b5160. arg4:
 * +4 source base, +8 source stride, +0x1C second stream base, +0x20 its stride (u0, u1 per
 * record), +0x34 angle stream base, +0x38 its stride, +0x68 and +0x6C floats c68, c6C. h1 = 0.5*u0,
 * h2 = 0.5*u1; A = pa*h1, B = pb*h2 with pa, pb the unscaled rotated vectors, and the offset
 * o = (c6C*pb) + (c68*pa). The corners are (P + o) + (-A - B), (P + o) + (A - B),
 * (P + o) + (A + B), (P + o) + (-A + B) at dst + k*S. Does nothing when arg3 <= 0. */

float func_0044b7b0(float x);
float func_0044b610(float x);

void func_003b5820(u8 *arg0, f32 *arg1, f32 *arg2, s32 arg3, u8 *arg4)
{
    u8 *dst;
    u8 *src;
    u8 *ang;
    u8 *ext;
    u32 stride;
    u32 srcStride;
    u32 angStride;
    u32 extStride;
    f32 c68;
    f32 c6C;
    s32 i;

    src = *(u8 **)(arg4 + 4);
    srcStride = *(u32 *)(arg4 + 8);
    ang = *(u8 **)(arg4 + 0x34);
    angStride = *(u32 *)(arg4 + 0x38);
    ext = *(u8 **)(arg4 + 0x1C);
    extStride = *(u32 *)(arg4 + 0x20);
    dst = *(u8 **)arg0;
    stride = *(u32 *)(arg0 + 4);
    c68 = *(f32 *)(arg4 + 0x68);
    c6C = *(f32 *)(arg4 + 0x6C);

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
        f32 ox, oy, oz;
        f32 h1, h2;

        s = func_0044b7b0(*(f32 *)ang);
        c = func_0044b610(*(f32 *)ang);
        ns = -s;
        pax = (ns * arg2[0]) + (c * arg1[0]);
        pay = (ns * arg2[1]) + (c * arg1[1]);
        paz = (ns * arg2[2]) + (c * arg1[2]);
        pbx = (c * arg2[0]) + (s * arg1[0]);
        pby = (c * arg2[1]) + (s * arg1[1]);
        pbz = (c * arg2[2]) + (s * arg1[2]);
        h1 = 0.5f * ((f32 *)ext)[0];
        h2 = 0.5f * ((f32 *)ext)[1];
        ox = (c6C * pbx) + (c68 * pax);
        oy = (c6C * pby) + (c68 * pay);
        oz = (c6C * pbz) + (c68 * paz);
        ax = pax * h1;
        ay = pay * h1;
        az = paz * h1;
        bx = pbx * h2;
        by = pby * h2;
        bz = pbz * h2;
        ang += angStride;
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
