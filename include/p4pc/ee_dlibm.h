#ifndef P4PC_EE_DLIBM_H
#define P4PC_EE_DLIBM_H

#include <stdint.h>

/* The retail soft-double ABI passes the bit pattern in one 64-bit GPR. */
uint64_t func_004470f0(uint64_t x);              /* acos */
uint64_t func_004476b0(uint64_t x);              /* log */
uint64_t func_00447b88(uint64_t x);              /* log10 */
uint64_t func_00447d00(uint64_t x, uint64_t y);  /* pow */
uint64_t func_00448a00(uint64_t x);              /* sqrt */
#include "p4pc/ee_dfloat.h"


#endif
