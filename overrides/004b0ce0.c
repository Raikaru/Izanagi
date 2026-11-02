#include "p4pc/vu0.h"

/* VU0 boundary: the source word is unpacked and scaled into vf10 and stored
 * as the record's scale vector. The per-record transform reloads vf28..vf31
 * from the saved matrix before each record, because the PS2 callees can
 * clobber the VF registers. */
void DistortParticleProjectRecords(u8 *arg0, s32 arg1) {
    s32 spFC;
    s32 *pt;
    f32 spF0[2];
    u8 spE0[16];
    u8 spD0[16];
    u8 spC0[16];
    u8 spB0[16];
    u8 sp70[0x40];
    u8 *var_18;
    s32 temp_16;
    s32 var_21;
    s32 var_21_2;
    u8 *temp_21;
    u8 *temp_3;
    u8 *temp_4;
    u8 *var_17;
    s32 scale;

    temp_3 = *(u8 **)(arg0 + 0x5C);
    temp_16 = *(s32 *)(temp_3 + 8);
    if ((temp_16 != 0) && (*(s32 *)(temp_3 + 0x10) != 0)) {
        if (*(s32 *)(arg0 + 0x60) == 0) {
            kAssertFail(D_00714550, 0x265);
        }
        var_18 = *(u8 **)(*(u8 **)(arg0 + 0x5C) + 0x18);
        var_17 = *(u8 **)(arg0 + 0x64);
        temp_21 = *(u8 **)(s32)TnSlotObject(0x15);
        if ((arg1 & 0xFF) == 1) {
            D_00887300[0](1, (u32)temp_21);
        }
        spF0[0] = (f32) * (s32 *)(temp_21 + 0xC);
        spF0[1] = (f32) * (s32 *)(temp_21 + 0x10);
        spFC = *(s32 *)(arg0 + 4);
        pt = &spFC;
        scale = iGpffff81f4;
        {
            P4Mmi128 lanes;

            lanes = p4_vu0_word_bytes_to_lanes((uint32_t)*pt);
            p4_vu0_qmtc2(10, &lanes);
            p4_vu0_itof(10, P4_VU_XYZW, 10);
            p4_vu0_set_x(2, (uint32_t)scale);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        }
        p4_vu0_sqc2(spE0, 10);
        temp_4 = *(u8 **)(arg0 + 0x5C);
        if (!(*(s32 *)(temp_4 + 0xC) & 1)) {
            var_21 = 0;
            while (var_21 < temp_16) {
                if (*(s32 *)(var_18 + 0x10) >= 0) {
                    DistortParticleDrawSky(arg0, var_17, var_18, var_18, spE0, spF0, arg1, effMiscRand(0) & 3);
                }
                var_21 += 1;
                var_18 += 0x20;
                var_17 += 0x1C;
            }
        } else {
            mdlEffectLoadQuadB(temp_4, spB0);
            mdlEffectLoadQuadA(*(u8 **)(arg0 + 0x5C), spC0);
            p4_vu0_lqc2(10, spB0);
            effMiscBuildRotationVU();
            p4_vu0_lqc2(31, spC0);
            p4_vu0_sqc2(sp70, 28);
            p4_vu0_sqc2(sp70 + 16, 29);
            p4_vu0_sqc2(sp70 + 32, 30);
            p4_vu0_sqc2(sp70 + 48, 31);
            var_21_2 = 0;
            while (var_21_2 < temp_16) {
                if (*(s32 *)(var_18 + 0x10) >= 0) {
                    p4_vu0_lqc2(28, sp70);
                    p4_vu0_lqc2(29, sp70 + 16);
                    p4_vu0_lqc2(30, sp70 + 32);
                    p4_vu0_lqc2(31, sp70 + 48);
                    p4_vu0_lqc2(10, var_18);
                    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                    p4_vu0_madd(10, P4_VU_XYZW, 31, 0, P4_VU_BC_W);
                    p4_vu0_sqc2(spD0, 10);
                    DistortParticleDrawSky(arg0, var_17, var_18, spD0, spE0, spF0, arg1, effMiscRand(0) & 3);
                }
                var_21_2 += 1;
                var_18 += 0x20;
                var_17 += 0x1C;
            }
        }
        D_00887300[0](1, 0);
    }
}
