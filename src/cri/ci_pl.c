/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI libadxe PL2 encoder wrappers and the "lsc" sound-volume helpers, translated from the retail asm 0x004EA838..0x004ED240.
 *
 * The PL2ENC entry points are lock wrappers: func_004ea710 takes the lock, func_004ea728 releases it (its result is the wrapper's result). The lsc helpers
 * test the sound server state (0x0073A748 == 1 means a server, 0x0073A740 its channel count). Every function follows the retail asm call for call. */
#include <stdint.h>

#define P4_GW(a) (*(volatile uint32_t *)P4_G2H(a))
#define P4_STR(a) ((const char *)P4_G2H(a))

extern void func_004ea710(void);
extern int32_t func_004ea728(void);
extern uint32_t func_004ea868(void);
extern void func_004ea9f8(void *a0);
extern void func_004eaa48(uint8_t *a0);
extern void func_004eaae8(uint8_t *a0, int32_t a1, int16_t *a2, int16_t *a3);
extern void func_004eaea8(uint8_t *a0, int32_t a1, int32_t a2);
extern void func_004eaf98(uint8_t *a0, int32_t a1);
extern void func_004ea798(const char *msg);
extern void *func_0043f9c8(void *d, int32_t c, uint32_t n);
extern int32_t func_004e58a0(uint32_t a0, uint32_t *a1, int32_t n1, uint32_t *a3, int32_t n2);
extern void func_004cd3f0(const char *fmt, ...);

/* 0x004ea838: func_004ea868(a0) under the PL2 lock; returns the unlock result. */
int32_t func_004ea838(void *a0)
{
    func_004ea710();
    func_004ea868();
    return func_004ea728();
}

/* 0x004ea9c8: func_004ea9f8(a0) under the PL2 lock; returns the unlock result. */
int32_t func_004ea9c8(void *a0)
{
    func_004ea710();
    func_004ea9f8(a0);
    return func_004ea728();
}

/* 0x004eaa18: func_004eaa48(a0) under the PL2 lock; returns the unlock result. */
int32_t func_004eaa18(void *a0)
{
    func_004ea710();
    func_004eaa48(a0);
    return func_004ea728();
}

/* 0x004eaa88 (PL2ENC_Encode): func_004eaae8(a0, (short)a1, a2, a3) under the PL2 lock; returns the unlock result. */
int32_t PL2ENC_Encode(void *a0, int32_t a1, void *a2, void *a3)
{
    int32_t s = (int16_t)a1;

    func_004ea710();
    func_004eaae8(a0, s, a2, a3);
    return func_004ea728();
}

/* 0x004eae58: func_004eaea8(a0, a1, a2) under the PL2 lock; returns the unlock result. */
int32_t func_004eae58(void *a0, int32_t a1, int32_t a2)
{
    func_004ea710();
    func_004eaea8(a0, a1, a2);
    return func_004ea728();
}

/* 0x004eaf58: func_004eaf98(a0, a1) under the PL2 lock; returns the unlock result. */
int32_t func_004eaf58(void *a0, int32_t a1)
{
    func_004ea710();
    func_004eaf98(a0, a1);
    return func_004ea728();
}

/* 0x004ec068: unless the sound server is up with no channel, sends the one time init request func_004e58a0(0xE, 0, 0, 0, 0) and marks it done (0x0073A770 = 1). */
void func_004ec068(void)
{
    if (P4_GW(0x0073A748) == 1 && (int32_t)P4_GW(0x0073A740) <= 0) {
        return;
    }
    if (P4_GW(0x0073A770) != 1) {
        func_004e58a0(0xE, 0, 0, 0, 0);
        P4_GW(0x0073A770) = 1;
    }
}

/* 0x004ec140: stores the byte a1 at a0 + 0x54 unless it is already there, the server is up without channel, or (the init request not sent yet) a1 is 1. */
void func_004ec140(uint8_t *a0, int32_t a1)
{
    int32_t cur;

    if (P4_GW(0x0073A748) == 1 && (int32_t)P4_GW(0x0073A740) <= 0) {
        return;
    }
    cur = (int8_t)a0[0x54];
    if (cur == a1) {
        return;
    }
    if (P4_GW(0x0073A770) != 0) {
        a0[0x54] = (uint8_t)a1;
    } else if (a1 != 1) {
        a0[0x54] = (uint8_t)a1;
    }
}

/* 0x004ecb40: with a server channel, (0x4000 - method(object)) / 2 where object = *(*(a0 + 8) + 8) and method the vtable slot +0x24 called as method(object, 0); else 0. */
int32_t func_004ecb40(uint8_t *a0)
{
    uint8_t *o;
    int32_t r;
    int32_t v;

    if (P4_GW(0x0073A748) == 1 && (int32_t)P4_GW(0x0073A740) <= 0) {
        return 0;
    }
    o = (uint8_t *)P4_G2H(*(uint32_t *)((uint8_t *)P4_G2H(*(uint32_t *)(a0 + 8)) + 8));
    r = ((int32_t (*)(uint8_t *, int32_t))p4_fn_g2h_sig(*(uint32_t *)((uint8_t *)P4_G2H(*(uint32_t *)o) + 0x24), P4_SIG(2, 0, 1, 0, 0, 0, 0, 0)))(o, 0);
    v = 0x4000 - r;
    return v / 2;
}

/* 0x004ecba8: with a server channel, method(object, 0) / 2 (object and method as in func_004ecb40); else 0. */
int32_t func_004ecba8(uint8_t *a0)
{
    uint8_t *o;
    int32_t r;

    if (P4_GW(0x0073A748) == 1 && (int32_t)P4_GW(0x0073A740) <= 0) {
        return 0;
    }
    o = (uint8_t *)P4_G2H(*(uint32_t *)((uint8_t *)P4_G2H(*(uint32_t *)(a0 + 8)) + 8));
    r = ((int32_t (*)(uint8_t *, int32_t))p4_fn_g2h_sig(*(uint32_t *)((uint8_t *)P4_G2H(*(uint32_t *)o) + 0x24), P4_SIG(2, 0, 1, 0, 0, 0, 0, 0)))(o, 0);
    return r / 2;
}

/* 0x004ed180: 0x10. */
int32_t func_004ed180(void)
{
    return 0x10;
}

/* 0x004ed240: prints 0x0075CB90 (func_004cd3f0) unless the sound server is up without channel; gives 0. */
int32_t func_004ed240(void)
{
    if (P4_GW(0x0073A748) == 1 && (int32_t)P4_GW(0x0073A740) <= 0) {
        return 0;
    }
    func_004cd3f0(P4_STR(0x0075CB90));
    return 0;
}

/* ---- PL2ENC helpers (0x004EA868..0x004EAF98) ------------------------------------------------------------------------------------------------- */

/* 0x004ea868: decrements the word at 0x00739788 and returns it (the PL2ENC use count). */
uint32_t func_004ea868(void)
{
    uint32_t value = P4_GW(0x00739788);

    value--;
    P4_GW(0x00739788) = value;
    return value;
}

/* 0x004ea9f8: clears 0x2C bytes at a0. */
void func_004ea9f8(void *a0)
{
    func_0043f9c8(a0, 0, 0x2C);
}

/* 0x004eaa48 (pl2enc_Reset): clears the two 0x60 word buffers (pointers at +8 and +0xC), +0x14 = 0 and +0x18 = 0x40. */
void func_004eaa48(uint8_t *a0)
{
    int32_t *p = (int32_t *)P4_G2H(*(uint32_t *)(a0 + 8));
    int32_t *q = (int32_t *)P4_G2H(*(uint32_t *)(a0 + 0xC));
    int32_t i;

    for (i = 0; i < 0x60; i++) {
        p[i] = 0;
        q[i] = 0;
    }
    *(int32_t *)(a0 + 0x14) = 0;
    *(int32_t *)(a0 + 0x18) = 0x40;
}

/* 0x004eaae8: one PL2 matrixing step: the sample a1 (16 bit) is scaled by the gain table words (int32 at 0x0075C080) selected by +0x1C, +0x20 and then by +0x28 and +0x24
 * (12 bit fixed point), mixed with the two delay lines (pointers +8 and +0xC, slot +0x14: new line words {-0xDEF * r1 - 0x7D6 * r2, 0xDEF * r2 + 0x7D6 * r1} >> 12) and the
 * two outputs (stored through a2 / a3 as clamped 16 bit) are the scaled values plus the old line words. The slot advances modulo +0x18. */
void func_004eaae8(uint8_t *a0, int32_t a1, int16_t *a2, int16_t *a3)
{
    const int32_t *tbl = (const int32_t *)P4_G2H(0x0075C080);
    int32_t x = (int16_t)a1;
    int32_t A = tbl[*(int32_t *)(a0 + 0x1C)];
    int32_t B = tbl[*(int32_t *)(a0 + 0x20)];
    int32_t C = tbl[*(int32_t *)(a0 + 0x28)];
    int32_t D = tbl[*(int32_t *)(a0 + 0x24)];
    int32_t *b1 = (int32_t *)P4_G2H(*(uint32_t *)(a0 + 8));
    int32_t *b2 = (int32_t *)P4_G2H(*(uint32_t *)(a0 + 0xC));
    int32_t i = *(int32_t *)(a0 + 0x14);
    int32_t p = (int32_t)((uint32_t)x * (uint32_t)A) >> 12;
    int32_t q = (int32_t)((uint32_t)x * (uint32_t)B) >> 12;
    int32_t r2 = (int32_t)((uint32_t)q * (uint32_t)C) >> 12;
    int32_t r1 = (int32_t)((uint32_t)p * (uint32_t)C) >> 12;
    int32_t n1 = (int32_t)((uint32_t)r1 * (uint32_t)-0xDEF + (uint32_t)r2 * (uint32_t)-0x7D6) >> 12;
    int32_t n2 = (int32_t)((uint32_t)r2 * 0xDEFu + (uint32_t)r1 * 0x7D6u) >> 12;
    int32_t old1 = b1[i];
    int32_t old2 = b2[i];
    int32_t L;
    int32_t R;

    L = ((int32_t)((uint32_t)p * (uint32_t)D) >> 12) + old1;
    R = ((int32_t)((uint32_t)q * (uint32_t)D) >> 12) + old2;
    b1[i] = n1;
    b2[i] = n2;
    if (R < -0x8000) {
        R = -0x8000;
    }
    if (L < -0x8000) {
        L = -0x8000;
    }
    if (0x7FFF < L) {
        L = 0x7FFF;
    }
    if (0x7FFF < R) {
        R = 0x7FFF;
    }
    i = i + 1;
    *a2 = (int16_t)L;
    *(int32_t *)(a0 + 0x14) = i;
    *a3 = (int16_t)R;
    if (!(i < *(int32_t *)(a0 + 0x18))) {
        *(int32_t *)(a0 + 0x14) = 0;
    }
}

/* 0x004eaea8: sets the pan: the object a0 and a1 / a2 in -0x7F..0x7F (anything else prints 0x0075C520 through func_004ea798, a null object 0x0075C4F8) store
 * |a1 + 0x7F| at +0x1C, |a1 - 0x7F| at +0x20, |a2 + 0x7F| at +0x24 and |a2 - 0x7F| at +0x28. */
void func_004eaea8(uint8_t *a0, int32_t a1, int32_t a2)
{
    int32_t t;
    int32_t u;

    if (a0 == 0) {
        func_004ea798(P4_STR(0x0075C4F8));
        return;
    }
    if (!((uint32_t)(a1 + 0x7F) < 0xFF) || a2 < -0x7F || !(a2 < 0x80)) {
        func_004ea798(P4_STR(0x0075C520));
        return;
    }
    t = a1 + 0x7F;
    u = a1 - 0x7F;
    *(int32_t *)(a0 + 0x1C) = (t < 0) ? -t : t;
    if (u < 0) {
        u = 0x7F - a1;
    }
    *(int32_t *)(a0 + 0x20) = u;
    t = a2 + 0x7F;
    u = a2 - 0x7F;
    if (t < 0) {
        t = -t;
    }
    *(int32_t *)(a0 + 0x24) = t;
    if (u < 0) {
        u = 0x7F - a2;
    }
    *(int32_t *)(a0 + 0x28) = u;
}

/* 0x004eaf98: sets the block size: a null object prints 0x0075C550 and a non positive size 0x0075C578 (func_004ea798); otherwise +0x10 = a1 and +0x18 = a1 / 0x2EE clamped to 1..0x60. */
void func_004eaf98(uint8_t *a0, int32_t a1)
{
    int32_t q;

    if (a0 == 0) {
        func_004ea798(P4_STR(0x0075C550));
        return;
    }
    if (!(a1 > 0)) {
        func_004ea798(P4_STR(0x0075C578));
        return;
    }
    *(int32_t *)(a0 + 0x10) = a1;
    q = a1 / 0x2EE;
    if (q <= 0) {
        q = 1;
    }
    if (!(q < 0x61)) {
        q = 0x60;
    }
    *(int32_t *)(a0 + 0x18) = q;
}
