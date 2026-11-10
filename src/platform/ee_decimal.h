/* Internal decimal conversion ABI. Bigint and reentrancy links remain four-byte
 * guest pointers, including on 64-bit hosts. */
#ifndef P4PC_EE_DECIMAL_H
#define P4PC_EE_DECIMAL_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/guest.h"
/* Shared retail tables used by dtoa and strtod. */
#define P4_DECIMAL_TENS ((const uint64_t *)P4_G2H(0x00754f38u))
#define P4_DECIMAL_BIGTENS ((const uint64_t *)P4_G2H(0x00755000u))
#define P4_DECIMAL_TINYTENS ((const uint64_t *)P4_G2H(0x00755028u))
typedef struct P4DecimalBig {
    uint32_t next;
    int32_t k, maxwds, sign, wds;
    uint32_t x[];
} P4DecimalBig;
_Static_assert(offsetof(P4DecimalBig, x) == 20, "retail bigint limb offset");
P4DecimalBig *p4_decimal_alloc(uint32_t *reent, int k);
void p4_decimal_free(uint32_t *reent, P4DecimalBig *b);
P4DecimalBig *p4_decimal_multadd(uint32_t *, P4DecimalBig *, int, int);
P4DecimalBig *p4_decimal_i2b(uint32_t *, int);
P4DecimalBig *p4_decimal_mult(uint32_t *, P4DecimalBig *, P4DecimalBig *);
P4DecimalBig *p4_decimal_pow5(uint32_t *, P4DecimalBig *, int);
P4DecimalBig *p4_decimal_lshift(uint32_t *, P4DecimalBig *, int);
int p4_decimal_cmp(const P4DecimalBig *, const P4DecimalBig *);
P4DecimalBig *p4_decimal_diff(uint32_t *, P4DecimalBig *, P4DecimalBig *);
int p4_decimal_hi0(uint32_t);
int p4_decimal_lo0(uint32_t *);
P4DecimalBig *p4_decimal_d2b(uint32_t *, uint64_t, int *, int *);
char *p4_decimal_dtoa(uint32_t *, uint64_t, int, int, int *, int *, char **);
int p4_decimal_quorem(P4DecimalBig *, const P4DecimalBig *);
P4DecimalBig *p4_decimal_s2b(uint32_t *, const char *, int, int, uint32_t);
uint64_t p4_decimal_ulp(uint64_t);
uint64_t p4_decimal_b2d(const P4DecimalBig *, int *);
uint64_t p4_decimal_ratio(const P4DecimalBig *, const P4DecimalBig *);
uint64_t p4_decimal_strtod(uint32_t *, const char *, const char **);
static inline void p4_decimal_copy(P4DecimalBig *dst, const P4DecimalBig *src)
{
    dst->sign = src->sign;
    dst->wds = src->wds;
    for (int i = 0; i < src->wds; ++i) dst->x[i] = src->x[i];
}
#endif
