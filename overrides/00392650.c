/* func_00392650 (1024 bytes): casts the segment arg1 (origin at +0, end at +0xC, floats) against every triangle of the geometry arg0 and calls arg3(arg2, &rec, arg4, t) for
 * each hit, stopping when it returns 0. arg0 has the triangle count at +0x10, the triangles at +0x2C (8 bytes: three halfword vertex indices) and the vertex array at
 * (*(u32 *)(arg0 + 0x5C))[+0x14] (12 bytes per vertex, the first morph target). Moller-Trumbore on d = end - origin with the tolerance 0x322BCC77 on the determinant (it must be
 * above it) and a relative slack of 0x3727C5AC * det on the barycentric ranges (u >= -slack, v >= -slack, u + v <= det + slack, t_num in the same range); t = t_num / det. The hit
 * record rec = {unit normal e1 x e2 (3 floats), first vertex (3 floats), triangle index, &v0, &v1, &v2}. All single precision in the order of the FPU's accumulator. */

typedef s32 (*P4RayHitFn_00392650)(u32 ctx, void *rec, f32 t, u32 ctx2); /* host argument order: the callbacks (FldFrame_RaycastTriangle & co) take the distance third, before the data word; on the EE f12 vs a2 made the order irrelevant */

void func_00392650(u8 *arg0, f32 *arg1, u32 arg2, P4RayHitFn_00392650 arg3, u32 arg4)
{
    u8 *va = *(u8 **)(*(u32 *)(arg0 + 0x5C) + 0x14);
    u16 *tri = *(u16 **)(arg0 + 0x2C);
    s32 n = *(s32 *)(arg0 + 0x10);
    u32 k1 = 0x322BCC77;
    u32 k2 = 0x3727C5AC;
    f32 eps = *(f32 *)&k1;
    f32 slack = *(f32 *)&k2;
    f32 dx = arg1[3] - arg1[0];
    f32 dy = arg1[4] - arg1[1];
    f32 dz = arg1[5] - arg1[2];
    u32 rec[12];
    s32 i;
    f32 *p0;
    f32 *p1;
    f32 *p2;
    f32 e1x, e1y, e1z, e2x, e2y, e2z;
    f32 px, py, pz, det, tol, hi;
    f32 tx, ty, tz, u, qx, qy, qz, v, tn, a;
    f32 nx, ny, nz, len, inv;
    s32 ok;

    for (i = 0; i < 12; i++) {
        rec[i] = 0;
    }
    for (i = 0; i < n; i++, tri += 4) {
        p0 = (f32 *)(va + tri[0] * 12);
        p1 = (f32 *)(va + tri[1] * 12);
        p2 = (f32 *)(va + tri[2] * 12);
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
            ty = arg1[1] - p0[1];
            tx = arg1[0] - p0[0];
            tz = arg1[2] - p0[2];
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
        if (arg3(arg2, rec, tn, arg4) == 0) {
            return;
        }
    }
}
