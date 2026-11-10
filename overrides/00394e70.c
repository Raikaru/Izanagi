/* func_00394e70 (352 bytes): as func_00394d70 but for the second hit-test family; returns arg0. Dispatch on the
 * shape type word at arg1 + 0x18. Type 5: the sphere (x, y, z, r) of func_003bfae0(*(arg1 + 0)) is copied to a local
 * and func_00394c70(arg0, &sphere, arg1, arg2, arg3) is called. Type 3: func_00394c70(arg0, arg1, arg1, arg2, arg3).
 * Type 1: with e = *(arg0 + 0x18) the matrix from func_003e9700(*(arg0 + 4)) goes through func_003e0960(&m, ...) and
 * func_003e42e0(&t, arg1, 2, &m) builds a transformed copy t; then, with *(e + 0x18) below 2
 * func_00392bf0(e, &t, arg1, arg2, arg3) runs, otherwise func_003941e0(e, arg0 + 0x4C, &t, arg1, arg2, arg3). Any other
 * type does nothing. */

extern void p4c_00392bf0_00394e70(u8 *e, void *t, u8 *a, u32 b, u32 c) __asm__("func_00392bf0");
extern void p4c_003941e0_00394e70(u8 *e, u8 *p, void *t, u8 *a, u32 b, u32 c) __asm__("func_003941e0");
extern void p4c_00394c70_00394e70(u8 *a, void *b, u8 *c, u32 d, u32 e) __asm__("func_00394c70");
extern u8 *p4c_003bfae0_00394e70(u32 o) __asm__("func_003bfae0");
extern void p4c_003e0960_00394e70(void *m, void *src) __asm__("func_003e0960");
extern void p4c_003e42e0_00394e70(void *t, u8 *a, s32 n, void *m) __asm__("func_003e42e0");
extern u8 *p4c_003e9700_00394e70(u32 o) __asm__("func_003e9700");

u32 func_00394e70(u8 *arg0, u8 *arg1, u32 arg2, u32 arg3)
{
    f32 *s;
    f32 sphere[4] __attribute__((aligned(16)));
    u8 *e;
    u8 t[32];
    u8 m[64] __attribute__((aligned(16)));

    switch (*(s32 *)(arg1 + 0x18)) {
    case 5:
        s = (f32 *)p4c_003bfae0_00394e70(*(u32 *)arg1);
        sphere[0] = s[0];
        sphere[1] = s[1];
        sphere[2] = s[2];
        sphere[3] = s[3];
        p4c_00394c70_00394e70(arg0, sphere, arg1, arg2, arg3);
        break;
    case 3:
        p4c_00394c70_00394e70(arg0, arg1, arg1, arg2, arg3);
        break;
    case 1:
        e = *(u8 **)(arg0 + 0x18);
        p4c_003e0960_00394e70(m, p4c_003e9700_00394e70(*(u32 *)(arg0 + 4)));
        p4c_003e42e0_00394e70(t, arg1, 2, m);
        if (*(s32 *)(e + 0x18) < 2) {
            p4c_00392bf0_00394e70(e, t, arg1, arg2, arg3);
        } else {
            p4c_003941e0_00394e70(e, arg0 + 0x4C, t, arg1, arg2, arg3);
        }
        break;
    default:
        break;
    }
    return (u32)arg0;
}
