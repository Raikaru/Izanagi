/* func_003941e0 (1668 bytes): func_00392650 on a morph-blended geometry: casts the segment arg2 (origin at +0, end at +0xC) against the triangles of arg0 (count +0x10, triangles +0x2C,
 * morph table +0x5C with 28-byte entries whose word +0x14 is the vertex array) calling arg4(arg3, &rec, arg5, t) for each hit until it returns 0. arg1 selects the two morph targets
 * (signed halfwords +4 and +6) and the blend (the floats at +0xC and +0x10 multiplied); with one array the corners are the array entries, with two they are v0 + (v1 - v0) * blend
 * built in locals. The test and the record {unit normal, first vertex, triangle index, &v0, &v1, &v2} are those of func_00392650 (tolerances 0x322BCC77 on the determinant and
 * 0x3727C5AC * det on the ranges, t = t_num / det). */

typedef s32 (*P4RayHitFn_003941e0)(u32 ctx, void *rec, f32 t, u32 ctx2); /* host argument order: the callbacks (FldFrame_RaycastTriangle & co) take the distance third, before the data word; on the EE f12 vs a2 made the order irrelevant */

void func_003941e0(u8 *arg0, u8 *arg1, f32 *arg2, u32 arg3, P4RayHitFn_003941e0 arg4, u32 arg5)
{
    u8 *arrays[2];
    f32 blend;
    u16 *tri = *(u16 **)(arg0 + 0x2C);
    s32 n = *(s32 *)(arg0 + 0x10);
    u32 k1 = 0x322BCC77;
    u32 k2 = 0x3727C5AC;
    f32 eps = *(f32 *)&k1;
    f32 slack = *(f32 *)&k2;
    f32 dx = arg2[3] - arg2[0];
    f32 dy = arg2[4] - arg2[1];
    f32 dz = arg2[5] - arg2[2];
    f32 la[3];
    f32 lb[3];
    f32 lc[3];
    u32 rec[12];
    s32 i;
    f32 *p0 = la;
    f32 *p1 = lb;
    f32 *p2 = lc;
    f32 *dst[3];
    f32 *v0;
    f32 *v1;
    u32 idx[3];
    u32 k;
    u32 w;
    f32 d;
    f32 q;
    f32 e1x, e1y, e1z, e2x, e2y, e2z;
    f32 px, py, pz, det, tol, hi;
    f32 tx, ty, tz, u, qx, qy, qz, v, tn, a;
    f32 nx, ny, nz, len, inv;
    s32 ok;
    f32 *ray;

    arrays[0] = *(u8 **)(arg0 + 0x5C) + *(s16 *)(arg1 + 4) * 28;
    arrays[0] = *(u8 **)(arrays[0] + 0x14);
    arrays[1] = *(u8 **)(arg0 + 0x5C) + *(s16 *)(arg1 + 6) * 28;
    arrays[1] = *(u8 **)(arrays[1] + 0x14);
    blend = *(f32 *)(arg1 + 0xC) * *(f32 *)(arg1 + 0x10);
    ray = arg2;
    for (i = 0; i < 12; i++) {
        rec[i] = 0;
    }
    for (i = 0; i < n; i++, tri += 4) {
        idx[0] = tri[0];
        idx[1] = tri[1];
        idx[2] = tri[2];
        if (arrays[0] == arrays[1]) {
            p0 = (f32 *)(arrays[0] + idx[0] * 12);
            p1 = (f32 *)(arrays[0] + idx[1] * 12);
            p2 = (f32 *)(arrays[0] + idx[2] * 12);
        } else {
            dst[0] = la;
            dst[1] = lb;
            dst[2] = lc;
            p0 = la;
            p1 = lb;
            p2 = lc;
            for (k = 0; k < 3; k++) {
                v0 = (f32 *)(arrays[0] + idx[k] * 12);
                v1 = (f32 *)(arrays[1] + idx[k] * 12);
                for (w = 0; w < 3; w++) {
                    d = v1[w] - v0[w];
                    q = d * blend;
                    dst[k][w] = q + v0[w];
                }
            }
        }
        e2x = p2[0] - p0[0];
        e1z = p1[2] - p0[2];
        e2y = p2[1] - p0[1];
        e2z = p2[2] - p0[2];
        e1y = p1[1] - p0[1];
        e1x = p1[0] - p0[0];
        a = dy * e2z;
        px = a - dz * e2y;
        a = dz * e2x;
        py = a - dx * e2z;
        a = dx * e2y;
        pz = a - dy * e2x;
        det = e1y * py;
        det = det + e1x * px;
        det = det + e1z * pz;
        ok = !(det <= eps);
        tn = 0.0f;
        if (ok) {
            tol = slack * -det;
            ty = ray[1] - p0[1];
            tx = ray[0] - p0[0];
            tz = ray[2] - p0[2];
            hi = det - tol;
            a = ty * py;
            a = a + tx * px;
            u = a + tz * pz;
            if (u < tol) {
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
                a = dy * qy;
                a = a + dx * qx;
                v = a + dz * qz;
                if (v < tol) {
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
                    if (tn < tol) {
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
        rec[6] = (u32)i;
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
        if (arg4(arg3, rec, tn, arg5) == 0) {
            return;
        }
    }
}
