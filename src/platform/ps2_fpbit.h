/* PS2 libgcc soft-float helpers that read and write the fp-bit struct.
 *
 * Retail routines (decomp names in parentheses): __pack_f at 0x0044DB98, __unpack_f at
 * 0x0044DC50, __unpack_d at 0x0044DDF0, __fpcmp_parts_d at 0x0044E508. Their fields sit at
 * class +0, sign +4, normal_exp +8, and frac +12 (float) or +16 (double, 64-bit). */
#ifndef P4PC_PS2_FPBIT_H
#define P4PC_PS2_FPBIT_H

#include <stddef.h>
#include <stdint.h>
#include "p4pc/align.h"
#include "p4pc/guest.h"

enum {
    P4_FP_SNAN = 0,
    P4_FP_QNAN = 1,
    P4_FP_ZERO = 2,
    P4_FP_NUMBER = 3,
    P4_FP_INF = 4,
};

typedef struct P4FpF {
    int32_t cls;
    int32_t sign;
    int32_t normal_exp;
    uint32_t frac;
} P4FpF;

typedef struct P4FpD {
    int32_t cls;
    int32_t sign;
    int32_t normal_exp;
    uint64_t frac P4_PS2_ALIGN64;
} P4FpD;

_Static_assert(offsetof(P4FpF, normal_exp) == 8, "P4FpF.normal_exp must sit at +8");
_Static_assert(offsetof(P4FpF, frac) == 12, "P4FpF.frac must sit at +12 (__pack_f)");
_Static_assert(sizeof(P4FpF) == 16, "P4FpF must be 16 bytes");
_Static_assert(offsetof(P4FpD, normal_exp) == 8, "P4FpD.normal_exp must sit at +8");
_Static_assert(offsetof(P4FpD, frac) == 16, "P4FpD.frac must sit at +16 (__unpack_d)");
_Static_assert(sizeof(P4FpD) == 24, "P4FpD must be 24 bytes");

/* Retail NaN object at 0x007564E8, returned by add and mul for INF-INF with opposite signs and
 * for 0 * INF. */
#define P4_FP_NAN_OBJECT ((P4FpD *)P4_G2H(0x007564E8u))

float func_0044db98(const P4FpF *fp);              /* __pack_f */
void func_0044dc50(const float *in, P4FpF *out);   /* __unpack_f */
void func_0044ddf0(const double *in, P4FpD *out);  /* __unpack_d */
int func_0044e508(const P4FpD *a, const P4FpD *b);  /* __fpcmp_parts_d */
uint64_t func_0044dd18(const P4FpD *fp);           /* __pack_d */
uint64_t func_0044e7a8(int32_t cls, int32_t sign, int32_t normal_exp, uint64_t frac);
P4FpD *func_0044de80(P4FpD *a, P4FpD *b, P4FpD *out);   /* __add_core */
P4FpD *func_0044e1c0(P4FpD *a, P4FpD *b, P4FpD *out);   /* __mul_core */
P4FpD *func_0044e410(P4FpD *a, P4FpD *b);                /* __div_core */
float func_0044dca8(int32_t cls, int32_t sign, int32_t normal_exp, uint32_t frac);   /* pack_f wrapper */
uint64_t fptodp(float f);                   /* fptodp */
uint64_t func_0044e0a8(uint64_t a, uint64_t b); /* double add wrapper */
uint64_t dpdiv(uint64_t a, uint64_t b);       /* double divide wrapper */
int dpcmp(uint64_t a, uint64_t b);           /* double compare wrapper */
uint64_t litodp(int32_t value);             /* signed word -> double bits */
int32_t dptoli(uint64_t bits);              /* double bits -> saturated word */
uint64_t dpsub(uint64_t a, uint64_t b);    /* dpsub */
uint64_t func_0044e168(uint64_t a, uint64_t b);    /* double multiply wrapper */
float dptofp(uint64_t bits);                /* dptofp */
uint64_t func_0044ba00(int64_t value);       /* signed doubleword -> double bits */
uint64_t func_0044bb60(uint64_t bits);       /* double bits -> unsigned doubleword */
int64_t func_0044bc68(uint64_t bits);        /* double bits -> signed doubleword */
uint32_t func_0044e830(uint64_t bits);       /* double bits -> unsigned word */

#endif /* P4PC_PS2_FPBIT_H */
