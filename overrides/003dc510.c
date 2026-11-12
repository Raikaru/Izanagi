/* func_003dc510 (128 bytes): quaternion (x, y, z, w) of a rotation matrix arg1 (rows of four floats, element (r, c) at
 * float index 4 * r + c) when the y component dominates; writes it to arg0 and returns arg0. With t = 1 + m5 -
 * (m10 + m0) and s = 0.5 / sqrt(t): y = 0.5 * sqrt(t), x = s * (m4 + m1), z = s * (m6 + m9), w = s * (m8 - m2). Helper of RtQuatConvertFromMatrix (func_003dc610). */

f32 *func_003dc510(f32 *arg0, f32 *arg1)
{
    f32 t;
    f32 root;
    f32 s;

    t = 1.0f + (arg1[5] - (arg1[10] + arg1[0]));
    root = __builtin_sqrtf(t);
    s = 0.5f / root;
    arg0[1] = 0.5f * root;
    arg0[3] = s * (arg1[8] - arg1[2]);
    arg0[0] = s * (arg1[4] + arg1[1]);
    arg0[2] = s * (arg1[6] + arg1[9]);
    return arg0;
}
