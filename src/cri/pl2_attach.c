/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI libadxe Dolby Pro Logic II encoder: create, encode a block, attach to an ADXT, translated from the
 * retail asm 0x004EA740..0x004EB180 and the ADXT create variant at 0x004D3C08 that attaches it.
 * Each entry point takes the PL2 lock (0x004ea710 / 0x004ea728) around an inner function; the
 * single-sample encoder (0x004eaae8), reset (0x004eaa48), parameter setter (0x004eaea8) and the
 * hooks 0x004eb228..0x004eb2e8 are in ci_pl.c and the decomp. The game's ACSSND parameter blocks
 * never ask for the encoder, so only the ACSSBAIX vtable reaches 0x004d3bc0. */
#include <stdint.h>

#include "cri_mem.h"

extern void func_004ea710(void);                                         /* PL2 lock */
extern int32_t func_004ea728(void);                                      /* PL2 unlock */
extern void func_004ea798(const char *msg);                              /* PL2 error report */
extern void func_004eaa48(void *pl2);                                    /* reset */
extern int32_t func_004eae58(void *pl2, int32_t a, int32_t b);           /* set parameters, locked */
extern void PL2LINK_DetachDolbyProLogicII(void *adxt);                   /* detach hook, 0x004eb228 */
extern void func_004eb298(void *adxb);
extern void PL2LINK_Encode(uint8_t *adxb, int16_t smp, int16_t *outl, int16_t *outr); /* 0x004eb2b0 */
extern void func_004eb2e8(uint32_t adxb);   /* decomp: its address is stored as a code word */
extern void func_004eb300(void);                                         /* error callback target */
extern void func_004c6a98(const char *msg);                              /* ADXERR_CallErrFunc1 */
extern void func_004cc9d0(void);                                         /* ADXM lock */
extern void func_004cc9e8(void);                                         /* ADXM unlock */
extern void func_004d4308(void *adxt);                                   /* ADXT_Stop */
extern void *adxt_Create(int32_t maxnch, void *work, int32_t size);        /* src/cri/adxt_wrap.c */
extern void *func_0043f9c8(void *d, int32_t c, uint32_t n);              /* memset */
extern int32_t func_0043f778(const void *a, const void *b, uint32_t n);  /* memcmp */

/* 0x004ea780: set the PL2 error function and its object. */
void func_004ea780(uint32_t fn, uint32_t obj)
{
    M32(0x0073978c) = fn;
    M32(0x00739790) = obj;
}

/* 0x004ea740: func_004ea780 under the PL2 lock. */
int32_t func_004ea740(uint32_t fn, uint32_t obj)
{
    func_004ea710();
    func_004ea780(fn, obj);
    return func_004ea728();
}

/* 0x004ea7f8: PL2 library init (the retail memcmp of the library state against itself has no effect). */
void func_004ea7f8(void)
{
    func_0043f778((const void *)P4_G2H(0x00739758u), (const void *)P4_G2H(0x00739758u), 360);
    M32(0x00739788) = M32(0x00739788) + 1;
}

/* 0x004ea7c8: func_004ea7f8 under the PL2 lock. */
int32_t func_004ea7c8(uint32_t a0)
{
    (void)a0;
    func_004ea710();
    func_004ea7f8();
    return func_004ea728();
}

/* 0x004ea8c8: create an encoder in a work area of at least 0x400 bytes: a 44-byte control block
 * (the work address and size, the two filter state arrays of 96 words at +8 and +12), reset.
 * Returns the control block, or 0 after an error report. */
void *func_004ea8c8(uint8_t *work, int32_t size)
{
    uint32_t pg, qg;
    uint8_t *p;
    const char *msg;

    func_0043f778((const void *)P4_G2H(0x00739758u), (const void *)P4_G2H(0x00739758u), 360);
    if (work == 0) {
        msg = (const char *)P4_G2H(0x0075c480u);
    } else if (size < 1024) {
        msg = (const char *)P4_G2H(0x0075c4a8u);
    } else {
        pg = P4_H2G(work);
        if (pg & 3) {
            pg = pg - (pg & 3) + 4;
        }
        p = (uint8_t *)P4_G2H(pg);
        func_0043f9c8(p, 0, 44);
        P32(p, 0) = P4_H2G(work);
        P32(p, 4) = (uint32_t)size;
        qg = pg + 44;
        if (qg & 3) {
            qg = qg - (qg & 3) + 4;
        }
        P32(p, 8) = qg;
        P32(p, 12) = qg + 384;
        func_004eaa48(p);
        if (P4_H2G(work) + (uint32_t)size >= qg + 768) {
            return p;
        }
        msg = (const char *)P4_G2H(0x0075c4d0u);
    }
    func_004ea798(msg);
    return 0;
}

/* 0x004ea880: func_004ea8c8 under the PL2 lock. */
void *func_004ea880(uint8_t *work, int32_t size)
{
    void *r;

    func_004ea710();
    r = func_004ea8c8(work, size);
    func_004ea728();
    return r;
}

/* 0x004eacb8: encode 32 samples: in[] (16-bit mono) to left and right out[]. Each sample is scaled by
 * two table gains (+0x1c, +0x20, table at 0x0075c080, 12-bit fixed point), each of those by the direct
 * gain (+0x24) added to the output and by the surround gain (+0x28) fed through a 2x2 rotation into
 * the filter state arrays (+8, +12); outputs are clamped to 16 bits. */
void func_004eacb8(void *pl2, const int16_t *in, int16_t *outl, int16_t *outr)
{
    int32_t *sl = (int32_t *)P4_G2H(P32(pl2, 8));
    int32_t *sr = (int32_t *)P4_G2H(P32(pl2, 12));
    int32_t i;

    for (i = 0; i < 32; i++) {
        int32_t s = in[i];
        int32_t g0 = (int32_t)M32(0x0075c080 + P32(pl2, 0x1c) * 4);
        int32_t g1 = (int32_t)M32(0x0075c080 + P32(pl2, 0x20) * 4);
        int32_t g2 = (int32_t)M32(0x0075c080 + P32(pl2, 0x24) * 4);
        int32_t g3 = (int32_t)M32(0x0075c080 + P32(pl2, 0x28) * 4);
        int32_t u0 = (int32_t)((uint32_t)s * (uint32_t)g0) >> 12;
        int32_t u1 = (int32_t)((uint32_t)s * (uint32_t)g1) >> 12;
        int32_t a2 = (int32_t)((uint32_t)u0 * (uint32_t)g3) >> 12;
        int32_t t0 = (int32_t)((uint32_t)u1 * (uint32_t)g3) >> 12;
        int32_t l = ((int32_t)((uint32_t)u0 * (uint32_t)g2) >> 12) + sl[i];
        int32_t r = ((int32_t)((uint32_t)u1 * (uint32_t)g2) >> 12) + sr[i];

        if (l < -32768) {
            l = -32768;
        }
        if (l > 32767) {
            l = 32767;
        }
        if (r < -32768) {
            r = -32768;
        }
        if (r > 32767) {
            r = 32767;
        }
        sl[i] = (int32_t)((uint32_t)a2 * (uint32_t)-3567 + (uint32_t)t0 * (uint32_t)-2006) >> 12;
        sr[i] = (int32_t)((uint32_t)t0 * (uint32_t)3567 + (uint32_t)a2 * (uint32_t)2006) >> 12;
        outl[i] = (int16_t)l;
        outr[i] = (int16_t)r;
    }
}

/* 0x004eac58: func_004eacb8 under the PL2 lock. */
int32_t func_004eac58(void *pl2, const int16_t *in, int16_t *outl, int16_t *outr)
{
    func_004ea710();
    func_004eacb8(pl2, in, outl, outr);
    return func_004ea728();
}

/* 0x004eb158: run the detach hook of a previously attached encoder, if any. */
void func_004eb158(void *adxt)
{
    uint32_t hook = M32(0x00724e80);

    if (hook) {
        ((void (*)(void *))p4_fn_g2h_sig(hook, P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(adxt);
    }
}

/* 0x004eb020: attach a Pro Logic II encoder (work area a1/a2) to a stereo-capable ADXT: stop it, detach
 * any earlier encoder, create the encoder with the ADXB's saved parameters (+0xec, +0xf0) and store it
 * at ADXB +0xe8, and install the detach, encode, reset and set-sfreq hooks. (A failed create returns
 * with the ADXM lock still held, as retail does.) */
void func_004eb020(uint8_t *adxt, void *work, int32_t size)
{
    uint8_t *adxb;
    void *pl2;

    if (S8(adxt, 3) < 2) {
        func_004c6a98((const char *)P4_G2H(0x0075c5a8u));
        return;
    }
    func_004d4308(adxt);
    func_004eb158(adxt);
    func_004ea740(P4_FN_H2G(func_004eb300), 0);
    func_004ea7c8(0);
    adxb = (uint8_t *)P4_G2H(P32((void *)P4_G2H(P32(adxt, 4)), 4));
    func_004cc9d0();
    pl2 = func_004ea880(work, size);
    if (pl2 == 0) {
        func_004c6a98((const char *)P4_G2H(0x0075c5e8u));
        return;
    }
    func_004eae58(pl2, (int32_t)P32(adxb, 0xec), (int32_t)P32(adxb, 0xf0));
    P32(adxb, 0xe8) = P4_H2G(pl2);
    M32(0x00724e80) = P4_FN_H2G(PL2LINK_DetachDolbyProLogicII);
    M32(0x00714c2c) = P4_FN_H2G(PL2LINK_Encode);
    M32(0x00714c30) = P4_FN_H2G(func_004eb298);
    M32(0x007212e4) = P4_FN_H2G(func_004eb2e8);
    func_004cc9e8();
}

/* 0x004eb180: change the parameters of the encoder attached to an ADXT. */
void func_004eb180(uint8_t *adxt, int32_t a, int32_t b)
{
    uint8_t *adxb;

    if (adxt == 0) {
        func_004c6a98((const char *)P4_G2H(0x0075c620u));
        return;
    }
    adxb = (uint8_t *)P4_G2H(P32((void *)P4_G2H(P32(adxt, 4)), 4));
    if (S8(adxt, 3) < 2 || P32(adxb, 0xe8) == 0) {
        func_004c6a98((const char *)P4_G2H(0x0075c650u));
        return;
    }
    func_004eae58((void *)P4_G2H(P32(adxb, 0xe8)), a, b);
    P32(adxb, 0xf0) = (uint32_t)b;
    P32(adxb, 0xec) = (uint32_t)a;
}

/* 0x004d3c08: ADXT create variant with the encoder: the first 1024 bytes of the work area are the
 * encoder's, the rest is a stereo ADXT's (adxt_Create(2, ...), 0x004d38a8). */
void *func_004d3c08(void *work, int32_t size)
{
    void *adxt = adxt_Create(2, (uint8_t *)work + 1024, size - 1024);

    if (adxt) {
        func_004eb020(adxt, work, 1024);
    }
    return adxt;
}
