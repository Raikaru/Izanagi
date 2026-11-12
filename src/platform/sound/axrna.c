/* AXRNA: the audio renderer under CRI's ADXRNA front end, backed by the mixer.
 *
 * On the PS2, ADXT's decoded PCM leaves the EE through PS2RNA, which DMAs it to an SPU2 ring and
 * plays it from there (an IOP transfer behind the DTX RPC). The decomp's compiled ADXRNA_*
 * wrappers (src/cri/re4/adx_rnaa.c) call the AXRNA_* functions defined here, so this file is the
 * hand-off: instead of a transfer to the SPU2, AXRNA_ExecServer moves the PCM from the decoder's
 * output stream joints into a stream voice of the mixer (src/platform/sound/p4snd.h).
 *
 * CRI's ADXT server (src/cri/adxt_svr.c, translated from the retail asm) sees the same quantities
 * as with the SPU2 ring:
 *   - GetNumData: samples pushed and not yet played, as PS2RNA reports them. PS2RNA's figure is
 *     its IOP buffer as the EE last heard of it: (0x4000 bytes - the free space the IOP returned)
 *     / 2 (PS2RNA_GetNumData, retail 0x004ecb40), at most 0x2000 samples. The IOP returns played
 *     chunks through SJX/DTX once per server pass. Its side is not in the decomp, so the report is
 *     modelled from retail's measured streams (collision 900: 87 voices and 3139 per-pass reports,
 *     all reproduced but one sampling artefact). The IOP consumes a stream in blocks of 512 SPU2
 *     output samples (93.75 per second; 256 samples of a 24 kHz stream) on a clock that runs from
 *     boot. Retail's block ends fall at tick time t(c) = (2c + 203) * 93.75 / 59.94 - 0.93675
 *     integer, where c is ADXT's server-pass count adxt_vsync_cnt (0x007188b8). It counts every
 *     second VBLANK from boot (vsync = 2c + 203 throughout the run), and PCSX2's VBLANK rate is
 *     59.94 Hz. A stream keyed on in the pass with count c is first heard consumed at the
 *     first block end after t(c) + 6.2525. So the report's start lands 6 or 7 blocks after
 *     key-on, by phase, and a voice of a given length can end a pass apart: collision 900's
 *     cue 0x116 (39989 samples) ended 53 passes after key-on at count 8811 and 54 at count 13600.
 *     Here the mixer plays at once. Each pass reports whole blocks counted from that start, never
 *     more than was pushed. So ADXT's start test
 *     (`ndata >= min(maxdecsmpl * 2, 0x4000)` or `nroom <= blksmpl`), PLAYEND_WAIT -> PLAYEND
 *     (joint data + `ndata` - flush_nsmpl <= 0) and ADXT_GetTime see what retail's EE sees, given
 *     the same pass count (lockstep transplants it at the anchor).
 *   - GetNumRoom: AXRNA_IOP_FRAMES (0x2000) minus GetNumData; the server never transfers more.
 *     The mixer ring (AXRNA_RING_FRAMES) is larger, to also hold the samples played but not yet
 *     reported.
 *   - SetTransSw: the server pulls from the joints only while it is on.
 *   - SetPlaySw: the voice runs while it is on. Turning play off with the transfer already off
 *     is a stop (ADXT_Stop, the end of a stream) and empties the ring; with the transfer on it
 *     is a pause and keeps the data.
 *
 * Input: one mono signed 16-bit stream joint per channel (ADXT's `sjo`). Joints are SJ_IF
 * objects (first word: the interface table; decomp src/cri/re4/sj.h). Line 1 (data) holds the
 * decoded samples; a consumed chunk goes back to line 0 (free).
 *
 * Units: volume is 1/10 dB (0 = full), pan -15..15 per channel (ADXT_SetOutPan). */
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "p4snd.h"
#include "p4pc/gmem.h"
#include "../../cri/cri_mem.h"

#define AXRNA_MAX_OBJ 16
#define AXRNA_RING_FRAMES 0x4000
#define AXRNA_IOP_FRAMES 0x2000   /* PS2RNA's IOP buffer: 0x4000 bytes of 16-bit samples per channel */
#define ADXT_VSYNC_CNT 0x007188b8u /* adxt_vsync_cnt: ADXM_ExecVint counts each server pass */
#define AXRNA_TICKS_PER_VBLANK (93.75 / 59.94) /* IOP blocks (512 SPU2 output samples) per PCSX2 VBLANK */
#define AXRNA_TICK_PHASE 0.93675  /* where retail's block ends fall (measured) */
#define AXRNA_START_TICKS 6.2525  /* key-on to the first block end heard (measured) */
#define AXRNA_PASS_JUMP 64        /* more passes than this between reports is a clock change, not a lag */
#define SJ_CK_FREE 0
#define SJ_CK_DATA 1

/* The renderer object is handed to the game (it keeps the pointer in its ADXT), so it lives in port-owned guest
 * memory, not in a host static. Everything the game could read in it is a guest address (the joints, the stream
 * header info); the voice is a mixer object only this file touches. A joint is an SJ_IF object (first word: the
 * interface table of guest code addresses), called through the cri_mem.h helpers. */
typedef struct AXRNA_OBJ {
    int32_t used;
    int32_t maxnch; /* channels with a joint and a ring */
    int32_t nch;    /* channels in use (SetNumChan) */
    uint32_t sj[P4SND_MAX_CH]; /* guest addresses of the stream joints */
    P4SndVoice *voice;         /* host mixer object, never given to the game */
    int32_t adjsfreq;
    uint32_t stmhd; /* guest address */
    int32_t bps;
    int32_t sfreq;
    int32_t vol;
    int32_t pan[P4SND_MAX_CH];
    int32_t play_sw;
    int32_t trans_sw;
    uint32_t heard;       /* consumption the EE has heard of, in the mixer's write count */
    uint32_t heard_start; /* heard when the current play run started */
    int64_t start_tick;   /* the last block end before the run's first counted one */
    uint32_t last_cnt;    /* the pass count of the last report */
} AXRNA_OBJ;

static AXRNA_OBJ *axrna_objs;
static int32_t axrna_init_cnt;
static void (*axrna_err_func)(void *obj, char *msg);
static void *axrna_err_obj;

static AXRNA_OBJ *axrna_table(void)
{
    if (axrna_objs == NULL) {
        axrna_objs = (AXRNA_OBJ *)P4_G2H(p4_gport_alloc(sizeof(AXRNA_OBJ) * AXRNA_MAX_OBJ, 16));
    }
    return axrna_objs;
}

#define AXRNA_JOINT(rna, ch) ((void *)P4_G2H((rna)->sj[ch]))

static void axrna_err(const char *msg)
{
    if (axrna_err_func != NULL) {
        axrna_err_func(axrna_err_obj, (char *)msg);
    }
}

/* The IOP's block clock at server-pass count `cnt` (see the header). */
static double axrna_tick_time(uint32_t cnt)
{
    return (2.0 * cnt + 203.0) * AXRNA_TICKS_PER_VBLANK - AXRNA_TICK_PHASE;
}

static uint32_t axrna_pass_count(void)
{
    return *(const uint32_t *)P4_G2H(ADXT_VSYNC_CNT);
}

/* Samples pushed into the voice so far: the mixer's write count. */
static uint32_t axrna_written(const AXRNA_OBJ *rna)
{
    return p4snd_stream_consumed(rna->voice) + p4snd_stream_data(rna->voice);
}

/* Samples PS2RNA would report buffered: pushed minus heard consumed. */
static int32_t axrna_data(const AXRNA_OBJ *rna)
{
    return (int32_t)(axrna_written(rna) - rna->heard);
}

/* A new or stopped voice: the IOP buffer starts empty where the ring is. */
static void axrna_sync(AXRNA_OBJ *rna)
{
    rna->heard = axrna_written(rna);
    rna->heard_start = rna->heard;
}

/* Play starts (key-on or resume) in the pass with count `cnt`: blocks are heard from the first
 * block end after t(cnt) + AXRNA_START_TICKS. */
static void axrna_start_run(AXRNA_OBJ *rna, uint32_t cnt)
{
    rna->heard_start = rna->heard;
    rna->start_tick = (int64_t)floor(axrna_tick_time(cnt) + AXRNA_START_TICKS);
}

/* The pass's report: whole blocks since the run's start, at the stream's rate (512 SPU2 output
 * samples per block), never more than was pushed. */
static void axrna_report(AXRNA_OBJ *rna, uint32_t cnt)
{
    int64_t blocks = (int64_t)floor(axrna_tick_time(cnt)) - rna->start_tick;
    uint32_t written = axrna_written(rna);
    uint32_t heard;

    if (!rna->play_sw || blocks <= 0) {
        return;
    }
    heard = rna->heard_start + (uint32_t)((uint64_t)blocks * (uint32_t)rna->sfreq * 512u / 48000u);
    if ((int32_t)(heard - written) > 0) {
        heard = written;
    }
    if ((int32_t)(heard - rna->heard) > 0) {
        rna->heard = heard;
    }
}

static void axrna_apply_gains(AXRNA_OBJ *rna)
{
    int ch;

    if (rna->voice == NULL) {
        return;
    }
    for (ch = 0; ch < rna->maxnch; ch++) {
        float l = 0.0f;
        float r = 0.0f;

        if (ch < rna->nch) {
            p4snd_cri_gains(rna->vol, rna->pan[ch], &l, &r);
        }
        p4snd_voice_set_gain(rna->voice, ch, l, r);
    }
}

void AXRNA_Init(void)
{
    if (axrna_init_cnt == 0) {
        memset(axrna_table(), 0, sizeof(AXRNA_OBJ) * AXRNA_MAX_OBJ);
    }
    axrna_init_cnt++;
}

void AXRNA_Finish(void)
{
    int32_t i;

    if (axrna_init_cnt > 0 && --axrna_init_cnt == 0) {
        AXRNA_OBJ *tab = axrna_table();

        for (i = 0; i < AXRNA_MAX_OBJ; i++) {
            if (tab[i].voice != NULL) {
                p4snd_voice_destroy(tab[i].voice);
            }
        }
        memset(tab, 0, sizeof(AXRNA_OBJ) * AXRNA_MAX_OBJ);
    }
}

void AXRNA_EntryErrFunc(void (*func)(void *obj, char *msg), void *obj)
{
    axrna_err_func = func;
    axrna_err_obj = obj;
}

/* The decomp's ADXRNA_Create front end (re4/adx_rnaa.c) declares (nch, sj) but is called by
 * ADXT_Create with (sjo, maxnch, work), so the two arguments currently arrive as (joint array,
 * channel count). Both are 32-bit integers: the channel count is the small one and the joint array
 * a guest address (an array of guest addresses of joints); either order is read the same way. */
AXRNA_OBJ *AXRNA_Create(uint32_t arg0, uint32_t arg1)
{
    AXRNA_OBJ *tab = axrna_table();
    AXRNA_OBJ *rna = NULL;
    int32_t nch;
    const uint32_t *sjo;
    int32_t i;

    if (arg0 < 0x1000u) {
        nch = (int32_t)arg0;
        sjo = (const uint32_t *)P4_G2H(arg1);
    } else {
        nch = (int32_t)arg1;
        sjo = (const uint32_t *)P4_G2H(arg0);
    }
    if (nch < 1 || nch > P4SND_MAX_CH || sjo == NULL) {
        axrna_err("E02080801 AXRNA_Create: parameter error");
        return NULL;
    }
    for (i = 0; i < AXRNA_MAX_OBJ; i++) {
        if (tab[i].used == 0) {
            rna = &tab[i];
            break;
        }
    }
    if (rna == NULL) {
        axrna_err("E02080802 AXRNA_Create: no free handle");
        return NULL;
    }
    memset(rna, 0, sizeof(*rna));
    rna->voice = p4snd_stream_create(nch, AXRNA_RING_FRAMES);
    if (rna->voice == NULL) {
        axrna_err("E02080803 AXRNA_Create: no free voice");
        return NULL;
    }
    rna->used = 1;
    rna->maxnch = nch;
    rna->nch = nch;
    rna->bps = 16;
    rna->sfreq = 44100;
    for (i = 0; i < nch; i++) {
        rna->sj[i] = sjo[i];
        rna->pan[i] = nch == 2 ? (i == 0 ? -15 : 15) : 0;
    }
    p4snd_voice_set_rate(rna->voice, rna->sfreq);
    axrna_apply_gains(rna);
    axrna_sync(rna);
    return rna;
}

int p4snd_adxt_set_category(uint32_t adxt, int category)
{
    const uint32_t size = (uint32_t)sizeof(AXRNA_OBJ);
    uint32_t base;
    uint32_t addr;
    AXRNA_OBJ *rna;

    /* ADXT keeps its renderer at +0x0c (decomp adx_tlk.c, adxt->rna). The caller has checked
     * that word is readable guest memory; only a word naming one of this file's live renderer
     * objects is followed, anything else is refused. */
    if (axrna_objs == NULL || adxt == 0 || (adxt & 3u)) {
        return -1;
    }
    base = P4_H2G(axrna_objs);
    addr = P4_G32(adxt + 0x0cu);
    if (addr < base || addr - base >= size * AXRNA_MAX_OBJ || (addr - base) % size) {
        return -1;
    }
    rna = &axrna_objs[(addr - base) / size];
    if (!rna->used || rna->voice == NULL) {
        return -1;
    }
    p4snd_voice_set_category(rna->voice, category);
    return 0;
}

void AXRNA_Destroy(AXRNA_OBJ *rna)
{
    if (rna != NULL && rna->used) {
        p4snd_voice_destroy(rna->voice);
        memset(rna, 0, sizeof(*rna));
    }
}

/* Moves decoded PCM from the joints into the voice ring, at most what PS2RNA would report free.
 * Every channel moves the same count. */
static void axrna_transfer(AXRNA_OBJ *rna)
{
    int32_t room = AXRNA_IOP_FRAMES - axrna_data(rna);
    int32_t n = (int32_t)p4snd_stream_room(rna->voice);
    int32_t ch;

    if (room < n) {
        n = room;
    }
    for (ch = 0; ch < rna->nch; ch++) {
        int32_t have = sj_num(AXRNA_JOINT(rna, ch), SJ_CK_DATA) >> 1;

        if (have < n) {
            n = have;
        }
    }
    if (n <= 0) {
        return;
    }
    for (ch = 0; ch < rna->nch; ch++) {
        void *sj = AXRNA_JOINT(rna, ch);
        int32_t done = 0;

        while (done < n) {
            Ck ck;

            sj_get(sj, SJ_CK_DATA, (n - done) * 2, &ck);
            if (ck.len <= 0) {
                break;
            }
            p4snd_stream_write(rna->voice, ch, (uint32_t)done, (const int16_t *)P4_G2H(ck.data),
                               (uint32_t)(ck.len >> 1));
            done += ck.len >> 1;
            sj_put(sj, SJ_CK_FREE, &ck);
        }
        if (done != n) {
            axrna_err("E02080804 AXRNA_ExecServer: short chunk");
        }
    }
    p4snd_stream_commit(rna->voice, (uint32_t)n);
}

void AXRNA_ExecServer(void)
{
    int32_t i;
    AXRNA_OBJ *tab = axrna_table();
    uint32_t cnt = axrna_pass_count();

    /* Each pass first takes the IOP's report, then fills the buffer up to it: retail's EE reports
     * the buffer full (0x2000 samples) after a pass once the joints keep up. */
    for (i = 0; i < AXRNA_MAX_OBJ; i++) {
        AXRNA_OBJ *rna = &tab[i];
        int32_t step;

        if (!rna->used) {
            continue;
        }
        /* A pass count that jumps (lockstep sets it to retail's at the anchor) moves the block
         * clock: a running stream keeps what it was heard to have played and counts on from
         * the new time. */
        step = (int32_t)(cnt - rna->last_cnt);
        if (rna->play_sw && (step < 0 || step > AXRNA_PASS_JUMP)) {
            rna->heard_start = rna->heard;
            rna->start_tick = (int64_t)floor(axrna_tick_time(cnt));
        }
        rna->last_cnt = cnt;
        axrna_report(rna, cnt);
        if (rna->trans_sw) {
            axrna_transfer(rna);
        }
    }
}

void AXRNA_SetAdjsfreqFlg(AXRNA_OBJ *rna, int32_t flg)
{
    rna->adjsfreq = flg;
}

void AXRNA_SetStmHdInfo(AXRNA_OBJ *rna, void *hdinfo)
{
    rna->stmhd = P4_H2G(hdinfo);
}

void AXRNA_DiscardData(AXRNA_OBJ *rna, int32_t nsmpl)
{
    if (nsmpl > 0) {
        uint32_t n = p4snd_stream_discard(rna->voice, (uint32_t)nsmpl);

        /* The discarded samples leave the report at once. */
        rna->heard += n;
        rna->heard_start += n;
    }
}

void AXRNA_SetBitPerSmpl(AXRNA_OBJ *rna, int32_t bps)
{
    rna->bps = bps;
}

void AXRNA_SetOutPan(AXRNA_OBJ *rna, int32_t ch, int32_t pan)
{
    if ((uint32_t)ch < P4SND_MAX_CH) {
        rna->pan[ch] = pan;
        axrna_apply_gains(rna);
    }
}

void AXRNA_SetOutVol(AXRNA_OBJ *rna, int32_t vol)
{
    rna->vol = vol;
    axrna_apply_gains(rna);
}

void AXRNA_SetSfreq(AXRNA_OBJ *rna, int32_t sfreq)
{
    rna->sfreq = sfreq;
    p4snd_voice_set_rate(rna->voice, sfreq);
}

void AXRNA_SetNumChan(AXRNA_OBJ *rna, int32_t nch)
{
    if (nch >= 1 && nch <= rna->maxnch) {
        rna->nch = nch;
        axrna_apply_gains(rna);
    }
}

int32_t AXRNA_GetNumRoom(AXRNA_OBJ *rna)
{
    int32_t room = AXRNA_IOP_FRAMES - axrna_data(rna);

    return room > 0 ? room : 0;
}

int32_t AXRNA_GetNumData(AXRNA_OBJ *rna)
{
    return axrna_data(rna);
}

void AXRNA_SetPlaySw(AXRNA_OBJ *rna, int32_t sw)
{
    int32_t was = rna->play_sw;

    rna->play_sw = sw != 0;
    p4snd_voice_play(rna->voice, rna->play_sw);
    if (!rna->play_sw && !rna->trans_sw) {
        p4snd_stream_flush(rna->voice);
        axrna_sync(rna);
    } else if (rna->play_sw && !was) {
        rna->last_cnt = axrna_pass_count();
        axrna_start_run(rna, rna->last_cnt);
    }
}

void AXRNA_SetTransSw(AXRNA_OBJ *rna, int32_t sw)
{
    rna->trans_sw = sw != 0;
}
