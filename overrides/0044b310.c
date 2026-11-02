/* func_0044b310 is fabs on a double. Retail takes the 64-bit value in a0, keeps the low
 * word, and masks the high word with 0x7fffffff, so only the sign bit is cleared (NaN
 * payloads are kept). The port passes doubles as their 64-bit bit pattern, as the soft-double
 * helpers dpsub, func_0044e168 and fptodp do (src/platform/ps2_fpbit.c): the decomp's callers feed
 * it fptodp's result and pass its result on as bits, so the bits come back in edx:eax, not in st(0). */
#include <stdint.h>

uint64_t func_0044b310(uint64_t bits)
{
    return bits & 0x7fffffffffffffffull;
}
