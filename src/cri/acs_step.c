/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI ACS per-frame steps, translated from the retail asm: the fader tick (0x004bd890), the
 * voice-highlight state machine (0x004c1038) and one frame of an ACSSND (0x004bf960). The ACSSND
 * layout is documented in acssnd.c, the fader's and the highlight block's in acs.c.
 */
#include <stdint.h>

#define W32(p, off) (*(int32_t *)((uint8_t *)(p) + (off)))
#define U32(p, off) (*(uint32_t *)((uint8_t *)(p) + (off)))
#define B8(p, off) (*(int8_t *)((uint8_t *)(p) + (off)))

extern int32_t func_004bd6a0(uint32_t msg);                        /* ACS error report */
extern void func_004bd658(void);                                   /* ACS lock */
extern void func_004bd670(void);                                   /* ACS unlock */
extern int32_t func_004bd748(uint8_t *f);                          /* fader state */
extern void func_004bd760(uint8_t *f, int32_t v);                  /* set fader level */
extern void func_004bd790(uint8_t *f, int32_t to, int32_t steps, int32_t a3, int32_t a4);
extern int32_t func_004bdd48(void *f);                             /* fade start hook */
extern int32_t func_004bdd78(void *f);                             /* fade end hook */
extern int32_t func_004bdaf0(void *f, int32_t level);              /* dB*10 of a linear level */
extern int32_t func_004bea98(uint32_t snd);   /* decomp leaf: the object's guest address */
extern int32_t func_004beec0(uint32_t snd);
extern int32_t func_004bee38(void *snd);                           /* ACSSND play phase */
extern int32_t func_004bedf0(void *snd);
extern void func_004c0450(void *snd);
extern void func_004c0518(void *snd, void *slot);
extern void func_004c12a0(void *blk, int32_t state);
extern int32_t func_004c12a8(void *blk);
extern int32_t func_004c12f8(void *blk);

void func_004bd890(uint8_t *f);

/* 0x004bd890: one tick of a fader. Idle (0) does nothing; the tick count (+8) advances and nothing
 * else happens until it reaches the start delay (+0x10). State 1 then becomes 2 (func_004bdd48
 * announces the start); state 2 steps the linear level (+0x20, 0..0x316938) by +4, converts it back to
 * dB*10 (func_004bdaf0 into +0x1c), and once that has reached the target (+0x24, from either side
 * by the step's sign, or at once with a zero step) sets the target through func_004bd760, moves to
 * state 3 and notes the tick (+0xc); state 3 returns to idle (func_004bdd78) after +0x14 ticks. */
void func_004bd890(uint8_t *f)
{
    int32_t state = func_004bd748(f);
    int32_t tick;

    if (state == 0) {
        return;
    }
    tick = W32(f, 8) + 1;
    W32(f, 8) = tick;
    if (tick < W32(f, 0x10)) {
        return;
    }
    if (state == 1) {
        state = 2;
        f[0] = 2;
        func_004bdd48(f);
    }
    if (state == 2) {
        int32_t v = W32(f, 0x20) + W32(f, 4);
        int32_t lvl;
        int32_t step;
        int32_t target;

        if (v < 0) {
            v = 0;
        }
        if (v > 0x316938) {
            v = 0x316938;
        }
        W32(f, 0x20) = v;
        lvl = func_004bdaf0(f, W32(f, 0x20));
        W32(f, 0x1c) = lvl;
        step = W32(f, 4);
        target = W32(f, 0x24);
        if (step == 0 || (step > 0 && !(lvl < target)) || (step < 0 && !(target < lvl))) {
            func_004bd760(f, target);
            state = 3;
            f[0] = 3;
            W32(f, 0xc) = W32(f, 8);
        }
    }
    if (state == 3 && W32(f, 8) - W32(f, 0xc) >= W32(f, 0x14)) {
        f[0] = 0;
        func_004bdd78(f);
    }
}

/* 0x004c1038: the voice-highlight state machine, once per frame. `blk` is the highlight block: +0
 * the handle table, +4 the fader, +0x50 the state, +0x54/+0x58/+0x5c the fade lengths and level.
 * A highlight is wanted while some playing handle (phase 1 or 2 of func_004bee38) reports
 * func_004bedf0 == 0. State 0 (idle) starts a fade down by +0x5c over +0x54 ticks when one is; 1
 * (fading down) starts the fade back up over func_004c12a8 when none is, else waits for the fader
 * to stop; 2 (held) fades up over +0x58 when none is; 3 (fading up) restarts the fade down over
 * func_004c12f8 when one is again, else returns to idle when the fader has stopped. */
void func_004c1038(uint8_t *blk)
{
    int32_t i;
    int32_t want = 0;
    int32_t state;
    uint8_t *fdr;

    for (i = 0; i < 16; i++) {
        uint8_t *h = (uint8_t *)P4_G2H(U32((uint8_t *)P4_G2H(U32(blk, 0)), i * 4));

        if (h != 0 && func_004bea98(P4_H2G(h)) != 0 && func_004beec0(P4_H2G(h)) == 2) {
            uint32_t phase = (uint32_t)func_004bee38(h);

            if (phase == 1 || phase == 2) {
                if (func_004bedf0(h) == 0) {
                    want = 1;
                }
            }
        }
    }
    fdr = (uint8_t *)P4_G2H(U32(blk, 4));
    state = B8(blk, 0x50);
    if (state == 1) {
        if (!want) {
            int32_t len = func_004c12a8(blk);

            func_004bd790(fdr, 0, len, 0, 0);
            func_004c12a0(blk, 3);
        } else if (func_004bd748(fdr) == 0) {
            func_004c12a0(blk, 2);
        }
    } else if (state == 0) {
        if (want) {
            func_004bd790(fdr, -W32(blk, 0x5c), W32(blk, 0x54), 0, 0);
            func_004c12a0(blk, 1);
        }
    } else if (state == 2) {
        if (!want) {
            func_004bd790(fdr, 0, W32(blk, 0x58), 0, 0);
            func_004c12a0(blk, 3);
        }
    } else if (state == 3) {
        if (want) {
            int32_t len = func_004c12f8(blk);

            func_004bd790(fdr, -W32(blk, 0x5c), len, 0, 0);
            func_004c12a0(blk, 1);
        } else if (func_004bd748(fdr) == 0) {
            func_004c12a0(blk, 0);
        }
    }
    func_004bd890(fdr);
}

/* A voice object's vtable slots the per-frame step uses: +0x24 status (2 = stopped), +0x1c stop. */
static int32_t snd_vcall0(void *obj, int32_t off)
{
    uint32_t fn = U32((uint8_t *)P4_G2H(U32(obj, 0)), off);

    return ((int32_t (*)(void *))p4_fn_g2h_sig(fn, P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(obj);
}

static void snd_vcall1(void *obj, int32_t off, int32_t a)
{
    uint32_t fn = U32((uint8_t *)P4_G2H(U32(obj, 0)), off);

    ((void (*)(void *, int32_t))p4_fn_g2h_sig(fn, P4_SIG(2, 0, 1, 0, 0, 0, 0, 0)))(obj, a);
}

/* 0x004bf960: one frame of an ACSSND (see func_004be4a8 for the layout). Error text 0x757288 when it
 * is not initialised. Under the ACS lock: nothing while +0x60 is set. With +0xa4 clear (no
 * stream-style object) and +0xc8 == 1, every buffer's slot gets func_004c0518. With +0xa4 == 1: a
 * pending switch (+0x64) completes once the current buffer's fader has stopped and the voice object
 * reports status 2 (it is then stopped and the switch flag cleared), after which every buffer slot
 * that is in use (byte at +0x1c) gets its fader ticked and func_004c0518, and func_004c0450 runs. */
void func_004bf960(uint8_t *snd)
{
    int32_t n;
    int32_t i;

    if (W32(snd, 0x50) == 0) {
        func_004bd6a0(0x00757288u);
        return;
    }
    func_004bd658();
    if (W32(snd, 0x60) == 1) {
        func_004bd670();
        return;
    }
    if (B8(snd, 0xa4) != 1) {
        if (W32(snd, 200) == 1) {
            for (i = 0; i < W32(snd, 0xa0); i++) {
                func_004c0518(snd, snd + 0x18 + i * 12);
            }
        }
    } else {
        if (W32(snd, 0x64) != 1 || func_004bd748((uint8_t *)P4_G2H(U32(snd, W32(snd, 0x10) * 12 + 0x20))) == 1) {
            n = W32(snd, 0xa0);
        } else {
            void *obj = (void *)P4_G2H(U32(snd, 8));

            if (snd_vcall0(obj, 0x24) != 2) {
                func_004bd670();
                return;
            }
            snd_vcall1(obj, 0x1c, 0);
            W32(snd, 0x64) = 0;
            n = W32(snd, 0xa0);
        }
        for (i = 0; i < n; i++) {
            if (B8(snd, 0x1c + i * 12) != 0) {
                func_004bd890((void *)P4_G2H(U32(snd, 0x20 + i * 12)));
                func_004c0518(snd, snd + 0x18 + i * 12);
                n = W32(snd, 0xa0);
            }
        }
        func_004c0450(snd);
    }
    func_004bd670();
}
