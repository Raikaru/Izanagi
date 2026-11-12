/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI init-path functions translated from the retail asm and checked against it: the ADXPS2 setup
 * that registers the CVFS devices, the ADX library teardown wrapper, the AIX player/library init
 * (the per-vsync server callbacks and their handle loops), the debug printf, and small
 * configuration setters. Boot path: kwlnInitSubsystems -> sdkSndInit / func_004d32f8.
 *
 * Retail data is read and written at its retail address; calls to retail functions use the
 * func_XXXXXXXX names the generator binds to the native (config/native_names.txt) or to a trap.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define M32(a) (*(volatile int32_t *)P4_G2H(a))
#define M8(a) (*(volatile uint8_t *)P4_G2H(a))
#define W32(p, off) (*(volatile int32_t *)((uint8_t *)(p) + (off)))   /* a word of an object by host pointer */

extern void func_004c6a98(const char *msg);                          /* ADX error report */
extern void func_004e2e28(void *func, void *obj);                    /* cvFsEntryErrFunc */
extern void func_004e17a0(const char *name, void *getif, int32_t a); /* cvFsAddDev */
extern void func_004e1ad8(const char *name);                         /* cvFsSetDefDev */
extern int32_t func_004f1278(int32_t *flag);                         /* SVM test-and-set */
extern void func_004f0638(void);                                     /* SVM lock */
extern void func_004f06b0(void);                                     /* SVM unlock */
extern void func_004e9fa8(void *obj);                                /* mfCi object step */
extern int32_t func_004e39b0(int32_t req);                           /* dvCi request status */
extern void func_004e3770(int32_t req);                              /* dvCi request release */
void *func_004e5a08(void);                                           /* dvCiGetInterface */
void *func_004e9e50(void);                                           /* mfCiGetInterface */
extern void func_0043f9c8(void *dst, int32_t c, uint32_t n);         /* memset */
extern int32_t func_004f0c78(int32_t svtype, int32_t id, void *func, void *obj, const char *name);
extern int32_t func_004f0a60(int32_t svtype, void *func, void *obj, const char *name);
extern int32_t func_004f0b88(int32_t svtype, int32_t id);
extern void func_004f0608(void);                                     /* SVM_Lock */
extern void func_004f0620(void);                                     /* SVM_Unlock */
extern void func_004ca720(void);                                     /* ADXT_Init */
extern void ADXT_Finish(void);
extern int32_t adxt_init_cnt;
extern int32_t func_00446ed8(char *buf, const char *fmt, va_list ap); /* vsprintf */
extern void func_004244c8(const char *text);                         /* printf */
extern uint32_t func_00442948(const char *s);                        /* strlen */

/* 0x004d32e0: CVFS device error callback: forwards the message (second argument) to the ADX error
 * path. */
void func_004d32e0(void *obj, const char *msg)
{
    func_004c6a98(msg);
}

/* 0x004e7280: dvCi read mode: byte 0, 1 and 2 of the mode record at 0x00733dc0 and the word at
 * 0x00733dc4. */
void func_004e7280(int32_t a0, int32_t a1, int32_t a2, int32_t a3)
{
    M8(0x00733dc0) = (uint8_t)a0;
    M8(0x00733dc1) = (uint8_t)a1;
    M8(0x00733dc2) = (uint8_t)a2;
    M32(0x00733dc4) = a3;
}

/* 0x004e72a0: dvCi root directory (record at 0x00733dd0). NULL clears it. A path that does not
 * start with / or \ gets the two-byte prefix at 0x0075b250 first. A trailing / or \ is cut. */
void func_004e72a0(const char *path)
{
    char *root = (char *)P4_G2H(0x00733dd0);

    if (path == 0) {
        root[0] = 0;
        return;
    }
    if (path[0] != '/' && path[0] != '\\') {
        root[0] = M8(0x0075b250);
        root[1] = M8(0x0075b251);
    } else {
        root[0] = 0;
    }
    strcat(root, path);
    if (path[func_00442948(path) - 1] == '/' || path[func_00442948(path) - 1] == '\\') {
        root[func_00442948(root) - 1] = 0;
    }
}

/* 0x004d32f8 ADXPS2 setup(prm): register the memory-file device MFS and the disc device CDV with
 * the CVFS, make CDV the default, and pass the read mode and root directory from prm
 * (+8, +0xc, +0x10, +4 for the mode, +0 for the root) or zeros. The two memcpy(x, x, n) calls on
 * 0x00724e28 and 0x00724e38 that retail starts with copy a block onto itself and are left out. */
void func_004d32f8(int32_t *prm)
{
    func_004e2e28((void *)func_004d32e0, 0);
    func_004e17a0((const char *)P4_G2H(0x00758b10u), (void *)func_004e9e50, 0);
    func_004e2e28((void *)func_004d32e0, 0);
    func_004e17a0((const char *)P4_G2H(0x00758b18u), (void *)func_004e5a08, 0);
    func_004e1ad8((const char *)P4_G2H(0x00758b18u));
    if (prm != 0) {
        func_004e7280(prm[2], prm[3], prm[4], prm[1]);
        func_004e72a0((const char *)P4_G2H(prm[0]));
        return;
    }
    func_004e7280(0, 0, 0, 0);
    func_004e72a0(0);
}

/* 0x004d8830: clear the 0x3c0-byte table at 0x00724ed0. */
void func_004d8830(void)
{
    func_0043f9c8((void *)P4_G2H(0x00724ed0), 0, 0x3c0);
}

/* 0x004ca8b0: the ADX library finish: the last release also brackets the teardown with the
 * "not busy" word at 0x0073249c (0 while tearing down, 1 after). ADXT_Finish is the counted
 * finish itself (decomp re4/adx_inis.c, same module order as retail). */
void func_004ca8b0(void)
{
    if (adxt_init_cnt == 1) {
        M32(0x0073249c) = 0;
        ADXT_Finish();
        M32(0x0073249c) = 1;
    } else {
        ADXT_Finish();
    }
}

/* 0x004cd3f0: ADX debug printf: formats with vsprintf into a 128-byte buffer and prints it, unless
 * the debug-off word at 0x007212d0 is 2. */
void func_004cd3f0(const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    if (M32(0x007212d0) != 2) {
        va_start(ap, fmt);
        func_00446ed8(buf, fmt, ap);
        va_end(ap);
        func_004244c8(buf);
    }
}

/* 0x004eb318: ps2rna voice configuration (nch0, nch1): stores them, the total voice count and the
 * buffer size (0x4100 bytes per voice) at 0x0073a740.. and arms the configured flag. Negative
 * counts, or more than 32 voices (nch0 * 2 + nch1), report through the ADX printf and change
 * nothing. */
void criConfigureVoiceCounts(int32_t nch0, int32_t nch1)
{
    int32_t total;

    if (nch0 < 0) {
        func_004cd3f0((const char *)P4_G2H(0x0075c6b8u), nch0);
        return;
    }
    if (nch1 < 0) {
        func_004cd3f0((const char *)P4_G2H(0x0075c6e8u));
        return;
    }
    total = nch0 * 2 + nch1;
    if (total > 32) {
        func_004cd3f0((const char *)P4_G2H(0x0075c718u), 32);
        return;
    }
    M32(0x0073a768) = nch0;
    M32(0x0073a76c) = nch1;
    M32(0x0073a744) = total * 0x4100;
    M32(0x0073a748) = 1;
    M32(0x0073a740) = total;
}


/* ---- AIX player / AIX library --------------------------------------------------------------- */

extern void func_004df800(void *h);                                  /* aixp handle server */
extern void func_004e0c60(void *h);
extern int32_t func_004e0db0(void *h);
extern void func_004e1100(void *h);
extern void func_004de860(void *h);                                  /* aixp handle destroy */
extern void func_004e0560(void);
extern void func_004e03b0(void);
extern void func_004e0408(void);
extern uint32_t func_004de310(void);                                 /* AIX player main-type callback */
extern void func_004e0380(void);                                     /* AIX lock */
extern void func_004e0398(void);                                     /* AIX unlock */

/* 0x004e1230: one AIX handle server step. Used handles (byte 0 == 1) in state 1 or 2 continue in
 * 0x004e0c60 / 0x004e0db0 / 0x004e1100; 0x004e0db0 is retried once when it answers -1. */
void func_004e1230(int8_t *h)
{
    int32_t state;
    int32_t v;

    if (h[0] != 1) {
        return;
    }
    state = h[1];
    if (state == 1) {
        func_004e0c60(h);
        return;
    }
    if (state != 2 || h[3] != 0) {
        return;
    }
    v = *(volatile int32_t *)((uint8_t *)h + 6488);
    if (v == 0) {
        if (func_004e0db0(h) == -1) {
            func_004e0db0(h);
        }
        return;
    }
    if (v == 1) {
        func_004e1100(h);
    }
}

/* 0x004e1200: locked handle step. */
void func_004e1200(void *h)
{
    func_004e0380();
    func_004e1230(h);
    func_004e0398();
}

/* 0x004e12e8: server step over every AIX handle (0x195c bytes each from 0x0072b680 up to
 * 0x00731bf0). */
void func_004e12e8(void)
{
    uint8_t *h = (uint8_t *)P4_G2H(0x0072b680);

    do {
        uint8_t *next = h + 0x195c;

        func_004e1200(h);
        h = next;
    } while (h < (uint8_t *)P4_G2H(0x00731bf0));
}

/* 0x004dfc38: server step over the four AIX player handles (0x26c bytes each from 0x0072acc8):
 * used ones (byte 0 == 1) go to 0x004df800. */
void func_004dfc38(void)
{
    uint8_t *h = (uint8_t *)P4_G2H(0x0072acc8);
    int32_t i;

    for (i = 3; i >= 0; i--) {
        if (h[0] == 1) {
            func_004df800(h);
        }
        h += 0x26c;
    }
}

/* 0x004de2e8: SVM vsync-type callback of the AIX player: the player step, then the library step. */
uint32_t func_004de2e8(void)
{
    func_004dfc38();
    func_004e12e8();
    return 0;
}

/* 0x004e03b0 AIX library init (counted at 0x0072b678): the first one clears the 0x6570-byte handle
 * area at 0x0072b680. 0x004e0408 is the matching finish: it clears the area again at count 0. */
void func_004e03b0(void)
{
    func_004e0380();
    if (M32(0x0072b678) == 0) {
        func_0043f9c8((void *)P4_G2H(0x0072b680), 0, 0x6570);
    }
    M32(0x0072b678) = M32(0x0072b678) + 1;
    func_004e0398();
}

/* 0x004de340 AIX player init (counted at 0x0072acb4): the first one starts the ADX library and the
 * AIX library, clears the four player handles (0x9b0 bytes at 0x0072acc8) and registers the
 * player's SVM callbacks (vsync-type id 0, main type; the main id is kept at 0x0072acb8). */
void func_004de340(void)
{
    if (M32(0x0072acb4) == 0) {
        func_004ca720();
        func_004e03b0();
        func_0043f9c8((void *)P4_G2H(0x0072acc8), 0, 0x9b0);
        func_004f0c78(2, 0, (void *)func_004de2e8, 0, (const char *)P4_G2H(0x007595f8u));
        M32(0x0072acb8) = func_004f0a60(5, (void *)func_004de310, 0, (const char *)P4_G2H(0x00759608u));
    }
    M32(0x0072acb4) = M32(0x0072acb4) + 1;
}

/* 0x004de3f0: the matching finish: the last one destroys the used handles, clears the table,
 * removes the SVM callbacks and finishes the AIX and ADX libraries. */
void func_004de3f0(void)
{
    uint8_t *h = (uint8_t *)P4_G2H(0x0072acc8);
    int32_t i;

    M32(0x0072acb4) = M32(0x0072acb4) - 1;
    if (M32(0x0072acb4) != 0) {
        return;
    }
    for (i = 3; i >= 0; i--) {
        if (h[0] == 1) {
            func_004de860(h);
        }
        h += 0x26c;
    }
    func_0043f9c8((void *)P4_G2H(0x0072acc8), 0, 0x9b0);
    func_004f0b88(2, 0);
    func_004f0b88(5, M32(0x0072acb8));
    func_004e0408();
    func_004ca8b0();
}

/* 0x004e5a08: dvCiGetInterface. Resets the dvCi root directory (func_004e72a0 with no path) and
 * returns the dvCi CVFS function table at 0x007330d8. */
void *func_004e5a08(void)
{
    func_004e72a0(0);
    return (void *)P4_G2H(0x007330d8);
}

/* 0x004e9e50: mfCiGetInterface, the mfCi CVFS function table at 0x00738d00. */
void *func_004e9e50(void)
{
    return (void *)P4_G2H(0x00738d00);
}

/* 0x004e5b10: one dvCi object's server step (0x48-byte object: +0 in use, +2 state, +0xc offset, +0x10
 * size, +0x14 read-so-far, +0x1c the pending request). Under the SVM lock, with a pending request the
 * request's status (func_004e39b0) decides: 0 (idle, request dropped) and 3 (done: the offset
 * advances by the size and the state becomes 1, the read-so-far taking the size) and 9 (error: state 3)
 * release the request (func_004e3770) and clear it; the other values 1, 2, 4..8 leave it pending. */
void func_004e5b10(uint8_t *obj)
{
    int32_t req = W32(obj, 0x1c);

    if (req == 0) {
        return;
    }
    func_004f0638();
    switch ((uint32_t)func_004e39b0(W32(obj, 0x1c))) {
    case 0:
        obj[2] = 0;
        break;
    case 3:
        W32(obj, 0x14) = W32(obj, 0x10);
        obj[2] = 1;
        W32(obj, 0xc) = W32(obj, 0xc) + W32(obj, 0x10);
        break;
    case 9:
        obj[2] = 3;
        break;
    default:
        func_004f06b0();
        return;
    }
    func_004e3770(W32(obj, 0x1c));
    W32(obj, 0x1c) = 0;
    func_004f06b0();
}

/* 0x004e5bd8: the dvCi server: re-entry is refused by a test-and-set flag (0x007330d0); each of the 40
 * dvCi objects at 0x00733140 that is in use gets its step. */
void func_004e5bd8(void)
{
    int32_t i;

    if (func_004f1278((int32_t *)P4_G2H(0x007330d0)) == 0) {
        return;
    }
    for (i = 0; i < 40; i++) {
        uint8_t *obj = (uint8_t *)P4_G2H(0x00733140 + i * 0x48);

        if ((int8_t)obj[0] == 1) {
            func_004e5b10(obj);
        }
    }
    M32(0x007330d0) = 0;
}

/* 0x004e9fb0: the mfCi server: every in-use object of the 40 at 0x00738d68 (0x38 bytes each) gets
 * func_004e9fa8. */
void func_004e9fb0(void)
{
    int32_t i;

    for (i = 0; i < 40; i++) {
        uint8_t *obj = (uint8_t *)P4_G2H(0x00738d68 + i * 0x38);

        if ((int8_t)obj[0] != 0) {
            func_004e9fa8(obj);
        }
    }
}
