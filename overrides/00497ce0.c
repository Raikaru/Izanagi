#include "p4pc/vu0.h"

void ThunderPairedUpdateStrip(u8 *arg0)
{
    extern void mdlEffectCopyCameraOrigin(f32 *arg0);
    extern void effMiscBuildRotationVU(void);
    extern f32 sinf(f32 arg0);
    extern void effMiscAxisAngleVU(f32 angle, f32 *axis);
    extern void func_003c22f0(u8 *geometry);
    extern f32 fGpffff8084;
    extern f32 fGpffff80d0;
    extern f32 fGpffff80d8;
    extern f32 iGpffff80d4;
    extern EffectVuVector D_00713CE0;
    extern EffectVuVector D_00713D00;
    extern f32 D_00713D10[];
    extern f32 D_00713D14[];
    extern f32 D_00713D18[];
    EffectVuVector innerExtent;
    EffectVuVector outerExtent;
    EffectVuVector outerFlip;
    EffectVuVector direction;
    EffectVuVector side;
    EffectVuVector tangent;
    EffectVuVector previousTangent;
    EffectVuVector position;
    EffectVuVector nextPosition;
    EffectVuVector up;
    EffectVuVector step;
    f32 spacing;
    f32 phase;
    f32 cycle;
    f32 sample;
    f32 wave;
    f32 amplitude;
    f32 period;
    f32 twoPi;
    f32 four;
    f32 waveRange;
    f32 two;
    f32 current;
    f32 one;
    f32 half;
    f32 swing;
    f32 zero;
    f32 previous;
    u8 *config;
    u8 *list;
    u8 *bolt;
    s32 count;
    s32 respawnRange;
    s32 row;
    s32 index;
    u8 *instance;
    s32 rows;
    u8 *vertices;
    u32 state;
    u32 invalid;

    list = *(u8 **)(arg0 + 0x30);
    config = *(u8 **)(arg0 + 0x34);
    bolt = *(u8 **)list;
    if ((*(u32 *)(config + 0x34) < *(u32 *)(arg0 + 0x28)) && (*(u32 *)(config + 0x34) != 0)) {
        return;
    }
    count = *(s32 *)(config + 0x38);
    respawnRange = *(s32 *)(config + 0x40);
    mdlEffectCopyCameraOrigin(up.lane);
    effectVuLoad10(&D_00713CE0);
    p4_vu0_move(12, P4_VU_XYZW, 10);
    {
        f32 inner = *(f32 *)(config + 0x7C);

        effectVuScale10(inner);
        effectVuStore10(&innerExtent);
        p4_vu0_move(10, P4_VU_XYZW, 12);
        effectVuScale10(inner + *(f32 *)(config + 0x80));
    }
    effectVuStore10(&outerExtent);
    p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
    effectVuStore10(&outerFlip);
    effectVuLoad10((EffectVuVector *)(arg0 + 0x10));
    effMiscBuildRotationVU();
    effectVuLoad10(&D_00713D00);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
    p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
    effectVuScale10(*(f32 *)(config + 0x84));
    {
        f32 length;

        /* Retail moves the vsqrt result through $2 (cfc2/mtc1 bridge). */
        p4_vu0_move(11, P4_VU_XYZW, 0);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
        p4_vu0_sqrt(2, P4_VU_BC_X);
        length = p4pc_vu0.q;
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        effectVuStore10(&direction);
        spacing = (f32)*(u32 *)(config + 0x3C);
        two = 2.0f;
        spacing = (two * length) / spacing;
    }
    amplitude = *(f32 *)(config + 0x44);
    wave = amplitude;
    index = 0;
    invalid = 0xFFFFFFFF;
    period = fGpffff80d0;
    zero = 0.0f;
    twoPi = fGpffff8084;
    four = 4.0f;
    waveRange = iGpffff80d4;
    one = 1.0f;
    half = 0.5f;
    swing = fGpffff80d8;
    for (; index < count; index++, bolt += 0xC) {
        if (*(s32 *)(bolt + 4) == -1) {
            *(u32 *)(bolt + 8) = invalid;
            goto advance;
        }
        state = *(u32 *)(bolt + 8);
        if ((state & 0xFF000000) > 0x40000000U) {
            *(u32 *)(bolt + 8) = state + 0xC0000000;
        } else {
            *(s32 *)(bolt + 4) = -1;
            if (respawnRange > 0) {
                *(s32 *)(bolt + 4) = *(s32 *)(bolt + 4) - effMiscRand(0) % respawnRange;
            }
            continue;
        }
        instance = *(u8 **)bolt;
        rows = (*(s16 *)(instance + 8) / 5) >> 1;
        RpGeometryLock(*(RpGeometry **)(*(u8 **)(instance + 0x10) + 0x18), 2);
        vertices = *(u8 **)(*(u8 **)(*(u8 **)(*(u8 **)(instance + 0x10) + 0x18) + 0x5C) + 0x14);
        effectVuLoad10(&up);
        effectVuLoad11(&direction);
        p4_vu0_opmula(10, 11);
        p4_vu0_opmsub(10, 11, 10);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        effectVuStore10(&side);
        phase = period * effMiscRandFloat(0);
        cycle = phase - period;
        sample = wave * sinf(phase);
        effectVuLoad10(&side);
        effectVuScale10(sample);
        effectVuStore10(&position);
        current = wave * sinf(phase);
        {
            f32 rise = current - sample;

            effectVuLoad10(&direction);
            effectVuScale10(spacing);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuLoad10(&side);
            effectVuScale10(rise);
        }
        p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
        p4_vu0_move(12, P4_VU_XYZW, 10);
        effectVuLoad11(&position);
        p4_vu0_add(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
        effectVuStore11(&nextPosition);
        p4_vu0_move(10, P4_VU_XYZW, 12);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        effectVuStore10(&tangent);
        sample = four * (twoPi / (f32)rows);
        for (row = 0; row < rows; row++) {
            u8 *nextRow;

            effectVuLoad10(&tangent);
            effectVuLoad11(&up);
            p4_vu0_opmula(10, 11);
            p4_vu0_opmsub(10, 11, 10);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
            p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            effectVuStore10(&tangent);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            effectVuLoad10(&position);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x18) = D_00713D10[0];
            *(f32 *)(vertices + 0x1C) = D_00713D14[0];
            *(f32 *)(vertices + 0x20) = D_00713D18[0];
            p4_vu0_move(10, P4_VU_XYZW, 11);
            effectVuLoad11(&innerExtent);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuLoad10(&position);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0xC) = D_00713D10[0];
            *(f32 *)(vertices + 0x10) = D_00713D14[0];
            *(f32 *)(vertices + 0x14) = D_00713D18[0];
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_sub(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 11);
            *(f32 *)(vertices + 0x24) = D_00713D10[0];
            *(f32 *)(vertices + 0x28) = D_00713D14[0];
            *(f32 *)(vertices + 0x2C) = D_00713D18[0];
            effectVuLoad10(&tangent);
            effectVuLoad11(&outerExtent);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuLoad10(&position);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x0) = D_00713D10[0];
            *(f32 *)(vertices + 0x4) = D_00713D14[0];
            *(f32 *)(vertices + 0x8) = D_00713D18[0];
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_sub(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 11);
            *(f32 *)(vertices + 0x30) = D_00713D10[0];
            *(f32 *)(vertices + 0x34) = D_00713D14[0];
            *(f32 *)(vertices + 0x38) = D_00713D18[0];
            nextRow = vertices + 0x3C;
            /* Retail copies these quads through $2 (lq/sq); a struct copy is equivalent. */
            previousTangent = tangent;
            position = nextPosition;
            effectVuLoad10(&position);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x54) = D_00713D10[0];
            *(f32 *)(vertices + 0x58) = D_00713D14[0];
            *(f32 *)(vertices + 0x5C) = D_00713D18[0];
            effectVuLoad10(&tangent);
            effectVuLoad11(&innerExtent);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuLoad10(&position);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x48) = D_00713D10[0];
            *(f32 *)(vertices + 0x4C) = D_00713D14[0];
            *(f32 *)(vertices + 0x50) = D_00713D18[0];
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_sub(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 11);
            *(f32 *)(vertices + 0x60) = D_00713D10[0];
            *(f32 *)(vertices + 0x64) = D_00713D14[0];
            *(f32 *)(vertices + 0x68) = D_00713D18[0];
            effectVuLoad10(&tangent);
            effectVuLoad11(&outerExtent);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuLoad10(&position);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x3C) = D_00713D10[0];
            *(f32 *)(vertices + 0x40) = D_00713D14[0];
            *(f32 *)(vertices + 0x44) = D_00713D18[0];
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_sub(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 11);
            *(f32 *)(vertices + 0x6C) = D_00713D10[0];
            *(f32 *)(vertices + 0x70) = D_00713D14[0];
            *(f32 *)(vertices + 0x74) = D_00713D18[0];
            vertices = nextRow + 0x3C;
            phase += sample;
            cycle += sample;
            if (!(cycle <= twoPi)) {
                cycle -= twoPi;
                wave = amplitude * (one - waveRange * effMiscRandFloat(0));
            }
            previous = current;
            current = wave * sinf(phase);
            {
                f32 rise = current - previous;

                effectVuLoad10(&direction);
                effectVuScale10(spacing);
                p4_vu0_move(12, P4_VU_XYZW, 10);
                effectVuLoad10(&side);
                effectVuScale10(rise);
            }
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            effectVuStore10(&step);
            effMiscAxisAngleVU(swing * (two * (effMiscRandFloat(0) - half)), up.lane);
            effectVuLoad10(&step);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuLoad11(&position);
            p4_vu0_add(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
            effectVuStore11(&nextPosition);
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
            p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            effectVuStore10(&tangent);
            {
                f32 dot;
                f32 dotLanes[4];
                u8 *anchor;

                effectVuLoad10(&nextPosition);
                effectVuLoad11(&position);
                p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
                p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
                p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
                p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
                p4_vu0_mulq(10, P4_VU_XYZ, 10);
                effectVuLoad11(&previousTangent);
                /* Retail moves the dot product through $2 (qmfc2/mtc1 bridge). */
                p4_vu0_mul(2, P4_VU_XYZ, 10, 11, P4_VU_NOBC);
                p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
                p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
                p4_vu0_sqc2(dotLanes, 2);
                dot = dotLanes[0];
                if (dot < zero) {
                    anchor = vertices - 0xC;
                    p4_vu0_lqc2(12, &outerExtent);
                } else {
                    anchor = vertices - 0x3C;
                    p4_vu0_lqc2(12, &outerFlip);
                }
                effectVuLoad10(&tangent);
                effectVuLoad11(&up);
                p4_vu0_opmula(10, 11);
                p4_vu0_opmsub(10, 11, 10);
                p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
                p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
                p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
                p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
                p4_vu0_mulq(10, P4_VU_XYZ, 10);
                p4_vu0_move(11, P4_VU_XYZW, 12);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                D_00713D10[0] = *(f32 *)(anchor + 0x0);
                D_00713D14[0] = *(f32 *)(anchor + 0x4);
                D_00713D18[0] = *(f32 *)(anchor + 0x8);
                effectVuLoad11((EffectVuVector *)D_00713D10);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                effectVuStore10(&position);
            }
        }
        vertices = *(u8 **)(*(u8 **)(instance + 0x10) + 0x18);
        func_003c22f0(vertices);
        if ((*(u16 *)instance & 4) != 0) {
            *(u16 *)(vertices + 0xC) |= 1;
        }
advance:
        *(s32 *)(bolt + 4) = *(s32 *)(bolt + 4) + 1;
    }
}
