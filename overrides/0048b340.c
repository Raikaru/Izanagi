#include "p4pc/vu0.h"

void mdlEffectUpdateParticleTrail(u8 *arg0, u8 *arg1)
{
    typedef struct TrailColor {
        u32 rgb : 24;
        u32 alpha : 8;
    } TrailColor;
    extern void memcpy(void *dst, void *src, u32 size);
    extern void mdlEffectCatmullRomVU(void *quads, f32 t);
    extern f32 D_00761134;
    u_long128 quadF0[4];
    u_long128 tmpE0;
    u_long128 tmpD0;
    u32 k;
    u32 i;
    u8 *src1;
    u8 *iter;
    u8 *clear;
    u32 j;
    u8 *dst1;
    s32 c4;
    u8 *config;
    u32 c0;
    u32 boundC0;
    u32 nmult;
    s32 node10;
    f32 one;
    f32 acc;
    f32 invN;
    f32 scaleTop;
    f32 inv2;
    f32 acc2;
    f32 baseX;
    f32 diffX;
    f32 baseY;
    f32 diffY;
    u32 stride;
    f32 ftmp;
    u32 alpha;
    u32 lo;
    u8 *second;
    u32 particleIndex;
    P4Mmi128 lanes;
    P4Mmi128 zero;
    P4Mmi128 r;

    config = *(u8 **)(arg0 + 0x20);
    c4 = *(s32 *)(config + 0xC4);
    c0 = *(u32 *)(config + 0xC0);
    nmult = c0 * (u32)c4;
    if (nmult == 0) {
        return;
    }
    __builtin_memset(&zero, 0, sizeof zero);
    node10 = *(s32 *)(arg1 + 0x10);
    particleIndex = ((u32)arg1 - (u32)*(u8 **)(arg0 + 24)) >> 5;
    clear = *(u8 **)(arg0 + 24) + ((*(u32 *)(arg0 + 4) + particleIndex * nmult) << 5);
    if (node10 == 0) {
        memcpy(clear, arg1, 0x20);
        src1 = clear;
        for (i = 0; i < nmult; i++) {
            *(s32 *)(src1 + 0x10) = -1;
            src1 += 0x20;
        }
        return;
    }
    *(s32 *)(clear + 0x10) = node10 - 1;
    one = 1.0f;
    invN = one / (f32)(u32)nmult;
    acc = 0.0f;
    scaleTop = (f32)(u32)((*(u32 *)(arg1 + 0x14)) >> 24);
    boundC0 = nmult - (u32)c4;
    dst1 = clear + nmult * 32 - 0x20;
    stride = (u32)(c4 * 32);
    src1 = dst1 - stride;
    for (j = 0; j < boundC0; j++) {
        memcpy(dst1, src1, 0x20);
        /* Retail 0048b4ec: memcpy does not initialize the source's color.
         * An invalid age does not bypass this scalar read. */
        lo = ((TrailColor *)(src1 + 0x14))->rgb;
        ftmp = scaleTop * acc;
        alpha = (u32)ftmp;
        *(u32 *)(dst1 + 0x14) = lo | (alpha << 24);
        acc = acc + invN;
        dst1 -= 0x20;
        src1 -= 0x20;
    }
    memcpy(clear, arg1, 0x20);
    *(s32 *)(clear + 0x10) = -1;
    if ((c4 == 1) || (c0 < 2)) {
        return;
    }
    if ((node10 >= 2) && (c0 >= 3)) {
        u8 *last;
        u32 colTmpA;
        u32 colTmpB;
        u32 packedTmp;

        last = (u8 *)((u32)(c4 << 6) + (u32)clear);
        if (*(s32 *)(last + 0x10) < 0) {
            return;
        }
        {
            u_long128 *dst = &quadF0[0];

            *dst = *(u_long128 *)last;
            dst = &quadF0[1];
            second = clear + (u32)c4 * 32;
            *dst = *(u_long128 *)second;
            dst = &quadF0[2];
            *dst = *(u_long128 *)clear;
            dst = &quadF0[3];
            *dst = *(u_long128 *)clear;
        }
        iter = second - 0x20;
        acc2 = 0.0f;
        inv2 = one / (f32)(u32)((u32)c4 + 1);
        baseX = *(f32 *)(second + 0x18);
        diffX = *(f32 *)(clear + 0x18) - baseX;
        baseY = *(f32 *)(second + 0x1C);
        diffY = *(f32 *)(clear + 0x1C) - baseY;
        colTmpA = *(u32 *)(clear + 0x14);

        /* Unpack colour A (4 bytes -> 4 floats, scaled) into VF10, copy to VF11. */
        lanes = p4_vu0_word_bytes_to_lanes(colTmpA);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, D_00761134);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_move(11, P4_VU_XYZW, 10);

        /* Unpack colour B into VF10 and keep it in tmpE0. */
        colTmpB = *(u32 *)(second + 0x14);
        lanes = p4_vu0_word_bytes_to_lanes(colTmpB);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, D_00761134);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_sqc2(&tmpE0, 10);

        /* VF11 = A - B; VF10 = (1,1,1,0) * inv2; tmpD0 = VF10 * VF11. */
        p4_vu0_sub(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
        p4_vu0_add(10, P4_VU_XYZ, 0, 0, P4_VU_BC_W);
        p4_vu0_mul(10, P4_VU_W, 0, 0, P4_VU_BC_X);
        p4_vu0_set_xf(2, inv2);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_sqc2(&tmpD0, 10);

        for (k = 0; k < (u32)c4 - 1; iter -= 0x20, k++) {
            acc2 = acc2 + inv2;
            mdlEffectCatmullRomVU(quadF0, acc2);
            p4_vu0_sqc2(iter, 10);
            *(f32 *)(iter + 0x18) = baseX + diffX * acc2;
            *(f32 *)(iter + 0x1C) = baseY + diffY * acc2;
            p4_vu0_lqc2(10, &tmpE0);
            p4_vu0_lqc2(11, &tmpD0);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(&tmpE0, 10);
            p4_vu0_set_xf(2, 255.0f);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
            p4_vu0_ftoi(10, P4_VU_XYZW, 10);
            p4_vu0_qmfc2(&r, 10);
            r = p4_mmi_ppach(zero, r);
            r = p4_mmi_ppacb(zero, r);
            packedTmp = (u32)r.w[0];
            *(u32 *)(iter + 0x14) = packedTmp;
            *(s32 *)(iter + 0x10) = node10;
        }
    } else {
        u8 *anchor;
        u32 colTmpA;
        u32 colTmpB;
        u32 packedTmp;
        f32 inv3;
        const f32 one3 = 1.0f;
        f32 acc3;
        f32 bx;
        f32 dx;
        f32 by;
        f32 dy;

        if (node10 <= 0) {
            return;
        }
        anchor = (u8 *)(stride + (u32)clear);
        if (*(s32 *)(anchor + 0x10) < 0) {
            return;
        }
        src1 = anchor - 0x20;
        acc3 = 0.0f;
        inv3 = one3 / (f32)(u32)((u32)c4 + 1);
        bx = *(f32 *)(anchor + 0x18);
        dx = *(f32 *)(clear + 0x18) - bx;
        by = *(f32 *)(anchor + 0x1C);
        dy = *(f32 *)(clear + 0x1C) - by;
        colTmpA = *(u32 *)(clear + 0x14);

        /* Unpack colour A into VF10 and copy to VF11. */
        lanes = p4_vu0_word_bytes_to_lanes(colTmpA);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, D_00761134);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_move(11, P4_VU_XYZW, 10);

        /* Unpack colour B (from the anchor) into VF10 and keep it in tmpE0. */
        colTmpB = *(u32 *)(anchor + 0x14);
        lanes = p4_vu0_word_bytes_to_lanes(colTmpB);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_xf(2, D_00761134);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_sqc2(&tmpE0, 10);

        p4_vu0_sub(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
        p4_vu0_add(10, P4_VU_XYZ, 0, 0, P4_VU_BC_W);
        p4_vu0_mul(10, P4_VU_W, 0, 0, P4_VU_BC_X);
        p4_vu0_set_xf(2, inv3);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_sqc2(&tmpD0, 10);

        for (k = 0; k < (u32)c4 - 1; src1 -= 0x20, k++) {
            acc3 = acc3 + inv3;
            p4_vu0_lqc2(10, anchor);
            p4_vu0_lqc2(11, clear);
            /* VF3.w = 1 - t; ACC = VF11 * t; VF10 = ACC + VF10 * VF3.w. */
            p4_vu0_set_xf(2, acc3);
            p4_vu0_sub(3, P4_VU_W, 0, 2, P4_VU_BC_X);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
            p4_vu0_madd(10, P4_VU_XYZW, 10, 3, P4_VU_BC_W);
            p4_vu0_sqc2(src1, 10);
            *(f32 *)(src1 + 0x18) = bx + dx * acc3;
            *(f32 *)(src1 + 0x1C) = by + dy * acc3;
            p4_vu0_lqc2(10, &tmpE0);
            p4_vu0_lqc2(11, &tmpD0);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(&tmpE0, 10);
            p4_vu0_set_xf(2, 255.0f);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
            p4_vu0_ftoi(10, P4_VU_XYZW, 10);
            p4_vu0_qmfc2(&r, 10);
            r = p4_mmi_ppach(zero, r);
            r = p4_mmi_ppacb(zero, r);
            packedTmp = (u32)r.w[0];
            *(u32 *)(src1 + 0x14) = packedTmp;
            *(s32 *)(src1 + 0x10) = node10;
        }
    }
}
