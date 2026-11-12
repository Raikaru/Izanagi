/* func_003b8e50 (spline plugin, module 0x102 open: basis tables): allocates 0x3030 bytes and
 * splits them into twelve 257-entry float tables (0x404 bytes each), whose pointers are kept in
 * the twelve retail gp globals at 0x0076470C + 4k. Entry i of each table is filled for
 * t = i / 256:
 *   cubic uniform B-spline weights   0: t^3/6,  1: (1 + 3t + 3t^2 - 3t^3)/6,
 *                                    2: (4 + 3t^3 - 6t^2)/6,  3: (1 - 3t + 3t^2 - t^3)/6
 *   quadratic weights                4: t^2/2,  5: (1 + 2t - 3t^2)/2,
 *                                    6: (3t - 4)t/2,  7: ((2 - t)t - 1)/2
 *   linear weights                   8: t,  9: 1 - 3t,  10: 3t - 2,  11: 1 - t
 * An allocation failure reports error 0x80000013 for module 0x102 and returns 0; success
 * returns 1.
 *
 * Direct transliteration of the retail code (EE madd.s/adda.s accumulator chains written as the
 * float expressions they compute, in the same operation order). */

typedef void *(*P4SplineAllocFn)(u32 size, u32 flags);
typedef s32 (*P4SplineErrFn)(s32 code, ...);

s32 func_003b8e50(void)
{
    u32 *tab = (u32 *)0x0076470C;
    u8 *base = (u8 *)(*(P4SplineAllocFn *)0x008873E8)(0x3030, 0x40102);
    s32 i;

    tab[0] = (u32)base;
    if (base == NULL) {
        s32 err[2];

        err[0] = 258;
        err[1] = ((P4SplineErrFn)func_003df590)((s32)0x80000013, 0x3030);
        func_003df4d0(err);
        return tab[0] != 0;
    }

    for (i = 1; i < 12; i++)
        tab[i] = tab[0] + 0x404 * i;

    for (i = 0; i < 257; i++) {
        f32 t = (f32)i / 256.0f;
        f32 t2 = t * t;
        f32 m3 = t * -3.0f;
        f32 p3 = t * 3.0f;
        f32 m1 = t * -1.0f;

        ((f32 *)tab[0])[i] = (t * t2) / 6.0f;
        ((f32 *)tab[1])[i] = (t * ((3.0f + m3) * t + 3.0f) + 1.0f) / 6.0f;
        ((f32 *)tab[2])[i] = (t * (p3 - 6.0f) * t + 4.0f) / 6.0f;
        ((f32 *)tab[3])[i] = (t * ((3.0f + m1) * t - 3.0f) + 1.0f) / 6.0f;
        ((f32 *)tab[4])[i] = t2 / 2.0f;
        ((f32 *)tab[5])[i] = ((2.0f + m3) * t + 1.0f) / 2.0f;
        ((f32 *)tab[6])[i] = ((-4.0f + p3) * t) / 2.0f;
        ((f32 *)tab[7])[i] = ((2.0f + m1) * t - 1.0f) / 2.0f;
        ((f32 *)tab[8])[i] = t;
        ((f32 *)tab[9])[i] = 1.0f + m3;
        ((f32 *)tab[10])[i] = p3 - 2.0f;
        ((f32 *)tab[11])[i] = 1.0f + m1;
    }
    return tab[0] != 0;
}
