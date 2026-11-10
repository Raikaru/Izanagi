#include "p4pc/vu0.h"

void *EffModelCreate(void *arg0) {
    f32 temp_f20;
    s32 temp_18;
    s32 temp_2_3;
    u8 *temp_2;
    u32 lookupType;
    u8 *temp_2_2;
    u8 *temp_2_4;

    chkmemAllocSiteTag(D_007144B8, 0x171);
    temp_2 = (u8 *)jtbl_008873E8[0](0x9C, 0x40000);
    memset(temp_2, 0, 0x9C);
    if (temp_2 == NULL) {
        kAssertFail(D_007144A8, 0x73);
    }
    *(s32 *)(temp_2 + 0x98) = 0;
    *(s32 *)(temp_2 + 0x28) = -1;
    temp_f20 = 1.0f;
    *(f32 *)(temp_2 + 0x20) = temp_f20;
    p4_vu0_sqc2(temp_2, 0);
    p4_vu0_sqc2(temp_2 + 16, 0);
    if (arg0 != NULL) {
        temp_2_2 = mdlEffectResolveRelPtrA(arg0);
        if (temp_2_2 == NULL) {
            kAssertFail(D_007144A8, 0x83);
        }
        memcpy(temp_2 + 0x2C, temp_2_2, 0x68);
        temp_2_3 = (s32)mdlEffectResolveRelPtrB(arg0);
        if (temp_2_3 != 0) {
            if (*(u16 *)((u8 *)arg0 + 0x1C) != 3) {
                kAssertFail(D_007144A8, 0x8A);
            }
            temp_18 = *(s32 *)((u8 *)arg0 + 0x24);
            while ((lookupType = 6, mdlFindModelObject(lookupType, iGpffffbb90, 0)) != 0) {
                iGpffffbb90 += 1;
            }
            temp_2_4 = mdlCreateModelObjectFromMemory(6, iGpffffbb90, (void *)temp_2_3, (u32)temp_18, 1);
            mdlInitIdentityMatrix(temp_2_4);
            if (mdlLayerIsPresent(temp_2_4, 0) != 0) {
                mdlDispatchAnimIfPresent(temp_2_4, 0, 0, 0, 0);
                mdlSetLayerWeight(temp_2_4, 0, temp_f20);
            }
            *(u32 *)(temp_2_4 + 0xD8) &= ~2U;
            mdlSetResetCallback(temp_2_4, EffModelResetCallback, temp_2_4);
            iGpffffbb90 += 1;
            *(u8 **)(temp_2 + 0x94) = temp_2_4;
        }
    }
    return temp_2;
}
