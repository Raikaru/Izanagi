#include "p4pc/vu0.h"

// Draws the particle billboards: builds each quad from the projected corners and submits them with the camera near and far planes.
// FUN_00488D70
void ParticleDrawBillboards(u8 *arg0)
{
    extern s32 kwlnGetCamera(void);
    extern void effMiscBuildRotationVU(void);
    extern void TlpSelectFrame(TLP *a, u32 b, TlpSel *c);
    extern BtlShuffleRenderStateSet D_00887300[];
    extern BtlShuffleRenderPrimitive D_00887310[];
    extern f32 cosf(f32);
    extern f32 sinf(f32);
    extern ParticleProjectedVec3 *func_003e42a0(ParticleProjectedVec3 *a, const ParticleProjectedVec3 *b, const BtlShuffleMatrix *c);
    extern void kAssertFail(void *a, s32 b);
    extern f32 fGpffff81f4;
    extern f32 D_008872F8[];
    extern f32 D_008872FC[];
    extern u8 D_00713CD0[];
    extern u8 D_00713D10[];
    ParticleProjectedVec3 tmpPos;
    ParticleProjectedVec3 tmpOut;
    f32 parent[4] __attribute__((aligned(16)));
    ParticleProjectedVertex qb[4];
    u32 snapB[4] __attribute__((aligned(16)));
    u32 snapA[4] __attribute__((aligned(16)));
    u8 base[64] __attribute__((aligned(16)));
    TlpSel outA0;
    u8 *temp_19;
    u8 *var_18;
    s32 temp_16;
    s32 i;
    f32 bufferFar;
    f32 bufferNear;
    f32 cameraNear;
    f32 cameraFar;
    ParticleProjectedColor packed;
    u32 pk;
    f32 colorScale;
    uint32_t colorScaleBits;
    {
        /* Parent colour: unpack the four bytes, scale by fGpffff81f4 on VF2.x, store VF10. */
        s32 inputColor = *(s32 *)(arg0 + 4);
        P4Mmi128 lanes;
        colorScale = fGpffff81f4;
        __builtin_memcpy(&colorScaleBits, &colorScale, sizeof colorScaleBits);
        lanes = p4_vu0_word_bytes_to_lanes((uint32_t)inputColor);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_x(2, colorScaleBits);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_sqc2(parent, 10);
    }
    temp_19 = *(u8 **)(arg0 + 0x4C);
    var_18 = *(u8 **)(temp_19 + 0x18);
    bufferNear = D_008872FC[0];
    bufferFar = D_008872F8[0];
    cameraFar = *(f32 *)((u8 *)(u32)kwlnGetCamera() + 0x84);
    cameraNear = *(f32 *)((u8 *)(u32)kwlnGetCamera() + 0x80);
    temp_16 = *(s32 *)(temp_19 + 8);
    switch (*(u16 *)(arg0 + 0xC)) {
    case 7:
    if ((*(s32 *)(temp_19 + 0xC) & 1) == 0) {
        f32 reciprocalDepth;
        f32 projectedDepth;
        f32 planeSpan;
        i = 0;
        planeSpan = cameraNear - cameraFar;
        bufferFar = bufferFar - bufferNear;
        while (i < temp_16) {
            if (*(s32 *)(var_18 + 0x10) >= 0) {
                s32 cw2 = *(s32 *)(var_18 + 0x14);
                P4Mmi128 lanes;
                /* Particle colour (VF10) times parent colour (VF11), then pack at scale 255. */
                lanes = p4_vu0_word_bytes_to_lanes((uint32_t)cw2);
                p4_vu0_qmtc2(10, &lanes);
                p4_vu0_itof(10, P4_VU_XYZW, 10);
                p4_vu0_set_x(2, colorScaleBits);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                p4_vu0_lqc2(11, parent);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                pk = particlePackAnimatedColor();
                packed.word = pk;
                if (packed.bytes.alpha != 0) {
                    f32 f29;
                    f32 f28;
                    TlpSelectFrame(*(TLP **)(arg0 + 0x48), (u32)*(s32 *)(var_18 + 0x10), &outA0);
                    D_00887300[0](rwRENDERSTATETEXTURERASTER, *(void **)outA0.tex);
                    f29 = *(f32 *)(var_18 + 0x18) * ((outA0.w / 32.0f) * outA0.a);
                    f28 = *(f32 *)(var_18 + 0x18) * ((outA0.h / 32.0f) * outA0.b);
                    qb[0].u.els.color.r = (f32)(u32)packed.bytes.red;
                    qb[0].u.els.color.g = (f32)(u32)packed.bytes.green;
                    qb[0].u.els.color.b = (f32)(u32)packed.bytes.blue;
                    qb[0].u.els.color.a = (f32)(u32)packed.bytes.alpha;
                    qb[1].u.els.color.r = (f32)(u32)packed.bytes.red;
                    qb[1].u.els.color.g = (f32)(u32)packed.bytes.green;
                    qb[1].u.els.color.b = (f32)(u32)packed.bytes.blue;
                    qb[1].u.els.color.a = (f32)(u32)packed.bytes.alpha;
                    qb[2].u.els.color.r = (f32)(u32)packed.bytes.red;
                    qb[2].u.els.color.g = (f32)(u32)packed.bytes.green;
                    qb[2].u.els.color.b = (f32)(u32)packed.bytes.blue;
                    qb[2].u.els.color.a = (f32)(u32)packed.bytes.alpha;
                    qb[3].u.els.color.r = (f32)(u32)packed.bytes.red;
                    qb[3].u.els.color.g = (f32)(u32)packed.bytes.green;
                    qb[3].u.els.color.b = (f32)(u32)packed.bytes.blue;
                    qb[3].u.els.color.a = (f32)(u32)packed.bytes.alpha;
                    tmpPos.x = *(f32 *)(var_18 + 0);
                    tmpPos.y = *(f32 *)(var_18 + 4);
                    tmpPos.z = *(f32 *)(var_18 + 8);
                    func_003e42a0(&tmpOut, &tmpPos, (const BtlShuffleMatrix *)((u8 *)(u32)kwlnGetCamera() + 0x20));
                    {
                        f32 ry = tmpOut.y / tmpOut.z;
                        f32 rx = tmpOut.x / tmpOut.z;
                        s32 bad;
                        if (rx < -2.0f || !(rx <= 2.0f) || ry < -2.0f || !(ry <= 2.0f)) {
                            bad = 1;
                        } else {
                            bad = 0;
                        }
                        if (bad == 0) {
                            f32 a0;
                            f32 a1;
                            f32 b0;
                            f32 b1;
                            projectedDepth = (cameraNear / tmpOut.z) * ((tmpOut.z - cameraFar) * (bufferFar / planeSpan)) + (bufferNear + 0.0f);
                            if (projectedDepth < 0.0f) {
                                projectedDepth = 0.0f;
                            }
                            reciprocalDepth = 1.0f / projectedDepth;
                            a0 = cosf(*(f32 *)(var_18 + 0x1C));
                            a1 = sinf(*(f32 *)(var_18 + 0x1C));
                            b0 = f29 * a0;
                            b1 = f28 * a0;
                            f29 = f29 * a1;
                            f28 = f28 * a1;
                            qb[0].u.els.u = 0.0f; qb[0].u.els.v = 0.0f;
                            qb[1].u.els.u = 1.0f; qb[1].u.els.v = 0.0f;
                            qb[2].u.els.u = 0.0f; qb[2].u.els.v = 1.0f;
                            qb[3].u.els.u = 1.0f; qb[3].u.els.v = 1.0f;
                            {
                                qb[0].u.els.scrVertex.x = 640.0f * ((tmpOut.x + (-b0 + f28)) / tmpOut.z);
                                qb[0].u.els.scrVertex.y = 448.0f * ((tmpOut.y - (-f29 - b1)) / tmpOut.z);
                                qb[0].u.els.scrVertex.z = projectedDepth;
                                qb[0].u.els.recipZ = reciprocalDepth;
                                qb[1].u.els.scrVertex.x = 640.0f * ((tmpOut.x + (b0 + f28)) / tmpOut.z);
                                qb[1].u.els.scrVertex.y = 448.0f * ((tmpOut.y - (f29 - b1)) / tmpOut.z);
                                qb[1].u.els.scrVertex.z = projectedDepth;
                                qb[1].u.els.recipZ = reciprocalDepth;
                                qb[2].u.els.scrVertex.x = 640.0f * ((tmpOut.x + (-b0 - f28)) / tmpOut.z);
                                qb[2].u.els.scrVertex.y = 448.0f * ((tmpOut.y - (-f29 + b1)) / tmpOut.z);
                                qb[2].u.els.scrVertex.z = projectedDepth;
                                qb[2].u.els.recipZ = reciprocalDepth;
                                qb[3].u.els.scrVertex.x = 640.0f * ((tmpOut.x + (b0 - f28)) / tmpOut.z);
                                qb[3].u.els.scrVertex.y = 448.0f * ((tmpOut.y - (f29 + b1)) / tmpOut.z);
                                qb[3].u.els.scrVertex.z = projectedDepth;
                                qb[3].u.els.recipZ = reciprocalDepth;
                            }
                            D_00887310[0](rwPRIMTYPETRISTRIP, qb, 4);
                        }
                    }
                }
            }
            i++;
            var_18 += 0x20;
        }
    } else {
        f32 scale32;
        f32 neg2;
        f32 pos2;
        f32 zero;
        f32 one;
        f32 s640;
        f32 s448;
        mdlEffectLoadQuadB((s32)temp_19, snapA);
        mdlEffectLoadQuadA((s32)temp_19, snapB);
        p4_vu0_lqc2(10, snapA);
        effMiscBuildRotationVU();
        p4_vu0_lqc2(31, snapB);
        p4_vu0_sqc2(base + 0, 28);
        p4_vu0_sqc2(base + 16, 29);
        p4_vu0_sqc2(base + 32, 30);
        p4_vu0_sqc2(base + 48, 31);
        i = 0;
        scale32 = 32.0f;
        neg2 = -2.0f;
        pos2 = 2.0f;
        zero = 0.0f;
        one = 1.0f;
        s640 = 640.0f;
        s448 = 448.0f;
        while (i < temp_16) {
            if (*(s32 *)(var_18 + 0x10) >= 0) {
                /* VF10 = (VF28*x + VF29*y + VF30*z) + VF31 for this record's position. */
                p4_vu0_lqc2(28, base + 0);
                p4_vu0_lqc2(29, base + 16);
                p4_vu0_lqc2(30, base + 32);
                p4_vu0_lqc2(31, base + 48);
                p4_vu0_lqc2(10, var_18);
                p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                p4_vu0_madd(10, P4_VU_XYZW, 31, 0, P4_VU_BC_W);
                p4_vu0_sqc2(D_00713D10, 10);
                tmpPos.x = *(f32 *)(D_00713D10 + 0);
                tmpPos.y = *(f32 *)(D_00713D10 + 4);
                tmpPos.z = *(f32 *)(D_00713D10 + 8);
                {
                    s32 cw3 = *(s32 *)(var_18 + 0x14);
                    P4Mmi128 lanes;
                    /* Particle colour (VF10) times parent colour (VF11), then pack at scale 255. */
                    lanes = p4_vu0_word_bytes_to_lanes((uint32_t)cw3);
                    p4_vu0_qmtc2(10, &lanes);
                    p4_vu0_itof(10, P4_VU_XYZW, 10);
                    p4_vu0_set_x(2, colorScaleBits);
                    p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                    p4_vu0_lqc2(11, parent);
                    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                    pk = particlePackAnimatedColorTransformed(255.0f);
                    packed.word = pk;
                    if (packed.bytes.alpha != 0) {
                        f32 g0;
                        f32 g1;
                        u32 frameIndex;

                        frameIndex = *(const u32 *)(var_18 + 0x10);
                        TlpSelectFrame(*(TLP **)(arg0 + 0x48), frameIndex, &outA0);
                        D_00887300[0](rwRENDERSTATETEXTURERASTER, *(void **)outA0.tex);
                        {
                            f32 particleScale = *(f32 *)(var_18 + 0x18);
                            f32 dimension = outA0.w / scale32;
                            dimension = dimension * outA0.a;
                            g0 = particleScale * dimension;
                            dimension = outA0.h / scale32;
                            dimension = dimension * outA0.b;
                            g1 = particleScale * dimension;
                        }
                        qb[0].u.els.color.r = (f32)(u32)packed.bytes.red;
                        qb[0].u.els.color.g = (f32)(u32)packed.bytes.green;
                        qb[0].u.els.color.b = (f32)(u32)packed.bytes.blue;
                        qb[0].u.els.color.a = (f32)(u32)packed.bytes.alpha;
                        qb[1].u.els.color.r = (f32)(u32)packed.bytes.red;
                        qb[1].u.els.color.g = (f32)(u32)packed.bytes.green;
                        qb[1].u.els.color.b = (f32)(u32)packed.bytes.blue;
                        qb[1].u.els.color.a = (f32)(u32)packed.bytes.alpha;
                        qb[2].u.els.color.r = (f32)(u32)packed.bytes.red;
                        qb[2].u.els.color.g = (f32)(u32)packed.bytes.green;
                        qb[2].u.els.color.b = (f32)(u32)packed.bytes.blue;
                        qb[2].u.els.color.a = (f32)(u32)packed.bytes.alpha;
                        qb[3].u.els.color.r = (f32)(u32)packed.bytes.red;
                        qb[3].u.els.color.g = (f32)(u32)packed.bytes.green;
                        qb[3].u.els.color.b = (f32)(u32)packed.bytes.blue;
                        qb[3].u.els.color.a = (f32)(u32)packed.bytes.alpha;
                        func_003e42a0(&tmpOut, &tmpPos, (const BtlShuffleMatrix *)((u8 *)(u32)kwlnGetCamera() + 0x20));
                        {
                            f32 ry2 = tmpOut.y / tmpOut.z;
                            f32 rx2 = tmpOut.x / tmpOut.z;
                            s32 bad2;
                            if (rx2 < neg2 || !(rx2 <= pos2) || ry2 < neg2 || !(ry2 <= pos2)) {
                                bad2 = 1;
                            } else {
                                bad2 = 0;
                            }
                            if (bad2 == 0) {
                                f32 f10;
                                f32 inv2;
                                f32 e0;
                                f32 e1;
                                f32 f0b;
                                f32 f1b;
                                f10 = (cameraNear / tmpOut.z) * ((tmpOut.z - cameraFar) * ((bufferFar - bufferNear) / (cameraNear - cameraFar))) + (bufferNear + 0.0f);
                                if (f10 < zero) {
                                    f10 = zero;
                                }
                                inv2 = one / f10;
                                e0 = cosf(*(f32 *)(var_18 + 0x1C));
                                e1 = sinf(*(f32 *)(var_18 + 0x1C));
                                f0b = g0 * e0;
                                f1b = g1 * e0;
                                g0 = g0 * e1;
                                g1 = g1 * e1;
                                qb[0].u.els.u = zero; qb[0].u.els.v = zero;
                                qb[1].u.els.u = one; qb[1].u.els.v = zero;
                                qb[2].u.els.u = zero; qb[2].u.els.v = one;
                                qb[3].u.els.u = one; qb[3].u.els.v = one;
                                {
                                    qb[0].u.els.scrVertex.x = s640 * ((tmpOut.x + (-f0b + g1)) / tmpOut.z);
                                    qb[0].u.els.scrVertex.y = s448 * ((tmpOut.y - (-g0 - f1b)) / tmpOut.z);
                                    qb[0].u.els.scrVertex.z = f10;
                                    qb[0].u.els.recipZ = inv2;
                                    qb[1].u.els.scrVertex.x = s640 * ((tmpOut.x + (f0b + g1)) / tmpOut.z);
                                    qb[1].u.els.scrVertex.y = s448 * ((tmpOut.y - (g0 - f1b)) / tmpOut.z);
                                    qb[1].u.els.scrVertex.z = f10;
                                    qb[1].u.els.recipZ = inv2;
                                    qb[2].u.els.scrVertex.x = s640 * ((tmpOut.x + (-f0b - g1)) / tmpOut.z);
                                    qb[2].u.els.scrVertex.y = s448 * ((tmpOut.y - (-g0 + f1b)) / tmpOut.z);
                                    qb[2].u.els.scrVertex.z = f10;
                                    qb[2].u.els.recipZ = inv2;
                                    qb[3].u.els.scrVertex.x = s640 * ((tmpOut.x + (f0b - g1)) / tmpOut.z);
                                    qb[3].u.els.scrVertex.y = s448 * ((tmpOut.y - (g0 + f1b)) / tmpOut.z);
                                    qb[3].u.els.scrVertex.z = f10;
                                    qb[3].u.els.recipZ = inv2;
                                }
                                D_00887310[0](rwPRIMTYPETRISTRIP, qb, 4);
                            }
                        }
                    }
                }
            }
            i++;
            var_18 += 0x20;
        }
    }
    break;
    default:
        kAssertFail(D_00713CD0, 0x4F3);
        break;
    }
    D_00887300[0](rwRENDERSTATETEXTURERASTER, 0);
}
