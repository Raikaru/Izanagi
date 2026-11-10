/* The sky2 driver's VU0 matrix and vector multiplies (retail 0x00402390..0x00402570, decomp src/rw/baskytran.c, excluded with
 * the sky driver), as src/platform/rw_skytran.c ports them. Retail's FINALIZESTART installs them into RenderWare's engine slots
 * (_rwMatrixSetMultFn, _rwVectorSetMultFn); the GL device does the same (src/render/p4gl_device.c). */
#ifndef P4PC_RW_SKYTRAN_H
#define P4PC_RW_SKYTRAN_H

#include <stdint.h>

void func_00402390(void *dst, const void *a, const void *b);                          /* matrixASMMult */
void *func_00402410(void *out, const void *in, const void *matrix);                   /* vectorASMMultPointSingle */
void *func_00402470(void *out, const void *in, int32_t count, const void *matrix);    /* vectorASMMultPoints */
void *func_00402510(void *out, const void *in, const void *matrix);                   /* vectorASMMultVectorSingle */
void *func_00402570(void *out, const void *in, int32_t count, const void *matrix);    /* vectors without translation */

#endif
