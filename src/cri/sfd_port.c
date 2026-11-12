/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* Sofdec (SFD) control and timing functions the decomp has no C for, translated from the retail
 * asm. The SFD object is addressed by byte offsets, as the asm does:
 *   +0x0044 flag word set by buffer updates      +0x0048 object state (3 and 4 = running)
 *   +0x1310 + n*0x74  SFBUF slot n  (+4 in use, +0x14 ring handle, +0x30/+0x34 byte counters;
 *                                    the words at +8 and +0xc are the term flags)
 *   +0x135c/+0x1360   per-slot transfer-driver links
 *   +0x1f28 + n*0x44  SFD_TR transfer slot n
 *   +0x2150           user-joint block: +8 node array, +0x10 node count
 *   +0x20d8           current buffer id of the video-out driver
 * Raw entries retain their retail names (func_XXXXXXXX). Canonical SDK helpers are reused only
 * where their argument classes and relative layouts agree with retail; remaining gaps are in
 * docs/asm_worklist.md.
 */
#include <stdint.h>
#include <string.h>

#include "cri_mem.h"
#include "p4pc/ps2_libc.h"

#define SFD_U32(sfd, off) (*(uint32_t *)((uint8_t *)(sfd) + (off)))
#define SFD_I32(sfd, off) (*(int32_t *)((uint8_t *)(sfd) + (off)))

#define SFBUF_BASE 0x1310
#define SFBUF_STRIDE 0x74
#define SFD_TR_BASE 0x1f28
#define SFD_TR_STRIDE 0x44

extern int32_t func_00514bd8(void *sfd, int32_t n, int32_t idx, void *node);
extern void func_00514ee8(void *ring);
extern void func_00514f78(void *ring, int32_t mode, Ck *ck0, Ck *ck1);
extern int32_t func_00515070(void *ring, int32_t mode, ...);
extern int32_t func_00515870(void *sfd, int32_t bufid, void *out, int32_t a);
extern void func_005158d0(void *sfd, int32_t bufid, int32_t a, int32_t b);
extern int32_t func_00517ad0(void *sfd, int32_t err);
extern int32_t func_00517cf0(void *sfd);
extern int32_t SFSET_GetCond(void *sfd, int32_t drv);
extern void func_005278b0(void *sfd, int32_t a, int32_t b, int32_t c, int32_t d);
extern void func_005286d8(void *sfd);
extern int32_t func_005287e8(void *sfd);
extern void func_00528760(void *sfd);
extern void func_00528820(void *sfd, void *nodes, int32_t n);
extern void func_005288c0(void *node, int32_t a, int32_t b, int32_t c, int32_t d);
extern void func_005289f0(void *sfd);
extern int32_t func_00528b40(void *sfd);
extern void func_00528ab8(void *sfd);
extern void func_004ca990(void *adxt, int32_t a, int32_t b, int32_t c);
extern void func_004d6510(void *adxt, int32_t a);
extern void func_004cd990(void);
extern void func_004ca8b0(void);
extern void func_005125f8(void *sfd);
extern void func_00513b90(void *sfd, int32_t mode);
extern void func_00513be0(void *sfd, int32_t mode);
extern int32_t func_00513380(void *adxt);
extern int32_t func_004d4bd0(void *adxt);
extern void func_00513c68(void *sfd, int32_t *out, int32_t sfreq);
extern int32_t func_004d63a0(void *adxt, int32_t v);
extern void func_00527b28(void *tm, const int64_t pair[2]);
extern void SFSET_SetCond(void *sfd, int32_t id, int32_t value);
extern int32_t SFTIM_GetAudioStartSample(void *tim, int32_t sfreq);
extern int32_t SFTIM_GetVideoStartSample(void *tim, int32_t sfreq, int32_t *known);
extern void SFTIM_SetStartTime(void *tim, int32_t sample, int32_t sfreq);
extern int32_t ADXT_IsHeader(uint8_t *data, int32_t len, int32_t *header_size);
extern int32_t ADXT_IsEndcode(uint8_t *data, int32_t len, int32_t *end_size);
extern int32_t sfadxt_SearchEndcode(uint8_t *data, int32_t len, int32_t *found);
extern int32_t func_004d6910(void *adxt, int32_t nch, int32_t nsmpl);
extern void MEM_Copy(void *dst, const void *src, uint32_t len);
extern void SJ_SplitChunk(void *chunk, int32_t len, void *head, void *tail);

/* ---- SFBUF slot accessors ------------------------------------------------------------------ */

/* 0x00515940: setter for the word at +0x08 of slot n. */
void func_00515940(void *sfd, int32_t n, uint32_t v)
{
    SFD_U32(sfd, n * SFBUF_STRIDE + 0x1318) = v;
}

/* 0x00514cc0: first and second chunk readable from slot n's ring in `mode`, as
 * out[0]=ck0.data, [1]=ck0.len, [2]=ck1.data, [3]=ck1.len; the other three words are cleared. */
int32_t func_00514cc0(void *sfd, int32_t n, uint32_t *out, int32_t mode)
{
    uint8_t *slot = (uint8_t *)sfd + SFBUF_BASE + n * SFBUF_STRIDE;
    Ck ck0, ck1;
    void *ring = P4_G2H(*(uint32_t *)(slot + 0x14));

    out[1] = 0;
    out[3] = 0;
    out[5] = 0;
    out[6] = 0;
    out[0] = 0;
    out[2] = 0;
    out[4] = 0;
    if (*(int32_t *)(slot + 4) != 0 && ring != 0) {
        func_00514f78(ring, mode, &ck0, &ck1);
        out[0] = ck0.data;
        out[3] = (uint32_t)ck1.len;
        out[1] = (uint32_t)ck0.len;
        out[2] = ck1.data;
    }
    return 0;
}

/* 0x00514d98: advance slot n's read (mode != 1) or write (mode 1) position by nbyte. Reports error
 * 0xff00040b when the ring holds fewer than nbyte bytes. */
int32_t func_00514d98(void *sfd, int32_t n, int32_t nbyte, int32_t mode)
{
    uint8_t *slot = (uint8_t *)sfd + SFBUF_BASE + n * SFBUF_STRIDE;
    void *ring = P4_G2H(*(uint32_t *)(slot + 0x14));
    int32_t ret = 0;
    int32_t have, v;

    if (nbyte == 0 || *(int32_t *)(slot + 4) == 0 || ring == 0) {
        return 0;
    }
    have = func_00515070(ring, mode);
    if (have < nbyte) {
        int32_t rem = nbyte - have;

        if (func_00515070(ring, mode, rem) < rem) {
            ret = func_00517ad0(sfd, 0xff00040b);
        }
    }
    if (mode != 1) {
        v = *(int32_t *)(slot + 0x30);
        if (v >= 0) {
            *(int32_t *)(slot + 0x30) = v + nbyte;
        }
    } else {
        if (n == 1) {
            func_00514ee8(slot + 0x10);
        }
        v = *(int32_t *)(slot + 0x34);
        if (v >= 0) {
            *(int32_t *)(slot + 0x34) = v + nbyte;
        }
    }
    SFD_U32(sfd, 0x44) = 1;
    return ret;
}

/* 0x005142d0 */
void func_005142d0(void *sfd, int32_t a)
{
    if (SFSET_GetCond(sfd, 6) != 0) {
        func_005278b0(sfd, 3, 8, a, 0);
    }
}

/* ---- transfer-driver links ------------------------------------------------------------------ */

/* 0x005277e0 sftrn_ConnBufTrn: transfer slot `tr` takes buffer `buf` as its output (+0x1f40);
 * the buffer links back (+0x1360). */
void func_005277e0(void *sfd, int32_t buf, int32_t tr)
{
    SFD_I32(sfd, tr * SFD_TR_STRIDE + 8000) = buf;
    SFD_I32(sfd, buf * SFBUF_STRIDE + 0x1360) = tr;
}

/* 0x00527aa0 */
void func_00527aa0(uint32_t *p, uint32_t v)
{
    *p = v;
}

/* ---- timecode to time conversion (SFTIM) ---------------------------------------------------
 * `tm` is the SFD timecode record: +8 hours, +0xc minutes, +0x10 seconds, +0x14 and +0x18 two
 * millisecond parts, +0x1e frames. Each variant yields a time in units of 1/1000 * rate. */

typedef struct {
    uint8_t pad0[8];
    int32_t hour;
    int32_t min;
    int32_t sec;
    int32_t ms0;
    int32_t ms1;
    uint8_t pad1c[2];
    int16_t frame;
} P4SfTm;

/* The three drop-frame variants differ in the frame-rate constants (12/0x15114/0x59e,
 * 15/0x1a574/0x706, 30/0x34b54/0xe0e). */
#define SFTIM_DROP(name, sec_mul, hour_mul, min_mul) \
    void name(uint32_t rate, const P4SfTm *tm, int32_t *out, uint32_t *out_rate) \
    { \
        *out = (int32_t)(((uint32_t)(tm->sec * (sec_mul) + tm->min / 10) * 2u \
                          + (uint32_t)tm->hour * (hour_mul) + (uint32_t)tm->min * (min_mul) \
                          + (uint32_t)tm->ms0 + (uint32_t)tm->ms1) * 1000u \
                         + (uint32_t)(tm->frame * 0x7c + tm->frame) * 4u); \
        *out_rate = rate; \
    }

SFTIM_DROP(func_005266d8, 0xc, 0x15114u, 0x59eu)
SFTIM_DROP(func_00526778, 0xf, 0x1a574u, 0x706u)
SFTIM_DROP(func_00526810, 0x1e, 0x34b54u, 0xe0eu)

/* ---- user output driver (SFUO) and video-out manual driver (SFVOM) --------------------------- */

/* 0x00528570 SFD_SetUsrSj(sfd, idx, a, b): bind user joint node idx; error 0xff000191 when the
 * library is not initialised, 0xff000602 when the driver slots are full. */
int32_t func_00528570(void *sfd, int32_t idx, int32_t a, int32_t b)
{
    uint8_t *blk = (uint8_t *)sfd + 0x2150;
    int32_t n;
    uint8_t *node;

    if (func_00517cf0(sfd) != 0) {
        return func_00517ad0(0, 0xff000191);
    }
    n = *(int32_t *)(blk + 0x10);
    if (n == 8) {
        return func_00517ad0(sfd, 0xff000602);
    }
    node = (uint8_t *)P4_G2H(*(uint32_t *)(blk + 8)) + idx * 0x10 + 4;
    func_005288c0(node, a, b, 0, 0);
    func_00514bd8(sfd, n, idx, node);
    return 0;
}

/* 0x00528690 SFUO_ExecServer */
int32_t func_00528690(void *sfd)
{
    int32_t ret;

    func_005286d8(sfd);
    ret = func_005287e8(sfd);
    func_00528760(sfd);
    return ret;
}

/* 0x005287f0 SFUO_Create */
int32_t func_005287f0(void *sfd)
{
    uint8_t *blk = (uint8_t *)sfd + 0x2150;

    *(uint32_t *)(blk + 8) = P4_H2G((uint8_t *)sfd + 0x3678);
    func_00528820(sfd, (uint8_t *)sfd + 0x3678, *(int32_t *)(blk + 0x10));
    return 0;
}

/* 0x00528998 SFVOM_ExecServer */
int32_t func_00528998(void *sfd)
{
    int32_t ret = 0;

    if (SFSET_GetCond(sfd, 5) != 0) {
        func_005289f0(sfd);
        ret = func_00528b40(sfd);
        func_00528ab8(sfd);
    }
    return ret;
}

/* 0x00528b78 and 0x00528b98: unsupported SFVOM entries, error 0xff000701. */
void func_00528b78(void *sfd)
{
    func_00517ad0(sfd, 0xff000701);
}

void func_00528b98(void *sfd)
{
    func_00517ad0(sfd, 0xff000701);
}

/* 0x00528bb8 SFVOM_GetRead: while the object is in state 3 or 4 read from the video-out buffer
 * (+0x20d8), otherwise report no data. */
int32_t func_00528bb8(void *sfd, uint32_t *out, int32_t a)
{
    if (SFD_U32(sfd, 0x48) - 3u >= 2u) {
        *out = 0;
        return 0;
    }
    return func_00515870(sfd, SFD_I32(sfd, 0x20d8), out, a);
}

/* 0x00528c00 SFVOM_AddRead */
void func_00528c00(void *sfd, int32_t a, int32_t b)
{
    func_005158d0(sfd, SFD_I32(sfd, 0x20d8), a, b);
}

/* ---- audio output through ADXT (SFAOAP) and ADXT transfer (SFADXT) -------------------------
 * The SFD object holds a driver block pointer at +0x2004 (word 0 is the ADXT handle, +0x30 a
 * running sample count, +0x34 a started flag, +0x3c the ADXT-ready callback). */

/* 0x00517b40: retain the first code, but report the current code to the
 * callback. Its second argument is live a1 at the retail jalr. */
void func_00517b40(void *error, int32_t code)
{
    if (SFD_I32(error, 8) == 0)
        SFD_I32(error, 8) = code;
    if (code != 0) {
        uint32_t callback = SFD_U32(error, 0);
        if (callback != 0) {
            ((void (*)(void *, int32_t))p4_fn_g2h_sig(
                callback, P4_SIG(2, 0, 1, 0, 0, 0, 0, 0)))(
                P4_G2H(SFD_U32(error, 4)), code);
        }
    }
}

/* 0x00517ad0: the SDK's i386 library-wide error is host BSS, so use the
 * actual guest record at 745a5c rather than aliasing the SDK SFD library. */
int32_t func_00517ad0(void *sfd, int32_t code)
{
    if (code == 0)
        return 0;
    if (sfd == 0) {
        func_00517b40(P4_G2H(0x00745a5c), code);
    } else {
        func_00517b40((uint8_t *)sfd + 0x9f8, code);
        if (SFD_I32(sfd, 0x48) > 0)
            SFD_I32(sfd, 0x48) = -SFD_I32(sfd, 0x48);
    }
    return code;
}

/* 0x00517840: the mux version is unknown until the file header is valid. */
int32_t func_00517840(void *sfd)
{
    if (SFD_I32(sfd, 0x78) == 0)
        return 0;
    return SFD_I32(sfd, 0x7c) * 100 + SFD_I32(sfd, 0x80);
}

/* 0x00513dc8: the retail SEE work and seek flag, not the RE4 SFD layout. */
uint32_t *func_00513dc8(void *sfd)
{
    uint32_t see = SFD_U32(sfd, 0x36b0);
    void *work;

    if (see == 0)
        return 0;
    work = P4_G2H(SFD_U32(sfd, 0x2004));
    if (SFD_I32(work, 0x40) > 0)
        return 0;
    return (uint32_t *)((uint8_t *)P4_G2H(see) + 0xd0c);
}

/* 0x00512ea8: failed header lookup leaves both output words untouched. */
int32_t func_00512ea8(void *sfd, int32_t *nch, int32_t *sfreq)
{
    uint32_t *header = func_00513dc8(sfd);

    if (header == 0)
        return -1;
    *nch = (int32_t)header[3];
    *sfreq = (int32_t)header[4];
    return 0;
}

/* 0x00512730 / 0x00512760: stream-joint chunk methods use guest-pointer
 * pairs. Keep the actual SJ method order, including the zero-length unget. */
void func_00512730(void *sj, int32_t len, Ck *chunk)
{
    sj_get(sj, 0, len, chunk);
}

void func_00512760(void *sj, int32_t len, Ck *chunk)
{
    Ck rest;

    if (len == 0) {
        sj_unget(sj, 0, chunk);
    } else {
        SJ_SplitChunk(chunk, len, chunk, &rest);
        sj_put(sj, 1, chunk);
        sj_unget(sj, 0, &rest);
    }
}

/* 0x005127e8: the terminal transfer callback copies at most 100 KiB. */
void func_005127e8(void *sfd, uint8_t *data, int32_t len, int32_t *nbyte)
{
    void *work = P4_G2H(SFD_U32(sfd, 0x2004));
    void *sj = P4_G2H(SFD_U32(work, 4));
    Ck chunk;
    int32_t copied;

    func_00512730(sj, SFD_I32(work, 8), &chunk);
    copied = len < chunk.len ? len : chunk.len;
    if (copied > 0x19000)
        copied = 0x19000;
    MEM_Copy(P4_G2H(chunk.data), data, (uint32_t)copied);
    func_00512760(sj, copied, &chunk);
    SFD_I32(work, 0x48) += copied;
    *nbyte = copied;
}

/* 0x00512b90: preserve all three live arguments of ADXT_InsertSilence. */
int32_t func_00512b90(void *sfd, int32_t nch, int32_t nsmpl)
{
    void *work = P4_G2H(SFD_U32(sfd, 0x2004));
    return func_004d6910(P4_G2H(SFD_U32(work, 0)), nch, nsmpl);
}

/* 0x005128a0: align audio samples with the retail timer at +0xd30. The
 * canonical timer helpers take host pointers; the numeric 00525c30 body
 * instead takes a 32-bit guest address and must not receive this pointer. */
void func_005128a0(void *sfd, uint8_t *data, int32_t len, int32_t *nbyte)
{
    void *work;
    void *tim = (uint8_t *)sfd + 0xd30;
    int32_t nch, sfreq, video_known;
    int32_t audio_start, video_start, diff;
    int32_t skip = 0;
    uint64_t bytes;

    *nbyte = 0;
    work = P4_G2H(SFD_U32(sfd, 0x2004));
    if (func_00512ea8(sfd, &nch, &sfreq) != 0) {
        SFD_U32(work, 0x3c) = P4_FN_H2G(func_005127e8);
        return;
    }
    audio_start = SFTIM_GetAudioStartSample(tim, sfreq);
    if (audio_start < 0)
        return;
    if (SFSET_GetCond(sfd, 5) == 0) {
        SFTIM_SetStartTime(tim, audio_start, sfreq);
        SFD_U32(work, 0x3c) = P4_FN_H2G(func_005127e8);
        return;
    }
    video_start = SFTIM_GetVideoStartSample(tim, sfreq, &video_known);
    if (video_start < 0)
        return;
    SFTIM_SetStartTime(tim, video_start, sfreq);
    diff = video_start - audio_start - SFD_I32(work, 0x38);
    if (diff >= 0) {
        int32_t end = 0;
        int32_t remaining = (diff >> 5) * nch * 18;

        if (remaining > 0) {
            int32_t frame_bytes = nch * 18;
            int32_t limit = len / frame_bytes * frame_bytes;

            if (remaining < limit)
                limit = remaining;
            skip = sfadxt_SearchEndcode(data, limit, &end);
            SFD_I32(work, 0x38) += skip / frame_bytes * 32;
            remaining -= limit;
        }
        if (remaining <= 0 && video_known != 0) {
            int32_t end_size;
            SFD_U32(work, 0x3c) = P4_FN_H2G(func_005127e8);
            end = ADXT_IsEndcode(data, len, &end_size);
        }
        if (end != 0)
            SFSET_SetCond(sfd, 6, 0);
    } else if (video_known != 0) {
        int32_t count = (-diff) / 32 * 32;

        if (count > 0) {
            int32_t inserted = func_00512b90(sfd, nch, count);
            count -= inserted;
            SFD_I32(work, 0x38) -= inserted;
        }
        if (count <= 0)
            SFD_U32(work, 0x3c) = P4_FN_H2G(func_005127e8);
    }
    bytes = *(uint64_t *)((uint8_t *)sfd + 0x9c8);
    *nbyte = skip;
    *(uint64_t *)((uint8_t *)sfd + 0x9c8) = bytes + skip;
}

/* 0x00512c70: unlike the SDK helper's byte count, the retail helper returns
 * a pointer to the selected frame phase. End-code ties retain the first hit. */
uint8_t *func_00512c70(void *sfd, uint8_t *data, int32_t len)
{
    uint8_t *phase;
    uint8_t *found = 0;
    uint8_t *found_phase = 0;
    int32_t hit = 0;

    for (phase = data; phase < data + 36; phase += 2) {
        uint8_t *p;
        hit = 0;
        for (p = phase; p < data + len; p += 18) {
            if ((int8_t)*p < 0) {
                int32_t end_size;
                hit = 1;
                if (ADXT_IsEndcode(p, 18, &end_size) &&
                    (uintptr_t)found < (uintptr_t)p) {
                    found = p;
                    found_phase = phase;
                }
                break;
            }
        }
        if (hit == 0)
            break;
    }
    if (hit != 0) {
        if (found_phase == 0) {
            func_00517ad0(sfd, 0xff000c0a);
            phase = data;
        } else {
            phase = found_phase;
        }
    }
    return phase;
}

/* 0x00512bb0: header exclusion always advances to AdjustSync once a
 * 288-byte input is available, even when a modern mux has no ADX header. */
void func_00512bb0(void *sfd, uint8_t *data, int32_t len, int32_t *nbyte)
{
    void *work;
    int32_t header_size, skip;
    uint64_t bytes;

    *nbyte = 0;
    work = P4_G2H(SFD_U32(sfd, 0x2004));
    if (len < 288)
        return;
    if (ADXT_IsHeader(data, len, &header_size))
        skip = header_size;
    else if (func_00517840(sfd) >= 108)
        skip = 0;
    else
        skip = (int32_t)(func_00512c70(sfd, data, len) - data);
    bytes = *(uint64_t *)((uint8_t *)sfd + 0x9c8);
    *nbyte = skip;
    SFD_U32(work, 0x3c) = P4_FN_H2G(func_005128a0);
    *(uint64_t *)((uint8_t *)sfd + 0x9c8) = bytes + skip;
}

/* 0x00512d90: old muxes can begin with 18-byte silence records. The last
 * complete record is deliberately retained, and a non-silence record is not
 * consumed. The byte counter is the retail 64-bit field at +0x9c8. */
void func_00512d90(void *sfd, uint8_t *data, int32_t len, int32_t *nbyte)
{
    void *work;
    int32_t offset = 0;
    int32_t nch, sfreq;
    uint64_t bytes;

    *nbyte = 0;
    work = P4_G2H(SFD_U32(sfd, 0x2004));
    if (func_00517840(sfd) >= 108) {
        SFD_U32(work, 0x3c) = P4_FN_H2G(func_00512bb0);
        return;
    }
    while (offset < len - 18) {
        if (func_0043f778(data + offset, P4_G2H(0x007601d0), 18) != 0) {
            SFD_U32(work, 0x3c) = P4_FN_H2G(func_00512bb0);
            break;
        }
        offset += 18;
    }
    bytes = *(uint64_t *)((uint8_t *)sfd + 0x9c8);
    *nbyte = offset;
    *(uint64_t *)((uint8_t *)sfd + 0x9c8) = bytes + offset;
    if (func_00512ea8(sfd, &nch, &sfreq) == 0)
        SFD_I32(work, 0x38) += offset / (nch * 18) * 32;
}

/* 0x00513d38 SFAOAP_Create-style start: when the driver block has not started and a stream
 * descriptor exists, hand the ADXT handle its stream parameters and install the callback. */
int32_t func_00513d38(void *sfd)
{
    uint32_t *desc = func_00513dc8(sfd);

    if (desc != 0) {
        uint32_t *blk = (uint32_t *)P4_G2H(SFD_U32(sfd, 0x2004));
        void *adxt = P4_G2H(blk[0]);

        if (blk[0xd] == 0 && desc[0] != 0) {
            func_004ca990(adxt, (int32_t)desc[3], (int32_t)desc[4], (int32_t)desc[5]);
            func_004d6510(adxt, 0);
            blk[0xd] = 1;
            blk[0xf] = P4_FN_H2G(func_00512d90);
        }
    }
    return 0;
}

/* 0x005141a8: with driver 6 present, point +0x2114 at the block at +0x365c (the stub at
 * 0x005141e8 returns immediately and is not called). */
int32_t func_005141a8(void *sfd)
{
    if (SFSET_GetCond(sfd, 6) != 0) {
        SFD_U32(sfd, 0x2114) = P4_H2G((uint8_t *)sfd + 0x365c);
    }
    return 0;
}

/* 0x005141f8, 0x00514240, 0x00514288: with driver 6 present, post message 5, 6 or 7 to the
 * transfer queue. */
void func_005141f8(void *sfd)
{
    if (SFSET_GetCond(sfd, 6) != 0) {
        func_005278b0(sfd, 3, 5, 0, 0);
    }
}

void func_00514240(void *sfd)
{
    if (SFSET_GetCond(sfd, 6) != 0) {
        func_005278b0(sfd, 3, 6, 0, 0);
    }
}

void func_00514288(void *sfd)
{
    if (SFSET_GetCond(sfd, 6) != 0) {
        func_005278b0(sfd, 3, 7, 0, 0);
    }
}

/* 0x00512528: ADXT library init: ADXT subsystem init, then finish helper (no arguments). */
int32_t func_00512528(void)
{
    func_004cd990();
    func_004ca8b0();
    return 0;
}

/* 0x005125e0 */
void func_005125e0(void *sfd)
{
    func_005125f8(sfd);
}

/* 0x00513a58 */
int32_t func_00513a58(void *sfd)
{
    func_00513b90(sfd, 0);
    return 0;
}

/* 0x00513aa0 SFADXT mode switch: 0 and 1 restart the play state (0 also clears the sample
 * count), 2 advances the sample count by the decoded amount and posts it to the timer. */
int32_t func_00513aa0(void *sfd, int32_t mode)
{
    uint32_t *blk = (uint32_t *)P4_G2H(SFD_U32(sfd, 0x2004));
    void *adxt = P4_G2H(blk[0]);

    if (mode == 1) {
        func_00513be0(sfd, 1);
    } else if (mode == 0) {
        blk[0xc] = 0;
        func_00513be0(sfd, 0);
    } else if (mode == 2) {
        int32_t tmp[4];
        int32_t sfreq, cnt, used;
        int64_t pair[2];

        if (func_00513380(adxt) == 0) {
            return 0;
        }
        sfreq = func_004d4bd0(adxt);
        func_00513c68(sfd, tmp, sfreq);
        cnt = tmp[0] + (int32_t)blk[0xc];
        used = func_004d63a0(adxt, cnt);
        blk[0xc] = (uint32_t)(cnt - used);
        pair[0] = tmp[0];
        pair[1] = sfreq;
        func_00527b28((uint8_t *)sfd + 0x1020, pair);
    }
    return 0;
}
