#include "p4pc/vu0.h"

/* VU0 transform used by both loops (macro-mode instruction sequence):
 *   ACC      = VF28 * VF10.x
 *   ACC     += VF29 * VF10.y
 *   ACC     += VF30 * VF10.z
 *   VF10     = ACC  + VF31 * VF0.w      (VF0.w is 1.0, so this adds VF31)
 * The rotation rows VF28..VF30 and translation row VF31 are re-loaded from the
 * transform copy before each record, since the child callbacks can use VU0. */
void ParticleUpdateScaled(u8* particle, f32 scale)
{
    f32 transformedPosition[4] __attribute__((aligned(16)));
    u32 translationSnapshot[4] __attribute__((aligned(16)));
    u32 rotationSnapshot[4] __attribute__((aligned(16)));
    f32 transform[4][4] __attribute__((aligned(16)));
    s32 particleCount;
    u16 subtype;
    s32 emitterAddress;
    ParticleUpdateRecord* record;

    emitterAddress = *(s32*)(particle + 0x4C);
    subtype = *(u16*)(particle + 0xC);
    switch (subtype)
    {
    case 5:
        record = *(ParticleUpdateRecord**)((u8*)emitterAddress + 0x18);
        /* The object-array traversal starts after the record-base lookup. */
        {
            u8** objects = *(u8***)(particle + 0x34);
            particleCount = *(s32*)((u8*)emitterAddress + 8);
            if ((*(u32*)((u8*)emitterAddress + 0xC) & 1) == 0)
            {
                {
                    /* This traversal starts only after its emitter/setup phase. */
                    s32 index = 0;
                    while (index < particleCount)
                    {
                        if (record->age == 0)
                        {
                            mdlEffectStartChild(*objects);
                        }
                        if (record->age >= 0)
                        {
                            mdlEffectScaleChild(*objects, record->scale * scale);
                            mdlEffectUpdateChildData(*objects, record);
                            mdlEffectCommitChild(*objects);
                        }
                        index++;
                        record++;
                        objects++;
                    }
                }
                return;
            }
            mdlEffectLoadQuadB(emitterAddress, (u32*)rotationSnapshot);
            mdlEffectLoadQuadA(emitterAddress, (u32*)translationSnapshot);
            p4_vu0_lqc2(10, rotationSnapshot);
            /* VU0 quaternion input is VF10; the helper produces VF28..VF31. */
            effMiscBuildRotationVU();
            p4_vu0_lqc2(31, translationSnapshot);
            p4_vu0_sqc2(transform[0], 28);
            p4_vu0_sqc2(transform[1], 29);
            p4_vu0_sqc2(transform[2], 30);
            p4_vu0_sqc2(transform[3], 31);
            {
                /* This traversal starts only after its emitter/setup phase. */
                s32 index = 0;
                while (index < particleCount)
                {
                    if (record->age >= 0)
                    {
                        p4_vu0_lqc2(28, transform[0]);
                        p4_vu0_lqc2(29, transform[1]);
                        p4_vu0_lqc2(30, transform[2]);
                        p4_vu0_lqc2(31, transform[3]);
                        p4_vu0_lqc2(10, record);
                        p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                        p4_vu0_madd(10, P4_VU_XYZW, 31, 0, P4_VU_BC_W);
                        p4_vu0_sqc2(transformedPosition, 10);
                        mdlEffectScaleChild(*objects, record->scale * scale);
                        mdlEffectUpdateChildData(*objects, transformedPosition);
                        mdlEffectCommitChild(*objects);
                    }
                    index++;
                    record++;
                    objects++;
                }
            }
            return;
        }
    case 6:
        record = *(ParticleUpdateRecord**)((u8*)emitterAddress + 0x18);
        /* The object-array traversal starts after the record-base lookup. */
        {
            u8** objects = *(u8***)(particle + 0x3C);
            particleCount = *(s32*)((u8*)emitterAddress + 8);
            if ((*(u32*)((u8*)emitterAddress + 0xC) & 1) == 0)
            {
                {
                    /* This traversal starts only after its emitter/setup phase. */
                    s32 index = 0;
                    while (index < particleCount)
                    {
                        if (record->age == 0)
                        {
                            mdlEffectStartChildren(*objects);
                        }
                        if (record->age >= 0)
                        {
                            mdlEffectSetChildScales(*objects, record->scale * scale);
                            mdlEffectSetChildPositions(*objects, (f32*)record);
                            mdlEffectCommitTreeFrame(*objects);
                        }
                        index++;
                        record++;
                        objects++;
                    }
                }
                return;
            }
            mdlEffectLoadQuadB(emitterAddress, (u32*)rotationSnapshot);
            mdlEffectLoadQuadA(emitterAddress, (u32*)translationSnapshot);
            p4_vu0_lqc2(10, rotationSnapshot);
            /* VU0 quaternion input is VF10; the helper produces VF28..VF31. */
            effMiscBuildRotationVU();
            p4_vu0_lqc2(31, translationSnapshot);
            p4_vu0_sqc2(transform[0], 28);
            p4_vu0_sqc2(transform[1], 29);
            p4_vu0_sqc2(transform[2], 30);
            p4_vu0_sqc2(transform[3], 31);
            {
                /* This traversal starts only after its emitter/setup phase. */
                s32 index = 0;
                while (index < particleCount)
                {
                    if (record->age == 0)
                    {
                        mdlEffectStartChildren(*objects);
                    }
                    if (record->age >= 0)
                    {
                        p4_vu0_lqc2(28, transform[0]);
                        p4_vu0_lqc2(29, transform[1]);
                        p4_vu0_lqc2(30, transform[2]);
                        p4_vu0_lqc2(31, transform[3]);
                        p4_vu0_lqc2(10, record);
                        p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                        p4_vu0_madd(10, P4_VU_XYZW, 31, 0, P4_VU_BC_W);
                        p4_vu0_sqc2(transformedPosition, 10);
                        mdlEffectSetChildScales(*objects, record->scale * scale);
                        mdlEffectSetChildPositions(*objects, (f32*)transformedPosition);
                        mdlEffectCommitTreeFrame(*objects);
                    }
                    index++;
                    record++;
                    objects++;
                }
            }
            return;
        }
    }
}
