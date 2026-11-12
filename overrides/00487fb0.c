#include "p4pc/vu0.h"

// Projects the particle vertices through the VU0 matrix and writes the packed colour per vertex.
// FUN_00487FB0
void ParticleProjectVertices(u8* particle, f32 scale)
{
    extern s32 func_003a5180(RpAtomic * atomic);
    extern s32 func_003a2770(RpAtomic * atomic, RpPTankLockStruct * lock, u32 dataFlags, RpPTankLockFlags access);
    extern RpAtomic *func_003a2920(RpAtomic *atomic);
    extern void mdlEffectWrapTrackFrames(int a, u32 b);
    extern void mdlEffectEvalSubPose(int a, float* b);
    extern s32 mdlEffectGetStreamHandle(u8 * *a, u32 b);
    extern void GeometryBlockSetPartState(u8 * a, s32 b);
    extern void GeometryBlockBindResource(u8 * a, s32 b);
    extern void mdlEffectSetChildColor(u8 * a, s32 b);
    extern void mdlEffectFlushChild(u8 * a);
    extern void mdlEffectApplyPackedColor(u8 * a, s32 b);
    extern void mdlEffectFlushTreeFrame(s32 a);
    extern s32 TnGetOffset1C(u16 a);
    extern void PrimAppendNode(u8 * list, u8 * node);
    extern f32 fGpffff8044;
    extern f32 fGpffff8080;
    extern f32 fGpffff8084;
    extern f32 fGpffff80a0;
    extern f32 fGpffff80a4;
    extern f32 fGpffff80a8;
    extern f32 fGpffff80ac;
    extern f32 fGpffff80b0;
    extern f32 fGpffff80b4;
    extern f32 fGpffff80b8;
    extern s32 iGpffffb610;
    extern u8 D_00713CD0[];
    ParticleRenderColor materialColor;
    u8 transformedPoint[16] __attribute__((aligned(16)));
    u8 translationSnapshot[16] __attribute__((aligned(16)));
    u8 rotationSnapshot[16] __attribute__((aligned(16)));
    u8 transformStorage[64] __attribute__((aligned(16)));
    f32 parentColor[4] __attribute__((aligned(16)));
    ParticleAnimationOutput animation;
    RpPTankLockStruct rotationLock;
    RpPTankLockStruct positionLock;
    RpPTankLockStruct sizeLock;
    RpPTankLockStruct colorLock;
    RpPTankLockStruct uvLock;
    u8* emitterOrObjects;
    u8* record;
    u8* atomicAddress;
    u8* settings;
    u8* animationSettings;
    s32 particleCount;
    s32 activeCount;
    f32 colorScale;
    uint32_t colorScaleBits;
    f32 scaleX;
    f32 scaleY;
    s32 plainIndex;
    s32 transformedIndex;
    s32 animatedIndex;
    s32 animatedTransformedIndex;
    s32 effectIndex;
    s32 modelIndex;
    emitterOrObjects = *(u8**)(particle + 0x4C);
    {
        /* Parent colour: unpack the four bytes, scale by fGpffff8044 on VF2.x, store VF10. */
        s32 inputColor = *(s32*)(particle + 4);
        P4Mmi128 lanes;
        colorScale = fGpffff8044;
        __builtin_memcpy(&colorScaleBits, &colorScale, sizeof colorScaleBits);
        lanes = p4_vu0_word_bytes_to_lanes((uint32_t)inputColor);
        p4_vu0_qmtc2(10, &lanes);
        p4_vu0_itof(10, P4_VU_XYZW, 10);
        p4_vu0_set_x(2, colorScaleBits);
        p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
        p4_vu0_sqc2(parentColor, 10);
    }
    particleCount = *(s32*)(emitterOrObjects + 8);
    if (particleCount == 0)
    {
        goto particle_render_done;
    }
    if (*(s32*)(emitterOrObjects + 0x10) == 0)
    {
        goto particle_render_done;
    }
    record = *(u8**)(emitterOrObjects + 0x18);
    switch (*(u16*)(particle + 0xC))
    {
    case 1:
    case 4:
    {
        f32 upperAngle;
        f32 angleStep;
        f32 lowerAngle;
        activeCount = 0;
        scaleX = ((EffParticleInstance *)particle)->texScaleX * scale;
        scaleY = ((EffParticleInstance *)particle)->texScaleY * scale;
        atomicAddress = *(u8**)(*(u8**)(particle + 0x50) + 0x10);
        settings = *(u8**)(emitterOrObjects + 0x20);
        func_003a5180((RpAtomic*)atomicAddress);
        func_003a2770((RpAtomic*)atomicAddress, &positionLock, 1, 0x40000000);
        func_003a2770((RpAtomic*)atomicAddress, &colorLock, 2, 0x40000000);
        func_003a2770((RpAtomic*)atomicAddress, &sizeLock, 4, 0x40000000);
        func_003a2770((RpAtomic*)atomicAddress, &rotationLock, 0x20, 0x40000000);
        if ((*(u32*)(emitterOrObjects + 0xC) & 1) == 0)
        {
            plainIndex = 0;
            angleStep = fGpffff8080;
            upperAngle = fGpffff8084;
            lowerAngle = fGpffff80a0;
            while (plainIndex < particleCount)
            {
                if (*(s32*)(record + 0x10) >= 0)
                {
                    *(f32*)(positionLock.data + 0) = *(f32*)(record + 0);
                    *(f32*)(positionLock.data + 4) = *(f32*)(record + 4);
                    *(f32*)(positionLock.data + 8) = *(f32*)(record + 8);
                    positionLock.data += positionLock.stride;
                    *(s32*)colorLock.data = *(s32*)(record + 0x14);
                    colorLock.data += colorLock.stride;
                    {
                        f32 f = *(f32*)(record + 0x1C);
                        if (!(f <= upperAngle))
                        {
                            while (!(f <= upperAngle))
                            {
                                f -= angleStep;
                            }
                        }
                        else if (f < lowerAngle)
                        {
                            while (f < lowerAngle)
                            {
                                f += angleStep;
                            }
                        }
                        *(f32*)(record + 0x1C) = f;
                        *(f32*)rotationLock.data = f;
                        rotationLock.data += rotationLock.stride;
                    }
                    {
                        f32 s = *(f32*)(record + 0x18);
                        *(f32*)(sizeLock.data + 0) = scaleX * s;
                        *(f32*)(sizeLock.data + 4) = scaleY * s;
                        sizeLock.data += sizeLock.stride;
                    }
                    activeCount++;
                }
                plainIndex++;
                record += 0x20;
            }
        }
        else
        {
            f32(*transformMatrix)[4];
            f32* transformedPosition;
            mdlEffectLoadQuadB((s32)emitterOrObjects, (u32*)rotationSnapshot);
            mdlEffectLoadQuadA((s32)emitterOrObjects, (u32*)translationSnapshot);
            p4_vu0_lqc2(10, rotationSnapshot);
            effMiscBuildRotationVU();
            p4_vu0_lqc2(31, translationSnapshot);
            transformMatrix = (f32(*)[4])transformStorage;
            p4_vu0_sqc2(transformMatrix[0], 28);
            p4_vu0_sqc2(transformMatrix[1], 29);
            p4_vu0_sqc2(transformMatrix[2], 30);
            p4_vu0_sqc2(transformMatrix[3], 31);
            transformedIndex = 0;
            upperAngle = fGpffff8084;
            angleStep = fGpffff80a4;
            lowerAngle = fGpffff80a8;
            transformedPosition = (f32*)transformedPoint;
            while (transformedIndex < particleCount)
            {
                if (*(s32*)(record + 0x10) >= 0)
                {
                    /* VF10 = (VF28*x + VF29*y + VF30*z) + VF31 for this record's position. */
                    p4_vu0_lqc2(28, transformMatrix[0]);
                    p4_vu0_lqc2(29, transformMatrix[1]);
                    p4_vu0_lqc2(30, transformMatrix[2]);
                    p4_vu0_lqc2(31, transformMatrix[3]);
                    p4_vu0_lqc2(10, record);
                    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                    p4_vu0_madd(10, P4_VU_XYZW, 31, 0, P4_VU_BC_W);
                    p4_vu0_sqc2(transformedPosition, 10);
                    *(f32*)(positionLock.data + 0) = *(f32*)(transformedPoint + 0);
                    *(f32*)(positionLock.data + 4) = *(f32*)(transformedPoint + 4);
                    *(f32*)(positionLock.data + 8) = *(f32*)(transformedPoint + 8);
                    positionLock.data += positionLock.stride;
                    *(s32*)colorLock.data = *(s32*)(record + 0x14);
                    colorLock.data += colorLock.stride;
                    {
                        f32 f = *(f32*)(record + 0x1C);
                        if (!(f <= upperAngle))
                        {
                            while (!(f <= upperAngle))
                            {
                                f -= angleStep;
                            }
                        }
                        else if (f < lowerAngle)
                        {
                            while (f < lowerAngle)
                            {
                                f += angleStep;
                            }
                        }
                        *(f32*)(record + 0x1C) = f;
                        *(f32*)rotationLock.data = f;
                        rotationLock.data += rotationLock.stride;
                    }
                    {
                        f32 s = *(f32*)(record + 0x18);
                        *(f32*)(sizeLock.data + 0) = scaleX * s;
                        *(f32*)(sizeLock.data + 4) = scaleY * s;
                        sizeLock.data += sizeLock.stride;
                    }
                    activeCount++;
                }
                transformedIndex++;
                record += 0x20;
            }
        }
        func_003a2920((RpAtomic *)atomicAddress);
        if (activeCount != 0)
        {
            u8* material;
            materialColor.word = *(u32*)(particle + 4);
            material = **(u8***)(*(u8**)(atomicAddress + 0x18) + 0x20);
            *(ParticleRenderColorBytes*)(material + 4) = materialColor.bytes;
            {
                *(s32*)(*(u8**)(atomicAddress + iGpffffb610) + 0x40) |= 0x800000;
                *(s32*)(*(u8**)(atomicAddress + iGpffffb610) + 4) = activeCount;
            }
            GeometryBlockSetPartState(*(u8**)(particle + 0x50), *(u16*)(settings + 0x54));
            GeometryBlockBindResource(*(u8**)(particle + 0x50), *(u16*)(settings + 0x54));
        }
        break;
    }
    case 2:
    {
        u8* animationResource = *(u8**)(particle + 0x44);
        if (animationResource == 0)
        {
            kAssertFail(D_00713CD0, 0x380);
        }
        /* Count only active particles in either animation path. This
         * initialized lifetime begins after the resource diagnostic and
         * ends after the active-count guard and PTank count store. */
        {
            s32 animatedActiveCount = 0;
            atomicAddress = *(u8**)(*(u8**)(particle + 0x50) + 0x10);
            animationSettings = *(u8**)(emitterOrObjects + 0x20);
            func_003a5180((RpAtomic*)atomicAddress);
            func_003a2770((RpAtomic*)atomicAddress, &positionLock, 1, 0x40000000);
            func_003a2770((RpAtomic*)atomicAddress, &colorLock, 2, 0x40000000);
            func_003a2770((RpAtomic*)atomicAddress, &sizeLock, 4, 0x40000000);
            func_003a2770((RpAtomic*)atomicAddress, &rotationLock, 0x20, 0x40000000);
            func_003a2770((RpAtomic*)atomicAddress, &uvLock, 0x80, 0x40000000);
            {
                s32 t = mdlEffectGetStreamHandle((u8**)animationResource, 0);
                func_003c42b0(**(RpMaterial***)(*(u8**)(atomicAddress + 0x18) + 0x20), (RwTexture*)t);
            }
            if ((*(u32*)(emitterOrObjects + 0xC) & 1) == 0)
            {
                animatedIndex = 0;
                while (animatedIndex < particleCount)
                {
                    if (*(s32*)(record + 0x10) >= 0)
                    {
                        mdlEffectWrapTrackFrames((int)animationResource, *(u32*)(record + 0x10));
                        mdlEffectEvalSubPose((int)animationResource, (float*)&animation);
                        *(f32*)(uvLock.data + 0) = animation.u0;
                        *(f32*)(uvLock.data + 4) = animation.v0;
                        *(f32*)(uvLock.data + 8) = animation.u1;
                        *(f32*)(uvLock.data + 12) = animation.v1;
                        uvLock.data += uvLock.stride;
                        *(f32*)(positionLock.data + 0) = *(f32*)(record + 0);
                        *(f32*)(positionLock.data + 4) = *(f32*)(record + 4);
                        *(f32*)(positionLock.data + 8) = *(f32*)(record + 8);
                        positionLock.data += positionLock.stride;
                        {
                            /* Particle colour (VF11) times animation colour (VF10), then pack. */
                            u32 particleColor = *(u32*)(record + 0x14);
                            u32 animationColor;
                            P4Mmi128 lanes;
                            lanes = p4_vu0_word_bytes_to_lanes(particleColor);
                            p4_vu0_qmtc2(11, &lanes);
                            p4_vu0_itof(11, P4_VU_XYZW, 11);
                            p4_vu0_set_x(2, colorScaleBits);
                            p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                            animationColor = animation.color;
                            lanes = p4_vu0_word_bytes_to_lanes(animationColor);
                            p4_vu0_qmtc2(10, &lanes);
                            p4_vu0_itof(10, P4_VU_XYZW, 10);
                            p4_vu0_set_x(2, colorScaleBits);
                            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                            *(s32*)colorLock.data = (s32)particlePackAnimatedColor();
                            colorLock.data += colorLock.stride;
                        }
                        {
                            f32 f = *(f32*)(record + 0x1C) + animation.angle;
                            f32 upperAngle = fGpffff8084;
                            if (!(f <= upperAngle))
                            {
                                f32 angleStep = fGpffff80ac;
                                while (!(f <= upperAngle))
                                {
                                    f -= angleStep;
                                }
                            }
                            else
                            {
                                f32 lowerAngle = fGpffff80b0;
                                if (f < lowerAngle)
                                {
                                    f32 angleStep = fGpffff80b4;
                                    while (f < lowerAngle)
                                    {
                                        f += angleStep;
                                    }
                                }
                            }
                            *(f32*)rotationLock.data = f;
                            rotationLock.data += rotationLock.stride;
                        }
                        {
                            f32 s = *(f32*)(record + 0x18) * scale;
                            *(f32*)(sizeLock.data + 0) = animation.width * s;
                            *(f32*)(sizeLock.data + 4) = animation.height * s;
                            sizeLock.data += sizeLock.stride;
                        }
                        animatedActiveCount++;
                    }
                    animatedIndex++;
                    record += 0x20;
                }
            }
            else
            {
                f32 colorByteScale;
                f32 upperAngle;
                f32 angleStep;
                f32 lowerAngle;
                f32(*transformMatrix)[4];
                mdlEffectLoadQuadB((s32)emitterOrObjects, (u32*)rotationSnapshot);
                mdlEffectLoadQuadA((s32)emitterOrObjects, (u32*)translationSnapshot);
                p4_vu0_lqc2(10, rotationSnapshot);
                effMiscBuildRotationVU();
                p4_vu0_lqc2(31, translationSnapshot);
                transformMatrix = (f32(*)[4])transformStorage;
                p4_vu0_sqc2(transformMatrix[0], 28);
                p4_vu0_sqc2(transformMatrix[1], 29);
                p4_vu0_sqc2(transformMatrix[2], 30);
                p4_vu0_sqc2(transformMatrix[3], 31);
                animatedTransformedIndex = 0;
                colorByteScale = 255.0f;
                upperAngle = fGpffff8084;
                angleStep = fGpffff80b4;
                lowerAngle = fGpffff80b8;
                while (animatedTransformedIndex < particleCount)
                {
                    if (*(s32*)(record + 0x10) >= 0)
                    {
                        mdlEffectWrapTrackFrames((int)animationResource, *(u32*)(record + 0x10));
                        mdlEffectEvalSubPose((int)animationResource, (float*)&animation);
                        p4_vu0_lqc2(28, transformMatrix[0]);
                        p4_vu0_lqc2(29, transformMatrix[1]);
                        p4_vu0_lqc2(30, transformMatrix[2]);
                        p4_vu0_lqc2(31, transformMatrix[3]);
                        p4_vu0_lqc2(10, record);
                        p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
                        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
                        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
                        p4_vu0_madd(10, P4_VU_XYZW, 31, 0, P4_VU_BC_W);
                        p4_vu0_sqc2(transformedPoint, 10);
                        *(f32*)(uvLock.data + 0) = animation.u0;
                        *(f32*)(uvLock.data + 4) = animation.v0;
                        *(f32*)(uvLock.data + 8) = animation.u1;
                        *(f32*)(uvLock.data + 12) = animation.v1;
                        uvLock.data += uvLock.stride;
                        *(f32*)(positionLock.data + 0) = *(f32*)(transformedPoint + 0);
                        *(f32*)(positionLock.data + 4) = *(f32*)(transformedPoint + 4);
                        *(f32*)(positionLock.data + 8) = *(f32*)(transformedPoint + 8);
                        positionLock.data += positionLock.stride;
                        {
                            /* Particle colour (VF11) times animation colour (VF10), then pack. */
                            u32 particleColor = *(u32*)(record + 0x14);
                            u32 animationColor;
                            P4Mmi128 lanes;
                            lanes = p4_vu0_word_bytes_to_lanes(particleColor);
                            p4_vu0_qmtc2(11, &lanes);
                            p4_vu0_itof(11, P4_VU_XYZW, 11);
                            p4_vu0_set_x(2, colorScaleBits);
                            p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                            animationColor = animation.color;
                            lanes = p4_vu0_word_bytes_to_lanes(animationColor);
                            p4_vu0_qmtc2(10, &lanes);
                            p4_vu0_itof(10, P4_VU_XYZW, 10);
                            p4_vu0_set_x(2, colorScaleBits);
                            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                            *(s32*)colorLock.data = (s32)particlePackAnimatedColorTransformed(colorByteScale);
                            colorLock.data += colorLock.stride;
                        }
                        {
                            f32 f = *(f32*)(record + 0x1C) + animation.angle;
                            if (!(f <= upperAngle))
                            {
                                while (!(f <= upperAngle))
                                {
                                    f -= angleStep;
                                }
                            }
                            else if (f < lowerAngle)
                            {
                                while (f < lowerAngle)
                                {
                                    f += angleStep;
                                }
                            }
                            *(f32*)rotationLock.data = f;
                            rotationLock.data += rotationLock.stride;
                        }
                        {
                            f32 s = *(f32*)(record + 0x18) * scale;
                            *(f32*)(sizeLock.data + 0) = animation.width * s;
                            *(f32*)(sizeLock.data + 4) = animation.height * s;
                            sizeLock.data += sizeLock.stride;
                        }
                        animatedActiveCount++;
                    }
                    animatedTransformedIndex++;
                    record += 0x20;
                }
            }
            func_003a2920((RpAtomic *)atomicAddress);
            if (animatedActiveCount != 0)
            {
                u8* material;
                materialColor.word = *(u32*)(particle + 4);
                material = **(u8***)(*(u8**)(atomicAddress + 0x18) + 0x20);
                *(ParticleRenderColorBytes*)(material + 4) = materialColor.bytes;
                {
                    *(s32*)(*(u8**)(atomicAddress + iGpffffb610) + 0x40) |= 0x800000;
                    *(s32*)(*(u8**)(atomicAddress + iGpffffb610) + 4) = animatedActiveCount;
                }
                GeometryBlockSetPartState(*(u8**)(particle + 0x50), *(u16*)(animationSettings + 0x54));
                GeometryBlockBindResource(*(u8**)(particle + 0x50), *(u16*)(animationSettings + 0x54));
            }
        }
    }
    break;
    case 5:
    {
        emitterOrObjects = *(u8**)(particle + 0x34);
        if (emitterOrObjects == 0)
        {
            kAssertFail(D_00713CD0, 0x3F3);
        }
        effectIndex = 0;
        while (effectIndex < particleCount)
        {
            if (*(s32*)(record + 0x10) >= 0)
            {
                u32 particleColor = *(u32*)(record + 0x14);
                u32 packedColor;
                P4Mmi128 lanes;
                /* Per-child colour: particle colour * scale * parent colour, then pack. */
                lanes = p4_vu0_word_bytes_to_lanes(particleColor);
                p4_vu0_qmtc2(10, &lanes);
                p4_vu0_itof(10, P4_VU_XYZW, 10);
                p4_vu0_set_x(2, colorScaleBits);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                p4_vu0_lqc2(11, parentColor);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                packedColor = particlePackAnimatedColor();
                mdlEffectSetChildColor((u8*)*(s32*)emitterOrObjects, packedColor);
                mdlEffectFlushChild((u8*)*(s32*)emitterOrObjects);
            }
            effectIndex++;
            record += 0x20;
            emitterOrObjects += sizeof(s32);
        }
    }
    break;
    case 6:
    {
        emitterOrObjects = *(u8**)(particle + 0x3C);
        if (emitterOrObjects == 0)
        {
            kAssertFail(D_00713CD0, 0x406);
        }
        modelIndex = 0;
        while (modelIndex < particleCount)
        {
            if (*(s32*)(record + 0x10) >= 0)
            {
                u32 particleColor = *(u32*)(record + 0x14);
                u32 packedColor;
                P4Mmi128 lanes;
                /* Per-child colour: particle colour * scale * parent colour, then pack. */
                lanes = p4_vu0_word_bytes_to_lanes(particleColor);
                p4_vu0_qmtc2(10, &lanes);
                p4_vu0_itof(10, P4_VU_XYZW, 10);
                p4_vu0_set_x(2, colorScaleBits);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                p4_vu0_lqc2(11, parentColor);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                packedColor = particlePackAnimatedColor();
                mdlEffectApplyPackedColor((u8*)*(s32*)emitterOrObjects, packedColor);
                mdlEffectFlushTreeFrame(*(s32*)emitterOrObjects);
            }
            modelIndex++;
            record += 0x20;
            emitterOrObjects += sizeof(s32);
        }
    }
    break;
    case 7:
    {
        u8* t = *(u8**)(emitterOrObjects + 0x20);
        ((EffParticleInstance *)particle)->link->field_0x00 = 0;
        ((EffParticleInstance *)particle)->link->field_0x04 = 0;
        {
            u8* pb = (u8*)TnGetOffset1C(*(u16*)(t + 0x54));
            PrimAppendNode(pb, *(u8**)(particle + 0x5C));
        }
    }
    break;
    default:
        kAssertFail(D_00713CD0, 0x41D);
        break;
    }
particle_render_done:
    return;
}
