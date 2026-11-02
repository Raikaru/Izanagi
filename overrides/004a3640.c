#include "p4pc/vu0.h"

void FallingFunnelWindUpdateStrip(u8 *arg0)
{
    EffectVuVector point;
    EffectVuVector direction;
    f32 t;
    f32 vStep;
    f32 gravity;
    f32 angle;
    f32 height;
    f32 sampleScale;
    f32 c;
    f32 swell;
    f32 centre;
    f32 one;
    f32 half;
    f32 end;
    f32 v0;
    f32 accel;
    f32 vSpan;
    f32 spin;
    f32 ten;
    f32 quarter;
    f32 zero;
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
    s32 state;

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
    accel = *(f32 *)(config + 0xD8);
    gravity = *(f32 *)(config + 0xE4);
    RpGeometryLock(*(RpGeometry **)(*(u8 **)(instance + 0x10) + 0x18), 0xFF2);
    geometry = *(u8 **)(*(u8 **)(instance + 0x10) + 0x18);
    vertices = *(u8 **)(*(u8 **)(geometry + 0x5C) + 0x14);
    texCoords = *(u8 **)(geometry + 0x34);
    segmentCount = *(s16 *)(instance + 8);
    sampleCount = *(u32 *)(config + 0x8C) + 1;
    sampleScale = (f32)sampleCount;
    vSpan = *(f32 *)(config + 0x98);
    centre = *(f32 *)(config + 0xEC);
    vStep = *(f32 *)(config + 0xA4);
    index = 0;
    one = 1.0f;
    spin = iGpffff8080;
    ten = 10.0f;
    half = 0.5f;
    quarter = 0.25f;
    zero = 0.0f;
    goto check;
body:
    {
        state = *(s32 *)particle;
        if (state == -2) {
            goto skip;
        }
        if (state == -1) {
            memset(vertices, 0, segmentCount * 12);
            if (pending == 0) {
                goto skip;
            }
            t = *(f32 *)(config + 0xC0);
            t = *(f32 *)(config + 0xBC) * ((one - t) + t * effMiscRandFloat(0));
            end = *(f32 *)(config + 0xC8);
            *(f32 *)(particle + 0xC) = (*(f32 *)(config + 0xC4) * ((one - end) + end * effMiscRandFloat(0)) - t) / (f32)lifetime;
            *(f32 *)(particle + 0x8) = t;
            t = *(f32 *)(config + 0xE0);
            *(f32 *)(particle + 0x10) = *(f32 *)(config + 0xDC) * ((one - t) + t * effMiscRandFloat(0));
            *(f32 *)(particle + 0x14) = *(f32 *)(config + 0xCC) * effMiscRandFloat(0);
            t = *(f32 *)(config + 0xE8);
            *(f32 *)(particle + 0x28) = *(f32 *)(config + 0xF0) * ((one - t) + t * effMiscRandFloat(0));
            *(f32 *)(particle + 0x1C) = spin * effMiscRandFloat(0);
            t = *(f32 *)(config + 0xD4);
            *(f32 *)(particle + 0x18) = *(f32 *)(config + 0xD0) * ((one - t) + t * effMiscRandFloat(0));
            t = *(f32 *)(config + 0xB4);
            *(f32 *)(particle + 0x20) = *(f32 *)(config + 0xB0) * ((one - t) + t * effMiscRandFloat(0));
            t = *(f32 *)(config + 0xAC);
            *(f32 *)(particle + 0x24) = ten * (*(f32 *)(config + 0xA8) * ((one - t) + t * effMiscRandFloat(0)));
            t = *(f32 *)(config + 0x9C);
            *(f32 *)(particle + 0x2C) = *(f32 *)(config + 0x98) * ((one - t) + t * effMiscRandFloat(0));
            if (spawnAll != 0) {
                *(u32 *)particle = effMiscRand(0) % lifetime;
                t = (f32)*(s32 *)particle;
                *(f32 *)(particle + 0x14) = *(f32 *)(particle + 0x14) + *(f32 *)(particle + 0x10) * t;
                *(f32 *)(particle + 0x14) = *(f32 *)(particle + 0x14) - half * (t * (gravity * t));
                *(f32 *)(particle + 0x8) = *(f32 *)(particle + 0x8) + *(f32 *)(particle + 0xC) * t;
            } else {
                *(u32 *)particle = 0;
            }
            pending--;
            goto skip;
        }
        if ((u32)state >= lifetime) {
            if (respawn != 0) {
                *(s32 *)particle = -1;
            } else {
                memset(vertices, 0, segmentCount * 12);
                *(s32 *)particle = -2;
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
        v0 = *(f32 *)(particle + 0x2C);
        end = (f32)state;
        angle = end * (*(f32 *)(particle + 0x18) + half * (accel * end));
        angle += *(f32 *)(particle + 0x1C);
        {
            f32 base = *(f32 *)(particle + 0x8);
            f32 offset;
            f32 width;
            f32 stepLength;
            f32 s;
            f32 lenSq[4];

            height = *(f32 *)(particle + 0x14);
            swell = *(f32 *)(particle + 0x28);
            offset = end / (f32)lifetime - centre;
            t = base + swell * (offset * offset);
            c = cosf(angle);
            s = sinf(angle);
            point.lane[0] = c * t;
            point.lane[1] = height;
            point.lane[2] = s * t;
            effectVuLoad11(&point);
            p4_vu0_move(12, P4_VU_XYZW, 11);
            height = (height + *(f32 *)(particle + 0x10)) - gravity * end;
            base = (*(f32 *)(particle + 0x8) + *(f32 *)(particle + 0xC)) + swell * (((one + end) / (f32)lifetime - centre) * ((one + end) / (f32)lifetime - centre));
            point.lane[0] = c * base;
            point.lane[1] = height;
            point.lane[2] = s * base;
            effectVuLoad10(&point);
            /* vf10 = (new - old) direction; normalise its xyz with Q = 1/sqrt(x^2+y^2+z^2). */
            p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            lenSq[0] = 0.0f;
            lenSq[1] = 0.0f;
            lenSq[2] = 0.0f;
            lenSq[3] = (p4pc_vu0.vf[2].f[0] + p4pc_vu0.vf[2].f[1]) + p4pc_vu0.vf[2].f[2];
            p4_vu0_store(2, P4_VU_W, lenSq);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            effectVuStore10(&direction);
            *(f32 *)(particle + 0x14) = height;
            *(f32 *)(particle + 0x8) = *(f32 *)(particle + 0x8) + *(f32 *)(particle + 0xC);
            width = quarter * *(f32 *)(particle + 0x20);
            stepLength = *(f32 *)(particle + 0x24) / sampleScale;
            point.lane[0] = c;
            point.lane[1] = zero;
            point.lane[2] = s;
            effectVuLoad11(&point);
            /* vf10 = ACC - (vf11 x vf10) with ACC = vf10 x vf11 (cross product, twice). */
            p4_vu0_opmula(10, 11);
            p4_vu0_opmsub(10, 11, 10);
            effectVuScale10(width);
            p4_vu0_move(11, P4_VU_XYZW, 10);
            for (sample = 0; sample < sampleCount; sample++) {
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
                effectVuLoad10(&direction);
                effectVuScale10(stepLength);
                p4_vu0_add(12, P4_VU_XYZW, 12, 10, P4_VU_NOBC);
            }
        }
        *(f32 *)(particle + 0x2C) = *(f32 *)(particle + 0x2C) + vStep;
        *(u32 *)particle = *(u32 *)particle + 1;
        goto next;
skip:
        vertices += segmentCount * 12;
        texCoords += segmentCount * 8;
    }
next:
    index++;
    particle += 0x30;
check:
    if (index < particleCount) {
        goto body;
    }
    geometry = *(u8 **)(*(u8 **)(instance + 0x10) + 0x18);
    func_003c22f0((RpGeometry *)geometry);
    if ((*(u16 *)instance & 4) != 0) {
        *(u16 *)(geometry + 0xC) |= 1;
    }
}
