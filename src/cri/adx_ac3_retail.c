/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* The three ADXB AC3-side dispatchers the retail ADXSJD functions call after their AHX counterparts (retail 0x004c1a68 / 0x004c1a98 / 0x004c1ab8), translated from the asm.
 * The ADXB object keeps a second decoder at +0xC0 (the AC3 module; null in this game): each dispatcher forwards to the function the module registered in a data word
 * when the decoder is attached. The decomp has them only as INCLUDE_ASM / asm blocks, and the RE4 copy of ADXSJD (src/cri/re4/adx_sjd.c) does not call them, which
 * config/mirror_fixes.txt now repairs (ADXSJD_SetInSj, ADXSJD_TermSupply, ADXSJD_SetMaxDecSmpl). */
#include <stdint.h>

#include "p4pc/gmem.h"

/* 0x004c1a68: set the input stream joint of the AC3 decoder: slot 0x007149F8 gets (decoder, sji); sji is the caller's second argument, left in $a1. */
void func_004c1a68(uint8_t *adxb, void *sji)
{
    void *dec = P4_G2H(*(uint32_t *)(adxb + 0xC0));

    if (dec != 0) {
        ((void (*)(void *, void *))p4_fn_g2h_sig(P4_G32(0x007149F8u), P4_SIG(2, 0, 1, 1, 0, 0, 0, 0)))(dec, sji);
    }
}

/* 0x004c1a98 (ADXB_SetAc3DecSmpl, an asm block in the decomp): the decode limit in samples: adxb[0xC4] = nsmpl, adxb[0xC8] = nsmpl / 0x600 (signed). */
void ADXB_SetAc3DecSmpl(uint8_t *adxb, int32_t nsmpl)
{
    *(int32_t *)(adxb + 0xC4) = nsmpl;
    *(int32_t *)(adxb + 0xC8) = nsmpl / 0x600;
}

/* 0x004c1ab8: end of input for the AC3 decoder: slot 0x007149FC gets the decoder. */
void func_004c1ab8(uint8_t *adxb)
{
    void *dec = P4_G2H(*(uint32_t *)(adxb + 0xC0));

    if (dec != 0) {
        ((void (*)(void *))p4_fn_g2h_sig(P4_G32(0x007149FCu), P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(dec);
    }
}
