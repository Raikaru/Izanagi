#include "p4pc/vu0.h"

// Render the model flare entries around the projected source position.
// FUN_004A6E70
void mdlEffectRenderFlares(u8 *arg0)
{
    extern s32 mdlEffectSamplePackedColor(u8 *a, u8 *b, s32 c, s32 d);
    extern void func_003c22f0(void *geometry);
    extern RpMaterial *RpMaterialSetTexture(RpMaterial *material, RwTexture *texture);
    extern s32 TnSlotObject(u16 b);
    extern s32 TnGetOffset38(u16 b);
    extern void PrimAppendNode(u8 *a, u8 *b);
    extern s32 kwlnGetCamera(void);
    extern RwMatrix *RwFrameGetLTM(RwFrame *frame);
    extern f32 sqrtf(f32 x);
    extern u8 D_007141B0[];
    extern f32 D_00922D80[];
    extern f32 fGpffff8044;
    f32 normalization;
    uint32_t normalizationBits;
    f32 proj[4] __attribute__((aligned(16)));
    f32 gmul[4] __attribute__((aligned(16)));
    f32 dist;
    f32 scale;
    f32 dx;
    f32 dy;
    f32 maxD;
    s32 cnt10;
    s32 cnt50;
    u8 *listBase;
    union { s32 word; MdlCol rgba; } color;
    u32 colA;
    s32 colB;
    u8 *entry;
    u8 *vtxCur;
    u8 *vtxBase;
    u32 *auxCur;
    u32 outer;
    u32 inner;
    s32 tmpX;
    s32 tmpY;
    f32 fx;
    f32 fy;
    f32 f0;
    f32 f1;
    f32 vx0;
    f32 vy0;
    f32 gw;
    s32 isz;
    s32 packed;
    s32 entryColor;
    s32 outWord;
    u8 *slot;
    f32 *fp;
    u32 j;

    /* Anchor position: VF10 = arg0[0..3], tested for on-screen, then stored to proj. */
    p4_vu0_lqc2(10, arg0);
    if (mdlEffectAnchorOnScreen() == 0) {
        return;
    }
    p4_vu0_sqc2(proj, 10);
    dx = proj[0] - 320.0f;
    dy = proj[1] - 224.0f;
    cnt50 = *(s32 *)(arg0 + 0x50);
    cnt10 = *(s32 *)(arg0 + 0x10);
    listBase = *(u8 **)(arg0 + 0x58);
    if (!(cnt50 <= 0 || cnt10 < cnt50)) {
        return;
    }
    dist = sqrtf(dx * dx + dy * dy);
    if (dist <= 0.0f) {
        return;
    }
    maxD = sqrtf(152576.0f); /* Squared distance from screen center to its corner. */
    scale = (maxD - dist) / maxD;
    scale = scale * *(f32 *)(arg0 + 0x4C);
    colA = mdlEffectSamplePackedColor(arg0 + 0x18, arg0 + 0x3C, cnt10, cnt50);
    normalization = fGpffff8044;
    __builtin_memcpy(&normalizationBits, &normalization, sizeof normalizationBits);

    /* gmul = unpack(colA) * scale (VF11) * unpack(colB) * scale (VF10), stored as the colour multiplier. */
    {
        P4Mmi128 lanes = p4_vu0_word_bytes_to_lanes(colA);
        p4_vu0_qmtc2(11, &lanes);
        p4_vu0_itof(11, P4_VU_XYZW, 11);
        p4_vu0_set_x(2, normalizationBits);
        p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
    }
    colB = *(s32 *)(arg0 + 0x14);
    {
        P4Mmi128 lanes = p4_vu0_word_bytes_to_lanes((uint32_t)colB);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_x(2, normalizationBits);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
    }
    p4_vu0_sqc2(gmul, 10);
    {
        u8 *p = *(u8 **)(listBase + 0x10);
        u8 *q = *(u8 **)(p + 0x18);
        RpGeometryLock((RpGeometry *)q, 0xFF2);
        vtxCur = *(u8 **)(*(u8 **)(*(u8 **)(listBase + 0x10) + 0x18) + 0x5C);
        vtxCur = *(u8 **)(vtxCur + 0x14);
        vtxBase = vtxCur;
        auxCur = *(u32 **)(*(u8 **)(*(u8 **)(listBase + 0x10) + 0x18) + 0x34);
        entry = D_007141B0 + (s32)*(u8 *)(arg0 + 0x54) * 0xD0;
        outer = 0;
        while (outer < 13) {
            if ((*(s32 *)(entry + 0xC) & 0xFF000000) == 0) {
                auxCur = (u32 *)((u8 *)auxCur + 0x80);
                for (inner = 0; inner < 4; inner++) {
                    *(RwV3d *)(vtxCur + 0x0) = *(const RwV3d *)D_00922D80;
                    *(RwV3d *)(vtxCur + 0xC) = *(const RwV3d *)D_00922D80;
                    *(RwV3d *)(vtxCur + 0x18) = *(const RwV3d *)D_00922D80;
                    *(RwV3d *)(vtxCur + 0x24) = *(const RwV3d *)D_00922D80;
                    vtxCur += 0x30;
                }
                {
                    if (iGpffffbb64.c3 != 0xFF) {
                        u8 *tbl = *(u8 **)(listBase + 0x54);
                        u8 *sl = *(u8 **)(tbl + (outer & 0xFFFF) * 4);
                        *(MdlCol *)(sl + 4) = iGpffffbb64;
                    } else {
                        u8 *tbl;
                        u8 *sl;
                        iGpffffbb64.c3 = 0xFE;
                        tbl = *(u8 **)(listBase + 0x54);
                        sl = *(u8 **)(tbl + (outer & 0xFFFF) * 4);
                        *(MdlCol *)(sl + 4) = iGpffffbb64;
                        iGpffffbb64.c3 = 0xFF;
                    }
                }
            } else {
                s32 idx;
                s32 dev = TnSlotObject(*(u8 *)(entry + 8));
                idx = (outer & 0xFFFF) * 4;
                RpMaterialSetTexture(*(RpMaterial **)(*(u8 **)(listBase + 0x54) + idx), (RwTexture *)(u32)dev);
                fx = dx * *(f32 *)(entry + 0x0);
                fy = dy * *(f32 *)(entry + 0x0);
                tmpX = (s32)(320.0f + fx);
                tmpY = (s32)(224.0f + fy);
                f0 = sqrtf(fx * fx + fy * fy);
                f1 = f0 / dist;
                f1 = f1 * scale;
                f0 = 128.0f;
                f0 = f0 * *(f32 *)(entry + 4);
                f0 = f0 * f1;
                isz = (s32)(0.5f * f0);
                packed = ((u32)(255.0f * sqrtf(f1)) << 24) | 0xFFFFFF;

                /* Flare colour: unpack(packed) * scale (VF11), unpack(entry colour) * scale * VF11
                 * (VF10), times gmul, then pack at scale 255. */
                {
                    P4Mmi128 lanes = p4_vu0_word_bytes_to_lanes((uint32_t)packed);
                    p4_vu0_qmtc2(11, &lanes);
                    p4_vu0_itof(11, P4_VU_XYZW, 11);
                    p4_vu0_set_x(2, normalizationBits);
                    p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                }
                entryColor = *(s32 *)(entry + 0xC);
                {
                    P4Mmi128 lanes = p4_vu0_word_bytes_to_lanes((uint32_t)entryColor);
                    P4Mmi128 zero;
                    P4Mmi128 r;
                    __builtin_memset(&zero, 0, sizeof zero);
                    p4_vu0_qmtc2(10, &lanes);
                    p4_vu0_itof(10, P4_VU_XYZW, 10);
                    p4_vu0_set_x(2, normalizationBits);
                    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                    p4_vu0_lqc2(11, gmul);
                    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                    /* Pack at scale 255.0f (0x437F0000). */
                    p4_vu0_set_x(2, 0x437F0000u);
                    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                    p4_vu0_ftoi(10, P4_VU_XYZW, 10);
                    p4_vu0_qmfc2(&r, 10);
                    r = p4_mmi_ppach(zero, r);
                    r = p4_mmi_ppacb(zero, r);
                    outWord = (s32)r.w[0];
                }
                {
                    color.word = outWord;
                    if (color.rgba.c3 != 0xFF) {
                        slot = *(u8 **)(*(u8 **)(listBase + 0x54) + idx);
                        *(MdlCol *)(slot + 4) = color.rgba;
                    } else {
                        color.rgba.c3 = 0xFE;
                        slot = *(u8 **)(*(u8 **)(listBase + 0x54) + idx);
                        *(MdlCol *)(slot + 4) = color.rgba;
                        color.rgba.c3 = 0xFF;
                    }
                }
                auxCur[0] = 0;
                auxCur[1] = 0;
                auxCur[2] = 0x3F700000;
                auxCur[3] = 0;
                auxCur[4] = 0;
                auxCur[5] = 0x3F700000;
                auxCur[6] = 0x3F700000;
                auxCur[7] = 0x3F700000;
                isz = (isz >> 1) - 1;
                vx0 = (f32)(tmpX - isz);
                *(f32 *)(vtxCur + 0x0) = vx0;
                vy0 = (f32)(tmpY - isz);
                *(f32 *)(vtxCur + 0x4) = vy0;
                gw = (f32)isz;
                *(f32 *)(vtxCur + 0xC) = *(f32 *)(vtxCur + 0x0) + gw;
                *(f32 *)(vtxCur + 0x10) = *(f32 *)(vtxCur + 0x4);
                *(f32 *)(vtxCur + 0x18) = *(f32 *)(vtxCur + 0x0);
                *(f32 *)(vtxCur + 0x1C) = *(f32 *)(vtxCur + 0x4) + gw;
                *(f32 *)(vtxCur + 0x24) = *(f32 *)(vtxCur + 0x0) + gw;
                *(f32 *)(vtxCur + 0x28) = *(f32 *)(vtxCur + 0x4) + gw;
                auxCur[8] = 0x3F700000;
                auxCur[9] = 0;
                auxCur[10] = 0;
                auxCur[11] = 0;
                auxCur[12] = 0x3F700000;
                auxCur[13] = 0x3F700000;
                auxCur[14] = 0;
                auxCur[15] = 0x3F700000;
                {
                    f32 vx1 = (f32)tmpX;
                    *(f32 *)(vtxCur + 0x30) = vx1;
                    *(f32 *)(vtxCur + 0x34) = vy0;
                    *(f32 *)(vtxCur + 0x3C) = *(f32 *)(vtxCur + 0x30) + gw;
                    *(f32 *)(vtxCur + 0x40) = *(f32 *)(vtxCur + 0x34);
                    *(f32 *)(vtxCur + 0x48) = *(f32 *)(vtxCur + 0x30);
                    *(f32 *)(vtxCur + 0x4C) = *(f32 *)(vtxCur + 0x34) + gw;
                    *(f32 *)(vtxCur + 0x54) = *(f32 *)(vtxCur + 0x30) + gw;
                    *(f32 *)(vtxCur + 0x58) = *(f32 *)(vtxCur + 0x34) + gw;
                }
                auxCur[16] = 0;
                auxCur[17] = 0x3F700000;
                auxCur[18] = 0x3F700000;
                auxCur[19] = 0x3F700000;
                auxCur[20] = 0;
                auxCur[21] = 0;
                auxCur[22] = 0x3F700000;
                auxCur[23] = 0;
                {
                    f32 vy1 = (f32)tmpY;
                    *(f32 *)(vtxCur + 0x60) = vx0;
                    *(f32 *)(vtxCur + 0x64) = vy1;
                    *(f32 *)(vtxCur + 0x6C) = *(f32 *)(vtxCur + 0x60) + gw;
                    *(f32 *)(vtxCur + 0x70) = *(f32 *)(vtxCur + 0x64);
                    *(f32 *)(vtxCur + 0x78) = *(f32 *)(vtxCur + 0x60);
                    *(f32 *)(vtxCur + 0x7C) = *(f32 *)(vtxCur + 0x64) + gw;
                    *(f32 *)(vtxCur + 0x84) = *(f32 *)(vtxCur + 0x60) + gw;
                    *(f32 *)(vtxCur + 0x88) = *(f32 *)(vtxCur + 0x64) + gw;
                }
                auxCur[24] = 0x3F700000;
                auxCur[25] = 0x3F700000;
                auxCur[26] = 0;
                auxCur[27] = 0x3F700000;
                auxCur[28] = 0x3F700000;
                auxCur[29] = 0;
                auxCur[30] = 0;
                auxCur[31] = 0;
                {
                    f32 vx1 = (f32)tmpX;
                    f32 vy1 = (f32)tmpY;
                    *(f32 *)(vtxCur + 0x90) = vx1;
                    *(f32 *)(vtxCur + 0x94) = vy1;
                    *(f32 *)(vtxCur + 0x9C) = *(f32 *)(vtxCur + 0x90) + gw;
                    *(f32 *)(vtxCur + 0xA0) = *(f32 *)(vtxCur + 0x94);
                    *(f32 *)(vtxCur + 0xA8) = *(f32 *)(vtxCur + 0x90);
                    *(f32 *)(vtxCur + 0xAC) = *(f32 *)(vtxCur + 0x94) + gw;
                    *(f32 *)(vtxCur + 0xB4) = *(f32 *)(vtxCur + 0x90) + gw;
                    *(f32 *)(vtxCur + 0xB8) = *(f32 *)(vtxCur + 0x94) + gw;
                }
                vtxCur += 0xC0;
                auxCur += 0x20;
            }
            outer++;
            entry += 0x10;
        }
    }
    {
        u8 *window = ((u8 *)(u32)kwlnGetCamera()) + 0x68;
        f32 nearPlane = *(f32 *)(((u8 *)(u32)kwlnGetCamera()) + 0x80);
        f32 farPlane = *(f32 *)(((u8 *)(u32)kwlnGetCamera()) + 0x84);
        f32 span = farPlane - nearPlane;
        f32 deviceDepth = (f32)(s32)0xFFFF0001;
        f32 denominator = deviceDepth * farPlane - -255.0f * span;
        f32 numerator;
        f32 k2 = (numerator = deviceDepth * nearPlane, numerator * farPlane) / denominator;
        f32 fA = 2.0f * (*(f32 *)(window + 0) * k2);
        f32 fB = 2.0f * (*(f32 *)(window + 4) * k2);
        f32 fC;
        j = 0;
        fC = 1.0f + k2;
        fp = (f32 *)vtxBase;
        while (j < 0xD0) {
            fp[0] = (-fp[0] / 640.0f + 0.5f) * fA;
            fp[1] = (-fp[1] / 448.0f + 0.5f) * fB;
            fp[2] = fC;
            fp += 3;
            j++;
        }
    }
    {
        u8 *p = *(u8 **)(listBase + 0x10);
        u8 *q = *(u8 **)(p + 0x18);
        func_003c22f0((RpGeometry *)q);
        if ((*(u16 *)listBase & 4) != 0) {
            *(u16 *)(q + 0xC) |= 1;
        }
        {
            u8 *r = (u8 *)((u8 *)(u32)kwlnGetCamera());
            RwMatrix *s2 = RwFrameGetLTM(*(RwFrame **)(r + 4));
            func_003e9cb0(*(RwFrame **)(listBase + 0xC), s2, rwCOMBINEREPLACE);
        }
        {
            u16 id = *(u16 *)(arg0 + 0x40);
            u8 *wd = (u8 *)(u32)TnGetOffset38(id);
            *(s32 *)(listBase + 0x18) = 0;
            *(s32 *)(listBase + 0x1C) = 0;
            PrimAppendNode(wd, listBase + 0x18);
        }
    }
}
