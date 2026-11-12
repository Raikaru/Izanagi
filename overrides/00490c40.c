#include "p4pc/vu0.h"

void mdlEffectUpdateBoundedEmitter(u8 *arg0)
{
    extern void mdlEffectSampleKeyState(u8 *arg0, u8 *arg1, s32 arg2, u_long128 *arg3);
    extern void mdlEffectUpdateParticleTrail(u8 *arg0, u8 *arg1);
    extern void *memcpy(void *dst, const void *src, u32 size);
    extern f32 fabsf(f32 x);
    extern f32 fGpffff807c;
    extern f32 fGpffff8080;
    extern f32 fGpffff8088;
    extern f32 fGpffff808c;
    extern u_long128 D_00922D70;
    EffectVuVector dir;
    EffectVuVector bounce;
    u_long128 prev;
    u_long128 origin;
    union {
        s32 i;
        f32 f;
    } qbits;
    u32 count;
    u32 flags;
    u8 *node;
    u8 *extra;
    u8 *config;
    u8 *self;
    s32 limit;
    s32 respawn;
    s32 divD8;
    s32 spawn;
    s32 burst;
    u32 i;
    s32 state;
    u8 mode;
    f32 half;
    f32 two;
    f32 one;
    f32 zero;
    f32 speed;
    f32 spin;
    f32 negone;
    f32 swayRange;
    f32 swayBase;
    f32 emitBase;
    f32 growth;
    f32 t;
    f32 travel;
    f32 reach;
    f32 dist;
    f32 dot;
    f32 over;

    self = arg0;
    count = *(u32 *)(self + 4);
    flags = *(u32 *)(self + 0xC);
    node = *(u8 **)(self + 0x18);
    extra = *(u8 **)(self + 0x1C);
    config = *(u8 **)(self + 0x20);
    if (*(s32 *)(config + 0xB8) == 0) {
        return;
    }
    if ((flags & 1) == 0) {
        origin = *(u_long128 *)config;
    } else {
        origin = D_00922D70;
    }
    limit = *(s32 *)(config + 0xB8);
    respawn = *(s32 *)(config + 0x20);
    divD8 = *(s32 *)(config + 0xD8);
    mode = *(u8 *)(config + 0x9C);
    growth = *(f32 *)(config + 0xE4);
    dir.lane[3] = 0.0f;
    if (respawn != 0 && !(*(s32 *)(self + 0x10) < respawn)) {
        burst = 0;
        spawn = 0;
    } else if (*(s32 *)(self + 0x10) == 0 && *(u8 *)(config + 0xBD) != 0) {
        burst = 1;
        if (!(*(f32 *)(config + 0x28) <= 0.0f)) {
            spawn = (s32)((f32)*(u32 *)(self + 4) * ((fGpffff807c - *(f32 *)(config + 0x28)) * effMiscRandFloat(0)));
        } else {
            spawn = *(u32 *)(self + 4);
        }
    } else {
        burst = 0;
        if (!(*(f32 *)(config + 0x28) <= 0.0f)) {
            f32 scale = (fGpffff807c - *(f32 *)(config + 0x28)) * effMiscRandFloat(0);
            *(f32 *)(self + 0x14) = *(f32 *)(self + 0x14) + (f32)*(u32 *)(config + 0x24) * scale;
        } else {
            *(f32 *)(self + 0x14) = *(f32 *)(self + 0x14) + (f32)*(u32 *)(config + 0x24);
        }
        emitBase = *(f32 *)(self + 0x14);
        spawn = (s32)fabsf(emitBase);
        *(f32 *)(self + 0x14) = emitBase - (f32)spawn;
    }
    i = 0;
    half = 0.5f;
    two = 2.0f;
    one = 1.0f;
    zero = 0.0f;
    spin = fGpffff8080;
    negone = -1.0f;
    swayRange = fGpffff8088;
    swayBase = fGpffff808c;
    for (; i < count; i++, extra += 0x24, node += 0x20) {
        if (!(*(s32 *)(node + 0x10) < limit)) {
            u8 *cfg;
            s32 nmult;

            *(s32 *)(node + 0x10) = respawn != 0 ? -2 : -1;
            cfg = *(u8 **)(self + 0x20);
            nmult = *(s32 *)(cfg + 0xC0) * *(s32 *)(cfg + 0xC4);
            if (nmult != 0) {
                u32 index = (u32)(node - *(u8 **)(self + 0x18)) >> 5;
                u8 *trail = *(u8 **)(self + 0x18) + ((*(u32 *)(self + 4) + index * (u32)nmult) << 5);
                s32 k;

                for (k = 0; k < nmult; trail += 0x20, k++) {
                    *(s32 *)(trail + 0x10) = -1;
                }
            }
        }
        state = *(s32 *)(node + 0x10);
        if (state == -2) {
            continue;
        }
        if (state == -1) {
            /* Spawn: random emission direction, speed, spin and opacity. */
            if (spawn == 0) {
                continue;
            }
            dir.lane[0] = two * (effMiscRandFloat(0) - half);
            dir.lane[1] = two * (effMiscRandFloat(0) - half);
            dir.lane[2] = two * (effMiscRandFloat(0) - half);
            effectVuLoad10(&dir);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
            p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            effectVuStore10(&dir);
            *(f32 *)(extra + 0x0) = dir.lane[0];
            *(f32 *)(extra + 0x4) = dir.lane[1];
            *(f32 *)(extra + 0x8) = dir.lane[2];
            t = *(f32 *)(config + 0xE0);
            *(f32 *)(extra + 0x14) = fabsf(*(f32 *)(config + 0xDC) * ((one - t) + t * effMiscRandFloat(0)));
            t = *(f32 *)(config + 0xCC);
            speed = *(f32 *)(config + 0xC8) * ((one - t) + t * effMiscRandFloat(0));
            t = *(f32 *)(config + 0xD4);
            t = *(f32 *)(config + 0xD0) * ((one - t) + t * effMiscRandFloat(0));
            *(f32 *)(extra + 0xC) = speed;
            *(f32 *)(extra + 0x10) = (t - speed) / (f32)divD8;
            if (burst != 0) {
                f32 step;

                *(s32 *)(node + 0x10) = (s32)(effMiscRand(0) % (u32)limit);
                *(f32 *)(extra + 0xC) = *(f32 *)(extra + 0xC) + *(f32 *)(extra + 0x10) * (f32)*(s32 *)(node + 0x10);
                step = *(f32 *)(extra + 0x10);
                if ((!(step <= zero) && !(*(f32 *)(extra + 0xC) <= t)) ||
                    ((step < zero) && (*(f32 *)(extra + 0xC) < t))) {
                    *(f32 *)(extra + 0xC) = t;
                }
                speed = *(f32 *)(extra + 0xC);
            } else {
                *(s32 *)(node + 0x10) = 0;
            }
            dir.lane[0] = two * (effMiscRandFloat(0) - half);
            dir.lane[1] = two * (effMiscRandFloat(0) - half);
            dir.lane[2] = two * (effMiscRandFloat(0) - half);
            effectVuLoad10(&dir);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
            p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            effectVuScale10(speed);
            if ((flags & 1) == 0) {
                effectVuLoad11((EffectVuVector *)&origin);
                p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            }
            effectVuStore10((EffectVuVector *)node);
            t = *(f32 *)(config + 0x6C);
            *(f32 *)(extra + 0x18) = (one - t) + t * effMiscRandFloat(0);
            state = mode;
            if (state != 2) {
                t = *(f32 *)(config + 0x98);
                *(f32 *)(extra + 0x20) = (one - t) + t * effMiscRandFloat(0);
                if (state == 1) {
                    *(f32 *)(extra + 0x1C) = spin * effMiscRandFloat(0);
                    if ((effMiscRand(0) & 1) != 0) {
                        *(f32 *)(extra + 0x20) = *(f32 *)(extra + 0x20) * negone;
                    }
                } else {
                    *(f32 *)(extra + 0x1C) = zero;
                }
            } else {
                *(f32 *)(extra + 0x1C) = zero;
                *(f32 *)(extra + 0x20) = one;
            }
            mdlEffectSampleKeyState(node, config, *(s32 *)(node + 0x10), (u_long128 *)node);
            *(f32 *)(node + 0x18) = *(f32 *)(node + 0x18) * *(f32 *)(extra + 0x18);
            *(f32 *)(node + 0x1C) = *(f32 *)(node + 0x1C) * *(f32 *)(extra + 0x20);
            *(f32 *)(node + 0x1C) = *(f32 *)(node + 0x1C) + *(f32 *)(extra + 0x1C);
            if (burst != 0) {
                u8 *cfg = *(u8 **)(self + 0x20);
                s32 nmult = *(s32 *)(cfg + 0xC0) * *(s32 *)(cfg + 0xC4);

                if (nmult != 0) {
                    u32 index = (u32)(node - *(u8 **)(self + 0x18)) >> 5;
                    u8 *trail = *(u8 **)(self + 0x18) + ((*(u32 *)(self + 4) + index * (u32)nmult) << 5);

                    memcpy(trail, node, 0x20);
                    *(s32 *)(trail + 0x10) = -1;
                }
                *(s32 *)(node + 0x10) = *(s32 *)(node + 0x10) + 1;
            }
            spawn--;
            continue;
        }

        /* Live particle: drift along the emission direction, then sway/bounce off the origin sphere. */
        prev = *(u_long128 *)node;
        reach = *(f32 *)(extra + 0xC);
        if (*(s32 *)(node + 0x10) < divD8) {
            *(f32 *)(extra + 0xC) = reach + *(f32 *)(extra + 0x10);
        }
        travel = *(f32 *)(extra + 0x14) + growth * (f32)state;
        if (travel < zero) {
            travel = zero;
        }
        dir.lane[0] = *(f32 *)(extra + 0x0);
        dir.lane[1] = *(f32 *)(extra + 0x4);
        dir.lane[2] = *(f32 *)(extra + 0x8);
        p4_vu0_lqc2(12, &dir);
        effectVuLoad11((EffectVuVector *)node);
        p4_vu0_move(10, P4_VU_XYZW, 12);
        effectVuScale10(travel);
        p4_vu0_add(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
        effectVuLoad10((EffectVuVector *)&origin);
        p4_vu0_sub(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
        p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
        p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
        p4_vu0_sqrt(2, P4_VU_BC_X);
        /* Distance is the sqrt result as the bit pattern of VI22 (retail cfc2/mtc1). */
        qbits.i = p4_vu0_cfc2(22);
        dist = qbits.f;
        if (!(dist <= reach)) {
            p4_vu0_mul(2, P4_VU_XYZ, 10, 10, P4_VU_NOBC);
            p4_vu0_mul(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_X);
            p4_vu0_madd(P4_VU_ACC, P4_VU_W, 0, 2, P4_VU_BC_Y);
            p4_vu0_madd(2, P4_VU_W, 0, 2, P4_VU_BC_Z);
            p4_vu0_rsqrt(0, P4_VU_BC_W, 2, P4_VU_BC_W);
            p4_vu0_mulq(10, P4_VU_XYZ, 10);
            p4_vu0_move(11, P4_VU_XYZW, 12);
            p4_vu0_mul(2, P4_VU_XYZ, 10, 11, P4_VU_NOBC);
            p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Y);
            p4_vu0_add(2, P4_VU_X, 2, 2, P4_VU_BC_Z);
            dot = p4pc_vu0.vf[2].f[0];
            effectVuScale10(two * (dot * (swayBase + swayRange * effMiscRandFloat(0))));
            p4_vu0_sub(11, P4_VU_XYZW, 11, 10, P4_VU_NOBC);
            effectVuStore11(&bounce);
            *(f32 *)(extra + 0x0) = bounce.lane[0];
            *(f32 *)(extra + 0x4) = bounce.lane[1];
            *(f32 *)(extra + 0x8) = bounce.lane[2];
            over = dist - reach;
            effectVuLoad10((EffectVuVector *)node);
            p4_vu0_set_xf(2, travel - over);
            p4_vu0_mul(12, P4_VU_XYZW, 12, 2, P4_VU_BC_X);
            p4_vu0_add(10, P4_VU_XYZW, 10, 12, P4_VU_NOBC);
            effectVuScale11(over);
            p4_vu0_add(10, P4_VU_XYZW, 10, 11, P4_VU_NOBC);
            effectVuStore10((EffectVuVector *)node);
        } else {
            effectVuStore11((EffectVuVector *)node);
        }
        mdlEffectSampleKeyState(node, config, state, &prev);
        *(f32 *)(node + 0x18) = *(f32 *)(node + 0x18) * *(f32 *)(extra + 0x18);
        *(f32 *)(node + 0x1C) = *(f32 *)(node + 0x1C) * *(f32 *)(extra + 0x20);
        *(f32 *)(node + 0x1C) = *(f32 *)(node + 0x1C) + *(f32 *)(extra + 0x1C);
        mdlEffectUpdateParticleTrail(self, node);
        *(s32 *)(node + 0x10) = state + 1;
    }
}
