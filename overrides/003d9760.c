/* func_003d9760 (2000 bytes): sphere / triangle overlap test; returns 1 when the sphere arg0 (centre x, y, z and radius at +0xC) touches the triangle arg1, arg2, arg3
 * (three float triples) and 0 otherwise. The unnormalised-then-normalised triangle normal is stored to arg4 (three floats; written before the early outs after it) and the signed
 * plane distance, negated, to *arg5 once the sphere reaches the plane. Steps, all single precision in the order of the FPU's accumulator ((0 + a) + b ... as adda / madd):
 * per axis the triangle's corners relative to the centre are rejected when they all lie beyond the radius on the same side (sign bits tested, as retail); the normal n = e1 x e2
 * (e1 = arg2 - arg1, e2 = arg3 - arg1; a zero length gives 0) is normalised and |n . p0| must not exceed the radius; a corner inside the sphere gives 1; the corner nearest the centre
 * is taken and its dot products with the other two corners decide whether the edge tests run (otherwise 0); for each edge the closest point of the infinite edge line to the
 * centre must be inside the sphere, or the opposite corner must not lie beyond it (q . p > q . q gives 0); the last edge ends with 1. */

static u32 p4_sgn_003d9760(f32 v)
{
    return *(u32 *)&v >> 31;
}

static s32 p4_axis_003d9760(f32 a, f32 b, f32 c, f32 r)
{
    u32 k = p4_sgn_003d9760(a) + p4_sgn_003d9760(b) + p4_sgn_003d9760(c);

    if (k == 0) {
        if ((p4_sgn_003d9760(a - r) | p4_sgn_003d9760(b - r) | p4_sgn_003d9760(c - r)) == 0) {
            return 1;
        }
    } else if (k == 3) {
        if ((p4_sgn_003d9760(a + r) & p4_sgn_003d9760(b + r) & p4_sgn_003d9760(c + r)) != 0) {
            return 1;
        }
    }
    return 0;
}

static f32 p4_dot_003d9760(f32 ay, f32 ax, f32 az, f32 by, f32 bx, f32 bz)
{
    f32 m = ay * by;

    m = 0.0f + m;
    m = m + ax * bx;
    m = 0.0f + m;
    return m + az * bz;
}

/* One edge from corner A (ax, ay, az) by (ex, ey, ez): the squared distance q2 of the centre to the edge line and q . C for the opposite corner. */
static f32 p4_edge_003d9760(f32 ax, f32 ay, f32 az, f32 ex, f32 ey, f32 ez, f32 cx, f32 cy, f32 cz, f32 *dot)
{
    f32 num;
    f32 den;
    f32 t;
    f32 qx;
    f32 qy;
    f32 qz;
    f32 q2;
    f32 s;

    num = ey * ay;
    num = num + ex * ax;
    num = num + ez * az;
    den = ey * ey;
    den = den + ex * ex;
    den = den + ez * ez;
    t = num / den;
    qx = (0.0f + ax) - ex * t;
    qy = (0.0f + ay) - ey * t;
    qz = (0.0f + az) - ez * t;
    q2 = qy * qy;
    q2 = q2 + qx * qx;
    q2 = q2 + qz * qz;
    s = qy * cy;
    s = 0.0f + s;
    s = s + qx * cx;
    s = 0.0f + s;
    s = s + qz * cz;
    *dot = s;
    return q2;
}

s32 func_003d9760(f32 *arg0, f32 *arg1, f32 *arg2, f32 *arg3, f32 *arg4, f32 *arg5)
{
    f32 r = arg0[3];
    f32 x0 = arg1[0] - arg0[0];
    f32 y0 = arg1[1] - arg0[1];
    f32 z0 = arg1[2] - arg0[2];
    f32 x1 = arg2[0] - arg0[0];
    f32 y1 = arg2[1] - arg0[1];
    f32 z1 = arg2[2] - arg0[2];
    f32 x2 = arg3[0] - arg0[0];
    f32 y2 = arg3[1] - arg0[1];
    f32 z2 = arg3[2] - arg0[2];
    f32 e1x;
    f32 e1y;
    f32 e1z;
    f32 e2x;
    f32 e2y;
    f32 e2z;
    f32 len2;
    f32 len;
    f32 inv;
    f32 dist;
    f32 r2;
    f32 d0;
    f32 d1;
    f32 d2;
    f32 a;
    f32 q2;
    f32 s;
    f32 lim;

    if (p4_axis_003d9760(x0, x1, x2, r)) {
        return 0;
    }
    if (p4_axis_003d9760(y0, y1, y2, r)) {
        return 0;
    }
    if (p4_axis_003d9760(z0, z1, z2, r)) {
        return 0;
    }
    e1x = arg2[0] - arg1[0];
    e1z = arg2[2] - arg1[2];
    e2x = arg3[0] - arg1[0];
    e1y = arg2[1] - arg1[1];
    e2y = arg3[1] - arg1[1];
    e2z = arg3[2] - arg1[2];
    a = e1y * e2z;
    arg4[0] = a - e1z * e2y;
    a = e1z * e2x;
    arg4[1] = a - e1x * e2z;
    a = e1x * e2y;
    arg4[2] = a - e1y * e2x;
    len2 = arg4[1] * arg4[1];
    len2 = arg4[0] * arg4[0] + len2;
    len2 = len2 + arg4[2] * arg4[2];
    if (len2 <= 0.0f) {
        return 0;
    }
    len = __builtin_sqrtf(len2);
    inv = len;
    if (!(len <= 0.0f)) {
        inv = 1.0f / len;
    }
    arg4[0] = arg4[0] * inv;
    arg4[1] = arg4[1] * inv;
    arg4[2] = arg4[2] * inv;
    dist = y0 * arg4[1];
    dist = dist + x0 * arg4[0];
    dist = dist + z0 * arg4[2];
    lim = -r;
    if (dist < lim) {
        return 0;
    }
    if (!(dist <= r)) {
        return 0;
    }
    *arg5 = -dist;
    r2 = r * r;
    a = y0 * y0;
    a = 0.0f + a;
    a = a + x0 * x0;
    a = 0.0f + a;
    d0 = a + z0 * z0;
    if (d0 <= r2) {
        return 1;
    }
    a = y1 * y1;
    a = 0.0f + a;
    a = a + x1 * x1;
    a = 0.0f + a;
    d1 = a + z1 * z1;
    if (d1 <= r2) {
        return 1;
    }
    a = y2 * y2;
    a = 0.0f + a;
    a = a + x2 * x2;
    a = 0.0f + a;
    d2 = a + z2 * z2;
    if (d2 <= r2) {
        return 1;
    }
    if (d0 < d1) {
        if (d2 < d0) {
            goto nearest2;
        }
        if (p4_dot_003d9760(y0, x0, z0, y1, x1, z1) <= d0) {
            goto edges;
        }
        if (p4_dot_003d9760(y0, x0, z0, y2, x2, z2) <= d0) {
            goto edges;
        }
        return 0;
    }
    if (d2 < d1) {
        goto nearest2;
    }
    if (p4_dot_003d9760(y1, x1, z1, y0, x0, z0) <= d1) {
        goto edges;
    }
    if (p4_dot_003d9760(y1, x1, z1, y2, x2, z2) <= d1) {
        goto edges;
    }
    return 0;
nearest2:
    if (p4_dot_003d9760(y2, x2, z2, y0, x0, z0) <= d2) {
        goto edges;
    }
    if (p4_dot_003d9760(y2, x2, z2, y1, x1, z1) <= d2) {
        goto edges;
    }
    return 0;
edges:
    q2 = p4_edge_003d9760(x0, y0, z0, x1 - x0, y1 - y0, z1 - z0, x2, y2, z2, &s);
    if (!(q2 <= r2) && q2 < s) {
        return 0;
    }
    q2 = p4_edge_003d9760(x1, y1, z1, x2 - x1, y2 - y1, z2 - z1, x0, y0, z0, &s);
    if (!(q2 <= r2) && q2 < s) {
        return 0;
    }
    q2 = p4_edge_003d9760(x2, y2, z2, x0 - x2, y0 - y2, z0 - z2, x1, y1, z1, &s);
    if (!(q2 <= r2) && q2 < s) {
        return 0;
    }
    return 1;
}
