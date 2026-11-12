/* func_003dc740 (1024 bytes): turns the quaternion arg0 (x, y, z, w) into the rotation of arg3 degrees about the axis arg1 (normalised with
 * RwV3dNormalize) and returns arg0 (0 after reporting an error). Mode arg2: 0 replaces the quaternion, 1 multiplies the old quaternion
 * q by the new rotation r on the right (q * r), 2 on the left (r * q); the products are accumulated in the order of the FPU's accumulator
 * (adda / madd). With the half angle h = arg3 * 0.0087266 (bits 0x3C0EFA35, pi / 360) the rotation is (sin h * axis, cos h), sin / cos being func_0044b7b0 / func_0044b610. A null quaternion or
 * axis reports {0x195, func_003df590(0x80000016)} and any other mode {0x195, func_003df590(0x80000003, 0x0070B6C0)} to func_003df4d0. */

extern void p4c_003df4d0_003dc740(s32 *err) __asm__("func_003df4d0");
extern s32 p4c_003df590_003dc740(s32 code, ...) __asm__("func_003df590");
/* RwV3dNormalize returns a float (its reciprocal length, in st(0) on the host): the result must be popped even though it is unused. */
extern f32 p4c_003e40b0_003dc740(f32 *dst, u32 *src) __asm__("RwV3dNormalize");

float func_0044b7b0(float x);
float func_0044b610(float x);

/* The half angle h = (pi / 360) * angle is a multiply that rounds toward zero (the EE float mode, include/p4pc/hostmath.h): for a turn of exactly
 * 180 degrees pi / 2 comes out one ulp lower (0x3FC90FDA against 0x3FC90FDB), and the cosine that becomes the quaternion's w is +7.5e-8 instead of
 * -4.4e-8. The sign matters: the unit's facing quaternion (0, 1, 0, w) rotates a forward vector to x = 2 y w, atan2 of which is +180 degrees for
 * w > 0 and -180 for w < 0, and btlUnitUpdateFollowPath compares it with the +180 of the target direction; with the wrong sign the remaining turn is
 * 360 degrees minus a hair, the "turn toward" packet never ends and the boss battle (Shadow Yosuke) never opens its command menu. The retail units
 * hold w = +1.9e-7. */

/* The callers declare (quat, axis, angle, mode): the host's argument order is the declared order, and the integer registers a2 (mode) and f12 (angle)
 * are different classes on the EE, so the float stays in the third position here. */
u32 *func_003dc740(u32 *arg0, u32 *arg1, f32 arg3, s32 arg2)
{
    f32 *qq = (f32 *)arg0;
    f32 q[4];
    f32 r[4];
    f32 h;
    f32 s;
    f32 c;
    f32 a;
    f32 w;
    s32 err[2];
    u32 k = 0x3C0EFA35;

    if (arg0 == (u32 *)0 || arg1 == (u32 *)0) {
        err[0] = 0x195;
        err[1] = p4c_003df590_003dc740(0x80000016);
        p4c_003df4d0_003dc740(err);
        return (u32 *)0;
    }
    if (arg2 == 2) {
        h = *(f32 *)&k * arg3;
        q[1] = qq[1];
        q[0] = qq[0];
        q[2] = qq[2];
        q[3] = qq[3];
        s = func_0044b7b0(h);
        c = func_0044b610(h);
        r[3] = c;
        p4c_003e40b0_003dc740(r, arg1);
        r[1] = r[1] * s;
        r[0] = r[0] * s;
        r[2] = r[2] * s;
        a = r[1] * q[1];
        w = r[3] * q[3];
        a = a + r[0] * q[0];
        a = a + r[2] * q[2];
        qq[3] = w - a;
        qq[0] = r[1] * q[2] - r[2] * q[1];
        qq[1] = r[2] * q[0] - r[0] * q[2];
        qq[2] = r[0] * q[1] - r[1] * q[0];
        qq[0] = (0.0f + qq[0]) + q[0] * r[3];
        qq[1] = (0.0f + qq[1]) + q[1] * r[3];
        qq[2] = (0.0f + qq[2]) + q[2] * r[3];
        qq[0] = (0.0f + qq[0]) + r[0] * q[3];
        qq[1] = (0.0f + qq[1]) + r[1] * q[3];
        qq[2] = (0.0f + qq[2]) + r[2] * q[3];
        return arg0;
    }
    if (arg2 == 1) {
        h = *(f32 *)&k * arg3;
        q[1] = qq[1];
        q[0] = qq[0];
        q[2] = qq[2];
        q[3] = qq[3];
        s = func_0044b7b0(h);
        c = func_0044b610(h);
        r[3] = c;
        p4c_003e40b0_003dc740(r, arg1);
        r[1] = r[1] * s;
        r[0] = r[0] * s;
        r[2] = r[2] * s;
        a = q[1] * r[1];
        w = q[3] * r[3];
        a = a + q[0] * r[0];
        a = a + q[2] * r[2];
        qq[3] = w - a;
        qq[0] = q[1] * r[2] - q[2] * r[1];
        qq[1] = q[2] * r[0] - q[0] * r[2];
        qq[2] = q[0] * r[1] - q[1] * r[0];
        qq[0] = (0.0f + qq[0]) + r[0] * q[3];
        qq[1] = (0.0f + qq[1]) + r[1] * q[3];
        qq[2] = (0.0f + qq[2]) + r[2] * q[3];
        qq[0] = (0.0f + qq[0]) + q[0] * r[3];
        qq[1] = (0.0f + qq[1]) + q[1] * r[3];
        qq[2] = (0.0f + qq[2]) + q[2] * r[3];
        return arg0;
    }
    if (arg2 == 0) {
        h = *(f32 *)&k * arg3;
        s = func_0044b7b0(h);
        c = func_0044b610(h);
        qq[3] = c;
        p4c_003e40b0_003dc740(qq, arg1);
        qq[0] = qq[0] * s;
        qq[1] = qq[1] * s;
        qq[2] = qq[2] * s;
        return arg0;
    }
    err[0] = 0x195;
    err[1] = p4c_003df590_003dc740(0x80000003, 0x0070B6C0);
    p4c_003df4d0_003dc740(err);
    return (u32 *)0;
}
