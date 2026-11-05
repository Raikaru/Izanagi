/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* ADXT handle create and destroy entry points: the public wrappers (retail 0x004d3850 and
 * 0x004d3cd8) and the inner create (0x004d38a8, adxt_Create), translated from the retail asm. The
 * wrappers take the ADX lock functions 0x004c54d8 and 0x004c54e0 (empty in retail) around the inner
 * call; the inner destroy (0x004d3d08) is the re4 ADXT_Destroy.
 *
 * The re4 ADXT_Create is the GameCube library's: it reserves 0x3060 output samples plus 0x40C0 bytes
 * of renderer work per channel in front of the file ring and hands that work to ADXRNA_Create. The
 * PS2 inner create reserves only the 0x2060-sample output rings (the IOP renderer takes no EE work),
 * so the file ring of the same work area is 0x2000 bytes per channel larger: the battle's two-channel
 * ACSSND voice handles (0x1c1d8 bytes of work) get 39 sectors and a reload threshold of 33, where the re4
 * layout gave 31 and 26. The create variant with the Pro Logic II encoder (0x004d3bc0 and
 * 0x004d3c08, which add 0x004eb020) and the third detach hook retail's destroy calls (0x00724e80,
 * set only by that encoder) are not translated: they are the PS2 audio-output path.
 */
#include <stdint.h>
#include <string.h>
#include "cri_mem.h"

#define ADXT_OBJ_SIZE 0xc8
#define ADXT_MAX_OBJ 16

/* The handle table (retail 0x007188c0, re4/adx_inis.c adxt_obj) is reached through the decomp's
 * accessor, so this create and the re4 server loop, ADXT_Init and ADXT_DestroyAll use the same
 * storage on every build: the i386 build left handles made at the retail address unserved. */
extern void *ADXT_GetObj(int32_t no);

extern void func_004c54d8(void);
extern void func_004c54e0(void);
extern void func_004c54a8(void);                               /* SVM lock */
extern void func_004c54c0(void);                               /* SVM unlock */
extern void func_004c6a98(const char *msg);                    /* ADXERR_CallErrFunc1 */
extern void func_004d5430(uint32_t adxt, uint8_t sw);          /* ainf_sw (+0xad) */
extern void ADXT_Destroy(void *adxt);
extern void ADXT_SetLnkSw(void *adxt, int32_t sw);
extern int32_t ADXSJD_GetNumChan(void *sjd);
extern void ADXERR_CallErrFunc1(const char *message);
extern void *SJRBF_Create(void *buf, int32_t bsize, int32_t xsize);
extern void *ADXSTM_Create(void *sj, int32_t mode);
extern void *ADXSJD_Create(void *sji, int32_t nch, void *sjo);
extern void *ADXRNA_Create(int32_t nch, void *sjo);
extern void *LSC_Create(void *sj);
extern void LSC_SetStmHndl(void *lsc, void *stm);
/* Retail keeps the default server frequency at 0x00724e6c; the port's ADXT_SetDefSvrFreq (bound to
 * 0x004d5440, config/native_names.txt) is the re4 body, which stores it here. */
extern int32_t adxt_def_svrfreq;

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

/* 0x004d38a8 (adxt_Create): set a free handle up inside `work` (64-byte aligned): maxnch output
 * rings of 0x2000 samples (0x2060 apart), then the file ring of whole sectors with 0x24 bytes behind
 * it; the file joint, its stream controller, the output joints, the decoder, the renderer and the
 * load scheduler; the default server frequency, the reload threshold at 85% of the file ring,
 * centred pans, loop switch and automatic recovery on, link switch off, ainf_sw on. NULL (with the
 * error text) for a bad argument or when all 16 handles are in use; a part that cannot be created
 * destroys the handle. */
void *adxt_Create(int32_t maxnch, void *work, int32_t worksize)
{
    uint32_t wk = (P4_H2G(work) + 0x3fu) & ~0x3fu;
    int32_t wksize = worksize - (int32_t)(wk - P4_H2G(work));
    int32_t obufbytes, ibufsize, i;
    uint8_t *adxt;

    if (maxnch < 0 || work == NULL || worksize < 0) {
        func_004c6a98((const char *)P4_G2H(0x00758b88u));
        return NULL;
    }
    for (i = 0; i < ADXT_MAX_OBJ; i++) {
        if (*(const int8_t *)ADXT_GetObj(i) == 0) {
            break;
        }
    }
    if (i == ADXT_MAX_OBJ) {
        func_004c6a98((const char *)P4_G2H(0x00758bb0u));
        return NULL;
    }
    adxt = (uint8_t *)ADXT_GetObj(i);
    memset(adxt, 0, ADXT_OBJ_SIZE);
    S8(adxt, 3) = (int8_t)maxnch;
    obufbytes = maxnch * 0x2060 * 2;
    P32(adxt, 0x2c) = wk;
    P32(adxt, 0x20) = wk + (uint32_t)obufbytes;
    ibufsize = (wksize - obufbytes - 0x124) / 0x800 * 0x800;
    S32(adxt, 0x24) = ibufsize;
    S32(adxt, 0x28) = 0x24;
    S32(adxt, 0x30) = 0x2000;
    S32(adxt, 0x34) = 0x2060;
    P32(adxt, 0xb0) = P32(adxt, 0x20) + (uint32_t)ibufsize + 0x24u;
    P32(adxt, 0x14) = 0;
    SETPP(adxt, 0x10, SJRBF_Create(PP(adxt, 0x20), ibufsize, 0x24));
    if (P32(adxt, 0x10) == 0) {
        goto fail;
    }
    SETPP(adxt, 8, ADXSTM_Create(PP(adxt, 0x10), 0));
    if (P32(adxt, 8) == 0) {
        goto fail;
    }
    for (i = 0; i < maxnch; i++) {
        SETPP(adxt, 0x18 + 4 * i,
              SJRBF_Create(P4_G2H(P32(adxt, 0x2c) + (uint32_t)(S32(adxt, 0x34) * i * 2)), S32(adxt, 0x30) * 2,
                           (S32(adxt, 0x34) - S32(adxt, 0x30)) * 2));
        if (P32(adxt, 0x18 + 4 * i) == 0) {
            goto fail;
        }
    }
    SETPP(adxt, 4, ADXSJD_Create(PP(adxt, 0x10), maxnch, adxt + 0x18));
    if (P32(adxt, 4) == 0) {
        goto fail;
    }
    /* Retail passes (joints, maxnch) to the PS2RNA create; the port's renderer front end
     * (re4/adx_rnaa.c -> AXRNA_Create) takes them as (nch, joints). */
    SETPP(adxt, 0x0c, ADXRNA_Create(maxnch, adxt + 0x18));
    if (P32(adxt, 0x0c) == 0) {
        goto fail;
    }
    SETPP(adxt, 0x94, LSC_Create(PP(adxt, 0x10)));
    if (P32(adxt, 0x94) == 0) {
        goto fail;
    }
    LSC_SetStmHndl(PP(adxt, 0x94), PP(adxt, 8));
    func_004c54a8();
    S16(adxt, 0x3c) = (int16_t)(ibufsize / 0x800);
    S16(adxt, 0x40) = 0;
    S32(adxt, 0x38) = adxt_def_svrfreq;
    S16(adxt, 0x3e) = (int16_t)(int32_t)(0x1.b33332p-1f * (float)S16(adxt, 0x3c)); /* retail 0x3f599999, just below 0.85f */
    for (i = 0; i < maxnch; i++) {
        S16(adxt, 0x42 + 2 * i) = -128;
    }
    S16(adxt, 0x46) = 0;
    S8(adxt, 0x6c) = 1;
    S32(adxt, 0x54) = 0;
    S32(adxt, 0x58) = 0;
    S32(adxt, 0x5c) = 0;
    S16(adxt, 0x60) = 0;
    S32(adxt, 0x64) = 0;
    S16(adxt, 0x68) = 0;
    S16(adxt, 0x6a) = 0;
    S8(adxt, 0x6d) = 1;
    S8(adxt, 0x72) = 0;
    S32(adxt, 0x88) = 0;
    ADXT_SetLnkSw(adxt, 0);
    func_004d5430(P4_H2G(adxt), 1);
    func_004c54c0();
    S8(adxt, 0) = 1;
    return adxt;
fail:
    ADXT_Destroy(adxt);
    return NULL;
}

/* 0x004d3850: adxt_Create under the ADX lock. */
void *func_004d3850(int32_t maxnch, void *work, int32_t worksize)
{
    void *adxt;

    func_004c54d8();
    adxt = adxt_Create(maxnch, work, worksize);
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
