/* SPDX-License-Identifier: MIT */
/* Izanagi RenderWare clean layer: quaternions (RtQuat), original code (docs/rw_clean.md).
 *
 * Recorded behaviour of the retail functions (the interpreter, generated inputs, byte-for-byte comparison):
 * - RtQuatConvertFromMatrix: with trace = (right.x + up.y) + at.z > 0, s = sqrt(trace + 1), real = s * 0.5 and each
 *   imaginary part a difference of two opposite elements times 0.5 / s; otherwise the largest diagonal element picks one
 *   of three helpers (a later axis wins a tie), each of them a retail function of its own. It returns TRUE, or FALSE
 *   without a word written when either pointer is NULL.
 * - RtQuatTransformVectors applies v' = (w^2 - u.u) v + 2 (u.v) u + 2w (u x v) to each vector (unit quaternion or not).
 * - RtQuatRotate builds the rotation of `angle` degrees about the normalised axis (half angle = angle * pi/360, one
 *   multiply) and replaces, preconcatenates (q * r) or postconcatenates (r * q) it; a combined quaternion's imaginary
 *   parts are never -0. A NULL quaternion or axis reports error 0x80000016, then any other combine value error
 *   0x80000003 (both for the toolkit, plugin 0x195; retail's message for the second, which only a debug build would
 *   format, is "Invalid combination type"); either returns NULL before any work. */
#include "rwc.h"

typedef struct RtQuat {
    RwV3d imag;
    RwReal real;
} RtQuat;

_Static_assert(sizeof(RtQuat) == 16, "RtQuat is 16 bytes");

RwReal RwV3dNormalize(RwV3d *out, const RwV3d *in);
float p4pc_ee_sqrt(float a);

/* The quaternion toolkit's plugin id and the core codes it reports: an invalid combine value and a NULL argument. */
#define RWC_QUAT_PLUGIN 0x195
#define RWC_ERR_INVALID_COMBINE 0x80000003
#define RWC_ERR_NULL_POINTER 0x80000016

static void rwcQuatError(RwInt32 code)
{
    RwError e;

    e.pluginID = RWC_QUAT_PLUGIN;
    e.errorCode = _rwerror(code);
    RwErrorSet(&e);
}

/* Element (row, column) of the rotation part. */
static RwReal rwcElement(const RwMatrix *m, int row, int col)
{
    const RwV3d *r = row == 0 ? &m->right : row == 1 ? &m->up : &m->at;

    return col == 0 ? r->x : col == 1 ? r->y : r->z;
}

/* The quaternion of a matrix whose `i` diagonal element dominates (j and k are the next two axes, cyclically). */
static RtQuat *rwcQuatFromDominant(RtQuat *q, const RwMatrix *m, int i)
{
    int j = (i + 1) % 3, k = (j + 1) % 3;
    RwReal v[3];
    RwReal s = p4pc_ee_sqrt((rwcElement(m, i, i) - (rwcElement(m, j, j) + rwcElement(m, k, k))) + 1.0f);
    RwReal r = 0.5f / s;

    v[i] = s * 0.5f;
    q->real = (rwcElement(m, j, k) - rwcElement(m, k, j)) * r;
    v[j] = (rwcElement(m, i, j) + rwcElement(m, j, i)) * r;
    v[k] = (rwcElement(m, i, k) + rwcElement(m, k, i)) * r;
    q->imag.x = v[0];
    q->imag.y = v[1];
    q->imag.z = v[2];
    return q;
}

// FUN_003DC490
RtQuat *rwcQuatFromMatrixX(RtQuat *q, const RwMatrix *m)
{
    return rwcQuatFromDominant(q, m, 0);
}

// FUN_003DC510
RtQuat *rwcQuatFromMatrixY(RtQuat *q, const RwMatrix *m)
{
    return rwcQuatFromDominant(q, m, 1);
}

// FUN_003DC590
RtQuat *rwcQuatFromMatrixZ(RtQuat *q, const RwMatrix *m)
{
    return rwcQuatFromDominant(q, m, 2);
}

// FUN_003DC610
RwBool RtQuatConvertFromMatrix(RtQuat *q, const RwMatrix *m)
{
    RwReal trace;

    if (!q || !m)
        return RWC_FALSE;
    trace = (m->right.x + m->up.y) + m->at.z;
    if (trace > 0.0f) {
        RwReal s = p4pc_ee_sqrt(trace + 1.0f);
        RwReal r = 0.5f / s;

        q->real = s * 0.5f;
        q->imag.x = (m->up.z - m->at.y) * r;
        q->imag.y = (m->at.x - m->right.z) * r;
        q->imag.z = (m->right.y - m->up.x) * r;
    } else if (m->at.z >= (m->up.y >= m->right.x ? m->up.y : m->right.x)) {
        rwcQuatFromMatrixZ(q, m);
    } else if (m->up.y >= m->right.x) {
        rwcQuatFromMatrixY(q, m);
    } else {
        rwcQuatFromMatrixX(q, m);
    }
    return RWC_TRUE;
}

/* out = a * b. Each imaginary part is (a.real * b + cross) + b.real * a, then plus zero (the sum is never -0). */
static void rwcQuatMultiply(RtQuat *out, const RtQuat *a, const RtQuat *b)
{
    RwReal ax = a->imag.x, ay = a->imag.y, az = a->imag.z, aw = a->real;
    RwReal bx = b->imag.x, by = b->imag.y, bz = b->imag.z, bw = b->real;
    RwReal x = ((aw * bx + (ay * bz - az * by)) + bw * ax) + 0.0f;
    RwReal y = ((aw * by + (az * bx - ax * bz)) + bw * ay) + 0.0f;
    RwReal z = ((aw * bz + (ax * by - ay * bx)) + bw * az) + 0.0f;

    out->real = aw * bw - ((ax * bx + ay * by) + az * bz);
    out->imag.x = x;
    out->imag.y = y;
    out->imag.z = z;
}

// FUN_003DC740
RtQuat *RtQuatRotate(RtQuat *q, const RwV3d *axis, RwReal angle, RwInt32 combine)
{
    RwReal half, s, c;
    RtQuat r, old;

    if (!q || !axis) {
        rwcQuatError(RWC_ERR_NULL_POINTER);
        return 0;
    }
    if (combine != RWC_COMBINE_REPLACE && combine != RWC_COMBINE_PRECONCAT && combine != RWC_COMBINE_POSTCONCAT) {
        rwcQuatError(RWC_ERR_INVALID_COMBINE);
        return 0;
    }
    half = angle * 0.00872664619f;   /* pi / 360 */
    s = sinf(half);
    c = cosf(half);
    RwV3dNormalize(&r.imag, axis);
    r.imag.x = r.imag.x * s;
    r.imag.y = r.imag.y * s;
    r.imag.z = r.imag.z * s;
    r.real = c;
    if (combine == RWC_COMBINE_REPLACE) {
        *q = r;
    } else {
        old = *q;
        if (combine == RWC_COMBINE_PRECONCAT)
            rwcQuatMultiply(q, &old, &r);
        else
            rwcQuatMultiply(q, &r, &old);
    }
    return q;
}

// FUN_003DCB40
RwV3d *RtQuatTransformVectors(RwV3d *out, const RwV3d *in, RwInt32 count, const RtQuat *q)
{
    RwReal x = q->imag.x, y = q->imag.y, z = q->imag.z, w = q->real;
    RwReal scale = w * w - ((x * x + y * y) + z * z);
    RwReal w2 = w + w;
    RwReal w2x = w2 * x, w2y = w2 * y, w2z = w2 * z;
    RwInt32 i;

    for (i = 0; i < count; i++) {
        RwReal vx = in[i].x, vy = in[i].y, vz = in[i].z;
        RwReal d = (x * vx + y * vy) + z * vz;
        RwReal d2 = d + d;

        out[i].x = (scale * vx + (d2 * x + (w2y * vz - w2z * vy))) + 0.0f;
        out[i].y = (scale * vy + (d2 * y + (w2z * vx - w2x * vz))) + 0.0f;
        out[i].z = (scale * vz + (d2 * z + (w2x * vy - w2y * vx))) + 0.0f;
    }
    return out;
}
