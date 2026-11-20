/* SPDX-License-Identifier: MIT */
/* Izanagi RenderWare clean layer: matrices (docs/rw_clean.md, phase 1).
 *
 * Retail facts this file reproduces (recorded by running the retail functions in the R5900 interpreter):
 * - The matrix module's globals: the matrix free list (its descriptor lives at retail 0x008871C0: 64-byte entries, 256
 *   per block, 16-byte aligned, one block preallocated, allocation hint 0x4000d), an optimisation word the open sets to
 *   0x20000 (RwMatrixInvert's identity shortcut needs that bit in it as well as in the matrix's flags), the
 *   matrix-multiply provider and the three tolerances RwMatrixOptimize uses (normal, orthogonal, identity; 0.01 each).
 * - A new matrix is the identity: only the three diagonal ones and the flags word are written.
 * - RwMatrixMultiply and the rotations compute in VU0 arithmetic and clear the padding words; the others use the EE FPU
 *   and leave the padding as it was. */
#include "rwc.h"
#include "p4pc/eefloat.h"

typedef void (*RwcMatrixMultFn)(RwMatrix *dst, const RwMatrix *a, const RwMatrix *b);

typedef struct RwMatrixTolerance {
    RwReal normal;
    RwReal orthogonal;
    RwReal identity;
} RwMatrixTolerance;

typedef struct RwcMatrixGlobals {
    void *freeList;
    RwUInt32 optimizations;
    RwcMatrixMultFn multiply;
    RwMatrixTolerance tolerance;
} RwcMatrixGlobals;

/* The matrix module's bookkeeping (retail gp-relative 0x00764858) and its free list's descriptor (0x008871C0). */
extern RwcModuleInfo D_00764858;
extern RwUInt8 D_008871C0[];
#define RWC_MATRIX RWC_MODULE_GLOBALS(RwcMatrixGlobals, D_00764858)

#define RWC_MATRIX_OPTIMIZE_IDENTITY 0x00020000u
#define RWC_MATRIX_HINT 0x0003000du
#define RWC_MATRIX_FREELIST_HINT 0x0004000du

void *RwFreeListCreateAndPreallocateSpace(RwInt32 entrySize, RwInt32 entriesPerBlock, RwInt32 alignment, RwInt32 numBlocksToPreallocate,
                                          void *inPlaceSpaceForFreeListStruct, RwUInt32 hint);
RwBool RwFreeListDestroy(void *freeList);

/* ---------------------------------------------------------------- products */

/* The default multiply provider: dst = a * b in EE FPU arithmetic, each row of a transformed by b. */
// FUN_003DF8C0
void MatrixMultiply(RwMatrix *dst, const RwMatrix *a, const RwMatrix *b)
{
    RwMatrix r;
    const RwV3d *row[4];
    RwV3d *out[4];
    int i;

    row[0] = &a->right; row[1] = &a->up; row[2] = &a->at; row[3] = &a->pos;
    out[0] = &r.right; out[1] = &r.up; out[2] = &r.at; out[3] = &r.pos;
    for (i = 0; i < 4; i++) {
        RwReal x = row[i]->x, y = row[i]->y, z = row[i]->z;

        out[i]->x = x * b->right.x + y * b->up.x + z * b->at.x;
        out[i]->y = x * b->right.y + y * b->up.y + z * b->at.y;
        out[i]->z = x * b->right.z + y * b->up.z + z * b->at.z;
    }
    r.pos.x = r.pos.x + b->pos.x;
    r.pos.y = r.pos.y + b->pos.y;
    r.pos.z = r.pos.z + b->pos.z;
    dst->right = r.right;
    dst->up = r.up;
    dst->at = r.at;
    dst->pos = r.pos;
}

void rwcVu0MatrixProduct(RwMatrix *dst, const RwMatrix *a, const RwMatrix *b);

/* dst = a * b (VU0 arithmetic and register effects, rwc_vu0.c); the result's type is what both factors share. */
// FUN_003E05F0
RwMatrix *RwMatrixMultiply(RwMatrix *dst, const RwMatrix *a, const RwMatrix *b)
{
    RwUInt32 flags = a->flags & b->flags;

    rwcVu0MatrixProduct(dst, a, b);
    dst->flags = flags;
    return dst;
}

// FUN_003E00F0
RwBool _rwMatrixSetMultFn(void (*multiply)(RwMatrix *dst, const RwMatrix *a, const RwMatrix *b))
{
    RWC_MATRIX->multiply = multiply ? multiply : MatrixMultiply;
    return RWC_TRUE;
}

/* ---------------------------------------------------------------- measures */

static RwReal rwcDot(const RwV3d *a, const RwV3d *b)
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

// FUN_003E0130
RwReal _rwMatrixDeterminant(const RwMatrix *m)
{
    return m->right.x * (m->up.y * m->at.z - m->up.z * m->at.y) + m->right.y * (m->up.z * m->at.x - m->up.x * m->at.z)
           + m->right.z * (m->up.x * m->at.y - m->up.y * m->at.x);
}

/* The sum of the squared dot products of the three axis pairs, the last two added first. */
// FUN_003E0180
RwReal _rwMatrixOrthogonalError(const RwMatrix *m)
{
    RwReal ru = rwcDot(&m->right, &m->up);
    RwReal ua = rwcDot(&m->up, &m->at);
    RwReal ar = rwcDot(&m->at, &m->right);

    return ru * ru + (ua * ua + ar * ar);
}

/* The sum over the three axes of (squared length - 1) squared. */
// FUN_003E01E0
RwReal _rwMatrixNormalError(const RwMatrix *m)
{
    RwReal r = rwcDot(&m->right, &m->right) - 1.0f;
    RwReal u = rwcDot(&m->up, &m->up) - 1.0f;
    RwReal a = rwcDot(&m->at, &m->at) - 1.0f;

    return r * r + u * u + a * a;
}

/* The sum of the squared differences of the twelve elements from the identity. */
RwReal _rwMatrixIdentityError(const RwMatrix *m)
{
    RwReal e[12];
    RwReal sum = 0.0f;
    int i;

    e[0] = m->right.x - 1.0f; e[1] = m->right.y; e[2] = m->right.z;
    e[3] = m->up.x; e[4] = m->up.y - 1.0f; e[5] = m->up.z;
    e[6] = m->at.x; e[7] = m->at.y; e[8] = m->at.z - 1.0f;
    e[9] = m->pos.x; e[10] = m->pos.y; e[11] = m->pos.z;
    for (i = 0; i < 12; i++)
        sum = sum + e[i] * e[i];
    return sum;
}

// FUN_003E0380
RwBool RwEngineGetMatrixTolerances(RwMatrixTolerance *tolerance)
{
    *tolerance = RWC_MATRIX->tolerance;
    return RWC_TRUE;
}

// FUN_003E03B0
RwBool RwEngineSetMatrixTolerances(const RwMatrixTolerance *tolerance)
{
    RWC_MATRIX->tolerance = *tolerance;
    return RWC_TRUE;
}

/* Classifies the matrix within the tolerances (the engine's when NULL; an error equal to its tolerance passes): normal,
 * orthogonal, and, when both, the identity. */
// FUN_003E03E0
RwMatrix *RwMatrixOptimize(RwMatrix *m, const RwMatrixTolerance *tolerance)
{
    RwUInt32 flags = m->flags & ~(RWC_MATRIX_TYPEMASK | RWC_MATRIX_IDENTITY);

    if (!tolerance)
        tolerance = &RWC_MATRIX->tolerance;
    if (_rwMatrixNormalError(m) <= tolerance->normal)
        flags |= RWC_MATRIX_NORMAL;
    if (_rwMatrixOrthogonalError(m) <= tolerance->orthogonal)
        flags |= RWC_MATRIX_ORTHOGONAL;
    if ((flags & RWC_MATRIX_ORTHONORMAL) == RWC_MATRIX_ORTHONORMAL && _rwMatrixIdentityError(m) <= tolerance->identity)
        flags |= RWC_MATRIX_IDENTITY;
    m->flags = flags;
    return m;
}

// FUN_003E05D0
RwMatrix *RwMatrixUpdate(RwMatrix *m)
{
    m->flags &= ~(RWC_MATRIX_TYPEMASK | RWC_MATRIX_IDENTITY);
    return m;
}

/* ---------------------------------------------------------------- normalising and inverting */

static void rwcCross(RwV3d *out, const RwV3d *a, const RwV3d *b)
{
    RwV3d r;

    r.x = a->y * b->z - a->z * b->y;
    r.y = a->z * b->x - a->x * b->z;
    r.z = a->x * b->y - a->y * b->x;
    *out = r;
}

RwReal rwcV3dNormalizeReciprocal(RwV3d *out, const RwV3d *in);

/* dst's axes become orthonormal. Of the three cyclic pairs of unit axes (right-up, up-at, at-right) the most nearly
 * perpendicular one (the smallest |dot|; a tie with the first pair keeps the first, a tie of the other two takes at-right)
 * keeps its first axis P; an axis of zero length instead makes P the axis after it. The axis after P's partner becomes
 * the unit cross product of P and its partner, and the partner the unit cross product of that axis and P. The position
 * is copied; dst keeps its other flag bits and becomes orthonormal (not the identity). */
// FUN_003DF9F0
RwMatrix *RwMatrixOrthoNormalize(RwMatrix *dst, const RwMatrix *src)
{
    const RwV3d *in[3];
    RwV3d n[3], out[3];
    RwReal d[3];
    int i, p, q, s;

    in[0] = &src->right; in[1] = &src->up; in[2] = &src->at;
    for (i = 0; i < 3; i++)
        rwcV3dNormalizeReciprocal(&n[i], in[i]);
    for (i = 0; i < 3; i++) {
        RwReal dot = rwcDot(&n[i], &n[(i + 1) % 3]);

        d[i] = dot < 0.0f ? -dot : dot;
    }
    p = d[0] <= d[1] && d[0] <= d[2] ? 0 : (d[1] < d[2] ? 1 : 2);
    for (i = 2; i >= 0; i--)
        if (in[i]->y * in[i]->y + in[i]->x * in[i]->x + in[i]->z * in[i]->z == 0.0f)
            p = (i + 1) % 3;
    q = (p + 1) % 3;
    s = (p + 2) % 3;
    out[p] = n[p];
    rwcCross(&out[s], &n[p], &n[q]);
    rwcV3dNormalizeReciprocal(&out[s], &out[s]);
    rwcCross(&out[q], &out[s], &n[p]);
    rwcV3dNormalizeReciprocal(&out[q], &out[q]);
    dst->right = out[0];
    dst->up = out[1];
    dst->at = out[2];
    dst->pos = src->pos;
    dst->flags = (dst->flags & ~(RWC_MATRIX_TYPEMASK | RWC_MATRIX_IDENTITY)) | RWC_MATRIX_ORTHONORMAL;
    return dst;
}

/* Retail 0x003e0670: a second entry of RwMatrixOrthoNormalize with the same arguments. */
// FUN_003E0670
RwMatrix *rwcMatrixOrthoNormalizeEntry(RwMatrix *dst, const RwMatrix *src)
{
    return RwMatrixOrthoNormalize(dst, src);
}

/* The inverse of a general affine matrix: the adjugate (each cofactor the 2x2 minor, negated at odd positions) scaled by
 * the reciprocal of the determinant expanded down the first column. Only a determinant of +0 leaves it unscaled: -0
 * scales by 1 / -0, which the EE gives as -FLT_MAX. Then the position; the result's flags are cleared. */
// FUN_003DFF20
RwMatrix *MatrixInvertGeneric(RwMatrix *dst, const RwMatrix *m)
{
    RwMatrix r;
    RwReal c00, c01, c02, c10, c11, c12, c20, c21, c22, det;

    c00 = m->up.y * m->at.z - m->up.z * m->at.y;
    c01 = -(m->up.x * m->at.z - m->up.z * m->at.x);
    c02 = m->up.x * m->at.y - m->up.y * m->at.x;
    c10 = -(m->right.y * m->at.z - m->right.z * m->at.y);
    c11 = m->right.x * m->at.z - m->right.z * m->at.x;
    c12 = -(m->right.x * m->at.y - m->right.y * m->at.x);
    c20 = m->right.y * m->up.z - m->right.z * m->up.y;
    c21 = -(m->right.x * m->up.z - m->right.z * m->up.x);
    c22 = m->right.x * m->up.y - m->right.y * m->up.x;
    det = m->right.x * c00 + m->up.x * c10 + m->at.x * c20;
    if (rwcBits(det) != 0) {
        RwReal inv = 1.0f / det;

        c00 = c00 * inv; c01 = c01 * inv; c02 = c02 * inv;
        c10 = c10 * inv; c11 = c11 * inv; c12 = c12 * inv;
        c20 = c20 * inv; c21 = c21 * inv; c22 = c22 * inv;
    }
    r.right.x = c00; r.right.y = c10; r.right.z = c20;
    r.up.x = c01; r.up.y = c11; r.up.z = c21;
    r.at.x = c02; r.at.y = c12; r.at.z = c22;
    r.pos.x = -(m->pos.x * r.right.x + m->pos.y * r.up.x + m->pos.z * r.at.x);
    r.pos.y = -(m->pos.x * r.right.y + m->pos.y * r.up.y + m->pos.z * r.at.y);
    r.pos.z = -(m->pos.x * r.right.z + m->pos.y * r.up.z + m->pos.z * r.at.z);
    dst->right = r.right;
    dst->up = r.up;
    dst->at = r.at;
    dst->pos = r.pos;
    dst->flags = 0;
    return dst;
}

/* The identity is copied whole (when the module's optimisation word allows it); an orthonormal matrix inverts by
 * transposition (and is flagged plainly orthonormal); anything else in general. */
// FUN_003E0960
RwMatrix *RwMatrixInvert(RwMatrix *dst, const RwMatrix *m)
{
    RwMatrix r;

    if (m->flags & RWC_MATRIX->optimizations & RWC_MATRIX_IDENTITY) {
        *dst = *m;
        return dst;
    }
    if ((m->flags & RWC_MATRIX_TYPEMASK) != RWC_MATRIX_ORTHONORMAL)
        return MatrixInvertGeneric(dst, m);
    r.right.x = m->right.x; r.right.y = m->up.x; r.right.z = m->at.x;
    r.up.x = m->right.y; r.up.y = m->up.y; r.up.z = m->at.y;
    r.at.x = m->right.z; r.at.y = m->up.z; r.at.z = m->at.z;
    r.pos.x = -rwcDot(&m->pos, &m->right);
    r.pos.y = -rwcDot(&m->pos, &m->up);
    r.pos.z = -rwcDot(&m->pos, &m->at);
    dst->right = r.right;
    dst->up = r.up;
    dst->at = r.at;
    dst->pos = r.pos;
    dst->flags = RWC_MATRIX_ORTHONORMAL;
    return dst;
}

/* ---------------------------------------------------------------- building transformations */

static void rwcCombine(RwMatrix *m, const RwMatrix *t, RwInt32 combine)
{
    if (combine == RWC_COMBINE_PRECONCAT)
        RwMatrixMultiply(m, t, m);
    else if (combine == RWC_COMBINE_POSTCONCAT)
        RwMatrixMultiply(m, m, t);
    else
        *m = *t;
}

/* A rotation about the axis (taken as given: the caller supplies a unit axis), given 1 - cos and sin of the angle:
 * each diagonal element is 1 - (1 - cos) * (1 - a_k^2), each other one (1 - cos) * (a_i * a_j) plus or minus sin * a_k.
 * The replaced matrix's last padding word is whatever retail's stack held; here it is cleared like the others. */
// FUN_003E0680
RwMatrix *RwMatrixRotateOneMinusCosineSine(RwMatrix *m, const RwV3d *axis, RwReal oneMinusCos, RwReal sine, RwInt32 combine)
{
    RwMatrix r;
    RwReal x = axis->x, y = axis->y, z = axis->z;
    RwReal xy = oneMinusCos * (x * y), xz = oneMinusCos * (x * z), yz = oneMinusCos * (y * z);

    r.right.x = 1.0f - oneMinusCos * (1.0f - x * x);
    r.right.y = xy + sine * z;
    r.right.z = xz - sine * y;
    r.up.x = xy - sine * z;
    r.up.y = 1.0f - oneMinusCos * (1.0f - y * y);
    r.up.z = yz + sine * x;
    r.at.x = xz + sine * y;
    r.at.y = yz - sine * x;
    r.at.z = 1.0f - oneMinusCos * (1.0f - z * z);
    r.pos.x = 0.0f; r.pos.y = 0.0f; r.pos.z = 0.0f;
    r.flags = RWC_MATRIX_ORTHONORMAL;
    r.pad1 = 0; r.pad2 = 0; r.pad3 = 0;
    rwcCombine(m, &r, combine);
    return m;
}

/* A rotation of `angle` degrees about the axis, normalised first. */
// FUN_003E0870
RwMatrix *RwMatrixRotate(RwMatrix *m, const RwV3d *axis, RwReal angle, RwInt32 combine)
{
    RwReal radians = angle * (3.14159265358979323846f / 180.0f);
    RwV3d unit;

    rwcV3dNormalizeReciprocal(&unit, axis);
    return RwMatrixRotateOneMinusCosineSine(m, &unit, 1.0f - cosf(radians), sinf(radians), combine);
}

// FUN_003E0A90
RwMatrix *RwMatrixScale(RwMatrix *m, const RwV3d *s, RwInt32 combine)
{
    if (combine == RWC_COMBINE_PRECONCAT) {
        m->right.x = m->right.x * s->x; m->right.y = m->right.y * s->x; m->right.z = m->right.z * s->x;
        m->up.x = m->up.x * s->y; m->up.y = m->up.y * s->y; m->up.z = m->up.z * s->y;
        m->at.x = m->at.x * s->z; m->at.y = m->at.y * s->z; m->at.z = m->at.z * s->z;
    } else if (combine == RWC_COMBINE_POSTCONCAT) {
        m->right.x = m->right.x * s->x; m->right.y = m->right.y * s->y; m->right.z = m->right.z * s->z;
        m->up.x = m->up.x * s->x; m->up.y = m->up.y * s->y; m->up.z = m->up.z * s->z;
        m->at.x = m->at.x * s->x; m->at.y = m->at.y * s->y; m->at.z = m->at.z * s->z;
        m->pos.x = m->pos.x * s->x; m->pos.y = m->pos.y * s->y; m->pos.z = m->pos.z * s->z;
    } else {
        m->right.x = s->x; m->right.y = 0.0f; m->right.z = 0.0f;
        m->up.x = 0.0f; m->up.y = s->y; m->up.z = 0.0f;
        m->at.x = 0.0f; m->at.y = 0.0f; m->at.z = s->z;
        m->pos.x = 0.0f; m->pos.y = 0.0f; m->pos.z = 0.0f;
    }
    m->flags &= ~(RWC_MATRIX_TYPEMASK | RWC_MATRIX_IDENTITY);
    return m;
}

// FUN_003E0C90
RwMatrix *RwMatrixTranslate(RwMatrix *m, const RwV3d *t, RwInt32 combine)
{
    if (combine == RWC_COMBINE_PRECONCAT) {
        m->pos.x = t->x * m->right.x + t->y * m->up.x + t->z * m->at.x + m->pos.x;
        m->pos.y = t->x * m->right.y + t->y * m->up.y + t->z * m->at.y + m->pos.y;
        m->pos.z = t->x * m->right.z + t->y * m->up.z + t->z * m->at.z + m->pos.z;
        m->flags &= ~RWC_MATRIX_IDENTITY;
    } else if (combine == RWC_COMBINE_POSTCONCAT) {
        m->pos.x = m->pos.x + t->x;
        m->pos.y = m->pos.y + t->y;
        m->pos.z = m->pos.z + t->z;
        m->flags &= ~RWC_MATRIX_IDENTITY;
    } else {
        m->right.x = 1.0f; m->right.y = 0.0f; m->right.z = 0.0f;
        m->up.x = 0.0f; m->up.y = 1.0f; m->up.z = 0.0f;
        m->at.x = 0.0f; m->at.y = 0.0f; m->at.z = 1.0f;
        m->pos = *t;
        m->flags = (m->flags & ~RWC_MATRIX_IDENTITY) | RWC_MATRIX_ORTHONORMAL;
    }
    return m;
}

// FUN_003E0E20
RwMatrix *RwMatrixTransform(RwMatrix *m, const RwMatrix *t, RwInt32 combine)
{
    if (combine == RWC_COMBINE_PRECONCAT || combine == RWC_COMBINE_POSTCONCAT) {
        rwcCombine(m, t, combine);
    } else {
        *m = *t;
    }
    return m;
}

/* ---------------------------------------------------------------- lifetime */

// FUN_003E0F40
RwBool RwMatrixDestroy(RwMatrix *m)
{
    D_008873FC(RWC_MATRIX->freeList, m);
    return RWC_TRUE;
}

// FUN_003E0F80
RwMatrix *RwMatrixCreate(void)
{
    RwMatrix *m = (RwMatrix *)D_008873F8(RWC_MATRIX->freeList, RWC_MATRIX_HINT);

    if (m) {
        m->right.x = 1.0f;
        m->flags = RWC_MATRIX_ORTHONORMAL | RWC_MATRIX_IDENTITY;
        m->up.y = 1.0f;
        m->at.z = 1.0f;
    }
    return m;
}

// FUN_003E02C0
void *_rwMatrixOpen(void *instance, RwInt32 offset, RwInt32 size)
{
    RwcMatrixGlobals *g;

    (void)size;
    D_00764858.globalsOffset = offset;
    g = RWC_MATRIX;
    g->freeList = RwFreeListCreateAndPreallocateSpace(sizeof(RwMatrix), 256, 16, 1, D_008871C0, RWC_MATRIX_FREELIST_HINT);
    if (!g->freeList)
        return 0;
    g->optimizations = RWC_MATRIX_OPTIMIZE_IDENTITY;
    _rwMatrixSetMultFn(0);
    g->tolerance.normal = 0.01f;
    g->tolerance.orthogonal = 0.01f;
    g->tolerance.identity = 0.01f;
    D_00764858.numInstances++;
    return instance;
}

// FUN_003E0250
void *_rwMatrixClose(void *instance, RwInt32 offset, RwInt32 size)
{
    RwcMatrixGlobals *g = RWC_MATRIX;

    (void)offset;
    (void)size;
    if (g->freeList) {
        RwFreeListDestroy(g->freeList);
        g->freeList = 0;
    }
    D_00764858.numInstances--;
    return instance;
}
