/* func_003dc590 (128 bytes): quaternion (x, y, z, w) of a rotation matrix arg1 (rows of four floats, element (r, c) at
 * float index 4 * r + c) when the z component dominates; writes it to arg0 and returns arg0. With t = 1 + m10 -
 * (m0 + m5) and s = 0.5 / sqrt(t): z = 0.5 * sqrt(t), x = s * (m8 + m2), y = s * (m9 + m6), w = s * (m1 - m4). Helper of RtQuatConvertFromMatrix (func_003dc610). */

f32 *func_003dc590(f32 *arg0, f32 *arg1)
{
    f32 t;
    f32 root;
    f32 s;

    t = 1.0f + (arg1[10] - (arg1[0] + arg1[5]));
    root = __builtin_sqrtf(t);
    s = 0.5f / root;
    arg0[2] = 0.5f * root;
    arg0[3] = s * (arg1[1] - arg1[4]);
    arg0[0] = s * (arg1[8] + arg1[2]);
    arg0[1] = s * (arg1[9] + arg1[6]);
    return arg0;
}
