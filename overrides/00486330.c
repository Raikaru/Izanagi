#include "p4pc/vu0.h"

/* Retail has no null test on the child's primitive (+0x90): without one it reads the halfword at address 4 and the word at address 8 of the EE
 * kernel's low memory and calls the kind-0 slot with the word (golden replay: the recorded children with a null primitive match an interpreted
 * retail run whose low memory is zero). The port does not map address 0, so a null primitive reads this zero record instead. */
void mdlEffectSetChildRotations(u8 *arg0, u8 *arg1)
{
    static const u8 p4_low_memory[16];
    u_long128 scratch[2];
    u8 *temp_4;
    u8 *var_16;
    s32 (*temp_2)(s32, void *);

    p4_vu0_lqc2(10, arg1);
    p4_vu0_sqc2(arg0 + 0x50, 10);
    p4_vu0_sqc2(&scratch[1], 10);
    effMiscBuildRotationVU();
    p4_vu0_lqc2(10, arg0 + 0x10);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
    p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
    p4_vu0_sqc2(arg0, 10);
    var_16 = *(u8 **)(arg0 + 0x8C);
    while (var_16 != NULL) {
        p4_vu0_lqc2(10, var_16 + 0x50);
        p4_vu0_lqc2(11, &scratch[1]);
        effMiscQuatMultiplyVU();
        p4_vu0_sqc2(&scratch[0], 10);
        temp_4 = *(u8 **)(var_16 + 0x90);
        if (temp_4 == NULL) {
            temp_4 = (u8 *)p4_low_memory;
        }
        temp_2 = *(s32 (**)(s32, void *))(((u8 *)D_00713480 + (*(u16 *)(temp_4 + 4) << 6)) + 0x24);
        if (temp_2 != NULL) {
            temp_2(*(s32 *)(temp_4 + 8), scratch);
        }
        var_16 = *(u8 **)(var_16 + 0xAC);
    }
    mdlEffectSetChildPositions(arg0, (f32 *)(arg0 + 0x40));
}
