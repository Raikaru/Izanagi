#include "p4pc/vu0.h"

void mdlEffectUpdateHelixEmitter(u8 *arg0)
{
    extern f32 cosf(f32 arg0);
    extern f32 sinf(f32 arg0);
    extern void mdlEffectSampleKeyState(u8 *arg0, u8 *arg1, s32 arg2, u_long128 *arg3);
    extern void memcpy(void *dst, void *src, u32 size);
    extern void mdlEffectUpdateParticleTrail(u8 *arg0, u8 *arg1);
    extern void effMiscBuildRotationVU(void);
    extern f32 fGpffff807c;
    extern f32 fGpffff8080;
    f32 tmp170[4] __attribute__((aligned(16)));
    f32 tmp160[4] __attribute__((aligned(16)));
    f32 tmp150[4] __attribute__((aligned(16)));
    u_long128 cfg10;
    f32 tmp130[4] __attribute__((aligned(16)));
    u_long128 cfg0;
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
    f32 a;
    f32 eF0val;
    f32 eECval;
    f32 c;
    f32 one;
    f32 angleScale;
    f32 zero;
    f32 negone;
    f32 half;
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
    eF0val = *(f32 *)(config + 240);
    eECval = *(f32 *)(config + 236);
    tmp170[3] = 0.0f;
    tmp130[3] = 0.0f;
    tmp160[3] = 0.0f;
    /* Snapshot the two config quads the rotation pipeline reads. */
    cfg0 = *(u_long128 *)config;
    cfg10 = *(u_long128 *)(config + 16);

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
            f32 scale = (fGpffff807c - *(f32 *)(config + 40)) * effMiscRandFloat(0);
            tmp = *(s32 *)(config + 36);
            ftmp2 = (f32)(u32)tmp;
            *(f32 *)(parent + 20) = *(f32 *)(parent + 20) + ftmp2 * scale;
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

    one = 1.0f;
    angleScale = fGpffff8080;
    zero = 0.0f;
    negone = -1.0f;
    half = 0.5f;

    for (idx = 0; idx < count; idx++, out += 16, nodes += 32) {
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
            /* Live particle: advance along the trail, sample keys, rebuild position. */
            b220buf = *(u_long128 *)nodes;
            ftmp1 = (f32)node10;
            c = ftmp1 * (out[11] + half * (eECval * ftmp1));
            c = c + out[10];
            out[7] = out[7] + out[12];
            out[7] = out[7] - eF0val * ftmp1;
            out[8] = out[8] + out[9];
            tmp130[0] = out[8] * cosf(c);
            tmp130[1] = out[7];
            tmp130[2] = out[8] * sinf(c);
            if ((flags & 1) == 0) {
                tmp150[0] = out[3];
                tmp150[1] = out[4];
                tmp150[2] = out[5];
                tmp150[3] = out[6];
                p4_vu0_lqc2(10, tmp150);
                effMiscBuildRotationVU();
                p4_vu0_lqc2(10, tmp130);
                p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                p4_vu0_sqc2(tmp130, 10);
                tmp130[0] = tmp130[0] + out[0];
                tmp130[1] = tmp130[1] + out[1];
                tmp130[2] = tmp130[2] + out[2];
            }
            *(f32 *)(nodes + 0) = tmp130[0];
            *(f32 *)(nodes + 4) = tmp130[1];
            *(f32 *)(nodes + 8) = tmp130[2];
            mdlEffectSampleKeyState(nodes, config, node10, &b220buf);
            *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out[13];
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out[15];
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out[14];
            mdlEffectUpdateParticleTrail(parent, nodes);
            *(s32 *)(nodes + 16) = node10 + 1;
            continue;
        }
        if (v15 == 0) {
            continue;
        }

        /* Spawn a fresh particle: spread, speed, angle and offset. */
        c = *(f32 *)(config + 208);
        b = effMiscRandFloat(0);
        c = *(f32 *)(config + 204) * ((one - c) + c * b);
        a = *(f32 *)(config + 216);
        b = effMiscRandFloat(0);
        ftmp1 = *(f32 *)(config + 212) * ((one - a) + a * b);
        out[8] = c;
        out[9] = (ftmp1 - c) / (f32)limitB8;
        b = effMiscRandFloat(0);
        out[10] = angleScale * b;
        a = *(f32 *)(config + 224);
        b = effMiscRandFloat(0);
        out[11] = *(f32 *)(config + 220) * ((one - a) + a * b);
        a = *(f32 *)(config + 232);
        b = effMiscRandFloat(0);
        out[12] = *(f32 *)(config + 228) * ((one - a) + a * b);
        tmp170[0] = cosf(out[10]);
        tmp170[1] = zero;
        tmp170[2] = sinf(out[10]);
        b = effMiscRandFloat(0);
        out[7] = *(f32 *)(config + 200) * b;
        tmp160[0] = c;
        tmp160[1] = c;
        tmp160[2] = c;
        p4_vu0_lqc2(10, tmp170);
        p4_vu0_lqc2(11, tmp160);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_set_xf(2, out[7]);
        p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
        if ((flags & 1) == 0) {
            p4_vu0_sqc2(tmp130, 10);
            out[0] = ((f32 *)&cfg0)[0];
            out[1] = ((f32 *)&cfg0)[1];
            out[2] = ((f32 *)&cfg0)[2];
            out[3] = ((f32 *)&cfg10)[0];
            out[4] = ((f32 *)&cfg10)[1];
            out[5] = ((f32 *)&cfg10)[2];
            out[6] = ((f32 *)&cfg10)[3];
            p4_vu0_lqc2(10, &cfg10);
            effMiscBuildRotationVU();
            p4_vu0_lqc2(10, tmp130);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            p4_vu0_lqc2(11, &cfg0);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        }
        p4_vu0_sqc2(nodes, 10);
        c = *(f32 *)(config + 108);
        b = effMiscRandFloat(0);
        out[13] = (one - c) + c * b;
        if (mode9C != 2) {
            c = *(f32 *)(config + 152);
            b = effMiscRandFloat(0);
            out[15] = (one - c) + c * b;
            if (mode9C == 1) {
                b = effMiscRandFloat(0);
                out[14] = angleScale * b;
                if ((effMiscRand(0) & 1) != 0) {
                    out[15] = out[15] * negone;
                }
            } else {
                out[14] = zero;
            }
        } else {
            out[14] = zero;
            out[15] = one;
        }
        *(s32 *)(nodes + 16) = 0;
        b220buf = *(u_long128 *)nodes;

        if (v14 != 0) {
            tmp = (u32)effMiscRand(0) % (u32)limitB8;
            c = (f32)(u32)tmp;
            out[7] = out[7] + out[12] * c;
            out[7] = out[7] - half * (c * (eF0val * c));
            out[8] = out[8] + out[9] * c;
            tmp130[0] = out[8] * cosf(out[10]);
            tmp130[1] = out[7];
            tmp130[2] = out[8] * sinf(out[10]);
            if ((flags & 1) == 0) {
                p4_vu0_lqc2(10, &cfg10);
                effMiscBuildRotationVU();
                p4_vu0_lqc2(10, tmp130);
                p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                p4_vu0_lqc2(11, &cfg0);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            } else {
                p4_vu0_lqc2(10, tmp130);
            }
            p4_vu0_sqc2(nodes, 10);
            *(s32 *)(nodes + 16) = (s32)c;
        }
        mdlEffectSampleKeyState(nodes, config, *(s32 *)(nodes + 16), &b220buf);
        *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out[13];
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out[15];
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out[14];
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
