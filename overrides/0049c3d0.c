#include "p4pc/vu0.h"

void FlashEvolvingRadialEmitQuads(u8 *effect)
{
    u32 innerWord;
    u32 outerWord;
    u32 outerPacked;
    u32 innerPacked;
    EffectVuVector innerColor;
    EffectVuVector outerColor;
    EffectVuVector direction;
    EffectVuVector width;
    EffectVuVector second;
    EffectVuVector first;
    s32 firstFrame;
    s32 count;
    s32 index;
    s32 fadeIn;
    s32 fadeOut;
    s32 recycle;
    FlashEvolvingRadialState *state;
    FlashEvolvingRadialParticle *particle;
    FlashEvolvingRadialConfig *config;
    u8 *owner;
    s32 life;
    FlashVertex *vertices;
    u32 *colors;
    f32 acceleration;
    f32 gravity;
    f32 height;
    f32 distance;
    f32 radiusRandomness;
    f32 phase;
    f32 fullTurn;
    f32 unit;
    f32 zero;
    f32 half;
    f32 byteScale;
    f32 normalization;
    const u32 *colorInput;
    u32 frame;
    u32 lastFrame;
    s32 births;

    config = *(FlashEvolvingRadialConfig **)(effect + 0x40);
    lastFrame = config->lastFrame;
    frame = *(u32 *)(effect + 0x34);
    if (lastFrame < frame && lastFrame != 0) {
        return;
    }
    state = *(FlashEvolvingRadialState **)(effect + 0x3c);
    particle = state->particles;
    owner = state->geometryOwner;
    count = config->count;
    life = config->life;
    if (life == 0) {
        return;
    }
    fadeIn = (s32)(config->fadeIn * (f32)life);
    fadeOut = (s32)(config->fadeOut * (f32)life);
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
    zero = 0.0f;
    direction.lane[1] = zero;
    direction.lane[3] = zero;
    innerWord = config->innerColor & 0x00ffffff;
    colorInput = &innerWord;
    normalization = fGpffff8044;
    effectVuUnpackColor10(colorInput, normalization);
    effectVuStore10(&innerColor);
    outerWord = config->outerColor & 0x00ffffff;
    effectVuUnpackColor10(&outerWord, normalization);
    effectVuStore10(&outerColor);
    acceleration = config->angularAcceleration;
    gravity = config->gravity;
    recycle = config->recycle;
    p4_vu0_sqc2(&first, 0);
    p4_vu0_sqc2(&second, 0);
    index = 0;
    fullTurn = fGpffff80d0;
    unit = 1.0f;
    half = 0.5f;
    byteScale = 255.0f;
    for (; index < count; index++, particle++, vertices += 13, colors += 13) {
        s32 age = particle->age;
        if (age == -2) {
            continue;
        }
        if (age == -1) {
            if (births != 0) {
                f32 factor;
                f32 endpoint;
                particle->angle = fullTurn * effMiscRandFloat(0);
                phase = config->radiusRandomness;
                radiusRandomness = unit - phase;
                factor = 0.0f + radiusRandomness + phase * effMiscRandFloat(0);
                particle->firstRadius = config->radius * factor;
                factor = 0.0f + radiusRandomness + phase * effMiscRandFloat(0);
                particle->secondRadius = config->radius * factor;
                radiusRandomness = config->startRadiusRandomness;
                factor = 0.0f + (unit - radiusRandomness) + radiusRandomness * effMiscRandFloat(0);
                radiusRandomness = config->startRadius * factor;
                phase = config->endRadiusRandomness;
                factor = 0.0f + (unit - phase) + phase * effMiscRandFloat(0);
                endpoint = config->endRadius * factor;
                particle->baseRadius = radiusRandomness;
                particle->radiusStep = (endpoint - radiusRandomness) / (f32)life;
                radiusRandomness = config->angularRandomness;
                factor = 0.0f + (unit - radiusRandomness) + radiusRandomness * effMiscRandFloat(0);
                particle->angularVelocity = config->angularVelocity * factor;
                radiusRandomness = config->verticalRandomness;
                factor = 0.0f + (unit - radiusRandomness) + radiusRandomness * effMiscRandFloat(0);
                particle->verticalVelocity = config->verticalVelocity * factor;
                radiusRandomness = config->sizeRandomness;
                factor = 0.0f + (unit - radiusRandomness) + radiusRandomness * effMiscRandFloat(0);
                particle->width = config->width * factor;
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
            for (vertex = 0; vertex < 13; vertex++) {
                vertices[vertex].x = zero;
                vertices[vertex].y = zero;
                vertices[vertex].z = zero;
                colors[vertex] = 0;
            }
            particle->age = recycle ? -1 : -2;
        } else {
            f32 time = (f32)age;
            f32 x;
            f32 z;
            f32 fade;
            u32 innerValue;
            u32 outerValue;
            s32 vertex;
            EffectVuVector *widthPointer;
            EffectVuVector *scratch;
            EffectVuVector *firstPointer;
            s32 secondVertex;
            f32 offset = half * (acceleration * time);
            phase = time * (particle->verticalVelocity + offset);
            phase += particle->angle;
            height = time * (particle->angularVelocity + offset);
            { f32 falling; f32 fallen; falling = gravity * time; fallen = time * falling; height = height - half * fallen; }
            distance = 0.0f + particle->baseRadius + particle->radiusStep * time;
            first.lane[1] = particle->firstRadius;
            second.lane[1] = particle->secondRadius;
            effectVuLoad10((EffectVuVector *)D_00713CE0);
            effectVuScale10(particle->width);
            effectVuStore10(&width);
            direction.lane[0] = cosf(phase);
            direction.lane[2] = sinf(phase);
            effectVuLoad10(&direction);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuScale10(distance);
            p4_vu0_set_xf(2, height);
            p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
            p4_vu0_move(2, P4_VU_XYZW, 10);
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_move(12, P4_VU_XYZW, 2);
            x = effectVuGetX10();
            z = effectVuGetZ10();
            effectVuSetX10(z);
            effectVuSetZ10(-x);
            widthPointer = &width;
            effectVuLoad11(widthPointer);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10(widthPointer);
            scratch = (EffectVuVector *)D_00713D10;
            p4_vu0_sqc2(scratch, 12);
            vertices[0].x = D_00713D10[0];
            vertices[0].y = D_00713D14[0];
            vertices[0].z = D_00713D18[0];
            /* The tables contain raw IEEE-754 words. The transfer latency
             * nop belongs to the COP2 bridge, not to scalar computation. */
            vertex = 0;
            firstPointer = &first;
            for (; vertex < 6; vertex++) {
                effectVuLoad10(firstPointer);
                p4_vu0_set_x(2, (uint32_t)D_00713FB0[vertex]);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
                effectVuLoad11(widthPointer);
                p4_vu0_set_x(2, (uint32_t)D_00713F70[vertex]);
                p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                effectVuStore10(scratch);
                vertices[vertex + 1].x = D_00713D10[0];
                vertices[vertex + 1].y = D_00713D14[0];
                vertices[vertex + 1].z = D_00713D18[0];
            }
            for (secondVertex = 6; secondVertex < 12; secondVertex++) {
                effectVuLoad10(&second);
                p4_vu0_set_x(2, (uint32_t)D_00713FB0[secondVertex]);
                p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
                p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
                effectVuLoad11(&width);
                p4_vu0_set_x(2, (uint32_t)D_00713F70[secondVertex]);
                p4_vu0_mul(11, P4_VU_XYZW, 11, 2, P4_VU_BC_X);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
                effectVuStore10((EffectVuVector *)D_00713D10);
                vertices[secondVertex + 1].x = D_00713D10[0];
                vertices[secondVertex + 1].y = D_00713D14[0];
                vertices[secondVertex + 1].z = D_00713D18[0];
            }
            if (age < fadeIn) {
                fade = (f32)age / (f32)fadeIn;
            } else if (age > fadeOut) {
                fade = (f32)(life - age) / (f32)(life - fadeOut);
            } else {
                fade = unit;
            }
            effectVuLoad11((EffectVuVector *)D_00713CE0);
            effectVuScale11(fade);
            effectVuLoad10(&outerColor);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            outerPacked = p4_vu0_pack_vf10_255();
            {
                s32 colorIndex;
                outerValue = outerPacked;
                for (colorIndex = 1; colorIndex <= 12; colorIndex++) {
                    colors[colorIndex] = outerValue;
                }
            }
            effectVuLoad10(&innerColor);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            innerPacked = p4_vu0_pack_vf10_255();
            innerValue = innerPacked;
            colors[0] = innerValue | ((u32)(byteScale * fade) << 24);
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
