/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI ACS (audio control server) glue and the ACS fader, translated from the retail asm and checked
 * against it function by function (0x004bd6a0..0x004c1030 range, boot path: sdkSndInit ->
 * ACSSVR_Init).
 *
 * ACSSVR object (0xb0 bytes, owned by the caller): +0x00 sixteen ACSSND handles, +0x40 pointer to the
 * fader at +0x44 (0x60 bytes), +0x4c voice-highlight flag, +0xa4 init count, +0xa8 init-in-progress,
 * +0xac the SVM main-server slot id.
 * ACSFDR fader (offsets in the object that func_004bd9f8 initialises): +0 state byte, +4 step,
 * +8 tick, +0xc, +0x10/+0x14 timing words, +0x18 curve byte, +0x1c current dB*10, +0x20 current
 * linear level, +0x24/+0x28 target and previous dB*10.
 *
 * Callees with a retail address are called by their retail names (func_XXXXXXXX); the generator
 * binds each to the native or to the trap stub at that address.
 */
#include "p4pc/ee_dlibm.h"
#include <stdint.h>

#define W32(p, off) (*(int32_t *)((uint8_t *)(p) + (off)))
#define U32(p, off) (*(uint32_t *)((uint8_t *)(p) + (off)))
#define B8(p, off) (*(int8_t *)((uint8_t *)(p) + (off)))

extern void func_0043f9c8(void *dst, int32_t c, uint32_t n);      /* memset */
extern int32_t func_004bd6a0(uint32_t msg);                        /* ACS error report (a tail call into the ADX one) */
extern void func_004ca720(void);                                   /* ADXF init */
extern void func_004ca8b0(void);                                   /* ADXF finish */
extern void func_004de340(void);                                   /* aixp init */
extern void func_004de3f0(void);                                   /* aixp finish */
extern int32_t func_004f0b88(int32_t svtype, int32_t id);          /* SVM_DelCbSvr */
extern int32_t func_004f0c78(int32_t svtype, int32_t id, void *func, void *obj, const char *name);
extern void *func_004be418(void *work, int32_t size);               /* ACSSND create: the object, a host pointer */
extern int32_t func_004be4a8(void *snd, const int32_t *prm, void *work, int32_t size);
extern void func_004be480(void *snd);
extern void func_004be9c0(void *snd);
extern int32_t func_004bea98(uint32_t snd);   /* decomp leaf: the object's guest address */
extern void func_004bf950(uint32_t snd);
extern int32_t func_004beec0(uint32_t snd);
extern void func_004bf938(uint32_t snd, int32_t v);
extern void func_004bf960(void *snd);                              /* ACSSND per-frame step */
extern void func_004c1038(void *fdr);                              /* highlight state machine */

/* ---- ACSFDR fader ------------------------------------------------------------------------- */

/* 0x004bd9f8: reset a fader object: all timing words 0, levels at the -96 dB floor (-960). */
void func_004bd9f8(uint8_t *f)
{
    W32(f, 0x28) = -960;
    f[0] = 0;
    W32(f, 4) = 0;
    W32(f, 8) = 0;
    W32(f, 0xc) = 0;
    W32(f, 0x10) = 0;
    W32(f, 0x14) = 0;
    f[0x18] = 0;
    W32(f, 0x1c) = -960;
    W32(f, 0x20) = 0;
    W32(f, 0x24) = -960;
    func_0043f9c8(f + 0x2c, 0, 12);
    func_0043f9c8(f + 0x38, 0, 12);
}

/* 0x004bd6b8: initialise a fader in `work` (at least 0x44 bytes). 0 and the error text when work is
 * NULL (0x7568c8) or too small (0x756900). */
uint8_t *func_004bd6b8(uint8_t *work, uint32_t size)
{
    const char *msg;

    if (work == 0) {
        msg = (const char *)P4_G2H(0x007568c8u);
    } else {
        if (size > 0x43) {
            func_004bd9f8(work);
            return work;
        }
        msg = (const char *)P4_G2H(0x00756900u);
    }
    func_004bd6a0(P4_H2G(msg));
    return 0;
}

/* 0x004bd720: re-initialise a fader. */
void func_004bd720(uint8_t *f)
{
    if (f != 0) {
        func_004bd9f8(f);
    }
}

/* 0x004bd748, 0x004bd750, 0x004bd758: state byte, tick count, current dB*10. */
int32_t func_004bd748(uint8_t *f)
{
    return (int8_t)f[0];
}

int32_t func_004bd758(uint8_t *f)
{
    return W32(f, 0x1c);
}

/* 0x004bdc20: linear level (0..0x316938) of a level in dB*10: 0 is full scale, -960 is silence.
 * Curve 1: 0x316938 * log(-v) / log(960); any other curve: 0x316938 * 10^(v/200). Single-precision
 * steps, as retail computes them with its soft-float double helpers. */
int32_t func_004bdc20(uint8_t *f, int32_t v)
{
    float lvl;

    if (v == 0) {
        return 0x316938;
    }
    if (v == -960) {
        return 0;
    }
    if (B8(f, 0x18) == 1) {
        float den = (float)p4pc_ee_log10(960.0);
        float num = (float)p4pc_ee_log10((double)(-v));

        lvl = (float)0x316938 * num / den;
    } else {
        lvl = (float)p4pc_ee_pow(10.0, (double)((float)v / 200.0f));
        lvl = lvl * 3238200.0f;
    }
    if (!(0.0f < lvl)) {
        lvl = 0.0f;
    }
    if (!(lvl < 3238200.0f)) {
        lvl = 3238200.0f;
    }
    return (int32_t)lvl;
}

/* 0x004bdaf0: dB*10 (0 full scale, -960 silence) of a linear level 0..0x316938, the inverse of
 * func_004bdc20: curve 0 is 200 * log10(level / 0x316938), curve 1 is -(10 ^ (log10(960) * (0x316938 -
 * level) / 0x316938)); both clamped to -960..0. Single-precision steps with the double log and pow of
 * the retail soft-float helpers. */
int32_t func_004bdaf0(uint8_t *f, int32_t level)
{
    float r;

    if (level == 0) {
        return -960;
    }
    if (level == 0x316938) {
        return 0;
    }
    if (B8(f, 0x18) == 1) {
        float l960 = (float)p4pc_ee_log10(960.0);
        float u = l960 * (float)(0x316938 - level) / 3238200.0f;

        r = -(float)p4pc_ee_pow(10.0, (double)u);
    } else {
        float x = (float)level / 3238200.0f;

        r = (float)(p4pc_ee_log10((double)x) * 200.0);
    }
    if (!(-960.0f < r)) {
        r = -960.0f;
    }
    if (!(r < 0.0f)) {
        r = 0.0f;
    }
    return (int32_t)r;
}

/* 0x004bdd48 and 0x004bdd78: call the fade-start hook (function +0x2c, arguments +0x30 and +0x34) and
 * the fade-end hook (function +0x38, arguments +0x3c and +0x40). 1 when the hook is set. */
int32_t func_004bdd48(uint8_t *f)
{
    void (*cb)(uint32_t, uint32_t) = (void (*)(uint32_t, uint32_t))p4_fn_g2h_sig(U32(f, 0x2c), P4_SIG(2, 0, 0, 0, 0, 0, 0, 0));

    if (cb == 0) {
        return 0;
    }
    cb(U32(f, 0x30), U32(f, 0x34));
    return 1;
}

int32_t func_004bdd78(uint8_t *f)
{
    void (*cb)(uint32_t, uint32_t) = (void (*)(uint32_t, uint32_t))p4_fn_g2h_sig(U32(f, 0x38), P4_SIG(2, 0, 0, 0, 0, 0, 0, 0));

    if (cb == 0) {
        return 0;
    }
    cb(U32(f, 0x3c), U32(f, 0x40));
    return 1;
}

/* 0x004bd760: set the current level and its linear value. */
void func_004bd760(uint8_t *f, int32_t v)
{
    W32(f, 0x1c) = v;
    W32(f, 0x20) = func_004bdc20(f, v);
}

/* 0x004bda68: linear step per tick from level `from` to level `to` over `steps` ticks; the full
 * scale value when steps <= 0. */
int32_t func_004bda68(uint8_t *f, int32_t from, int32_t to, int32_t steps)
{
    if (steps > 0) {
        int32_t a = func_004bdc20(f, to);
        int32_t b = func_004bdc20(f, from);

        return (a - b) / steps;
    }
    return 0x316938;
}

/* 0x004bd790: start a fade to level `to` over `steps` ticks (a4 is kept at +0x14, a3 at +0x10). With
 * no ticks the fade completes at once (state 2, level set by func_004bd760). */
void func_004bd790(uint8_t *f, int32_t to, int32_t steps, int32_t a3, int32_t a4)
{
    int32_t prev = W32(f, 0x24);
    int32_t cur = W32(f, 0x1c);

    W32(f, 0x24) = to;
    W32(f, 0x28) = prev;
    W32(f, 4) = func_004bda68(f, cur, to, steps);
    W32(f, 0x10) = a3;
    W32(f, 0x14) = a4;
    W32(f, 8) = 0;
    W32(f, 0xc) = 0;
    if (steps > 0) {
        f[0] = 1;
        return;
    }
    f[0] = 2;
    func_004bd760(f, to);
}

/* 0x004bd850: reverse the fade: swap target and previous level, negate the step; state 3 becomes 2. */
void func_004bd850(uint8_t *f)
{
    int32_t cur_target = W32(f, 0x24);
    int32_t prev = W32(f, 0x28);

    W32(f, 4) = -W32(f, 4);
    W32(f, 0x24) = prev;
    W32(f, 0x28) = cur_target;
    if (f[0] == 3) {
        f[0] = 2;
    }
}

/* ---- ACSSVR ------------------------------------------------------------------------------ */

/* 0x004c0e40: the server is initialised (init count > 0). */
int32_t func_004c0e40(uint8_t *svr)
{
    return 0 < W32(svr, 0xa4);
}

/* 0x004c0e08: clear the handle table and the counters. */
void func_004c0e08(uint8_t *svr)
{
    int32_t i;

    for (i = 15; i >= 0; i--) {
        U32(svr, i * 4) = 0;
    }
    W32(svr, 0xac) = 0;
    W32(svr, 0xa4) = 0;
    W32(svr, 0xa8) = 0;
}

/* 0x004c1248: initialise the 0x60-byte fader block (head pointers, 0x44-byte work area, fade
 * limits 30/60/120). */
void func_004c1248(uint8_t *blk)
{
    U32(blk, 0) = 0;
    U32(blk, 4) = 0;
    func_0043f9c8(blk + 8, 0, 0x44);
    U32(blk, 0x4c) = 0;
    U32(blk, 0x54) = 0x1e;
    U32(blk, 0x5c) = 0x78;
    U32(blk, 0x58) = 0x3c;
    blk[0x50] = 0;
}

/* 0x004c0e78: set up the fader block at `blk` (size must be at least 0x60). */
uint8_t *func_004c0e78(uint8_t *blk, uint32_t size)
{
    const char *msg;

    if (blk == 0) {
        msg = (const char *)P4_G2H(0x00757498u);
    } else {
        if (size > 0x5f) {
            func_004c1248(blk);
            return blk;
        }
        msg = (const char *)P4_G2H(0x007574d0u);
    }
    func_004bd6a0(P4_H2G(msg));
    return 0;
}

/* 0x004c0ee0: clear a fader block. */
void func_004c0ee0(uint8_t *blk)
{
    if (blk != 0) {
        func_004c1248(blk);
    }
}

/* 0x004c0f08: create the fader object in the block (at +8, 0x44 bytes) and start it at level 0 with
 * no fade. 1 on success. */
int32_t func_004c0f08(uint8_t *blk)
{
    const char *msg;

    if (U32(blk, 4) != 0) {
        msg = (const char *)P4_G2H(0x00757508u);
    } else {
        uint8_t *fdr = func_004bd6b8(blk + 8, 0x44);

        U32(blk, 4) = P4_H2G(fdr);
        msg = (const char *)P4_G2H(0x00757538u);
        if (fdr != 0) {
            func_004bd790(fdr, 0, 0, 0, 0);
            return 1;
        }
    }
    func_004bd6a0(P4_H2G(msg));
    return 0;
}

/* 0x004c0f88: release the fader object of a block. */
void func_004c0f88(uint8_t *blk)
{
    if (U32(blk, 4) != 0) {
        func_004bd720((uint8_t *)P4_G2H(U32(blk, 4)));
        U32(blk, 4) = 0;
    }
}

/* 0x004c1030: store the owner pointer at the head of the block. */
void func_004c1030(uint32_t *blk, uint32_t owner)
{
    blk[0] = owner;
}

/* 0x004c0e00: the fader block of a server. */
uint32_t func_004c0e00(uint8_t *svr)
{
    return U32(svr, 0x40);
}

/* 0x004c0fc0, 0x004c0fd0, 0x004c0fd8, 0x004c0fe0, 0x004c0fe8: voice-highlight switch in the block
 * (+0x4c), its mode byte (+0x50) and the fader level of the block's fader. */
void ACSVHL_EnableVoiceHighlight(uint8_t *blk)
{
    W32(blk, 0x4c) = 1;
}

void func_004c0fd0(uint8_t *blk)
{
    W32(blk, 0x4c) = 0;
}

int32_t func_004c0fd8(uint8_t *blk)
{
    return W32(blk, 0x4c);
}

int32_t func_004c0fe0(uint8_t *blk)
{
    return B8(blk, 0x50);
}

int32_t func_004c0fe8(uint8_t *blk)
{
    return func_004bd758((uint8_t *)P4_G2H(U32(blk, 4)));
}

/* 0x004c0a40 ACSSVR_DestroySnd(svr, snd): destroy a sound handle and clear its slot. */
void ACSSVR_DestroySnd(uint8_t *svr, uint8_t *snd)
{
    int32_t i;

    if (snd == 0) {
        return;
    }
    func_004be9c0(snd);
    for (i = 0; i < 16; i++) {
        if ((uint8_t *)P4_G2H(U32(svr, i * 4)) == snd) {
            func_004be480(snd);
            U32(svr, i * 4) = 0;
            return;
        }
    }
}

/* 0x004c07f0 ACSSVR_Finish: drop one init level; on the last (or when init was in progress) destroy
 * every sound, release the fader block, and remove the SVM server entries. A negative count
 * reports the error text at 0x7573b0. */
void ACSSVR_Finish(uint8_t *svr)
{
    int32_t prog = W32(svr, 0xa8);
    int32_t cnt;
    int32_t i;

    if (prog == 0) {
        W32(svr, 0xa4) = W32(svr, 0xa4) - 1;
    }
    cnt = W32(svr, 0xa4);
    if (cnt < 0) {
        func_004bd6a0(0x007573b0u);
        return;
    }
    if (cnt != 0 && prog != 1) {
        return;
    }
    for (i = 0; i < 16; i++) {
        ACSSVR_DestroySnd(svr, (uint8_t *)P4_G2H(U32(svr, i * 4)));
    }
    if (U32(svr, 0x40) != 0) {
        func_004c0f88((uint8_t *)P4_G2H(U32(svr, 0x40)));
        func_004c0ee0((uint8_t *)P4_G2H(U32(svr, 0x40)));
        U32(svr, 0x40) = 0;
    }
    func_004f0b88(2, 4);
    func_004f0b88(5, W32(svr, 0xac));
    func_004de3f0();
    func_004ca8b0();
    W32(svr, 0xa8) = 0;
}

extern uint32_t func_004c0e50(uint8_t *svr);

/* 0x004c0718 ACSSVR_Init: first init sets up the ADXF and aixp libraries, registers the server's
 * ExecAll as SVM vsync-type callback 4 and creates the fader block (0x60 bytes at +0x44). Later
 * inits only count. 1 on success, 0 when the fader cannot be created (the server is finished again). */
uint32_t ACSSVR_Init(uint8_t *svr)
{
    int32_t cnt;

    if (func_004c0e40(svr)) {
        cnt = W32(svr, 0xa4);
    } else if (W32(svr, 0xa8) != 0) {
        cnt = W32(svr, 0xa4);
    } else {
        uint8_t *blk;

        func_004c0e08(svr);
        W32(svr, 0xa8) = 1;
        func_004ca720();
        func_004de340();
        func_004f0c78(2, 4, (void *)func_004c0e50, svr, (const char *)P4_G2H(0x00757368u));
        blk = func_004c0e78(svr + 0x44, 0x60);
        U32(svr, 0x40) = P4_H2G(blk);
        if (!func_004c0f08(blk)) {
            func_004bd6a0(0x00757378u);
            ACSSVR_Finish(svr);
            return 0;
        }
        func_004c1030((uint32_t *)P4_G2H(U32(svr, 0x40)), P4_H2G(svr));
        W32(svr, 0xa8) = 0;
        cnt = W32(svr, 0xa4);
    }
    W32(svr, 0xa4) = cnt + 1;
    return 1;
}

/* 0x004c0928 ACSSVR_CreatSnd(svr, a1, work, size): take the first free handle slot, create the sound
 * in `work` (0xd0 bytes of it) and set it up with the rest. 0 and the error text on failure. */
uint32_t ACSSVR_CreatSnd(uint8_t *svr, int32_t a1, uint8_t *work, int32_t size)
{
    uint32_t snd = 0;
    int32_t i;
    const char *msg;

    if (!func_004c0e40(svr)) {
        func_004bd6a0(0x007573e8u);
        return 0;
    }
    for (i = 0; i < 16; i++) {
        if (U32(svr, i * 4) == 0) {
            snd = P4_H2G(func_004be418(work, size));
            work += 0xd0;
            size -= 0xd0;
            U32(svr, i * 4) = snd;
            break;
        }
    }
    if (i == 16) {
        msg = (const char *)P4_G2H(0x00757418u);
    } else {
        if (func_004be4a8((void *)P4_G2H(snd), (const int32_t *)P4_G2H(a1), work, size) != 0) {
            return snd;
        }
        msg = (const char *)P4_G2H(0x00757458u);
    }
    func_004bd6a0(P4_H2G(msg));
    ACSSVR_DestroySnd(svr, (uint8_t *)P4_G2H(snd));
    return 0;
}

/* 0x004c0d60 ACSSVR_ExecHndl(svr, snd): one server step of one sound: update it, and while the
 * server's voice highlight is on and the sound is in state 1, push the fader level into it. */
void ACSSVR_ExecHndl(uint8_t *svr, uint8_t *snd)
{
    uint8_t *fdr;

    if (!func_004c0e40(svr)) {
        return;
    }
    if (func_004bea98(P4_H2G(snd)) == 0) {
        return;
    }
    func_004bf950(P4_H2G(snd));
    fdr = (uint8_t *)P4_G2H(U32(svr, 0x40));
    if (func_004c0fd8(fdr) != 0 && func_004c0fe0(fdr) != 0 && func_004beec0(P4_H2G(snd)) == 1) {
        func_004bf938(P4_H2G(snd), func_004c0fe8(fdr));
    }
    func_004bf960(snd);
}

/* 0x004c0ce0: the server's ExecAll: run the highlight fade (when on), then every live handle. */
void func_004c0ce0(uint8_t *svr)
{
    int32_t i;

    if (!func_004c0e40(svr)) {
        return;
    }
    if (func_004c0fd8((uint8_t *)P4_G2H(U32(svr, 0x40))) != 0) {
        func_004c1038((uint8_t *)P4_G2H(U32(svr, 0x40)));
    }
    for (i = 0; i < 16; i++) {
        uint32_t snd = U32(svr, i * 4);

        if (snd != 0) {
            ACSSVR_ExecHndl(svr, (uint8_t *)P4_G2H(snd));
        }
    }
}

/* 0x004c0e50: SVM vsync-type server callback: ExecAll over the server object; 1 when it ran. */
uint32_t func_004c0e50(uint8_t *svr)
{
    uint32_t ran = 0;

    if (svr != 0) {
        func_004c0ce0(svr);
        ran = 1;
    }
    return ran;
}
