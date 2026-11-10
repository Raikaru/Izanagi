/* func_003954b0 (240 bytes): builds a 3x3 rotation plus translation (RwMatrix layout) from a
 * quaternion record. arg1 holds the quaternion as floats at +8, +0xC, +0x10, +0x14 (a, b, c, d;
 * d is the real part) and a translation at +0x18, +0x1C, +0x20. Writes to arg0:
 *
 *   +0x00 1 - 2(bb + cc)   +0x04 2(ab + dc)       +0x08 2(ca - db)
 *   +0x10 2(ab - dc)       +0x14 1 - 2(aa + cc)   +0x18 2(bc + da)
 *   +0x20 2(ca + db)       +0x24 2(bc - da)       +0x28 1 - 2(aa + bb)
 *   +0x0C = 3 (integer), +0x30..0x38 = translation.
 *
 * The three diagonal terms are the retail msub.s form: the accumulator is loaded with 1.0 and the
 * product 2*(sum) is subtracted. The words at +0x1C, +0x2C and +0x3C are left alone. */

void func_003954b0(u8 *arg0, u8 *arg1)
{
    f32 a = *(f32 *)(arg1 + 0x08);
    f32 b = *(f32 *)(arg1 + 0x0C);
    f32 c = *(f32 *)(arg1 + 0x10);
    f32 d = *(f32 *)(arg1 + 0x14);
    f32 aa = a * a;
    f32 bb = b * b;
    f32 cc = c * c;
    f32 bc = b * c;
    f32 ca = c * a;
    f32 ab = a * b;
    f32 dc = d * c;
    f32 da = d * a;
    f32 db = d * b;
    f32 *m = (f32 *)arg0;

    m[0x00 / 4] = 1.0f - 2.0f * (bb + cc);
    m[0x04 / 4] = 2.0f * (ab + dc);
    m[0x08 / 4] = 2.0f * (ca - db);
    m[0x10 / 4] = 2.0f * (ab - dc);
    m[0x14 / 4] = 1.0f - 2.0f * (aa + cc);
    m[0x18 / 4] = 2.0f * (bc + da);
    m[0x20 / 4] = 2.0f * (ca + db);
    m[0x24 / 4] = 2.0f * (bc - da);
    m[0x28 / 4] = 1.0f - 2.0f * (aa + bb);
    *(s32 *)(arg0 + 0x0C) = 3;
    *(u32 *)(arg0 + 0x30) = *(u32 *)(arg1 + 0x18);
    *(u32 *)(arg0 + 0x34) = *(u32 *)(arg1 + 0x1C);
    *(u32 *)(arg0 + 0x38) = *(u32 *)(arg1 + 0x20);
}
