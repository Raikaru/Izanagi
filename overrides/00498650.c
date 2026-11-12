#include "p4pc/vu0.h"

void ThunderPairedWriteRows(u8 *arg0)
{
    u8 *obj;
    u8 *ctrl;
    u8 *list;
    s32 n;
    s32 count;
    u8 spDCb[4];
    s32 spD8;
    s32 spD4;
    s32 spD0;
    s32 spCC;
    f32 spB0[4];
    s32 tmp;
    f32 scale;
    s32 i;
    u8 *e0;
    u8 *dst;
    f32 sp70[16];
    P4Mmi128 lanes;
    P4Mmi128 mmiZero;
    P4Mmi128 mmiR;

    __builtin_memset(&mmiZero, 0, sizeof mmiZero);

    obj = *(u8 **)(arg0 + 0x30);
    ctrl = *(u8 **)(arg0 + 0x34);
    list = *(u8 **)obj;
    n = *(s32 *)(arg0 + 0x28);
    count = *(s32 *)(ctrl + 0x34);
    if ((count >= n) || (count == 0))
    {
        count = *(s32 *)(ctrl + 0x38);
        tmp = (s32)mdlEffectSamplePackedColor(ctrl, ctrl + 0x24, n, *(s32 *)(ctrl + 0x34));
        spD8 = *(s32 *)(arg0 + 0x24);
        scale = iGpffff8044;
        lanes = p4_vu0_word_bytes_to_lanes((uint32_t)spD8);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, scale);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_move(11, P4_VU_XYZW, 10);
        spD4 = tmp;
        lanes = p4_vu0_word_bytes_to_lanes((uint32_t)spD4);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, scale);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_sqc2(spB0, 10);
        GeometryMatrixBuild(&sp70[0], arg0, 0, *(f32 *)(arg0 + 0x20));
        i = 0;
        while (i < count)
        {
            if (*(s32 *)(list + 4) > 0)
            {
                e0 = *(u8 **)list;
                func_003e9cb0(*(void **)(e0 + 0xC), &sp70[0], 0);
                spD0 = *(s32 *)(list + 8);
                lanes = p4_vu0_word_bytes_to_lanes((uint32_t)spD0);
                p4_vu0_qmtc2(10, &lanes);
                p4_vu0_itof(10, P4_VU_XYZW, 10);
                p4_vu0_set_xf(2, scale);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                p4_vu0_lqc2(11, spB0);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_set_x(2, 0x437F0000u); /* 255.0f */
                p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                p4_vu0_ftoi(10, P4_VU_XYZW, 10);
                p4_vu0_qmfc2(&mmiR, 10);
                mmiR = p4_mmi_ppach(mmiZero, mmiR);
                mmiR = p4_mmi_ppacb(mmiZero, mmiR);
                spCC = (s32)mmiR.w[0];
                /* measured: mwcc b210 hoists the spCC reload above the inline COP2
                   store and serves a stale value, so the post-asm read is volatile. */
                *(s32 *)spDCb = *(volatile s32 *)&spCC;
                if (spDCb[3] != 0xFF)
                {
                    dst = *(u8 **)(e0 + 0x14);
                    *(Color4 *)(dst + 4) = *(Color4 *)spDCb;
                }
                else
                {
                    spDCb[3] = 0xFE;
                    dst = *(u8 **)(e0 + 0x14);
                    *(Color4 *)(dst + 4) = *(Color4 *)spDCb;
                    spDCb[3] = 0xFF;
                }
                if (*(u8 *)(ctrl + 0x5C) != 0)
                {
                    *(u16 *)e0 = *(u16 *)e0 | 1;
                }
                else
                {
                    *(u16 *)e0 = *(u16 *)e0 & 0xFFFE;
                }
                {
                    s32 b = *(u16 *)(ctrl + 0x28);
                    GeometryBlockBindResource(e0, b);
                }
            }
            i++;
            list += 0xC;
        }
    }
}
