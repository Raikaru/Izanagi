/* func_003dc490 (128 bytes): quaternion (x, y, z, w) of a rotation matrix arg1 (rows of four floats, element (r, c) at
 * float index 4 * r + c) when the x component dominates; writes it to arg0 and returns arg0. With t = 1 + m0 -
 * (m5 + m10) and s = 0.5 / sqrt(t): x = 0.5 * sqrt(t), y = s * (m1 + m4), z = s * (m2 + m8), w = s * (m6 - m9). Helper of RtQuatConvertFromMatrix (func_003dc610). */

f32 *func_003dc490(f32 *arg0, f32 *arg1)
{
    f32 t;
    f32 root;
    f32 s;

    t = 1.0f + (arg1[0] - (arg1[5] + arg1[10]));
    root = __builtin_sqrtf(t);
    s = 0.5f / root;
    arg0[0] = 0.5f * root;
    arg0[3] = s * (arg1[6] - arg1[9]);
    arg0[1] = s * (arg1[1] + arg1[4]);
    arg0[2] = s * (arg1[2] + arg1[8]);
    return arg0;
}
