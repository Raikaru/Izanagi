#include "p4pc/vu0.h"

void FlashRotatingEmitQuads(u8 *effect)
{
    u32 innerWord;
    u32 outerWord;
    u32 innerPacked;
    u32 outerPacked;
    EffectVuVector innerColor;
    EffectVuVector outerColor;
    EffectVuVector direction;
    EffectVuVector radial;
    EffectVuVector side;
    EffectVuVector end;
    s32 firstFrame;
    s32 count;
    s32 fadeIn;
    s32 fadeOut;
    s32 recycle;
    FlashRotatingState *state;
    FlashRotatingParticle *particle;
    FlashRotatingConfig *config;
    u8 *owner;
    s32 life;
    FlashVertex *vertices;
    u32 *colors;
    f32 randomness;
    f32 angle;
    f32 acceleration;
    f32 fullTurn;
    f32 normalization;
    const u32 *colorInput;
    u32 frame;
    u32 lastFrame;
    s32 index;
    s32 births;

    config = *(FlashRotatingConfig **)(effect + 0x40);
    lastFrame = config->lastFrame;
    frame = *(u32 *)(effect + 0x34);
    if (lastFrame < frame && lastFrame != 0) {
        return;
    }
    state = *(FlashRotatingState **)(effect + 0x3c);
    particle = state->particles;
    owner = state->geometryOwner;
    count = config->count;
    life = config->life;
    if (life == 0) {
        return;
    }
    if (config->burst != 0 && frame == 0) {
        firstFrame = 1;
        births = count;
    } else {
        firstFrame = 0;
        births = config->births;
    }
    RpGeometryLock(*(RpGeometry **)(*(u8 **)(owner + 0x10) + 0x18), 0xa);
    vertices = *(FlashVertex **)(*(u8 **)(*(u8 **)(*(u8 **)(owner + 0x10) + 0x18) + 0x5c) + 0x14);
    colors = *(u32 **)(*(u8 **)(*(u8 **)(owner + 0x10) + 0x18) + 0x30);
    direction.lane[1] = 0.0f;
    direction.lane[3] = 0.0f;
    innerWord = config->innerColor & 0x00ffffff;
    colorInput = &innerWord;
    normalization = fGpffff8044;
    effectVuUnpackColor10(colorInput, normalization);
    effectVuStore10(&innerColor);
    outerWord = config->outerColor & 0x00ffffff;
    effectVuUnpackColor10(&outerWord, normalization);
    effectVuStore10(&outerColor);
    fadeIn = (s32)(config->fadeIn * (f32)life);
    fadeOut = (s32)(config->fadeOut * (f32)life);
    acceleration = config->angularAcceleration;
    recycle = config->recycle;
    for (index = 0, fullTurn = fGpffff80d0; index < count; index++, particle++, vertices += 5, colors += 5) {
        s32 age = particle->age;
        if (age == -2) {
            continue;
        }
        if (age == -1) {
            if (births != 0) {
                f32 factor;
                particle->angle = fullTurn * effMiscRandFloat(0);
                randomness = config->angularRandomness;
                factor = 0.0f + (1.0f - randomness) + randomness * effMiscRandFloat(0);
                particle->angularVelocity = config->angularVelocity * factor;
                if (effMiscRand(0) & 1) {
                    particle->angularVelocity *= -1.0f;
                }
                randomness = config->radiusRandomness;
                factor = 0.0f + (1.0f - randomness) + randomness * effMiscRandFloat(0);
                particle->radius = config->radius * factor;
                randomness = config->sizeRandomness;
                factor = 0.0f + (1.0f - randomness) + randomness * effMiscRandFloat(0);
                particle->width = config->width * factor;
                particle->length = config->length * factor;
                if (firstFrame) {
                    particle->age = effMiscRand(0) % (u32)life;
                } else {
                    particle->age = 0;
                }
                births--;
            }
            continue;
        }
        if (age >= life) {
            s32 vertex;
            for (vertex = 0; vertex < 5; vertex++) {
                vertices[vertex].x = 0.0f;
                vertices[vertex].y = 0.0f;
                vertices[vertex].z = 0.0f;
                colors[vertex] = 0;
            }
            particle->age = recycle ? -1 : -2;
        } else {
            f32 time = (f32)age;
            f32 x;
            f32 z;
            f32 fade;
            u32 innerValue;
            EffectVuVector *sidePointer;
            EffectVuVector *endPointer;
            EffectVuVector *scratch;
            if (particle->angularVelocity > 0.0f) {
                angle = time * (0.0f + particle->angularVelocity + 0.5f * (acceleration * time));
            } else {
                angle = time * (0.0f + particle->angularVelocity - 0.5f * (acceleration * time));
            }
            angle += particle->angle;
            effectVuLoad10((EffectVuVector *)D_00713CE0);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            effectVuScale10(particle->radius);
            effectVuStore10(&radial);
            p4_vu0_move(10, P4_VU_XYZW, 11);
            effectVuScale10(particle->width);
            effectVuScale11(particle->length);
            effectVuStore10(&side);
            effectVuStore11(&end);
            direction.lane[0] = cosf(angle);
            direction.lane[2] = sinf(angle);
            effectVuLoad10(&direction);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            x = effectVuGetX10();
            z = effectVuGetZ10();
            effectVuSetX10(z);
            effectVuSetZ10(-x);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            sidePointer = &side;
            effectVuLoad10(sidePointer);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10(sidePointer);
            endPointer = &end;
            effectVuLoad10(endPointer);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10(endPointer);
            p4_vu0_move(10, P4_VU_XYZW, 12);
            effectVuLoad11(&radial);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuLoad10(sidePointer);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            scratch = (EffectVuVector *)D_00713D10;
            effectVuStore10(scratch);
            vertices[2].x = D_00713D10[0];
            vertices[2].y = D_00713D14[0];
            vertices[2].z = D_00713D18[0];
            p4_vu0_sub(11, P4_VU_XYZ, 0, 11, P4_VU_NOBC);
            effectVuStore11(scratch);
            vertices[4].x = D_00713D10[0];
            vertices[4].y = D_00713D14[0];
            vertices[4].z = D_00713D18[0];
            effectVuLoad10(endPointer);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            p4_vu0_sub(11, P4_VU_XYZ, 0, 11, P4_VU_NOBC);
            effectVuStore10(scratch);
            vertices[0].x = D_00713D10[0];
            vertices[0].y = D_00713D14[0];
            vertices[0].z = D_00713D18[0];
            p4_vu0_add(11, P4_VU_XYZW, 11, 12, P4_VU_NOBC);
            effectVuStore11(scratch);
            vertices[1].x = D_00713D10[0];
            vertices[1].y = D_00713D14[0];
            vertices[1].z = D_00713D18[0];
            if (age < fadeIn) {
                fade = (f32)age / (f32)fadeIn;
            } else if (age > fadeOut) {
                fade = (f32)(life - age) / (f32)(life - fadeOut);
            } else {
                fade = 1.0f;
            }
            effectVuLoad11((EffectVuVector *)D_00713CE0);
            effectVuScale11(fade);
            effectVuLoad10(&outerColor);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            outerPacked = p4_vu0_pack_vf10_255();
            colors[0] = outerPacked;
            *(Color4 *)(colors + 1) = *(Color4 *)colors;
            effectVuLoad10(&innerColor);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            innerPacked = p4_vu0_pack_vf10_255();
            innerValue = innerPacked;
            if (index & 1) {
                u32 alpha;
                colors[3] = innerValue | 0xff000000;
                alpha = (u32)(128.0f * fade) << 24;
                colors[2] = alpha;
                colors[4] = alpha;
            } else {
                u32 alpha;
                colors[3] = innerValue | 0x40000000;
                alpha = (u32)(255.0f * fade) << 24;
                colors[2] = alpha;
                colors[4] = alpha;
            }
            particle->age = age + 1;
        }
    }
    {
        u8 *geometry = *(u8 **)(*(u8 **)(owner + 0x10) + 0x18);
        func_003c22f0(geometry);
        if (*(u16 *)owner & 4) {
            *(u16 *)(geometry + 0xc) |= 1;
        }
    }
}
