#include "p4pc/vu0.h"

/* Retail has no null test on the child's primitive (+0x90): without one it reads the halfword at address 4 and the word at address 8 of the EE
 * kernel's low memory and calls the kind-0 slot with the word (golden replay: the recorded children with a null primitive match an interpreted
 * retail run whose low memory is zero). The port does not map address 0, so a null primitive reads this zero record instead. */
void mdlEffectSetChildScales(u8 *arg0, f32 arg1)
{
    static const u8 p4_low_memory[16];
    u_long128 sp50;
    u_long128 sp40;
    f32 var_21;
    f32 var_20;
    f32 temp_f12;
    f32 temp_f0;
    s32 flags;
    u8 *temp_4;
    u8 *var_16;
    void (*temp_3)(s32, f32);   /* the scale slot takes (instance, scale): a0 and f12; the retail prim in a1 is never read */
    void (*temp_3_2)(s32, void *, u8 *);
    void (*temp_3_3)(s32, void *, u8 *);

    *(f32 *)(arg0 + 0x60) = arg1;
    var_21 = arg1 * *(f32 *)(arg0 + 0x74);
    var_16 = *(u8 **)(arg0 + 0x8C);
    var_20 = 5.0f;
    while (var_16 != NULL) {
        flags = *(s32 *)(var_16 + 0x68);
        if ((flags & 0x100) == 0) {
            temp_f12 = *(f32 *)(var_16 + 0x60);
            if ((*(s32 *)(var_16 + 0x84) & 1) != 0) {
                temp_f12 *= var_21;
            }
            temp_4 = *(u8 **)(var_16 + 0x90);
        if (temp_4 == NULL) {
            temp_4 = (u8 *)p4_low_memory;
        }
            temp_3 = *(void (**)(s32, f32))(((u8 *)D_00713480 + (*(u16 *)(temp_4 + 4) << 6)) + 0x28);
            if (temp_3 != NULL) {
                temp_3(*(s32 *)(temp_4 + 8), temp_f12);
            }
        } else {
            p4_vu0_lqc2(10, var_16 + 0x20);
            temp_f0 = *(f32 *)(var_16 + 0x60);
            if ((*(s32 *)(var_16 + 0x84) & 1) != 0) {
                temp_f0 *= var_21;
            }
            p4_vu0_set_xf(2, temp_f0);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
            p4_vu0_sqc2(&sp40, 10);
            temp_4 = *(u8 **)(var_16 + 0x90);
        if (temp_4 == NULL) {
            temp_4 = (u8 *)p4_low_memory;
        }
            temp_3_2 = *(void (**)(s32, void *, u8 *))(((u8 *)D_00713480 + (*(u16 *)(temp_4 + 4) << 6)) + 0x2C);
            if (temp_3_2 != NULL) {
                temp_3_2(*(s32 *)(temp_4 + 8), &sp40, temp_4);
            }
        }
        flags = *(s32 *)(var_16 + 0x68);
        if ((flags & 0x80) != 0) {
            p4_vu0_lqc2(10, arg0 + 0x40);
            p4_vu0_lqc2(11, arg0);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            if ((flags & 4) != 0) {
                p4_vu0_set_xf(2, var_20);
                p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
            }
            p4_vu0_lqc2(11, var_16 + 0x40);
            p4_vu0_set_xf(2, var_21);
            p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(&sp50, 10);
            temp_4 = *(u8 **)(var_16 + 0x90);
        if (temp_4 == NULL) {
            temp_4 = (u8 *)p4_low_memory;
        }
            temp_3_3 = *(void (**)(s32, void *, u8 *))(((u8 *)D_00713480 + (*(u16 *)(temp_4 + 4) << 6)) + 0x20);
            if (temp_3_3 != NULL) {
                temp_3_3(*(s32 *)(temp_4 + 8), &sp50, temp_4);
            }
        }
        var_16 = *(u8 **)(var_16 + 0xAC);
    }
}
