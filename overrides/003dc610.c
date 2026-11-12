/* RtQuatConvertFromMatrix (0x003dc610, 292 bytes): arg0 receives the quaternion (x, y, z at +0, +4, +8,
 * w at +0xC) of the rotation matrix arg1 (rows of four floats; the diagonal is +0, +0x14, +0x28). Returns 1, or 0
 * when either pointer is null.
 *
 * With trace = m00 + m11 + m22 above 0: w = 0.5 * sqrt(1 + trace), s = 0.5 / sqrt(1 + trace),
 * x = s * (m12 - m21), y = s * (m20 - m02), z = s * (m01 - m10), with the matrix words m12 at +0x18, m21 at +0x24,
 * m20 at +0x20, m02 at +8, m01 at +4 and m10 at +0x10. Otherwise the largest diagonal entry picks one of the retail
 * helpers func_003dc490, func_003dc510 or func_003dc590 (all called as helper(arg0, arg1)). */

extern void p4c_003dc490_003dc610(f32 *q, f32 *m) __asm__("func_003dc490");
extern void p4c_003dc510_003dc610(f32 *q, f32 *m) __asm__("func_003dc510");
extern void p4c_003dc590_003dc610(f32 *q, f32 *m) __asm__("func_003dc590");

s32 func_003dc610(f32 *arg0, f32 *arg1)
{
    f32 m00;
    f32 m11;
    f32 m22;
    f32 trace;
    f32 r;
    f32 s;

    if (arg0 == (f32 *)0 || arg1 == (f32 *)0) {
        return 0;
    }
    m00 = arg1[0];
    m11 = arg1[5];
    m22 = arg1[10];
    trace = m22 + (m00 + m11);
    if (trace <= 0.0f) {
        if (m00 <= m11) {
            if (m11 <= m22) {
                p4c_003dc590_003dc610(arg0, arg1);
            } else {
                p4c_003dc510_003dc610(arg0, arg1);
            }
        } else if (m00 <= m22) {
            p4c_003dc590_003dc610(arg0, arg1);
        } else {
            p4c_003dc490_003dc610(arg0, arg1);
        }
        return 1;
    }
    r = __builtin_sqrtf(1.0f + trace);
    s = 0.5f / r;
    arg0[3] = 0.5f * r;
    arg0[0] = s * (arg1[6] - arg1[9]);
    arg0[1] = s * (arg1[8] - arg1[2]);
    arg0[2] = s * (arg1[1] - arg1[4]);
    return 1;
}
