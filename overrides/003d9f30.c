/* func_003d9f30 (4692 bytes): tests the triangle (arg1, arg2, arg3: three floats x, y, z each) against the axis aligned box arg0 = {max x, max y, max z, min x, min y, min z} (strict inside
 * test on the plane stage) and returns 1 when they overlap, else 0.
 * Every vertex gets an outcode: x 1 above max x, 2 below min x; y 4 / 8; z 0x10 / 0x20. A vertex inside (code 0) returns 1; all three codes sharing a bit returns 0. Then each edge
 * whose end codes share no bit (P0 P1, P0 P2, P1 P2) is tested for every bit of the xor of its codes (1, 2, 4, 8, 0x10, 0x20 in this order): with the offsets from the edge start to the
 * box faces and the edge direction d, the edge crosses the face of a bit when the two cross products p1 = (x offset) * dy .. are between the face limits; a sign of the direction component
 * below 0 uses the strict inequalities the other way round (see func_003d9f30's local helper). Any crossing returns 1. Finally the box is tested against the plane of the triangle with
 * normal N = (P1 - P0) x (P2 - P0): the box corner and diagonal are picked from the signs of N (the box y and z extents are turned when their sign differs from the sign of N.x), s (corner) and nd
 * (diagonal) have to satisfy nd < s < 0 (0 < s < nd when nd >= 0), and the two edge cross terms u and v of the box corner against the triangle edges must too (v negated; u + v as well). */

typedef union {
    f32 f;
    u32 u;
} P4F9f30;

/* the face test of one bit: w is the direction component along the face axis, (p1, a1, b1) and (p2, a2, b2) the two cross terms against the limits of the two other axes */
static s32 hit_003d9f30(f32 w, f32 p1, f32 a1, f32 b1, f32 p2, f32 a2, f32 b2)
{
    if (w < 0.0f) {
        if (!(p1 < a1)) {
            return 0;
        }
        if (p1 <= b1) {
            return 0;
        }
        if (!(p2 < a2)) {
            return 0;
        }
        if (p2 <= b2) {
            return 0;
        }
        return 1;
    }
    if (p1 <= a1) {
        return 0;
    }
    if (!(p1 < b1)) {
        return 0;
    }
    if (p2 <= a2) {
        return 0;
    }
    if (!(p2 < b2)) {
        return 0;
    }
    return 1;
}

static s32 edge_003d9f30(f32 *b, f32 x0, f32 y0, f32 z0, f32 dx, f32 dy, f32 dz, u32 bits)
{
    f32 xm = x0 - b[0];
    f32 ym = y0 - b[1];
    f32 zm = z0 - b[2];
    f32 xn = x0 - b[3];
    f32 yn = y0 - b[4];
    f32 zn = z0 - b[5];

    if ((bits & 1) && hit_003d9f30(dx, xm * dy, ym * dx, yn * dx, xm * dz, zm * dx, zn * dx)) {
        return 1;
    }
    if ((bits & 2) && hit_003d9f30(dx, xn * dy, ym * dx, yn * dx, xn * dz, zm * dx, zn * dx)) {
        return 1;
    }
    if ((bits & 4) && hit_003d9f30(dy, ym * dz, zm * dy, zn * dy, ym * dx, xm * dy, xn * dy)) {
        return 1;
    }
    if ((bits & 8) && hit_003d9f30(dy, yn * dz, zm * dy, zn * dy, yn * dx, xm * dy, xn * dy)) {
        return 1;
    }
    if ((bits & 0x10) && hit_003d9f30(dz, zm * dx, xm * dz, xn * dz, zm * dy, ym * dz, yn * dz)) {
        return 1;
    }
    if ((bits & 0x20) && hit_003d9f30(dz, zn * dx, xm * dz, xn * dz, zn * dy, ym * dz, yn * dz)) {
        return 1;
    }
    return 0;
}

static u32 code_003d9f30(f32 *b, f32 *p)
{
    u32 c;

    if (p[1] <= b[1]) {
        c = (p[1] < b[4]) ? 8 : 0;
    } else {
        c = 4;
    }
    if (p[0] <= b[0]) {
        c |= (p[0] < b[3]) ? 2 : 0;
    } else {
        c |= 1;
    }
    if (p[2] <= b[2]) {
        c |= (p[2] < b[5]) ? 0x20 : 0;
    } else {
        c |= 0x10;
    }
    return c;
}

s32 func_003d9f30(f32 *arg0, f32 *arg1, f32 *arg2, f32 *arg3)
{
    u32 o0 = code_003d9f30(arg0, arg1);
    u32 o1;
    u32 o2;
    f32 dx01;
    f32 dy01;
    f32 dz01;
    f32 dx02;
    f32 dy02;
    f32 dz02;
    P4F9f30 nx;
    P4F9f30 ny;
    P4F9f30 nz;
    f32 ex;
    f32 ey;
    f32 ez;
    f32 cx;
    f32 cy;
    f32 cz;
    f32 nd;
    f32 s;
    f32 acc;
    f32 xc;
    f32 yc;
    f32 zc;
    f32 u;
    f32 v;
    f32 t;

    if (o0 == 0) {
        return 1;
    }
    o1 = code_003d9f30(arg0, arg2);
    if (o1 == 0) {
        return 1;
    }
    o2 = code_003d9f30(arg0, arg3);
    if (o2 == 0) {
        return 1;
    }
    if ((o0 & o1 & o2) != 0) {
        return 0;
    }
    if ((o0 & o1) == 0) {
        if (edge_003d9f30(arg0, arg1[0], arg1[1], arg1[2], arg2[0] - arg1[0], arg2[1] - arg1[1], arg2[2] - arg1[2], o0 ^ o1)) {
            return 1;
        }
    }
    if ((o0 & o2) == 0) {
        if (edge_003d9f30(arg0, arg1[0], arg1[1], arg1[2], arg3[0] - arg1[0], arg3[1] - arg1[1], arg3[2] - arg1[2], o0 ^ o2)) {
            return 1;
        }
    }
    if ((o1 & o2) == 0) {
        if (edge_003d9f30(arg0, arg2[0], arg2[1], arg2[2], arg3[0] - arg2[0], arg3[1] - arg2[1], arg3[2] - arg2[2], o1 ^ o2)) {
            return 1;
        }
    }
    dy01 = arg2[1] - arg1[1];
    dz02 = arg3[2] - arg1[2];
    dx01 = arg2[0] - arg1[0];
    dy02 = arg3[1] - arg1[1];
    dz01 = arg2[2] - arg1[2];
    dx02 = arg3[0] - arg1[0];
    nx.f = dy01 * dz02 - dz01 * dy02;
    ny.f = dz01 * dx02 - dx01 * dz02;
    nz.f = dx01 * dy02 - dy01 * dx02;
    ex = arg0[0] - arg0[3];
    cx = arg0[3] - arg1[0];
    if ((s32)(nx.u ^ ny.u) < 0) {
        ey = arg0[4] - arg0[1];
        cy = arg0[1] - arg1[1];
    } else {
        ey = arg0[1] - arg0[4];
        cy = arg0[4] - arg1[1];
    }
    if ((s32)(nx.u ^ nz.u) < 0) {
        cz = arg0[2] - arg1[2];
        ez = arg0[5] - arg0[2];
    } else {
        cz = arg0[5] - arg1[2];
        ez = arg0[2] - arg0[5];
    }
    nd = -(ey * ny.f + ex * nx.f + ez * nz.f);
    s = cy * ny.f + cx * nx.f + cz * nz.f;
    acc = 0.0f + cy * ez;
    xc = acc - cz * ey;
    acc = 0.0f + cz * ex;
    yc = acc - cx * ez;
    acc = 0.0f + cx * ey;
    zc = acc - cy * ex;
    acc = 0.0f + dy02 * yc;
    t = acc + dx02 * xc;
    acc = 0.0f + t;
    u = acc + dz02 * zc;
    if (nd < 0.0f) {
        if (0.0f <= s) {
            return 0;
        }
        if (s <= nd) {
            return 0;
        }
        if (0.0f <= u) {
            return 0;
        }
        if (u <= nd) {
            return 0;
        }
        acc = 0.0f + dy01 * yc;
        t = acc + dx01 * xc;
        acc = 0.0f + t;
        v = -(acc + dz01 * zc);
        if (0.0f <= v) {
            return 0;
        }
        if (u + v <= nd) {
            return 0;
        }
        return 1;
    }
    if (!(0.0f < s)) {
        return 0;
    }
    if (!(s < nd)) {
        return 0;
    }
    if (!(0.0f < u)) {
        return 0;
    }
    if (!(u < nd)) {
        return 0;
    }
    acc = 0.0f + dy01 * yc;
    t = acc + dx01 * xc;
    acc = 0.0f + t;
    v = -(acc + dz01 * zc);
    if (!(0.0f < v)) {
        return 0;
    }
    if (!(u + v < nd)) {
        return 0;
    }
    return 1;
}
