#include "p4pc/vu0.h"

void ThunderOrbitBoltUpdateStrip(u8 *arg0)
{
    extern void mdlEffectCopyCameraOrigin(f32 *arg0);
    extern f32 sinf(f32 arg0);
    extern f32 cosf(f32 arg0);
    extern f32 sqrtf(f32 arg0);
    extern f32 func_0044b938(f32 arg0);
    extern f32 func_0044b340(f32 arg0);
    extern void effMiscRotateXVU(f32 arg0);
    extern void effMiscRotateYVU(f32 arg0);
    extern void effMiscCombineRotationVU(void);
    extern void effMiscAxisAngleVU(f32 angle, f32 *axis);
    extern void func_003c22f0(u8 *geometry);
    extern f32 fGpffff804c;
    extern f32 fGpffff8084;
    extern f32 fGpffff8098;
    extern f32 fGpffff809c;
    extern f32 iGpffff80d4;
    extern f32 fGpffff80d8;
    extern f32 fGpffff80dc;
    extern f32 fGpffff80e0;
    extern f32 D_00713D10[];
    extern f32 D_00713D14[];
    extern f32 D_00713D18[];
    f32 drift[5];
    f32 widths[5];
    EffectVuVector delta;
    EffectVuVector heading;
    EffectVuVector side;
    EffectVuVector up;
    EffectVuVector point;
    EffectVuVector position;
    EffectVuVector step;
    f32 segmentLength;
    f32 phase;
    f32 wave;
    f32 amplitude;
    f32 previous;
    f32 angle;
    f32 jitter;
    f32 radius;
    f32 zero;
    f32 half;
    f32 two;
    f32 twoPi;
    f32 outer;
    f32 inner;
    f32 phaseStep;
    f32 shift;
    f32 spin;
    f32 dropScale;
    f32 drop;
    f32 stepLength;
    f32 one;
    f32 tilt;
    f32 negone;
    f32 waveRange;
    f32 waveBase;
    f32 dropRange;
    f32 dropBase;
    f32 jitterRange;
    f32 jitterBase;
    f32 orbit;
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
    u8 mode;

    list = *(u8 **)(arg0 + 0x30);
    config = *(u8 **)(arg0 + 0x34);
    bolt = *(u8 **)list;
    if ((*(u32 *)(config + 0x34) < *(u32 *)(arg0 + 0x28)) && (*(u32 *)(config + 0x34) != 0)) {
        return;
    }
    count = *(s32 *)(config + 0x38);
    respawnRange = *(s32 *)(config + 0x40);
    {
        f32 segmentScale = (f32)*(u32 *)(config + 0x3C);

        segmentLength = *(f32 *)(config + 0x84) / segmentScale;
        phaseStep = (fGpffff8084 * (f32)*(u32 *)(config + 0x4C)) / segmentScale;
    }
    amplitude = *(f32 *)(config + 0x50);
    spin = *(f32 *)(config + 0xA8);
    mode = *(u8 *)(config + 0x88);
    if (segmentLength <= 0.0f) {
        return;
    }
    mdlEffectCopyCameraOrigin(up.lane);
    jitter = fGpffff80d8;
    dropScale = -(fGpffff804c * (segmentLength / 10.0f));
    zero = 0.0f;
    widths[0] = 0.0f;
    widths[1] = *(f32 *)(config + 0x80);
    widths[2] = widths[1] + *(f32 *)(config + 0x7C);
    widths[3] = widths[2] + *(f32 *)(config + 0x7C);
    widths[4] = widths[3] + widths[1];
    one = 1.0f;
    delta.lane[3] = 1.0f;
    point.lane[3] = 1.0f;
    radius = *(f32 *)(list + 4);
    *(f32 *)(list + 4) = radius + *(f32 *)(list + 8);
    index = 0;
    half = 0.5f;
    two = 2.0f;
    twoPi = fGpffff8084;
    tilt = fGpffff80dc;
    negone = -1.0f;
    waveRange = iGpffff80d4;
    waveBase = fGpffff80e0;
    dropRange = fGpffff809c;
    dropBase = fGpffff8098;
    jitterRange = 0.75f;
    jitterBase = 0.25f;
    for (; index < count; index++, bolt += 0x30) {
        if (*(s32 *)(bolt + 0x14) == -1) {
            f32 t = *(f32 *)(config + 0xA4);

            *(f32 *)(bolt + 0x24) = *(f32 *)(config + 0xA0) * ((one - t) + t * effMiscRandFloat(0));
            *(f32 *)(bolt + 0x28) = zero;
            if (mode == 0) {
                *(f32 *)(bolt + 0x1C) = twoPi * (two * (effMiscRandFloat(0) - half));
                *(f32 *)(bolt + 0x2C) = zero;
            } else {
                *(f32 *)(bolt + 0x1C) = tilt * (two * (effMiscRandFloat(0) - half));
                *(f32 *)(bolt + 0x2C) = *(f32 *)(config + 0x9C) * effMiscRandFloat(0);
            }
            *(f32 *)(bolt + 0x20) = twoPi * (two * (effMiscRandFloat(0) - half));
            effMiscRotateXVU(*(f32 *)(bolt + 0x1C));
            effMiscRotateYVU(*(f32 *)(bolt + 0x20));
            effMiscCombineRotationVU();
            angle = *(f32 *)(bolt + 0x28);
            point.lane[0] = radius * cosf(angle);
            point.lane[1] = zero;
            point.lane[2] = radius * sinf(angle);
            effectVuLoad10(&point);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            effectVuStore10(&point);
            *(f32 *)(bolt + 0x4) = point.lane[0];
            *(f32 *)(bolt + 0x8) = point.lane[1];
            *(f32 *)(bolt + 0xC) = point.lane[2];
            *(s32 *)(bolt + 0x18) = -1;
            goto advance;
        }
        state = *(u32 *)(bolt + 0x18);
        if ((state & 0xFF000000) > 0x40000000U) {
            *(u32 *)(bolt + 0x18) = state + 0xC0000000;
        } else {
            *(s32 *)(bolt + 0x14) = -1;
            if (respawnRange > 0) {
                *(s32 *)(bolt + 0x14) = *(s32 *)(bolt + 0x14) - effMiscRand(0) % respawnRange;
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

            f32 tilted = func_0044b938(rise / sqrtf(rise * rise + segmentLength * segmentLength));

            inner = *(f32 *)(config + 0x7C);
            outer = inner + *(f32 *)(config + 0x80);
            angle = tilted;
        }
        drop = dropScale * (dropBase + dropRange * effMiscRandFloat(0));
        {
            f32 t = (f32)(*(s32 *)(bolt + 0x14) + 1);

            orbit = t * (*(f32 *)(bolt + 0x24) + half * (spin * t));
            orbit += *(f32 *)(bolt + 0x28);
        }
        effMiscRotateXVU(*(f32 *)(bolt + 0x1C));
        effMiscRotateYVU(*(f32 *)(bolt + 0x20));
        effMiscCombineRotationVU();
        point.lane[0] = radius * cosf(orbit);
        point.lane[1] = zero;
        point.lane[2] = radius * sinf(orbit);
        effectVuLoad10(&point);
        p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
        p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
        effectVuStore10(&point);
        effectVuStore10(&position);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        effectVuStore10(&heading);
        delta.lane[0] = position.lane[0] - *(f32 *)(bolt + 0x4);
        delta.lane[1] = position.lane[1] - *(f32 *)(bolt + 0x8);
        delta.lane[2] = position.lane[2] - *(f32 *)(bolt + 0xC);
        effectVuLoad10(&delta);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        p4_vu0_move(12, P4_VU_XYZW, 10);
        effectVuLoad11(&heading);
        effectVuStore10(&delta);
        p4_vu0_opmula(10, 11);
        p4_vu0_opmsub(10, 11, 10);
        effectVuStore10(&side);
        effectVuLoad11(&up);
        p4_vu0_move(10, P4_VU_XYZW, 12);
        p4_vu0_opmula(10, 11);
        p4_vu0_opmsub(10, 11, 10);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        p4_vu0_move(12, P4_VU_XYZW, 10);
        p4_vu0_move(11, P4_VU_XYZW, 10);
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
        point.lane[1] = point.lane[1] + *(f32 *)(bolt + 0x2C);
        effectVuLoad11(&point);
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
        *(f32 *)(bolt + 0x4) = position.lane[0];
        *(f32 *)(bolt + 0x8) = position.lane[1];
        *(f32 *)(bolt + 0xC) = position.lane[2];
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
                drop = dropScale * (dropBase + dropRange * effMiscRandFloat(0));
            }
            next = wave * sinf(phase);
            {
                f32 rise = next - previous;

                nextAngle = func_0044b938(rise / sqrtf(rise * rise + segmentLength * segmentLength)) + shift;
            }
            effMiscAxisAngleVU(angle, heading.lane);
            effectVuLoad10(&delta);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            effectVuStore10(&step);
            effMiscAxisAngleVU(drop, side.lane);
            effectVuLoad10(&delta);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            effectVuStore10(&delta);
            effectVuLoad10(&position);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            effectVuStore10(&position);
            /* Retail moves the vsqrt result through $2 (cfc2/mtc1 bridge). */
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
            p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
            p4_vu0_sqrt(2, P4_VU_BC_X);
            stepLength = p4pc_vu0.q;
            slope = func_0044b340(half * (nextAngle - angle));
            previous = next;
            angle = nextAngle;
            p4_vu0_lqc2(12, &step);
            for (k = 0; k < 5; k++) {
                u8 *vertex;
                f32 offset = slope * widths[k];

                drift[k] = drift[k] + offset;
                p4_vu0_move(11, P4_VU_XYZW, 12);
                vertex = vertices + k * 12;
                D_00713D10[0] = *(f32 *)(vertex - 0x3C);
                D_00713D14[0] = *(f32 *)(vertex - 0x38);
                D_00713D18[0] = *(f32 *)(vertex - 0x34);
                effectVuLoad10((EffectVuVector *)D_00713D10);
                effectVuScale11(stepLength + drift[k]);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_sqc2(D_00713D10, 10);
                *(f32 *)(vertex + 0x0) = D_00713D10[0];
                *(f32 *)(vertex + 0x4) = D_00713D14[0];
                *(f32 *)(vertex + 0x8) = D_00713D18[0];
                drift[k] = offset;
            }
        }
        vertices = *(u8 **)(*(u8 **)(instance + 0x10) + 0x18);
        func_003c22f0(vertices);
        if ((*(u16 *)instance & 4) != 0) {
            *(u16 *)(vertices + 0xC) |= 1;
        }
advance:
        *(s32 *)(bolt + 0x14) = *(s32 *)(bolt + 0x14) + 1;
    }
}
