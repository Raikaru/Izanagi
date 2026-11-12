#include "p4pc/vu0.h"

void FlashTexturedEmitDirectionalQuads(u8 *effect)
{
    u32 innerWord, outerWord, outerPacked, innerPacked;
    EffectVuVector innerColor, outerColor;
    EffectVuVector direction, widthVector, lengthVector;
    s32 firstFrame, count, index, fadeIn, fadeOut, initialAge, terminalAge, ageStep, recycle;
    u8 *state, *owner, *config;
    s32 *particle;
    f32 *pf;
    s32 life;
    FlashVertex *vertices;
    u32 *colors;
    f32 acceleration, phase, factor, fullTurn, unit, zero, half, byteScale, normalization;
    const u32 *colorInput;
    u32 frame, lastFrame;
    s32 births;

    config = *(u8 **)(effect + 0x40);
    lastFrame = *(u32 *)(config + 0x34);
    frame = *(u32 *)(effect + 0x34);
    if (lastFrame < frame && lastFrame != 0) return;
    state = *(u8 **)(effect + 0x3c);
    particle = *(s32 **)state;
    owner = *(u8 **)(state + 4);
    count = *(s32 *)(config + 0x38);
    life = *(s32 *)(config + 0x4c);
    if (life == 0) return;
    if (*(u8 *)(config + 0x84)) {
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
    acceleration = *(f32 *)(config + 0x80);
    recycle = *(u8 *)(config + 0x54);
    index = 0;
    fullTurn = fGpffff80d0;
    unit = 1.0f;
    half = 0.5f;
    byteScale = 255.0f;
    for (; index < count; index++, particle += 6, vertices += 4, colors += 4) {
        s32 age = *particle;
        pf = (f32 *)particle;
        if (age == -2) continue;
        if (age == -1) {
            if (births == 0) continue;
            pf[2] = fullTurn * effMiscRandFloat(0);
            phase = *(f32 *)(config + 0x6c);
            factor = 0.0f + (unit - phase) + phase * effMiscRandFloat(0);
            pf[3] = *(f32 *)(config + 0x68) * factor;
            phase = *(f32 *)(config + 0x74);
            factor = 0.0f + (unit - phase) + phase * effMiscRandFloat(0);
            pf[4] = *(f32 *)(config + 0x70) * factor;
            phase = *(f32 *)(config + 0x7c);
            factor = 0.0f + (unit - phase) + phase * effMiscRandFloat(0);
            pf[1] = *(f32 *)(config + 0x78) * factor;
            phase = *(f32 *)(config + 0x64);
            factor = 0.0f + (unit - phase) + phase * effMiscRandFloat(0);
            pf[5] = *(f32 *)(config + 0x60) * factor;

            if (firstFrame) {
                *particle = effMiscRand(0) % (u32)life;
            } else {
                *particle = initialAge;
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
            *particle = recycle ? -1 : -2;
        } else {
            f32 time = (f32)age;
            f32 distance;
            f32 fade;
            f32 sine, cosine;
            phase = pf[2];
            if (acceleration < zero) {
                f32 maximum = half * (-pf[1] / (half * acceleration));
                if (time > maximum) time = maximum;
            }
            distance = time * (0.0f + pf[1] + half * (acceleration * time));
            distance += pf[4];
            direction.lane[0] = cosf(phase);
            direction.lane[2] = sinf(phase);
            effectVuLoad10(&direction);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            effectVuScale10(pf[3]);
            effectVuStore10(&lengthVector);
            p4_vu0_move(10, P4_VU_XYZW, 11);
            effectVuScale10(distance);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            p4_vu0_move(10, P4_VU_XYZW, 11);
            cosine = effectVuGetX10();
            sine = effectVuGetZ10();
            effectVuSetX10(sine);
            effectVuSetZ10(-cosine);
            effectVuScale10(pf[5]);
            effectVuStore10(&widthVector);
            effectVuLoad10(&lengthVector);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            effectVuLoad11(&widthVector);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10((EffectVuVector *)D_00713D10);
            vertices[0].x = D_00713D10[0];
            vertices[0].y = D_00713D14[0];
            vertices[0].z = D_00713D18[0];
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10((EffectVuVector *)D_00713D10);
            vertices[1].x = D_00713D10[0];
            vertices[1].y = D_00713D14[0];
            vertices[1].z = D_00713D18[0];
            effectVuLoad10(&lengthVector);
            p4_vu0_sub(10, P4_VU_XYZ, 0, 10, P4_VU_NOBC);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            effectVuLoad11(&widthVector);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10((EffectVuVector *)D_00713D10);
            vertices[2].x = D_00713D10[0];
            vertices[2].y = D_00713D14[0];
            vertices[2].z = D_00713D18[0];
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10((EffectVuVector *)D_00713D10);
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
            *particle = age + ageStep;
        }
    }
    {
        u8 *geometry = *(u8 **)(*(u8 **)(owner + 0x10) + 0x18);
        func_003c22f0(geometry);
        if (*(u16 *)owner & 4) *(u16 *)(geometry + 0xc) |= 1;
    }
}
