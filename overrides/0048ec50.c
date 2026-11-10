#include "p4pc/vu0.h"

void EffConeAngleEmitterUpdate(u8 *arg0)
{
    extern f32 cosf(f32 arg0);
    extern f32 sinf(f32 arg0);
    extern void mdlEffectSampleKeyState(u8 *arg0, u8 *arg1, s32 arg2, u_long128 *arg3);
    extern void *memcpy(void *dst, const void *src, u32 size);
    extern void mdlEffectUpdateParticleTrail(u8 *arg0, u8 *arg1);
    extern f32 fGpffff807c;
    extern f32 fGpffff8080;
    extern f32 fGpffff8090;
    extern f32 fGpffff8094;
    EffectVuVector vec130;
    typedef struct MotionScale3 {
        f32 lane[3];
    } __attribute__((aligned(16))) MotionScale3;
    typedef char MotionScale3Size[(sizeof(MotionScale3) == 16) ? 1 : -1];
    MotionScale3 vec120;
    u_long128 b220buf;
    u32 count;
    u32 flags;
    u8 *nodes;
    f32 *out;
    u8 *config;
    u8 *const parent = arg0;
    s32 limitB8;
    s32 saved20;
    u8 mode9C;
    f32 variation;
    f32 transient;
    f32 e4val;
    f32 e0val;
    f32 half;
    f32 two;
    f32 angleScale;
    f32 angleOffset;
    f32 one;
    f32 zero;
    f32 g80;
    f32 negone;
    s32 v15;
    s32 v14;
    u8 initialPreroll;
    u32 idx;
    s32 tmp;
    f32 ftmp1;
    f32 ftmp2;
    s32 node10;
    s32 c0;
    s32 c4;
    s32 nmult;
    u8 *clear;
    s32 ci;
    u32 particleIndex;
    f32 b;

    count = *(u32 *)(parent + 4);
    flags = *(u32 *)(parent + 12);
    nodes = *(u8 **)(parent + 24);
    out = *(f32 **)(parent + 28);
    config = *(u8 **)(parent + 32);
    limitB8 = *(s32 *)(config + 184);
    if (limitB8 == 0) {
        return;
    }
    saved20 = *(s32 *)(config + 32);
    mode9C = *(u8 *)(config + 156);
    e4val = *(f32 *)(config + 228);
    e0val = *(f32 *)(config + 224);
    vec130.lane[3] = 0.0f;
    effectVuLoad10((const EffectVuVector *)(config + 16));
    effMiscBuildRotationVU();

    /* Preroll and emission-count setup. */
    if ((saved20 != 0) && (*(s32 *)(parent + 16) >= saved20)) {
        v14 = 0;
        v15 = 0;
    } else if ((*(s32 *)(parent + 16) == 0) && (*(u8 *)(config + 189) != 0)) {
        initialPreroll = 1;
        v14 = initialPreroll;
        if (!(*(f32 *)(config + 40) <= 0.0f)) {
            f32 prerollScale = (fGpffff807c - *(f32 *)(config + 40)) * effMiscRandFloat(0);
            ftmp2 = (f32)*(u32 *)(parent + 4);
            v15 = (s32)(ftmp2 * prerollScale);
        } else {
            v15 = *(s32 *)(parent + 4);
        }
    } else {
        v14 = 0;
        if (!(*(f32 *)(config + 40) <= 0.0f)) {
            ftmp1 = (fGpffff807c - *(f32 *)(config + 40)) * effMiscRandFloat(0);
            tmp = *(s32 *)(config + 36);
            ftmp2 = (f32)(u32)tmp;
            *(f32 *)(parent + 20) = *(f32 *)(parent + 20) + ftmp2 * ftmp1;
        } else {
            tmp = *(s32 *)(config + 36);
            ftmp1 = (f32)(u32)tmp;
            *(f32 *)(parent + 20) = *(f32 *)(parent + 20) + ftmp1;
        }
        ftmp1 = *(f32 *)(parent + 20);
        ftmp1 = fabsf(ftmp1);
        v15 = (s32)ftmp1;
        *(f32 *)(parent + 20) = *(f32 *)(parent + 20) - (f32)v15;
    }

    half = 0.5f;
    two = 2.0f;
    angleScale = fGpffff8090;
    angleOffset = fGpffff8094;
    one = 1.0f;
    zero = 0.0f;
    g80 = fGpffff8080;
    negone = -1.0f;

    for (idx = 0; idx < count; idx++, out += 7, nodes += 32) {
        if (*(s32 *)(nodes + 16) >= limitB8) {
            tmp = (saved20 != 0) ? -2 : -1;
            *(s32 *)(nodes + 16) = tmp;
            c0 = *(s32 *)(config + 192);
            c4 = *(s32 *)(config + 196);
            nmult = c0 * c4;
            if (nmult != 0) {
                particleIndex = ((u32)nodes - (u32)*(u8 **)(parent + 24)) >> 5;
                clear = *(u8 **)(parent + 24) + ((*(u32 *)(parent + 4) + particleIndex * (u32)nmult) << 5);
                for (ci = 0; ci < nmult; ci++) {
                    *(s32 *)(clear + 16) = -1;
                    clear += 32;
                }
            }
        }

        node10 = *(s32 *)(nodes + 16);
        if (node10 == -2) {
            continue;
        }
        if (node10 != -1) {
            /* Live particle: advance along the cone, sample keys, rebuild position. */
            b220buf = *(u_long128 *)nodes;
            effectVuLoad11((const EffectVuVector *)nodes);
            {
                f32 liveDistance = out[3] + e0val * (f32)node10;
                if (liveDistance < zero) {
                    liveDistance = zero;
                }
                vec130.lane[0] = liveDistance * out[0];
                vec130.lane[1] = liveDistance * out[1];
                vec130.lane[2] = liveDistance * out[2];
            }
            vec130.lane[1] = vec130.lane[1] - e4val * (f32)node10;
            effectVuLoad10(&vec130);
            spawnVuAdd10();
            effectVuStore10((EffectVuVector *)nodes);
            mdlEffectSampleKeyState(nodes, config, node10, &b220buf);
            *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out[4];
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out[6];
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out[5];
            mdlEffectUpdateParticleTrail(parent, nodes);
            *(s32 *)(nodes + 16) = node10 + 1;
            continue;
        }
        if (v15 == 0) {
            continue;
        }

        /* Spawn a fresh particle: angular position on the cone rim, speed and offset. */
        b = effMiscRandFloat(0);
        ftmp1 = two * (b - half);
        transient = angleScale * (half * ((f32)*(s32 *)(config + 232) * ftmp1)) - angleOffset;
        variation = *(f32 *)(config + 212);
        b = effMiscRandFloat(0);
        variation = *(f32 *)(config + 208) * ((one - variation) + variation * b);
        vec130.lane[0] = variation * cosf(transient);
        vec130.lane[1] = -(one - variation);
        vec130.lane[2] = variation * sinf(transient);
        effectVuLoad10(&vec130);
        spawnVuNormalize10();
        if ((flags & 1) == 0) {
            spawnVuTransform10();
        }
        effectVuStore10(&vec130);
        out[0] = vec130.lane[0];
        out[1] = vec130.lane[1];
        out[2] = vec130.lane[2];
        transient = *(f32 *)(config + 220);
        b = effMiscRandFloat(0);
        out[3] = fabsf(*(f32 *)(config + 216) * ((one - transient) + transient * b));
        transient = *(f32 *)(config + 204);
        b = effMiscRandFloat(0);
        vec120.lane[0] = *(f32 *)(config + 200) * ((one - transient) + transient * b);
        vec120.lane[1] = vec120.lane[0];
        vec120.lane[2] = vec120.lane[0];
        vec120.lane[3] = 0.0f;
        effectVuLoad10(&vec130);
        if (*(f32 *)(config + 216) < zero) {
            p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
        }
        if ((flags & 1) != 0) {
            p4_vu0_lqc2(11, &vec120);
            spawnVuMultiply10();
        } else {
            p4_vu0_lqc2(11, &vec120);
            spawnVuMultiply10();
            effectVuLoad11((const EffectVuVector *)config);
            spawnVuAdd10();
        }
        effectVuStore10((EffectVuVector *)nodes);
        transient = *(f32 *)(config + 108);
        b = effMiscRandFloat(0);
        out[4] = (one - transient) + transient * b;
        if (mode9C != 2) {
            transient = *(f32 *)(config + 152);
            b = effMiscRandFloat(0);
            out[6] = (one - transient) + transient * b;
            if (mode9C == 1) {
                b = effMiscRandFloat(0);
                out[5] = g80 * b;
                if ((effMiscRand(0) & 1) != 0) {
                    out[6] = out[6] * negone;
                }
            } else {
                out[5] = zero;
            }
        } else {
            out[5] = zero;
            out[6] = one;
        }
        *(s32 *)(nodes + 16) = 0;
        b220buf = *(u_long128 *)nodes;

        if (v14 != 0) {
            f32 prerollElapsed;
            f32 prerollDistance;
            f32 travelled;
            f32 accelerationDistance;
            f32 accelerationSquaredDistance;
            f32 gravityDistance;
            f32 gravitySquaredDistance;
            tmp = (u32)effMiscRand(0) % (u32)limitB8;
            prerollElapsed = (f32)(u32)tmp;
            effectVuLoad11((const EffectVuVector *)nodes);
            travelled = out[3] * prerollElapsed;
            accelerationDistance = e0val * prerollElapsed;
            accelerationSquaredDistance = prerollElapsed * accelerationDistance;
            prerollDistance = travelled + half * accelerationSquaredDistance;
            if (prerollDistance < zero) {
                prerollDistance = zero;
            }
            vec130.lane[0] = prerollDistance * out[0];
            vec130.lane[1] = prerollDistance * out[1];
            vec130.lane[2] = prerollDistance * out[2];
            gravityDistance = e4val * prerollElapsed;
            gravitySquaredDistance = prerollElapsed * gravityDistance;
            vec130.lane[1] = vec130.lane[1] - half * gravitySquaredDistance;
            effectVuLoad10(&vec130);
            spawnVuAdd10();
            effectVuStore10((EffectVuVector *)nodes);
            *(s32 *)(nodes + 16) = (s32)prerollElapsed;
        }
        mdlEffectSampleKeyState(nodes, config, *(s32 *)(nodes + 16), &b220buf);
        *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out[4];
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out[6];
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out[5];
        if (v14 != 0) {
            nmult = *(s32 *)(config + 192) * *(s32 *)(config + 196);
            if (nmult != 0) {
                particleIndex = ((u32)nodes - (u32)*(u8 **)(parent + 24)) >> 5;
                clear = *(u8 **)(parent + 24) + ((*(u32 *)(parent + 4) + particleIndex * (u32)nmult) << 5);
                memcpy(clear, nodes, 32);
                *(s32 *)(clear + 16) = -1;
            }
            *(s32 *)(nodes + 16) = *(s32 *)(nodes + 16) + 1;
        }
        v15 -= 1;
    }
}
