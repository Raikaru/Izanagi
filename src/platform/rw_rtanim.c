/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* RenderWare functions that the decomp holds only in excluded units (src/promoted/code1_003d_cw119.c,
 * src/rw/badevice.c); their markers are not in a compiled unit, so an overrides/ file would not
 * apply. Port-owned definitions at their retail names, written from the retail asm.
 *
 * Retail 003d5790 (RtAnimInterpolatorCreate): allocates an interpolator for `numNodes` nodes of
 * `maxInterpKeyFrameSize` bytes (allocation size numNodes * size + 0x4c, hint 0x3001b) and clears it.
 * Words set: [8] and [9] = the keyframe size, [10] = -1 (no animation keyframe size yet), [0xb] = node
 * count, [5] = -1.0 (no callback time), [0xe] = the interpolator itself (the keyframe-array base
 * link); the rest are zero. The allocation result is not checked, as in retail. */
#include <stdint.h>

#include "p4pc/gmem.h"

/* The allocator slot 0x008873E8 (jtbl_008873E8[0]): a guest code address. */
typedef void *(*P4AnimAllocFn)(int size, unsigned int flags);

unsigned int *func_003d5790(int numNodes, int maxInterpKeyFrameSize)
{
    unsigned int *w = (unsigned int *)((P4AnimAllocFn)p4_fn_g2h_sig(P4_G32(0x008873E8u), P4_SIG(2, 1, 0, 0, 0, 0, 0, 0)))(numNodes * maxInterpKeyFrameSize + 0x4c, 0x3001b);

    w[0xb] = (unsigned int)numNodes;
    w[0] = 0;
    w[2] = 0;
    w[1] = 0;
    w[3] = 0;
    w[5] = 0xbf800000u;
    w[4] = 0;
    w[6] = 0;
    w[7] = 0;
    w[9] = (unsigned int)maxInterpKeyFrameSize;
    w[10] = 0xffffffffu;
    w[8] = (unsigned int)maxInterpKeyFrameSize;
    w[0xc] = 0;
    w[0xd] = 0;
    w[0xe] = P4_H2G(w);
    w[0xf] = 0;
    w[0x11] = 0;
    w[0x10] = 0;
    w[0x12] = 0;
    return w;
}

/* Retail 003e8920 (badevice.c; the unit stays excluded for its data duplicates): returns the constant
 * 0x37002, the RenderWare 3.7 library-version word that RwEngine plugin registration passes on. */
unsigned int func_003e8920(void)
{
    return 0x37002;
}

/* The decomp's mdlManager.c declares retail 003d5e40 a second time as func_003d5e40_typed(f32 frame, void
 * *interpolator): mwcc passes the float in f12 and the pointer in a0 whatever the declaration order, so
 * the retail call is func_003d5e40(interpolator, frame). The generator aliased the typed name to
 * func_003d5e40 with a jump, which on x86 leaves the stack arguments in the declared order and
 * passes the float as the interpolator pointer. This swaps them. */
extern int func_003d5e40(unsigned char *interpolator, float frame);

void func_003d5e40_typed(float frame, void *interpolator)
{
    func_003d5e40((unsigned char *)interpolator, frame);
}
