#include "p4pc/vu0.h"

void mdlEffectApplyPackedColor(u8 *arg0, s32 arg1)
{
    u32 baseColor;
    u32 childColor;
    u32 packedColor;
    f32 parent[4] __attribute__((aligned(16)));
    f32 scale;
    const u32 *source;
    u8 *node;
    u8 *data;
    void (*callback)(s32, u32);
    u32 color;
    P4Mmi128 zero;
    P4Mmi128 r;

    __builtin_memset(&zero, 0, sizeof zero);

    *(s32 *)(arg0 + 0x64) = arg1;
    baseColor = (u32)arg1;
    source = &baseColor;
    scale = fGpffff8044;
    mdlEffectNormalizePackedColor(source, scale);
    p4_vu0_sqc2(parent, 10);

    node = *(u8 **)(arg0 + 0x8C);
    while (node != NULL) {
        childColor = *(u32 *)(node + 0x64);
        source = &childColor;
        mdlEffectNormalizePackedColor(source, scale);
        p4_vu0_lqc2(11, parent);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_set_x(2, 0x437F0000U); /* 255.0f */
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_ftoi(10, P4_VU_XYZW, 10);
        p4_vu0_qmfc2(&r, 10);
        r = p4_mmi_ppach(zero, r);
        r = p4_mmi_ppacb(zero, r);
        packedColor = r.w[0];
        color = packedColor;
        data = *(u8 **)(node + 0x90);
        callback = *(void (**)(s32, u32))(
            (u8 *)D_00713480 + *(u16 *)(data + 4) * 0x40 + 0x30);
        if (callback != NULL) {
            callback(*(s32 *)(data + 8), color);
        }
        node = *(u8 **)(node + 0xAC);
    }
}
