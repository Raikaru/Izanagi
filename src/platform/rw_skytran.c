/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* The sky2 driver's VU0 matrix and vector multiplies (retail 0x00402390..0x00402570, decomp src/rw/baskytran.c, excluded with
 * the sky driver). Retail's FINALIZESTART installs them into RenderWare's engine slots, so every RwV3dTransformPoint(s) and
 * RwV3dTransformVector(s) (0x003e42a0..0x003e4360) runs them, not the generic C VectorMultPoint family. Each one is the retail
 * instruction sequence on the shared VU0 register file (include/p4pc/vu0.h): the same registers, broadcast lanes and
 * accumulator order (x, then y, then z, then the translation through vf0.w), the VU0 rounding of the helpers (each product
 * truncated, then the exact sum truncated), and the registers left as retail leaves them. A point is loaded as lwu, lwu,
 * lwu, pcpyld and qmtc2 build it: x, y, z and a zero w; results are stored as the three words of the low 96 bits. */

#include "p4pc/rw_skytran.h"
#include "p4pc/vu0.h"

static void load_point(int vf, const void *in)
{
    uint32_t q[4];

    __builtin_memcpy(q, in, 12);
    q[3] = 0;
    p4_vu0_qmtc2(vf, q);
}

static void store_point(void *out, int vf)
{
    uint32_t q[4];

    p4_vu0_qmfc2(q, vf);
    __builtin_memcpy(out, q, 12);
}

static void load_rows(int first, const void *matrix, int rows)
{
    const uint8_t *m = matrix;
    int i;

    for (i = 0; i < rows; i++) {
        p4_vu0_lqc2(first + i, m + 16 * i);
    }
}

/* vmulax.xyz ACC, vf[r], vf[p]x; vmadday.xyz ACC, vf[r+1], vf[p]y; then z into `dst` (vmaddz) or into ACC followed by the
 * translation row vf[r+3] times vf0w (vmaddaz, vmaddw). */
static void transform(int dst, int r, int p, int translate)
{
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, r, p, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, r + 1, p, P4_VU_BC_Y);
    if (translate) {
        p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, r + 2, p, P4_VU_BC_Z);
        p4_vu0_madd(dst, P4_VU_XYZ, r + 3, 0, P4_VU_BC_W);
    } else {
        p4_vu0_madd(dst, P4_VU_XYZ, r + 2, p, P4_VU_BC_Z);
    }
}

/* matrixASMMult: dst = a x b for the affine rows. Each row of `a` broadcasts its x, y, z over b's right, up and at rows; the
 * position row also adds b's position (vf8 x vf0w). Row 0's w is dst's own flags word (lqc2 vf13 then vmove.w); the w of
 * the other rows is whatever vf10..vf12 held. */
void func_00402390(void *dst, const void *a, const void *b)
{
    uint8_t *d = dst;

    load_rows(1, a, 4);
    load_rows(5, b, 4);
    p4_vu0_lqc2(13, dst);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, 5, 1, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 6, 1, P4_VU_BC_Y);
    p4_vu0_madd(9, P4_VU_XYZ, 7, 1, P4_VU_BC_Z);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, 5, 2, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 6, 2, P4_VU_BC_Y);
    p4_vu0_madd(10, P4_VU_XYZ, 7, 2, P4_VU_BC_Z);
    p4pc_vu0.vf[9].u[3] = p4pc_vu0.vf[13].u[3];   /* vmove.w is a bit copy: the flags word is not a float */
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, 5, 3, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 6, 3, P4_VU_BC_Y);
    p4_vu0_madd(11, P4_VU_XYZ, 7, 3, P4_VU_BC_Z);
    p4_vu0_mul(P4_VU_ACC, P4_VU_XYZ, 5, 4, P4_VU_BC_X);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 6, 4, P4_VU_BC_Y);
    p4_vu0_madd(P4_VU_ACC, P4_VU_XYZ, 7, 4, P4_VU_BC_Z);
    p4_vu0_madd(12, P4_VU_XYZ, 8, 0, P4_VU_BC_W);
    p4_vu0_sqc2(d, 9);
    p4_vu0_sqc2(d + 16, 10);
    p4_vu0_sqc2(d + 32, 11);
    p4_vu0_sqc2(d + 48, 12);
}

/* vectorASMMultPointSingle: out = in transformed by the matrix with its translation. */
void *func_00402410(void *out, const void *in, const void *matrix)
{
    load_rows(1, matrix, 4);
    load_point(5, in);
    transform(5, 1, 5, 1);
    store_point(out, 5);
    return out;
}

/* vectorASMMultPoints: `count` points. Retail loads the next point before it stores the current result, so it reads one point
 * past the last one (and the first one even for count <= 0). */
void *func_00402470(void *out, const void *in, int32_t count, const void *matrix)
{
    uint8_t *o = out;
    const uint8_t *i = in;

    load_rows(1, matrix, 4);
    load_point(5, i);
    for (; count > 0; count--) {
        i += 12;
        transform(6, 1, 5, 1);
        load_point(5, i);
        store_point(o, 6);
        o += 12;
    }
    return out;
}

/* vectorASMMultVectorSingle: out = in transformed by the matrix's right, up and at rows (no translation). */
void *func_00402510(void *out, const void *in, const void *matrix)
{
    load_rows(1, matrix, 3);
    load_point(4, in);
    transform(4, 1, 4, 0);
    store_point(out, 4);
    return out;
}

/* The vectors form of 00402510 (no retail name): the same loop and read-ahead as 00402470, without the translation. */
void *func_00402570(void *out, const void *in, int32_t count, const void *matrix)
{
    uint8_t *o = out;
    const uint8_t *i = in;

    load_rows(1, matrix, 3);
    load_point(5, i);
    for (; count > 0; count--) {
        i += 12;
        transform(6, 1, 5, 0);
        load_point(5, i);
        store_point(o, 6);
        o += 12;
    }
    return out;
}
