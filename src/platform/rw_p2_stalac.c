/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* Retail 00411740 (StalacTiteAlloc) and 004117b0 (StalacMiteAlloc): the two-ended bump allocator
 * that RxLockedPipeUnlock and the pipeline registry passes (overrides/00412ca0.c, 004143f0.c, ...)
 * use for temporary memory. The decomp's copies are file statics in src/renderware/p2/p2define.c
 * (gMemoryLimits), which retail code outside that file reaches through the retail words
 * gp-17976 (StalacTiteBase, 0x00764AB8) and gp-17972 (StalacMiteBase, 0x00764ABC); these
 * definitions use those words so every user sees one state. The Tite end grows down from
 * StalacTiteBase and the Mite end grows up from StalacMiteBase; running them into each other
 * reports error 0x80000013 (module 1) and returns NULL with the pointer restored.
 *
 * Direct transliteration of the retail code. */
#include <stdint.h>

extern int32_t func_003df590(int32_t code, ...);
extern void *func_003df4d0(void *err);

#define P4_STALAC_TITE (*(uint32_t *)P4_G2H(0x00764AB8u))
#define P4_STALAC_MITE (*(uint32_t *)P4_G2H(0x00764ABCu))

static void p4_stalac_nomem(uint32_t size)
{
    int32_t err[2];

    err[0] = 1;
    err[1] = func_003df590((int32_t)0x80000013, size);
    func_003df4d0(err);
}

void *func_00411740(uint32_t size)
{
    size = (size + 3) & ~3u;
    P4_STALAC_TITE -= size;
    if (P4_STALAC_TITE < P4_STALAC_MITE) {
        P4_STALAC_TITE += size;
        p4_stalac_nomem(size);
        return NULL;
    }
    return (void *)P4_G2H(P4_STALAC_TITE);
}

void *func_004117b0(uint32_t size)
{
    size = (size + 3) & ~3u;
    P4_STALAC_MITE += size;
    if (P4_STALAC_TITE < P4_STALAC_MITE) {
        P4_STALAC_MITE -= size;
        p4_stalac_nomem(size);
        return NULL;
    }
    return (void *)P4_G2H(P4_STALAC_MITE - size);
}
