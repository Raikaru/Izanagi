#include "p4pc/vu0.h"

void *mdlEffectCreateInstance(void *source)
{
    u8 *arg0 = (u8 *)source;
    u16 *temp_2_4;
    u16 temp_4;
    u8 *temp_2;
    u8 *temp_2_2;
    u8 *temp_2_3;
    u8 *temp_3;

    chkmemAllocSiteTag(D_007141A0, 0x1C);
    temp_2 = (u8 *)jtbl_008873E8[0](0xD4, 0x40000);
    if (temp_2 == NULL) {
        kAssertFail(D_007141A0, 0x1D);
    }
    memset(temp_2, 0, 0xD4);
    p4_vu0_sqc2(temp_2, 0);
    p4_vu0_sqc2(temp_2 + 0x10, 0);
    *(s32 *)(temp_2 + 0x24) = -1;
    *(f32 *)(temp_2 + 0x20) = 1.0f;
    if (arg0 == NULL) {
        return temp_2;
    }
    temp_2_2 = GeometryCreateTableGrid(1, 2, 4, D_00713408, 0x4C);
    *(u8 **)(temp_2 + 0xC8) = temp_2_2;
    temp_3 = *(u8 **)(*(u8 **)(*(u8 **)(temp_2_2 + 0x10) + 0x18) + 0x30);
    *(s32 *)(temp_3 + 0) = -1;
    *(s32 *)(temp_3 + 4) = -1;
    *(s32 *)(temp_3 + 8) = -1;
    *(s32 *)(temp_3 + 0xC) = -1;
    *(s32 *)(temp_2 + 0x28) = *(u16 *)(arg0 + 0xC);
    temp_2_3 = mdlEffectResolveRelPtrA(arg0);
    if (temp_2_3 == NULL) {
        kAssertFail(D_007141A0, 0x47);
    }
    memcpy(temp_2 + 0x30, temp_2_3, 0x98);
    temp_2_4 = mdlEffectResolveRelPtrB(arg0);
    if (temp_2_4 != NULL) {
        temp_4 = *(u16 *)(arg0 + 0x1C);
        switch (temp_4) {
        case 1:
            GeometryBlockSetTextureFromHandle(*(u8 **)(temp_2 + 0xC8), temp_2_4);
            break;
        case 2:
            *(s32 *)(temp_2 + 0xD0) = mdlEffectCreateTrackInstance(temp_2_4);
            break;
        case 4:
            func_003c42b0(
                *(u8 **)(*(u8 **)(temp_2 + 0xC8) + 0x14),
                TnSlotObject(*temp_2_4));
            break;
        case 7:
            *(s32 *)(temp_2 + 0xCC) = TlpCreate(temp_2_4);
            break;
        default:
            kAssertFail(D_007141A0, 0x5D);
            break;
        }
    }
    return temp_2;
}
