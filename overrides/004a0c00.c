#include "p4pc/vu0.h"

void FlashAgedBoxEmitQuads(u8 *effect)
{
    u32 innerWord, outerWord, outerPacked, innerPacked;
    EffectVuVector innerColor, outerColor;
    EffectVuVector direction, widthVector, lengthVector;
    s32 firstFrame, count, index, fadeIn, fadeOut, initialAge, terminalAge, ageStep, recycle;
    u8 *state, *owner, *config;
    /* 004a09e0/004a0af0 allocate 0x20 bytes per particle; 004a07b0
     * advances eight words. Retail loads the age as a word and the
     * seven remaining fields as floats. Keep those types in one record. */
    typedef struct FlashInclinedParticle {
        s32 age;
        f32 velocity;
        f32 phase;
        f32 length;
        f32 radius;
        f32 radiusStep;
        f32 inclination;
        f32 width;
    } FlashInclinedParticle;
    typedef char FlashInclinedParticleExtent[sizeof(FlashInclinedParticle) == 0x20 ? 1 : -1];
    FlashInclinedParticle *particle;
    s32 life;
    FlashVertex *vertices;
    u32 *colors;
    f32 acceleration, distance, phase, inclination, fullTurn, unit, zero, half, byteScale, normalization, radius, cosine, factor, endpoint;
    const u32 *colorInput;
    u32 frame, lastFrame;
    s32 births;

    config = *(u8 **)(effect + 0x40);
    lastFrame = *(u32 *)(config + 0x34);
    frame = *(u32 *)(effect + 0x34);
    if (lastFrame < frame && lastFrame != 0) return;
    state = *(u8 **)(effect + 0x3c);
    particle = *(FlashInclinedParticle **)state;
    owner = *(u8 **)(state + 4);
    count = *(s32 *)(config + 0x38);
    life = *(s32 *)(config + 0x4c);
    if (life == 0) return;
    if (*(u8 *)(config + 0x94)) {
        initialAge = life; terminalAge = 0; ageStep = -1;
        fadeIn = (s32)((1.0f - *(f32 *)(config + 0x48)) * (f32)life);
        fadeOut = (s32)((1.0f - *(f32 *)(config + 0x44)) * (f32)life);
    } else {
        initialAge = 0; terminalAge = life; ageStep = 1;
        fadeIn = (s32)(*(f32 *)(config + 0x44) * (f32)life);
        fadeOut = (s32)(*(f32 *)(config + 0x48) * (f32)life);
    }
    if (*(u8 *)(config + 0x55) != 0 && frame == 0) {
        firstFrame = 1;
        births = count;
    } else {
        firstFrame = 0;
        births = *(s32 *)(config + 0x50);
    }
    RpGeometryLock(*(RpGeometry **)(*(u8 **)(owner + 0x10) + 0x18), 10);
    vertices = *(FlashVertex **)(*(u8 **)(*(u8 **)(*(u8 **)(owner + 0x10) + 0x18) + 0x5c) + 0x14);
    colors = *(u32 **)(*(u8 **)(*(u8 **)(owner + 0x10) + 0x18) + 0x30);
    zero = 0.0f;
    direction.lane[1] = zero;
    direction.lane[3] = zero;
    innerWord = *(u32 *)(config + 0x58);
    colorInput = &innerWord;
    normalization = fGpffff8044;
    effectVuUnpackColor10(colorInput, normalization);
    effectVuStore10(&innerColor);
    outerWord = *(u32 *)(config + 0x5c);
    effectVuUnpackColor10(&outerWord, normalization);
    effectVuStore10(&outerColor);
    acceleration = *(f32 *)(config + 0x90);
    recycle = *(u8 *)(config + 0x54);
    index = 0;
    fullTurn = fGpffff80d0;
    unit = 1.0f;
    half = 0.5f;
    byteScale = 255.0f;
    for (; index < count; index++, particle++, vertices += 4, colors += 4) {
        s32 age = particle->age;
        if (age == -2) continue;
        if (age == -1) {
            if (births == 0) continue;
            particle->phase = fullTurn * effMiscRandFloat(0);
            cosine = *(f32 *)(config + 0x6c);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            particle->length = *(f32 *)(config + 0x68) * factor;
            cosine = *(f32 *)(config + 0x74);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            cosine = *(f32 *)(config + 0x70) * factor;
            radius = *(f32 *)(config + 0x7c);
            factor = 0.0f + (unit - radius) + radius * effMiscRandFloat(0);
            endpoint = *(f32 *)(config + 0x78) * factor;
            particle->radius = cosine;
            particle->radiusStep = (endpoint - cosine) / (f32)life;
            cosine = *(f32 *)(config + 0x8c);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            particle->velocity = *(f32 *)(config + 0x88) * factor;
            cosine = *(f32 *)(config + 0x64);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            particle->width = *(f32 *)(config + 0x60) * factor;
            cosine = *(f32 *)(config + 0x84);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            particle->inclination = *(f32 *)(config + 0x80) * factor;
            if (firstFrame) {
                particle->age = effMiscRand(0) % (u32)life;

            } else {
                particle->age = initialAge;
            }
            births--;
            continue;
        }
        if (age == terminalAge) {
            s32 vertex;
            for (vertex = 0; vertex < 4; vertex++) {
                vertices[vertex].x = zero;
                vertices[vertex].y = zero;
                vertices[vertex].z = zero;
                colors[vertex] = 0;
            }
            particle->age = recycle ? -1 : -2;
        } else {
            EffectVuVector *scratch;
            f32 time = (f32)age;
            f32 fade;
            f32 sine;
            phase = particle->phase;
            if (acceleration < zero) {
                f32 maximum = half * (-particle->velocity / (half * acceleration));
                if (time > maximum) time = maximum;
            }
            distance = time * (0.0f + particle->velocity + half * (acceleration * time));
            radius = 0.0f + particle->radius + particle->radiusStep * time;
            inclination = particle->inclination;
            cosine = cosf(phase);
            sine = sinf(phase);
            direction.lane[0] = cosine * inclination;
            direction.lane[1] = unit - inclination;
            direction.lane[2] = sine * inclination;
            scratch = &direction;
            effectVuLoad10(scratch);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_set_xf(2, particle->length);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 2, P4_VU_BC_X);
            effectVuStore10(&lengthVector);
            p4_vu0_move(10, P4_VU_XYZW, 11);
            effectVuScale10(distance);
            effectVuStore10(scratch);
            direction.lane[0] = 0.0f + direction.lane[0] + cosine * radius;
            direction.lane[2] = 0.0f + direction.lane[2] + sine * radius;
            effectVuSetX10(sine);
            p4_vu0_set_xf(2, zero);
            p4_vu0_add(10, P4_VU_Y, 0, 2, P4_VU_BC_X);
            effectVuSetZ10(-cosine);
            effectVuScale10(particle->width);
            effectVuStore10(&widthVector);
            /* lqc2 $vf12 reads the scratch copy, including the lane 0/2 edits above. */
            p4_vu0_lqc2(12, scratch);
            effectVuLoad10(&lengthVector);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            effectVuLoad11(&widthVector);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            scratch = (EffectVuVector *)D_00713D10;
            effectVuStore10(scratch);
            vertices[0].x = D_00713D10[0];
            vertices[0].y = D_00713D14[0];
            vertices[0].z = D_00713D18[0];
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10(scratch);
            vertices[1].x = D_00713D10[0];
            vertices[1].y = D_00713D14[0];
            vertices[1].z = D_00713D18[0];
            effectVuLoad10(&lengthVector);
            p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            effectVuLoad11(&widthVector);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10(scratch);
            vertices[2].x = D_00713D10[0];
            vertices[2].y = D_00713D14[0];
            vertices[2].z = D_00713D18[0];
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10(scratch);
            vertices[3].x = D_00713D10[0];
            vertices[3].y = D_00713D14[0];
            vertices[3].z = D_00713D18[0];

            if (age < fadeIn) fade = (f32)age / (f32)fadeIn;
            else if (age > fadeOut) fade = (f32)(life - age) / (f32)(life - fadeOut);
            else fade = unit;
            effectVuLoad11((EffectVuVector *)D_00713CE0);
            effectVuScale11(fade);
            effectVuLoad10(&outerColor);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            outerPacked = p4_vu0_pack_vf10_255();

            colors[0] = outerPacked;
            colors[1] = outerPacked;
            effectVuLoad10(&innerColor);
            p4_vu0_mul(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            innerPacked = p4_vu0_pack_vf10_255();

            colors[2] = innerPacked;
            colors[3] = innerPacked;
            particle->age = age + ageStep;
        }
    }
    {
        u8 *geometry = *(u8 **)(*(u8 **)(owner + 0x10) + 0x18);
        func_003c22f0(geometry);
        if (*(u16 *)owner & 4) *(u16 *)(geometry + 0xc) |= 1;
    }
}
