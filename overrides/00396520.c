/* func_00396520 (340 bytes): keyframe combine for the animation blender. arg0 = arg1 * arg2 for key frames whose
 * rotation quaternion (a, b, c, d) sits at +8, +0xC, +0x10, +0x14 and whose translation sits at +0x18, +0x1C, +0x20:
 * the scalar part is x.d * y.d - dot(x, y), the vector part cross(x, y) + y * x.d + x * y.d (x = arg1.q, y = arg2.q;
 * cross(x, y) = (x.b*y.c - x.c*y.b, x.c*y.a - x.a*y.c, x.a*y.b - x.b*y.a) with the second product added to the
 * first, in that order), and the translation is arg1's plus arg2's. Returns nothing. */

void func_00396520(f32 *arg0, f32 *arg1, f32 *arg2)
{
    f32 xa = arg1[2];
    f32 xb = arg1[3];
    f32 xc = arg1[4];
    f32 xd = arg1[5];
    f32 ya = arg2[2];
    f32 yb = arg2[3];
    f32 yc = arg2[4];
    f32 yd = arg2[5];
    f32 acc;
    f32 t;

    acc = xb * yb;
    acc = acc + xa * ya;
    acc = acc + xc * yc;
    arg0[5] = xd * yd - acc;

    acc = xb * yc;
    arg0[2] = acc - xc * yb;
    acc = xc * ya;
    arg0[3] = acc - xa * yc;
    acc = xa * yb;
    arg0[4] = acc - xb * ya;

    t = 0.0f + arg0[2];
    arg0[2] = t + ya * xd;
    t = 0.0f + arg0[3];
    arg0[3] = t + yb * xd;
    t = 0.0f + arg0[4];
    arg0[4] = t + yc * xd;
    t = 0.0f + arg0[2];
    arg0[2] = t + xa * yd;
    t = 0.0f + arg0[3];
    arg0[3] = t + xb * yd;
    t = 0.0f + arg0[4];
    arg0[4] = t + xc * yd;

    arg0[6] = arg1[6] + arg2[6];
    arg0[7] = arg1[7] + arg2[7];
    arg0[8] = arg1[8] + arg2[8];
}
