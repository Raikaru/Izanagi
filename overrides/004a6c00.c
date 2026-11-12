#include "p4pc/vu0.h"

// Creates a grid-backed object from the 0x40-byte record at arg0, copied to +0x18: allocates the 0x5C object, builds a multi-part grid geometry at +0x58, and sets 0x34 sixteen-byte slots of its table to -1.
u8 *mdlEffectCreateGridForRecord(u8 *arg0)
{
    u16 *temp_18;
    u16 *temp_2_2;
    u16 *temp_3;
    u32 var_4;
    u8 *temp_17;
    u8 *temp_2;
    u8 *var_5;
    s32 fill;

    chkmemAllocSiteTag(D_00714350, 0x78);
    temp_2 = (u8 *)jtbl_008873E8[0](0x5C, 0x40000);
    if (temp_2 == NULL) {
        kAssertFail(D_00714350, 0x79);
    }
    memset(temp_2, 0, 0x5C);
    p4_vu0_sqc2(temp_2, 0);
    memcpy(temp_2 + 0x18, arg0, 0x40);
    temp_2_2 = GeometryCreateTableGridMultiPart(0xD, 4, 2, 4, D_00713408, 0x4C);
    *(u16 **)(temp_2 + 0x58) = temp_2_2;
    *temp_2_2 = *temp_2_2 & 0xFFFE;
    temp_3 = *(u16 **)(temp_2 + 0x58);
    *temp_3 = *temp_3 & 0xFFFB;
    RpGeometryLock(
        *(RpGeometry **)(*(u8 **)(*(u8 **)(temp_2 + 0x58) + 0x10) + 0x18), 8);
    var_5 = *(u8 **)(*(u8 **)(*(u8 **)(*(u8 **)(temp_2 + 0x58) + 0x10) + 0x18) + 0x30);
    fill = -1;
    for (var_4 = 0; var_4 < 0x34U; var_4++, var_5 += 0x10) {
        *(s32 *)(var_5 + 0) = fill;
        *(s32 *)(var_5 + 4) = fill;
        *(s32 *)(var_5 + 8) = fill;
        *(s32 *)(var_5 + 0xC) = fill;
    }
    temp_18 = *(u16 **)(temp_2 + 0x58);
    temp_17 = *(u8 **)(*(u8 **)((u8 *)temp_18 + 0x10) + 0x18);
    func_003c22f0(temp_17, var_5);
    if ((*temp_18 & 4) != 0) {
        *(u16 *)(temp_17 + 0xC) = *(u16 *)(temp_17 + 0xC) | 1;
    }
    return temp_2;
}
