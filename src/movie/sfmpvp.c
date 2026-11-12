/* sfmpvp: Sofdec's IPU video path (SFMPVP, the viBuf ring and the libmpeg calls under it) on libavcodec.
 *
 * Retail Sofdec copies each video elementary-stream chunk into a ring (viBuf), DMAs the ring into the
 * PS2 IPU and has Sony's libmpeg decode one picture per sceMpegGetPicture call into the frame
 * buffer the game later uploads to the GS. P4 always takes this path: MwsfdCrePrm calls
 * func_005207f8(1) before creating a player, and the software decoder it falls back to is a stub
 * (0x00504d08 returns -1).
 *
 * This file replaces the hardware-facing functions at 0x0051fdc0..0x005213a8 and keeps the rest of
 * Sofdec (sfmpv, sfd_mps, the frame pool, the timing code) untouched. The struct offsets below are
 * the retail ones (SFMPVP object at *(sfd + 0x1fc0)):
 *   +0x1128 ctx   +0 used, +4/+8/+0xc picture-start-code counts, +0x10..0x12 last three ES bytes,
 *                 +0x14 in GetPicture, +0x18 starved/failed, +0x1c flushed, +0x20 flush argument
 *   +0x1150 mp    libmpeg object (72 bytes): +8 frame count, +0x40 private state
 *   +0x1198 vb    viBuf, 0x60 bytes (struct ViBuf)
 *   +0x11f8       8-byte libmpeg error record, +0x11fc is the message word SFD_GetSceErrMessage returns
 * The caller's picture-size words are at +0xc (width) and +0x10 (height).
 *
 * viBuf model. The ring holds the stream in 2048-byte chunks. rd is the chunk the DMA has reached,
 * q the chunks queued behind it, r the bytes put but not yet queued. Retail reads the DMA address
 * back from the hardware to move rd; here the decoder's consumption point C plays that role. The
 * IPU FIFO holds up to 8 quadwords, so the DMA is at most 128 bytes ahead of C (IPU_BP semantics:
 * StopDMA/RestartDMA hand back what the FIFO held). Absolute (never wrapping) counters live in a
 * side table keyed by the viBuf.
 *
 * Picture order. libmpeg outputs in display order, so a call may consume more than one picture
 * (I and P at the start) and outputs the held reference at a sequence end code; libavcodec does
 * the same. A picture unit runs from its start code (with any sequence/GOP headers before it) to the
 * next sequence, GOP, picture or sequence end code, which the IPU would leave unread.
 * [INFERENCE: display order and the one-picture-ahead consumption follow from Sofdec requiring
 * three picture start codes (ctx+0xc >= 3) before decoding, and from sceMpegIsRefBuffEmpty existing;
 * not yet checked against a PCSX2 run.] */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mpeg2_dec.h"
#include "p4pc/gmem.h"
#include "p4pc/platform.h"

/* ---- Retail globals and callees ------------------------------------------------------------ */

#define G_USE_SCEMPEG (*(volatile int32_t *)P4_G2H(0x00745af8u)) /* set by func_005207f8 */
#define G_KEEP_MPEG (*(volatile int32_t *)P4_G2H(0x00745abcu))   /* func_005230c8 returns it */
#define G_ERR_FLAG (*(volatile int32_t *)P4_G2H(0x00948978u))
#define G_ERR_REC ((uint8_t *)P4_G2H(0x00948980u))
#define G_SHADOW ((uint8_t *)P4_G2H(0x00948930u))     /* 72-byte copy of the libmpeg object */

extern int32_t func_00504d08(void);                       /* software decode stub: returns -1 */
extern int32_t func_00504d10(void *mpv, void *sj, void *frm);
extern int32_t func_00515c60(void *sfd);
extern int32_t func_00515cc8(void *sfd);
extern int32_t func_00517ad0(void *sfd, int32_t code);    /* SFD error report */

/* ---- Layouts -------------------------------------------------------------------------------- */

typedef struct ViBuf {
    uint32_t base;      /* 0x00 ring address */
    uint32_t tags;      /* 0x04 DMA tag array (not used here) */
    int32_t nchunks;    /* 0x08 */
    int32_t rd;         /* 0x0c chunk the DMA has reached (mod nchunks) */
    int32_t q;          /* 0x10 chunks queued behind rd */
    int32_t r;          /* 0x14 bytes put, not yet queued */
    int32_t size;       /* 0x18 nchunks * 2048 */
    uint32_t saved[9];  /* 0x1c..0x3c DMA/IPU registers saved by StopDMA (unused) */
    int32_t sema;       /* 0x40 retail semaphore; this model never blocks */
    int32_t active;     /* 0x44 DMA may be queued (1 after Restart/Create, 0 after Stop) */
    uint32_t put_lo;    /* 0x48 total bytes put (64-bit) */
    uint32_t put_hi;    /* 0x4c */
    uint32_t entries;   /* 0x50 tag entries (24 bytes each) */
    int32_t nentries;   /* 0x54 */
    int32_t f58;        /* 0x58 */
    int32_t f5c;        /* 0x5c */
} ViBuf;
_Static_assert(sizeof(ViBuf) == 0x60, "viBuf is 0x60 bytes");

#define CHUNK 2048
#define FIFO_AHEAD 128   /* IPU input FIFO: 8 quadwords */

static inline uint8_t *U8(uint32_t a) { return (uint8_t *)P4_G2H(a); }
static inline int32_t *W(uint32_t a) { return (int32_t *)P4_G2H(a); }
static inline uint32_t P_OF(void *sfd) { return (uint32_t)*W(P4_H2G(sfd) + 0x1fc0); }

typedef struct Obj {    /* addresses of one SFMPVP object's parts */
    uint32_t p, ctx, mp, vb, err;
} Obj;

static Obj obj_of(void *sfd) {
    Obj o;
    o.p = P_OF(sfd);
    o.ctx = o.p + 0x1128;
    o.mp = o.p + 0x1150;
    o.vb = o.p + 0x1198;
    o.err = o.p + 0x11f8;
    return o;
}

/* ---- Side table: absolute counters per viBuf ------------------------------------------------ */

typedef struct VbState {
    uint32_t vb;      /* key, 0 when free */
    int64_t c;        /* bytes the decoder has consumed (absolute stream offset) */
    int64_t rd;       /* absolute chunk the DMA has reached */
    int64_t eend;     /* absolute end chunk of the queued data */
    int starved;      /* the decoder has read everything queued: the DMA has delivered all of it */
} VbState;

#define MAX_VB 8
static VbState g_vb[MAX_VB];

static VbState *vb_state(uint32_t vb, int create) {
    VbState *free_slot = NULL;
    for (int i = 0; i < MAX_VB; i++) {
        if (g_vb[i].vb == vb) return &g_vb[i];
        if (!g_vb[i].vb && !free_slot) free_slot = &g_vb[i];
    }
    if (create && free_slot) {
        memset(free_slot, 0, sizeof *free_slot);
        free_slot->vb = vb;
        return free_slot;
    }
    return NULL;
}

static void vb_sync(ViBuf *v, const VbState *s) {
    v->rd = (int32_t)(s->rd % v->nchunks);
    v->q = (int32_t)(s->eend - s->rd);
}

/* ---- viBuf ----------------------------------------------------------------------------------- */

int32_t viBufCreate(void *vbp, uint32_t base, uint32_t tags, int32_t nchunks, uint32_t entries, int32_t nentries) {
    ViBuf *v = vbp;
    v->base = base;
    v->tags = (tags & 0x0fffffffu) | 0x20000000u;
    v->nchunks = nchunks;
    v->size = nchunks << 11;
    v->entries = entries;
    v->nentries = nentries;
    v->sema = 0;
    /* viBufReset */
    v->active = 1;
    v->rd = v->q = v->r = 0;
    v->f58 = v->f5c = 0;
    for (int i = 0; i < nentries; i++) {
        uint8_t *e = U8(entries) + i * 24;
        memset(e, 0xff, 16);
        *(int32_t *)(e + 0x10) = 0;
        *(int32_t *)(e + 0x14) = 0;
    }
    v->put_lo = v->put_hi = 0;
    VbState *s = vb_state(P4_H2G(v), 1);
    if (s) { s->c = s->rd = s->eend = 0; s->starved = 0; }
    return 1;
}

/* viBufBeginPut: where the next put may write. Out: first area (ptr, bytes), second area. */
int32_t viBufBeginPut(void *vbp, uint32_t *o1, int32_t *o2, uint32_t *o3, int32_t *o4) {
    ViBuf *v = vbp;
    int32_t w = ((v->rd + v->q) << 11) + v->r;
    int32_t woff = w % v->size;
    int32_t free_b = ((v->nchunks - v->q) << 11) - 4096 - v->r;
    int32_t to_end = v->size - woff;
    if (to_end < free_b) {
        *o1 = v->base + woff;
        *o2 = to_end;
        *o3 = v->base;
        *o4 = free_b - to_end;
    } else {
        *o1 = v->base + woff;
        *o2 = free_b;
        *o3 = 0;
        *o4 = 0;
    }
    return 1;
}

int32_t viBufEndPut(void *vbp, int32_t n) {
    ViBuf *v = vbp;
    uint64_t t = ((uint64_t)v->put_hi << 32) | v->put_lo;
    t += (uint64_t)(int64_t)n;
    v->r += n;
    v->put_lo = (uint32_t)t;
    v->put_hi = (uint32_t)(t >> 32);
    return 1;
}

int32_t viBufUnput(void *vbp, int32_t n) {
    ViBuf *v = vbp;
    uint64_t t = ((uint64_t)v->put_hi << 32) | v->put_lo;
    t -= (uint64_t)(int64_t)n;
    v->r -= n;
    if (v->r < 0) v->r = 0; /* retail executes a break instruction here */
    v->put_lo = (uint32_t)t;
    v->put_hi = (uint32_t)(t >> 32);
    return n;
}

int32_t viBufFlush(void *vbp) {
    ViBuf *v = vbp;
    int32_t x = v->r + 2051;
    if (x < 0) x = v->r + 4098;
    v->r = (x >> 11) << 11;
    return 1;
}

/* The DMA position: the decoder's consumption, rounded down to a quadword, plus what the IPU FIFO
 * has pulled in, never past the queued data. */
static int64_t dma_pos(const VbState *s) {
    if (s->starved) return s->eend << 11;
    int64_t c16 = s->c & ~(int64_t)15;
    int64_t d = c16 + FIFO_AHEAD;
    int64_t e = s->eend << 11;
    return d > e ? e : d;
}

int32_t viBufAddDMA(void *vbp) {
    ViBuf *v = vbp;
    VbState *s = vb_state(P4_H2G(v), 0);
    if (!s || !v->active) return 0;
    int64_t rd = dma_pos(s) >> 11;
    if (rd > s->rd) s->rd = rd;
    int32_t nfull = v->r / CHUNK;
    v->r -= nfull * CHUNK;
    s->eend += nfull;
    vb_sync(v, s);
    return 1;
}

int32_t viBufStopDMA(void *vbp) {
    ViBuf *v = vbp;
    v->active = 0;
    return 1;
}

int32_t viBufRestartDMA(void *vbp) {
    ViBuf *v = vbp;
    VbState *s = vb_state(P4_H2G(v), 0);
    if (s) {
        /* Retail rewinds the DMA to the first quadword the IPU has not consumed. */
        s->rd = (s->c & ~(int64_t)15) >> 11;
        s->starved = 0;
        vb_sync(v, s);
    }
    v->active = 1;
    return 1;
}

int32_t viBufDelete(void *vbp) {
    VbState *s = vb_state(P4_H2G(vbp), 0);
    if (s) s->vb = 0;
    return 1;
}

/* ---- SFMPVP ---------------------------------------------------------------------------------- */

int32_t SFMPVP_IsSceMpegUsed(void *sfd) { return *W(P_OF(sfd) + 0x1128) != 0; }

/* func_00520818: SFMPVP_Init */
void func_00520818(void) {
    if (G_USE_SCEMPEG != 0) memset(G_SHADOW, 0, 72);
}

int32_t SFMPVP_GetViBufInf(void *sfd, int32_t *size, int32_t *wr_off, int32_t *rd_off, int32_t *fill) {
    ViBuf *v = (ViBuf *)P4_G2H(obj_of(sfd).vb);
    *size = v->size;
    *rd_off = v->rd << 11;
    *wr_off = (((v->rd + v->q) << 11) + v->r) % v->size;
    *fill = (v->q << 11) + v->r;
    return *fill;
}

int32_t SFD_GetSceErrMessage(void *sfd) { return *W(obj_of(sfd).p + 0x11fc); }

int32_t copy2area(uint32_t d1, int32_t l1, uint32_t d2, int32_t l2, uint32_t sa, int32_t la, uint32_t sb, int32_t lb) {
    int32_t total = la + lb;
    if (total > l1 + l2) return 0;
    /* The source is the two pieces back to back; the destination the two areas back to back. */
    int32_t done = 0;
    for (int piece = 0; piece < 2; piece++) {
        const uint8_t *src = U8(piece == 0 ? sa : sb);
        int32_t len = piece == 0 ? la : lb;
        for (int32_t i = 0; i < len;) {
            int32_t pos = done + i;
            uint8_t *dst = pos < l1 ? U8(d1) + pos : U8(d2) + (pos - l1);
            int32_t room = pos < l1 ? l1 - pos : l2 - (pos - l1);
            int32_t n = len - i < room ? len - i : room;
            memcpy(dst, src + i, n);
            i += n;
        }
        done += len;
    }
    return total;
}

int32_t sfmpvp_PutVibuf(void *vbp, const void *data, int32_t len) {
    uint32_t a1, a2;
    int32_t l1, l2;
    viBufBeginPut(vbp, &a1, &l1, &a2, &l2);
    int32_t copied = copy2area(a1 & 0x0fffffffu, l1, a2 & 0x0fffffffu, l2, P4_H2G(data), len, 0, 0);
    viBufEndPut(vbp, copied);
    return copied;
}

int32_t SFMPVP_Put(void *sfd, const void *data, int32_t len) {
    if (!SFMPVP_IsSceMpegUsed(sfd)) return len;
    return sfmpvp_PutVibuf((void *)P4_G2H(obj_of(sfd).vb), data, len);
}

int32_t SFMPVP_Unput(void *sfd, int32_t n) {
    if (!SFMPVP_IsSceMpegUsed(sfd)) return n;
    return viBufUnput((void *)P4_G2H(obj_of(sfd).vb), n);
}

void SFMPVP_Flush(void *sfd, int32_t arg) {
    Obj o = obj_of(sfd);
    if (*W(o.ctx + 0x1c) != 1) {
        viBufFlush((void *)P4_G2H(o.vb));
        *W(o.ctx + 0x20) = arg;
        *W(o.ctx + 0x1c) = 1;
    }
}

int32_t SFMPVP_GetFrameCount(void *sfd) { return *W(obj_of(sfd).mp + 8); }

int32_t sfmpvp_ConsumePsc(void *sfd) {
    int32_t *c = W(obj_of(sfd).ctx + 0xc);
    *c -= 1;
    return *c;
}

/* func_005213a8: a picture can be decoded once three picture start codes are buffered, or the
 * stream has been flushed with argument 1. */
int32_t func_005213a8(void *sfd) {
    if (!SFMPVP_IsSceMpegUsed(sfd)) return 1;
    Obj o = obj_of(sfd);
    if (*W(o.ctx + 0x20) == 1) return 1;
    return *W(o.ctx + 0xc) >= 3;
}

/* Start code classes of 0x00503c58: only the picture (4) and sequence end (0x80) matter here. */
static uint32_t sc_class(const uint8_t *p) {
    uint32_t v = ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
    if (v == 0x100) return 4;
    if (v == 0x101) return 3;
    if (v - 0x102u <= 0xad) return 1;
    if (v == 0x1b2) return 0x20;
    if (v == 0x1b3) return 0x40;
    if (v == 0x1b5) return 0x10;
    if (v == 0x1b7) return 0x80;
    if (v == 0x1b8) return 8;
    return 0;
}

/* 0x00503df0 with mask 0x84: the first picture or sequence end start code in data. */
static const uint8_t *find_psc(const uint8_t *p, int32_t len) {
    for (int32_t i = 0; i + 4 <= len; i++)
        if (p[i] == 0 && p[i + 1] == 0 && p[i + 2] == 1 && (sc_class(p + i) & 0x84)) return p + i;
    return NULL;
}

void SFMPVP_SearchPsc(void *sfd, const uint8_t *data, int32_t len) {
    Obj o = obj_of(sfd);
    int32_t *c4 = W(o.ctx + 4), *c8 = W(o.ctx + 8), *cc = W(o.ctx + 0xc), *c10 = W(o.ctx + 0x10);
    uint8_t *tail = U8(o.ctx + 0x10);
    int found_end = 0;
    *cc += *c4;
    *c4 = *c8;
    *c8 = 0;
    uint8_t buf[6];
    buf[0] = tail[0]; buf[1] = tail[1]; buf[2] = tail[2];
    buf[3] = len > 0 ? data[0] : 0;
    buf[4] = len > 1 ? data[1] : 0;
    buf[5] = len > 2 ? data[2] : 0;
    (void)c10;
    /* Start codes that straddle the previous chunk's last three bytes. */
    int32_t lim = len < 4 ? len : 3;
    for (int32_t i = 0; i < lim; i++) {
        if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 1) {
            if (buf[i + 3] == 0) { (*c8)++; break; }
            if (buf[i + 3] == 0xb7) { found_end = 1; break; }
        }
    }
    if (len < 3) {
        tail[0] = buf[len]; tail[1] = buf[len + 1]; tail[2] = buf[len + 2];
    } else {
        tail[0] = data[len - 3]; tail[1] = data[len - 2]; tail[2] = data[len - 1];
    }
    while (len >= 4) {
        const uint8_t *hit = find_psc(data, len);
        if (!hit) break;
        if (sc_class(hit) & 4) (*c8)++;
        else found_end = 1;
        len -= (int32_t)(hit + 4 - data);
        data = hit + 4;
    }
    if (found_end && func_00515c60(sfd) == 0 && func_00515cc8(sfd) == 0) SFMPVP_Flush(sfd, 1);
}

/* ---- Create / Destroy ------------------------------------------------------------------------ */

typedef struct Work {
    P4Mpeg2 *dec;
    int64_t out_in_seq;   /* pictures output since the sequence started */
} Work;

static Work *work_of(uint32_t mp, int create) {
    Work *w = (Work *)P4_G2H(*W(mp + 0x40));
    if (!w && create) {
        w = p4pc_heap_calloc(1, sizeof *w); /* the libmpeg object points at it: guest memory */
        if (w) {
            w->dec = p4mpeg2_open();
            if (!w->dec) { p4pc_heap_free(w); w = NULL; }
        }
        *W(mp + 0x40) = P4_H2G(w);
    }
    return w;
}

void func_005208c8(void *sfd) {
    Obj o = obj_of(sfd);
    uint32_t P = o.p;
    if (G_USE_SCEMPEG == 0) { *W(o.ctx) = 0; return; }
    int32_t w = *W(P + 0xc), h = *W(P + 0x10);
    int32_t s4 = (int32_t)(((int64_t)w * h * 9) / 2) + 10240;
    if (*W(P + 0x28) == 0) {
        *W(P + 0x28) = (*W(P + 0x14) + 63) & ~63;
        *W(P + 0x2c) = (*W(P + 0x28) + s4 + 63) & ~63;
    } else {
        *W(P + 0x28) = (*W(P + 0x28) + 63) & ~63;
        *W(P + 0x2c) = (*W(P + 0x2c) + 63) & ~63;
    }
    uint32_t ring = (uint32_t)*W(P + 0x2c);
    uint32_t tags = (ring + 0x8003f) & ~63u;
    uint32_t entries = (tags + 4175) & ~63u;
    if (G_KEEP_MPEG == 1) {
        memcpy(U8(o.mp), G_SHADOW, 72);           /* func_00520da0 */
    } else {
        memset(U8(o.mp), 0, 0x48);                /* sceMpegCreate: the libmpeg object starts empty */
        work_of(o.mp, 1);
    }
    viBufCreate((void *)P4_G2H(o.vb), ring, tags, 256, entries, 512);
    viBufAddDMA((void *)P4_G2H(o.vb));
    viBufStopDMA((void *)P4_G2H(o.vb));
    U8(o.ctx)[0x10] = U8(o.ctx)[0x11] = U8(o.ctx)[0x12] = 0;
    *W(o.ctx) = 1;
    *W(o.ctx + 0x20) = 0;
    *W(o.err) = -1;
    *W(o.err + 4) = 0;
    *W(o.ctx + 4) = 0;
    *W(o.ctx + 8) = 0;
    *W(o.ctx + 0xc) = 0;
    *W(o.ctx + 0x14) = 0;
    *W(o.ctx + 0x18) = 0;
    *W(o.ctx + 0x1c) = 0;
}

void SFMPVP_Destroy(void *sfd) {
    if (!SFMPVP_IsSceMpegUsed(sfd)) return;
    Obj o = obj_of(sfd);
    viBufDelete((void *)P4_G2H(o.vb));
    Work *w = work_of(o.mp, 0);
    if (G_KEEP_MPEG == 1) {
        if (w) p4mpeg2_reset(w->dec);             /* sceMpegReset keeps the decoder for the next movie */
        *W(o.mp + 8) = 0;
        memcpy(G_SHADOW, U8(o.mp), 72);           /* func_00520d78 */
    } else if (w) {
        p4mpeg2_close(w->dec);
        p4pc_heap_free(w);
        *W(o.mp + 0x40) = 0;
    }
}

/* ---- Decode ---------------------------------------------------------------------------------- */

static const char kMsgSize[] = "sceMpeg: picture larger than the output area";
static const char kMsgDecode[] = "sceMpeg: decode error";

static void report_error(void *sfd, Obj o, const char *msg) {
    G_ERR_FLAG = 1;
    uint32_t rec[2] = {1, p4_gport_str(msg)}; /* the message word is a guest address */
    memcpy(G_ERR_REC, rec, 8);
    memcpy(U8(o.err), rec, 8);
    func_00517ad0(sfd, (int32_t)0xff000f40);
}

/* Ring byte at stream offset `abs`; off0 = (from % size) lets the scan stay in 32-bit arithmetic. */
static inline uint8_t ring_at(const ViBuf *v, uint32_t off0, uint32_t i) {
    return U8(v->base)[(off0 + i) % (uint32_t)v->size];
}

typedef enum { UNIT_NEED_DATA, UNIT_PICTURE, UNIT_SEQ_END } UnitKind;

/* Finds the end of the next decode unit in the queued data [from, end_q); *len is its length
 * (to the start code that ends it, which the IPU leaves unread). */
static UnitKind find_unit(const ViBuf *v, int64_t from, int64_t end_q, uint32_t *len) {
    const uint32_t off0 = (uint32_t)(from % v->size);
    const uint32_t avail = (uint32_t)(end_q - from);
    int seen_pic = 0;
    for (uint32_t i = 0; i + 4 <= avail; i++) {
        if (ring_at(v, off0, i) || ring_at(v, off0, i + 1) || ring_at(v, off0, i + 2) != 1) continue;
        uint8_t code = ring_at(v, off0, i + 3);
        if (code == 0xb7) { *len = i; return seen_pic ? UNIT_PICTURE : UNIT_SEQ_END; }
        if (code == 0x00) {
            if (seen_pic) { *len = i; return UNIT_PICTURE; }
            seen_pic = 1;
        } else if ((code == 0xb3 || code == 0xb8) && seen_pic) {
            *len = i;
            return UNIT_PICTURE;
        }
        i += 3;
    }
    return UNIT_NEED_DATA;
}

static const uint8_t kTerminator[16] = {0, 0, 1, 0xb7, 0, 0, 1, 0xb7, 0, 0, 1, 0xb7, 0, 0, 1, 0xb7};

/* The libmpeg no-data callback (0x00520bf8). Returns 1 if more data is queued afterwards. */
static int nodata(void *sfd, Obj o) {
    ViBuf *v = (ViBuf *)P4_G2H(o.vb);
    VbState *s = vb_state(o.vb, 0);
    if (*W(o.ctx + 0x18) == 1) return 0;
    int64_t before = s->eend;
    s->starved = 1;   /* the IPU has drained everything the DMA delivered */
    viBufAddDMA(v);
    if (*W(o.ctx + 0x14) == 0) return s->eend > before;
    if (v->q != 0) return s->eend > before;
    *W(o.ctx + 0x18) = 1;
    sfmpvp_PutVibuf(v, kTerminator, 16);
    v->r = v->size;
    viBufAddDMA(v);
    (void)sfd;
    return s->eend > before;
}

/* sceMpegGetPicture / sceMpegGetPictureRAW8: decodes until one picture is output. */
static void get_picture(void *sfd, Obj o, uint32_t dst, int32_t n, int raw8) {
    ViBuf *v = (ViBuf *)P4_G2H(o.vb);
    VbState *s = vb_state(o.vb, 0);
    Work *w = work_of(o.mp, 1);
    if (!s || !w) { report_error(sfd, o, kMsgDecode); *W(o.ctx + 0x18) = 1; return; }
    uint8_t *unit = NULL;
    size_t unit_cap = 0;
    int seq_ended = 0;
    for (;;) {
        P4Mpeg2Frame f;
        if (p4mpeg2_receive(w->dec, &f)) {
            int32_t mbs = ((f.width + 15) / 16) * ((f.height + 15) / 16);
            if (mbs > n) { report_error(sfd, o, kMsgSize); break; }
            uint8_t *d = U8(dst & 0x0fffffffu);
            if (raw8 ? p4mpeg2_to_ipu_raw8(&f, d) : p4mpeg2_to_ipu_rgb32(w->dec, &f, d)) report_error(sfd, o, kMsgDecode);
            w->out_in_seq++;
            if (seq_ended) { *W(o.mp + 8) = (int32_t)(w->out_in_seq - 1); w->out_in_seq = 0; }
            break;
        }
        if (seq_ended) {   /* nothing held at the sequence end: libmpeg returns without output */
            if (w->out_in_seq > 0) *W(o.mp + 8) = (int32_t)(w->out_in_seq - 1);
            w->out_in_seq = 0;
            break;
        }
        uint32_t len = 0;
        UnitKind k = find_unit(v, s->c, s->eend << 11, &len);
        if (k == UNIT_NEED_DATA) {
            if (!nodata(sfd, o)) {
                *W(o.ctx + 0x18) = 1;   /* libmpeg aborts: GetPictureAbort */
                break;
            }
            continue;
        }
        if (k == UNIT_SEQ_END) {
            s->c += (int64_t)len + 4;
            p4mpeg2_end_sequence(w->dec);
            seq_ended = 1;
            continue;
        }
        if (len > unit_cap) {
            uint8_t *nb = realloc(unit, len);
            if (!nb) { report_error(sfd, o, kMsgDecode); break; }
            unit = nb;
            unit_cap = len;
        }
        {
            const uint32_t off0 = (uint32_t)(s->c % v->size);
            const uint32_t first = (uint32_t)v->size - off0 < len ? (uint32_t)v->size - off0 : len;
            memcpy(unit, U8(v->base) + off0, first);
            if (first < len) memcpy(unit + first, U8(v->base), len - first);
        }
        s->c += len;
        if (p4mpeg2_send_unit(w->dec, unit, (int)len) < 0) { report_error(sfd, o, kMsgDecode); break; }
    }
    free(unit);
}

int32_t SFMPVP_DecodeFrmSj(void *mpv, void *sj, void *frm, void *sfd) {
    if (!SFMPVP_IsSceMpegUsed(sfd)) return func_00504d08();
    Obj o = obj_of(sfd);
    int32_t status = func_00504d10(mpv, sj, frm);
    viBufRestartDMA((void *)P4_G2H(o.vb));
    viBufAddDMA((void *)P4_G2H(o.vb));
    int32_t fmt = *W(P4_H2G(sfd) + 0x38);
    int32_t nmb = ((*W(o.p + 0xc) + 15) >> 4) * ((*W(o.p + 0x10) + 15) >> 4);
    *W(o.ctx + 0x14) = 1;
    get_picture(sfd, o, *W(P4_H2G(frm) + 0x20), nmb, fmt != 2);
    *W(o.ctx + 0x14) = 0;
    viBufStopDMA((void *)P4_G2H(o.vb));
    sfmpvp_ConsumePsc(sfd);
    return *W(o.ctx + 0x18) == 0 ? status : -3;
}
