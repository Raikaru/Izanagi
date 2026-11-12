/* func_00391c10 (2616 bytes): finds the collision-world triangles that touch the sphere arg1 (centre x, y, z and radius at +0xC, floats) and calls arg3(arg2, leaf, &rec, arg4, t)
 * for each, stopping when the callback returns 0; t is the signed plane distance of the centre divided by the radius. The walk is that of func_00391400 with the bounds of the sphere
 * (outer box c +- r, inner box c +- 0.57 r, the constant 0x3F11EB85): a kd node sends the sphere to the high child (word 3) when its plane float +0x14 is at or below the outer maximum on that
 * axis, also pushing the low child (word 2) when the float +0x10 is not below the outer minimum, else to the low child only. A leaf mesh (*(leaf + *(s32 *)0x007646C0), gp-0x4A30; gp is
 * 0x007690F0) is skipped when the squared distance of the centre to its box (min at +0x10, +0x14, +0x18, max at +4, +8, +0xC) exceeds r * r or the box does not reach the outer box.
 * The second-level tree carries the six-bit mask of func_00391400 initialised against the inner box; an entry with mask 0 yields every triangle below it, a pair of children is chosen
 * by its plane floats at +0xC / +4 against the outer maximum / minimum and the bits are cleared against the inner minimum / maximum. A triangle (index through the halfword table
 * mesh[0x24 / 4] when not null) of an entry with a nonzero mask goes through func_003d9760(sphere, v0, v1, v2, &normal, &dist); for mask 0 the normal is made as in func_00392650 and the
 * distance is (c - v0) . n. The record rec = {unit normal, first vertex, triangle number, &v0, &v1, &v2} is passed to the callback. */

extern s32 p4c_003d9760_00391c10(f32 *s, f32 *a, f32 *b, f32 *c, f32 *n, f32 *d) __asm__("func_003d9760");

typedef s32 (*P4SphHitFn_00391c10)(u32 shape, u8 *leaf, void *rec, f32 t, u32 ctx);

typedef struct {
    u32 node;
    u32 mask;
} P4Ent2_00391c10;

void func_00391c10(u8 *arg0, f32 *arg1, u32 arg2, P4SphHitFn_00391c10 arg3, u32 arg4)
{
    u32 k57 = 0x3F11EB85;
    f32 r = arg1[3];
    f32 invr = 1.0f / r;
    f32 h = *(f32 *)&k57 * r;
    f32 omax[3];
    f32 omin[3];
    f32 imax[3];
    f32 imin[3];
    u32 s1[64];
    P4Ent2_00391c10 s2[40];
    s32 sp1;
    s32 sp2;
    u8 fake[8];
    u32 rec[12];
    u32 node;
    u32 mesh;
    u8 *tris;
    u8 *verts;
    u32 i;

    for (i = 0; i < 12; i++) {
        rec[i] = 0;
    }
    omin[0] = arg1[0] - r;
    omin[1] = arg1[1] - r;
    omin[2] = arg1[2] - r;
    omax[0] = arg1[0] + r;
    omax[1] = arg1[1] + r;
    omax[2] = arg1[2] + r;
    imin[0] = arg1[0] - h;
    imin[1] = arg1[1] - h;
    imin[2] = arg1[2] - h;
    imax[0] = arg1[0] + h;
    imax[1] = arg1[1] + h;
    imax[2] = arg1[2] + h;
    s1[0] = *(u32 *)(arg0 + 0x1C);
    sp1 = 1;
    while (sp1 > 0) {
        sp1--;
        node = s1[sp1];
        while ((s32) * (u32 *)node >= 0) {
            u32 axis = *(u32 *)node;

            if (*(f32 *)(node + 0x14) <= omax[axis / 4]) {
                s1[sp1] = *(u32 *)(node + 0xC);
                if (!(*(f32 *)(node + 0x10) < omin[axis / 4])) {
                    sp1++;
                    s1[sp1] = *(u32 *)(node + 8);
                }
            } else {
                s1[sp1] = *(u32 *)(node + 8);
            }
            node = s1[sp1];
        }
        mesh = *(u32 *)(node + *(s32 *)0x007646C0);
        if (mesh == 0) {
            continue;
        }
        {
            f32 *mf = (f32 *)mesh;
            f32 dx;
            f32 dy;
            f32 dz;
            f32 d2;

            if (arg1[0] < mf[0x10 / 4]) {
                dx = mf[0x10 / 4] - arg1[0];
            } else if (arg1[0] <= mf[1]) {
                dx = 0.0f;
            } else {
                dx = arg1[0] - mf[1];
            }
            if (arg1[1] < mf[0x14 / 4]) {
                dy = mf[0x14 / 4] - arg1[1];
            } else if (arg1[1] <= mf[2]) {
                dy = 0.0f;
            } else {
                dy = arg1[1] - mf[2];
            }
            if (arg1[2] < mf[0x18 / 4]) {
                dz = mf[0x18 / 4] - arg1[2];
            } else if (arg1[2] <= mf[3]) {
                dz = 0.0f;
            } else {
                dz = arg1[2] - mf[3];
            }
            d2 = dx * dx;
            d2 = (0.0f + d2) + dy * dy;
            d2 = (0.0f + d2) + dz * dz;
            if (!(d2 <= r * r)) {
                continue;
            }
            tris = *(u8 **)(node + 4);
            verts = *(u8 **)(node + 8);
            if (!(mf[0x10 / 4] <= omax[0]) || !(mf[0x14 / 4] <= omax[1]) || !(mf[0x18 / 4] <= omax[2]) || mf[1] < omin[0] || mf[2] < omin[1] || mf[3] < omin[2]) {
                continue;
            }
            fake[1] = 0xFF;
            *(u16 *)(fake + 2) = 0;
            if (*(u16 *)(mesh + 0x1E) == 0) {
                fake[1] = (u8) * (u16 *)(mesh + 0x1C);
            }
            s2[0].node = (u32)fake;
            s2[0].mask = (mf[0x14 / 4] < imin[1] ? 4 : 0) | (mf[0x10 / 4] < imin[0] ? 1 : 0) | (mf[0x18 / 4] < imin[2] ? 0x10 : 0) | (mf[1] <= imax[0] ? 0 : 2)
                         | (mf[2] <= imax[1] ? 0 : 8) | (mf[3] <= imax[2] ? 0 : 0x20);
        }
        sp2 = 1;
        while (sp2 > 0) {
            u32 n2;
            u32 first;
            u32 count;
            u32 mask;
            u32 ti;
            u32 tend;

            sp2--;
            for (;;) {
                n2 = s2[sp2].node;
                if (*(u8 *)(n2 + 1) != 0xFF) {
                    mask = s2[sp2].mask;
                    count = *(u8 *)(n2 + 1);
                    first = *(u16 *)(n2 + 2);
                    break;
                }
                if (s2[sp2].mask == 0) {
                    u8 *base = *(u8 **)(mesh + 0x20);
                    u8 *N = base + ((u32) * (u16 *)(n2 + 2) << 4);
                    u8 *lm = N;
                    u8 *rm;
                    u32 rc;

                    if (lm[1] == 0xFF) {
                        do {
                            lm = base + ((u32) * (u16 *)(lm + 2) << 4);
                        } while (lm[1] == 0xFF);
                    }
                    rc = N[9];
                    rm = N + 8;
                    if (rc == 0xFF) {
                        do {
                            u8 *q = base + ((u32) * (u16 *)(rm + 2) << 4);

                            rc = q[9];
                            rm = q + 8;
                        } while (rc == 0xFF);
                    }
                    mask = 0;
                    first = *(u16 *)(lm + 2);
                    count = (*(u16 *)(rm + 2) + rc - first) & 0xFFFF;
                    break;
                }
                {
                    u8 *N = *(u8 **)(mesh + 0x20) + ((u32) * (u16 *)(n2 + 2) << 4);
                    u32 axis = *N & 0xC;

                    if (*(f32 *)(N + 0xC) <= omax[axis / 4]) {
                        s2[sp2].node = (u32)N + 8;
                        if (!(*(f32 *)(N + 4) < omin[axis / 4])) {
                            sp2++;
                            s2[sp2].node = (u32)N;
                            s2[sp2].mask = s2[sp2 - 1].mask;
                            if (*(f32 *)(N + 4) <= imax[axis / 4]) {
                                s2[sp2].mask = s2[sp2].mask & ~(2u << (axis >> 1));
                            }
                            if (!(*(f32 *)(N + 0xC) < imin[axis / 4])) {
                                s2[sp2 - 1].mask = s2[sp2 - 1].mask & ~(1u << (axis >> 1));
                            }
                        }
                    } else {
                        s2[sp2].node = (u32)N;
                    }
                }
            }
            tend = ((first & 0xFFFF) + (count & 0xFFFF));
            for (ti = first & 0xFFFF; (s32)ti < (s32)tend; ti = (ti + 1) & 0xFFFF) {
                u32 tri;
                u16 *tr;
                f32 *p0;
                f32 *p1;
                f32 *p2;
                f32 e1x, e1y, e1z, e2x, e2y, e2z;
                f32 nx, ny, nz, len, inv, a, dist;

                if (*(u32 *)(mesh + 0x24) != 0) {
                    tri = *(u16 *)(*(u32 *)(mesh + 0x24) + (ti << 1));
                } else {
                    tri = ti;
                }
                tri = tri & 0xFFFF;
                tr = (u16 *)(tris + tri * 8);
                p0 = (f32 *)(verts + tr[0] * 12);
                p1 = (f32 *)(verts + tr[1] * 12);
                p2 = (f32 *)(verts + tr[2] * 12);
                if (mask != 0) {
                    if (p4c_003d9760_00391c10(arg1, p0, p1, p2, (f32 *)rec, &dist) == 0) {
                        continue;
                    }
                } else {
                    e1x = p1[0] - p0[0];
                    e1y = p1[1] - p0[1];
                    e2x = p2[0] - p0[0];
                    e1z = p1[2] - p0[2];
                    e2z = p2[2] - p0[2];
                    e2y = p2[1] - p0[1];
                    a = e1y * e2z;
                    nx = a - e1z * e2y;
                    a = e1z * e2x;
                    ny = a - e1x * e2z;
                    a = e1x * e2y;
                    nz = a - e1y * e2x;
                    a = ny * ny;
                    a = a + nx * nx;
                    len = __builtin_sqrtf(a + nz * nz);
                    inv = len;
                    if (!(len <= 0.0f)) {
                        inv = 1.0f / len;
                    }
                    nx = nx * inv;
                    ny = ny * inv;
                    nz = nz * inv;
                    *(f32 *)&rec[0] = nx;
                    *(f32 *)&rec[1] = ny;
                    *(f32 *)&rec[2] = nz;
                    a = (arg1[1] - p0[1]) * ny;
                    a = a + (arg1[0] - p0[0]) * nx;
                    dist = a + (arg1[2] - p0[2]) * nz;
                }
                dist = dist * invr;
                rec[3] = *(u32 *)&p0[0];
                rec[4] = *(u32 *)&p0[1];
                rec[5] = *(u32 *)&p0[2];
                rec[6] = tri;
                rec[7] = (u32)p0;
                rec[8] = (u32)p1;
                rec[9] = (u32)p2;
                if (arg3(arg2, (u8 *)node, rec, dist, arg4) == 0) {
                    return;
                }
            }
        }
    }
}
