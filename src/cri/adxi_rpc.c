/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* Host side of the sif RPC client the EE uses to talk to CRI_ADXI.IRX (CRI ADX Driver Ver.10.52).
 *
 * The module runs three servers behind one RPC dispatcher (IOP 0x740: fno 0x400 + id, id < 64,
 * routed through the tables registered with 0xe10(id, func, ctx)):
 *
 *    0..3    SJX/DTX    the EE's stream-joint exchange slots and the DTX open (handlers 0x98ac..0x9988)
 *    8..18   IOPRNA     PCM output objects ("PSM") and the two SPU2 cores (handlers 0x1258..0x1604)
 *    32..42  remote SJ  create and call stream joints that live in IOP memory (handlers 0x9240..0x9518)
 *
 * The EE side is func_004e58a0 (src/cri/ci_dv.c), which copies n words to the server's receive
 * buffer, calls func_004258b0 = sceSifCallRpc(0x00732D30, 0x400 + id, 0, send, 4n, recv, 4m, 0, 0)
 * and copies m reply words back. This file is func_004258b0 for that client: it runs the command
 * the way the IOP handler does and leaves the reply words in the buffer.
 *
 * What is modelled (every handler read against the IRX disassembly; the IOP numbers in the comments
 * are offsets of the .text image, handlers get (ctx, buf, nwords)):
 *  - the server buffer persists between calls and only the bytes sent are overwritten: handler
 *    0x1468 (SetSfreq) reads words 1 and 2 that the EE never sends;
 *  - remote SJ: the joint objects are CRI's own (SJRBF/SJMEM/SJUNI, the same library the IOP links),
 *    built over IOP RAM (src/platform/sound/sdr_mem.c). A handle on the wire is the object's
 *    address, chunk data pointers on the wire are IOP addresses;
 *  - IOPRNA: the global mode/flag state machine of 0x1838..0x2494 ported as is (core mode, pause,
 *    sync, per core running/started flags) and the PSM object table. A PSM becomes a mixer stream
 *    voice (p4snd.h); while its SPU2 core runs, its joints' PCM moves into the voice, the way the
 *    IOP's output thread moves it into the SPU2 ring.
 *
 * What is not modelled: the DTX data path (EE to IOP block transfers with callbacks to the EE),
 * the SJX slots do nothing after creation, and the PSM mixing registers (volume tables, the SPU2
 * mix attributes of 0x612c are kept, not applied). PSM volume is full, pan is hard left/right for
 * two channels. A PSM_Create failure hangs the IOP (0x1324 spins); here it replies handle 0.
 *
 * Handles that the IOP hands out are addresses in its data image; the same values are used here
 * (SJX slots 0x15c40 + 20 i, IOPRNA slots 0xf060 + 28 i). Unknown commands and clients are logged
 * once by id. */
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adxi_rpc.h"
#include "cri_mem.h"
#include "../platform/sound/sdr_mem.h"
#include "p4pc/profile_trace.h"

extern void *SJRBF_Create(void *buf, int32_t size, int32_t extra);
extern void *func_004ed600(void *buf, int32_t size); /* SJMEM_Create */
extern void *SJUNI_Create(int32_t mode, void *work, int32_t wksize);

#define ADXI_BUF_WORDS 64
#define ADXI_NUM_ID 64
#define SJX_NUM 32
#define SJX_BASE 0x15c40u
#define SJX_SIZE 20u
#define RNA_NUM 16
#define RNA_BASE 0xf060u
#define RNA_SIZE 28u
#define SJ_NUM 64
#define SJ_FREE 0
#define SJ_DATA 1
#define PSM_RING_FRAMES 0x4000
#define PSM_MAX_CH 2

typedef struct {
    uint8_t used;
    uint16_t id; /* word 1 of cmd 2, kept at +2 */
    uint32_t w[4]; /* create words: +8 buf[0], +4 buf[1], +12 (buf[2] == 1 ? 0 : 1), +16 buf[3] */
} SjxSlot;

typedef struct {
    uint8_t used;
    uint8_t psm_alive;
    int32_t nch;
    int32_t core;
    uint32_t sj[PSM_MAX_CH];
    int32_t sfreq;
    int32_t hold;     /* PSM +80: set by volume/hold calls that no RPC command makes */
    int32_t syncflag; /* the 0xdb... per PSM flag of the sync start/stop pair */
    P4SndVoice *voice;
} RnaSlot;

static struct {
    uint32_t buf[ADXI_BUF_WORDS];
    SjxSlot sjx[SJX_NUM];
    int32_t dtx_open;  /* DTX 0 of cmd 3 */
    int32_t dtx1_open; /* DTX 1 of IOPRNA init */
    RnaSlot rna[RNA_NUM];
    uint32_t sj[SJ_NUM]; /* live remote joints */
    /* IOPRNA globals (IOP .data, address minus 0x10000 in the comments) */
    int32_t mode;         /* -8964 */
    int32_t core_mode;    /* -8992 */
    int32_t pause;        /* -8984 */
    int32_t sync;         /* -8988 */
    int32_t init_cnt;     /* -8996 */
    int32_t core_flag[2]; /* -8980 + 4 c */
    int32_t core_started[2]; /* -8972 + 4 c */
    int32_t sfreq;        /* -7980 */
    int32_t voices;       /* -7968 */
    uint32_t wk, wksize;  /* -7964, -7952 */
    uint16_t mix[2];      /* -8876 */
    int tick_hooked;
} S;

static atomic_flag lock = ATOMIC_FLAG_INIT;

static void lock_take(void)
{
    while (atomic_flag_test_and_set_explicit(&lock, memory_order_acquire)) {
    }
}

static void lock_drop(void)
{
    atomic_flag_clear_explicit(&lock, memory_order_release);
}

/* ------------------------------------------------------------------ logging */

static uint32_t seen_fno[ADXI_NUM_ID];
static int seen_fno_n;
static uint32_t seen_client[8];
static int seen_client_n;
static int seen_endfunc;
static uint8_t seen_bad[16];

/* An fno the server has no handler for: reported once. */
static void log_unknown(uint32_t fno)
{
    uint32_t id = fno - 0x400u;
    int i;

    for (i = 0; i < seen_fno_n; i++) {
        if (seen_fno[i] == fno) {
            return;
        }
    }
    if (seen_fno_n < ADXI_NUM_ID) {
        seen_fno[seen_fno_n++] = fno;
    }
    if (id < ADXI_NUM_ID) {
        fprintf(stderr, "adxi_rpc: unhandled command id %u (fno 0x%x)\n", (unsigned)id, (unsigned)fno);
    } else {
        fprintf(stderr, "adxi_rpc: unhandled fno 0x%x\n", (unsigned)fno);
    }
}

/* A handle or argument no real run produces (the IOP would crash on it): reported once per kind. */
static void log_bad(int what, const char *text, uint32_t v)
{
    if (seen_bad[what]) {
        return;
    }
    seen_bad[what] = 1;
    fprintf(stderr, "adxi_rpc: %s 0x%x\n", text, (unsigned)v);
}

/* ------------------------------------------------------------------ IOP RAM */

/* The IOP address of a guest address inside IOP RAM (a stream-joint chunk holds one), 0 outside it. */
static uint32_t iop_addr(uint32_t gaddr)
{
    uint32_t base = p4sdr_iop_gaddr(0);

    if (gaddr >= base && gaddr < base + P4SDR_IOP_RAM_SIZE) {
        return gaddr - base;
    }
    return 0;
}

/* ------------------------------------------------------------------ remote stream joints */

static int sj_known(uint32_t h)
{
    int i;

    for (i = 0; i < SJ_NUM; i++) {
        if (h != 0 && S.sj[i] == h) {
            return 1;
        }
    }
    return 0;
}

/* Registers a freshly created joint and returns its handle (0 when the table is full or the
 * creator failed). */
static uint32_t sj_register(void *sj, void (*destroy_fn)(void *))
{
    int i;

    if (sj == NULL) {
        return 0;
    }
    for (i = 0; i < SJ_NUM; i++) {
        if (S.sj[i] == 0) {
            S.sj[i] = P4_H2G(sj);
            return S.sj[i];
        }
    }
    destroy_fn(sj);
    return 0;
}

static void sj_destroy_obj(void *sj)
{
    ((void (*)(void *))p4_fn_g2h_sig(sj_vt(sj)[3], P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(sj);
}

/* The wire form of a chunk: data is an IOP address. A chunk with a length must lie in IOP RAM. */
static int ck_from_wire(uint32_t addr, uint32_t len, Ck *ck)
{
    ck->len = (int32_t)len;
    ck->data = 0;
    if ((len > 0 || addr != 0) && p4sdr_iop_ptr(addr, len) != NULL) {
        ck->data = p4sdr_iop_gaddr(addr);
    }
    return len == 0 || ck->data != 0;
}

static void sj_cmd(uint32_t id, uint32_t *b)
{
    switch (id) {
    case P4ADXI_SJ_CREATE_RBF: { /* 0x9240 -> SJRBF_Create(buf, size, extra) */
        int32_t size = (int32_t)b[1];
        int32_t extra = (int32_t)b[2];
        uint8_t *mem = (size >= 0 && extra >= 0) ? p4sdr_iop_ptr(b[0], (uint32_t)size + (uint32_t)extra) : NULL;

        b[0] = mem != NULL ? sj_register(SJRBF_Create(mem, size, extra), sj_destroy_obj) : 0;
        return;
    }
    case P4ADXI_SJ_CREATE_MEM: { /* 0x9280 -> SJMEM_Create(buf, size) */
        int32_t size = (int32_t)b[1];
        uint8_t *mem = size >= 0 ? p4sdr_iop_ptr(b[0], (uint32_t)size) : NULL;

        b[0] = mem != NULL ? sj_register(func_004ed600(mem, size), sj_destroy_obj) : 0;
        return;
    }
    case P4ADXI_SJ_CREATE_UNI: { /* 0x92bc -> SJUNI_Create(mode, work, wksize) */
        int32_t wksize = (int32_t)b[2];
        uint8_t *mem = wksize >= 0 ? p4sdr_iop_ptr(b[1], (uint32_t)wksize) : NULL;

        b[0] = mem != NULL ? sj_register(SJUNI_Create((int32_t)b[0], mem, wksize), sj_destroy_obj) : 0;
        return;
    }
    default:
        break;
    }

    if (!sj_known(b[0])) {
        log_bad(0, "remote SJ call on an unknown handle", b[0]);
        if (id == P4ADXI_SJ_GETUUID) {
            memset(b, 0, 16);
        } else if (id == P4ADXI_SJ_GETCHUNK) {
            b[0] = b[1] = 0;
        } else if (id == P4ADXI_SJ_GETNUMDATA) {
            b[0] = 0;
        } else if (id == P4ADXI_SJ_ISGETCHUNK) {
            b[0] = b[1] = 0;
        }
        return;
    }

    {
        void *sj = (void *)P4_G2H(b[0]);
        Ck ck;
        int32_t rbyte;

        switch (id) {
        case P4ADXI_SJ_DESTROY: { /* 0x92fc: vtbl +12 */
            int i;

            for (i = 0; i < SJ_NUM; i++) {
                if (S.sj[i] == b[0]) {
                    S.sj[i] = 0;
                }
            }
            sj_destroy_obj(sj);
            break;
        }
        case P4ADXI_SJ_GETUUID: { /* 0x9334: vtbl +16, the 16 byte uuid goes back */
            const void *u = ((const void *(*)(void *))p4_fn_g2h_sig(sj_vt(sj)[4], P4_SIG(1, 1, 1, 0, 0, 0, 0, 0)))(sj);

            if (u != NULL) {
                memcpy(b, u, 16);
            } else {
                memset(b, 0, 16);
            }
            break;
        }
        case P4ADXI_SJ_RESET: /* 0x93a4: vtbl +20 */
            sj_reset(sj);
            break;
        case P4ADXI_SJ_GETCHUNK: /* 0x93dc: vtbl +24 (sj, id, nbyte, &ck) -> data, len */
            sj_get(sj, (int32_t)b[1], (int32_t)b[2], &ck);
            b[0] = iop_addr(ck.data);
            b[1] = (uint32_t)ck.len;
            break;
        case P4ADXI_SJ_UNGETCHUNK: /* 0x9434: vtbl +28 (sj, id, &{b[2], b[3]}) */
            if (ck_from_wire(b[2], b[3], &ck)) {
                sj_unget(sj, (int32_t)b[1], &ck);
            } else {
                log_bad(1, "remote SJ UngetChunk outside IOP RAM, addr", b[2]);
            }
            break;
        case P4ADXI_SJ_PUTCHUNK: /* 0x9484: vtbl +32 */
            if (ck_from_wire(b[2], b[3], &ck)) {
                sj_put(sj, (int32_t)b[1], &ck);
            } else {
                log_bad(2, "remote SJ PutChunk outside IOP RAM, addr", b[2]);
            }
            break;
        case P4ADXI_SJ_GETNUMDATA: /* 0x94d4: vtbl +36 */
            b[0] = (uint32_t)sj_num(sj, (int32_t)b[1]);
            break;
        case P4ADXI_SJ_ISGETCHUNK: { /* 0x9518: vtbl +40 (sj, id, nbyte, &rbyte) -> result, rbyte */
            int32_t r = ((int32_t(*)(void *, int32_t, int32_t, int32_t *))p4_fn_g2h_sig(sj_vt(sj)[10], P4_SIG(4, 0, 1, 0, 0, 1, 0, 0)))(
                sj, (int32_t)b[1], (int32_t)b[2], &rbyte);

            b[0] = (uint32_t)r;
            b[1] = (uint32_t)rbyte;
            break;
        }
        default:
            break;
        }
    }
}

/* ------------------------------------------------------------------ SJX / DTX */

static SjxSlot *sjx_of(uint32_t h)
{
    uint32_t off;

    if (h < SJX_BASE) {
        return NULL;
    }
    off = h - SJX_BASE;
    if (off % SJX_SIZE != 0 || off / SJX_SIZE >= SJX_NUM) {
        return NULL;
    }
    return &S.sjx[off / SJX_SIZE];
}

static void sjx_cmd(uint32_t id, uint32_t *b)
{
    SjxSlot *s;
    int i;

    switch (id) {
    case P4ADXI_SJX_CREATE: /* 0x98ac: first free of 32 slots; none: handle 0 */
        for (i = 0; i < SJX_NUM; i++) {
            if (!S.sjx[i].used) {
                break;
            }
        }
        if (i == SJX_NUM) {
            b[0] = 0;
            return;
        }
        s = &S.sjx[i];
        memset(s, 0, sizeof(*s));
        s->w[1] = b[1];
        s->w[0] = b[0];
        s->w[2] = b[2] == 1 ? 0 : 1;
        s->w[3] = b[3];
        s->used = 1;
        b[0] = SJX_BASE + SJX_SIZE * (uint32_t)i;
        return;
    case P4ADXI_SJX_DESTROY: /* 0x996c: clears the used byte */
        s = sjx_of(b[0]);
        if (s == NULL) {
            log_bad(3, "SJX destroy on a bad handle", b[0]);
            return;
        }
        s->used = 0;
        return;
    case P4ADXI_SJX_SETID: /* 0x9978: halfword +2 of the slot = low half of word 1 */
        s = sjx_of(b[0]);
        if (s == NULL) {
            log_bad(4, "SJX set id on a bad handle", b[0]);
            return;
        }
        s->id = (uint16_t)b[1];
        return;
    case P4ADXI_DTX_OPEN: /* 0x9988: DTX 0 created, receive and complete callbacks set */
        S.dtx_open = 1;
        return;
    default:
        return;
    }
}

/* ------------------------------------------------------------------ IOPRNA: PSM and cores */

static void psm_destroy(RnaSlot *r)
{
    if (r->voice != NULL) {
        p4snd_voice_destroy(r->voice);
        r->voice = NULL;
    }
    r->psm_alive = 0;
}

/* 0x1aa8 */
static void core_start(int c)
{
    int cm_ok = S.core_mode == 2 || c == S.core_mode;

    switch (S.mode) {
    case 1:
        if (cm_ok) {
            S.core_flag[c] = 1;
        }
        S.core_started[c] = 1;
        break;
    case 3:
        if (!S.pause) {
            S.core_flag[c] = 1;
        }
        break;
    case 4:
        if (cm_ok && !S.pause && S.core_started[c] == 1) {
            S.core_flag[c] = 1;
        }
        break;
    case 5:
        if (S.sync == 1 || (S.core_started[c] == 1 && cm_ok)) {
            S.core_flag[c] = 1;
        }
        break;
    case 0:
        if (S.core_mode == 2 && !S.pause) {
            S.core_started[c] = 1;
            S.core_flag[c] = 1;
        }
        break;
    default: /* 2: finishing */
        break;
    }
}

/* 0x1d4c */
static void core_stop(int c)
{
    int cm_ok = c == S.core_mode || S.core_mode == 2;

    switch (S.mode) {
    case 2:
        S.core_flag[c] = 0;
        S.core_started[c] = 0;
        break;
    case 3:
    case 4:
        S.core_flag[c] = 0;
        break;
    case 5:
        if (S.sync == 1 || cm_ok) {
            S.core_flag[c] = 0;
        }
        break;
    case 0:
        if (S.core_mode == 2 && S.sync == 0) {
            S.core_started[c] = 0;
            S.core_flag[c] = 0;
        }
        break;
    default: /* 1: initialising */
        break;
    }
}

/* 0x2294 */
static void rna_init(void)
{
    int i;

    if (S.init_cnt == 0) {
        S.mode = 1;
        for (i = 0; i < 2; i++) {
            core_start(i);
        }
        for (i = 0; i < RNA_NUM; i++) {
            psm_destroy(&S.rna[i]);
            S.rna[i].syncflag = 0;
        }
        S.mode = 0;
    }
    S.init_cnt++;
}

/* 0x237c */
static void rna_finish(void)
{
    int i;

    S.init_cnt--;
    if (S.init_cnt == 0) {
        S.mode = 2;
        for (i = 0; i < RNA_NUM; i++) {
            if (S.rna[i].used && S.rna[i].psm_alive) {
                psm_destroy(&S.rna[i]);
            }
        }
        core_stop(0);
        core_stop(1);
        S.mode = 0;
    }
}

/* 0x1894 */
static void rna_sync_start(void)
{
    int i;

    S.mode = 3;
    core_stop(0);
    core_stop(1);
    S.sync = 1;
    for (i = 0; i < RNA_NUM; i++) {
        RnaSlot *r = &S.rna[i];

        if (r->psm_alive && r->hold == 0 && r->core == 0) {
            r->syncflag = 1;
            r->core = 1;
        }
    }
    core_start(0);
    core_start(1);
    S.mode = 0;
}

/* 0x19bc */
static void rna_sync_stop(void)
{
    int i;

    S.mode = 4;
    core_stop(0);
    core_stop(1);
    S.sync = 0;
    for (i = 0; i < RNA_NUM; i++) {
        RnaSlot *r = &S.rna[i];

        if (r->syncflag == 1) {
            r->core = 0;
            r->syncflag = 0;
        }
    }
    core_start(0);
    core_start(1);
    S.mode = 0;
}

/* 0x2484 */
static void rna_pause(int32_t v)
{
    int i;

    if (v == S.pause) {
        return;
    }
    S.mode = 5;
    S.pause = v;
    for (i = 0; i < 2; i++) {
        if (v == 1) {
            core_stop(i);
        } else {
            core_start(i);
        }
    }
    S.mode = 0;
}

static RnaSlot *rna_of(uint32_t h)
{
    uint32_t off;

    if (h < RNA_BASE) {
        return NULL;
    }
    off = h - RNA_BASE;
    if (off % RNA_SIZE != 0 || off / RNA_SIZE >= RNA_NUM || !S.rna[off / RNA_SIZE].used) {
        return NULL;
    }
    return &S.rna[off / RNA_SIZE];
}

static void adxi_tick(double seconds)
{
    (void)seconds;
    p4adxi_pump();
}

/* 0x1258: words [nch, core, sj0, sj1...]; reply: the handle. */
static void rna_create(uint32_t *b)
{
    int32_t nch = (int32_t)b[0];
    int32_t core = (int32_t)b[1];
    RnaSlot *r;
    int i;
    int ch;

    for (i = 0; i < RNA_NUM; i++) {
        if (!S.rna[i].used) {
            break;
        }
    }
    if (i == RNA_NUM) {
        b[0] = 0;
        return;
    }
    r = &S.rna[i];
    /* 0x2600: the core is 0 or 1 and the channel count not negative; the layout of the object
     * leaves room for two channels. */
    if (core < 0 || core > 1 || nch < 0 || nch > PSM_MAX_CH) {
        log_bad(5, "PSM_Create refused (core/channels), nch", (uint32_t)nch);
        b[0] = 0;
        return;
    }
    for (ch = 0; ch < nch; ch++) {
        if (!sj_known(b[2 + ch])) {
            log_bad(6, "PSM_Create on an unknown joint handle", b[2 + ch]);
            b[0] = 0;
            return;
        }
    }
    memset(r, 0, sizeof(*r));
    r->nch = nch;
    r->core = core;
    for (ch = 0; ch < nch; ch++) {
        r->sj[ch] = b[2 + ch];
    }
    r->sfreq = S.sfreq;
    if (nch > 0) {
        r->voice = p4snd_stream_create(nch, PSM_RING_FRAMES);
        if (r->voice == NULL) {
            log_bad(7, "PSM_Create: no mixer voice, nch", (uint32_t)nch);
            b[0] = 0;
            return;
        }
        p4snd_voice_set_rate(r->voice, r->sfreq > 0 ? (double)r->sfreq : (double)P4SND_RATE);
        for (ch = 0; ch < nch; ch++) {
            float l, rr;

            p4snd_cri_gains(0, nch == 2 ? (ch == 0 ? -15 : 15) : 0, &l, &rr);
            p4snd_voice_set_gain(r->voice, ch, l, rr);
        }
        if (!S.tick_hooked) {
            S.tick_hooked = 1;
            p4snd_add_tick_hook(adxi_tick);
        }
    }
    r->psm_alive = 1;
    r->used = 1;
    b[0] = RNA_BASE + RNA_SIZE * (uint32_t)i;
}

static void reply_flags(uint32_t *b)
{
    b[0] = (uint32_t)S.core_flag[0];
    b[1] = (uint32_t)S.core_flag[1];
}

static void rna_cmd(uint32_t id, uint32_t *b)
{
    RnaSlot *r;

    switch (id) {
    case P4ADXI_RNA_CREATE:
        rna_create(b);
        return;
    case P4ADXI_RNA_DESTROY: /* 0x1370: destroy the PSM, free the slot */
        r = rna_of(b[0]);
        if (r == NULL) {
            log_bad(8, "IOPRNA destroy on a bad handle", b[0]);
            return;
        }
        psm_destroy(r);
        r->used = 0;
        return;
    case P4ADXI_RNA_INIT: /* 0x13a8: (voices, work address, work size), DTX 1 opened */
        S.voices = (int32_t)b[0];
        S.wk = b[1];
        S.wksize = b[2];
        S.dtx1_open = 1;
        return;
    case P4ADXI_RNA_SETCOREMODE: /* 0x1434: SetCoreMode(w0), Finish, Init; no reply */
        if (b[0] <= 2) {
            S.core_mode = (int32_t)b[0];
            S.mode = 0;
        }
        rna_finish();
        rna_init();
        return;
    case P4ADXI_RNA_SETSFREQ: /* 0x1468: base sfreq, SPU2 mix attributes from words 1 and 2, Init */
        S.sfreq = (int32_t)b[0];
        if ((uint16_t)b[1] == 0xffff || (uint16_t)b[2] == 0xffff) {
            S.mix[0] = 0;
            S.mix[1] = 2177;
        } else {
            S.mix[0] = (uint16_t)b[1];
            S.mix[1] = (uint16_t)((uint16_t)b[1] | ((uint16_t)b[2] | 1));
        }
        rna_init();
        b[0] = (uint32_t)S.core_mode;
        b[1] = (uint32_t)S.core_flag[0];
        b[2] = (uint32_t)S.core_flag[1];
        return;
    case P4ADXI_RNA_FINISH: /* 0x14d0 */
        rna_finish();
        reply_flags(b);
        return;
    case P4ADXI_RNA_SYNCSTART: /* 0x150c */
        rna_sync_start();
        reply_flags(b);
        return;
    case P4ADXI_RNA_SYNCSTOP: /* 0x1548 */
        rna_sync_stop();
        reply_flags(b);
        return;
    case P4ADXI_RNA_STARTCORE: /* 0x1584: core index unchecked on the IOP */
        if (b[0] > 1) {
            log_bad(9, "IOPRNA start of a bad core", b[0]);
            b[0] = 0;
            return;
        }
        core_start((int)b[0]);
        b[0] = (uint32_t)S.core_flag[b[0]];
        return;
    case P4ADXI_RNA_STOPCORE: /* 0x15c4 */
        if (b[0] > 1) {
            log_bad(10, "IOPRNA stop of a bad core", b[0]);
            b[0] = 0;
            return;
        }
        core_stop((int)b[0]);
        b[0] = (uint32_t)S.core_flag[b[0]];
        return;
    case P4ADXI_RNA_PAUSE: /* 0x1604 */
        rna_pause((int32_t)b[0]);
        reply_flags(b);
        return;
    default:
        return;
    }
}

/* ------------------------------------------------------------------ the output thread's work */

static void pump_rna(RnaSlot *r)
{
    int running = r->core >= 0 && r->core < 2 && S.core_flag[r->core] && !r->hold;
    int32_t room;
    int32_t n;
    int ch;

    p4snd_voice_play(r->voice, running);
    if (!running) {
        return;
    }
    room = (int32_t)p4snd_stream_room(r->voice);
    n = room;
    for (ch = 0; ch < r->nch; ch++) {
        int32_t have = sj_num((void *)P4_G2H(r->sj[ch]), SJ_DATA) >> 1;

        if (have < n) {
            n = have;
        }
    }
    if (n <= 0) {
        return;
    }
    for (ch = 0; ch < r->nch; ch++) {
        void *sj = (void *)P4_G2H(r->sj[ch]);
        int32_t done = 0;

        while (done < n) {
            Ck ck;

            sj_get(sj, SJ_DATA, (n - done) * 2, &ck);
            if (ck.len <= 0) {
                break;
            }
            p4snd_stream_write(r->voice, ch, (uint32_t)done, (const int16_t *)P4_G2H(ck.data), (uint32_t)(ck.len >> 1));
            done += ck.len >> 1;
            sj_put(sj, SJ_FREE, &ck);
        }
    }
    p4snd_stream_commit(r->voice, (uint32_t)n);
}

static void pump_locked(void)
{
    int i;

    for (i = 0; i < RNA_NUM; i++) {
        if (S.rna[i].used && S.rna[i].psm_alive && S.rna[i].voice != NULL) {
            pump_rna(&S.rna[i]);
        }
    }
}

void p4adxi_pump(void)
{
    uint64_t trace_start = p4pc_prof_span_begin();
    lock_take();
    uint64_t trace_locked = p4pc_prof_span_stop(trace_start);
    pump_locked();
    lock_drop();
    uint64_t trace_end = p4pc_prof_span_stop(trace_start);
    p4pc_prof_span_record(trace_start, trace_locked, "audio.adxi.lock", 0, 0, 0, NULL);
    p4pc_prof_span_record(trace_start, trace_end, "audio.adxi.pump", 0, 0, 0, NULL);
}

/* ------------------------------------------------------------------ dispatcher */

typedef void (*AdxiHandler)(uint32_t id, uint32_t *buf);

static AdxiHandler handler_of(uint32_t id)
{
    if (id <= 3) {
        return sjx_cmd;
    }
    if (id >= 8 && id <= 18) {
        return rna_cmd;
    }
    if (id >= 32 && id <= 42) {
        return sj_cmd;
    }
    return NULL;
}

/* 0x004258b0: sceSifCallRpc(client, fno, mode, send, ssize, recv, rsize, endfunc, efarg). Returns
 * 0 once the command has run (the call is synchronous here, so a no-wait mode has finished too)
 * and -1 for a client that is not the module's server. */
int32_t func_004258b0(uint32_t client, uint32_t fno, uint32_t mode, uint32_t send, uint32_t ssize, uint32_t recv,
                      uint32_t rsize, uint32_t endfunc, uint32_t efarg)
{
    uint32_t id = fno - 0x400u;
    AdxiHandler h;
    int i;

    (void)mode;
    (void)efarg;
    if (client != P4ADXI_CLIENT) {
        for (i = 0; i < seen_client_n; i++) {
            if (seen_client[i] == client) {
                return -1;
            }
        }
        if (seen_client_n < 8) {
            seen_client[seen_client_n++] = client;
        }
        fprintf(stderr, "adxi_rpc: sceSifCallRpc on client 0x%08x (fno 0x%x) is not the CRI_ADXI server\n",
                (unsigned)client, (unsigned)fno);
        return -1;
    }
    if (ssize > sizeof(S.buf) || rsize > sizeof(S.buf)) {
        fprintf(stderr, "adxi_rpc: fno 0x%x moves %u/%u bytes, the server buffer is %u\n", (unsigned)fno,
                (unsigned)ssize, (unsigned)rsize, (unsigned)sizeof(S.buf));
        return -1;
    }
    if (endfunc != 0 && !seen_endfunc) {
        seen_endfunc = 1;
        fprintf(stderr, "adxi_rpc: end function 0x%x of fno 0x%x is not called\n", (unsigned)endfunc, (unsigned)fno);
    }

    uint64_t trace_start = p4pc_prof_span_begin();
    lock_take();
    uint64_t trace_locked = p4pc_prof_span_stop(trace_start);
    if (send != 0 && ssize != 0) {
        memcpy(S.buf, (const void *)P4_G2H(send), ssize);
    }
    h = id < ADXI_NUM_ID ? handler_of(id) : NULL;
    if (h != NULL) {
        h(id, S.buf);
    } else if (fno > 1) {
        /* fno 0 and 1 are empty in the IOP dispatcher; 2 and 3 (0x9f0, 0xb08) belong to the DTX
         * callbacks, which the EE never reaches through this client */
        log_unknown(fno);
    }
    if (recv != 0 && rsize != 0) {
        memcpy((void *)P4_G2H(recv), S.buf, rsize);
    }
    lock_drop();
    uint64_t trace_end = p4pc_prof_span_stop(trace_start);
    p4pc_prof_span_record(trace_start, trace_locked, "audio.rpc.lock", ssize, 0, fno, NULL);
    p4pc_prof_span_record(trace_start, trace_end, "audio.rpc.call", ssize, 0, fno, NULL);
    return 0;
}

/* ------------------------------------------------------------------ test and reset access */

void p4adxi_reset(void)
{
    int i;

    lock_take();
    for (i = 0; i < RNA_NUM; i++) {
        psm_destroy(&S.rna[i]);
    }
    for (i = 0; i < SJ_NUM; i++) {
        if (S.sj[i] != 0) {
            sj_destroy_obj((void *)P4_G2H(S.sj[i]));
        }
    }
    {
        int hooked = S.tick_hooked;

        memset(&S, 0, sizeof(S));
        S.tick_hooked = hooked;
    }
    lock_drop();
}

int32_t p4adxi_core_running(int32_t core)
{
    return core >= 0 && core < 2 ? S.core_flag[core] : 0;
}

P4SndVoice *p4adxi_rna_voice(uint32_t handle)
{
    RnaSlot *r = rna_of(handle);

    return r != NULL ? r->voice : NULL;
}

int32_t p4adxi_rna_core(uint32_t handle)
{
    RnaSlot *r = rna_of(handle);

    return r != NULL ? r->core : -1;
}
