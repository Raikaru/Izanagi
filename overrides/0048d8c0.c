#include "p4pc/vu0.h"

void mdlEffectUpdateOrbitEmitter(u8 *arg0)
{
    extern f32 cosf(f32 arg0);
    extern f32 sinf(f32 arg0);
    extern void mdlEffectSampleKeyState(u8 *arg0, u8 *arg1, s32 arg2, u_long128 *arg3);
    extern void memcpy(void *dst, void *src, u32 size);
    extern void mdlEffectUpdateParticleTrail(u8 *arg0, u8 *arg1);
    extern void effMiscRotateXVU(f32 arg0);
    extern void effMiscRotateYVU(f32 arg0);
    extern void effMiscCombineRotationVU(void);
    extern f32 fGpffff807c;
    extern f32 fGpffff8080;
    extern f32 fGpffff8084;
    extern f32 fGpffff8094;
    f32 sp150[4] __attribute__((aligned(16)));
    f32 sp130[4] __attribute__((aligned(16)));
    u_long128 quad120;
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
    f32 e4val;
    f32 e0val;
    f32 one;
    f32 orbitScale;
    f32 zero;
    f32 half;
    f32 two;
    f32 tiltScale;
    f32 angleScale;
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
    f32 a;
    f32 b;
    f32 c;

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
    sp150[3] = 0.0f;
    sp130[3] = 0.0f;
    /* Snapshot the offset quad that is added to every emitted position. */
    quad120 = *(u_long128 *)config;

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
    orbitScale = fGpffff8094;
    zero = 0.0f;
    half = 0.5f;
    two = 2.0f;
    tiltScale = fGpffff8084;
    angleScale = fGpffff8080;
    negone = -1.0f;

    for (idx = 0; idx < count; idx++, out += 13, nodes += 32) {
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
            /* Live particle: advance its orbit, sample keys, rebuild position. */
            b220buf = *(u_long128 *)nodes;
            ftmp1 = (f32)node10;
            ftmp2 = ftmp1 * (out[7] + half * (e0val * ftmp1));
            ftmp2 = ftmp2 + out[8];
            out[9] = out[9] - e4val * ftmp1;
            out[5] = out[5] + out[6];
            effMiscRotateXVU(out[3]);
            effMiscRotateYVU(out[4]);
            effMiscCombineRotationVU();
            c = out[5];
            sp150[0] = c * cosf(ftmp2);
            sp150[1] = zero;
            sp150[2] = c * sinf(ftmp2);
            p4_vu0_lqc2(10, sp150);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            p4_vu0_sqc2(sp130, 10);
            if ((flags & 1) == 0) {
                sp130[0] = sp130[0] + out[0];
                sp130[1] = sp130[1] + out[1];
                sp130[2] = sp130[2] + out[2];
            }
            *(f32 *)(nodes + 0) = sp130[0];
            *(f32 *)(nodes + 4) = sp130[1] + out[9];
            *(f32 *)(nodes + 8) = sp130[2];
            mdlEffectSampleKeyState(nodes, config, node10, &b220buf);
            *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out[10];
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out[12];
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out[11];
            mdlEffectUpdateParticleTrail(parent, nodes);
            *(s32 *)(nodes + 16) = node10 + 1;
            continue;
        }
        if (v15 == 0) {
            continue;
        }

        /* Spawn a fresh particle: orbit radius, tilt, and offset. */
        c = *(f32 *)(config + 204);
        b = effMiscRandFloat(0);
        c = *(f32 *)(config + 200) * ((one - c) + c * b);
        a = *(f32 *)(config + 212);
        b = effMiscRandFloat(0);
        ftmp1 = *(f32 *)(config + 208) * ((one - a) + a * b);
        out[5] = c;
        out[6] = (ftmp1 - c) / (f32)limitB8;
        a = *(f32 *)(config + 220);
        b = effMiscRandFloat(0);
        out[7] = *(f32 *)(config + 216) * ((one - a) + a * b);
        b = effMiscRandFloat(0);
        out[8] = orbitScale * b;
        out[9] = zero;
        b = effMiscRandFloat(0);
        out[3] = tiltScale * (two * (b - half));
        b = effMiscRandFloat(0);
        out[4] = tiltScale * (two * (b - half));
        effMiscRotateXVU(out[3]);
        effMiscRotateYVU(out[4]);
        effMiscCombineRotationVU();
        sp150[0] = c * cosf(out[8]);
        sp150[1] = zero;
        sp150[2] = c * sinf(out[8]);
        p4_vu0_lqc2(10, sp150);
        p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
        p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
        if ((flags & 1) == 0) {
            out[0] = ((f32 *)&quad120)[0];
            out[1] = ((f32 *)&quad120)[1];
            out[2] = ((f32 *)&quad120)[2];
            p4_vu0_lqc2(11, &quad120);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        }
        p4_vu0_sqc2(nodes, 10);
        c = *(f32 *)(config + 108);
        b = effMiscRandFloat(0);
        out[10] = (one - c) + c * b;
        if (mode9C != 2) {
            c = *(f32 *)(config + 152);
            b = effMiscRandFloat(0);
            out[12] = (one - c) + c * b;
            if (mode9C == 1) {
                b = effMiscRandFloat(0);
                out[11] = angleScale * b;
                if ((effMiscRand(0) & 1) != 0) {
                    out[12] = out[12] * negone;
                }
            } else {
                out[11] = zero;
            }
        } else {
            out[11] = zero;
            out[12] = one;
        }
        *(s32 *)(nodes + 16) = 0;
        b220buf = *(u_long128 *)nodes;

        if (v14 != 0) {
            tmp = (u32)effMiscRand(0) % (u32)limitB8;
            ftmp1 = (f32)(u32)tmp;
            out[9] = out[9] - half * (ftmp1 * (e4val * ftmp1));
            c = out[5] + out[6] * ftmp1;
            out[5] = c;
            sp150[0] = c * cosf(out[8]);
            sp150[1] = zero;
            sp150[2] = c * sinf(out[8]);
            p4_vu0_lqc2(10, sp150);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            if ((flags & 1) == 0) {
                p4_vu0_move(11, P4_VU_XYZW, 10);
                p4_vu0_lqc2(10, &quad120);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            }
            p4_vu0_sqc2(nodes, 10);
            *(f32 *)(nodes + 4) = *(f32 *)(nodes + 4) + out[9];
            *(s32 *)(nodes + 16) = (s32)ftmp1;
        }
        mdlEffectSampleKeyState(nodes, config, *(s32 *)(nodes + 16), &b220buf);
        *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out[10];
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out[12];
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out[11];
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
