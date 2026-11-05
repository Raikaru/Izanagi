/* SPDX-License-Identifier: MIT */
/* Izanagi RenderWare clean layer: the PlayStation 2 driver's matrix and vector transform providers (docs/rw_clean.md, phase 1).
 *
 * At engine start the retail driver installs VU0 routines in the engine's matrix-multiply and vector-transform slots
 * (retail 0x00402390..0x00402570); the GL device installs these (src/render/p4gl_device.c). They compute in VU0 macro
 * arithmetic (rwc.h: rwcVuMul/rwcVuMadd): each output lane is the accumulated sum right * x, + up * y, + at * z, then
 * + pos * 1 for a point, one truncation per step. A matrix product is that transform of each of the left matrix's rows,
 * stored as whole register quadwords: the destination's padding words get the w words vf10-vf12 already held, and the
 * provider keeps the destination's flags word (RwMatrixMultiply writes its own).
 *
 * VU0 registers are state the game sees: code that runs after these routines stores lanes it did not set, so each routine
 * leaves the port's VU0 register file (p4pc_vu0, include/p4pc/vu0.h) as retail's does (recorded in the interpreter):
 * - matrix product: vf1-vf4 the left matrix's rows and vf5-vf8 the right's (all four words), vf9-vf12 the result rows
 *   (x, y, z; w kept, and stored with the rows), ACC the position row's sum before the right matrix's position is added
 *   (x, y, z; w kept); the provider also loads the destination's first row into vf13 and keeps its flags word in vf9.w;
 * - transforms: vf1-vf4 (vf1-vf3 for vectors) the matrix rows; a single point or vector computes in vf5 (vf4) loaded
 *   with the input and w = 0, ACC the sum before the last term; the array forms leave the last result in vf6 (x, y, z)
 *   and in vf5 the vector after the last one (x, y, z, w = 0), which they load ahead. */
#include "rwc.h"
#include "p4pc/vu0.h"

/* A matrix row with the word after it (flags or padding), as the VU's quadword load reads it. */
static void rwcVuLoadRow(int reg, const RwV3d *row)
{
    const RwUInt32 *w = (const RwUInt32 *)row;

    p4pc_vu0.vf[reg].u[0] = w[0];
    p4pc_vu0.vf[reg].u[1] = w[1];
    p4pc_vu0.vf[reg].u[2] = w[2];
    p4pc_vu0.vf[reg].u[3] = w[3];
}

static void rwcVuLoadVector(int reg, const RwV3d *v)
{
    const RwUInt32 *w = (const RwUInt32 *)v;

    p4pc_vu0.vf[reg].u[0] = w[0];
    p4pc_vu0.vf[reg].u[1] = w[1];
    p4pc_vu0.vf[reg].u[2] = w[2];
    p4pc_vu0.vf[reg].u[3] = 0;
}

static void rwcVuSetXyz(P4Vu0Reg *reg, RwReal x, RwReal y, RwReal z)
{
    reg->u[0] = rwcBits(x);
    reg->u[1] = rwcBits(y);
    reg->u[2] = rwcBits(z);
}

/* One input through m: out = (right * x + up * y + at * z) [+ pos], acc = the sum before its last term. */
static void rwcVuTransform(RwV3d *out, RwV3d *acc, RwReal x, RwReal y, RwReal z, const RwMatrix *m, int point)
{
    RwV3d s;

    s.x = rwcVuMadd(rwcVuMul(m->right.x, x), m->up.x, y);
    s.y = rwcVuMadd(rwcVuMul(m->right.y, x), m->up.y, y);
    s.z = rwcVuMadd(rwcVuMul(m->right.z, x), m->up.z, y);
    if (point) {
        s.x = rwcVuMadd(s.x, m->at.x, z);
        s.y = rwcVuMadd(s.y, m->at.y, z);
        s.z = rwcVuMadd(s.z, m->at.z, z);
        out->x = rwcVuMadd(s.x, m->pos.x, 1.0f);
        out->y = rwcVuMadd(s.y, m->pos.y, 1.0f);
        out->z = rwcVuMadd(s.z, m->pos.z, 1.0f);
    } else {
        out->x = rwcVuMadd(s.x, m->at.x, z);
        out->y = rwcVuMadd(s.y, m->at.y, z);
        out->z = rwcVuMadd(s.z, m->at.z, z);
    }
    *acc = s;
}

/* dst = a * b with its register effects; the caller writes dst's flags and decides vf13/vf9.w. */
void rwcVu0MatrixProduct(RwMatrix *dst, const RwMatrix *a, const RwMatrix *b)
{
    RwMatrix ra = *a, rb = *b;
    RwV3d r[4], acc;
    int i;

    rwcVuLoadRow(1, &ra.right); rwcVuLoadRow(2, &ra.up); rwcVuLoadRow(3, &ra.at); rwcVuLoadRow(4, &ra.pos);
    rwcVuLoadRow(5, &rb.right); rwcVuLoadRow(6, &rb.up); rwcVuLoadRow(7, &rb.at); rwcVuLoadRow(8, &rb.pos);
    rwcVuTransform(&r[0], &acc, ra.right.x, ra.right.y, ra.right.z, &rb, 0);
    rwcVuTransform(&r[1], &acc, ra.up.x, ra.up.y, ra.up.z, &rb, 0);
    rwcVuTransform(&r[2], &acc, ra.at.x, ra.at.y, ra.at.z, &rb, 0);
    rwcVuTransform(&r[3], &acc, ra.pos.x, ra.pos.y, ra.pos.z, &rb, 1);
    for (i = 0; i < 4; i++)
        rwcVuSetXyz(&p4pc_vu0.vf[9 + i], r[i].x, r[i].y, r[i].z);
    rwcVuSetXyz(&p4pc_vu0.acc, acc.x, acc.y, acc.z);
    dst->right = r[0];
    dst->up = r[1];
    dst->pad1 = p4pc_vu0.vf[10].u[3];
    dst->at = r[2];
    dst->pad2 = p4pc_vu0.vf[11].u[3];
    dst->pos = r[3];
    dst->pad3 = p4pc_vu0.vf[12].u[3];
}

/* dst = a * b: each row of a transformed by b, the position row as a point. */
// FUN_00402390
void rwcVu0MatrixMultiply(RwMatrix *dst, const RwMatrix *a, const RwMatrix *b)
{
    rwcVuLoadRow(13, &dst->right);
    rwcVu0MatrixProduct(dst, a, b);
    p4pc_vu0.vf[9].u[3] = p4pc_vu0.vf[13].u[3];
}

static void rwcVuTransformOne(RwV3d *out, const RwV3d *in, const RwMatrix *m, int point)
{
    int reg = point ? 5 : 4;
    RwV3d r, acc;

    rwcVuLoadRow(1, &m->right); rwcVuLoadRow(2, &m->up); rwcVuLoadRow(3, &m->at);
    if (point)
        rwcVuLoadRow(4, &m->pos);
    rwcVuLoadVector(reg, in);
    rwcVuTransform(&r, &acc, p4pc_vu0.vf[reg].f[0], p4pc_vu0.vf[reg].f[1], p4pc_vu0.vf[reg].f[2], m, point);
    rwcVuSetXyz(&p4pc_vu0.vf[reg], r.x, r.y, r.z);
    rwcVuSetXyz(&p4pc_vu0.acc, acc.x, acc.y, acc.z);
    *out = r;
}

static void rwcVuTransformMany(RwV3d *out, const RwV3d *in, RwInt32 count, const RwMatrix *m, int point)
{
    RwInt32 i;
    RwV3d r, acc;

    rwcVuLoadRow(1, &m->right); rwcVuLoadRow(2, &m->up); rwcVuLoadRow(3, &m->at);
    if (point)
        rwcVuLoadRow(4, &m->pos);
    rwcVuLoadVector(5, &in[0]);
    for (i = 0; i < count; i++) {
        RwReal x = p4pc_vu0.vf[5].f[0], y = p4pc_vu0.vf[5].f[1], z = p4pc_vu0.vf[5].f[2];

        rwcVuLoadVector(5, &in[i + 1]);
        rwcVuTransform(&r, &acc, x, y, z, m, point);
        rwcVuSetXyz(&p4pc_vu0.vf[6], r.x, r.y, r.z);
        rwcVuSetXyz(&p4pc_vu0.acc, acc.x, acc.y, acc.z);
        out[i] = r;
    }
}

// FUN_00402410
RwV3d *rwcVu0TransformPoint(RwV3d *out, const RwV3d *in, const RwMatrix *m)
{
    rwcVuTransformOne(out, in, m, 1);
    return out;
}

// FUN_00402470
RwV3d *rwcVu0TransformPoints(RwV3d *out, const RwV3d *in, RwInt32 count, const RwMatrix *m)
{
    rwcVuTransformMany(out, in, count, m, 1);
    return out;
}

// FUN_00402510
RwV3d *rwcVu0TransformVector(RwV3d *out, const RwV3d *in, const RwMatrix *m)
{
    rwcVuTransformOne(out, in, m, 0);
    return out;
}

// FUN_00402570
RwV3d *rwcVu0TransformVectors(RwV3d *out, const RwV3d *in, RwInt32 count, const RwMatrix *m)
{
    rwcVuTransformMany(out, in, count, m, 0);
    return out;
}
