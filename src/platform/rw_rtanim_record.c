/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* RtAnim record helpers that the decomp holds only in the excluded src/promoted/code1_003d_cw119.c (its
 * marker is not in a compiled unit, so an overrides/ file would not apply): port-owned definitions at their
 * retail names, written from the retail asm (checked instruction by instruction against
 * asm/nonmatchings/code1_003d/func_003d59a0.s and func_003d5e90.s). Pure C, so tests/rtanim_test.c builds
 * this file directly. */
#include <stdint.h>
#include <string.h>

/* Retail 003d59a0: copies the keyframe storage of an animation record: the 76-byte header is skipped and
 * (dst[36] * dst[44]) bytes (the frame size times the frame count of `dst`) are copied from src + 76 to
 * dst + 76 (the retail call is memcpy at 0x43f810). Returns 1. */
int func_003d59a0(unsigned char *dst, unsigned char *src)
{
    memcpy(dst + 76, src + 76, (size_t)(*(int *)(dst + 36) * *(int *)(dst + 44)));
    return 1;
}

/* Retail 003d5e90: blends two animation records into a third, node by node. The node count is a[44] and
 * each node's size is its record's +36 (a, b and c may differ). For every node the blend callback at a[64]
 * is called with the three node pointers (record + 76 + node * size) and the blend factor `t`; its result
 * is ignored. Returns 1 (also when there are no nodes). */
int func_003d5e90(unsigned char *a, unsigned char *b, unsigned char *c, float t)
{
    int node;

    for (node = 0; node < *(int *)(a + 44); node++) {
        ((void (*)(unsigned char *, unsigned char *, unsigned char *, float))
         p4_fn_g2h_sig(*(unsigned int *)(a + 64), P4_SIG(4, 0, 1, 1, 1, 0, 0, 0) | P4_SIGX(3)))(
            a + 76 + node * *(int *)(a + 36), b + 76 + node * *(int *)(b + 36), c + 76 + node * *(int *)(c + 36), t);
    }
    return 1;
}
