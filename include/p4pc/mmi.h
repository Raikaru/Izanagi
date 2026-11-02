/* PS2 EE multimedia (MMI) intrinsics the decomp calls by their MWCCPS2 builtin names.
 * Each works on P4_Q128 (compat.h): the 16-byte quadword, little-endian lanes. */
#ifndef P4PC_MMI_H
#define P4PC_MMI_H

#include <stdint.h>

/* pcpyld rd, rs, rt: rd[127:64] = rs[63:0], rd[63:0] = rt[63:0]. */
static inline P4_Q128 _pcpyld(uint64_t rs, uint64_t rt)
{
    P4_Q128 r;
    r.w[0] = (uint32_t)rt;
    r.w[1] = (uint32_t)(rt >> 32);
    r.w[2] = (uint32_t)rs;
    r.w[3] = (uint32_t)(rs >> 32);
    return r;
}

#endif /* P4PC_MMI_H */
