/* RtQuatTransformVectors (0x003dcb40, 300 bytes): rotates arg2 vectors (12 bytes each, from arg1) by the
 * quaternion arg3 (x, y, z at +0, +4, +8, w at +0xC) into arg0. Returns arg0.
 *
 * With d = 2 (q.v), k = w*w - (y*y + x*x + z*z), and c = 2w * (q x v) as the retail code forms it:
 * out = (c + q * d) + v * k per component, single precision, in the order of the EE multiply-accumulate
 * sequences (each product rounded before it is added). */

f32 *func_003dcb40(f32 *arg0, f32 *arg1, s32 arg2, f32 *arg3)
{
    f32 x = arg3[0];
    f32 y = arg3[1];
    f32 z = arg3[2];
    f32 w = arg3[3];
    f32 k;
    f32 tx = x + x;
    f32 ty = y + y;
    f32 tz = z + z;
    f32 xw = tx * w;
    f32 yw = ty * w;
    f32 zw = tz * w;
    f32 *out = arg0;
    s32 i;

    k = w * w;
    {
        f32 s = y * y;

        s = s + x * x;
        s = s + z * z;
        k = k - s;
    }
    for (i = 0; i < arg2; i++) {
        f32 vx = arg1[0];
        f32 vy = arg1[1];
        f32 vz = arg1[2];
        f32 d;
        f32 c0;
        f32 c1;
        f32 c2;
        f32 a;

        a = ty * vy;
        a = a + tx * vx;
        d = a + tz * vz;
        c0 = yw * vz - zw * vy;
        c1 = zw * vx - xw * vz;
        c2 = xw * vy - yw * vx;
        arg1 += 3;
        out[0] = c0;
        out[1] = c1;
        out[2] = c2;
        out[0] = (out[0] + x * d) + vx * k;
        out[1] = (out[1] + y * d) + vy * k;
        out[2] = (out[2] + z * d) + vz * k;
        out += 3;
    }
    return arg0;
}
