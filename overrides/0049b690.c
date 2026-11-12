#include "p4pc/vu0.h"

void FlashRadialEmitQuads(u8 *effect)
{
    u32 innerWord;
    u32 outerWord;
    u32 outerPacked;
    u32 innerPacked;
    EffectVuVector innerColor;
    EffectVuVector outerColor;
    EffectVuVector center;
    EffectVuVector direction;
    EffectVuVector width;
    EffectVuVector second;
    EffectVuVector first;
    s32 firstFrame;
    s32 count;
    s32 index;
    s32 fadeIn;
    s32 fadeOut;
    s32 initialAge;
    s32 terminalAge;
    s32 ageStep;
    s32 recycle;
    FlashRadialState *state;
    FlashRadialParticle *particle;
    FlashRadialConfig *config;
    u8 *owner;
    s32 life;
    FlashVertex *vertices;
    u32 *colors;
    f32 acceleration;
    f32 distance;
    f32 radiusRandomness;
    f32 fullTurn;
    f32 unit;
    f32 zero;
    f32 half;
    f32 byteScale;
    f32 phase;
    f32 normalization;
    const u32 *colorInput;
    u32 frame;
    u32 lastFrame;
    s32 births;

    config = *(FlashRadialConfig **)(effect + 0x40);
    lastFrame = config->lastFrame;
    frame = *(u32 *)(effect + 0x34);
    if (lastFrame < frame && lastFrame != 0) {
        return;
    }
    state = *(FlashRadialState **)(effect + 0x3c);
    particle = state->particles;
    owner = state->geometryOwner;
    count = config->count;
    life = config->life;
    if (life == 0) {
        return;
    }
    if (config->reverse) {
        initialAge = life;
        terminalAge = 0;
        ageStep = -1;
        fadeIn = (s32)((1.0f - config->fadeOut) * (f32)life);
        fadeOut = (s32)((1.0f - config->fadeIn) * (f32)life);
    } else {
        initialAge = 0;
        terminalAge = life;
        ageStep = 1;
        fadeIn = (s32)(config->fadeIn * (f32)life);
        fadeOut = (s32)(config->fadeOut * (f32)life);
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
    acceleration = config->acceleration;
    recycle = config->recycle;
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
                particle->angle = fullTurn * effMiscRandFloat(0);
                radiusRandomness = config->radiusRandomness;
                phase = unit - radiusRandomness;
                factor = 0.0f + phase + radiusRandomness * effMiscRandFloat(0);
                particle->firstRadius = config->radius * factor;
                factor = 0.0f + phase + radiusRandomness * effMiscRandFloat(0);
                particle->secondRadius = config->radius * factor;
                phase = config->baseRandomness;
                factor = 0.0f + (unit - phase) + phase * effMiscRandFloat(0);
                particle->baseRadius = config->baseRadius * factor;
                phase = config->velocityRandomness;
                factor = 0.0f + (unit - phase) + phase * effMiscRandFloat(0);
                particle->velocity = config->velocity * factor;
                phase = config->sizeRandomness;
                factor = 0.0f + (unit - phase) + phase * effMiscRandFloat(0);
                particle->width = config->width * factor;
                if (firstFrame) {
                    particle->age = effMiscRand(0) % (u32)life;
                } else {
                    particle->age = initialAge;
                }
                births--;
            }
            continue;
        }
        if (age == terminalAge) {
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
            EffectVuVector *firstPointer;
            EffectVuVector *widthPointer;
            EffectVuVector *scratch;
            s32 secondVertex;
            phase = particle->angle;
            if (acceleration < zero) {
                f32 maximumTime = half * (-particle->velocity / (half * acceleration));
                if (!(time <= maximumTime)) {
                    time = maximumTime;
                }
            }
            distance = time * (0.0f + particle->velocity + half * (acceleration * time));
            distance += particle->baseRadius;
            effectVuLoad10((EffectVuVector *)D_00713CE0);
            effectVuScale10(particle->width);
            effectVuStore10(&width);
            direction.lane[0] = cosf(phase);
            direction.lane[2] = sinf(phase);
            effectVuLoad10(&direction);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            effectVuScale10(particle->firstRadius);
            p4_vu0_move(11, P4_VU_XYZW, 12);
            firstPointer = &first;
            effectVuStore10(firstPointer);
            effectVuScale11(particle->secondRadius);
            effectVuStore11(&second);
            p4_vu0_move(10, P4_VU_XYZW, 12);
            effectVuScale10(distance);
            effectVuStore10(&center);
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
            for (vertex = 0; vertex < 6; vertex++) {
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
            particle->age = age + ageStep;
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
