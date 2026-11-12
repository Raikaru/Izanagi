#include "p4pc/vu0.h"

/* Retail has no null test on the child's primitive (+0x90): without one it reads the halfword at address 4 and the word at address 8 of the EE
 * kernel's low memory and calls the kind-0 slot with the word (golden replay: the recorded children with a null primitive match an interpreted
 * retail run whose low memory is zero). The port does not map address 0, so a null primitive reads this zero record instead. */
void mdlEffectSetChildPositions(u8 *arg0, f32 *arg1)
{
    static const u8 p4_low_memory[16];
    u_long128 sp50[4];
    u_long128 sp40;
    u_long128 sp30;
    f32 var_21;
    f32 var_20;
    s32 flags;
    u8 *temp_4;
    s32 (*temp_2)(s32, void *, u8 *);

    p4_vu0_lqc2(10, arg1);
    p4_vu0_sqc2(arg0 + 0x40, 10);
    p4_vu0_lqc2(11, arg0);
    p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    p4_vu0_sqc2(&sp40, 10);
    p4_vu0_lqc2(10, arg0 + 0x50);
    effMiscBuildRotationVU();
    p4_vu0_sqc2(&sp50[0], 28);
    p4_vu0_sqc2(&sp50[1], 29);
    p4_vu0_sqc2(&sp50[2], 30);
    p4_vu0_sqc2(&sp50[3], 31);
    var_21 = *(f32 *)(arg0 + 0x60) * *(f32 *)(arg0 + 0x74);
    arg0 = *(u8 **)(arg0 + 0x8C);
    var_20 = 5.0f;
    while (arg0 != NULL) {
        p4_vu0_lqc2(10, &sp40);
        flags = *(s32 *)(arg0 + 0x68);
        if ((flags & 4) != 0) {
            p4_vu0_set_xf(2, var_20);
            p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
        }
        p4_vu0_lqc2(11, arg0 + 0x40);
        if ((flags & 0x80) != 0) {
            p4_vu0_set_xf(2, var_21);
            p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
        }
        p4_vu0_lqc2(28, &sp50[0]);
        p4_vu0_lqc2(29, &sp50[1]);
        p4_vu0_lqc2(30, &sp50[2]);
        p4_vu0_lqc2(31, &sp50[3]);
        p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 11, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 11, P4_VU_BC_Y);
        p4_vu0_madd(11, P4_VU_XYZW, 30, 11, P4_VU_BC_Z);
        p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_sqc2(&sp30, 10);
        temp_4 = *(u8 **)(arg0 + 0x90);
        if (temp_4 == NULL) {
            temp_4 = (u8 *)p4_low_memory;
        }
        temp_2 = *(s32 (**)(s32, void *, u8 *))(((u8 *)D_00713480 + (*(u16 *)(temp_4 + 4) << 6)) + 0x20);
        if (temp_2 != NULL) {
            temp_2(*(s32 *)(temp_4 + 8), &sp30, temp_4);
        }
        arg0 = *(u8 **)(arg0 + 0xAC);
    }
}
