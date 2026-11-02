#include "p4pc/vu0.h"

void ThunderBoltUpdateStrip(u8 *arg0)
{
    extern void mdlEffectCopyCameraOrigin(f32 *arg0);
    extern void effMiscBuildRotationVU(void);
    extern f32 sinf(f32 arg0);
    extern f32 cosf(f32 arg0);
    extern f32 sqrtf(f32 arg0);
    extern f32 func_0044b938(f32 arg0);
    extern void effMiscAxisAngleVU(f32 angle, f32 *axis);
    extern void func_003c22f0(u8 *geometry);
    extern f32 fGpffff8084;
    extern f32 iGpffff8084;
    extern f32 fGpffff80e4;
    extern f32 fGpffff80e8;
    extern f32 fGpffff80ec;
    extern EffectVuVector D_00713D00;
    extern f32 D_00713D10[];
    extern f32 D_00713D14[];
    extern f32 D_00713D18[];
    f32 drift[5];
    f32 widths[5];
    EffectVuVector forward;
    EffectVuVector up;
    EffectVuVector axis;
    f32 segmentLength;
    f32 segmentScale;
    f32 phase;
    f32 wave;
    f32 amplitude;
    f32 previous;
    f32 angle;
    f32 jitter;
    f32 offset;
    f32 length;
    f32 negone;
    f32 zero;
    f32 phaseStep;
    f32 shift;
    f32 base;
    f32 lengthScale;
    f32 lengthSpread;
    f32 hypot;
    f32 one;
    f32 twoPi;
    f32 waveRange;
    f32 waveBase;
    f32 jitterRange;
    f32 jitterBase;
    f32 half;
    f32 next;
    f32 nextAngle;
    u8 *config;
    u8 *bolt;
    u8 *list;
    u32 state;
    s32 index;
    s32 count;
    s32 respawnRange;
    u8 *instance;
    s32 row;
    u8 *vertices;
    s32 rows;
    s32 j;
    s32 k;

    list = *(u8 **)(arg0 + 0x30);
    config = *(u8 **)(arg0 + 0x34);
    bolt = *(u8 **)list;
    if ((*(u32 *)(config + 0x34) < *(u32 *)(arg0 + 0x28)) && (*(u32 *)(config + 0x34) != 0)) {
        return;
    }
    count = *(s32 *)(config + 0x38);
    respawnRange = *(s32 *)(config + 0x40);
    lengthScale = *(f32 *)(config + 0x54);
    lengthSpread = *(f32 *)(config + 0x58);
    segmentScale = (f32)*(u32 *)(config + 0x3C);
    segmentLength = *(f32 *)(config + 0x84) / segmentScale;
    twoPi = fGpffff8084;
    /* Retail reloads the 2pi literal here instead of reusing twoPi. */
    phaseStep = (iGpffff8084 * (f32)*(u32 *)(config + 0x4C)) / segmentScale;
    amplitude = *(f32 *)(config + 0x50);
    if (segmentLength <= 0.0f) {
        return;
    }
    mdlEffectCopyCameraOrigin(up.lane);
    effectVuLoad10((EffectVuVector *)(arg0 + 0x10));
    effMiscBuildRotationVU();
    effectVuLoad10(&D_00713D00);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
    p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
    effectVuStore10(&forward);
    jitter = fGpffff80e4;
    zero = 0.0f;
    widths[0] = 0.0f;
    widths[1] = *(f32 *)(config + 0x80);
    widths[2] = widths[1] + *(f32 *)(config + 0x7C);
    widths[3] = widths[2] + *(f32 *)(config + 0x7C);
    widths[4] = widths[3] + widths[1];
    index = 0;
    one = 1.0f;
    negone = -1.0f;
    waveRange = fGpffff80e8;
    waveBase = fGpffff80ec;
    jitterRange = 0.75f;
    jitterBase = 0.25f;
    half = 0.5f;
    for (; index < count; index++, bolt += 0x10) {
        if (*(s32 *)(bolt + 4) == -1) {
            *(s32 *)(bolt + 8) = -1;
            goto advance;
        }
        length = (lengthScale * ((one - lengthSpread) + lengthSpread * effMiscRandFloat(0))) / segmentScale;
        base = *(f32 *)(bolt + 0xC);
        if ((effMiscRand(0) & 1) != 0) {
            length = length * negone;
        }
        offset = length;
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
        rows = *(s16 *)(instance + 8) / 5;
        RpGeometryLock(*(RpGeometry **)(*(u8 **)(instance + 0x10) + 0x18), 2);
        vertices = *(u8 **)(*(u8 **)(*(u8 **)(*(u8 **)(instance + 0x10) + 0x18) + 0x5C) + 0x14);
        if ((effMiscRand(0) & 1) != 0) {
            amplitude = amplitude * negone;
        }
        phase = twoPi * effMiscRandFloat(0);
        wave = amplitude * (waveBase + waveRange * effMiscRandFloat(0));
        angle = wave * sinf(phase);
        phase += phaseStep;
        previous = wave * sinf(phase);
        {
            f32 rise = previous - angle;
            f32 inner;
            f32 outer;

            angle = func_0044b938(rise / sqrtf(rise * rise + segmentLength * segmentLength));
            inner = *(f32 *)(config + 0x7C);
            outer = inner + *(f32 *)(config + 0x80);
            angle = angle + base;
            effectVuLoad10(&forward);
            effectVuLoad11(&up);
            p4_vu0_opmula(10, 11);
            p4_vu0_opmsub(10, 11, 10);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
            p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuLoad11(&forward);
            p4_vu0_opmula(10, 11);
            p4_vu0_opmsub(10, 11, 10);
            effectVuStore10(&axis);
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_move(11, P4_VU_XYZW, 12);
            effectVuScale10(outer);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x0) = D_00713D10[0];
            *(f32 *)(vertices + 0x4) = D_00713D14[0];
            *(f32 *)(vertices + 0x8) = D_00713D18[0];
            p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x30) = D_00713D10[0];
            *(f32 *)(vertices + 0x34) = D_00713D14[0];
            *(f32 *)(vertices + 0x38) = D_00713D18[0];
            effectVuScale11(inner);
            p4_vu0_sqc2(D_00713D10, 11);
            *(f32 *)(vertices + 0xC) = D_00713D10[0];
            *(f32 *)(vertices + 0x10) = D_00713D14[0];
            *(f32 *)(vertices + 0x14) = D_00713D18[0];
            p4_vu0_sub(11, P4_VU_XYZ, 0, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 11);
            *(f32 *)(vertices + 0x24) = D_00713D10[0];
            *(f32 *)(vertices + 0x28) = D_00713D14[0];
            *(f32 *)(vertices + 0x2C) = D_00713D18[0];
            *(f32 *)(vertices + 0x18) = zero;
            *(f32 *)(vertices + 0x1C) = zero;
            *(f32 *)(vertices + 0x20) = zero;
            p4_vu0_move(11, P4_VU_XYZW, 12);
            effectVuScale11(previous);
        }
        for (j = 0; j < 5; j++) {
            u8 *vertex = vertices + j * 12;
            D_00713D10[0] = *(f32 *)(vertex + 0x0);
            D_00713D14[0] = *(f32 *)(vertex + 0x4);
            D_00713D18[0] = *(f32 *)(vertex + 0x8);
            effectVuLoad10((EffectVuVector *)D_00713D10);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertex + 0x0) = D_00713D10[0];
            *(f32 *)(vertex + 0x4) = D_00713D14[0];
            *(f32 *)(vertex + 0x8) = D_00713D18[0];
            drift[j] = zero;
        }
        vertices += 0x3C;
        for (row = 1; row < rows; row++, vertices += 0x3C) {
            f32 slope;

            shift = jitter * (jitterBase + jitterRange * effMiscRandFloat(0));
            if ((effMiscRand(0) & 1) != 0) {
                jitter = jitter * negone;
            }
            phase += phaseStep;
            if (!(phase < twoPi)) {
                phase -= twoPi;
                amplitude = amplitude * negone;
                wave = amplitude * (waveBase + waveRange * effMiscRandFloat(0));
            }
            next = wave * sinf(phase);
            {
                f32 rise = next - previous;

                hypot = sqrtf(rise * rise + segmentLength * segmentLength);
                nextAngle = func_0044b938(rise / hypot) + (shift + offset) + base;
            }
            effMiscAxisAngleVU(angle, axis.lane);
            effectVuLoad10(&forward);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            previous = half * (nextAngle - angle);
            angle = sinf(previous);
            slope = angle / cosf(previous);
            previous = next;
            angle = nextAngle;
            offset += length;
            p4_vu0_move(12, P4_VU_XYZW, 10);
            for (k = 0; k < 5; k++) {
                u8 *vertex;
                f32 step = slope * widths[k];

                drift[k] = drift[k] + step;
                p4_vu0_move(11, P4_VU_XYZW, 12);
                vertex = vertices + k * 12;
                D_00713D10[0] = *(f32 *)(vertex - 0x3C);
                D_00713D14[0] = *(f32 *)(vertex - 0x38);
                D_00713D18[0] = *(f32 *)(vertex - 0x34);
                effectVuLoad10((EffectVuVector *)D_00713D10);
                effectVuScale11(hypot + drift[k]);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_sqc2(D_00713D10, 10);
                *(f32 *)(vertex + 0x0) = D_00713D10[0];
                *(f32 *)(vertex + 0x4) = D_00713D14[0];
                *(f32 *)(vertex + 0x8) = D_00713D18[0];
                drift[k] = step;
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
