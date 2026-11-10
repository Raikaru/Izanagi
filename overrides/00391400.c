/* func_00391400 (2052 bytes): finds the collision-world triangles that overlap the axis-aligned box arg1 (min x, y, z at +0, max x, y, z at +0xC, floats) and calls
 * arg2(arg1, leaf, &rec, arg3, 0.0f) for each, stopping when the callback returns 0. The world arg0 is walked like in func_00390680 (kd tree from arg0[7]; leaves hold the triangle
 * array in word 1, the vertex array in word 2 and the mesh at *(leaf + *(s32 *)0x007646C0) (gp-0x4A30; gp is 0x007690F0)), but with a box: an inner kd node sends the box to the
 * high child (word 3) when its plane float +0x14 is at or below the box minimum, also pushing the low child (word 2) when the plane float +0x10 is not below the box maximum, and
 * otherwise to the low child only. A leaf mesh is skipped unless mesh[+0x10, +0x14, +0x18] <= the box minimum and the box maximum <= mesh[+4, +8, +0xC]. The mesh's
 * second-level tree (8-byte nodes, pairs of children in 16-byte records at mesh[0x20 / 4]; root as in func_00390680) carries a six-bit mask of the sides of the box that are not yet
 * known to be inside the node (1 / 2 for x, 4 / 8 for y, 0x10 / 0x20 for z, initialised from the mesh bounds); an entry whose mask is 0 contributes every triangle below it (the
 * range from the first triangle of its leftmost leaf to the end of its rightmost leaf), otherwise the children are chosen from the plane floats at +0xC / +4 of the pair like the kd nodes and
 * the bits whose plane the box crosses are cleared. A triangle (index through the halfword table mesh[0x24 / 4] when it is not null) of an entry with a nonzero mask must pass
 * func_003d9f30(box, v0, v1, v2); the record rec = {unit normal, first vertex, triangle number, &v0, &v1, &v2} is then built as in func_00392650. */

extern s32 p4c_003d9f30_00391400(f32 *box, f32 *a, f32 *b, f32 *c) __asm__("func_003d9f30");

typedef s32 (*P4BoxHitFn_00391400)(u8 *box, u8 *leaf, void *rec, f32 t, u32 ctx);

typedef struct {
    u32 node;
    u32 mask;
} P4Ent2_00391400;

void func_00391400(u8 *arg0, u8 *arg1, P4BoxHitFn_00391400 arg2, u32 arg3)
{
    f32 *box = (f32 *)arg1;
    u32 s1[64];
    P4Ent2_00391400 s2[40];
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
    s1[0] = *(u32 *)(arg0 + 0x1C);
    sp1 = 1;
    while (sp1 > 0) {
        sp1--;
        node = s1[sp1];
        while ((s32) * (u32 *)node >= 0) {
            u32 axis = *(u32 *)node;

            if (*(f32 *)(node + 0x14) <= box[axis / 4]) {
                s1[sp1] = *(u32 *)(node + 0xC);
                if (!(*(f32 *)(node + 0x10) < box[3 + axis / 4])) {
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
        tris = *(u8 **)(node + 4);
        verts = *(u8 **)(node + 8);
        {
            f32 *mf = (f32 *)mesh;

            if (!(mf[0x10 / 4] <= box[0]) || !(mf[0x14 / 4] <= box[1]) || !(mf[0x18 / 4] <= box[2]) || mf[1] < box[3] || mf[2] < box[4] || mf[3] < box[5]) {
                continue;
            }
        }
        fake[1] = 0xFF;
        *(u16 *)(fake + 2) = 0;
        if (*(u16 *)(mesh + 0x1E) == 0) {
            fake[1] = (u8) * (u16 *)(mesh + 0x1C);
        }
        {
            f32 *mf = (f32 *)mesh;
            u32 mk;

            mk = (mf[0x14 / 4] < box[4] ? 4 : 0) | (mf[0x10 / 4] < box[3] ? 1 : 0) | (mf[0x18 / 4] < box[5] ? 0x10 : 0) | (mf[1] <= box[0] ? 0 : 2)
                 | (mf[2] <= box[1] ? 0 : 8) | (mf[3] <= box[2] ? 0 : 0x20);
            s2[0].node = (u32)fake;
            s2[0].mask = mk;
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
                    f32 *mn = box;
                    f32 *mx = box + 3;

                    if (*(f32 *)(N + 0xC) <= mn[axis / 4]) {
                        s2[sp2].node = (u32)N + 8;
                        if (!(*(f32 *)(N + 4) < mx[axis / 4])) {
                            sp2++;
                            s2[sp2].node = (u32)N;
                            s2[sp2].mask = s2[sp2 - 1].mask;
                            if (*(f32 *)(N + 4) <= mn[axis / 4]) {
                                s2[sp2].mask = s2[sp2].mask & ~(2u << (axis >> 1));
                            }
                            if (!(*(f32 *)(N + 0xC) < mx[axis / 4])) {
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
                f32 nx, ny, nz, len, inv, a;

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
                if (mask != 0 && p4c_003d9f30_00391400(box, p0, p1, p2) == 0) {
                    continue;
                }
                rec[3] = *(u32 *)&p0[0];
                rec[4] = *(u32 *)&p0[1];
                rec[5] = *(u32 *)&p0[2];
                rec[6] = tri;
                rec[7] = (u32)p0;
                rec[8] = (u32)p1;
                rec[9] = (u32)p2;
                e1x = p1[0] - p0[0];
                e2x = p2[0] - p0[0];
                e1z = p1[2] - p0[2];
                e1y = p1[1] - p0[1];
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
                *(f32 *)&rec[0] = nx * inv;
                *(f32 *)&rec[1] = ny * inv;
                *(f32 *)&rec[2] = nz * inv;
                if (arg2(arg1, (u8 *)node, rec, 0.0f, arg3) == 0) {
                    return;
                }
            }
        }
    }
}
