#include "p4pc/vu0.h"

void mdlEffectMultiplyPaletteColorVU(u8 *arg0, u8 *arg1, s32 arg2)
{
    extern f32 D_00761134;
    s32 spC;
    s32 sp8;
    s32 sp4;
    s32 temp_hi;
    s32 temp_lo;
    s32 quotient;
    s32 remainder;
    f32 temp_f1;

    temp_lo = *(s32 *)(arg0 + 0x38) / 6;
    if (temp_lo > 0) {
        /* Keep the entry multiplier in VF12 for the final product. */
        p4_vu0_move(12, P4_VU_XYZW, 10);
        quotient = arg2 / temp_lo;
        remainder = arg2 % temp_lo;
        temp_f1 = (f32)remainder / (f32)temp_lo;
        temp_hi = (*(s8 *)(arg1 + 0x14) + quotient) % 6;

        /* Next and current colour words, each unpacked and scaled by D_00761134. */
        spC = ((s32 *)((u32)(((temp_hi + 1) % 6) * 4) + (u32)arg0))[0xF];
        effectVuUnpackColor10V0((const u32 *)&spC, D_00761134);
        p4_vu0_move(11, P4_VU_XYZW, 10);
        sp8 = ((s32 *)((u32)(temp_hi * 4) + (u32)arg0))[0xF];
        effectVuUnpackColor10V0((const u32 *)&sp8, D_00761134);

        /* Lerp: VF10 = cur * (1 - t) + next * t, then multiply by the entry multiplier. */
        p4_vu0_set_xf(2, 1.0f - temp_f1);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_set_xf(2, temp_f1);
        p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
        p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_move(11, P4_VU_XYZW, 12);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    } else {
        /* Entry multiplier parked in VF11 so the unpack can reuse VF10. */
        p4_vu0_move(11, P4_VU_XYZW, 10);
        sp4 = ((s32 *)((u32)(*(s8 *)(arg1 + 0x14) * 4) + (u32)arg0))[0xF];
        effectVuUnpackColor10V0((const u32 *)&sp4, D_00761134);
        p4_vu0_mul(10, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
    }
}
