/* SPDX-License-Identifier: MIT */
/* Izanagi RenderWare clean layer: 2D and 3D vectors (docs/rw_clean.md, phase 1).
 *
 * Retail facts this file reproduces (recorded by running the retail functions in the R5900 interpreter):
 * - The vector module's globals hold two unused words (zero in retail) and the four transform providers the engine
 *   calls for RwV3dTransformPoint(s)/Vector(s); a NULL provider selects this file's default.
 * - Lengths use the EE's sqrt.s; normalising scales by the reciprocal length (a zero vector by zero, written as zeros)
 *   and RwV3dNormalize/RwV2dNormalize report a zero length as core error 0x19. */
#include "rwc.h"
#include "p4pc/eefloat.h"

typedef RwV3d *(*RwcVectorMultFn)(RwV3d *out, const RwV3d *in, const RwMatrix *matrix);
typedef RwV3d *(*RwcVectorsMultFn)(RwV3d *out, const RwV3d *in, RwInt32 count, const RwMatrix *matrix);

typedef struct RwcVectorGlobals {
    RwUInt32 reserved0;
    RwUInt32 reserved1;
    RwcVectorMultFn multPoint;
    RwcVectorsMultFn multPoints;
    RwcVectorMultFn multVector;
    RwcVectorsMultFn multVectors;
} RwcVectorGlobals;

_Static_assert(__builtin_offsetof(RwcVectorGlobals, multPoint) == 8, "vector providers follow two words");

/* The vector module's bookkeeping (retail gp-relative 0x00764890). */
extern RwcModuleInfo D_00764890;
#define RWC_VECTOR RWC_MODULE_GLOBALS(RwcVectorGlobals, D_00764890)

#define RWC_ERR_ZERO_LENGTH 0x19

static void rwcReportZeroLength(void)
{
    RwError e;

    e.pluginID = 1;
    e.errorCode = _rwerror(RWC_ERR_ZERO_LENGTH);
    RwErrorSet(&e);
}

/* The default providers: point = in.x * right + in.y * up + in.z * at + pos, a vector without pos; a lane is never -0. */

// FUN_003E3D00
RwV3d *VectorMultPoints(RwV3d *out, const RwV3d *in, RwInt32 count, const RwMatrix *m)
{
    RwInt32 i;

    for (i = 0; i < count; i++) {
        RwReal x = in[i].x, y = in[i].y, z = in[i].z;

        out[i].x = (x * m->right.x + y * m->up.x + z * m->at.x + m->pos.x) + 0.0f;
        out[i].y = (x * m->right.y + y * m->up.y + z * m->at.y + m->pos.y) + 0.0f;
        out[i].z = (x * m->right.z + y * m->up.z + z * m->at.z + m->pos.z) + 0.0f;
    }
    return out;
}

// FUN_003E3DC0
RwV3d *VectorMultPoint(RwV3d *out, const RwV3d *in, const RwMatrix *m)
{
    RwReal x = in->x, y = in->y, z = in->z;

    out->x = (x * m->right.x + y * m->up.x + z * m->at.x + m->pos.x) + 0.0f;
    out->y = (x * m->right.y + y * m->up.y + z * m->at.y + m->pos.y) + 0.0f;
    out->z = (x * m->right.z + y * m->up.z + z * m->at.z + m->pos.z) + 0.0f;
    return out;
}

// FUN_003E3E60
RwV3d *VectorMultVectors(RwV3d *out, const RwV3d *in, RwInt32 count, const RwMatrix *m)
{
    RwInt32 i;

    for (i = 0; i < count; i++) {
        RwReal x = in[i].x, y = in[i].y, z = in[i].z;

        out[i].x = (x * m->right.x + y * m->up.x + z * m->at.x) + 0.0f;
        out[i].y = (x * m->right.y + y * m->up.y + z * m->at.y) + 0.0f;
        out[i].z = (x * m->right.z + y * m->up.z + z * m->at.z) + 0.0f;
    }
    return out;
}

// FUN_003E3F00
RwV3d *VectorMultVector(RwV3d *out, const RwV3d *in, const RwMatrix *m)
{
    RwReal x = in->x, y = in->y, z = in->z;

    out->x = (x * m->right.x + y * m->up.x + z * m->at.x) + 0.0f;
    out->y = (x * m->right.y + y * m->up.y + z * m->at.y) + 0.0f;
    out->z = (x * m->right.z + y * m->up.z + z * m->at.z) + 0.0f;
    return out;
}

// FUN_003E3F80
RwBool _rwVectorSetMultFn(RwV3d *(*point)(RwV3d *, const RwV3d *, const RwMatrix *),
                          RwV3d *(*points)(RwV3d *, const RwV3d *, RwInt32, const RwMatrix *),
                          RwV3d *(*vector)(RwV3d *, const RwV3d *, const RwMatrix *),
                          RwV3d *(*vectors)(RwV3d *, const RwV3d *, RwInt32, const RwMatrix *))
{
    RwcVectorGlobals *g = RWC_VECTOR;

    g->multPoint = point ? point : VectorMultPoint;
    g->multPoints = points ? points : VectorMultPoints;
    g->multVector = vector ? vector : VectorMultVector;
    g->multVectors = vectors ? vectors : VectorMultVectors;
    return RWC_TRUE;
}

/* Engine module open/close (called by the engine with the module's place in the globals block). */
void *_rwVectorOpen(void *instance, RwInt32 offset, RwInt32 size)
{
    RwcVectorGlobals *g;

    (void)size;
    D_00764890.globalsOffset = offset;
    g = RWC_VECTOR;
    g->reserved0 = 0;
    g->reserved1 = 0;
    _rwVectorSetMultFn(0, 0, 0, 0);
    D_00764890.numInstances++;
    return instance;
}

void *_rwVectorClose(void *instance, RwInt32 offset, RwInt32 size)
{
    (void)offset;
    (void)size;
    D_00764890.numInstances--;
    return instance;
}

/* The scale a normalise applies: the reciprocal of a positive length, else the (zero) length itself, so a zero vector
 * is written as zeros of its components' signs. */
static RwReal rwcNormalizeScale(RwReal len)
{
    return len > 0.0f ? 1.0f / len : len;
}

/* Retail 0x003e4030: normalises `in` into `out` and returns the scale it applied (the reciprocal length); no error. */
// FUN_003E4030
RwReal rwcV3dNormalizeReciprocal(RwV3d *out, const RwV3d *in)
{
    RwReal recip = rwcNormalizeScale(p4pc_ee_sqrt(in->y * in->y + in->x * in->x + in->z * in->z));

    out->x = in->x * recip;
    out->y = in->y * recip;
    out->z = in->z * recip;
    return recip;
}

/* The engine-internal normalise the core's geometry code uses: the unchecked form of RwV3dNormalize. */
RwReal _rwV3dNormalize(RwV3d *out, const RwV3d *in)
{
    return rwcV3dNormalizeReciprocal(out, in);
}

// FUN_003E40B0
RwReal RwV3dNormalize(RwV3d *out, const RwV3d *in)
{
    RwReal len = p4pc_ee_sqrt(in->y * in->y + in->x * in->x + in->z * in->z);
    RwReal recip = rwcNormalizeScale(len);

    if (!(len > 0.0f))
        rwcReportZeroLength();
    out->x = in->x * recip;
    out->y = in->y * recip;
    out->z = in->z * recip;
    return len;
}

// FUN_003E4180
RwReal RwV3dLength(const RwV3d *in)
{
    return p4pc_ee_sqrt(in->y * in->y + in->x * in->x + in->z * in->z);
}

// FUN_003E41B0
RwReal RwV2dLength(const RwV2d *in)
{
    return p4pc_ee_sqrt(in->y * in->y + in->x * in->x);
}

// FUN_003E41E0
RwReal RwV2dNormalize(RwV2d *out, const RwV2d *in)
{
    RwReal len = p4pc_ee_sqrt(in->y * in->y + in->x * in->x);
    RwReal recip = rwcNormalizeScale(len);

    if (!(len > 0.0f))
        rwcReportZeroLength();
    out->x = in->x * recip;
    out->y = in->y * recip;
    return len;
}

// FUN_003E42A0
RwV3d *RwV3dTransformPoint(RwV3d *out, const RwV3d *in, const RwMatrix *matrix)
{
    return RWC_VECTOR->multPoint(out, in, matrix);
}

// FUN_003E42E0
RwV3d *RwV3dTransformPoints(RwV3d *out, const RwV3d *in, RwInt32 count, const RwMatrix *matrix)
{
    return RWC_VECTOR->multPoints(out, in, count, matrix);
}

// FUN_003E4320
RwV3d *RwV3dTransformVector(RwV3d *out, const RwV3d *in, const RwMatrix *matrix)
{
    return RWC_VECTOR->multVector(out, in, matrix);
}

// FUN_003E4360
RwV3d *RwV3dTransformVectors(RwV3d *out, const RwV3d *in, RwInt32 count, const RwMatrix *matrix)
{
    return RWC_VECTOR->multVectors(out, in, count, matrix);
}
