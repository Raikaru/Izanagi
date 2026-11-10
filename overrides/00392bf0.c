/* func_00392bf0 (3344 bytes): casts the segment arg1 (origin at +0, end at +0xC, floats) against the triangles of the geometry arg0 through the collision mesh at
 * *(arg0 + *(s32 *)0x007646BC) (gp-0x4A34; gp is 0x007690F0) and calls arg3(arg2, &rec, arg4, t) for each hit, stopping when it returns 0; without a mesh func_00392650 runs instead
 * with the same arguments. arg0 supplies the triangles at +0x2C and the vertex array (*(u32 *)(arg0 + 0x5C))[+0x14]; the mesh's second-level tree is the one of func_00390680 (root pseudo
 * node from halfwords +0x1E / +0x1C, 16-byte child pairs at mesh[0x20 / 4], optional index table mesh[0x24 / 4]). The parameter interval [0, 1] is first clipped against the mesh box
 * (min at +0x10, +0x14, +0x18, max at +4, +8, +0xC, widened by the tolerance 0x358637BD * the largest |coordinate| per axis) with the usual slabs (a zero direction component only checks the
 * origin); an empty interval ends the call. Traversal, the Moller-Trumbore test and the hit record {unit normal, first vertex, triangle number, &v0, &v1, &v2} are those of
 * func_00390680 / func_00392650. */

extern void p4c_00392650_00392bf0(u8 *g, u8 *ray, u32 ctx, void *cb, u32 ctx2) __asm__("func_00392650");

typedef s32 (*P4RayHitFn_00392bf0)(u32 ctx, void *rec, f32 t, u32 ctx2); /* host argument order: the callbacks (FldFrame_RaycastTriangle & co) take the distance third, before the data word; on the EE f12 vs a2 made the order irrelevant */

typedef struct {
    u32 node;
    f32 t0;
    f32 t1;
} P4Ent_00392bf0;

static f32 p4_abs_00392bf0(f32 v)
{
    return v < 0.0f ? -v : v;
}

void func_00392bf0(u8 *arg0, f32 *arg1, u32 arg2, P4RayHitFn_00392bf0 arg3, u32 arg4)
{
    u32 mesh = *(u32 *)(arg0 + *(s32 *)0x007646BC);
    u8 *tris;
    u8 *verts;
    f32 o[3];
    f32 d[3];
    f32 invd[3];
    f32 tol[3];
    f32 mx[3];
    f32 lo[3];
    f32 hi[3];
    P4Ent_00392bf0 s2[40];
    s32 sp2;
    u8 fake[8];
    u32 k1 = 0x358637BD;
    u32 k2 = 0x322BCC77;
    u32 k3 = 0x3727C5AC;
    f32 epsTol = *(f32 *)&k1;
    f32 epsDet = *(f32 *)&k2;
    f32 slack = *(f32 *)&k3;
    u32 rec[12];
    f32 *mf;
    u32 i;
    f32 *tt0;
    f32 *tt1;
    s32 inside;

    if (mesh == 0) {
        p4c_00392650_00392bf0(arg0, (u8 *)arg1, arg2, (void *)arg3, arg4);
        return;
    }
    mf = (f32 *)mesh;
    tris = *(u8 **)(arg0 + 0x2C);
    verts = *(u8 **)(*(u32 *)(arg0 + 0x5C) + 0x14);
    for (i = 0; i < 12; i++) {
        rec[i] = 0;
    }
    fake[1] = 0xFF;
    *(u16 *)(fake + 2) = 0;
    if (*(u16 *)(mesh + 0x1E) == 0) {
        fake[1] = (u8) * (u16 *)(mesh + 0x1C);
    }
    o[0] = arg1[0];
    o[1] = arg1[1];
    o[2] = arg1[2];
    d[0] = arg1[3] - arg1[0];
    d[1] = arg1[4] - arg1[1];
    d[2] = arg1[5] - arg1[2];
    for (i = 0; i < 3; i++) {
        invd[i] = 0.0f;
        if (!(d[i] == 0.0f)) {
            invd[i] = 1.0f / d[i];
        }
        mx[i] = (p4_abs_00392bf0(arg1[i]) < p4_abs_00392bf0(arg1[3 + i])) ? p4_abs_00392bf0(arg1[3 + i]) : p4_abs_00392bf0(arg1[i]);
    }
    tol[2] = mx[2] * epsTol;
    tol[0] = mx[0] * epsTol;
    tol[1] = mx[1] * epsTol;
    s2[0].node = (u32)fake;
    s2[0].t0 = 0.0f;
    s2[0].t1 = 1.0f;
    tt0 = &s2[0].t0;
    tt1 = &s2[0].t1;
    lo[0] = mf[0x10 / 4] - tol[0];
    lo[1] = mf[0x14 / 4] - tol[1];
    lo[2] = mf[0x18 / 4] - tol[2];
    hi[0] = mf[1] + tol[0];
    hi[1] = mf[2] + tol[1];
    hi[2] = mf[3] + tol[2];
    inside = 1;
    for (i = 0; i < 3 && inside; i++) {
        if (invd[i] == 0.0f) {
            inside = 0;
            if (!(o[i] < lo[i]) && o[i] <= hi[i]) {
                inside = 1;
            }
        } else {
            f32 ta = invd[i] * (lo[i] - o[i]);
            f32 tb = invd[i] * (hi[i] - o[i]);
            f32 tmin;
            f32 tmax;

            if (tb < ta) {
                tmin = tb;
                tmax = ta;
            } else {
                tmin = ta;
                tmax = tb;
            }
            inside = 0;
            if (*tt0 <= tmax && !(*tt1 < tmin)) {
                inside = 1;
                if (*tt0 < tmin) {
                    *tt0 = tmin;
                }
                if (!(*tt1 <= tmax)) {
                    *tt1 = tmax;
                }
            }
        }
    }
    if (!inside) {
        return;
    }
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
            if (arg3(arg2, rec, tn, arg4) == 0) {
                return;
            }
        }
    }

}
