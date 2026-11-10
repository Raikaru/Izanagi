/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* The ADXT handle server (adx_tsvr), translated from the retail asm (0x004d7320..0x004d8760). It
 * replaces decomp src/cri/re4/adx_tsvr.c (the GameCube library, config/excluded_units.txt), whose
 * server differs from the PS2 one:
 *   - retail splits the per-handle step into one function per state (adxt_ExecHndl 0x004d8698);
 *   - PLAYING -> PLAYEND_WAIT leaves the renderer's transfer on (re4 turns it off);
 *   - PLAYEND_WAIT keeps padding every output joint with up to maxdecsmpl samples of silence and
 *     counts them in flush_nsmpl (+0xa8, cleared by the decoder-information step); the handle ends
 *     when the joints' data plus the renderer's data minus that padding is no longer positive
 *     (re4 waits for an empty renderer instead);
 *   - PREP starts the renderer at min(2 * maxdecsmpl, 0x4000) samples;
 *   - a decoder error (ADXSJD state 4) during the decoder-information step is the handle's error
 *     state 6; the output volume goes through ADXT_SetOutVol (the stream's default volume only
 *     while ainf_sw is set); a loop switch turned off stops the decoder at the stream window +0xc4
 *     (ADXT_SetLpFlg), not at once.
 *
 * ADXT handle offsets: see decomp re4/adx_t.h (stride 0xc8). The debug words adxt_dbg_nch
 * (0x00724ebc), adxt_dbg_ndt (0x00724ec0) and adxt_dbg_rna_ndata (0x00724ec4) and the
 * decoder-information callback adxt_enddecinfo_cbfn (0x00724eb8, a guest code word) are the retail
 * data words.
 *
 * Retail's PLAYEND_WAIT -> PLAYEND step also sends the PS2RNA "through" switch to the IOP renderer
 * (0x004eca48(rna, 1)); the port's renderer (AXRNA, src/platform/sound/axrna.c) empties its ring
 * when the transfer and play switches are both off, so that call has no counterpart. */
#include <stdint.h>
#include <string.h>

#include "cri_mem.h"

#define ADXT_DBG_ENDDECINFO_CBFN 0x00724eb8u
#define ADXT_DBG_NCH 0x00724ebcu
#define ADXT_DBG_NDT 0x00724ec0u
#define ADXT_DBG_RNA_NDATA 0x00724ec4u
#define ADXT_VSYNC_CNT 0x007188b8u

#define SJ_LINE_FREE 0
#define SJ_LINE_DATA 1

extern void func_004c54a8(void);                                   /* SVM lock */
extern void func_004c54c0(void);                                   /* SVM unlock */
extern void func_004c6a98(const char *msg);                        /* ADXERR_CallErrFunc1 */
extern void func_004c6af0(const char *msg, const char *arg);       /* ADXERR_CallErrFunc2 */
extern void func_004c6c68(int32_t a, int32_t b, char *buf, int32_t len); /* ADXERR_ItoA2 */
extern int32_t func_004e9420(uint8_t *lsc);                        /* LSC_GetStat */
extern int32_t func_004cebf0(uint32_t sjd);                        /* ADXSJD_GetDecDtLen (+0x30) */
extern int32_t func_004cebf8(uint32_t sjd);                        /* ADXSJD_GetDecNumSmpl (+0x2c) */
extern void func_004d65b8(void *adxt, int32_t sw);                 /* ADXT_SetLnkSw */
extern int32_t ADX_DecodeFooter(const uint8_t *ibuf, int32_t ibuflen, int16_t *dlen);
extern int32_t ADX_ScanInfoCode(const uint8_t *ibuf, int32_t ibuflen, int16_t *dlen);
extern void SJ_SplitChunk(Ck *src, int32_t nbyte, Ck *a, Ck *b);

extern int32_t ADXSJD_GetStat(void *sjd);
extern int32_t ADXSJD_GetNumChan(void *sjd);
extern int32_t ADXSJD_GetSfreq(void *sjd);
extern int32_t ADXSJD_GetNumLoop(void *sjd);
extern int32_t ADXSJD_GetBlkSmpl(void *sjd);
extern void ADXSJD_SetMaxDecSmpl(void *sjd, int32_t nsmpl);
extern int32_t ADXSJD_GetLpEndOfst(void *sjd);
extern int32_t ADXSJD_GetLpEndPos(void *sjd);
extern int32_t ADXSJD_GetLpStartPos(void *sjd);
extern int32_t ADXSJD_GetLpStartOfst(void *sjd);
extern int32_t ADXSJD_GetTotalNumSmpl(void *sjd);
extern int32_t ADXSJD_GetOutBps(void *sjd);
extern int32_t ADXSJD_GetFormat(void *sjd);
extern void *ADXSJD_GetSpsdInfo(void *sjd);
extern void ADXSJD_SetTrapNumSmpl(void *sjd, int32_t nsmpl);
extern void ADXSJD_SetTrapDtLen(void *sjd, int32_t len);
extern void ADXSJD_SetTrapCnt(void *sjd, int32_t cnt);
extern void ADXSJD_EntryTrapFunc(void *sjd, void (*fn)(void *obj), void *obj);
extern void ADXSJD_SetDecPos(void *sjd, int32_t pos);
extern void ADXSJD_TakeSnapshot(void *sjd);
extern void ADXSJD_RestoreSnapshot(void *sjd);
extern void ADXSJD_Start(void *sjd);
extern void ADXSJD_Stop(void *sjd);
extern void ADXSJD_ExecHndl(void *sjd);
extern void ADXSJD_TermSupply(void *sjd);
extern int32_t ADXSTM_GetStat(void *stm);
extern void ADXSTM_SetEos(void *stm, int32_t nsct);
extern void ADXSTM_EntryEosFunc(void *stm, void (*fn)(void *obj), void *obj);
extern int32_t ADXSTM_Seek(void *stm, int32_t nsct);
extern int32_t ADXRNA_GetNumData(void *rna);
extern int32_t ADXRNA_GetNumRoom(void *rna);
extern void ADXRNA_SetTransSw(void *rna, int32_t sw);
extern void ADXRNA_SetPlaySw(void *rna, int32_t sw);
extern void ADXRNA_SetBitPerSmpl(void *rna, int32_t bps);
extern void ADXRNA_SetSfreq(void *rna, int32_t sfreq);
extern void ADXRNA_SetNumChan(void *rna, int32_t nch);
extern void ADXRNA_SetTotalNumSmpl(void *rna, int32_t nsmpl);
extern int32_t ADXRNA_SetStmHdInfo(void *rna, void *hdinfo);
extern void ADXAMP_SetSfreq(void *amp, int32_t sfreq);
extern int32_t ADXT_GetStat(void *adxt);
extern int32_t ADXT_GetNumChan(void *adxt);
extern void ADXT_Stop(void *adxt);
extern void ADXT_SetOutVol(void *adxt, int32_t vol);
extern void ADXT_SetOutPan(void *adxt, int32_t ch, int32_t pan);
extern void ADXT_GetTranspose(void *adxt, int32_t *oct, int32_t *cent);
extern void ADXT_SetTranspose(void *adxt, int32_t oct, int32_t cent);
extern void adxt_start_stm(void *adxt, void *fname, void *dir, int32_t ofst, int32_t nsct);

static void adxt_trap_entry(void *obj);
static void adxt_eos_entry(void *obj);
static void adxt_nlp_trap_entry(void *obj);

/* The output joints of the handle: +0x18, one guest address per channel. */
static void *adxt_sjo(uint8_t *adxt, int32_t ch)
{
    return PP(adxt, 0x18 + 4 * ch);
}

/* 0x004d7320: first trap at the loop start: keep the decoder state, arm the loop-end trap. */
static void adxt_trap_entry_lps(void *obj)
{
    uint8_t *adxt = obj;
    void *sjd = PP(adxt, 4);
    int32_t lpstart = ADXSJD_GetLpStartPos(sjd);
    int32_t lpstartofst = ADXSJD_GetLpStartOfst(sjd);
    int32_t lpend = ADXSJD_GetLpEndPos(sjd);

    ADXSJD_TakeSnapshot(sjd);
    ADXSJD_SetTrapCnt(sjd, 0);
    S32(adxt, 0x90) = lpend - lpstart;
    ADXSJD_SetTrapNumSmpl(sjd, lpend - lpstart);
    ADXSJD_SetTrapDtLen(sjd, lpstartofst);
    ADXSJD_SetDecPos(sjd, lpstart);
    ADXSJD_EntryTrapFunc(sjd, adxt_trap_entry, adxt);
}

/* 0x004d73d8: trap at the loop end: drop the input up to the end of the loop's last sector and
 * rewind the decoder to the loop start (a memory input, mode 2, rewinds its joint too); with the
 * loop switch off a memory or joint input (modes 2, 3) just lets the decoder run on. */
static void adxt_trap_entry(void *obj)
{
    uint8_t *adxt = obj;
    void *sjd = PP(adxt, 4);
    void *sji = PP(adxt, 0x14);
    int32_t lpstart = ADXSJD_GetLpStartPos(sjd);
    int32_t lpstartofst = ADXSJD_GetLpStartOfst(sjd);
    int32_t lpend = ADXSJD_GetLpEndPos(sjd);
    Ck ck;

    if ((uint8_t)(P8(adxt, 2) - 2) < 2 && S8(adxt, 0x6c) == 0) {
        ADXSJD_SetTrapNumSmpl(PP(adxt, 4), -1);
        return;
    }
    sj_get(sji, SJ_LINE_DATA, S32(adxt, 0x50), &ck);
    if (ck.len < S32(adxt, 0x50)) {
        func_004c6a98((const char *)P4_G2H(0x00759460u));
    }
    sj_put(sji, SJ_LINE_FREE, &ck);
    ADXSJD_SetTrapCnt(sjd, 0);
    S32(adxt, 0x90) = lpend - lpstart;
    ADXSJD_SetTrapNumSmpl(sjd, lpend - lpstart);
    ADXSJD_SetTrapDtLen(sjd, lpstartofst);
    ADXSJD_SetDecPos(sjd, lpstart);
    if (S8(adxt, 2) == 2) {
        sj_reset(sji);
        sj_get(sji, SJ_LINE_DATA, lpstartofst, &ck);
        sj_put(sji, SJ_LINE_FREE, &ck);
    }
    ADXSJD_RestoreSnapshot(sjd);
    S32(adxt, 0x4c) += 1;
}

/* 0x004d7570: stream end-of-sector callback of a looping file: seek back to the loop start sector;
 * with the loop switch off, stop the decoder once it has read the stream window (+0xc4) and let the
 * stream run to the file's end. */
static void adxt_eos_entry(void *obj)
{
    uint8_t *adxt = obj;
    void *stm = PP(adxt, 8);
    int32_t ofst;

    if (stm == NULL || PP(adxt, 4) == NULL) {
        return;
    }
    ofst = ADXSJD_GetLpStartOfst(PP(adxt, 4));
    if (S8(adxt, 0x6c) != 0) {
        ADXSTM_Seek(stm, ofst / 0x800);
        return;
    }
    if (func_004cebf0(P32(adxt, 4)) >= S32(adxt, 0xc4)) {
        ADXSJD_SetTrapNumSmpl(PP(adxt, 4), -1);
    }
    ADXSTM_SetEos(PP(adxt, 8), 0x7fffffff);
}

/* 0x004d7618: the output pan of each channel the stream has. */
static void adxt_set_outpan(uint8_t *adxt)
{
    if (ADXSJD_GetNumChan(PP(adxt, 4)) == 1) {
        ADXT_SetOutPan(adxt, 0, S16(adxt, 0x42));
        return;
    }
    ADXT_SetOutPan(adxt, 0, S16(adxt, 0x42));
    ADXT_SetOutPan(adxt, 1, S16(adxt, 0x44));
}

/* 0x004d7680: non-looping end trap. With the link switch on, look for a following ADX file behind
 * the footer (its info code), drop everything before it and restart the decoder on it. */
static void adxt_nlp_trap_entry(void *obj)
{
    uint8_t *adxt = obj;
    void *sjd = PP(adxt, 4);
    void *sji = PP(adxt, 0x14);
    Ck ck, ck2, ck3, ck4;
    int16_t ofst, ofst2 = 0;
    int32_t skip, found1, found2 = -1;

    if (S8(adxt, 0x98) == 0) {
        return;
    }
    sj_get(sji, SJ_LINE_DATA, 0x7fffffff, &ck);
    sj_get(sji, SJ_LINE_DATA, 0x7fffffff, &ck2);
    if (ADX_DecodeFooter((const uint8_t *)P4_G2H(ck.data), ck.len, &ofst) != 0) {
        func_004d65b8(adxt, 0);
        sj_unget(sji, SJ_LINE_DATA, &ck2);
        sj_unget(sji, SJ_LINE_DATA, &ck);
        return;
    }
    skip = ofst;
    found1 = ADX_ScanInfoCode((const uint8_t *)P4_G2H(ck.data) + skip, ck.len - skip, &ofst);
    if (found1 != 0) {
        found2 = ADX_ScanInfoCode((const uint8_t *)P4_G2H(ck2.data), ck2.len, &ofst2);
    }
    skip += ofst;
    if (found1 != 0 && found2 != 0) {
        sj_unget(sji, SJ_LINE_DATA, &ck2);
        sj_unget(sji, SJ_LINE_DATA, &ck);
        func_004d65b8(adxt, 0);
        return;
    }
    if (found1 == 0) {
        sj_unget(sji, SJ_LINE_DATA, &ck2);
        SJ_SplitChunk(&ck, skip, &ck, &ck3);
        sj_put(sji, SJ_LINE_FREE, &ck);
        sj_unget(sji, SJ_LINE_DATA, &ck3);
    } else {
        sj_put(sji, SJ_LINE_FREE, &ck);
        SJ_SplitChunk(&ck2, ofst2, &ck2, &ck4);
        sj_put(sji, SJ_LINE_FREE, &ck2);
        sj_unget(sji, SJ_LINE_DATA, &ck4);
    }
    S32(adxt, 0xa4) += func_004cebf8(P32(adxt, 4));
    ADXSJD_Stop(sjd);
    ADXSJD_Start(sjd);
    ADXSJD_ExecHndl(sjd);
    if (ADXSJD_GetStat(sjd) != 2) {
        func_004d65b8(adxt, 0);
        return;
    }
    ADXSJD_SetMaxDecSmpl(sjd, S32(adxt, 0x48));
    ADXSJD_SetTrapNumSmpl(sjd, ADXSJD_GetTotalNumSmpl(sjd));
    ADXSJD_SetTrapDtLen(sjd, 0);
    ADXSJD_SetTrapCnt(sjd, 0);
}

/* 0x004d7968: state DECINFO. Starts a pending file stream once the stream is idle; when the
 * decoder has read the header, sizes the decode step (3 server periods of samples for a looping
 * stream, 1.5 otherwise, rounded up to two blocks), arms the loop or end trap, programs the
 * renderer and goes to PREP. */
static void adxt_stat_decinfo(uint8_t *adxt)
{
    void *sjd = PP(adxt, 4);
    void *rna;
    int32_t oct = 0, cent = 0;
    int32_t stat, nch, sfreq, nloop, blk, nsmpl;
    void (*cbfn)(void *adxt, int32_t sfreq, int32_t nch, int32_t nsmpl);
    char buf[16];

    S32(adxt, 0xa8) = 0;
    if (P8(adxt, 2) < 2 && S8(adxt, 0xac) == 1) {
        if (ADXSTM_GetStat(PP(adxt, 8)) == 2) {
            return;
        }
        if (PP(adxt, 0x10) != NULL) {
            sj_reset(PP(adxt, 0x10));
        }
        adxt_start_stm(adxt, PP(adxt, 0xb4), PP(adxt, 0xb8), S32(adxt, 0xbc), S32(adxt, 0xc0));
        S8(adxt, 0xac) = 0;
    }
    stat = ADXSJD_GetStat(sjd);
    if (stat != 2) {
        if (stat == 4) {
            S8(adxt, 1) = 6;
        }
        return;
    }
    nch = ADXSJD_GetNumChan(sjd);
    if (S8(adxt, 3) < nch) {
        func_004c6c68(nch, S8(adxt, 3), buf, 16);
        func_004c6af0((const char *)P4_G2H(0x00759490u), buf);
        ADXT_Stop(adxt);
        return;
    }
    sfreq = ADXSJD_GetSfreq(sjd);
    nloop = ADXSJD_GetNumLoop(sjd);
    if (nloop > 0) {
        S32(adxt, 0x48) = sfreq / S32(adxt, 0x38) * 3;
    } else {
        S32(adxt, 0x48) = sfreq / S32(adxt, 0x38) * 3 / 2;
    }
    blk = ADXSJD_GetBlkSmpl(sjd) * 2;
    S32(adxt, 0x48) = (S32(adxt, 0x48) + blk) / blk * blk;
    ADXSJD_SetMaxDecSmpl(sjd, S32(adxt, 0x48));
    if (nloop > 0) {
        if (S8(adxt, 2) == 2) {
            S32(adxt, 0x50) = 0;
        } else {
            int32_t lpendofst = ADXSJD_GetLpEndOfst(sjd);
            int32_t lpendsct = (lpendofst + 0x7ff) / 0x800;

            S32(adxt, 0x50) = (0x800 - lpendofst % 0x800) % 0x800;
            S32(adxt, 0x8c) = lpendsct;
            ADXSTM_SetEos(PP(adxt, 8), lpendsct);
            ADXSTM_EntryEosFunc(PP(adxt, 8), adxt_eos_entry, adxt);
        }
        (void)ADXSJD_GetLpEndPos(sjd);
        S32(adxt, 0x90) = ADXSJD_GetLpStartPos(sjd);
        ADXSJD_SetTrapNumSmpl(sjd, S32(adxt, 0x90));
        ADXSJD_SetTrapDtLen(sjd, 0);
        ADXSJD_SetTrapCnt(sjd, 0);
        ADXSJD_EntryTrapFunc(sjd, adxt_trap_entry_lps, adxt);
    } else {
        if (PP(adxt, 8) != NULL) {
            ADXSTM_SetEos(PP(adxt, 8), 0x7fffffff);
        }
        ADXSJD_SetTrapNumSmpl(sjd, ADXSJD_GetTotalNumSmpl(sjd));
        ADXSJD_SetTrapDtLen(sjd, 0);
        ADXSJD_SetTrapCnt(sjd, 0);
        ADXSJD_EntryTrapFunc(sjd, adxt_nlp_trap_entry, adxt);
    }
    sfreq = ADXSJD_GetSfreq(sjd);
    nch = ADXSJD_GetNumChan(sjd);
    nsmpl = ADXSJD_GetTotalNumSmpl(sjd);
    rna = PP(adxt, 0x0c);
    ADXRNA_SetBitPerSmpl(rna, ADXSJD_GetOutBps(sjd));
    ADXRNA_SetSfreq(PP(adxt, 0x0c), sfreq);
    ADXRNA_SetNumChan(PP(adxt, 0x0c), nch);
    ADXRNA_SetTotalNumSmpl(PP(adxt, 0x0c), nsmpl);
    ADXT_SetOutVol(adxt, S16(adxt, 0x40));
    ADXT_GetTranspose(adxt, &oct, &cent);
    if (oct != 0 || cent != 0) {
        ADXT_SetTranspose(adxt, oct, cent);
    }
    adxt_set_outpan(adxt);
    if (PP(adxt, 0x74) != NULL) {
        ADXAMP_SetSfreq(PP(adxt, 0x74), sfreq);
    }
    if (ADXSJD_GetFormat(sjd) == 2) {
        ADXRNA_SetStmHdInfo(PP(adxt, 0x0c), ADXSJD_GetSpsdInfo(sjd));
    }
    ADXRNA_SetTransSw(PP(adxt, 0x0c), 1);
    if (M32(ADXT_DBG_ENDDECINFO_CBFN) != 0) {
        cbfn = (void (*)(void *, int32_t, int32_t, int32_t))p4_fn_g2h_sig(M32(ADXT_DBG_ENDDECINFO_CBFN),
                                                                           P4_SIG(4, 0, 1, 0, 0, 0, 0, 0));
        cbfn(adxt, sfreq, nch, nsmpl);
    }
    S8(adxt, 1) = 2;
}

/* 0x004d7d90: state PREP. Starts the renderer once it holds min(2 * maxdecsmpl, 0x4000) samples,
 * has no more room than one block, or the decoder has ended; once the decoder has ended, pads each
 * output joint with maxdecsmpl samples of silence. A paused handle (+0x70) waits in PREP. */
static void adxt_stat_prep(uint8_t *adxt)
{
    void *rna = PP(adxt, 0x0c);
    void *sjd = PP(adxt, 4);
    int32_t ndata = ADXRNA_GetNumData(rna);
    int32_t nroom = ADXRNA_GetNumRoom(rna);
    int32_t start = S32(adxt, 0x48) * 2;

    if (start > 0x4000) {
        start = 0x4000;
    }
    if (ndata >= start || ADXSJD_GetBlkSmpl(sjd) >= nroom || ADXSJD_GetStat(PP(adxt, 4)) == 3) {
        if (S8(adxt, 0x70) == 0) {
            if (S8(adxt, 0x72) == 0) {
                ADXRNA_SetPlaySw(rna, 1);
                P32(adxt, 0x9c) = 0;
                S32(adxt, 0xa0) = M32(ADXT_VSYNC_CNT);
            }
            S8(adxt, 1) = 3;
        }
        S8(adxt, 0x71) = 1;
    }
    if (ADXSJD_GetStat(PP(adxt, 4)) == 3) {
        int32_t nch = ADXT_GetNumChan(adxt);
        int32_t nbyte = S32(adxt, 0x48) * nch * 2;
        int32_t ch;

        for (ch = 0; ch < nch; ch++) {
            void *sj = adxt_sjo(adxt, ch);
            Ck ck;

            sj_get(sj, SJ_LINE_FREE, nbyte, &ck);
            memset(P4_G2H(ck.data), 0, (uint32_t)ck.len);
            sj_put(sj, SJ_LINE_DATA, &ck);
        }
    }
}

/* 0x004d7f18: state PLAYING. With the loop switch off, stops the decoder once it has read the
 * stream window (+0xc4); once the decoder has ended and every output joint holds less than 64 bytes,
 * goes to PLAYEND_WAIT (the renderer keeps transferring). */
static void adxt_stat_playing(uint8_t *adxt)
{
    int32_t nch, ch;

    if (S8(adxt, 0x6c) == 0 && func_004cebf0(P32(adxt, 4)) >= S32(adxt, 0xc4)) {
        ADXSJD_SetTrapNumSmpl(PP(adxt, 4), -1);
    }
    if (ADXSJD_GetStat(PP(adxt, 4)) != 3) {
        return;
    }
    nch = ADXSJD_GetNumChan(PP(adxt, 4));
    M32(ADXT_DBG_NCH) = (uint32_t)nch;
    for (ch = 0; ch < nch; ch++) {
        int32_t ndt = sj_num(adxt_sjo(adxt, ch), SJ_LINE_DATA);

        M32(ADXT_DBG_NDT) = (uint32_t)ndt;
        if (ndt >= 0x40) {
            break;
        }
    }
    if (ch == nch) {
        S8(adxt, 1) = 4;
    }
}

/* 0x004d8000: state PLAYEND_WAIT. Under the SVM lock, pads every output joint with the same number
 * of silent samples (at most maxdecsmpl, at most what every joint has room for) and adds it to
 * flush_nsmpl. The handle has played its stream when the most data any joint holds plus the
 * renderer's buffered samples, minus everything padded, is no longer positive: then the renderer
 * stops (transfer and play off) and the state is PLAYEND. */
static void adxt_stat_playend_wait(uint8_t *adxt)
{
    Ck ck[2], rest[2];
    int32_t nch, ch, n, maxdata;

    M32(ADXT_DBG_RNA_NDATA) = (uint32_t)ADXRNA_GetNumData(PP(adxt, 0x0c));
    nch = ADXSJD_GetNumChan(PP(adxt, 4));
    func_004c54a8();
    n = S32(adxt, 0x48);
    for (ch = 0; ch < nch; ch++) {
        sj_get(adxt_sjo(adxt, ch), SJ_LINE_FREE, n * 2, &ck[ch]);
        if ((uint32_t)ck[ch].len >> 1 < (uint32_t)n) {
            n = (int32_t)((uint32_t)ck[ch].len >> 1);
        }
    }
    for (ch = 0; ch < nch; ch++) {
        void *sj = adxt_sjo(adxt, ch);

        SJ_SplitChunk(&ck[ch], n * 2, &ck[ch], &rest[ch]);
        memset(P4_G2H(ck[ch].data), 0, (uint32_t)ck[ch].len);
        sj_put(sj, SJ_LINE_DATA, &ck[ch]);
        sj_unget(sj, SJ_LINE_FREE, &rest[ch]);
    }
    S32(adxt, 0xa8) += n;
    func_004c54c0();
    maxdata = 0;
    for (ch = 0; ch < nch; ch++) {
        int32_t ndata = (int32_t)((uint32_t)sj_num(adxt_sjo(adxt, ch), SJ_LINE_DATA) >> 1);

        if (ndata >= maxdata) {
            maxdata = ndata;
        }
    }
    if (maxdata + ADXRNA_GetNumData(PP(adxt, 0x0c)) - S32(adxt, 0xa8) <= 0) {
        ADXRNA_SetTransSw(PP(adxt, 0x0c), 0);
        ADXRNA_SetPlaySw(PP(adxt, 0x0c), 0);
        S8(adxt, 1) = 5;
    }
}

/* 0x004d85d0: once the handle has started, ends the decoder's supply when a file stream (modes 0,
 * 1) has read its last sector or for a memory input (mode 2); a joint input or the load scheduler
 * (modes 3, 4) supply until stopped. */
static void adxt_term_supply(uint8_t *adxt)
{
    if (PP(adxt, 8) == NULL || ADXT_GetStat(adxt) == 0) {
        return;
    }
    switch (P8(adxt, 2)) {
    case 0:
    case 1:
        if (ADXSTM_GetStat(PP(adxt, 8)) == 3) {
            ADXSJD_TermSupply(PP(adxt, 4));
        }
        break;
    case 2:
        ADXSJD_TermSupply(PP(adxt, 4));
        break;
    default:
        break;
    }
}

/* 0x004d8550: a stream read error (ADXSTM state 4) or a load scheduler error (LSC state 3) is the
 * handle's error state 6 with error code -1. */
void ADXT_ExecRdErrChk(void *obj)
{
    uint8_t *adxt = obj;

    if (PP(adxt, 8) != NULL && ADXSTM_GetStat(PP(adxt, 8)) == 4) {
        S16(adxt, 0x60) = -1;
        S8(adxt, 1) = 6;
    }
    if (PP(adxt, 0x94) != NULL && func_004e9420(PP(adxt, 0x94)) == 3) {
        S8(adxt, 1) = 6;
        S16(adxt, 0x60) = -1;
    }
}

/* 0x004d8698 (adxt_ExecHndl): one server step of a handle: the step of its state (PLAYEND, state
 * 5, has none), the decoder supply and the read error check. */
void ADXT_ExecHndl(void *obj)
{
    uint8_t *adxt = obj;

    if (adxt == NULL) {
        func_004c6a98((const char *)P4_G2H(0x007594e8u));
        return;
    }
    switch (S8(adxt, 1)) {
    case 3:
        adxt_stat_playing(adxt);
        break;
    case 1:
        adxt_stat_decinfo(adxt);
        break;
    case 2:
        adxt_stat_prep(adxt);
        break;
    case 4:
        adxt_stat_playend_wait(adxt);
        break;
    default:
        break;
    }
    adxt_term_supply(adxt);
    ADXT_ExecRdErrChk(adxt);
}
