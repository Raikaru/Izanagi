#include "p4pc/vu0.h"

/* VU0 boundary: the colour unpack/scale and the 255-scale pack run on the
 * shared VU0 model. The per-record transform saves and reloads vf28..vf31
 * through snap[], because the PS2 callees can clobber the VF registers. */
void ObjectParticleDraw(u8 *arg0)
{
    extern u_long128 mdlEffectLoadQuadB(int param_1, u32 *param_2);
    extern u_long128 mdlEffectLoadQuadA(int param_1, u32 *param_2);
    extern void effMiscBuildRotationVU(void);
    extern void mdlEffectMultiplyPaletteColorVU(u8 *a, u8 *b, s32 c);
    extern void *mdlEffectSetAllColors(void *object, void *data);
    extern u8 *func_003bfe90(u8 *arg0);
    extern void RwMatrixMultiply(EffObjectMatrix *dst, EffObjectMatrix *a, EffObjectMatrix *b);
    extern void RwMatrixRotate(EffObjectMatrix *m, f32 *axis, f32 angle, s32 op);
    extern void RwMatrixScale(EffObjectMatrix *m, f32 *scale, s32 op);
    extern void RwMatrixTranslate(EffObjectMatrix *m, f32 *pos, s32 op);
    extern void func_003e9cb0(void *a, EffObjectMatrix *b, s32 c);
    extern void TnInvokeCallback(void);
    extern f32 fGpffff81f4;
    extern f32 fGpffff8048;
    extern f32 D_00713D20[];
    extern f32 D_00713D24[];
    extern f32 D_00713D28[];
    extern f32 D_00713D10[];
    extern f32 D_00713D14[];
    extern f32 D_00713D18[];
    extern EffectVuVector D_00713CE0;
    EffObjectRGBA packedArg;
    u32 colorWord;
    u32 colorA;
    u32 colorB;
    EffObjectRGBA packedA;
    EffObjectRGBA packedB;
    f32 vec[3] __attribute__((aligned(16)));
    f32 pos[3] __attribute__((aligned(16)));
    f32 axisA[3] __attribute__((aligned(16)));
    f32 axisB[3] __attribute__((aligned(16)));
    EffectVuVector colorBase;
    EffectVuVector snapB;
    EffectVuVector snapA;
    EffectVuVector snap[4];
    EffObjectMatrix matB;
    EffObjectMatrix base;
    EffObjectMatrix matA;
    f32 inv;
    f32 cscale;
    f32 full;
    f32 half;
    f32 zero;
    f32 one;
    f32 gscale;
    s32 i;
    ObjectParticleInstance *self;
    u8 *list;
    s32 count;
    u8 *p18;
    u8 *p17;
    u8 *clump;

    self = (ObjectParticleInstance *)arg0;
    colorWord = self->packedColour;
    {
        const u32 *word = &colorWord;

        cscale = fGpffff81f4;
        effectVuUnpackColor10V0(word, cscale);
    }
    effectVuStore10(&colorBase);
    list = self->list;
    count = *(s32 *)(list + 8);
    if (count != 0 && *(s32 *)(list + 0x10) != 0) {
    p18 = *(u8 **)(list + 0x18);
    p17 = self->entries;
    clump = self->clump;
    {
        f32 v = self->field_0x08;

        if (v != 0.0f) {
            one = 1.0f;
            inv = one / v;
        } else {
            one = 1.0f;
            inv = one;
        }
    }
    switch (self->kind) {
    case 3:
        if ((*(s32 *)(list + 0xC) & 1) == 0) {
            mdlEffectLoadQuadB((int)list, (u32 *)&snapA);
            effectVuLoad10(&snapA);
            effMiscBuildRotationVU();
            p4_vu0_sqc2(&snap[0], 28);
            p4_vu0_sqc2(&snap[1], 29);
            p4_vu0_sqc2(&snap[2], 30);
            p4_vu0_sqc2(&snap[3], 31);
            base.right[0] = snap[0].lane[0];
            base.right[1] = snap[0].lane[1];
            base.right[2] = snap[0].lane[2];
            base.up[0] = snap[1].lane[0];
            base.up[1] = snap[1].lane[1];
            base.up[2] = snap[1].lane[2];
            base.at[0] = snap[2].lane[0];
            base.at[1] = snap[2].lane[1];
            base.at[2] = snap[2].lane[2];
            base.pos[0] = snap[3].lane[0];
            base.pos[1] = snap[3].lane[1];
            base.pos[2] = snap[3].lane[2];
            for (i = 0; i < count; i++, p18 += 0x20, p17 += 0x18) {
                s32 life = *(s32 *)(p18 + 0x10);

                if (life >= 0) {
                    f32 f;

                    colorA = *(u32 *)(p18 + 0x14);
                    effectVuUnpackColor10V0(&colorA, cscale);
                    effectVuLoad11(&colorBase);
                    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                    if (*(s8 *)(p17 + 0x14) >= 0) {
                        mdlEffectMultiplyPaletteColorVU((u8 *)self, p17, life);
                    }
                    {
                        P4Mmi128 zeroMmi = {0};
                        P4Mmi128 r;

                        p4_vu0_set_x(2, 0x437F0000U);
                        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                        p4_vu0_ftoi(10, P4_VU_XYZW, 10);
                        p4_vu0_qmfc2(&r, 10);
                        r = p4_mmi_ppach(zeroMmi, r);
                        r = p4_mmi_ppacb(zeroMmi, r);
                        *(u32 *)&packedA = r.w[0];
                    }
                    *(u32 *)&packedArg = *(u32 *)&packedA;
                    RpClumpForAllAtomics(clump, (KClumpCallback)mdlEffectSetAllColors, &packedArg);
                    {
                        f32 age = (f32)*(s32 *)(p18 + 0x10);

                        f = *(f32 *)(p17 + 0x10) * age;
                        f = f + 0.5f * (age * (self->ageFactor * age));
                    }
                    if (f < 0.0f) {
                        matA.right[0] = matA.up[1] = matA.at[2] = 1.0f;
                        matA.right[1] = matA.right[2] = matA.up[0] = 0.0f;
                        matA.up[2] = matA.at[0] = matA.at[1] = 0.0f;
                        matA.pos[0] = matA.pos[1] = matA.pos[2] = 0.0f;
                        matA.flags = matA.flags | 0x20003;
                    } else {
                        axisA[0] = *(f32 *)(p17 + 0);
                        axisA[1] = *(f32 *)(p17 + 4);
                        axisA[2] = *(f32 *)(p17 + 8);
                        RwMatrixRotate(&matA, axisA, fGpffff8048 * (f + *(f32 *)(p17 + 0xC)), 0);
                    }
                    RwMatrixMultiply(&matB, &matA, &base);
                    if (self->field_0x30 == 0) {
                        vec[0] = D_00713D20[0] * *(f32 *)(p18 + 0x18);
                        vec[1] = D_00713D24[0] * *(f32 *)(p18 + 0x18);
                        vec[2] = D_00713D28[0] * *(f32 *)(p18 + 0x18);
                    } else {
                        f32 s = *(f32 *)(p18 + 0x18) * inv;

                        vec[0] = D_00713D20[0] * s;
                        vec[1] = D_00713D24[0] * s;
                        vec[2] = D_00713D28[0] * s;
                    }
                    RwMatrixScale(&matB, vec, 2);
                    vec[0] = *(f32 *)(p18 + 0);
                    vec[1] = *(f32 *)(p18 + 4);
                    vec[2] = *(f32 *)(p18 + 8);
                    RwMatrixTranslate(&matB, vec, 2);
                    func_003e9cb0(*(void **)(clump + 4), &matB, 0);
                    func_003bfe90(clump);
                }
            }
        } else {
            u32 mask;

            mdlEffectLoadQuadB((int)list, (u32 *)&snapA);
            mdlEffectLoadQuadA((int)self->list, (u32 *)&snapB);
            effectVuLoad10(&snapA);
            effMiscBuildRotationVU();
            p4_vu0_lqc2(31, &snapB);
            p4_vu0_sqc2(&snap[0], 28);
            p4_vu0_sqc2(&snap[1], 29);
            p4_vu0_sqc2(&snap[2], 30);
            p4_vu0_sqc2(&snap[3], 31);
            i = 0;
            full = 255.0f;
            half = 0.5f;
            zero = 0.0f;
            mask = 0x20003;
            gscale = fGpffff8048;
            for (; i < count; i++, p18 += 0x20, p17 += 0x18) {
                if (*(s32 *)(p18 + 0x10) >= 0) {
                    f32 f;

                    p4_vu0_lqc2(28, &snap[0]);
                    p4_vu0_lqc2(29, &snap[1]);
                    p4_vu0_lqc2(30, &snap[2]);
                    p4_vu0_lqc2(31, &snap[3]);
                    effectVuLoad10((EffectVuVector *)p18);
                    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                    p4_vu0_madd(10, P4_VU_XYZW, 31, 0, P4_VU_BC_W);
                    p4_vu0_sqc2(D_00713D10, 10);
                    pos[0] = D_00713D10[0];
                    pos[1] = D_00713D14[0];
                    pos[2] = D_00713D18[0];
                    colorB = *(u32 *)(p18 + 0x14);
                    effectVuUnpackColor10V0(&colorB, cscale);
                    effectVuLoad11(&colorBase);
                    p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                    if (*(s8 *)(p17 + 0x14) >= 0) {
                        mdlEffectMultiplyPaletteColorVU((u8 *)self, p17, *(s32 *)(p18 + 0x10));
                    }
                    effectVuScale10(full);
                    {
                        P4Mmi128 zeroMmi = {0};
                        P4Mmi128 r;

                        p4_vu0_ftoi(10, P4_VU_XYZW, 10);
                        p4_vu0_qmfc2(&r, 10);
                        r = p4_mmi_ppach(zeroMmi, r);
                        r = p4_mmi_ppacb(zeroMmi, r);
                        *(u32 *)&packedB = r.w[0];
                    }
                    *(u32 *)&packedArg = *(u32 *)&packedB;
                    RpClumpForAllAtomics(clump, (KClumpCallback)mdlEffectSetAllColors, &packedArg);
                    {
                        f32 age = (f32)*(s32 *)(p18 + 0x10);

                        f = *(f32 *)(p17 + 0x10) * age;
                        f = f + half * (age * (self->ageFactor * age));
                    }
                    if (f < zero) {
                        matB.right[0] = matB.up[1] = matB.at[2] = one;
                        matB.right[1] = matB.right[2] = matB.up[0] = zero;
                        matB.up[2] = matB.at[0] = matB.at[1] = zero;
                        matB.pos[0] = matB.pos[1] = matB.pos[2] = zero;
                        matB.flags = matB.flags | mask;
                    } else {
                        axisB[0] = *(f32 *)(p17 + 0);
                        axisB[1] = *(f32 *)(p17 + 4);
                        axisB[2] = *(f32 *)(p17 + 8);
                        RwMatrixRotate(&matB, axisB, gscale * (f + *(f32 *)(p17 + 0xC)), 0);
                    }
                    effectVuLoad10(&D_00713CE0);
                    if (self->field_0x30 == 0) {
                        vec[0] = D_00713D20[0] * *(f32 *)(p18 + 0x18);
                        vec[1] = D_00713D24[0] * *(f32 *)(p18 + 0x18);
                        vec[2] = D_00713D28[0] * *(f32 *)(p18 + 0x18);
                    } else {
                        f32 s = *(f32 *)(p18 + 0x18) * inv;

                        vec[0] = D_00713D20[0] * s;
                        vec[1] = D_00713D24[0] * s;
                        vec[2] = D_00713D28[0] * s;
                    }
                    RwMatrixScale(&matB, vec, 2);
                    RwMatrixTranslate(&matB, pos, 2);
                    func_003e9cb0(*(void **)(clump + 4), &matB, 0);
                    func_003bfe90(clump);
                }
            }
        }
        break;
    default:
        kAssertFail(D_00714520, 0x2A2);
        break;
    }
    TnInvokeCallback();
    }
}
