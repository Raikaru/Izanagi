#include "p4pc/vu0.h"

void FlashBoxEmitQuads(u8 *effect)
{
    u32 innerWord, outerWord, outerPacked, innerPacked;
    EffectVuVector innerColor, outerColor;
    EffectVuVector center, direction, lengthVector, widthVector;
    s32 firstFrame, count, index, fadeIn, fadeOut, recycle;
    u8 *state, *owner, *config;
    s32 *particle;
    f32 *pf;
    s32 life;
    FlashVertex *vertices;
    u32 *colors;
    f32 acceleration, phase, factor, endpoint, inclination, fullTurn, unit, zero, half, byteScale, normalization, cosine, radialPosition;
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
    center.lane[1] = zero;
    center.lane[3] = zero;
    innerWord = *(u32 *)(config + 0x58) & 0x00ffffff;
    colorInput = &innerWord;
    normalization = fGpffff8044;
    effectVuUnpackColor10(colorInput, normalization);
    effectVuStore10(&innerColor);
    outerWord = *(u32 *)(config + 0x5c) & 0x00ffffff;
    effectVuUnpackColor10(&outerWord, normalization);
    effectVuStore10(&outerColor);

    fadeIn = (s32)(*(f32 *)(config + 0x44) * (f32)life);
    fadeOut = (s32)(*(f32 *)(config + 0x48) * (f32)life);
    acceleration = *(f32 *)(config + 0xa4);
    recycle = *(u8 *)(config + 0x54);
    index = 0;
    fullTurn = fGpffff80d0;
    unit = 1.0f;
    half = 0.5f;
    byteScale = 255.0f;
    for (; index < count; index++, particle += 11, vertices += 5, colors += 5) {
        s32 age = *particle;
        pf = (f32 *)particle;
        if (age == -2) continue;
        if (age == -1) {
            if (births == 0) continue;
            pf[1] = fullTurn * effMiscRandFloat(0);
            cosine = *(f32 *)(config + 0x70);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            cosine = *(f32 *)(config + 0x6c) * factor;
            inclination = *(f32 *)(config + 0x78);
            factor = 0.0f + (unit - inclination) + inclination * effMiscRandFloat(0);
            endpoint = *(f32 *)(config + 0x74) * factor;
            pf[3] = cosine;
            pf[4] = (endpoint - cosine) / (f32)life;
            cosine = *(f32 *)(config + 0x80);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            cosine = *(f32 *)(config + 0x7c) * factor;
            inclination = *(f32 *)(config + 0x88);
            factor = 0.0f + (unit - inclination) + inclination * effMiscRandFloat(0);
            endpoint = *(f32 *)(config + 0x84) * factor;
            pf[5] = cosine;
            pf[6] = (endpoint - cosine) / (f32)life;
            cosine = *(f32 *)(config + 0x90);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            cosine = *(f32 *)(config + 0x8c) * factor;
            inclination = *(f32 *)(config + 0x98);
            factor = 0.0f + (unit - inclination) + inclination * effMiscRandFloat(0);
            endpoint = *(f32 *)(config + 0x94) * factor;
            pf[7] = cosine;
            pf[8] = (endpoint - cosine) / (f32)life;
            cosine = *(f32 *)(config + 0x68);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            pf[9] = *(f32 *)(config + 0x60) * factor;
            pf[10] = *(f32 *)(config + 0x64) * factor;
            cosine = *(f32 *)(config + 0xa0);
            factor = 0.0f + (unit - cosine) + cosine * effMiscRandFloat(0);
            pf[2] = *(f32 *)(config + 0x9c) * factor;

            if (firstFrame) {
                f32 remainder = (f32)(u32)(effMiscRand(0) % (u32)life);
                pf[3] = pf[3] + pf[4] * remainder;
                pf[5] = pf[5] + pf[6] * remainder;
                pf[7] = pf[7] + pf[8] * remainder;
                *particle = (s32)remainder;
            } else {
                *particle = 0;
            }

            births--;
            continue;
        }
        if (age >= life) {
            s32 vertex;
            for (vertex = 0; vertex < 5; vertex++) {
                vertices[vertex].x = zero;
                vertices[vertex].y = zero;
                vertices[vertex].z = zero;
                colors[vertex] = 0;
            }
            *particle = recycle ? -1 : -2;
        } else {
            f32 time = (f32)age;
            f32 fade;
            f32 sine;
            f32 lengthSquared;
            float squares[4];
            factor = 0.0f + pf[2] + half * (acceleration * time);
            phase = time * factor;
            phase += pf[1];
            inclination = pf[7];
            radialPosition = pf[5];
            cosine = cosf(phase);
            sine = sinf(phase);
            center.lane[0] = cosine * radialPosition;
            center.lane[2] = sine * radialPosition;
            direction.lane[0] = cosine * inclination;
            direction.lane[1] = unit - inclination;
            direction.lane[2] = sine * inclination;
            effectVuLoad10(&direction);
            /* vmul.xyz vf2 = dir*dir; ACC/vmadd sum the lanes in x,y,z order into
             * vf2.w; vrsqrt Q = 1/sqrt(vf2.w); vmulq.xyz normalizes. */
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            lengthSquared = ((p4pc_vu0.vf[2].f[0] + p4pc_vu0.vf[2].f[1]) + p4pc_vu0.vf[2].f[2]);
            squares[0] = 0.0f;
            squares[1] = 0.0f;
            squares[2] = 0.0f;
            squares[3] = lengthSquared;
            p4_vu0_store(2, P4_VU_W, squares);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            direction.lane[0] = sine;
            direction.lane[1] = zero;
            direction.lane[2] = -cosine;
            effectVuLoad10(&direction);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            effectVuScale10(pf[9]);
            effectVuScale11(pf[10]);
            effectVuStore10(&lengthVector);
            effectVuStore11(&widthVector);
            effectVuLoad10(&center);
            effectVuStore10((EffectVuVector *)D_00713D10);
            vertices[3].x = D_00713D10[0];
            vertices[3].y = D_00713D14[0];
            vertices[3].z = D_00713D18[0];
            effectVuLoad11(&lengthVector);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10((EffectVuVector *)D_00713D10);
            vertices[2].x = D_00713D10[0];
            vertices[2].y = D_00713D14[0];
            vertices[2].z = D_00713D18[0];
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10((EffectVuVector *)D_00713D10);
            vertices[4].x = D_00713D10[0];
            vertices[4].y = D_00713D14[0];
            vertices[4].z = D_00713D18[0];
            p4_vu0_move(11, P4_VU_XYZW, 12);
            effectVuScale11(pf[3]);
            effectVuLoad10(&center);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
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
            pf[3] = pf[3] + pf[4];
            pf[7] = pf[7] + pf[8];
            pf[5] = pf[5] + pf[6];

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

            {
                u32 innerValue = innerPacked;
                u32 alpha;
                if ((index & 1) != 0) {
                    colors[3] = innerValue | 0xff000000;
                    alpha = ((u32)(128.0f * fade)) << 24;
                    colors[2] = alpha;
                    colors[4] = alpha;
                } else {
                    colors[3] = innerValue | 0x80000000;
                    alpha = ((u32)(byteScale * fade)) << 24;
                    colors[2] = alpha;
                    colors[4] = alpha;
                }
            }
            *particle = age + 1;
        }
    }
    {
        u8 *geometry = *(u8 **)(*(u8 **)(owner + 0x10) + 0x18);
        func_003c22f0(geometry);
        if (*(u16 *)owner & 4) *(u16 *)(geometry + 0xc) |= 1;
    }
}
