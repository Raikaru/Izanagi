#include "p4pc/vu0.h"

void mdlEffectUpdateFloorEmitter(u8 *arg0)
{
    extern void mdlEffectSampleKeyState(u8 *arg0, u8 *arg1, s32 arg2, u_long128 *arg3);
    extern void mdlEffectUpdateParticleTrail(u8 *arg0, u8 *arg1);
    extern void effMiscBuildRotationVU(void);
    extern f32 fabsf(f32 arg0);
    extern f32 fGpffff807c;
    extern f32 fGpffff8080;
    EffectVuVector vec120;
    EffectVuVector vec110;
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
    f32 eE8;
    f32 eE4;
    f32 eF4;
    f32 eD8;
    f32 one;
    f32 half;
    f32 two;
    f32 zero;
    f32 g80;
    f32 negone;
    f32 ftmp1;
    f32 ftmp2;
    f32 b;
    s32 v15;
    u32 idx;
    s32 tmp;
    s32 node10;
    s32 c0;
    s32 c4;
    s32 nmult;
    u8 *clear;
    s32 ci;
    u32 particleIndex;

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
    eE8 = *(f32 *)(config + 232);
    eF4 = *(f32 *)(config + 244);
    eE4 = *(f32 *)(config + 228);
    eD8 = *(f32 *)(config + 216);
    vec120.lane[3] = 0.0f;
    effectVuLoad10((const EffectVuVector *)(config + 16));
    effMiscBuildRotationVU();

    if ((saved20 != 0) && (*(s32 *)(parent + 16) >= saved20)) {
        v15 = 0;
    } else if ((*(s32 *)(parent + 16) == 0) && (*(u8 *)(config + 189) != 0)) {
        if (!(*(f32 *)(config + 40) <= 0.0f)) {
            f32 prerollScale = (fGpffff807c - *(f32 *)(config + 40)) * effMiscRandFloat(0);
            ftmp2 = (f32)*(u32 *)(parent + 4);
            v15 = (s32)(ftmp2 * prerollScale);
        } else {
            v15 = *(s32 *)(parent + 4);
        }
    } else {
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
        ftmp1 = fabsf(*(f32 *)(parent + 20));
        v15 = (s32)ftmp1;
        *(f32 *)(parent + 20) = *(f32 *)(parent + 20) - (f32)v15;
    }

    one = 1.0f;
    half = 0.5f;
    two = 2.0f;
    zero = 0.0f;
    g80 = fGpffff8080;
    negone = -1.0f;

    for (idx = 0; idx < count; idx++, out += 8, nodes += 32) {
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
            /* Live particle: advance position, bounce off the floor, resample keys. */
            f32 liveDistance;
            f32 height;

            b220buf = *(u_long128 *)nodes;
            effectVuLoad11((const EffectVuVector *)nodes);
            liveDistance = out[3] + eE4 * (f32)node10;
            if (liveDistance < zero) {
                liveDistance = zero;
            }
            out[1] = out[1] - eE8;
            vec120.lane[0] = liveDistance * out[0];
            vec120.lane[1] = out[1];
            vec120.lane[2] = liveDistance * out[2];
            effectVuLoad10(&vec120);
            spawnVuAdd10();
            effectVuStore10((EffectVuVector *)nodes);
            /* The retail qmfc2/prot3w/mtc1 sequence reads the rotated lane 1 (Y) of VF10. */
            height = p4pc_vu0.vf[10].f[1];
            if (height < eD8) {
                out[4] = out[4] * eF4;
                out[1] = out[1] * out[4];
                ftmp1 = out[4];
                *(f32 *)(nodes + 4) = eD8 + ftmp1 * (*(f32 *)(nodes + 4) - eD8);
            }
            mdlEffectSampleKeyState(nodes, config, node10, &b220buf);
            *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out[5];
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out[7];
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out[6];
            mdlEffectUpdateParticleTrail(parent, nodes);
            *(s32 *)(nodes + 16) = node10 + 1;
            continue;
        }
        if (v15 == 0) {
            continue;
        }

        /* Spawn a fresh particle: direction, speed, scale and spin. */
        variation = *(f32 *)(config + 212);
        b = effMiscRandFloat(0);
        variation = *(f32 *)(config + 208) * ((one - variation) + variation * b);
        vec120.lane[0] = variation * ((effMiscRandFloat(0) - half) * two);
        vec120.lane[1] = -(one - variation);
        vec120.lane[2] = variation * ((effMiscRandFloat(0) - half) * two);
        effectVuLoad10(&vec120);
        spawnVuNormalize10();
        if ((flags & 1) == 0) {
            spawnVuTransform10();
        }
        effectVuStore10(&vec120);
        out[0] = vec120.lane[0];
        out[1] = vec120.lane[1];
        out[2] = vec120.lane[2];
        variation = *(f32 *)(config + 224);
        b = effMiscRandFloat(0);
        out[3] = fabsf(*(f32 *)(config + 220) * ((one - variation) + variation * b));
        variation = *(f32 *)(config + 240);
        b = effMiscRandFloat(0);
        ftmp1 = (one - variation) + variation * b;
        out[4] = -*(f32 *)(config + 236) * ftmp1;
        variation = *(f32 *)(config + 204);
        b = effMiscRandFloat(0);
        vec110.lane[0] = *(f32 *)(config + 200) * ((one - variation) + variation * b);
        vec110.lane[1] = vec110.lane[0];
        vec110.lane[2] = vec110.lane[0];
        vec110.lane[3] = 0.0f;
        effectVuLoad10(&vec120);
        if (*(f32 *)(config + 220) < zero) {
            p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
        }
        if ((flags & 1) != 0) {
            effectVuLoad11(&vec110);
            spawnVuMultiply10();
        } else {
            effectVuLoad11(&vec110);
            spawnVuMultiply10();
            effectVuLoad11((const EffectVuVector *)config);
            spawnVuAdd10();
        }
        effectVuStore10((EffectVuVector *)nodes);
        variation = *(f32 *)(config + 108);
        b = effMiscRandFloat(0);
        out[5] = (one - variation) + variation * b;
        if (mode9C != 2) {
            variation = *(f32 *)(config + 152);
            b = effMiscRandFloat(0);
            out[7] = (one - variation) + variation * b;
            if (mode9C == 1) {
                b = effMiscRandFloat(0);
                out[6] = g80 * b;
                if ((effMiscRand(0) & 1) != 0) {
                    out[7] = out[7] * negone;
                }
            } else {
                out[6] = zero;
            }
        } else {
            out[6] = zero;
            out[7] = one;
        }
        *(s32 *)(nodes + 16) = 0;
        mdlEffectSampleKeyState(nodes, config, 0, (u_long128 *)nodes);
        *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out[5];
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out[7];
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out[6];
        v15 -= 1;
    }
}
