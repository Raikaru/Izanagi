/* func_003e0680 (RwMatrixRotate with a precomputed angle): builds the rotation about the unit
 * vector `axis` from t = oneMinusCosine and s = sine and combines it into the 64-byte matrix
 * like func_003e0e20 (mode 0 replace, 1 rotation * matrix, 2 matrix * rotation; any other
 * mode reports {plugin 1, "Invalid combination type"} and returns 0).
 *   m[0] = 1 - (1 - x*x)*t   m[1] = x*y*t + z*s   m[2] = z*x*t - y*s
 *   m[4] = x*y*t - z*s       m[5] = 1 - (1 - y*y)*t   m[6] = y*z*t + x*s
 *   m[8] = z*x*t + y*s       m[9] = y*z*t - x*s       m[10] = 1 - (1 - z*z)*t
 *   m[3] = 3 (flags word), m[12..14] = 0.
 * Words 7, 11 and 15 of the rotation are never written by retail (stack residue); they are 0
 * here. The return value is the matrix. */
u8 *func_003e0680(u8 *matrix, const u8 *axis, f32 oneMinusCosine, f32 sine, s32 combineOp)
{
    u32 rot[16] __attribute__((aligned(16)));
    u32 tmp[16] __attribute__((aligned(16)));
    f32 *m = (f32 *)rot;
    f32 x = ((const f32 *)axis)[0];
    f32 y = ((const f32 *)axis)[1];
    f32 z = ((const f32 *)axis)[2];
    f32 t = oneMinusCosine;
    f32 s = sine;
    f32 yz;
    f32 zx;
    f32 xy;
    u32 *src;
    u32 *dst;
    s32 err[2];
    s32 i;

    for (i = 0; i < 16; i++) {
        rot[i] = 0;
    }
    rot[3] = 3;
    m[0] = 1.0f - (1.0f - x * x) * t;
    m[5] = 1.0f - (1.0f - y * y) * t;
    m[10] = 1.0f - (1.0f - z * z) * t;
    yz = y * z * t;
    zx = z * x * t;
    xy = x * y * t;
    m[1] = xy + z * s;
    m[4] = xy - z * s;
    m[2] = zx - y * s;
    m[8] = zx + y * s;
    m[6] = yz + x * s;
    m[9] = yz - x * s;

    if (combineOp == 2) {
        RwMatrixMultiply((u8 *)tmp, matrix, (u8 *)rot);
        src = tmp;
    } else if (combineOp == 1) {
        RwMatrixMultiply((u8 *)tmp, (u8 *)rot, matrix);
        src = tmp;
    } else if (combineOp == 0) {
        src = rot;
    } else {
        err[0] = 1;
        err[1] = func_003df590(0x80000003, (char *)0x0070B6F0);
        func_003df4d0(err);
        return (u8 *)0;
    }
    dst = (u32 *)matrix;
    for (i = 0; i < 16; i++) {
        dst[i] = src[i];
    }
    return matrix;
}
