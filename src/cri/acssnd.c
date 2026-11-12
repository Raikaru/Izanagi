/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI ACSSND (the ACS sound object) creation, translated from the retail asm (0x004be4a8 and
 * 0x004bfba0), the part sdkSndInit reaches through ACSSVR_CreatSnd.
 *
 * ACSSND object, offsets as the retail code uses them: +0x00 the ACS voice-object pointers (one per
 * buffer, two at most), +0x08 the current buffer's pointer (copied from +0x18 after init), +0x10
 * the current index, +0x18 and +0x24 twelve-byte per-buffer slots whose first word is the voice
 * object, +0x20 and +0x2c the per-buffer fader (0x44 bytes), +0x30 clear (32 bytes), +0x34 and +0x70
 * the control block (0x100 bytes), +0x50 initialised, +0x54 init in progress, +0x5c double-buffer
 * flag, +0x6c the extra 0x3000-byte-per-buffer area, +0x78 a 40-byte copy of the parameter block,
 * +0xa0 buffer count (1 or 2), +0xa4 byte, +0xa8 30, +0xb8 60, +0xc0 the per-slot table of
 * parameter-count words.
 *
 * Parameter block (ten words, ACSSVR_CalcWorkCprm's layout): [0] count, [1] channels, [2] rate,
 * [3] n3, [4] n4, [6] double buffer, [7] mode A, [8] extra-buffer flag, [9] mode B.
 */
#include <stdint.h>

#define W32(p, off) (*(int32_t *)((uint8_t *)(p) + (off)))
#define U32(p, off) (*(uint32_t *)((uint8_t *)(p) + (off)))
#define B8(p, off) (*(int8_t *)((uint8_t *)(p) + (off)))

extern void func_0043f9c8(void *dst, int32_t c, uint32_t n);          /* memset */
extern int32_t func_004bd6a0(uint32_t msg);                            /* ACS error report */
extern void *func_004bd6b8(void *work, uint32_t size);                 /* fader init */
extern void *func_004bdda8(void *work, uint32_t size);                 /* voice object, mono (12 bytes) */
extern void *func_004be0e8(void *work, uint32_t size);                 /* voice object, stereo (16 bytes) */
extern void func_004be9c0(void *snd);                                  /* ACSSND finish */
extern void func_004bd658(void);                                       /* ACS lock */
extern void func_004bd670(void);                                       /* ACS unlock */
extern int32_t func_004bd748(uint8_t *f);                              /* fader state */
extern int32_t func_004bd758(uint8_t *f);                              /* fader level (dB*10) */
extern void func_004bd760(uint8_t *f, int32_t v);                      /* set fader level */
extern void func_004bd790(uint8_t *f, int32_t to, int32_t steps, int32_t a3, int32_t a4);
extern void func_004bd850(uint8_t *f);                                 /* reverse a fade */
extern void func_004bd9d0(uint8_t *f, int32_t curve);                  /* fader curve */
extern void func_004bd9d8(uint8_t *f, uint32_t fn, uint32_t obj, uint32_t ev); /* fade start hook (code and argument words) */
extern void func_004bd9e8(uint8_t *f, uint32_t fn, uint32_t obj, uint32_t ev); /* fade end hook */
extern int32_t func_004bf930(uint32_t snd);                               /* +0xa4 byte */
extern void func_004c01c0(uint32_t snd, int32_t v);                       /* +0xa4 byte = v */
extern int32_t func_004d43f8(void *adxt);                               /* ADXT status of a voice's handle */
extern void *func_004deec0(void *aixp, int32_t i);                     /* AIXP -> its ADXT */
extern void func_004db250(void *adxt, void *work, int32_t size);       /* AHX attach */
extern void func_004db398(void *adxt);                                 /* AHX detach */
extern int32_t func_004be3a8(void *voice, int32_t track, int32_t vol); /* voice track volume */

/* The voice object's first vtable slot: init(obj, cfg, work, size), jalr'd through *(*obj). */
static int32_t voice_init(void *voice, void *cfg, void *work, int32_t size)
{
    uint32_t fn = ((uint32_t *)P4_G2H(*(uint32_t *)voice))[0];

    return ((int32_t (*)(void *, void *, void *, int32_t))p4_fn_g2h_sig(fn, P4_SIG(4, 0, 1, 1, 1, 0, 0, 0)))(voice, cfg, work, size);
}

/* 0x004bfba0: reset the object: no voices, no faders, the retail defaults (+0xa8 = 30, +0xb8 = 60). */
void func_004bfba0(uint8_t *snd)
{
    int32_t i;

    W32(snd, 8) = 0;
    W32(snd, 12) = 0;
    W32(snd, 16) = 0;
    W32(snd, 20) = 0;
    W32(snd, 0) = 0;
    W32(snd, 4) = 0;
    for (i = 0; i < 2; i++) {
        func_0043f9c8(snd + 24 + i * 12, 0, 12);
    }
    func_0043f9c8(snd + 48, 0, 32);
    W32(snd, 80) = 0;
    W32(snd, 84) = 0;
    W32(snd, 88) = 0;
    W32(snd, 92) = 0;
    W32(snd, 96) = 0;
    W32(snd, 100) = 0;
    W32(snd, 104) = 0;
    W32(snd, 108) = 0;
    W32(snd, 112) = 0;
    W32(snd, 116) = 0;
    func_0043f9c8(snd + 120, 0, 40);
    W32(snd, 160) = 0;
    W32(snd, 168) = 30;
    W32(snd, 184) = 60;
    W32(snd, 204) = 0;
    snd[164] = 0;
    W32(snd, 172) = 0;
    W32(snd, 176) = 0;
    W32(snd, 180) = 0;
    W32(snd, 188) = 0;
    W32(snd, 192) = 0;
    W32(snd, 196) = 0;
    W32(snd, 200) = 0;
}

/* Bytes of PCM ring one voice needs, before the per-channel part: ((rate / 1000) * 25000 / 44) *
 * (count + 1) rounded up to a 0x800 multiple, as the retail 32-bit arithmetic does (the signed
 * round-up of x + 0x800 is x + 0xfff when that is negative, the compiler's >> 11 fix-up). */
static int32_t ring_bytes(const int32_t *p, int32_t channels)
{
    int32_t span = ((channels * (p[2] / 1000) * 25000) / 0x2c) * (p[0] + 1);
    int32_t t = span + 0x800;

    if (t < 0) {
        t = span + 0xfff;
    }
    return (t >> 11) * 0x800;
}

/* 0x004be4a8: set the object up inside `work` (size bytes): per buffer a voice object, its fader and
 * the ring sizes from the parameter block; then the control block. 1 on success; 0 with the error
 * text reported (0x756be8 already initialised, 0x756c18/0x756c88 no voice object, 0x756c50/0x756cc0
 * the voice object's init failed, 0x756cf8 no fader, 0x756d38 the work area is too small), and the
 * partial setup torn down with func_004be9c0. */
int32_t func_004be4a8(uint8_t *snd, const int32_t *p, uint8_t *work, int32_t size)
{
    int32_t cfg[3];
    int32_t off = 0;
    int32_t i;
    int32_t n;
    uint8_t *cur;
    const char *msg;

    if (W32(snd, 80) == 1 || W32(snd, 84) == 1) {
        func_004bd6a0(0x00756be8u);
        return 0;
    }
    func_004bfba0(snd);
    W32(snd, 84) = 1;
    __builtin_memcpy(snd + 120, p, 40);
    func_0043f9c8(cfg, 0, 12);
    cfg[0] = p[1];
    cfg[1] = (p[7] == 1) ? p[3] : 1;
    cfg[2] = p[9];
    if (p[6] == 1) {
        W32(snd, 92) = 1;
        W32(snd, 160) = 2;
    } else {
        W32(snd, 160) = 1;
        W32(snd, 92) = 0;
    }
    n = W32(snd, 160);
    for (i = 0; i < n; i++) {
        void *voice;
        int32_t vsize;
        void *fdr;

        if (p[7] != 0) {
            voice = func_004be0e8(work + off, 16);
            U32(snd, i * 4) = P4_H2G(voice);
            if (voice == 0) {
                msg = (const char *)P4_G2H(0x00756c18u);
                goto fail;
            }
            off += 16;
            if (p[9] != 0) {
                vsize = (ring_bytes(p, 1) + 0x85a4) * p[3];
            } else {
                vsize = (ring_bytes(p, p[1]) + p[1] * 0x40c0 + 0x24) * p[3];
            }
            vsize += 0x40;
            if (voice_init(voice, cfg, work + off, vsize) == 0) {
                msg = (const char *)P4_G2H(0x00756c50u);
                goto fail;
            }
        } else {
            voice = func_004bdda8(work + off, 12);
            U32(snd, i * 4) = P4_H2G(voice);
            if (voice == 0) {
                msg = (const char *)P4_G2H(0x00756c88u);
                goto fail;
            }
            off += 12;
            if (p[9] != 0) {
                vsize = (ring_bytes(p, 1) + 0x24) * p[4] + 0x85c0;
            } else {
                vsize = (ring_bytes(p, p[1]) + 0x24) * p[4] + p[1] * 0x40c0 + 0x40;
            }
            if (voice_init(voice, cfg, work + off, vsize) == 0) {
                msg = (const char *)P4_G2H(0x00756cc0u);
                goto fail;
            }
        }
        U32(snd, 24 + i * 12) = U32(snd, i * 4);
        fdr = func_004bd6b8(work + off + vsize, 68);
        U32(snd, 32 + i * 12) = P4_H2G(fdr);
        if (fdr == 0) {
            msg = (const char *)P4_G2H(0x00756cf8u);
            goto fail;
        }
        off += vsize + 68;
    }
    W32(snd, 16) = 0;
    U32(snd, 8) = U32(snd, 24);
    if (p[8] == 1) {
        W32(snd, 108) = P4_H2G(work + off);
        off += n * 0x3000;
    }
    cur = work + off;
    W32(snd, 52) = P4_H2G(cur);
    off += 0x100;
    W32(snd, 112) = P4_H2G(cur);
    if (p[7] == 1) {
        W32(snd, 192) = P4_H2G(work + off);
        func_0043f9c8(work + off, 0, p[3] * 4);
        off += p[3] * 4;
    }
    if (size >= off) {
        W32(snd, 80) = 1;
        W32(snd, 84) = 0;
        return 1;
    }
    msg = (const char *)P4_G2H(0x00756d38u);
fail:
    func_004bd6a0(P4_H2G(msg));
    func_004be9c0(snd);
    return 0;
}

/* ---- ACSSND API ------------------------------------------------------------------------------
 * More of the object (see the top of the file): +0x08 the current voice, +0x0c the previous one, +0x10
 * and +0x14 the current and previous buffer index; slot i (12 bytes at +0x18 + 12 * i): +0 voice, +4 fade
 * state byte (0 idle, 1 fading out, 2 fading in, 3 fading to a level), +5 byte the fade hooks pass on,
 * +8 the fader; +0x5c double buffer, +0x60 locked out, +0x64 switch pending, +0x84 track count, +0x94
 * voice kind (1 = AIXP, else ADXT), +0xa0 buffers, +0xa4 byte, +0xa8/+0xac/+0xb0/+0xb4 fade ticks,
 * +0xb8 fade start delay, +0xbc output volume, +0xc0 per-track volumes, +0xc4/+0xc8 highlight offset and
 * flag, +0xcc user callback(snd, event), +0x30 the start parameters.
 * Voice objects have a vtable at +0: +8 start A(p1), +0xc start B(p2, p3), +0x10 start C(p4, p5), +0x14
 * start D(p6, p7), +0x18 pause, +0x1c stop(flag), +0x24 status, +0x28 volume, +0x30 track volume, +0x38
 * loop flag, +0x3c the ADXT/AIXP handle.
 */
void func_004c01c8(uint8_t *snd, uint8_t *slot);

#define SLOT(snd, i) ((uint8_t *)(snd) + 0x18 + (i) * 12)
#define VFN(obj, off) p4_fn_g2h_sig(U32((uint8_t *)P4_G2H(U32(obj, 0)), off), P4_SIG(4, 0, 1, 0, 0, 0, 0, 0))

static int32_t vcall(void *obj, int32_t off, int32_t a, int32_t b, int32_t c)
{
    return ((int32_t (*)(void *, int32_t, int32_t, int32_t))VFN(obj, off))(obj, a, b, c);
}

/* 0x004bdfa0: the voice's play state from its ADXT status: 0 stopped, 1 for 1..2, 2 for 3..4, 3 for 5, 4
 * for 6 and anything above. */
int32_t func_004bdfa0(uint8_t *obj)
{
    uint32_t st = (uint32_t)func_004d43f8(P4_G2H(U32(obj, 4)));

    switch (st) {
    case 0:
        return 0;
    case 1:
    case 2:
        return 1;
    case 3:
    case 4:
        return 2;
    case 5:
        return 3;
    default:
        return 4;
    }
}

/* 0x004be318: the volume of every track of a voice object (+0xc tracks), through 0x004be3a8. */
void func_004be318(uint8_t *obj, int32_t vol)
{
    int32_t i;

    for (i = 0; i < W32(obj, 0xc); i++) {
        func_004be3a8(obj, i, vol);
    }
}

/* 0x004c0658: fade hook: calls the user callback (+0xcc) with the object and the event. */
void func_004c0658(uint8_t *snd, int32_t ev)
{
    void (*cb)(void *, int32_t);

    if (snd != 0) {
        cb = (void (*)(void *, int32_t))p4_fn_g2h_sig(U32(snd, 0xcc), P4_SIG(2, 0, 1, 0, 0, 0, 0, 0));
        if (cb != 0) {
            cb(snd, ev);
        }
    }
}

/* 0x004c0068: start the slot's fade-in: level -960, then up to +0xbc over +0xac ticks (delay a2, end
 * delay +0xb8), hooks 1 (start) and 4 (end); state 2, a3 in byte +5. */
void func_004c0068(uint8_t *snd, uint8_t *slot, int32_t delay, int32_t a3)
{
    uint8_t *f = (uint8_t *)P4_G2H(U32(slot, 8));

    func_004bd760(f, -960);
    func_004bd790(f, W32(snd, 0xbc), W32(snd, 0xac), delay, W32(snd, 0xb8));
    func_004bd9d8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 1);
    func_004bd9e8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 4);
    slot[5] = (uint8_t)a3;
    slot[4] = 2;
}

/* 0x004c0118: start the slot's fade-out: from the current level to -960 over +0xa8 ticks (delay a2, end
 * delay +0xb8), hooks 0 and 3; state 1, a3 in byte +5. */
void func_004c0118(uint8_t *snd, uint8_t *slot, int32_t delay, int32_t a3)
{
    uint8_t *f = (uint8_t *)P4_G2H(U32(slot, 8));

    func_004bd790(f, -960, W32(snd, 0xa8), delay, W32(snd, 0xb8));
    func_004bd9d8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 0);
    func_004bd9e8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 3);
    slot[5] = (uint8_t)a3;
    slot[4] = 1;
}

/* 0x004bff98: start the voice with the parameter block p: kind 0 -> start A(p[1]), 1 -> B(p[2], p[3]), 2
 * -> C(p[4], p[5]), 3 -> D(p[6], p[7]). */
void func_004bff98(uint8_t *snd, const int32_t *p)
{
    void *voice = (void *)P4_G2H(U32(snd, 8));

    switch (p[0]) {
    case 0:
        vcall(voice, 8, p[1], 0, 0);
        break;
    case 1:
        vcall(voice, 0xc, p[2], p[3], 0);
        break;
    case 2:
        vcall(voice, 0x10, p[4], p[5], 0);
        break;
    case 3:
        vcall(voice, 0x14, p[6], p[7], 0);
        break;
    default:
        break;
    }
}

/* 0x004c0308: advance to the next buffer: the current index becomes the previous one, wraps at +0xa0,
 * +8/+0xc follow. With the lock-out clear and a switch pending (+0x64) the old voice is stopped first. */
void func_004c0308(uint8_t *snd)
{
    int32_t cur;

    func_004bd658();
    if (W32(snd, 0x60) == 0 && W32(snd, 0x64) == 1) {
        vcall((void *)P4_G2H(U32(snd, 8)), 0x1c, 0, 0, 0);
    }
    cur = W32(snd, 0x10);
    W32(snd, 0x14) = cur;
    W32(snd, 0x10) = cur + 1;
    if (W32(snd, 0xa0) <= cur + 1) {
        W32(snd, 0x10) = 0;
    }
    U32(snd, 0xc) = U32(SLOT(snd, W32(snd, 0x14)), 0);
    U32(snd, 8) = U32(SLOT(snd, W32(snd, 0x10)), 0);
    func_004bd670();
}

/* 0x004c03b8: step back to the previous buffer. */
void func_004c03b8(uint8_t *snd)
{
    func_004bd658();
    W32(snd, 0x10) = W32(snd, 0x10) - 1;
    if (W32(snd, 0x10) < 0) {
        W32(snd, 0x10) = W32(snd, 0xa0) - 1;
    }
    W32(snd, 0x14) = W32(snd, 0x10) - 1;
    if (W32(snd, 0x14) < 0) {
        W32(snd, 0x14) = W32(snd, 0xa0) - 1;
    }
    U32(snd, 0xc) = U32(SLOT(snd, W32(snd, 0x14)), 0);
    U32(snd, 8) = U32(SLOT(snd, W32(snd, 0x10)), 0);
    func_004bd670();
}

/* 0x004c05f0: 1 when every buffer's fader is idle, else 0 (retail returns i == count, so a negative count
 * gives 0). */
int32_t func_004c05f0(uint8_t *snd)
{
    int32_t i = 0;

    while (i < W32(snd, 0xa0)) {
        if (func_004bd748((uint8_t *)P4_G2H(U32(SLOT(snd, i), 8))) != 0) {
            break;
        }
        i++;
    }
    return i == W32(snd, 0xa0);
}

/* 0x004c0518: push a slot's fader level to its voice: with AIXP voices every track gets the track volume
 * (+0xc0) plus the level and the highlight offset (+0xc4); otherwise the voice volume gets level +
 * offset. */
void func_004c0518(uint8_t *snd, uint8_t *slot)
{
    uint8_t *f = (uint8_t *)P4_G2H(U32(slot, 8));
    void *voice = (void *)P4_G2H(U32(slot, 0));
    int32_t i;

    if (W32(snd, 0x94) == 1) {
        for (i = 0; i < W32(snd, 0x84); i++) {
            int32_t lvl = func_004bd758(f);

            vcall(voice, 0x30, i, W32((uint8_t *)P4_G2H(U32(snd, 0xc0)), i * 4) + lvl + W32(snd, 0xc4), 0);
        }
    } else {
        vcall(voice, 0x28, func_004bd758(f) + W32(snd, 0xc4), 0, 0);
    }
}

/* 0x004c0450: after a frame: every idle fader whose slot has the byte +4 set gets that cleared and
 * func_004c01c8 run; once all faders are idle the user callback gets event 6 and +0xa4 clears. */
void func_004c0450(uint8_t *snd)
{
    int32_t i;

    for (i = 0; i < W32(snd, 0xa0); i++) {
        uint8_t *slot = SLOT(snd, i);

        if (func_004bd748((uint8_t *)P4_G2H(U32(slot, 8))) == 0 && slot[4] != 0) {
            slot[4] = 0;
            func_004c01c8(snd, slot);
        }
    }
    if (func_004c05f0(snd) != 1) {
        return;
    }
    if (U32(snd, 0xcc) != 0) {
        ((void (*)(void *, int32_t))p4_fn_g2h_sig(U32(snd, 0xcc), P4_SIG(2, 0, 1, 0, 0, 0, 0, 0)))(snd, 6);
    }
    func_004c01c0(P4_H2G(snd), 0);
}

/* 0x004c01c8: what a slot's fade leaves to do (byte +5): 1 restarts playback: lock, the fade-in delay is
 * +0xb4 - +0xa8 (0 when not positive), +0xa4 set, the voice stopped with flag 1, switch pending, the
 * fade-in started on the slot, unlock, the slot's level pushed and the saved start parameters (+0x30)
 * replayed; 2 pauses the voice, 3 stops it with flag 1. */
void func_004c01c8(uint8_t *snd, uint8_t *slot)
{
    void *voice = (void *)P4_G2H(U32(slot, 0));
    int32_t what = (int8_t)slot[5];
    int32_t delay;

    if (what != 1) {
        if (what == 2) {
            vcall(voice, 0x18, 0, 0, 0);
        } else if (what == 3) {
            vcall(voice, 0x1c, 1, 0, 0);
        }
        return;
    }
    func_004bd658();
    delay = W32(snd, 0xb4) - W32(snd, 0xa8);
    if (W32(snd, 0xb4) <= W32(snd, 0xa8)) {
        delay = 0;
    }
    func_004c01c0(P4_H2G(snd), 1);
    vcall(voice, 0x1c, 1, 0, 0);
    W32(snd, 0x64) = 1;
    func_004c0068(snd, slot, delay, 0);
    func_004bd670();
    func_004c0518(snd, slot);
    func_004bff98(snd, (const int32_t *)(snd + 0x30));
}

/* 0x004c12a0: the highlight block's state byte (+0x50). */
void func_004c12a0(uint8_t *blk, int32_t state)
{
    blk[0x50] = (uint8_t)state;
}

/* 0x004c12a8: ticks left to return the fader to full level from its current one: +0x54 - level * +0x54 /
 * -(+0x5c) (the retail divide-by-zero break when +0x5c is 0 is the host's division fault). */
int32_t func_004c12a8(uint8_t *blk)
{
    int32_t lvl = func_004bd758((uint8_t *)P4_G2H(U32(blk, 4)));

    return W32(blk, 0x54) - (lvl * W32(blk, 0x54)) / -W32(blk, 0x5c);
}

/* 0x004c12f8: the same with +0x58 as the multiplier. */
int32_t func_004c12f8(uint8_t *blk)
{
    int32_t lvl = func_004bd758((uint8_t *)P4_G2H(U32(blk, 4)));

    return W32(blk, 0x54) - (lvl * W32(blk, 0x58)) / -W32(blk, 0x5c);
}

/* 0x004bebf0: stop. Fades the current slot out (state 2 byte, no delay) and, with the lock-out set
 * (+0x60), stops the current voice (and the previous one when double-buffered) and mutes their faders. */
void ACSSND_Stop(uint8_t *snd)
{
    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00756e50u);
        return;
    }
    func_004bd658();
    func_004c01c0(P4_H2G(snd), 1);
    func_004c0118(snd, SLOT(snd, W32(snd, 0x10)), 0, 2);
    if (W32(snd, 0x60) == 1) {
        vcall((void *)P4_G2H(U32(SLOT(snd, W32(snd, 0x10)), 0)), 0x18, 0, 0, 0);
        func_004bd760((uint8_t *)P4_G2H(U32(SLOT(snd, W32(snd, 0x10)), 8)), -960);
        if (W32(snd, 0x5c) != 0) {
            vcall((void *)P4_G2H(U32(SLOT(snd, W32(snd, 0x14)), 0)), 0x18, 0, 0, 0);
            func_004bd760((uint8_t *)P4_G2H(U32(SLOT(snd, W32(snd, 0x14)), 8)), -960);
        }
    }
    func_004bd670();
}

/* 0x004beec8: set the output volume (+0xbc): unless the current slot is fading out (byte +4 == 1) it
 * fades to the new level over +0xb0 ticks (end delay +0xb8) with hooks 2 and 5, state 3, byte +5 = 0.
 * Error text 0x00756f48 when not initialised. */
void ACSSND_SetOutVol(uint8_t *snd, int32_t vol)
{
    uint8_t *slot;
    uint8_t *f;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00756f48u);
        return;
    }
    func_004bd658();
    W32(snd, 0xbc) = vol;
    slot = SLOT(snd, W32(snd, 0x10));
    if ((int8_t)slot[4] != 1) {
        func_004c01c0(P4_H2G(snd), 1);
        f = (uint8_t *)P4_G2H(U32(slot, 8));
        func_004bd790(f, vol, W32(snd, 0xb0), 0, W32(snd, 0xb8));
        func_004bd9d8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 2);
        func_004bd9e8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 5);
        slot[4] = 3;
        slot[5] = 0;
    }
    func_004bd670();
}

/* 0x004befd0: set a track's volume: remembered in the table at +0xc0; applied to every voice (as track
 * volume plus the output volume) when the object is not stream style, or is and locked out (+0x60 ==
 * 1). Needs the AIXP kind (+0x94), a track in range (+0x84) and a table. Error text 0x00756f78. */
void ACSSND_SetTrackVol(uint8_t *snd, int32_t track, int32_t vol)
{
    int32_t n;
    int32_t i;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00756f78u);
        return;
    }
    if (W32(snd, 0x94) == 0 || track < 0 || track >= W32(snd, 0x84)) {
        return;
    }
    if (U32(snd, 0xc0) == 0) {
        return;
    }
    W32((uint8_t *)P4_G2H(U32(snd, 0xc0)), track * 4) = vol;
    if (B8(snd, 0xa4) != 1) {
        n = W32(snd, 0xa0);
    } else {
        if (W32(snd, 0x60) != 1) {
            return;
        }
        n = W32(snd, 0xa0);
    }
    for (i = 0; i < n; i = i + 1) {
        vcall((void *)P4_G2H(U32(SLOT(snd, i), 0)), 0x30, track, W32(snd, 0xbc) + vol, 0);
        n = W32(snd, 0xa0);
    }
}

/* 0x004bf0e8: a track's remembered volume (0 without the AIXP kind, a table, or in range). */
int32_t func_004bf0e8(uint8_t *snd, int32_t track)
{
    if (W32(snd, 0x94) == 0) {
        return 0;
    }
    if (track < 0 || W32(snd, 0x84) <= track) {
        return 0;
    }
    if (U32(snd, 0xc0) == 0) {
        return 0;
    }
    return W32((uint8_t *)P4_G2H(U32(snd, 0xc0)), track * 4);
}

/* 0x004bf138: the loop flag on every voice. Error text 0x00756fb0. */
void ACSSND_SetLpFlg(uint8_t *snd, int32_t flg)
{
    int32_t i;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00756fb0u);
        return;
    }
    for (i = 0; i < W32(snd, 0xa0); i++) {
        vcall((void *)P4_G2H(U32(SLOT(snd, i), 0)), 0x38, flg, 0, 0);
    }
}

static void *snd_adxt(uint8_t *snd, int32_t i);

/* 0x004bf240: attach an AHX work area (+0x6c, 0x3000 bytes per buffer) to every buffer's ADXT. Error
 * texts: 0x00756fe0 not initialised, 0x00757010 no AHX area (+0x98 clear). */
void ACSSND_AttachAhx(uint8_t *snd)
{
    int32_t i;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00756fe0u);
        return;
    }
    if (W32(snd, 0x98) == 0) {
        func_004bd6a0(0x00757010u);
        return;
    }
    for (i = 0; i < W32(snd, 0xa0); i++) {
        void *adxt = snd_adxt(snd, i);

        func_004db250(adxt, (uint8_t *)P4_G2H(U32(snd, 0x6c)) + i * 0x3000, 0x3000);
    }
}

/* 0x004bf350: detach it again. Error texts 0x00757040 and 0x00757070. */
void ACSSND_DetachAhx(uint8_t *snd)
{
    int32_t i;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00757040u);
        return;
    }
    if (W32(snd, 0x98) == 0) {
        func_004bd6a0(0x00757070u);
        return;
    }
    for (i = 0; i < W32(snd, 0xa0); i++) {
        func_004db398(snd_adxt(snd, i));
    }
}

/* 0x004bf438, 0x004bf498, 0x004bf510, 0x004bf590: the ADXT handles of an ADX-kind object (+0x94 != 1): the
 * active buffer's, the next one's (wrapping at +0xa0), buffer i's (in range), and the count. 0 for the
 * other kind. Error texts 0x007570a0, 0x007570d8, 0x00757110, 0x00757140 when not initialised. The voices
 * are read from the pointer table at +0, as the retail code does, not from the slots. */
void *ACSSND_GetActiveAdxt(uint8_t *snd)
{
    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x007570a0u);
        return 0;
    }
    if (W32(snd, 0x94) == 1) {
        return 0;
    }
    return (void *)P4_G2H(vcall((void *)P4_G2H(U32(snd, 8)), 0x3c, 0, 0, 0));
}

void *ACSSND_GetNextAdxt(uint8_t *snd)
{
    int32_t i;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x007570d8u);
        return 0;
    }
    if (W32(snd, 0x94) == 1) {
        return 0;
    }
    i = W32(snd, 0x10) + 1;
    if (W32(snd, 0xa0) <= i) {
        i = 0;
    }
    return (void *)P4_G2H(vcall((void *)P4_G2H(U32(snd, i * 4)), 0x3c, 0, 0, 0));
}

void *ACSSND_GetAdxt(uint8_t *snd, int32_t i)
{
    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00757110u);
        return 0;
    }
    if (W32(snd, 0x94) == 1 || i < 0 || W32(snd, 0xa0) <= i) {
        return 0;
    }
    return (void *)P4_G2H(vcall((void *)P4_G2H(U32(snd, i * 4)), 0x3c, 0, 0, 0));
}

int32_t ACSSND_GetNumAdxt(uint8_t *snd)
{
    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00757140u);
        return 0;
    }
    return W32(snd, 0x94) == 1 ? 0 : W32(snd, 0xa0);
}

/* 0x004bf5e0, 0x004bf638, 0x004bf6a8, 0x004bf718: the same for the AIXP kind (+0x94 != 0). Error texts
 * 0x00757178, 0x007571b0, 0x007571e8, 0x00757218. */
void *ACSSND_GetActiveAixp(uint8_t *snd)
{
    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00757178u);
        return 0;
    }
    if (W32(snd, 0x94) == 0) {
        return 0;
    }
    return (void *)P4_G2H(vcall((void *)P4_G2H(U32(snd, 8)), 0x3c, 0, 0, 0));
}

void *ACSSND_GetNextAixp(uint8_t *snd)
{
    int32_t i;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x007571b0u);
        return 0;
    }
    if (W32(snd, 0x94) == 0) {
        return 0;
    }
    i = W32(snd, 0x10) + 1;
    if (W32(snd, 0xa0) <= i) {
        i = 0;
    }
    return (void *)P4_G2H(vcall((void *)P4_G2H(U32(snd, i * 4)), 0x3c, 0, 0, 0));
}

void *ACSSND_GetAixp(uint8_t *snd, int32_t i)
{
    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x007571e8u);
        return 0;
    }
    if (W32(snd, 0x94) == 0 || i < 0 || W32(snd, 0xa0) <= i) {
        return 0;
    }
    return (void *)P4_G2H(vcall((void *)P4_G2H(U32(snd, i * 4)), 0x3c, 0, 0, 0));
}

int32_t ACSSND_GetNumAixp(uint8_t *snd)
{
    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00757218u);
        return 0;
    }
    return W32(snd, 0x94) == 0 ? 0 : W32(snd, 0xa0);
}

/* The ADXT of buffer i for the AHX calls: an AIXP kind's first ADXT (func_004deec0 of its AIXP), else
 * the ADXT of the ADX voice. */
static void *snd_adxt(uint8_t *snd, int32_t i)
{
    if (W32(snd, 0x94) == 1) {
        return func_004deec0(ACSSND_GetAixp(snd, i), 0);
    }
    return ACSSND_GetAdxt(snd, i);
}

/* 0x004bf758: cancel the fading of the current slot: a fade-out (1) turns into a fade-in (2, hooks 1 and
 * 4, byte +5 = 0), a fade-in (2) into a fade-out (1, hooks 0 and 3, byte +5 = 2); the fader's direction
 * is reversed (func_004bd850). Double-buffered and back to fading out, the previous buffer becomes the
 * current one and gets the same treatment unless idle (when the buffers advance instead). Only while
 * +0xa4 is 1. Error text 0x00757250. */
void ACSSND_CancelFading(uint8_t *snd)
{
    uint8_t *slot;
    uint8_t *f;
    int32_t st;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00757250u);
        return;
    }
    if (func_004bf930(P4_H2G(snd)) != 1) {
        return;
    }
    func_004bd658();
    slot = SLOT(snd, W32(snd, 0x10));
    st = (int8_t)slot[4];
    f = (uint8_t *)P4_G2H(U32(slot, 8));
    if (st == 1) {
        slot[4] = 2;
        func_004bd9d8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 1);
        func_004bd9e8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 4);
        slot[5] = 0;
    } else if (st == 2) {
        slot[4] = 1;
        func_004bd9d8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 0);
        func_004bd9e8(f, p4_fn_h2g_sig(func_004c0658, "vpi"), P4_H2G(snd), 3);
        slot[5] = 2;
    }
    func_004bd850(f);
    if (W32(snd, 0x5c) != 0 && (int8_t)slot[4] == 1) {
        func_004c03b8(snd);
        if (SLOT(snd, W32(snd, 0x10))[4] != 0) {
            ACSSND_CancelFading(snd);
        } else {
            func_004c0308(snd);
        }
    }
    func_004bd670();
}

/* 0x004bfb28: the fade curve of every buffer's fader. */
void func_004bfb28(uint8_t *snd, int32_t curve)
{
    int32_t i;

    for (i = 0; i < W32(snd, 0xa0); i++) {
        func_004bd9d0((uint8_t *)P4_G2H(U32(SLOT(snd, i), 8)), curve);
    }
}

/* 0x004bfca0: start playing with the parameter block p (see func_004bff98). Error text 0x007572c0 when
 * not initialised. Under the lock, +0xa4 is set and the fade-in delay (+0xb4: a negative value delays the
 * previous buffer's fade-out instead). Double-buffered: advance to the next buffer, stop its voice (flag
 * 1), mark the switch pending, fade the new buffer in (no delay when the previous voice is idle (status
 * 0)), and fade the previous buffer out when it is still playing (muting and stopping it at once when
 * locked out). Single buffer: a voice that is not stopped (status 2) or a locked-out object is stopped,
 * and the buffer fades in; otherwise the buffer is faded out again. After the unlock a started buffer
 * gets its level pushed and the start parameters applied. */
void ACSSND_Start(uint8_t *snd, const int32_t *p)
{
    int32_t started = 0;
    int32_t delay_in;
    int32_t delay_out;
    int32_t v;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x007572c0u);
        return;
    }
    func_004bd658();
    func_004c01c0(P4_H2G(snd), 1);
    v = W32(snd, 0xb4);
    if (v >= 0) {
        delay_in = v;
        delay_out = 0;
    } else {
        delay_in = 0;
        delay_out = -v;
    }
    if (W32(snd, 0x5c) != 0) {
        func_004c0308(snd);
        vcall((void *)P4_G2H(U32(snd, 8)), 0x1c, 1, 0, 0);
        W32(snd, 0x64) = 1;
        if (vcall((void *)P4_G2H(U32(SLOT(snd, W32(snd, 0x14)), 0)), 0x24, 0, 0, 0) == 0) {
            func_004c0068(snd, SLOT(snd, W32(snd, 0x10)), 0, 0);
        } else {
            func_004c0068(snd, SLOT(snd, W32(snd, 0x10)), delay_in, 0);
        }
        if (vcall((void *)P4_G2H(U32(SLOT(snd, W32(snd, 0x14)), 0)), 0x24, 0, 0, 0) != 0) {
            func_004c0118(snd, SLOT(snd, W32(snd, 0x14)), delay_out, 2);
            if (W32(snd, 0x60) == 1) {
                vcall((void *)P4_G2H(U32(SLOT(snd, W32(snd, 0x14)), 0)), 0x18, 0, 0, 0);
                func_004bd760((uint8_t *)P4_G2H(U32(SLOT(snd, W32(snd, 0x14)), 8)), -960);
            }
        }
        started = 1;
    } else {
        int32_t st = vcall((void *)P4_G2H(U32(snd, 8)), 0x24, 0, 0, 0);

        if (st != 2 || W32(snd, 0x60) == 1) {
            vcall((void *)P4_G2H(U32(snd, 8)), 0x1c, 1, 0, 0);
            started = 1;
            W32(snd, 0x64) = 1;
            func_004c0068(snd, SLOT(snd, W32(snd, 0x10)), 0, 0);
        } else {
            func_004c0118(snd, SLOT(snd, W32(snd, 0x10)), 0, 1);
        }
    }
    func_004bd670();
    if (started == 1) {
        func_004c0518(snd, SLOT(snd, W32(snd, 0x10)));
        func_004bff98(snd, p);
    }
}
