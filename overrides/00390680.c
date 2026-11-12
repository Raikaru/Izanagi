/* func_00390680 (3448 bytes): walks the segment arg1 (origin at +0, end at +0xC, floats) through the collision world arg0 and calls arg2(arg1, leaf, &rec, arg3, t) for
 * every triangle it hits, stopping when the callback returns 0. arg0[7] is the root of a two-level tree: the first level is a kd tree whose inner nodes have the axis (a byte
 * offset 0, 4 or 8, negative for a leaf) in word 0, the child nodes in words 2 (low) and 3 (high) and the plane coordinates in words 4 and 5 (floats at +0x10 / +0x14);
 * a leaf holds the triangle array in word 1, the vertex array in word 2 and the mesh at *(leaf + *(s32 *)0x007646C0) (gp-0x4A30; gp is 0x007690F0), skipped when null. The mesh's
 * second-level tree has 8-byte nodes (byte +1 = 0xFF for an inner node whose halfword +2 indexes a 16-byte pair of children at mesh[0x20 / 4] + 16 * index, otherwise the leaf's
 * triangle count with the first index in halfword +2); a pair has the axis in byte 0 (& 0xC) and the plane coordinates at +4 and +0xC. The root is the pseudo node
 * {byte +1 = 0xFF, index 0} when the mesh's halfword +0x1E is nonzero, else {count = halfword +0x1C, index 0}. Triangle indices go through the halfword table mesh[0x24 / 4] when it is not null.
 * Both levels clip the parameter interval [t0, t1] against the plane slabs, widened by the tolerance 0x358637BD * the largest |coordinate| of the segment per axis, and defer the far child on a
 * stack of {node, t0, t1}. A triangle is tested as in func_00392650 (Moller-Trumbore with the 0x322BCC77 / 0x3727C5AC tolerances) and the record rec = {unit normal, first vertex, triangle number,
 * &v0, &v1, &v2} goes to the callback together with t = t_num / det. All single precision, accumulations in the order of the FPU's accumulator. */

typedef s32 (*P4RayHitFn_00390680)(u8 *shape, u8 *leaf, void *rec, f32 t, u32 ctx); /* host argument order: the callbacks (FldFrame_RaycastTriangle & co) take the distance third, before the data word; on the EE f12 vs a2 made the order irrelevant */

typedef struct {
    u32 node;
    f32 t0;
    f32 t1;
} P4Ent_00390680;

static f32 p4_abs_00390680(f32 v)
{
    return v < 0.0f ? -v : v;
}

void func_00390680(u8 *arg0, u8 *arg1, P4RayHitFn_00390680 arg2, u32 arg3)
{
    f32 *sh = (f32 *)arg1;
    f32 o[3];
    f32 d[3];
    f32 invd[3];
    f32 tol[3];
    f32 mx[3];
    P4Ent_00390680 s1[64];
    P4Ent_00390680 s2[40];
    s32 sp1;
    s32 sp2;
    u8 fake[8];
    u32 k1 = 0x358637BD;
    u32 k2 = 0x322BCC77;
    u32 k3 = 0x3727C5AC;
    f32 epsTol = *(f32 *)&k1;
    f32 epsDet = *(f32 *)&k2;
    f32 slack = *(f32 *)&k3;
    u32 i;
    u32 node;
    u32 rec[12];
    u8 *mesh;
    u8 *tris;
    u8 *verts;
    u32 leaf;

    for (i = 0; i < 12; i++) {
        rec[i] = 0;
    }
    o[0] = sh[0];
    o[1] = sh[1];
    o[2] = sh[2];
    d[0] = sh[3] - sh[0];
    d[1] = sh[4] - sh[1];
    d[2] = sh[5] - sh[2];
    for (i = 0; i < 3; i++) {
        invd[i] = 0.0f;
        if (!(d[i] == 0.0f)) {
            invd[i] = 1.0f / d[i];
        }
        mx[i] = (p4_abs_00390680(sh[i]) < p4_abs_00390680(sh[3 + i])) ? p4_abs_00390680(sh[3 + i]) : p4_abs_00390680(sh[i]);
    }
    tol[2] = mx[2] * epsTol;
    tol[0] = mx[0] * epsTol;
    tol[1] = mx[1] * epsTol;
    s1[0].node = *(u32 *)(arg0 + 0x1C);
    s1[0].t0 = 0.0f;
    s1[0].t1 = 1.0f;
    sp1 = 1;
    while (sp1 > 0) {
        sp1--;
        node = s1[sp1].node;
        while ((s32) * (u32 *)node >= 0) {
            u32 axis = *(u32 *)node;
            f32 A = *(f32 *)(node + 0x10);
            f32 B = *(f32 *)(node + 0x14);
            f32 oo = o[axis / 4];
            f32 dd = d[axis / 4];
            f32 tl = tol[axis / 4];
            f32 iv = invd[axis / 4];
            f32 p1 = (0.0f + oo) + s1[sp1].t1 * dd;
            f32 hiT = B - tl;
            f32 loT = A + tl;
            f32 p0 = (0.0f + oo) + s1[sp1].t0 * dd;
            f32 a;

            if (!(dd <= 0.0f)) {
                if (!(hiT <= p1)) {
                    s1[sp1].node = *(u32 *)(node + 8);
                } else if (loT < p0) {
                    s1[sp1].node = *(u32 *)(node + 0xC);
                } else {
                    s32 cond = loT < p1;

                    sp1++;
                    s1[sp1].node = *(u32 *)(node + 8);
                    s1[sp1].t0 = s1[sp1 - 1].t0;
                    s1[sp1].t1 = s1[sp1 - 1].t1;
                    if (cond) {
                        a = loT - p1;
                        s1[sp1].t1 = (0.0f + s1[sp1].t1) + iv * a;
                    }
                    s1[sp1 - 1].node = *(u32 *)(node + 0xC);
                    if (!(hiT <= p0)) {
                        a = hiT - p0;
                        s1[sp1 - 1].t0 = (0.0f + s1[sp1 - 1].t0) + iv * a;
                    }
                }
            } else {
                if (loT < p1) {
                    s1[sp1].node = *(u32 *)(node + 0xC);
                } else if (!(hiT <= p0)) {
                    s1[sp1].node = *(u32 *)(node + 8);
                } else {
                    s32 cond = hiT <= p1;

                    sp1++;
                    s1[sp1].node = *(u32 *)(node + 0xC);
                    s1[sp1].t0 = s1[sp1 - 1].t0;
                    s1[sp1].t1 = s1[sp1 - 1].t1;
                    if (!cond) {
                        a = hiT - p1;
                        s1[sp1].t1 = (0.0f + s1[sp1].t1) + iv * a;
                    }
                    s1[sp1 - 1].node = *(u32 *)(node + 8);
                    if (loT < p0) {
                        a = loT - p0;
                        s1[sp1 - 1].t0 = (0.0f + s1[sp1 - 1].t0) + iv * a;
                    }
                }
            }
            node = s1[sp1].node;
        }
        leaf = *(u32 *)(node + *(s32 *)0x007646C0);
        if (leaf == 0) {
            continue;
        }
        mesh = (u8 *)leaf;
        tris = *(u8 **)(node + 4);
        verts = *(u8 **)(node + 8);
        fake[1] = 0xFF;
        *(u16 *)(fake + 2) = 0;
        if (*(u16 *)(mesh + 0x1E) == 0) {
            fake[1] = (u8) * (u16 *)(mesh + 0x1C);
        }
        s2[0].node = (u32)fake;
        s2[0].t0 = s1[sp1].t0;
        s2[0].t1 = s1[sp1].t1;
        sp2 = 1;
        while (sp2 > 0) {
            u32 n2;
            u32 first;
            s32 ti;
            s32 tend;

            sp2--;
            n2 = s2[sp2].node;
            while (*(u8 *)(n2 + 1) == 0xFF) {
                u8 *N = *(u8 **)(mesh + 0x20) + ((u32) * (u16 *)(n2 + 2) << 4);
                u32 axis = *N & 0xC;
                f32 A = *(f32 *)(N + 4);
                f32 B = *(f32 *)(N + 0xC);
                f32 oo = o[axis / 4];
                f32 dd = d[axis / 4];
                f32 tl = tol[axis / 4];
                f32 iv = invd[axis / 4];
                f32 hiT = B - tl;
                f32 p1 = (0.0f + oo) + s2[sp2].t1 * dd;
                f32 loT = A + tl;
                f32 p0 = (0.0f + oo) + s2[sp2].t0 * dd;
                f32 a;

                if (!(dd <= 0.0f)) {
                    if (!(hiT <= p1)) {
                        s2[sp2].node = (u32)N;
                    } else if (loT < p0) {
                        s2[sp2].node = (u32)N + 8;
                    } else {
                        s32 cond = loT < p1;

                        sp2++;
                        s2[sp2].node = (u32)N;
                        s2[sp2].t0 = s2[sp2 - 1].t0;
                        s2[sp2].t1 = s2[sp2 - 1].t1;
                        if (cond) {
                            a = loT - p1;
                            s2[sp2].t1 = (0.0f + s2[sp2].t1) + iv * a;
                        }
                        s2[sp2 - 1].node = (u32)N + 8;
                        if (!(hiT <= p0)) {
                            a = hiT - p0;
                            s2[sp2 - 1].t0 = (0.0f + s2[sp2 - 1].t0) + iv * a;
                        }
                    }
                } else {
                    if (loT < p1) {
                        s2[sp2].node = (u32)N + 8;
                    } else if (!(hiT <= p0)) {
                        s2[sp2].node = (u32)N;
                    } else {
                        s32 cond = hiT <= p1;

                        sp2++;
                        s2[sp2].node = (u32)N + 8;
                        s2[sp2].t0 = s2[sp2 - 1].t0;
                        s2[sp2].t1 = s2[sp2 - 1].t1;
                        if (!cond) {
                            a = hiT - p1;
                            s2[sp2].t1 = (0.0f + s2[sp2].t1) + iv * a;
                        }
                        s2[sp2 - 1].node = (u32)N;
                        if (loT < p0) {
                            a = loT - p0;
                            s2[sp2 - 1].t0 = (0.0f + s2[sp2 - 1].t0) + iv * a;
                        }
                    }
                }
                n2 = s2[sp2].node;
            }
            first = *(u16 *)(n2 + 2);
            tend = (s32)first + *(u8 *)(n2 + 1);
            for (ti = (s32)first; ti < tend; ti = (s32)((ti + 1) & 0xFFFF)) {
                u32 tri;
                u16 *tr;
                f32 *p0;
                f32 *p1;
                f32 *p2;
                f32 e1x, e1y, e1z, e2x, e2y, e2z;
                f32 px, py, pz, det, tolr, hi;
                f32 tx, ty, tz, u, qx, qy, qz, v, tn, a;
                f32 nx, ny, nz, len, inv;
                s32 ok;

                if (*(u32 *)(mesh + 0x24) != 0) {
                    tri = *(u16 *)(*(u32 *)(mesh + 0x24) + (((u32)ti & 0xFFFF) << 1));
                } else {
                    tri = (u32)ti;
                }
                tri = tri & 0xFFFF;
                tr = (u16 *)(tris + tri * 8);
                p0 = (f32 *)(verts + tr[0] * 12);
                p1 = (f32 *)(verts + tr[1] * 12);
                p2 = (f32 *)(verts + tr[2] * 12);
                e1x = p1[0] - p0[0];
                e1z = p1[2] - p0[2];
                e2x = p2[0] - p0[0];
                e1y = p1[1] - p0[1];
                e2y = p2[1] - p0[1];
                e2z = p2[2] - p0[2];
                a = d[1] * e2z;
                px = a - d[2] * e2y;
                a = d[2] * e2x;
                py = a - d[0] * e2z;
                a = d[0] * e2y;
                pz = a - d[1] * e2x;
                det = e1y * py;
                det = det + e1x * px;
                det = det + e1z * pz;
                ok = !(det <= epsDet);
                tn = 0.0f;
                if (ok) {
                    tolr = slack * -det;
                    tx = o[0] - p0[0];
                    ty = o[1] - p0[1];
                    tz = o[2] - p0[2];
                    hi = det - tolr;
                    a = ty * py;
                    a = a + tx * px;
                    u = a + tz * pz;
                    if (u < tolr) {
                        ok = 0;
                    }
                    if (ok && !(u <= hi)) {
                        ok = 0;
                    }
                    if (ok) {
                        a = ty * e1z;
                        qx = a - tz * e1y;
                        a = tz * e1x;
                        qy = a - tx * e1z;
                        a = tx * e1y;
                        qz = a - ty * e1x;
                        a = d[1] * qy;
                        a = a + d[0] * qx;
                        v = a + d[2] * qz;
                        if (v < tolr) {
                            ok = 0;
                        }
                        if (ok) {
                            v = u + v;
                            if (!(v <= hi)) {
                                ok = 0;
                            }
                        }
                        if (ok) {
                            a = e2y * qy;
                            a = a + e2x * qx;
                            tn = a + e2z * qz;
                            if (tn < tolr) {
                                ok = 0;
                            }
                            if (ok && !(tn <= hi)) {
                                ok = 0;
                            }
                            if (ok) {
                                tn = tn / det;
                            }
                        }
                    }
                }
                if (!ok) {
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
                if (arg2(arg1, (u8 *)node, rec, tn, arg3) == 0) {
                    return;
                }
            }
        }
    }
}
