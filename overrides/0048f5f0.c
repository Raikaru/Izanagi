#include "p4pc/vu0.h"

void EffBezierEmitterUpdate(u8 *arg0)
{
    typedef struct BezierParticleWork {
        f32 control0[3];
        f32 control1[3];
        f32 control2[3];
        f32 offset[3];
        f32 rotationSpeed;
        f32 travelSpeed;
        union { s32 bits; f32 value; } length;
        f32 progress;
        f32 opacity;
        f32 spin;
        f32 scale;
    } BezierParticleWork;
    extern f32 cosf(f32 arg0);
    extern f32 sinf(f32 arg0);
    extern void effMiscAxisAngleVU(void *arg0, f32 arg1);
    extern void mdlEffectSampleKeyState(u8 *arg0, u8 *arg1, s32 arg2, u_long128 *arg3);
    extern void *memcpy(void *dst, const void *src, u32 size);
    extern void mdlEffectUpdateParticleTrail(u8 *arg0, u8 *arg1);
    extern f32 fGpffff807c;
    extern f32 fGpffff8080;
    extern f32 fGpffff8084;
    extern f32 fGpffff8090;
    extern u8 D_00713D00[];
    f32 tmp1C0[4] __attribute__((aligned(16)));
    u_long128 b220buf;
    f32 stack1A0[4] __attribute__((aligned(16)));
    f32 vecA[4] __attribute__((aligned(16)));
    f32 tmp180[4] __attribute__((aligned(16)));
    f32 tmp170[4] __attribute__((aligned(16)));
    f32 vecB2[4] __attribute__((aligned(16)));
    f32 vecB[4] __attribute__((aligned(16)));
    f32 coeff140[4] __attribute__((aligned(16)));
    f32 quad130[4] __attribute__((aligned(16)));
    f32 coeff120[4] __attribute__((aligned(16)));
    f32 quad110[4] __attribute__((aligned(16)));
    f32 ftmp1;
    f32 save30;
    f32 save29;
    f32 angle;
    f32 half, two, orbitScale, hundred, stackDC, one, zero, spinScale, negone, thousand, three;
    f32 vB;
    f32 ccB;
    f32 cc2B;
    f32 c3B;
    f32 vc2B;
    f32 b1B;
    f32 vvB;
    f32 cv2B;
    f32 b2B;
    f32 v3B;
    f32 nf;
    f32 f1B;
    f32 f12B;
    f32 bx;
    f32 by;
    f32 bz;
    u32 count;
    u8 *nodes;
    BezierParticleWork *out;
    u8 *config;
    u8 *const parent = arg0;
    s32 limitB8;
    s32 saved20;
    u8 mode9C;
    s32 v15;
    s32 v14;
    u8 initialPreroll;
    u32 idx;
    s32 tmp;
    s32 tmp2;
    f32 ftmp2;
    f32 ftmp3;
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
    f32 d;

    count = *(u32 *)(parent + 4);
    nodes = *(u8 **)(parent + 24);
    out = *(BezierParticleWork **)(parent + 28);
    config = *(u8 **)(parent + 32);
    limitB8 = *(s32 *)(config + 184);
    if (limitB8 == 0) {
        return;
    }
    saved20 = *(s32 *)(config + 32);
    mode9C = *(u8 *)(config + 156);
    save29 = *(f32 *)(config + 236);
    save30 = *(f32 *)(config + 248);
    stack1A0[0] = *(f32 *)(config + 252);
    stack1A0[1] = *(f32 *)(config + 256);
    stack1A0[2] = *(f32 *)(config + 260);
    stack1A0[3] = 0.0f;
    vecA[3] = 0.0f;
    p4_vu0_lqc2(10, config + 16);
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

    half = 0.5f;
    two = 2.0f;
    orbitScale = fGpffff8084;
    hundred = 100.0f;
    stackDC = fGpffff8090;
    one = 1.0f;
    zero = 0.0f;
    spinScale = fGpffff8080;
    negone = -1.0f;
    thousand = 1000.0f;
    three = 3.0f;

    for (idx = 0; idx < count; idx++, out += 1, nodes += 32) {
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
            /* Live particle: advance along the bezier, sample keys, rebuild position. */
            b220buf = *(u_long128 *)nodes;
            nf = (f32)node10;
            f1B = out->travelSpeed * nf;
            f1B = f1B + half * (nf * (save30 * nf));
            if (f1B < zero) {
                f1B = zero;
            }
            f12B = out->rotationSpeed * nf;
            f12B = f12B + half * (nf * (save29 * nf));
            if (f12B < zero) {
                f12B = zero;
            }
            angle = f1B / thousand;
            if (!(angle <= one)) {
                angle = one;
            }
            ftmp2 = out->progress;
            if (ftmp2 > angle) {
                angle = ftmp2;
            } else {
                out->progress = angle;
            }
            bx = *(f32 *)(config + 264) + out->offset[0];
            by = *(f32 *)(config + 268) + out->offset[1];
            bz = *(f32 *)(config + 272) + out->offset[2];
            vB = angle;
            ccB = one - vB;
            cc2B = ccB * ccB;
            c3B = ccB * cc2B;
            coeff120[0] = c3B;
            vc2B = vB * cc2B;
            b1B = three * vc2B;
            coeff120[1] = b1B;
            vvB = vB * vB;
            cv2B = ccB * vvB;
            b2B = three * cv2B;
            coeff120[2] = b2B;
            v3B = vB * vvB;
            coeff120[3] = v3B;
            quad110[0] = bx * coeff120[3] +
                (*(f32 *)(config + 252) * coeff120[0] + out->control0[0] * coeff120[1] +
                 out->control1[0] * coeff120[2]);
            quad110[1] = by * coeff120[3] +
                (*(f32 *)(config + 256) * coeff120[0] + out->control0[1] * coeff120[1] +
                 out->control1[1] * coeff120[2]);
            quad110[2] = bz * coeff120[3] +
                (*(f32 *)(config + 260) * coeff120[0] + out->control0[2] * coeff120[1] +
                 out->control1[2] * coeff120[2]);
            quad110[3] = 0.0f;
            p4_vu0_lqc2(10, quad110);
            if (zero != f12B) {
                p4_vu0_sqc2(tmp170, 10);
                vecA[0] = out->control2[0];
                vecA[1] = out->control2[1];
                vecA[2] = out->control2[2];
                p4_vu0_lqc2(10, stack1A0);
                p4_vu0_lqc2(11, vecA);
                p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
                p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
                p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
                p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
                p4_vu0_mulq(10, P4_VU_XYZ, 10);
                p4_vu0_sqc2(tmp1C0, 10);
                effMiscAxisAngleVU(tmp1C0, f12B);
                p4_vu0_lqc2(11, tmp1C0);
                p4_vu0_set_xf(2, out->length.value * angle);
                p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                p4_vu0_lqc2(10, tmp170);
                p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            }
            p4_vu0_sqc2(nodes, 10);
            mdlEffectSampleKeyState(nodes, config, node10, &b220buf);
            *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out->opacity;
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out->scale;
            *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out->spin;
            mdlEffectUpdateParticleTrail(parent, nodes);
            *(s32 *)(nodes + 16) = node10 + 1;
            continue;
        }
        if (v15 == 0) {
            continue;
        }

        /* Spawn a fresh particle: orbit offset, frame, control points and spin. */
        b = effMiscRandFloat(0);
        c = two * (b - half);
        angle = orbitScale * c;
        a = *(f32 *)(config + 284) * cosf(angle);
        out->offset[0] = a;
        out->offset[1] = -*(f32 *)(config + 288);
        a = *(f32 *)(config + 284) * sinf(angle);
        out->offset[2] = a;
        vecA[0] = *(f32 *)(config + 264) + out->offset[0];
        vecA[1] = *(f32 *)(config + 268) + out->offset[1];
        vecA[2] = *(f32 *)(config + 272) + out->offset[2];
        p4_vu0_lqc2(12, stack1A0);
        p4_vu0_move(10, P4_VU_XYZW, 12);
        p4_vu0_lqc2(11, vecA);
        {
            s16 h200 = *(s16 *)(config + 200);
            f32 f200 = (f32)h200 / hundred;
            p4_vu0_set_xf(2, f200);
        }
        p4_vu0_sub(3, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
        p4_vu0_madd(10, P4_VU_XYZW, 10, 3, P4_VU_BC_W);
        p4_vu0_sqc2(vecB, 10);
        p4_vu0_move(10, P4_VU_XYZW, 12);
        {
            s16 h212 = *(s16 *)(config + 212);
            f32 f212 = (f32)h212 / hundred;
            p4_vu0_set_xf(2, f212);
        }
        p4_vu0_sub(3, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
        p4_vu0_madd(10, P4_VU_XYZW, 10, 3, P4_VU_BC_W);
        p4_vu0_sqc2(vecB2, 10);
        p4_vu0_move(10, P4_VU_XYZW, 12);
        p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
        p4_vu0_sqrt(2, P4_VU_BC_X);
        tmp2 = p4_vu0_cfc2(22);
        out->length.bits = tmp2;
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
        p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
        p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
        p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
        p4_vu0_mulq(10, P4_VU_XYZ, 10);
        p4_vu0_sqc2(tmp1C0, 10);
        p4_vu0_move(12, P4_VU_XYZW, 10);
        p4_vu0_lqc2(11, D_00713D00);
        p4_vu0_opmula(10, 11);
        p4_vu0_opmsub(10, 11, 10);
        p4_vu0_move(11, P4_VU_XYZW, 12);
        p4_vu0_opmula(10, 11);
        p4_vu0_opmsub(10, 11, 10);
        p4_vu0_sqc2(tmp180, 10);
        {
            f32 fE0;
            b = effMiscRandFloat(0);
            c = two * (b - half);
            fE0 = (f32)*(s16 *)(config + 224) * c;
            ftmp1 = half * fE0;
            effMiscAxisAngleVU(tmp1C0, stackDC * ftmp1);
        }
        {
            angle = *(f32 *)(config + 208);
            b = effMiscRandFloat(0);
            {
                f32 blend208 = (one - angle) + angle * b;
                f32 scale204 = *(f32 *)(config + 204) * blend208;
                p4_vu0_lqc2(10, vecB);
                p4_vu0_lqc2(12, tmp180);
                p4_vu0_move(11, P4_VU_XYZW, 12);
                p4_vu0_set_xf(2, scale204);
                p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 11, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 11, P4_VU_BC_Y);
                p4_vu0_madd(11, P4_VU_XYZW, 30, 11, P4_VU_BC_Z);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_sqc2(tmp170, 10);
                out->control0[0] = tmp170[0];
                out->control0[1] = tmp170[1];
                out->control0[2] = tmp170[2];
            }
        }
        {
            angle = *(f32 *)(config + 220);
            b = effMiscRandFloat(0);
            {
                f32 blend220 = (one - angle) + angle * b;
                f32 scale216 = *(f32 *)(config + 216) * blend220;
                p4_vu0_lqc2(10, vecB2);
                p4_vu0_move(11, P4_VU_XYZW, 12);
                p4_vu0_set_xf(2, scale216);
                p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 11, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 11, P4_VU_BC_Y);
                p4_vu0_madd(11, P4_VU_XYZW, 30, 11, P4_VU_BC_Z);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_sqc2(tmp170, 10);
                out->control1[0] = tmp170[0];
                out->control1[1] = tmp170[1];
                out->control1[2] = tmp170[2];
            }
        }
        angle = *(f32 *)(config + 244);
        b = effMiscRandFloat(0);
        out->travelSpeed = *(f32 *)(config + 240) * ((one - angle) + angle * b);
        angle = *(f32 *)(config + 232);
        b = effMiscRandFloat(0);
        out->rotationSpeed = *(f32 *)(config + 228) * ((one - angle) + angle * b);
        out->progress = zero;
        *(u_long128 *)nodes = *(u_long128 *)stack1A0;
        angle = *(f32 *)(config + 108);
        b = effMiscRandFloat(0);
        out->opacity = (one - angle) + angle * b;
        if (mode9C != 2) {
            angle = *(f32 *)(config + 152);
            b = effMiscRandFloat(0);
            out->scale = (one - angle) + angle * b;
            if (mode9C == 1) {
                b = effMiscRandFloat(0);
                out->spin = spinScale * b;
                if ((effMiscRand(0) & 1) != 0) {
                    out->scale = out->scale * negone;
                }
            } else {
                out->spin = zero;
            }
        } else {
            out->spin = zero;
            out->scale = one;
        }
        *(s32 *)(nodes + 16) = 0;
        b220buf = *(u_long128 *)nodes;

        if (v14 != 0) {
            tmp = (u32)effMiscRand(0) % (u32)limitB8;
            ftmp1 = (f32)(u32)tmp;
            d = out->travelSpeed * ftmp1;
            d = d + half * (ftmp1 * (save30 * ftmp1));
            if (d < zero) {
                d = zero;
            }
            ftmp3 = out->rotationSpeed * ftmp1;
            ftmp3 = ftmp3 + half * (ftmp1 * (save29 * ftmp1));
            if (ftmp3 < zero) {
                ftmp3 = zero;
            }
            angle = d / thousand;
            if (!(angle <= one)) {
                angle = one;
            }
            if (out->progress > angle) {
                angle = out->progress;
            } else {
                out->progress = angle;
            }
            bx = vecA[0];
            by = vecA[1];
            bz = vecA[2];
            vB = angle;
            ccB = one - vB;
            cc2B = ccB * ccB;
            c3B = ccB * cc2B;
            coeff140[0] = c3B;
            vc2B = vB * cc2B;
            b1B = three * vc2B;
            coeff140[1] = b1B;
            vvB = vB * vB;
            cv2B = ccB * vvB;
            b2B = three * cv2B;
            coeff140[2] = b2B;
            v3B = vB * vvB;
            coeff140[3] = v3B;
            quad130[0] = bx * coeff140[3] +
                (*(f32 *)(config + 252) * coeff140[0] + out->control0[0] * coeff140[1] +
                 out->control1[0] * coeff140[2]);
            quad130[1] = by * coeff140[3] +
                (*(f32 *)(config + 256) * coeff140[0] + out->control0[1] * coeff140[1] +
                 out->control1[1] * coeff140[2]);
            quad130[2] = bz * coeff140[3] +
                (*(f32 *)(config + 260) * coeff140[0] + out->control0[2] * coeff140[1] +
                 out->control1[2] * coeff140[2]);
            quad130[3] = 0.0f;
            p4_vu0_lqc2(10, quad130);
            if (zero != ftmp3) {
                p4_vu0_sqc2(tmp170, 10);
                effMiscAxisAngleVU(tmp1C0, ftmp3);
                p4_vu0_lqc2(11, tmp1C0);
                p4_vu0_set_xf(2, out->length.value * angle);
                p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                p4_vu0_lqc2(10, tmp170);
                p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            }
            p4_vu0_sqc2(nodes, 10);
            *(s32 *)(nodes + 16) = (s32)ftmp1;
        }
        mdlEffectSampleKeyState(nodes, config, *(s32 *)(nodes + 16), &b220buf);
        *(f32 *)(nodes + 24) = *(f32 *)(nodes + 24) * out->opacity;
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) * out->scale;
        *(f32 *)(nodes + 28) = *(f32 *)(nodes + 28) + out->spin;
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
