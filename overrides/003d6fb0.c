/* func_003d6fb0 (272 bytes): extracts the texture animation parameters of the matrix arg1 (16 words) into arg0 (six floats) and returns arg0.
 * With the working copy M of the matrix: arg0[0] = func_0044b950(-M[4], M[0]); func_003e0c90(M, 0x00752F88, 2); func_003e0870(M, V, 2,
 * 180 * (-arg0[0] / pi)) with V the three floats at 0x0070B5F8, 0x0070B5FC and 0x0070B600; func_003e0c90(M, 0x00752F98, 2). Then
 * arg0[1] = M[0], arg0[3] = M[1], arg0[2] = M[5], arg0[4] = M[12] and arg0[5] = M[13]. */

extern void p4c_003e0870_003d6fb0(void *m, void *v, s32 mode, f32 ang) __asm__("func_003e0870");
extern void p4c_003e0c90_003d6fb0(void *m, void *v, s32 mode) __asm__("func_003e0c90");
extern f32 p4c_0044b950_003d6fb0(f32 a, f32 b) __asm__("func_0044b950");

f32 *func_003d6fb0(f32 *arg0, f32 *arg1)
{
    f32 m[16] __attribute__((aligned(16)));
    f32 v[3] __attribute__((aligned(16)));
    f32 a;
    s32 i;

    for (i = 0; i < 16; i++) {
        m[i] = arg1[i];
    }
    v[0] = *(f32 *)0x0070B5F8;
    v[1] = *(f32 *)0x0070B5FC;
    v[2] = *(f32 *)0x0070B600;
    arg0[0] = p4c_0044b950_003d6fb0(-m[4], m[0]);
    p4c_003e0c90_003d6fb0(m, (void *)0x00752F88, 2);
    a = -arg0[0];
    p4c_003e0870_003d6fb0(m, v, 2, 180.0f * (a / 3.14159274f));
    p4c_003e0c90_003d6fb0(m, (void *)0x00752F98, 2);
    arg0[1] = m[0];
    arg0[3] = m[1];
    arg0[2] = m[5];
    arg0[4] = m[12];
    arg0[5] = m[13];
    return arg0;
}
