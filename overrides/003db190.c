/* func_003db190 (464 bytes): accumulates a texel-density statistic for one triangle. arg0 is an array of 2-D points (8 bytes each),
 * arg1 an array of 3-D points (12 bytes each), arg2/arg3/arg4 the indices of the triangle's corners and arg5 a record whose word +8 points
 * to a structure with width (+0xC) and height (+0x10). The texture-space area is |0.5 * cross| of the three 2-D points times
 * width * height; the model-space area is half the length of the normal (func_003e4180 of the cross product of the 3-D edges). When the model
 * area is positive and the texture area is above 1, the record gets +0xC += sqrt(|modelArea * record[+0x14] / textureArea|) and the counter
 * at +0x10 is incremented. The float products are kept unfused, as on the FPU's accumulator. */

extern f32 p4c_003e4180_003db190(f32 *v) __asm__("func_003e4180");

void func_003db190(f32 *arg0, f32 *arg1, s32 arg2, s32 arg3, s32 arg4, u8 *arg5)
{
    f32 *pi = arg0 + arg2 * 2;
    f32 *pj = arg0 + arg3 * 2;
    f32 *pk = arg0 + arg4 * 2;
    f32 *a = arg1 + arg2 * 3;
    f32 *b = arg1 + arg3 * 3;
    f32 *c = arg1 + arg4 * 3;
    u32 *tex = *(u32 **)(*(u32 **)(arg5 + 8));
    f32 acc;
    f32 cross;
    f32 texArea;
    f32 n[3];
    f32 e1x, e1y, e1z, e2x, e2y, e2z;
    f32 half;
    f32 v;

    acc = pi[0] * pj[1];
    acc = acc - pj[0] * pi[1];
    acc = acc + pj[0] * pk[1];
    acc = acc - pk[0] * pj[1];
    acc = acc + pk[0] * pi[1];
    cross = acc - pi[0] * pk[1];
    texArea = __builtin_fabsf(0.5f * cross) * (f32)(s32)(tex[3] * tex[4]);
    e1x = b[0] - a[0];
    e1y = b[1] - a[1];
    e1z = b[2] - a[2];
    e2x = c[0] - a[0];
    e2y = c[1] - a[1];
    e2z = c[2] - a[2];
    acc = e1y * e2z;
    n[0] = acc - e1z * e2y;
    acc = e1z * e2x;
    n[1] = acc - e1x * e2z;
    acc = e1x * e2y;
    n[2] = acc - e1y * e2x;
    half = 0.5f * p4c_003e4180_003db190(n);
    if (half <= 0.0f) {
        return;
    }
    if (texArea <= 1.0f) {
        return;
    }
    v = half * *(f32 *)(arg5 + 0x14);
    v = v / texArea;
    v = __builtin_sqrtf(__builtin_fabsf(v));
    *(f32 *)(arg5 + 0xC) = *(f32 *)(arg5 + 0xC) + v;
    *(u32 *)(arg5 + 0x10) = *(u32 *)(arg5 + 0x10) + 1;
}
