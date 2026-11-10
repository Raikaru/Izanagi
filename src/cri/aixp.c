/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI AIX player (libadxe aixp / aix_sjd), translated from the retail asm 0x004de2b0..0x004e1240.
 *
 * An AIX file holds several ADX streams (tracks: voice, music layers) interleaved in packets. The
 * AIX player ("AIXP", up to four handles of 0x26c bytes at 0x0072acc8) reads such a file through an
 * ADXSTM into a ring joint, the AIX stream joint decoder ("AIXSJD", four handles of 0x195c bytes at
 * 0x0072b680) splits its packets into one ring joint per track, and one ADXT per track decodes and plays
 * it. ACSSND's AIXP kind (a parameter block whose mode word is not 0) creates these through
 * aixp_Create / aixp_Create3D; the game's four ACSSND objects all use the ADXT kind, so the player is
 * not reached by the game itself. A file that is not an AIX header (no "AIXF") is handled as one raw
 * stream of track 0.
 *
 * AIXP handle: +0 used, +1 state (0 idle, 1 started, 2 playing, 3 ended, 4 error), +2 tracks the
 * stream may carry, +3 tracks played, +4 AIXSJD, +8 ADXSTM, +0xc the ADXT of each track, +0x8c the
 * input ring joint, +0x90 the source joint (a memory or caller joint) or the input ring, +0x94 each
 * track's ring joint, +0x114 .. +0x120 the work areas of the rings, +0x1a8 pause, +0x1ac started,
 * +0x1b0 / +0x1b1 loop switches, +0x1b2 start pending, +0x1bc current phrase, +0x1c0 first phrase,
 * +0x1c4 / +0x1c8 the loop range of phrases, +0x1cc..+0x1d8 the stream start arguments, +0x1dc how the
 * source is given (0 file, 1 AFS, 2 memory, 3 joint), +0x1e0 the start position as a fraction of the
 * stream (float), +0x1e4 master volume, +0x1e8 each track's volume, +0x268 the 3D / PL2 flag. */
#include <string.h>

#include "cri_mem.h"

extern void func_0043f9c8(void *dst, int32_t c, uint32_t n);      /* memset */
extern int32_t func_0043f778(const void *a, const void *b, uint32_t n); /* memcmp */
extern uint32_t func_00442948(const char *s);                     /* strlen */
extern void func_004c6a98(const char *msg);                       /* ADXERR_CallErrFunc1 */
extern void func_004e0380(void);                                  /* AIX lock */
extern void func_004e0398(void);                                  /* AIX unlock */

extern void *func_004d1c58(void *sj, int32_t a);                  /* ADXSTM_Create on a ring joint */
extern void func_004d1ce8(void *stm);                             /* ADXSTM_Destroy */
extern void func_004d2348(void *stm, void *fn, void *obj);        /* ADXSTM_EntryEosFunc */
extern void func_004d1d70(void *stm, void *fname, void *dir, int32_t ofst, int32_t nsct); /* BindFileNw */
extern void func_004d1f00(void *stm);                             /* ReleaseFileNw */
extern int32_t func_004d1ff8(void *stm);                          /* ADXSTM_GetStat */
extern void func_004d2038(void *stm, int32_t pos);                /* ADXSTM_Seek */
extern void func_004d2128(void *stm);                             /* ADXSTM_Start */
extern void func_004d2230(void *stm);                             /* ADXSTM_StopNw */
extern void func_004d23a8(void *stm, int32_t n);                  /* ADXSTM_SetEos */
extern void func_004d2c30(void *stm, int32_t reload, int32_t total); /* ADXSTM_SetBufSize */
extern void func_004d4308(void *adxt);                            /* ADXT_Stop */
extern int32_t func_004d43f8(void *adxt);                         /* ADXT_GetStat */
extern int32_t func_004d4b48(void *adxt);                         /* ADXT_GetNumSmpl */
extern void func_004d5110(void *adxt, int32_t vol);               /* ADXT_SetOutVol */
extern void func_004d6050(void *adxt, int32_t sw);                /* ADXT_Pause */
extern void func_004d65b8(void *adxt, int32_t sw);                /* ADXT_SetLnkSw */
extern void func_004d6bc8(void *adxt, const char *key);           /* ADXT_SetKeyString */
extern void *func_004d3850(int32_t maxnch, void *work, int32_t size); /* ADXT_CreateWork */
extern void *func_004d3bc0(void *work, int32_t size);             /* ADXT create with the PL2 encoder */
extern void func_004d3cd8(void *adxt);                            /* ADXT_Destroy */
extern void *SJRBF_Create(void *buf, int32_t size, int32_t extra); /* 0x004ee018 */
extern void *func_004ed600(void *buf, int32_t size);              /* SJMEM_Create */
extern void func_004e0560(void *h);                               /* AIXSJD_Destroy */
extern int32_t func_004c99a0(int32_t ptid, int32_t fid, void *dirwk, int32_t *fname, int32_t *ofst, int32_t *nsct);
extern int32_t func_004f1278(uint32_t *flag);                     /* test and clear a flag atomically */
extern void SJ_SplitChunk(void *ck, int32_t n, void *ck1, void *ck2);


/* prototypes of the functions defined below (they call each other in both directions) */
void func_004de2c0(const char *msg);
uint32_t func_004de318(void);
void *func_004e04b0(int32_t n, uint32_t arg, uint32_t *sjo);
void *func_004e0458(int32_t n, uint32_t arg, uint32_t *sjo);
void func_004e0598(uint8_t *h);
void func_004e05d8(uint8_t *h, uint32_t sj);
void func_004e05e8(uint8_t *h, uint32_t v);
void func_004e05f0(uint8_t *h, uint32_t v);
int32_t func_004e05f8(uint8_t *h);
uint32_t func_004e0600(uint8_t *h, int32_t i);
int32_t func_004e0610(uint8_t *h, int32_t i);
uint32_t func_004e0620(uint8_t *h);
void func_004e0628(uint8_t *h, int32_t i, uint32_t *a, uint32_t *b);
void func_004e0658(uint8_t *h, int32_t i, uint32_t *a, uint32_t *b);
uint32_t func_004e0688(uint8_t *h);
void func_004e0690(uint8_t *h, int32_t i, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d);
int32_t func_004e06f0(uint8_t *h);
uint32_t func_004e0728(uint8_t *h);
void func_004e0b30(uint8_t *hdr);
int32_t func_004e0778(const uint8_t *buf, int32_t len, uint8_t *hdr);
int32_t func_004e0730(const uint8_t *buf, int32_t len, uint8_t *hdr);
uint32_t func_004e0b80(const uint8_t *p, uint32_t len, uint8_t *out);
void func_004e0c60(uint8_t *h);
int32_t func_004e0db0(uint8_t *h);
void func_004e1100(uint8_t *h);
uint8_t *func_004de5c0(const int32_t *cfg, uint8_t *work, int32_t size);
void *aixp_Create(int32_t tracks, int32_t channels, void *work, int32_t size);
void *aixp_Create3D(int32_t tracks, void *work, int32_t size);
void func_004de860(uint8_t *h);
void func_004de998(void);
void func_004de9f8(uint8_t *h, uint32_t fname, uint32_t dir, int32_t ofst, int32_t nsct);
void func_004deb38(uint8_t *h, uint32_t fname, uint32_t dir);
void func_004deb58(uint8_t *h, int32_t ptid, int32_t fid);
void func_004dec30(uint8_t *h, uint32_t sj);
void func_004decb8(uint8_t *h, void *data, int32_t size);
void func_004ded60(uint8_t *h);
void func_004dee48(uint8_t *h, uint32_t *a, uint32_t *b);
void *func_004deec0(uint8_t *h, int32_t i);
void func_004def18(uint8_t *h);
void func_004defa0(uint8_t *h, float pos);
float func_004defe8(uint8_t *h);
void func_004df068(uint8_t *h, uint32_t v);
void func_004df098(uint8_t *h, uint32_t v);
void func_004df0c8(uint8_t *h, int32_t v);
void func_004df128(uint8_t *h, int32_t first, int32_t count);
void func_004df1a0(uint8_t *h, int32_t *first, int32_t *count);
void func_004df260(uint8_t *h, int32_t sw);
void func_004df220(uint8_t *h, int32_t sw);
int32_t AIXP_GetNumSmpl(uint8_t *h);
int32_t AIXP_GetNumTrack(uint8_t *h);
int32_t AIXP_GetNumPhrase(uint8_t *h);
int32_t AIXP_GetNumSong(uint8_t *h);
int32_t AIXP_GetByteRate(uint8_t *h);
void func_004df5b0(uint8_t *h);
void func_004df708(uint8_t *h);
void func_004df800(uint8_t *h);
void func_004df850(uint8_t *h);
void func_004dfcd8(uint8_t *h, int32_t vol);
void func_004dfc98(uint8_t *h, int32_t vol);
void func_004dfdc8(uint8_t *h, int32_t trk, int32_t vol);
void func_004dfd78(uint8_t *h, int32_t trk, int32_t vol);
void func_004dfe38(uint8_t *h, int32_t trk, int32_t vol);
int32_t func_004dfe60(uint8_t *h, uint8_t *work, int32_t size);
void func_004dff78(uint8_t *h);
void func_004dfff0(uint8_t *h);
void func_004e0058(uint8_t *h, int32_t sw);
void func_004e00d8(uint8_t *h);
void func_004e0158(uint8_t *h, int32_t sw);
uint32_t func_004e01c0(uint8_t *h);
uint32_t func_004e0268(uint8_t *h);
void func_004e0308(uint8_t *h, const char *key);

/* Retail lock wrappers the player calls: 0x004d4170 ADXT_StartSj and 0x004d4778 ADXT_GetTime under the ADX
 * lock (0x004c54d8 / 0x004c54e0, empty in retail), 0x004c9b80 the ADXF partition record (0x004c9bb8) under
 * the ADXF lock (0x004c9d18 / 0x004c9d30). */
extern void func_004c54d8(void);
extern void func_004c54e0(void);
extern void ADXT_StartSj(void *adxt, void *sj);
extern void ADXT_GetTime(void *adxt, int32_t *ncount, int32_t *tscale);
extern void *func_004c9bb8(int32_t ptid);   /* the archive-name field: a host pointer */
extern void func_004c9d18(void);
extern void func_004c9d30(void);

void func_004d4170(void *adxt, void *sj)
{
    func_004c54d8();
    ADXT_StartSj(adxt, sj);
    func_004c54e0();
}

void func_004d4778(void *adxt, int32_t *ncount, int32_t *tscale)
{
    func_004c54d8();
    ADXT_GetTime(adxt, ncount, tscale);
    func_004c54e0();
}

int32_t func_004c9b80(int32_t ptid)
{
    int32_t r;

    func_004c9d18();
    r = (int32_t)P4_H2G(func_004c9bb8(ptid));
    func_004c9d30();
    return r;
}

static uint32_t rd32(const uint8_t *p);
static uint32_t rd16(const uint8_t *p);

#define AIXP_TABLE 0x0072acc8u
#define AIXP_SIZE 0x26cu
#define AIXSJD_TABLE 0x0072b680u
#define AIXSJD_SIZE 0x195cu

/* 0x004de2c0: report an error through the ADX error callback. */
void func_004de2c0(const char *msg)
{
    func_004c6a98(msg);
}

/* 0x004de318: the two server callbacks, run once (the retail "execute server" entry). */
uint32_t func_004de318(void)
{
    extern uint32_t func_004de2e8(void);
    extern uint32_t func_004de310(void);

    func_004de2e8();
    func_004de310();
    return 0;
}

/* ---- the AIXSJD library ------------------------------------------------------------------------
 * AIXSJD handle: +0 used, +1 state (0 idle, 1 reading the AIX header, 2 demuxing, 3 ended, 4 error),
 * +2 tracks, +3 hold (set by the player until the first packets are in), +4 more input expected, +8
 * bytes read, +0x1c input joint, +0x20 the output joint of each track, +0xa0 the parsed header:
 *   +0 +1 two header bytes, +4 a word, +8 (float) a 16-bit value, +0xc tracks (byte), +0x10 + 16 * i the
 *   channels (byte) and +0x14 + 16 * i the sampling rate of track i, +0x210 phrases, +0x214 + 16 * i the
 *   four words of phrase i, +0x1894 songs, +0x1898 + 8 * i the four halfwords of song i;
 * +0x1958 the stream is raw (no AIX header). */

/* 0x004e0458 (locked wrapper) / 0x004e04b0: take a handle for `n` tracks writing to the joints sjo; 0 when
 * all four are in use. */
void *func_004e04b0(int32_t n, uint32_t arg, uint32_t *sjo)
{
    int32_t i, k;
    uint8_t *h;

    for (i = 0; i < 4; i++) {
        if (M8(AIXSJD_TABLE + (uint32_t)i * AIXSJD_SIZE) == 0) {
            break;
        }
    }
    if (i == 4) {
        return 0;
    }
    h = (uint8_t *)P4_G2H(AIXSJD_TABLE + (uint32_t)i * AIXSJD_SIZE);
    P32(h, 0x1c) = arg;
    P8(h, 1) = 0;
    for (k = 0; k < n; k++) {
        P32(h, 0x20 + k * 4) = sjo[k];
    }
    P8(h, 2) = (uint8_t)n;
    P8(h, 3) = 0;
    P8(h, 4) = 0;
    P8(h, 0) = 1;
    return h;
}

void *func_004e0458(int32_t n, uint32_t arg, uint32_t *sjo)
{
    void *h;

    func_004e0380();
    h = func_004e04b0(n, arg, sjo);
    func_004e0398();
    return h;
}

/* 0x004e0598 AIXSJD_Start: restart the demuxer at the head of the stream. */
void func_004e0598(uint8_t *h)
{
    func_004e0380();
    P32(h, 0x10) = 0;
    P8(h, 1) = 1;
    P32(h, 8) = 0;
    P32(h, 0xc) = 0;
    func_004e0398();
}

/* 0x004e05d8: input joint, 0x004e05e8: the hold byte, 0x004e05f0: the more-input byte. */
void func_004e05d8(uint8_t *h, uint32_t sj)
{
    P32(h, 0x1c) = sj;
}

void func_004e05e8(uint8_t *h, uint32_t v)
{
    P8(h, 3) = (uint8_t)v;
}

void func_004e05f0(uint8_t *h, uint32_t v)
{
    P8(h, 4) = (uint8_t)v;
}

/* 0x004e05f8..0x004e0728: the parsed header. */
int32_t func_004e05f8(uint8_t *h)
{
    return S8(h, 0xac);
}

uint32_t func_004e0600(uint8_t *h, int32_t i)
{
    return P32(h, 0xb4 + i * 16);
}

int32_t func_004e0610(uint8_t *h, int32_t i)
{
    return S8(h, 0xb0 + i * 16);
}

uint32_t func_004e0620(uint8_t *h)
{
    return P32(h, 0x2b0);
}

void func_004e0628(uint8_t *h, int32_t i, uint32_t *a, uint32_t *b)
{
    h += i * 16;
    if (a != 0) {
        *a = P32(h, 700);
    }
    if (b != 0) {
        *b = P32(h, 0x2c0);
    }
}

void func_004e0658(uint8_t *h, int32_t i, uint32_t *a, uint32_t *b)
{
    h += i * 16;
    if (a != 0) {
        *a = P32(h, 0x2b4);
    }
    if (b != 0) {
        *b = P32(h, 0x2b8);
    }
}

uint32_t func_004e0688(uint8_t *h)
{
    return P32(h, 0x1934);
}

void func_004e0690(uint8_t *h, int32_t i, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    h += i * 8;
    if (a != 0) {
        *a = P16(h, 0x1938);
    }
    if (b != 0) {
        *b = P16(h, 0x193a);
    }
    if (c != 0) {
        *c = P16(h, 0x193c);
    }
    if (d != 0) {
        *d = P16(h, 0x193e);
    }
}

/* 0x004e06f0: total samples of all phrases. */
int32_t func_004e06f0(uint8_t *h)
{
    int32_t n = S32(h, 0x2b0);
    int32_t sum = 0;
    int32_t i;

    for (i = 0; i < n; i++) {
        sum += S32(h, 700 + i * 16);
    }
    return sum;
}

/* 0x004e0728: the stream is raw. */
uint32_t func_004e0728(uint8_t *h)
{
    return P32(h, 0x1958);
}

static uint32_t rd32(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static uint32_t rd16(const uint8_t *p)
{
    return ((uint32_t)p[0] << 8) | p[1];
}

/* 0x004e0b30: the header of a stream that has none: one track, one phrase, one song. */
void func_004e0b30(uint8_t *hdr)
{
    func_0043f9c8(hdr, 0, 0x18b8);
    P16(hdr, 0x189a) = 1;
    P8(hdr, 0xc) = 1;
    P32(hdr, 0x210) = 1;
    P32(hdr, 0x1894) = 1;
    P16(hdr, 0x1898) = 0;
    P16(hdr, 0x189c) = 0;
    P16(hdr, 0x189e) = 0;
}

/* 0x004e0778: parse an AIX header ("AIXF", the header length, the "(c)CRI" mark that ends it) into
 * `hdr`. Returns the header length plus 8; 0 for a stream that is not an AIX (hdr is the raw default);
 * -1 when fewer than 0x1800 bytes are there or the end mark is missing. */
int32_t func_004e0778(const uint8_t *buf, int32_t len, uint8_t *hdr)
{
    const uint8_t *s = buf;
    const char *mark = (const char *)P4_G2H(0x00759c48u);
    uint32_t marklen = func_00442948(mark);
    uint32_t hlen;
    int32_t n, i, nsong;

    if (len < 0x1800) {
        return -1;
    }
    if (rd32(s) != 0x41495846u) { /* "AIXF" */
        func_004e0b30(hdr);
        return 0;
    }
    s += 4;
    hlen = rd32(s);
    s += 4;
    if (func_0043f778(buf + (hlen - marklen) + 8, mark, marklen) != 0) {
        return -1;
    }
    hdr[0] = s[0];
    hdr[1] = s[1];
    s += 2;
    *(float *)(hdr + 8) = (float)(int32_t)rd16(s);
    s += 2;
    P32(hdr, 4) = rd32(s);
    s += 4;
    s += 8;
    n = (int32_t)rd16(s);
    s += 2;
    P32(hdr, 0x210) = (uint32_t)n;
    s += 6;
    for (i = 0; i < n; i++) {
        P32(hdr, 0x214 + i * 16) = rd32(s);
        P32(hdr, 0x218 + i * 16) = rd32(s + 4);
        P32(hdr, 0x21c + i * 16) = rd32(s + 8);
        P32(hdr, 0x220 + i * 16) = rd32(s + 12);
        s += 16;
    }
    nsong = (int8_t)s[0];
    P32(hdr, 0x1894) = (uint32_t)nsong;
    s += 8;
    for (i = 0; i < nsong; i++) {
        P16(hdr, 0x1898 + i * 8) = (uint16_t)rd16(s);
        P16(hdr, 0x189a + i * 8) = (uint16_t)rd16(s + 2);
        P16(hdr, 0x189c + i * 8) = (uint16_t)rd16(s + 4);
        P16(hdr, 0x189e + i * 8) = (uint16_t)rd16(s + 6);
        s += 8;
    }
    P8(hdr, 0xc) = s[0];
    s += 8;
    for (i = 0; i < (int8_t)hdr[0xc]; i++) {
        P32(hdr, 0x14 + i * 16) = rd32(s);
        P8(hdr, 0x10 + i * 16) = s[4];
        s += 8;
    }
    return (int32_t)hlen + 8;
}

/* 0x004e0730: the checked entry. */
int32_t func_004e0730(const uint8_t *buf, int32_t len, uint8_t *hdr)
{
    if (buf == 0 || len <= 0 || hdr == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759c50u));
        return -1;
    }
    return func_004e0778(buf, len, hdr);
}

/* 0x004e0b80: a packet header (16 bytes: the tag, the size, the track, a byte, a halfword, a word). 16
 * when the data holds one, else 0. */
uint32_t func_004e0b80(const uint8_t *p, uint32_t len, uint8_t *out)
{
    if (len < 16) {
        return 0;
    }
    P32(out, 0) = rd32(p);
    P32(out, 4) = rd32(p + 4);
    out[8] = p[8];
    out[9] = p[9];
    P16(out, 10) = (uint16_t)rd16(p + 10);
    P32(out, 12) = rd32(p + 12);
    return 16;
}

/* 0x004e0c60 state 1: parse the file header from the input joint, hand the header bytes back and go
 * demuxing (2). */
void func_004e0c60(uint8_t *h)
{
    void *in = (void *)P4_G2H(P32(h, 0x1c));
    Ck ck, ck2;
    int32_t r;

    sj_get(in, 1, 0x1800, &ck);
    r = func_004e0778((const uint8_t *)P4_G2H(ck.data), ck.len, h + 0xa0);
    if (r == -1) {
        sj_unget(in, 1, &ck);
        return;
    }
    if (r == 0) {
        P32(h, 0x1958) = 1;
    } else {
        if (r != ck.len) {
            func_004de2c0((const char *)P4_G2H(0x00759cb0u));
            sj_unget(in, 1, &ck);
            P8(h, 1) = 4;
            return;
        }
        P32(h, 0x1958) = 0;
    }
    SJ_SplitChunk(&ck, r, &ck, &ck2);
    sj_put(in, 0, &ck);
    sj_unget(in, 1, &ck2);
    P8(h, 1) = 2;
    P32(h, 8) = P32(h, 8) + (uint32_t)r;
}

/* 0x004e0db0 state 2, AIX stream: one packet for each of the tracks. The packet's 16-byte header says the
 * track, its payload length and a word; the payload goes to that track's joint. Returns the word of the
 * last packet, -3 when nothing was read. An "AIXE" packet ends the stream. */
int32_t func_004e0db0(uint8_t *h)
{
    void *in = (void *)P4_G2H(P32(h, 0x1c));
    int32_t ret = -3;
    int32_t ntrk = S8(h, 0xac);
    int32_t done;

    for (done = 0; done < ntrk; done++) {
        Ck ck, ck2, ck3, ck4;
        uint8_t ph[16];
        int32_t trk, n;
        void *out;

        sj_get(in, 1, 0x7fffffff, &ck);
        if ((uint32_t)ck.len < 16 || func_004e0b80((const uint8_t *)P4_G2H(ck.data), (uint32_t)ck.len, ph) == 0 ||
            ck.len < S32(ph, 4) + 8) {
            sj_unget(in, 1, &ck);
            return ret;
        }
        if (P32(ph, 0) == 0x41495845u) { /* "AIXE" */
            SJ_SplitChunk(&ck, S32(ph, 4) + 8, &ck, &ck2);
            sj_put(in, 0, &ck);
            sj_unget(in, 1, &ck2);
            if (sj_num(in, 1) < 1) {
                P8(h, 4) = 0;
            }
            if (P8(h, 4) != 0) {
                return ret;
            }
            P8(h, 1) = 3;
            return ret;
        }
        trk = ph[8];
        if (S8(h, 2) <= trk) {
            func_004de2c0((const char *)P4_G2H(0x00759cd8u));
            sj_unget(in, 1, &ck);
            P8(h, 1) = 4;
            return ret;
        }
        out = (void *)P4_G2H(P32(h, 0x20 + trk * 4));
        if (sj_num(out, 0) < (int16_t)P16(ph, 10)) {
            sj_unget(in, 1, &ck);
            return ret;
        }
        ret = (int32_t)P32(ph, 12);
        SJ_SplitChunk(&ck, S32(ph, 4) + 8, &ck, &ck2);
        sj_unget(in, 1, &ck2);
        SJ_SplitChunk(&ck, 16, &ck, &ck3);
        sj_put(in, 0, &ck);
        n = (int16_t)P16(ph, 10);
        if (ck3.len < n) {
            func_004de2c0((const char *)P4_G2H(0x00759d00u));
            sj_unget(in, 1, &ck3);
            sj_unget(in, 1, &ck);
            P8(h, 1) = 4;
            return ret;
        }
        {
            int32_t copied = 0;

            while (copied < n) {
                sj_get(out, 0, ck3.len - copied, &ck4);
                memcpy(P4_G2H(ck4.data), P4_G2H(ck3.data + (uint32_t)copied), (size_t)ck4.len);
                sj_put(out, 1, &ck4);
                copied += ck4.len;
            }
        }
        sj_put(in, 0, &ck3);
        P32(h, 8) = P32(h, 8) + (uint32_t)ck3.len + 16;
    }
    return ret;
}

/* 0x004e1100 state 2, raw stream: everything the input holds goes to track 0. */
void func_004e1100(uint8_t *h)
{
    void *in = (void *)P4_G2H(P32(h, 0x1c));
    void *out = (void *)P4_G2H(P32(h, 0x20));
    Ck ck, ck2, ck4;

    sj_get(in, 1, 0x7fffffff, &ck);
    sj_get(out, 0, ck.len, &ck4);
    memcpy(P4_G2H(ck4.data), P4_G2H(ck.data), (size_t)ck4.len);
    sj_put(out, 1, &ck4);
    SJ_SplitChunk(&ck, ck4.len, &ck, &ck2);
    sj_put(in, 0, &ck);
    sj_unget(in, 1, &ck2);
    P32(h, 8) = P32(h, 8) + (uint32_t)ck4.len;
}

/* ---- the AIX player ---------------------------------------------------------------------------- */

/* 0x004de5c0: take a handle and build it in the work area (aligned to 64 bytes, `size` bytes):
 *   the tracks (an ADXT each, 0x4de... sizes below), an input ring joint per track, the input ring, the
 *   AIXSJD and the ADXSTM that fills the input ring. cfg: [0] tracks, [1] channels per track (byte),
 *   [2] 1 for the 3D (PL2) kind. 0 when nothing is free or the work area is too small. */
uint8_t *func_004de5c0(const int32_t *cfg, uint8_t *work, int32_t size)
{
    uint8_t *aligned = (uint8_t *)P4_G2H((P4_H2G(work) + 63u) & ~63u);
    int32_t rem = size - (int32_t)(aligned - work);
    int32_t i, k, tsize, r, tail;
    uint8_t *h, *wk;

    if (work == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759618u));
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (M8(AIXP_TABLE + (uint32_t)i * AIXP_SIZE) == 0) {
            break;
        }
    }
    if (i == 4) {
        return 0;
    }
    h = (uint8_t *)P4_G2H(AIXP_TABLE + (uint32_t)i * AIXP_SIZE);
    func_0043f9c8(h, 0, AIXP_SIZE);
    P8(h, 3) = (uint8_t)cfg[0];
    P8(h, 2) = (uint8_t)cfg[1];
    P32(h, 0x1a0) = 0x2000;
    P32(h, 0x268) = (uint32_t)cfg[2];
    P32(h, 0x1a4) = 0x40;
    if (cfg[2] == 1) {
        tsize = cfg[0] * 0x85c0;
    } else {
        tsize = (cfg[1] * 0x40c0 + 0x40) * cfg[0];
    }
    r = func_004dfe60(h, aligned, tsize);
    rem -= tsize;
    if (r < 0 || rem < 0) {
        goto fail;
    }
    wk = aligned + tsize;
    for (k = 0; k < S8(h, 3); k++) {
        int32_t rsz = (int32_t)P32(h, 0x1a0);
        int32_t extra = (int32_t)P32(h, 0x1a4);

        P32(h, 0x120 + k * 4) = P4_H2G(wk);
        rem -= rsz + extra;
        if (rem < 0) {
            goto fail;
        }
        wk += rsz + extra;
        P32(h, 0x94 + k * 4) = P4_H2G(SJRBF_Create((void *)P4_G2H(P32(h, 0x120 + k * 4)), rsz, extra));
        if (P32(h, 0x94 + k * 4) == 0) {
            goto fail;
        }
    }
    P32(h, 0x114) = P4_H2G(wk);
    tail = rem - 0x2000;
    tail = ((tail < 0 ? rem - 0x1801 : tail) >> 11) * 0x800;
    P32(h, 0x11c) = 0x2000;
    P32(h, 0x90) = 0;
    P32(h, 0x118) = (uint32_t)tail;
    P32(h, 0x8c) = P4_H2G(SJRBF_Create(wk, tail, 0x2000));
    if (P32(h, 0x8c) == 0) {
        goto fail;
    }
    P32(h, 4) = P4_H2G(func_004e0458(cfg[0], P32(h, 0x90), (uint32_t *)(h + 0x94)));
    if (P32(h, 4) == 0) {
        goto fail;
    }
    P32(h, 8) = P4_H2G(func_004d1c58((void *)P4_G2H(P32(h, 0x8c)), 0));
    if (P32(h, 8) == 0) {
        goto fail;
    }
    func_004d2348((void *)P4_G2H(P32(h, 8)), (void *)func_004df5b0, h);
    P32(h, 0x1a8) = 0;
    P8(h, 0x1b1) = 1;
    P8(h, 0) = 1;
    P8(h, 0x1b0) = 0;
    P32(h, 0x1b8) = 0;
    P32(h, 0x1c0) = 0;
    P32(h, 0x1bc) = 0;
    P32(h, 0x1c4) = 0;
    P32(h, 0x1c8) = 1;
    P32(h, 0x1e0) = 0;
    return h;
fail:
    func_004de860(h);
    return 0;
}

/* 0x004de4c0, 0x004de540: aixp_Create(tracks, channels, work, size) and aixp_Create3D(tracks, work, size) */
void *aixp_Create(int32_t tracks, int32_t channels, void *work, int32_t size)
{
    int32_t cfg[3];
    void *h;

    func_0043f9c8(cfg, 0, 12);
    cfg[0] = tracks;
    cfg[1] = channels;
    cfg[2] = 0;
    func_004e0380();
    h = func_004de5c0(cfg, work, size);
    func_004e0398();
    return h;
}

void *aixp_Create3D(int32_t tracks, void *work, int32_t size)
{
    int32_t cfg[3];
    void *h;

    func_0043f9c8(cfg, 0, 12);
    cfg[0] = tracks;
    cfg[1] = 2;
    cfg[2] = 1;
    func_004e0380();
    h = func_004de5c0(cfg, work, size);
    func_004e0398();
    return h;
}

/* 0x004de860: destroy a handle: stop it, free the tracks, the ring joints, the AIXSJD, and clear it. */
void func_004de860(uint8_t *h)
{
    int32_t k;

    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759648u));
        return;
    }
    func_004ded60(h);
    if (P32(h, 8) != 0) {
        void *stm = (void *)P4_G2H(P32(h, 8));

        P32(h, 8) = 0;
        func_004d2348(stm, 0, 0);
        func_004d1ce8(stm);
    }
    if (P32(h, 0xc) != 0) {
        func_004dff78(h);
    }
    func_004e0380();
    for (k = 0; k < S8(h, 3); k++) {
        void *sj = (void *)P4_G2H(P32(h, 0x94 + k * 4));

        if (sj != 0) {
            P32(h, 0x94 + k * 4) = 0;
            ((void (*)(void *))p4_fn_g2h_sig(sj_vt(sj)[3], P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(sj);
        }
    }
    if (P32(h, 0x8c) != 0) {
        void *sj = (void *)P4_G2H(P32(h, 0x8c));

        P32(h, 0x8c) = 0;
        ((void (*)(void *))p4_fn_g2h_sig(sj_vt(sj)[3], P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(sj);
    }
    if (P32(h, 4) != 0) {
        void *s = (void *)P4_G2H(P32(h, 4));

        P32(h, 4) = 0;
        func_004e0560(s);
    }
    func_0043f9c8(h, 0, AIXP_SIZE);
    P8(h, 0) = 0;
    func_004e0398();
}

/* 0x004de998: destroy every handle in use. */
void func_004de998(void)
{
    int32_t i;

    for (i = 0; i < 4; i++) {
        uint8_t *h = (uint8_t *)P4_G2H(AIXP_TABLE + (uint32_t)i * AIXP_SIZE);

        if (S8(h, 0) == 1) {
            func_004de860(h);
        }
    }
}

/* 0x004de9f8: start playing a file: name, directory, first sector, sector count. */
void func_004de9f8(uint8_t *h, uint32_t fname, uint32_t dir, int32_t ofst, int32_t nsct)
{
    if (h == 0 || fname == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759678u));
        return;
    }
    if (ofst < 0) {
        func_004de2c0((const char *)P4_G2H(0x007596a8u));
        return;
    }
    if (nsct <= 0) {
        func_004de2c0((const char *)P4_G2H(0x007596d8u));
        return;
    }
    func_004ded60(h);
    func_004e0380();
    P32(h, 0x1cc) = fname;
    P32(h, 0x1d4) = (uint32_t)ofst;
    P32(h, 0x1d0) = dir;
    P32(h, 0x1d8) = (uint32_t)nsct;
    P32(h, 0x90) = P32(h, 0x8c);
    func_004e05d8((uint8_t *)P4_G2H(P32(h, 4)), P32(h, 0x8c));
    P32(h, 0x1dc) = 0;
    P8(h, 1) = 1;
    P8(h, 0x1b2) = 1;
    func_004e0398();
}

/* 0x004deb38: the whole file. */
void func_004deb38(uint8_t *h, uint32_t fname, uint32_t dir)
{
    func_004de9f8(h, fname, dir, 0, 0xfffff);
}

/* 0x004deb58: start playing file `fid` of the AFS partition `ptid`. */
void func_004deb58(uint8_t *h, int32_t ptid, int32_t fid)
{
    int32_t a, b, c;

    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759708u));
        return;
    }
    func_004ded60(h);
    func_004e0380();
    if (func_004c99a0(ptid, fid, (void *)P4_G2H(0x009230f0u), &a, &b, &c) == 0) {
        P32(h, 0x1cc) = (uint32_t)func_004c9b80(ptid);
        P32(h, 0x90) = P32(h, 0x8c);
        P32(h, 0x1d0) = (uint32_t)a;
        P32(h, 0x1d4) = (uint32_t)b;
        P32(h, 0x1d8) = (uint32_t)c;
        func_004e05d8((uint8_t *)P4_G2H(P32(h, 4)), P32(h, 0x8c));
        P8(h, 1) = 1;
        P32(h, 0x1dc) = 1;
        P8(h, 0x1b2) = 1;
    }
    func_004e0398();
}

/* 0x004dec30: start playing from a stream joint the caller fills. */
void func_004dec30(uint8_t *h, uint32_t sj)
{
    if (h == 0 || sj == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759738u));
        return;
    }
    func_004ded60(h);
    func_004e0380();
    P32(h, 0x90) = sj;
    func_004e05d8((uint8_t *)P4_G2H(P32(h, 4)), sj);
    P32(h, 0x1dc) = 3;
    P8(h, 1) = 1;
    func_004e0398();
}

/* 0x004decb8: start playing from memory (a memory joint is made over it). */
void func_004decb8(uint8_t *h, void *data, int32_t size)
{
    if (h == 0 || data == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759768u));
        return;
    }
    func_004ded60(h);
    func_004e0380();
    P32(h, 0x90) = P4_H2G(func_004ed600(data, size));
    func_004e05d8((uint8_t *)P4_G2H(P32(h, 4)), P32(h, 0x90));
    P32(h, 0x1dc) = 2;
    P8(h, 1) = 1;
    func_004e0398();
}

/* 0x004ded60: stop: the tracks, the stream and the demuxer; a memory joint made by the start is destroyed. */
void func_004ded60(uint8_t *h)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759798u));
        return;
    }
    func_004dfff0(h);
    if (P32(h, 8) != 0) {
        func_004d2230((void *)P4_G2H(P32(h, 8)));
    }
    func_004e0380();
    if (P32(h, 4) != 0) {
        P8((uint8_t *)P4_G2H(P32(h, 4)), 1) = 0;
    }
    if (S32(h, 0x1dc) != 2) {
        P32(h, 0x1ac) = 0;
    } else {
        void *sj = (void *)P4_G2H(P32(h, 0x90));

        P32(h, 0x90) = 0;
        if (sj != 0) {
            ((void (*)(void *))p4_fn_g2h_sig(sj_vt(sj)[3], P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(sj);
        }
        P32(h, 0x1ac) = 0;
    }
    P32(h, 0x90) = 0;
    P8(h, 1) = 0;
    func_004e0398();
}

/* 0x004dee48: the first track's header values (two words) */
void func_004dee48(uint8_t *h, uint32_t *a, uint32_t *b)
{
    int32_t x = 0, y = 0;

    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x007597f8u));
        return;
    }
    func_004d4778((void *)P4_G2H(P32(h, 0xc)), &x, &y);
    if (a != 0) {
        *a = (uint32_t)x;
    }
    if (b != 0) {
        *b = (uint32_t)y;
    }
}

/* 0x004deec0: the ADXT of track i. */
void *func_004deec0(uint8_t *h, int32_t i)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759828u));
        return 0;
    }
    if (i >= 0x21) {
        func_004de2c0((const char *)P4_G2H(0x00759858u));
        return 0;
    }
    return (void *)P4_G2H(P32(h, 0xc + i * 4));
}

/* 0x004def18: size the stream's buffer refill: at the start position (a fraction of the byte rate) when one
 * is set, else half the ring. */
void func_004def18(uint8_t *h)
{
    float pos = *(float *)(h + 0x1e0);

    if (pos == 0.0f) {
        int32_t n = S32(h, 0x118);

        func_004d2c30((void *)P4_G2H(P32(h, 8)), (n + (int32_t)((uint32_t)n >> 31)) >> 1, n);
    } else {
        int32_t rate = AIXP_GetByteRate(h);

        func_004d2c30((void *)P4_G2H(P32(h, 8)), (int32_t)((float)rate * pos), S32(h, 0x118));
    }
}

/* 0x004defa0: the start position as a fraction of the byte rate. */
void func_004defa0(uint8_t *h, float pos)
{
    *(float *)(h + 0x1e0) = pos;
    func_004e0380();
    if (S8(h, 1) == 2) {
        func_004def18(h);
    }
    func_004e0398();
}

/* 0x004defe8: the fraction of a second of data the input ring holds. */
float func_004defe8(uint8_t *h)
{
    int32_t rate = AIXP_GetByteRate(h);
    float f = 0.0f;

    if (rate != 0 && P32(h, 0x90) != 0) {
        f = (float)sj_num((void *)P4_G2H(P32(h, 0x90)), 1) / (float)rate;
    }
    return f;
}

/* 0x004df068, 0x004df098, 0x004df0c8, 0x004df0f8: loop switches and the first phrase. */
void func_004df068(uint8_t *h, uint32_t v)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759890u));
        return;
    }
    P8(h, 0x1b0) = (uint8_t)v;
}

void func_004df098(uint8_t *h, uint32_t v)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x007598c0u));
        return;
    }
    P8(h, 0x1b1) = (uint8_t)v;
}

void func_004df0c8(uint8_t *h, int32_t v)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x007598f0u));
        return;
    }
    S32(h, 0x1c0) = v;
}

/* 0x004df128, 0x004df1a0: the loop range of phrases: first, count. */
void func_004df128(uint8_t *h, int32_t first, int32_t count)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759950u));
        return;
    }
    func_004e0380();
    S32(h, 0x1c4) = first;
    S32(h, 0x1c8) = first + count;
    func_004e0398();
}

void func_004df1a0(uint8_t *h, int32_t *first, int32_t *count)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759980u));
        return;
    }
    if (first != 0) {
        *first = S32(h, 0x1c4);
    }
    if (count != 0) {
        *count = S32(h, 0x1c8) - S32(h, 0x1c4);
    }
}

/* 0x004df220, 0x004df260: pause. */
void func_004df260(uint8_t *h, int32_t sw)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x007599e0u));
        return;
    }
    S32(h, 0x1a8) = sw;
    if (S8(h, 1) != 1) {
        func_004e0058(h, sw);
    }
}

void func_004df220(uint8_t *h, int32_t sw)
{
    func_004e0380();
    func_004df260(h, sw);
    func_004e0398();
}

/* 0x004df320: total samples (the sum of the phrases, or the one track's), -1 for no handle, 0 when the
 * handle is not started. */
int32_t AIXP_GetNumSmpl(uint8_t *h)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759a50u));
        return -1;
    }
    if ((uint32_t)(P8(h, 1) - 1) >= 3) {
        return 0;
    }
    {
        uint8_t *s = (uint8_t *)P4_G2H(P32(h, 4));
        uint32_t raw = func_004e0728(s);

        if (raw == 0) {
            return func_004e06f0(s);
        }
        if (raw == 1) {
            return func_004d4b48((void *)P4_G2H(P32(h, 0xc)));
        }
    }
    return 0;
}

/* 0x004df3c0, 0x004df418, 0x004df470: tracks, phrases, songs. */
int32_t AIXP_GetNumTrack(uint8_t *h)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759a88u));
        return -1;
    }
    if ((uint32_t)(P8(h, 1) - 1) >= 3) {
        return 0;
    }
    return func_004e05f8((uint8_t *)P4_G2H(P32(h, 4)));
}

int32_t AIXP_GetNumPhrase(uint8_t *h)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759ac8u));
        return -1;
    }
    if ((uint32_t)(P8(h, 1) - 1) >= 3) {
        return 0;
    }
    return (int32_t)func_004e0620((uint8_t *)P4_G2H(P32(h, 4)));
}

int32_t AIXP_GetNumSong(uint8_t *h)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759b08u));
        return -1;
    }
    if ((uint32_t)(P8(h, 1) - 1) >= 3) {
        return 0;
    }
    return (int32_t)func_004e0688((uint8_t *)P4_G2H(P32(h, 4)));
}

/* 0x004df4c8: bytes per second of the stream: for every track rate * channels * 18 / 32 (an ADX frame
 * of 18 bytes holds 32 samples of a channel). */
int32_t AIXP_GetByteRate(uint8_t *h)
{
    uint8_t *s;
    int32_t n, i, sum = 0;

    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759b40u));
        return -1;
    }
    if ((uint32_t)(P8(h, 1) - 1) >= 3) {
        return 0;
    }
    s = (uint8_t *)P4_G2H(P32(h, 4));
    n = func_004e05f8(s);
    for (i = 0; i < n; i++) {
        int32_t v = (int32_t)func_004e0600(s, i) * func_004e0610(s, i);

        v = v * 9 * 2;
        sum += (v < 0 ? v + 31 : v) >> 5;
    }
    return sum;
}

/* 0x004df5b0: the stream's end-of-range callback: next phrase of the loop range, reading from its sectors. */
void func_004df5b0(uint8_t *h)
{
    int32_t cur, n;

    if (S32(h, 0x1ac) != 1 && S8(h, 1) != 2) {
        return;
    }
    cur = S32(h, 0x1bc) + 1;
    S32(h, 0x1bc) = cur;
    if (S8(h, 0x1b0) == 1 && cur >= S32(h, 0x1c8)) {
        S32(h, 0x1bc) = S32(h, 0x1c4);
    }
    if (S8(h, 0x1b1) != 1) {
        return;
    }
    n = (int32_t)func_004e0620((uint8_t *)P4_G2H(P32(h, 4)));
    if (S32(h, 0x1bc) >= n) {
        return;
    }
    func_004e0380();
    {
        uint8_t *s = (uint8_t *)P4_G2H(P32(h, 4));
        uint32_t raw = func_004e0728(s);
        void *stm = (void *)P4_G2H(P32(h, 8));

        if (raw == 0) {
            uint32_t a, b;
            int32_t pos;

            func_004e0658(s, S32(h, 0x1bc), &a, &b);
            pos = (int32_t)a;
            func_004d2038(stm, (pos < 0 ? pos + 2047 : pos) >> 11);
            pos = (int32_t)(a + b);
            func_004d23a8(stm, (pos < 0 ? pos + 2047 : pos) >> 11);
            func_004d2128(stm);
        } else if (raw == 1) {
            func_004d2038(stm, 0);
            func_004d23a8(stm, -1);
            func_004d2128(stm);
        }
    }
    func_004e0398();
}

/* 0x004df708: start the stream: rewind the rings, bind the file and read from sector 0. */
void func_004df708(uint8_t *h)
{
    int32_t k;
    void *stm = (void *)P4_G2H(P32(h, 8));
    int32_t n;

    func_004d1f00(stm);
    sj_reset((void *)P4_G2H(P32(h, 0x90)));
    for (k = 0; k < S8(h, 3); k++) {
        sj_reset((void *)P4_G2H(P32(h, 0x94 + k * 4)));
    }
    func_004d1d70(stm, P4_G2H(P32(h, 0x1cc)), P4_G2H(P32(h, 0x1d0)), (int32_t)P32(h, 0x1d4), (int32_t)P32(h, 0x1d8));
    n = S32(h, 0x118);
    func_004d2c30(stm, (n + (int32_t)((uint32_t)n >> 31)) >> 1, 0x7fffffff);
    func_004d2038(stm, 0);
    func_004d23a8(stm, 3);
    func_004d2128(stm);
    func_004e05e8((uint8_t *)P4_G2H(P32(h, 4)), 1);
    func_004e0598((uint8_t *)P4_G2H(P32(h, 4)));
}

/* 0x004df800: the player's step, at most once for every call of the library's step (a flag the server
 * sets), then 0x004df850. */
void func_004df800(uint8_t *h)
{
    if (func_004f1278((uint32_t *)P4_G2H(0x0072acbcu)) != 0) {
        func_004df850(h);
        M32(0x0072acbcu) = 0;
    }
}

/* 0x004df850: one step of a player. State 1 waits for the stream to be up, starts the tracks and moves
 * to 2; state 2 follows the demuxer and the tracks to 3 (ended); the stream going into error is 4. */
void func_004df850(uint8_t *h)
{
    uint8_t *sjd;
    int32_t st;
    int32_t k;

    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759b80u));
        return;
    }
    sjd = (uint8_t *)P4_G2H(P32(h, 4));
    st = S8(h, 1);
    if (st == 1) {
        func_004e0058(h, 1);
        if (P32(h, 0x1ac) == 0) {
            if (P32(h, 0x1dc) < 2) {
                int32_t stm = func_004d1ff8((void *)P4_G2H(P32(h, 8)));

                func_004e0380();
                if (S8(h, 0x1b2) == 1 && stm != 2) {
                    P8(h, 0x1b2) = 0;
                    func_004e0398();
                    func_004df708(h);
                } else {
                    func_004e0398();
                }
                if (S8(sjd, 1) == 2) {
                    uint32_t raw;

                    func_004e05f8(sjd);
                    raw = func_004e0728(sjd);
                    if (raw == 0) {
                        int32_t first = S32(h, 0x1c0);
                        uint32_t a, b;
                        int32_t pos;

                        S32(h, 0x1bc) = first;
                        if (first < 0 || first >= (int32_t)func_004e0620(sjd)) {
                            P8(h, 1) = 3;
                            return;
                        }
                        func_004e0658(sjd, first, &a, &b);
                        pos = (int32_t)a;
                        func_004d2038((void *)P4_G2H(P32(h, 8)), (pos < 0 ? pos + 2047 : pos) >> 11);
                        pos = (int32_t)(a + b);
                        func_004d23a8((void *)P4_G2H(P32(h, 8)), (pos < 0 ? pos + 2047 : pos) >> 11);
                        func_004e05e8(sjd, 0);
                        if (S8(h, 0x1b0) == 1 || S8(h, 0x1b1) == 1) {
                            func_004e05f0(sjd, 1);
                        }
                        func_004e0158(h, 1);
                        func_004e00d8(h);
                    } else if (raw == 1) {
                        func_004d23a8((void *)P4_G2H(P32(h, 8)), -1);
                        func_004e05e8(sjd, 0);
                        if (S8(h, 0x1b0) == 1 || S8(h, 0x1b1) == 1) {
                            func_004e05f0(sjd, 1);
                        }
                        func_004d65b8((void *)P4_G2H(P32(h, 0xc)), 1);
                        func_004d4170((void *)P4_G2H(P32(h, 0xc)), (void *)P4_G2H(P32(h, 0x94)));
                    }
                    P32(h, 0x1ac) = 1;
                }
            } else {
                int32_t sst = S8(sjd, 1);

                if (sst == 0) {
                    for (k = 0; k < S8(h, 3); k++) {
                        sj_reset((void *)P4_G2H(P32(h, 0x94 + k * 4)));
                    }
                    func_004e0598(sjd);
                } else if (sst == 2) {
                    func_004e00d8(h);
                    P32(h, 0x1ac) = 1;
                }
            }
        } else if (func_004e01c0(h) == 1) {
            func_004e0380();
            if (P32(h, 0x1a8) == 0) {
                func_004e0058(h, 0);
            }
            func_004def18(h);
            P8(h, 1) = 2;
            P32(h, 0x1ac) = 0;
            func_004e0398();
        }
        st = S8(h, 1);
    }
    if (st == 2) {
        uint32_t raw = func_004e0728(sjd);

        if (raw == 0) {
            if (S8(sjd, 1) == 3 && func_004e0268(h) == 1) {
                P8(h, 1) = 3;
            }
        } else if (raw == 1) {
            if (func_004d1ff8((void *)P4_G2H(P32(h, 8))) != 2) {
                int32_t t = func_004d43f8((void *)P4_G2H(P32(h, 0xc)));

                if (t == 5 || t == 0) {
                    P8(h, 1) = 3;
                }
            }
        }
    }
    func_004e0380();
    if (P32(h, 8) != 0 && func_004d1ff8((void *)P4_G2H(P32(h, 8))) == 4) {
        P8(h, 1) = 4;
    }
    func_004e0398();
}

/* 0x004dfc98, 0x004dfcd8: master volume (1/10 dB), added to every track's own. */
void func_004dfcd8(uint8_t *h, int32_t vol)
{
    int32_t k;

    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759bb0u));
        return;
    }
    S32(h, 0x1e4) = vol;
    for (k = 0; k < S8(h, 3); k++) {
        func_004dfe38(h, k, S32(h, 0x1e4) + S32(h, 0x1e8 + k * 4));
    }
}

void func_004dfc98(uint8_t *h, int32_t vol)
{
    func_004e0380();
    func_004dfcd8(h, vol);
    func_004e0398();
}

/* 0x004dfd78, 0x004dfdc8: one track's volume. */
void func_004dfdc8(uint8_t *h, int32_t trk, int32_t vol)
{
    if (h == 0) {
        func_004de2c0((const char *)P4_G2H(0x00759be0u));
        return;
    }
    if (trk >= 33) {
        func_004de2c0((const char *)P4_G2H(0x00759c10u));
        return;
    }
    S32(h, 0x1e8 + trk * 4) = vol;
    func_004dfe38(h, trk, S32(h, 0x1e4) + vol);
}

void func_004dfd78(uint8_t *h, int32_t trk, int32_t vol)
{
    func_004e0380();
    func_004dfdc8(h, trk, vol);
    func_004e0398();
}

/* 0x004dfe38: the output volume of one track's ADXT. */
void func_004dfe38(uint8_t *h, int32_t trk, int32_t vol)
{
    func_004d5110((void *)P4_G2H(P32(h, 0xc + trk * 4)), vol);
}

/* 0x004dfe60: create the ADXT of every track in `work` (`size` bytes after aligning to 64): a PL2
 * encoder ADXT of 0x85c0 bytes each for the 3D kind, else an ADXT of the channel count. 0, or -1 when the
 * area is too small or a track cannot be made. */
int32_t func_004dfe60(uint8_t *h, uint8_t *work, int32_t size)
{
    uint8_t *aligned = (uint8_t *)P4_G2H((P4_H2G(work) + 63u) & ~63u);
    int32_t rem = size - (int32_t)(aligned - work);
    int32_t tsize, k;

    if (S32(h, 0x268) == 1) {
        tsize = 0x85c0;
    } else {
        tsize = (S8(h, 2) * 0x40c0 + 127) & ~63;
    }
    for (k = 0; k < S8(h, 3); k++) {
        uint8_t *wk = aligned;
        void *adxt;

        rem -= tsize;
        aligned += tsize;
        if (rem < 0) {
            return -1;
        }
        if (S32(h, 0x268) == 1) {
            adxt = func_004d3bc0(wk, tsize);
        } else {
            adxt = func_004d3850(S8(h, 2), wk, tsize);
        }
        P32(h, 0xc + k * 4) = P4_H2G(adxt);
        if (adxt == 0) {
            return -1;
        }
    }
    return 0;
}

/* 0x004dff78: destroy the tracks' ADXTs. */
void func_004dff78(uint8_t *h)
{
    int32_t k;

    for (k = 0; k < S8(h, 3); k++) {
        uint32_t t = P32(h, 0xc + k * 4);

        if (t != 0) {
            P32(h, 0xc + k * 4) = 0;
            func_004d3cd8((void *)P4_G2H(t));
        }
    }
}

/* 0x004dfff0: stop the tracks. */
void func_004dfff0(uint8_t *h)
{
    int32_t k;

    for (k = 0; k < S8(h, 3); k++) {
        func_004d4308((void *)P4_G2H(P32(h, 0xc + k * 4)));
    }
}

/* 0x004e0058: pause or resume the tracks. */
void func_004e0058(uint8_t *h, int32_t sw)
{
    int32_t k;

    for (k = 0; k < S8(h, 3); k++) {
        uint32_t t = P32(h, 0xc + k * 4);

        if (t != 0) {
            func_004d6050((void *)P4_G2H(t), sw);
        }
    }
}

/* 0x004e00d8: start every track of the stream on its ring joint. */
void func_004e00d8(uint8_t *h)
{
    int32_t n = func_004e05f8((uint8_t *)P4_G2H(P32(h, 4)));
    int32_t k;

    for (k = 0; k < n; k++) {
        func_004d4170((void *)P4_G2H(P32(h, 0xc + k * 4)), (void *)P4_G2H(P32(h, 0x94 + k * 4)));
    }
}

/* 0x004e0158: the link switch of the stream's tracks. */
void func_004e0158(uint8_t *h, int32_t sw)
{
    int32_t n = func_004e05f8((uint8_t *)P4_G2H(P32(h, 4)));
    int32_t k;

    for (k = 0; k < n; k++) {
        func_004d65b8((void *)P4_G2H(P32(h, 0xc + k * 4)), sw);
    }
}

/* 0x004e01c0: 1 when every track of the stream is playing (3); a track in error (6) puts the player in error (4). */
uint32_t func_004e01c0(uint8_t *h)
{
    int32_t n = func_004e05f8((uint8_t *)P4_G2H(P32(h, 4)));
    int32_t k;

    for (k = 0; k < n; k++) {
        int32_t st = func_004d43f8((void *)P4_G2H(P32(h, 0xc + k * 4)));

        if (st == 6) {
            P8(h, 1) = 4;
        }
        if (st != 3) {
            break;
        }
    }
    return k == n;
}

/* 0x004e0268: 1 when the demuxer has ended and every track has played to the end (5) or stopped (0). */
uint32_t func_004e0268(uint8_t *h)
{
    int32_t k = 0;

    if (S8((uint8_t *)P4_G2H(P32(h, 4)), 1) != 3) {
        return 0;
    }
    while (k < S8(h, 3)) {
        int32_t st = func_004d43f8((void *)P4_G2H(P32(h, 0xc + k * 4)));

        if (st != 5 && st != 0) {
            break;
        }
        k++;
    }
    return k == S8(h, 3);
}

/* 0x004e0308: the decryption key string of every track. */
void func_004e0308(uint8_t *h, const char *key)
{
    int32_t k;

    for (k = 0; k < S8(h, 3); k++) {
        func_004d6bc8((void *)P4_G2H(P32(h, 0xc + k * 4)), key);
    }
}

/* 0x004d3bc0: an ADXT with the Pro Logic II encoder: the ADXT from the work area past the first 1024 bytes
 * and the encoder in those 1024 (0x004d3c08). */
extern void func_004c54d8(void);
extern void func_004c54e0(void);
extern void *func_004d3c08(void *work, int32_t size);

void *func_004d3bc0(void *work, int32_t size)
{
    void *r;

    func_004c54d8();
    r = func_004d3c08(work, size);
    func_004c54e0();
    return r;
}
