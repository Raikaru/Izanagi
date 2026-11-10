/* func_00394d70 (256 bytes): dispatches a hit test by the shape type word at arg1 + 0x18 and returns arg0 (the
 * result record). Type 1: func_00390680(arg0, arg1, arg2, arg3); type 4: func_00391400(arg0, arg1, arg2, arg3);
 * type 3: func_00391c10(arg0, arg1, arg1, arg2, arg3); type 5: the sphere (x, y, z, r) returned by
 * func_003bfae0(*(arg1 + 0)) is copied to a local and func_00391c10(arg0, &sphere, arg1, arg2, arg3) is called; any
 * other type does nothing. */

extern void p4c_00390680_00394d70(u32 a, u8 *b, u32 c, u32 d) __asm__("func_00390680");
extern void p4c_00391400_00394d70(u32 a, u8 *b, u32 c, u32 d) __asm__("func_00391400");
extern void p4c_00391c10_00394d70(u32 a, void *b, void *c, u32 d, u32 e) __asm__("func_00391c10");
extern u8 *p4c_003bfae0_00394d70(u32 o) __asm__("func_003bfae0");

u32 func_00394d70(u32 arg0, u8 *arg1, u32 arg2, u32 arg3)
{
    f32 *s;
    f32 sphere[4];

    switch (*(s32 *)(arg1 + 0x18)) {
    case 5:
        s = (f32 *)p4c_003bfae0_00394d70(*(u32 *)arg1);
        sphere[0] = s[0];
        sphere[1] = s[1];
        sphere[2] = s[2];
        sphere[3] = s[3];
        p4c_00391c10_00394d70(arg0, sphere, arg1, arg2, arg3);
        break;
    case 4:
        p4c_00391400_00394d70(arg0, arg1, arg2, arg3);
        break;
    case 3:
        p4c_00391c10_00394d70(arg0, arg1, arg1, arg2, arg3);
        break;
    case 1:
        p4c_00390680_00394d70(arg0, arg1, arg2, arg3);
        break;
    default:
        break;
    }
    return arg0;
}
