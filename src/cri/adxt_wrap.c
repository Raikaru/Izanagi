/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* ADXT handle create and destroy entry points (retail 0x004d3850 and 0x004d3cd8): the public wrappers
 * around the inner create (0x004d38a8) and destroy (0x004d3d08), which the re4 ADXT_Create and
 * ADXT_Destroy implement. Each takes the ADX lock functions 0x004c54d8 and 0x004c54e0 (empty in
 * retail) around the inner call and returns its result.
 *
 * The inner create differs from the re4 one in one store: retail sets ainf_sw (+0xad) through
 * 0x004d5430, which config/mirror_fixes.txt adds to the re4 body. The create variant with the
 * Pro Logic II encoder (0x004d3bc0 and 0x004d3c08, which add 0x004eb020) and the third detach hook
 * retail's destroy calls (0x00724e80, set only by that encoder) are not translated: they are the
 * PS2 audio-output path.
 */
#include <stdint.h>
#include "cri_mem.h"

extern void func_004c54d8(void);
extern void func_004c54e0(void);
extern void *ADXT_Create(int32_t maxnch, void *work, int32_t worksize);
extern void ADXT_Destroy(void *adxt);
extern int32_t ADXSJD_GetNumChan(void *sjd);
extern void ADXERR_CallErrFunc1(const char *message);

/* Retail 0x004d4c58 / 0x004d4c90: the public/inner full-width channel getter,
 * distinct from the format-width getter at 0x004d4d68 / 0x004d4da0.
 * ADXT_ExecHndl uses this count to walk the two output joints at +0x18. */
int32_t ADXT_GetNumChan(void *adxt)
{
    const uint8_t *handle = adxt;

    if (handle == NULL) {
        ADXERR_CallErrFunc1("E02080820 ADXT_GetNumChan: parameter error");
        return -1;
    }
    if ((int8_t)handle[1] >= 2) {
        return ADXSJD_GetNumChan(PP(handle, 4));
    }
    return 0;
}

/* 0x004d3850: ADXT_Create under the ADX lock. */
void *func_004d3850(int32_t maxnch, void *work, int32_t worksize)
{
    void *adxt;

    func_004c54d8();
    adxt = ADXT_Create(maxnch, work, worksize);
    func_004c54e0();
    return adxt;
}

/* 0x004d3cd8: ADXT_Destroy under the ADX lock. */
void func_004d3cd8(void *adxt)
{
    func_004c54d8();
    ADXT_Destroy(adxt);
    func_004c54e0();
}
