/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI AHX decoder (libadxe ahx_bsr / ahx_dcd / ahx_link / ahx_sjd), translated from the retail asm
 * 0x004d9348..0x004de2b0. AHX is an MPEG audio layer II style voice codec: a 96 sample frame holds
 * three groups of 32 sub-band samples, a bit allocation, scale factors and quantised samples in the
 * ADX header's container. The decoder keeps the retail data layout: its object tables and its constant
 * tables live in the retail image at their link addresses (the objects are carved out of the work
 * areas the game hands to ACSSND_AttachAhx), so every offset below is the retail one.
 *
 *   AHXBSR  bit reader on a stream joint, 0x2c bytes, 16 slots at 0x00728330
 *   AHXDCD  frame decoder, 0xa28 bytes of work, 16 slots at 0x007288e8
 *   AHXSBF  synthesis filter state, 0x1014 bytes, 16 slots at 0x00729998
 *   AHXSJD  stream joint decoder, 0x50 bytes + work, 16 slots at 0x00729a40
 *
 * The synthesis filter bank (ahxsbf_mult_flt_int_ex and the sixteen ahxsbf_mult_win rows) is in
 * ahx_sbf.c; ADXT_AttachAHX and the ADXB hooks are at the end of this file. */
#include <string.h>

#include "cri_mem.h"

extern void func_0043f9c8(void *dst, int32_t c, uint32_t n);     /* memset */
extern uint32_t func_00442948(const char *s);                     /* strlen */
extern int32_t ADX_DecodeHeader(const uint8_t *ibuf, int32_t len, int32_t *hdrlen, void *out);
extern void SJ_SplitChunk(void *ck, int32_t n, void *ck1, void *ck2);
extern uint32_t func_004d9ae0(void);                              /* AHX library use count + 1 */
extern uint32_t func_004dd608(void);                              /* the retail profiling clock: 0 */
extern void *func_004dd768(uint8_t *work, uint32_t size);         /* synthesis filter state create */
extern void func_004dd840(uint8_t *sbf);                           /* ... and free */
extern void func_004dd880(uint8_t *sbf, uint8_t *in, int32_t ch, int16_t *out);
extern void func_004dd620(void);
extern void func_004dd718(void);
extern void func_004d9478(void *bsr);                             /* AHXBSR_Destroy */
extern void func_004ddc98(uint32_t h, uint32_t sj);                  /* AHXSJD_SetInSj (decomp: register words) */

/* ---- AHXBSR: the bit reader ------------------------------------------------------------------
 * +0 used, +4 joint, +8 bit accumulator, +0xc bits in it, +0x10 bits consumed, +0x14 bytes in the joint at
 * creation, +0x18 a quarter of that (the largest chunk asked for), +0x1c the current chunk (data,
 * length), +0x24 bytes left in it, +0x28 its read pointer. */
#define BSR_TABLE 0x00728330u
#define BSR_MASKS 0x007282a8u /* mask of n bits, one word per n */
#define BSR_COUNT 0x007282a4u /* reads, a statistic */

/* 0x004d9348 */
void *func_004d9348(void *sj)
{
    int32_t i, n1, n0;
    uint8_t *b;

    for (i = 0; i < 16; i++) {
        if (M32(BSR_TABLE + i * 0x2c) == 0) {
            break;
        }
    }
    if (i == 16) {
        return 0;
    }
    b = (uint8_t *)P4_G2H(BSR_TABLE + i * 0x2c);
    func_0043f9c8(b, 0, 0x2c);
    P32(b, 4) = P4_H2G(sj);
    P32(b, 0x10) = 0;
    n1 = sj_num(sj, 1);
    n0 = sj_num(sj, 0);
    P32(b, 0x24) = 0;
    n1 += n0;
    S32(b, 0x14) = n1;
    S32(b, 0x18) = (n1 < 0 ? n1 + 3 : n1) >> 2;
    P32(b, 0) = 1;
    P32(b, 0x1c) = 0;
    P32(b, 0x20) = 0;
    P32(b, 0x28) = 0;
    return b;
}

/* 0x004d94a8: the mask of the low n bits, built a byte and then a bit at a time. */
uint32_t func_004d94a8(int32_t n)
{
    uint32_t m = 0;
    int32_t bytes = (n < 0 ? n + 7 : n) >> 3;
    int32_t bits;

    while (bytes > 0) {
        m = (m << 8) | 0xff;
        bytes--;
    }
    bits = n - (((n < 0 ? n + 7 : n) >> 3) << 3);
    while (bits > 0) {
        m = (m << 1) | 1;
        bits--;
    }
    return m;
}

/* 0x004d9528: top the accumulator up with whole bytes from the joint. A chunk with fewer than four bytes
 * left goes back to the joint (the used part to the free line, the rest to the data line) and the next
 * chunk is taken. */
void func_004d9528(uint8_t *b)
{
    int32_t room = 32 - S32(b, 0xc);
    int32_t n = (room < 0 ? room + 7 : room) >> 3;
    int32_t rem = S32(b, 0x24);
    uint32_t acc;
    uint8_t *p;
    int32_t take;

    if (rem < 4) {
        Ck ck, ck2;
        void *sj = (void *)P4_G2H(P32(b, 4));

        memcpy(&ck, b + 0x1c, 8);
        if (ck.len != 0) {
            SJ_SplitChunk(&ck, ck.len - rem, &ck, &ck2);
            sj_put(sj, 0, &ck);
            sj_unget(sj, 1, &ck2);
        }
        sj_get(sj, 1, S32(b, 0x18), (Ck *)(b + 0x1c));
        P32(b, 0x28) = P32(b, 0x1c);
        P32(b, 0x24) = P32(b, 0x20);
        rem = S32(b, 0x24);
    }
    take = n < rem ? n : rem;
    if (take < 1 || take > 4) {
        return;
    }
    p = (uint8_t *)P4_G2H(P32(b, 0x28));
    acc = P32(b, 8);
    if (take == 4) {
        acc = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
    } else {
        int32_t k;

        for (k = 0; k < take; k++) {
            acc = (acc << 8) | p[k];
        }
    }
    S32(b, 0x24) = rem - take;
    P32(b, 0x28) = P4_H2G(p + take);
    P32(b, 8) = acc;
    S32(b, 0xc) += take * 8;
}

/* 0x004d9768: are n bits there (reading more when not)? */
uint32_t func_004d9768(uint8_t *b, int32_t n)
{
    M32(BSR_COUNT) = M32(BSR_COUNT) + 1;
    if (S32(b, 0xc) < n) {
        func_004d9528(b);
    }
    return n <= S32(b, 0xc);
}

/* 0x004d97c8: read n bits; when the stream ends early, what is left is dropped and 0 returned. */
uint32_t func_004d97c8(uint8_t *b, int32_t n)
{
    int32_t avail;

    M32(BSR_COUNT) = M32(BSR_COUNT) + 1;
    avail = S32(b, 0xc);
    if (avail < n) {
        func_004d9528(b);
        avail = S32(b, 0xc);
        if (avail < n) {
            P32(b, 0x10) = P32(b, 0x10) + avail;
            P32(b, 0xc) = 0;
            return 0;
        }
    }
    P32(b, 0x10) = P32(b, 0x10) + n;
    P32(b, 0xc) = avail - n;
    return (uint32_t)((int32_t)P32(b, 8) >> ((avail - n) & 31)) & M32(BSR_MASKS + n * 4);
}

/* 0x004d9880: AHXBSR_IsEos: nothing left in the joint, in the accumulator or in the chunk. */
uint32_t AHXBSR_IsEos(uint8_t *b)
{
    if (sj_num((void *)P4_G2H(P32(b, 4)), 1) != 0) {
        return 0;
    }
    if (P32(b, 0xc) != 0) {
        return 0;
    }
    return P32(b, 0x24) == 0;
}

/* 0x004d98d8: skip to a byte boundary and read until the low nbits bits equal pat, a byte at a time;
 * 1 when found, 0 at the end of the stream. */
uint32_t func_004d98d8(uint8_t *b, uint32_t pat, int32_t nbits)
{
    uint32_t mask = M32(BSR_MASKS + nbits * 4);
    uint32_t c = P32(b, 0x10);
    uint32_t s;

    if ((c & 7) != 0) {
        func_004d97c8(b, 8 - (int32_t)(c & 7));
    }
    s = func_004d97c8(b, nbits);
    while ((s & mask) != pat) {
        if (AHXBSR_IsEos(b) != 0) {
            break;
        }
        s = (s << 8) | func_004d97c8(b, 8);
    }
    return AHXBSR_IsEos(b) == 0;
}

/* 0x004d9998: find the next frame sync: 1 for a sync word (12 ones, ANDed with the mask at 0x007282d8),
 * 2 for the end marker 0x8001000c, -1 at the end of the stream. */
int32_t func_004d9998(uint8_t *b)
{
    uint32_t mask = M32(0x007282d8);
    uint32_t c = P32(b, 0x10);
    uint32_t s;

    if ((c & 7) != 0) {
        func_004d97c8(b, 8 - (int32_t)(c & 7));
    }
    s = func_004d97c8(b, 12);
    for (;;) {
        if (AHXBSR_IsEos(b) != 0) {
            return -1;
        }
        if ((s & mask) == 0xfff) {
            return 1;
        }
        if (s == 0x8001000cu) {
            return 2;
        }
        s = (s << 4) | func_004d97c8(b, 4);
    }
}

/* 0x004d9a58: entry i of the bit allocation table list: its first word, its address in *out. */
uint32_t func_004d9a58(int32_t i, uint32_t *out)
{
    uint32_t v = M32(0x00725298u + (uint32_t)i * 4);

    *out = 0x007252a0u + (uint32_t)i * 0x3000u;
    return v;
}

/* 0x004d9a90, 0x004d9aa8: the AHX error callback and its caller. */
uint32_t func_004d9a90(uint32_t fn, uint32_t obj)
{
    M32(0x007286f4) = fn;
    M32(0x007286f8) = obj;
    return 0;
}

uint32_t func_004d9aa8(uint32_t arg)
{
    uint32_t fn = M32(0x007286f4);

    if (fn != 0) {
        ((void (*)(uint32_t, uint32_t))p4_fn_g2h_sig(fn, P4_SIG(2, 0, 0, 0, 0, 0, 0, 0)))(M32(0x007286f8), arg);
    }
    return 0;
}

/* ---- the extension key ------------------------------------------------------------------------
 * A string is run through three linear generators (start values at 0x007577d8, 0x007579d8, 0x00757bd8, a
 * multiplier per character at 0x007576d8, the result looked up in the table at 0x007575d8 after the
 * product is reduced modulo 1024 toward zero) giving three 16-bit keys. */
static int16_t key_run(const int8_t *s, int32_t len, uint32_t start)
{
    int16_t v = (int16_t)M16(start);
    int32_t i;

    for (i = 0; i < len; i++) {
        int32_t p = (int32_t)v * (int16_t)M16(0x007576d8u + (uint32_t)(int32_t)s[i] * 2);
        int32_t q = (p < 0 ? p + 1023 : p) >> 10;

        v = (int16_t)M16(0x007575d8u + (uint32_t)((p - (q << 10)) * 2));
    }
    return v;
}

/* 0x004d9b20 */
uint32_t func_004d9b20(const int8_t *s, int32_t len, int16_t *k0, int16_t *k1, int16_t *k2)
{
    if (M32(0x007286f0) == 0) {
        func_004d9ae0();
    }
    *k0 = 0;
    *k1 = 0;
    *k2 = 0;
    if (s != 0 || len > 0) {
        int16_t a = key_run(s, len, 0x007577d8u);
        int16_t b = key_run(s, len, 0x007579d8u);
        int16_t c = key_run(s, len, 0x00757bd8u);

        *k0 = a;
        *k1 = b;
        *k2 = c;
    }
    return 0;
}

/* 0x004d9cd8: the default extension key (the string at 0x00759558, six characters). */
uint32_t func_004d9cd8(int16_t *k0, int16_t *k1, int16_t *k2)
{
    if (M32(0x007286f0) == 0) {
        func_004d9ae0();
    }
    func_004d9b20((const int8_t *)P4_G2H(0x00759558u), 6, k0, k1, k2);
    return 0;
}

/* 0x004d9d48: *a = *c + *a * *b on unsigned halfwords. */
uint32_t func_004d9d48(uint16_t *a, const uint16_t *b, const uint16_t *c)
{
    *a = (uint16_t)(*c + (uint32_t)*a * *b);
    return 0;
}

/* ---- AHXDCD: the frame decoder --------------------------------------------------------------
 * +0 the 32-byte aligned sample buffer inside the work (3 x 64 bytes for each channel), +0x1a4 used,
 * +0x1a5 header decoded, +0x1a6 slot, +0x1a8 groups of 96 samples decoded in this frame (12 is the
 * end), +0x1ac bit reader, +0x1b0 synthesis state, +0x1b4 the 12 frame header words, +0x1e4 the stream
 * info (1 valid, +0x1e8 channels, +0x1ec sampling rate, +0x1f0 total samples, +0x200 the play
 * parameters of two channels, +0x218 the four key halfwords), +0x220 bit allocation, +0x320 scale
 * factor select, +0x420 scale factors, +0x720 samples, +0xa20 the extension key, +0xa28 the synthesis
 * state. */
#define DCD_TABLE 0x007288e8u

/* 0x004d9d68, 0x004d9dc0: AHXDCD library init and finish (use count at 0x007288b8). */
void func_004d9d68(void)
{
    if (M32(0x007288b8) == 0) {
        func_004dd620();
        func_0043f9c8((void *)P4_G2H(DCD_TABLE), 0, 0x40);
    }
    M32(0x007288b8) = M32(0x007288b8) + 1;
}

void func_004d9dc0(void)
{
    M32(0x007288b8) = M32(0x007288b8) - 1;
    if (M32(0x007288b8) == 0) {
        func_0043f9c8((void *)P4_G2H(DCD_TABLE), 0, 0x40);
        func_004dd718();
    }
}

/* 0x004d9e10: carve a decoder out of a work area (0xa28 bytes of its own, then the synthesis state).
 * 0 when all 16 slots are in use or the state does not fit. */
void *func_004d9e10(uint8_t *work, uint32_t size)
{
    int32_t i;
    void *sbf;

    for (i = 0; i < 16; i++) {
        if (M32(DCD_TABLE + (uint32_t)i * 4) == 0) {
            break;
        }
    }
    if (i == 16) {
        return 0;
    }
    func_0043f9c8(work, 0, size);
    M32(DCD_TABLE + (uint32_t)i * 4) = P4_H2G(work);
    P8(work, 0x1a6) = (uint8_t)i;
    sbf = func_004dd768(work + 0xa28, 0x1014);
    P32(work, 0x1b0) = P4_H2G(sbf);
    if (sbf == 0) {
        return 0;
    }
    P32(work, 0) = (P4_H2G(work) + 0x23u) & 0xffffffe0u;
    func_0043f9c8(work + 0x1e4, 0, 8);
    P8(work, 0x1a4) = 1;
    return work;
}

/* 0x004d9ef0 */
void func_004d9ef0(uint8_t *d)
{
    void *sbf = (void *)P4_G2H(P32(d, 0x1b0));
    int8_t slot = S8(d, 0x1a6);

    if (sbf != 0) {
        P32(d, 0x1b0) = 0;
        func_004dd840(sbf);
    }
    func_0043f9c8(d, 0, 0xa28);
    M32(DCD_TABLE + (uint32_t)(int32_t)slot * 4) = 0;
}

/* 0x004d9f50 AHXDCD_Reset: forget the stream, keep the bit reader's joint and a fresh synthesis state. */
void func_004d9f50(uint8_t *d)
{
    uint32_t sbf_work = P32(d, 0x1b0);
    void *sbf;

    func_0043f9c8(d + 0x1b4, 0, 0x30);
    func_0043f9c8(d + 0x1e4, 0, 0x3c);
    func_0043f9c8(d + 0x220, 0, 0x100);
    func_0043f9c8(d + 0x320, 0, 0x100);
    func_0043f9c8(d + 0x420, 0, 0x300);
    func_0043f9c8((void *)P4_G2H(P32(d, 0)), 0, 0x180);
    func_0043f9c8(d + 0x720, 0, 0x300);
    P8(d, 0x1a5) = 0;
    sbf = (void *)P4_G2H(P32(d, 0x1b0));
    P32(d, 0x1a8) = 0;
    if (sbf != 0) {
        P32(d, 0x1b0) = 0;
        func_004dd840(sbf);
        P32(d, 0x1b0) = P4_H2G(func_004dd768((void *)P4_G2H(sbf_work), 0x1014));
    }
}

/* 0x004da018 AHXDCD_SetBsr */
void func_004da018(uint8_t *d, void *bsr)
{
    func_004d9f50(d);
    P32(d, 0x1ac) = P4_H2G(bsr);
}

/* 0x004da050: read the ADX header at the head of the stream (4 bytes, then the rest its length field
 * names) into the static buffer at 0x00922ef0 and take the stream info from it. */
void func_004da050(uint8_t *d)
{
    uint8_t hdr[0x40];
    uint8_t *buf = (uint8_t *)P4_G2H(0x00922ef0u);
    uint8_t *bsr = (uint8_t *)P4_G2H(P32(d, 0x1ac));
    uint8_t *p, *end;
    int32_t hdrlen = 0;
    int32_t rev;

    if (bsr == 0) {
        return;
    }
    func_0043f9c8(hdr, 0, 0x40);
    for (p = buf; p != buf + 4; p++) {
        *p = (uint8_t)func_004d97c8(bsr, 8);
    }
    ADX_DecodeHeader(buf, 4, &hdrlen, 0);
    end = buf + 4 + hdrlen;
    for (p = buf + 4; p != end; p++) {
        *p = (uint8_t)func_004d97c8(bsr, 8);
    }
    if (ADX_DecodeHeader(buf, 0x200, &hdrlen, hdr) < 0) {
        return;
    }
    memcpy(d + 0x200, hdr + 36, 12);
    S32(d, 0x1e8) = (int8_t)hdr[3];
    memcpy(d + 0x20c, hdr + 48, 12);
    P32(d, 0x1ec) = P32(hdr, 4);
    P32(d, 0x1f0) = P32(hdr, 8);
    P32(d, 0x1e4) = 1;
    rev = (int8_t)hdr[61];
    if (rev >= 16) {
        P16(d, 0x218) = 0;
        func_004d9cd8((int16_t *)(d + 0x21a), (int16_t *)(d + 0x21c), (int16_t *)(d + 0x21e));
    } else if (rev >= 8) {
        memcpy(d + 0x218, d + 0xa20, 8);
    } else {
        func_0043f9c8(d + 0x218, 0, 8);
    }
}

/* 0x004da630: the frame header, 12 fields of 1..4 bits. -1 for a layer or mode the decoder does not
 * handle (the layer must be 3 and the sampling rate index 0). */
int32_t func_004da630(uint8_t *bsr, uint32_t *h)
{
    h[0] = func_004d97c8(bsr, 1);
    h[1] = 4 - func_004d97c8(bsr, 2);
    h[2] = func_004d97c8(bsr, 1) == 0;
    h[3] = func_004d97c8(bsr, 4);
    h[4] = func_004d97c8(bsr, 2);
    h[5] = func_004d97c8(bsr, 1);
    h[6] = func_004d97c8(bsr, 1);
    h[7] = func_004d97c8(bsr, 2);
    h[8] = func_004d97c8(bsr, 2);
    h[9] = func_004d97c8(bsr, 1);
    h[10] = func_004d97c8(bsr, 1);
    h[11] = func_004d97c8(bsr, 2);
    if (h[7] == 3 && h[8] == 0) {
        return 0;
    }
    return -1;
}

/* 0x004da748 */
uint32_t func_004da748(void *a, int32_t b, uint32_t *out)
{
    (void)a;
    (void)b;
    return func_004d9a58(0, out);
}

/* 0x004da768: derive the stream layout from the frame header: info[1] channels (1 for the mode 3
 * single channel, else 2), info[4] the number of sub-bands, info[5] the joint stereo bound, info[2]
 * the bit rate. */
void func_004da768(uint8_t *h, uint8_t *info)
{
    int32_t v;

    if (S32(info, 0) == 0) {
        S32(info, 4) = S32(h, 0x1c) == 3 ? 1 : 2;
    }
    if (S32(h, 4) == 2) {
        v = (int32_t)func_004da748(h, S32(info, 4), (uint32_t *)(info + 24));
    } else {
        v = 32;
    }
    S32(info, 16) = v;
    if (S32(h, 0x1c) == 1) {
        S32(info, 20) = (int32_t)M32(0x00728888u + (uint32_t)(S32(h, 4) * 4 + S32(h, 0x20)) * 4);
    } else {
        S32(info, 20) = S32(info, 16);
    }
    if (S32(info, 0) == 0) {
        S32(info, 8) = (int32_t)M32(0x00728700u + (uint32_t)(S32(h, 0) * 4 + S32(h, 0x10)) * 4);
    }
}

/* 0x004da850: the bit allocation of every sub-band and channel: out[ch * 32 + sb] is the allocation
 * index, read with the width the sub-band's table row gives. Above the joint stereo bound one value
 * serves both channels; above the last sub-band all are zero. */
void func_004da850(uint8_t *bsr, uint8_t *info, uint32_t *out)
{
    int32_t nch = S32(info, 4);
    int32_t limit = S32(info, 16);
    int32_t bound = S32(info, 20);
    uint8_t *tab = (uint8_t *)P4_G2H(P32(info, 24));
    int32_t sb, ch;

    for (sb = 0; sb < bound; sb++) {
        for (ch = 0; ch < nch; ch++) {
            out[ch * 32 + sb] = func_004d97c8(bsr, S32(tab, sb * 0x180 + 4));
        }
    }
    for (sb = bound; sb < limit; sb++) {
        uint32_t v = func_004d97c8(bsr, S32(tab, sb * 0x180 + 4));

        out[sb] = v;
        out[32 + sb] = v;
    }
    for (sb = limit; sb < 32; sb++) {
        for (ch = 0; ch < nch; ch++) {
            out[ch * 32 + sb] = 0;
        }
    }
}

/* 0x004da9d8: the scale factor select information (2 bits per sub-band that has an allocation, undone
 * with the stream's key words) and the scale factors it selects (three per sub-band, shared as 0, 1 or 2
 * of them say): scale[(ch * 3 + k) * 32 + sb]. */
void func_004da9d8(uint8_t *bsr, uint8_t *info, int32_t *atbl, int32_t *sel, int32_t *scale)
{
    int32_t nch = S32(info, 4);
    int32_t limit = S32(info, 16);
    int32_t sb, ch, key;

    if (atbl[0] != 0) {
        sel[0] = (int32_t)func_004d97c8(bsr, 2);
    } else {
        sel[0] = 0;
    }
    key = S16(info, 52 + sel[0] * 2);
    for (sb = 1; sb < limit; sb++) {
        if (atbl[sb] != 0) {
            sel[sb] = (int32_t)func_004d97c8(bsr, 2) ^ (key & 3);
        }
        key >>= 2;
    }
    for (sb = limit; sb < 32; sb++) {
        for (ch = 0; ch < nch; ch++) {
            sel[ch * 32 + sb] = 0;
        }
    }
    for (sb = 0; sb < limit; sb++) {
        for (ch = 0; ch < nch; ch++) {
            int32_t *o = scale + ch * 96 + sb;

            if (atbl[ch * 32 + sb] == 0) {
                o[0] = 63;
                o[32] = 63;
                o[64] = 63;
                continue;
            }
            switch (sel[ch * 32 + sb]) {
            case 0:
                o[0] = (int32_t)func_004d97c8(bsr, 6);
                o[32] = (int32_t)func_004d97c8(bsr, 6);
                o[64] = (int32_t)func_004d97c8(bsr, 6);
                break;
            case 1:
                o[0] = (int32_t)func_004d97c8(bsr, 6);
                o[32] = o[0];
                o[64] = (int32_t)func_004d97c8(bsr, 6);
                break;
            case 2:
                o[0] = (int32_t)func_004d97c8(bsr, 6);
                o[64] = o[0];
                o[32] = o[0];
                break;
            case 3:
                o[0] = (int32_t)func_004d97c8(bsr, 6);
                o[32] = (int32_t)func_004d97c8(bsr, 6);
                o[64] = o[32];
                break;
            default:
                break;
            }
        }
    }
    for (sb = limit; sb < 32; sb++) {
        for (ch = 0; ch < nch; ch++) {
            int32_t *o = scale + ch * 96 + sb;

            o[0] = 63;
            o[32] = 63;
            o[64] = 63;
        }
    }
}

/* One dequantised sample: ((x * c + d) >> 2) * scale >> 16, in 32 bits. */
static int16_t deq(int32_t x, int32_t c, int32_t d, uint32_t mul)
{
    int32_t v = (int32_t)((uint32_t)x * (uint32_t)c + (uint32_t)d) >> 2;

    return (int16_t)((int32_t)((uint32_t)v * mul) >> 16);
}

/* 0x004dacb8: the samples of one group of 96: for every sub-band and channel the three samples the
 * allocation names (a packed triple for the class 3 rows, a table lookup for the 3, 5 and 9 level rows,
 * else digits of the value in base `levels`), scaled; out[ch * 96 + k * 32 + sb]. Sub-bands from the
 * joint stereo bound up carry channel 0 only and are copied to channel 1. */
void func_004dacb8(uint8_t *bsr, uint8_t *info, int32_t *atbl, int32_t *scale, int32_t gr, int16_t *out)
{
    int32_t limit = S32(info, 16);
    int32_t nch = S32(info, 4);
    int32_t bound = S32(info, 20);
    uint8_t *tab = (uint8_t *)P4_G2H(P32(info, 24));
    int32_t sb, ch, k;

    M32(0x00728948u) = 0;
    M32(BSR_COUNT) = 0;
    for (sb = 0; sb < limit; sb++) {
        int32_t nonjoint = sb < bound;

        for (ch = 0; nonjoint ? ch < nch : ch < 1; ch++) {
            int16_t *o = out + ch * 96 + sb;
            int32_t a1 = atbl[ch * 32 + sb];

            if (a1 == 0) {
                o[64] = 0;
                o[0] = 0;
                o[32] = 0;
            } else {
                uint32_t mul = M32(0x007285f0u + (uint32_t)scale[(ch * 3 + gr) * 32 + sb] * 4);
                uint8_t *ent = tab + sb * 0x180 + a1 * 24;
                int32_t type = S32(ent, 8);
                int32_t d = S32(ent, 20);
                int32_t c = S32(ent, 16);

                if (type == 3) {
                    int32_t bits = S32(ent, 4);

                    if (bits < 9) {
                        int32_t v = (int32_t)func_004d97c8(bsr, bits * 3);
                        int32_t mask = (int32_t)M32(0x007288c0u + (uint32_t)bits * 4);

                        o[64] = deq(v & mask, c, d, mul);
                        v >>= bits;
                        o[32] = deq(v & mask, c, d, mul);
                        v >>= bits;
                        o[0] = deq(v, c, d, mul);
                    } else {
                        o[0] = deq((int32_t)func_004d97c8(bsr, bits), c, d, mul);
                        o[32] = deq((int32_t)func_004d97c8(bsr, bits), c, d, mul);
                        o[64] = deq((int32_t)func_004d97c8(bsr, bits), c, d, mul);
                    }
                } else {
                    int32_t grp = S32(ent, 0);
                    int32_t v = (int32_t)func_004d97c8(bsr, S32(ent, 4));

                    if (grp == 3) {
                        o[0] = deq((int8_t)M8(0x00729a90u + (uint32_t)v), c, d, mul);
                        o[32] = deq((int8_t)M8(0x00729ab0u + (uint32_t)v), c, d, mul);
                        o[64] = deq((int8_t)M8(0x00729ad0u + (uint32_t)v), c, d, mul);
                    } else if (grp == 5) {
                        o[0] = deq((int8_t)M8(0x00729af0u + (uint32_t)v), c, d, mul);
                        o[32] = deq((int8_t)M8(0x00729b70u + (uint32_t)v), c, d, mul);
                        o[64] = deq((int8_t)M8(0x00729bf0u + (uint32_t)v), c, d, mul);
                    } else if (grp == 9) {
                        o[0] = deq((int8_t)M8(0x00729c70u + (uint32_t)v), c, d, mul);
                        o[32] = deq((int8_t)M8(0x0072a070u + (uint32_t)v), c, d, mul);
                        o[64] = deq((int8_t)M8(0x0072a470u + (uint32_t)v), c, d, mul);
                    } else {
                        for (k = 0; k < 3; k++) {
                            o[k * 32] = deq(v % grp, c, d, mul);
                            v = v / grp;
                        }
                    }
                }
            }
            if (nch == 2 && !nonjoint) {
                int16_t *s = out + sb;

                for (k = 0; k < 3; k++) {
                    s[96 + k * 32] = s[k * 32];
                }
            }
        }
    }
    for (sb = limit; sb < 32; sb++) {
        for (ch = 0; ch < nch; ch++) {
            int16_t *o = out + ch * 96 + sb;

            o[64] = 0;
            o[32] = 0;
            o[0] = 0;
        }
    }
}

/* 0x004da1f8 AHXDCD_DecodeFrmHdr: find the next frame, read its header, allocation, select and scale
 * information. 0 when a frame is ready, 1 at the end marker, -1 when the data ran out or is not a frame. */
int32_t func_004da1f8(uint8_t *d)
{
    uint8_t *bsr = (uint8_t *)P4_G2H(P32(d, 0x1ac));
    int32_t s;

    if (bsr == 0) {
        return -1;
    }
    s = func_004d9998(bsr);
    if (s == -1) {
        return -1;
    }
    if (s != 1) {
        return 1;
    }
    if (func_004d9768(bsr, 20) == 0 || func_004da630(bsr, (uint32_t *)(d + 0x1b4)) < 0) {
        return -1;
    }
    func_004da768(d + 0x1b4, d + 0x1e4);
    func_004da850(bsr, d + 0x1e4, (uint32_t *)(d + 0x220));
    func_004da9d8(bsr, d + 0x1e4, (int32_t *)(d + 0x220), (int32_t *)(d + 0x320), (int32_t *)(d + 0x420));
    P8(d, 0x1a5) = 1;
    P32(d, 0x1a8) = 0;
    return 0;
}

/* 0x004da2e8: decode the next group of 96 samples: the left channel into outl, the right (when the
 * stream has two channels) into outr. Returns 96, or 0 when the frame is used up. */
uint32_t func_004da2e8(uint8_t *d, int16_t *outl, int16_t *outr)
{
    int32_t i;
    int32_t nch = S32(d, 0x1e8);
    uint8_t *sbf = (uint8_t *)P4_G2H(P32(d, 0x1b0));
    uint8_t *buf = (uint8_t *)P4_G2H(P32(d, 0));

    if (S32(d, 0x1a8) == 12) {
        return 0;
    }
    M32(0x00728928u) = func_004dd608();
    func_004dacb8((uint8_t *)P4_G2H(P32(d, 0x1ac)), d + 0x1e4, (int32_t *)(d + 0x220),
                  (int32_t *)(d + 0x420), S32(d, 0x1a8) >> 2, (int16_t *)buf);
    M32(0x00728934u) = func_004dd608();
    for (i = 0; i < 3; i++) {
        func_004dd880(sbf, buf + i * 64, 0, outl + i * 32);
    }
    M32(0x00728938u) = func_004dd608();
    if (nch >= 2) {
        for (i = 0; i < 3; i++) {
            func_004dd880(sbf, buf + i * 64 + 192, 1, outr + i * 32);
        }
    }
    M32(0x0072893cu) = func_004dd608();
    P32(d, 0x1a8) = P32(d, 0x1a8) + 1;
    return 0x60;
}

/* 0x004da448, 0x004da468, 0x004da4a0: channels (0 before a header), samples per group, samples per frame. */
int32_t AHXDCD_GetNumChan(uint8_t *d)
{
    return S8(d, 0x1a5) != 0 ? S32(d, 0x1e8) : 0;
}

int32_t func_004da468(void)
{
    return 0x60;
}

int32_t func_004da4a0(void)
{
    return 0x60 * 12;
}

/* 0x004da4c8 AHXDCD_SetExtString: the extension key from a string. */
void AHXDCD_SetExtString(uint8_t *d, const char *s)
{
    int32_t len = (int32_t)func_00442948(s);

    func_004d9b20((const int8_t *)s, len, (int16_t *)(d + 0xa22), (int16_t *)(d + 0xa24), (int16_t *)(d + 0xa26));
    P16(d, 0xa20) = 0;
}

/* ---- the synthesis filter state --------------------------------------------------------------
 * +0 used, +1 slot, +4 and +8 the ring positions of the two channels (a step of 64 samples back per
 * group), +0xc the matrix table, +0x10 the window table, +0x14 two 1024 sample rings (0x800 bytes each). */
#define SBF_TABLE 0x00729998u

/* 0x004db558: the 64 matrixed samples of a group of 32 sub-band samples: 32 sums of products with a row
 * of the matrix table (16 rows, a gap, 15 rows and the last one), kept 16 bit after a shift of 12, the
 * odd symmetric half being the bitwise complement (rows 0..15) or the same value (rows 33..47). */
int16_t *func_004db558(const int16_t *in, const int16_t *mat, int16_t *out)
{
    int32_t i, j;

#define ROW(r, res)                                                  \
    do {                                                             \
        uint32_t s_ = 0;                                             \
        const int16_t *m_ = mat + (r) * 32;                          \
        for (j = 0; j < 32; j++) {                                   \
            s_ += (uint32_t)((int32_t)in[j] * (int32_t)m_[j]);        \
        }                                                            \
        (res) = (int32_t)s_ >> 12;                                   \
    } while (0)

    for (i = 0; i < 16; i++) {
        int32_t r;

        ROW(i, r);
        out[i] = (int16_t)r;
        out[32 - i] = (int16_t)(-r - 1);
    }
    out[16] = 0;
    for (i = 0; i < 15; i++) {
        int32_t r;

        ROW(33 + i, r);
        out[33 + i] = (int16_t)r;
        out[63 - i] = (int16_t)r;
    }
    {
        int32_t r;

        ROW(48, r);
        out[48] = (int16_t)r;
    }
#undef ROW
    return out;
}

/* 0x004dd678: the two constant tables are copied up by 16 bytes so that they start on a 32-byte
 * boundary (the window table 0x0072a870 -> 0x0072a880, 1025 bytes; the matrix table 0x00728950 ->
 * 0x00728960, 4097 bytes). */
void func_004dd678(void)
{
    int32_t i;

    if (M32(0x00729994) != 0) {
        return;
    }
    M32(0x007299dc) = 0x0072a880u;
    for (i = 1024; i >= 0; i--) {
        M8(0x0072a880u + (uint32_t)i) = M8(0x0072a870u + (uint32_t)i);
    }
    M32(0x007299d8) = 0x00728960u;
    for (i = 4096; i >= 0; i--) {
        M8(0x00728960u + (uint32_t)i) = M8(0x00728950u + (uint32_t)i);
    }
    M32(0x00729994) = 1;
}

/* 0x004dd620, 0x004dd718: library init and finish (use count at 0x00729990). */
void func_004dd620(void)
{
    if (M32(0x00729990) == 0) {
        func_0043f9c8((void *)P4_G2H(SBF_TABLE), 0, 0x40);
        func_004dd678();
    }
    M32(0x00729990) = M32(0x00729990) + 1;
}

void func_004dd718(void)
{
    M32(0x00729990) = M32(0x00729990) - 1;
    if (M32(0x00729990) == 0) {
        func_0043f9c8((void *)P4_G2H(SBF_TABLE), 0, 0x40);
    }
}

/* 0x004dd768 */
void *func_004dd768(uint8_t *work, uint32_t size)
{
    int32_t i;

    for (i = 0; i < 16; i++) {
        if (M32(SBF_TABLE + (uint32_t)i * 4) == 0) {
            break;
        }
    }
    if (i == 16) {
        return 0;
    }
    func_0043f9c8(work, 0, size);
    M32(SBF_TABLE + (uint32_t)i * 4) = P4_H2G(work);
    P8(work, 1) = (uint8_t)i;
    P32(work, 4) = 0x40;
    P32(work, 8) = 0x40;
    P32(work, 0x10) = M32(0x007299dc);
    P32(work, 0xc) = M32(0x007299d8);
    P8(work, 0) = 1;
    return work;
}

/* 0x004dd840 */
void func_004dd840(uint8_t *sbf)
{
    int8_t slot = S8(sbf, 1);

    func_0043f9c8(sbf, 0, 0x1014);
    M32(SBF_TABLE + (uint32_t)(int32_t)slot * 4) = 0;
}

extern void ahxsbf_mult_win0(short *, short *, uint16_t *);
extern void ahxsbf_mult_win1(short *, short *, uint16_t *);
extern void ahxsbf_mult_win2(short *, short *, uint16_t *);
extern void ahxsbf_mult_win3(short *, short *, uint16_t *);
extern void ahxsbf_mult_win4(short *, short *, uint16_t *);
extern void ahxsbf_mult_win5(short *, short *, uint16_t *);
extern void ahxsbf_mult_win6(short *, short *, uint16_t *);
extern void ahxsbf_mult_win7(short *, short *, uint16_t *);
extern void ahxsbf_mult_win8(short *, short *, uint16_t *);
extern void ahxsbf_mult_win9(short *, short *, uint16_t *);
extern void ahxsbf_mult_win10(short *, short *, uint16_t *);
extern void ahxsbf_mult_win11(short *, short *, uint16_t *);
extern void ahxsbf_mult_win12(short *, short *, uint16_t *);
extern void ahxsbf_mult_win13(short *, short *, uint16_t *);
extern void ahxsbf_mult_win14(short *, short *, uint16_t *);
extern void ahxsbf_mult_win15(short *, short *, uint16_t *);

/* 0x004dd880: one group of 32 sub-band samples of channel ch through the synthesis filter: step the ring
 * back by 64, matrix the 32 samples into it, and window the ring into 32 output samples with the variant
 * of the sixteen that matches the ring position. The input and the matrix table must be 32-byte
 * aligned (the retail code spins forever otherwise; here the group is skipped). */
void func_004dd880(uint8_t *sbf, uint8_t *in, int32_t ch, int16_t *out)
{
    static void (*const win[16])(short *, short *, uint16_t *) = {
        ahxsbf_mult_win0,  ahxsbf_mult_win1,  ahxsbf_mult_win2,  ahxsbf_mult_win3,
        ahxsbf_mult_win4,  ahxsbf_mult_win5,  ahxsbf_mult_win6,  ahxsbf_mult_win7,
        ahxsbf_mult_win8,  ahxsbf_mult_win9,  ahxsbf_mult_win10, ahxsbf_mult_win11,
        ahxsbf_mult_win12, ahxsbf_mult_win13, ahxsbf_mult_win14, ahxsbf_mult_win15,
    };
    uint8_t *ring = sbf + ch * 0x800;
    uint32_t pos;
    uint32_t mat = P32(sbf, 0xc);

    pos = (P32(sbf, 4 + ch * 4) - 64) & 0x3ff;
    P32(sbf, 4 + ch * 4) = pos;
    M32(0x00729a20u) = func_004dd608();
    if ((P4_H2G(in) & 0x1f) != 0 || (mat & 0x1f) != 0) {
        return;
    }
    func_004db558((const int16_t *)in, (const int16_t *)P4_G2H(mat), (int16_t *)(ring + pos * 2 + 20));
    M32(0x00729a30u) = func_004dd608();
    pos = P32(sbf, 4 + ch * 4);
    M32(0x00729a20u) = func_004dd608();
    win[pos >> 6]((short *)(ring + 20 + pos * 2), (short *)P4_G2H(P32(sbf, 0x10)), (uint16_t *)out);
    M32(0x00729a30u) = func_004dd608();
}

/* ---- AHXSJD: the stream joint decoder ---------------------------------------------------------
 * 0x50 bytes at the head of a work area (the decoder follows at +0x50): +0 decoder, +4 used, +5 state
 * (0 idle, 1 reading the header, 2 decoding, 3 ended), +6 channels, +7 header read, +0xc bit reader,
 * +0x10 input joint, +0x14 output joints, +0x20 samples decoded, +0x24 bytes read, +0x28 and +0x34
 * counters, +0x2c maximum samples per step, +0x30 trap position (-1 none), +0x40/+0x44 trap function and
 * object, +0x48/+0x4c the decoded data tap and its object. */
#define SJD_TABLE 0x00729a40u

/* 0x004dd9c8, 0x004dda30: AHXSJD library init and finish (use count at 0x00729a38). */
void func_004dd9c8(void)
{
    if (M32(0x00729a38) == 0) {
        func_004d9d68();
        func_0043f9c8((void *)P4_G2H(SJD_TABLE), 0, 0x40);
    }
    M32(0x00729a38) = M32(0x00729a38) + 1;
}

void func_004dda30(void)
{
    if (M32(0x00729a38) == 1) {
        func_0043f9c8((void *)P4_G2H(SJD_TABLE), 0, 0x40);
        func_004d9dc0();
    }
    M32(0x00729a38) = M32(0x00729a38) - 1;
}

/* 0x004dda88 */
void func_004dda88(uint8_t *h)
{
    S32(h, 0x30) = -1;
    S32(h, 0x2c) = 0x7fffffff;
    P8(h, 7) = 0;
    P32(h, 8) = 0;
    P32(h, 0x1c) = 0;
    P32(h, 0x20) = 0;
    P32(h, 0x24) = 0;
    P32(h, 0x28) = 0;
    P32(h, 0x34) = 0;
    P32(h, 0x38) = 0;
}

/* 0x004ddac0: a decoder over the input joint `in`, writing `nch` output joints (sjo); the work area holds
 * the handle and the frame decoder. 0 when no slot is free or something does not fit. */
void *func_004ddac0(void *in, int32_t nch, uint32_t *sjo, uint8_t *work, uint32_t size)
{
    int32_t i, k;
    void *bsr, *dcd;

    for (i = 0; i < 16; i++) {
        if (M32(SJD_TABLE + (uint32_t)i * 4) == 0) {
            break;
        }
    }
    if (i == 16) {
        return 0;
    }
    func_0043f9c8(work, 0, size);
    P32(work, 0x3c) = (uint32_t)i;
    M32(SJD_TABLE + (uint32_t)i * 4) = P4_H2G(work);
    bsr = func_004d9348(in);
    P32(work, 0xc) = P4_H2G(bsr);
    if (bsr == 0) {
        return 0;
    }
    dcd = func_004d9e10(work + 0x50, 0x1a3c);
    P32(work, 0) = P4_H2G(dcd);
    if (dcd == 0) {
        return 0;
    }
    P32(work, 0x10) = P4_H2G(in);
    P8(work, 6) = (uint8_t)nch;
    for (k = 0; k < nch; k++) {
        P32(work, 0x14 + k * 4) = sjo[k];
    }
    P8(work, 5) = 0;
    func_004dda88(work);
    P32(work, 0x40) = 0;
    P32(work, 0x44) = 0;
    P32(work, 0x48) = 0;
    P32(work, 0x4c) = 0;
    P8(work, 4) = 1;
    return work;
}

/* 0x004ddc10: free a decoder handle with its bit reader and frame decoder. */
void func_004ddc10(uint8_t *h)
{
    uint32_t bsr, slot;

    if (h == 0) {
        return;
    }
    bsr = P32(h, 0xc);
    slot = P32(h, 0x3c);
    if (bsr != 0) {
        P32(h, 0xc) = 0;
        func_004d9478((void *)P4_G2H(bsr));
    }
    if (P32(h, 0) != 0) {
        uint32_t d = P32(h, 0);

        P32(h, 0) = 0;
        func_004d9ef0((uint8_t *)P4_G2H(d));
    }
    func_0043f9c8(h, 0, 0x50);
    M32(SJD_TABLE + slot * 4) = 0;
}

/* 0x004ddca0: AHXSJD_SetOutSj(h, i, sj). */
void AHXSJD_SetOutSj(uint8_t *h, int32_t i, uint32_t sj)
{
    P32(h, 0x14 + i * 4) = sj;
}

/* 0x004ddcb8 AHXSJD_Start */
void func_004ddcb8(uint8_t *h)
{
    void *bsr;

    func_004dda88(h);
    if (P32(h, 0xc) != 0) {
        uint32_t old = P32(h, 0xc);

        P32(h, 0xc) = 0;
        func_004d9478((void *)P4_G2H(old));
    }
    bsr = func_004d9348((void *)P4_G2H(P32(h, 0x10)));
    P32(h, 0xc) = P4_H2G(bsr);
    func_004da018((uint8_t *)P4_G2H(P32(h, 0)), bsr);
    P8(h, 5) = 1;
}

/* 0x004ddd10 AHXSJD_Stop */
void func_004ddd10(uint8_t *h)
{
    func_004d9f50((uint8_t *)P4_G2H(P32(h, 0)));
    P8(h, 5) = 0;
}

/* 0x004ddd40: wait for the header (36 bytes), read it and the first frame header. */
void func_004ddd40(uint8_t *h)
{
    if (sj_num((void *)P4_G2H(P32(h, 0x10)), 1) < 36) {
        return;
    }
    func_004da050((uint8_t *)P4_G2H(P32(h, 0)));
    func_004da1f8((uint8_t *)P4_G2H(P32(h, 0)));
    P8(h, 7) = 1;
    P8(h, 5) = 2;
}

/* 0x004ddda0: one step of decoding: one group of 96 samples into the first output joint when it has
 * room, taking the next frame when the last one is used up. */
void func_004ddda0(uint8_t *h)
{
    uint8_t *d = (uint8_t *)P4_G2H(P32(h, 0));
    void *out = (void *)P4_G2H(P32(h, 0x14));
    void *bsr;
    int32_t flag = S8(h, 7);
    int32_t grp, bytes, room;
    Ck ck, ck2;
    int32_t n;

    if (flag == 1 && sj_num((void *)P4_G2H(P32(h, 0x10)), 1) == 0 &&
        AHXBSR_IsEos((uint8_t *)P4_G2H(P32(h, 0xc))) == 1) {
        P8(h, 5) = 3;
        P32(h, 0x20) = P32(h, 0x2c);
        return;
    }
    grp = func_004da468();
    bytes = (16 < 0 ? 16 + 7 : 16) >> 3;
    room = sj_num(out, 0);
    if (bytes == 0) {
        return;
    }
    if (room / bytes < grp) {
        return;
    }
    if (S32(d, 0x1a8) == 12) {
        int32_t r;

        M32(0x00729a88u) = func_004dd608();
        r = func_004da1f8(d);
        if (r == 1) {
            P8(h, 5) = 3;
            P32(h, 0x20) = P32(h, 0x2c);
            return;
        }
        if (r == -1) {
            return;
        }
        M32(0x00729a8cu) = func_004dd608();
    }
    sj_get(out, 0, grp * bytes, &ck);
    if (((uint32_t)ck.len >> 1) != (uint32_t)grp) {
        sj_unget(out, 0, &ck);
        return;
    }
    M32(0x00729a80u) = func_004dd608();
    n = (int32_t)func_004da2e8(d, (int16_t *)P4_G2H(ck.data), 0);
    M32(0x00729a84u) = func_004dd608();
    SJ_SplitChunk(&ck, n * bytes, &ck, &ck2);
    if (P32(h, 0x48) != 0) {
        ((void (*)(uint32_t, int32_t, uint8_t *, int32_t))p4_fn_g2h_sig(P32(h, 0x48), P4_SIG(4, 0, 0, 0, 1, 0, 0, 0)))(P32(h, 0x4c), 0, (uint8_t *)P4_G2H(ck.data), ck.len);
    }
    sj_put(out, 1, &ck);
    sj_unget(out, 0, &ck2);
    P32(h, 0x20) = P32(h, 0x20) + (uint32_t)n;
    bsr = (void *)P4_G2H(P32(h, 0xc));
    {
        int32_t c = (int32_t)P32(bsr, 0x10);

        P32(h, 0x24) = (uint32_t)((c + 7 < 0 ? c + 14 : c + 7) >> 3);
    }
    P32(h, 0x28) = P32(h, 0x28) + (uint32_t)n;
    P32(h, 0x34) = P32(h, 0x34) + (uint32_t)n;
    if (S32(h, 0x30) >= 0 && S32(h, 0x34) >= S32(h, 0x30) && P32(h, 0x40) != 0) {
        ((void (*)(uint32_t))p4_fn_g2h_sig(P32(h, 0x40), P4_SIG(1, 0, 0, 0, 0, 0, 0, 0)))(P32(h, 0x44));
    }
}

/* 0x004de048 AHXSJD_ExecHndl, 0x004de088: every active decoder. */
void func_004de048(uint8_t *h)
{
    if (S8(h, 5) == 2) {
        func_004ddda0(h);
    } else if (S8(h, 5) == 1) {
        func_004ddd40(h);
    }
}

void func_004de088(void)
{
    int32_t i;

    for (i = 0; i < 16; i++) {
        uint8_t *h = (uint8_t *)P4_G2H(M32(SJD_TABLE + (uint32_t)i * 4));

        if (h != 0 && S8(h, 4) == 1) {
            func_004de048(h);
        }
    }
}

/* 0x004de140..0x004de160: the +0x34 and +0x38 counters and a no-op. */
void func_004de140(uint8_t *h, uint32_t v)
{
    P32(h, 0x34) = v;
}

uint32_t func_004de148(uint8_t *h)
{
    return P32(h, 0x34);
}

void func_004de150(uint8_t *h, uint32_t v)
{
    P32(h, 0x38) = v;
}

uint32_t func_004de158(uint8_t *h)
{
    return P32(h, 0x38);
}

uint32_t func_004de160(void)
{
    return 0;
}

/* ---- ADXT_AttachAHX ---------------------------------------------------------------------------
 * Gives an ADXT an AHX decoder: the ADXB of its decoder object gets the handle at +0xb4, and the
 * ADXB / ADXT hooks the AHX module registers (the four ADXB_EntryAhxFunc functions, the key
 * function, the ADXT detach hook) point here. */
extern void func_004cc9d0(void);                                  /* ADXM lock */
extern void func_004cc9e8(void);                                  /* ADXM unlock */
extern void func_004d4308(void *adxt);                            /* ADXT_Stop */
extern void ADXERR_CallErrFunc2(const char *, const char *);
extern void ADXB_EntryAhxFunc(void (*)(void *, void *), void (*)(void *, int32_t), void (*)(void *), void (*)(void *));
extern void ADXB_SetAhxExtFunc(void (*)(void *, int16_t *));
extern uint32_t ahxdetachfunc; /* decomp global (adx_tlk.c): a guest code address */
extern void func_004db430(uint32_t h, uint32_t v);
extern void func_004db448(uint32_t h);
extern void func_004de110(uint32_t h, uint32_t fn, uint32_t obj);
extern void func_004de290(uint32_t h, uint32_t key);

/* 0x004db398: the detach hook, when an AHX decoder has been attached. */
void func_004db398(void *adxt)
{
    if (ahxdetachfunc != 0) {
        ((void (*)(void *))p4_fn_g2h_sig(ahxdetachfunc, P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(adxt);
    }
}

/* 0x004db3c0 AHXLINK_DetachAhx */
void func_004db3c0(uint8_t *adxt)
{
    uint8_t *sjd, *adxb;
    uint32_t ahx;

    func_004d4308(adxt);
    sjd = (uint8_t *)P4_G2H(P32(adxt, 4));
    adxb = (uint8_t *)P4_G2H(P32(sjd, 4));
    ahx = P32(adxb, 0xb4);
    if (ahx == 0) {
        return;
    }
    func_004cc9d0();
    func_004ddd10((uint8_t *)P4_G2H(ahx));
    func_004ddc10((uint8_t *)P4_G2H(ahx));
    P32(adxb, 0xb4) = 0;
    func_004cc9e8();
    func_004dda30();
}

/* 0x004db460: the ADXB decode step of an AHX stream. */
void func_004db460(uint8_t *adxb)
{
    uint8_t *h;

    func_004cc9d0();
    h = (uint8_t *)P4_G2H(P32(adxb, 0xb4));
    if (S8(h, 5) == 0) {
        P32(adxb, 0x90) = 0;
        P32(adxb, 0x88) = 0;
        func_004ddd10(h);
    }
    if (S32(adxb, 4) == 1) {
        P32(adxb, 0x88) = 0;
        func_004ddcb8(h);
        S32(adxb, 4) = 2;
    } else if (S32(adxb, 4) == 2) {
        int32_t i, v;

        for (i = 0; i < S32(adxb, 0xbc); i++) {
            func_004de048(h);
        }
        v = (int32_t)P32(h, 0x20) - S32(adxb, 0x88);
        P32(adxb, 0x90) = (uint32_t)v;
        P32(adxb, 0x88) = P32(adxb, 0x88) + (uint32_t)v;
        P32(adxb, 0x94) = P32(h, 0x24);
        if (S8(h, 5) == 3) {
            func_004ddd10(h);
            S32(adxb, 4) = 0;
        }
    }
    func_004cc9e8();
}

/* 0x004db250 ADXT_AttachAHX(adxt, work, size): the handle and the frame decoder are built in `work`. */
void func_004db250(uint8_t *adxt, uint8_t *work, uint32_t size)
{
    uint8_t *sjd, *adxb;
    uint8_t *h;

    func_004d4308(adxt);
    func_004db398(adxt);
    func_004dd9c8();
    sjd = (uint8_t *)P4_G2H(P32(adxt, 4));
    adxb = (uint8_t *)P4_G2H(P32(sjd, 4));
    func_004cc9d0();
    h = func_004ddac0((void *)P4_G2H(P32(adxt, 0x10)), 1, (uint32_t *)(adxt + 0x18), work, size);
    if (h == 0) {
        ADXERR_CallErrFunc2((const char *)P4_G2H(0x00759560u), (const char *)P4_G2H(0x00759578u));
        return;
    }
    P32(adxb, 0xb4) = P4_H2G(h);
    ahxdetachfunc = p4_fn_h2g_sig(func_004db3c0, "vp");
    ADXB_EntryAhxFunc((void (*)(void *, void *))func_004ddc98, (void (*)(void *, int32_t))func_004db430,
                      (void (*)(void *))func_004db448, (void (*)(void *))func_004db460);
    ADXB_SetAhxExtFunc((void (*)(void *, int16_t *))func_004de290);
    func_004de110(P4_H2G(h), P32(sjd, 0x50), P32(sjd, 0x54));
    func_004cc9e8();
}
