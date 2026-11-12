#include "p4pc/vu0.h"

void SpiralWindUpdateStrip(u8 *arg0)
{
    EffectVuVector point;
    f32 sampleScale;
    f32 one;
    f32 half;
    f32 two;
    f32 spread;
    f32 zero;
    f32 start;
    f32 end;
    f32 t;
    f32 width;
    f32 vStep;
    f32 v0;
    f32 angle;
    f32 step;
    f32 radius;
    f32 accel;
    f32 vSpan;
    f32 spin;
    f32 quarter;
    u32 index;
    s32 segmentCount;
    u8 *instance;
    u32 lifetime;
    u8 *particle;
    u32 sample;
    u8 *vertices;
    u8 *texCoords;
    u8 *config;
    u32 particleCount;
    u32 respawn;
    u32 sampleCount;
    s32 spawnAll;
    u32 pending;
    u32 elapsed;
    u32 limit;
    u8 *list;
    u8 *geometry;
    u32 state;

    config = *(u8 **)(arg0 + 0x40);
    limit = *(u32 *)(config + 0x34);
    elapsed = *(u32 *)(arg0 + 0x34);
    if ((limit < elapsed) && (limit != 0)) {
        return;
    }
    particleCount = *(u32 *)(config + 0x38);
    lifetime = *(u32 *)(config + 0x80);
    if (lifetime == 0) {
        return;
    }
    if ((*(u8 *)(config + 0xB8) != 0) && (elapsed == 0)) {
        spawnAll = 1;
        pending = particleCount;
    } else {
        spawnAll = 0;
        pending = *(u32 *)(config + 0x84);
    }
    list = *(u8 **)(arg0 + 0x3C);
    particle = *(u8 **)list;
    instance = *(u8 **)(list + 4);
    respawn = *(u8 *)(config + 0x88);
    accel = *(f32 *)(config + 0xD4);
    RpGeometryLock(*(RpGeometry **)(*(u8 **)(instance + 0x10) + 0x18), 0xFF2);
    geometry = *(u8 **)(*(u8 **)(instance + 0x10) + 0x18);
    vertices = *(u8 **)(*(u8 **)(geometry + 0x5C) + 0x14);
    texCoords = *(u8 **)(geometry + 0x34);
    segmentCount = *(s16 *)(instance + 8);
    sampleCount = *(u32 *)(config + 0x8C) + 1;
    sampleScale = (f32)sampleCount;
    vSpan = *(f32 *)(config + 0x98);
    vStep = *(f32 *)(config + 0xA4);
    index = 0;
    one = 1.0f;
    half = 0.5f;
    two = 2.0f;
    spread = iGpffff8084;
    spin = iGpffff8080;
    quarter = 0.25f;
    zero = 0.0f;
    for (; index < particleCount; index++, particle += 0x2C) {
        state = *(u32 *)particle;
        if (state == (u32)-2) {
            goto skip;
        }
        if (state == (u32)-1) {
            memset(vertices, 0, segmentCount * 12);
            if (pending == 0) {
                goto skip;
            }
            start = *(f32 *)(config + 0xC0);
            start = *(f32 *)(config + 0xBC) * ((one - start) + start * effMiscRandFloat(0));
            end = *(f32 *)(config + 0xC8);
            *(f32 *)(particle + 0x14) = (*(f32 *)(config + 0xC4) * ((one - end) + end * effMiscRandFloat(0)) - start) / (f32)lifetime;
            *(f32 *)(particle + 0x10) = start;
            *(f32 *)(particle + 0x8) = spread * (two * (effMiscRandFloat(0) - half));
            *(f32 *)(particle + 0xC) = spread * (two * (effMiscRandFloat(0) - half));
            *(f32 *)(particle + 0x1C) = spin * effMiscRandFloat(0);
            t = *(f32 *)(config + 0xD0);
            *(f32 *)(particle + 0x18) = *(f32 *)(config + 0xCC) * ((one - t) + t * effMiscRandFloat(0));
            t = *(f32 *)(config + 0xB4);
            *(f32 *)(particle + 0x20) = *(f32 *)(config + 0xB0) * ((one - t) + t * effMiscRandFloat(0));
            t = *(f32 *)(config + 0xAC);
            *(f32 *)(particle + 0x24) = *(f32 *)(config + 0xA8) * ((one - t) + t * effMiscRandFloat(0));
            t = *(f32 *)(config + 0x9C);
            *(f32 *)(particle + 0x28) = *(f32 *)(config + 0x98) * ((one - t) + t * effMiscRandFloat(0));
            if (spawnAll != 0) {
                *(u32 *)particle = effMiscRand(0) % lifetime;
                t = (f32)*(s32 *)particle;
                *(f32 *)(particle + 0x10) = *(f32 *)(particle + 0x10) + *(f32 *)(particle + 0x14) * t;
            } else {
                *(u32 *)particle = 0;
            }
            pending--;
            goto skip;
        }
        if (state >= lifetime) {
            if (respawn != 0) {
                *(u32 *)particle = (u32)-1;
            } else {
                memset(vertices, 0, segmentCount * 12);
                *(u32 *)particle = (u32)-2;
            }
            if (iGpffffbb64.c3 != 0xFF) {
                u8 *color = *(u8 **)(*(u8 **)(instance + 0x54) + (u16)index * 4);
                *(PolygonWindColor *)(color + 4) = iGpffffbb64;
            } else {
                iGpffffbb64.c3 = 0xFE;
                {
                    u8 *color = *(u8 **)(*(u8 **)(instance + 0x54) + (u16)index * 4);
                    *(PolygonWindColor *)(color + 4) = iGpffffbb64;
                }
                iGpffffbb64.c3 = 0xFF;
            }
            goto skip;
        }
        t = (f32)(s32)state;
        angle = t * (*(f32 *)(particle + 0x18) + half * (accel * t));
        angle += *(f32 *)(particle + 0x1C);
        radius = (*(f32 *)(particle + 0x10) += *(f32 *)(particle + 0x14));
        width = quarter * *(f32 *)(particle + 0x20);
        if (!(*(f32 *)(particle + 0x18) < zero)) {
            step = *(f32 *)(particle + 0x24) / sampleScale;
        } else {
            step = -*(f32 *)(particle + 0x24) / sampleScale;
        }
        v0 = *(f32 *)(particle + 0x28);
        effMiscRotateXVU(*(f32 *)(particle + 0x8));
        effMiscRotateYVU(*(f32 *)(particle + 0xC));
        effMiscCombineRotationVU();
        point.lane[0] = cosf(angle);
        point.lane[1] = zero;
        point.lane[2] = sinf(angle);
        for (sample = 0; sample < sampleCount; sample++) {
            float lenSq[4];

            /* vf10 = vf28*x + vf29*y + vf30*z (rotation columns from the effMisc setup). */
            effectVuLoad10(&point);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            effectVuScale10(radius);
            p4_vu0_move(12, P4_VU_XYZW, 10);
            angle += step;
            point.lane[0] = cosf(angle);
            point.lane[1] = zero;
            point.lane[2] = sinf(angle);
            effectVuLoad10(&point);
            p4_vu0_mul(P4_VU_ACC, P4_VU_XYZW, 28, 10, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_XYZW, 29, 10, P4_VU_BC_Y);
            p4_vu0_madd(10, P4_VU_XYZW, 30, 10, P4_VU_BC_Z);
            effectVuScale10(radius);
            p4_vu0_move(11, P4_VU_XYZW, 12);
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_opmula(10, 11);
            p4_vu0_opmsub(10, 11, 10);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            lenSq[0] = 0.0f;
            lenSq[1] = 0.0f;
            lenSq[2] = 0.0f;
            lenSq[3] = (p4pc_vu0.vf[2].f[0] + p4pc_vu0.vf[2].f[1]) + p4pc_vu0.vf[2].f[2];
            p4_vu0_store(2, P4_VU_W, lenSq);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            effectVuScale10(width);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0xC) = D_00713D10[0];
            *(f32 *)(vertices + 0x10) = D_00713D14[0];
            *(f32 *)(vertices + 0x14) = D_00713D18[0];
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x0) = D_00713D10[0];
            *(f32 *)(vertices + 0x4) = D_00713D14[0];
            *(f32 *)(vertices + 0x8) = D_00713D18[0];
            p4_vu0_move(10, P4_VU_XYZW, 12);
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x18) = D_00713D10[0];
            *(f32 *)(vertices + 0x1C) = D_00713D14[0];
            *(f32 *)(vertices + 0x20) = D_00713D18[0];
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_sqc2(D_00713D10, 10);
            *(f32 *)(vertices + 0x24) = D_00713D10[0];
            *(f32 *)(vertices + 0x28) = D_00713D14[0];
            *(f32 *)(vertices + 0x2C) = D_00713D18[0];
            vertices += 0x30;
            {
                f32 v = v0 + vSpan * ((f32)sample / sampleScale);
                *(f32 *)(texCoords + 0x4) = v;
                *(f32 *)(texCoords + 0xC) = v;
                *(f32 *)(texCoords + 0x14) = v;
                *(f32 *)(texCoords + 0x1C) = v;
            }
            texCoords += 0x20;
        }
        *(f32 *)(particle + 0x28) = *(f32 *)(particle + 0x28) + vStep;
        *(u32 *)particle = *(u32 *)particle + 1;
        continue;
skip:
        vertices += segmentCount * 12;
        texCoords += segmentCount * 8;
    }
    geometry = *(u8 **)(*(u8 **)(instance + 0x10) + 0x18);
    func_003c22f0((RpGeometry *)geometry);
    if ((*(u16 *)instance & 4) != 0) {
        *(u16 *)(geometry + 0xC) |= 1;
    }
}
