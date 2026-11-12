/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI "dvCi" CD file device (libadxe dvci.c) and its sceIo glue, translated from the retail asm 0x004E3A10..0x004E7258.
 *
 * The retail functions below have no `// FUN_` marker in the decomp tree (they are either unmarked or sit next to marked statics), so they are
 * port-owned here. Every function follows the retail asm call for call (the callees are the retail ones: func_004e4618 trace, func_004e4700 /
 * func_004e4738 lock pair, func_004244c8 scePrintf, the sceCd wrappers of libadxe at 0x004e36e8..0x004e4190 ...).
 *
 * Handle (DVS_CI_OBJ, 0x48 bytes, 40 in the table at 0x00733140): +0 used, +1 cached name, +2 state (0 idle, 1 done, 2 reading, 9 sceIo error), +4 size,
 * +8 length in sectors, +0xC position, +0x10 sectors of the running read, +0x14 sectors transferred, +0x18 buffer, +0x1C sceCd request handle,
 * +0x20..+0x43 the file record (+0x24 size), +0x44..+0x46 sceCd mode bytes. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "p4pc/ps2_libc.h"

#define P4_GW(a) (*(volatile uint32_t *)P4_G2H(a))
#define P4_STR(a) ((const char *)P4_G2H(a))

extern int func_004244c8(const char *fmt, ...);
extern void func_004e4618(int32_t code);
extern void func_004e5c68(char *name);
extern int32_t func_0042e2b8(void *rec, const void *name, int32_t mode);
extern int32_t func_004e6db0(const char *name);
extern int32_t func_004e6df0(const char *name, uint32_t *sector, uint32_t *sectors);
extern void func_004e6cc0(uint32_t *rec, const char *name, uint8_t *base, int32_t count);
extern void func_004f0638(void);
extern void func_004f06b0(void);
extern void func_004e36b8(void);
extern void func_004e39e8(uint8_t *req);
extern int32_t func_004e6490(const char *name);
extern void func_004cd378(const char *fmt, ...);
extern int32_t func_004e6778(const char *name, const char *entry);
extern int32_t func_004e6720(int32_t a, int32_t b);
extern int32_t func_004e66e8(int32_t c);
extern void func_004e4700(void);
extern void func_004e4738(void);
extern void func_004e39b8(void);
extern int32_t func_004e48b0(int32_t fd, int32_t cmd, void *arg);
extern int32_t func_00427868(int32_t fd, int32_t cmd, void *arg);
extern int64_t func_00429358(int32_t fd, int64_t off, int32_t whence);
extern int32_t func_004270f8(int32_t fd, int32_t off, int32_t whence);
extern int32_t func_0042f4b0(void);
extern int32_t func_0042f3e0(int32_t a);
extern int32_t func_004f1278(void *p);
extern void func_004f0668(void);
extern void func_004f06e0(void);
extern void func_004f0608(void);
extern void func_004f0620(void);
extern int32_t func_004258b0(uint32_t client, uint32_t fno, uint32_t mode, uint32_t send, uint32_t ssize, uint32_t recv, uint32_t rsize,
                             uint32_t endfunc, uint32_t efarg);
extern void func_004e72a0(const char *path);
extern void func_004e5b10(uint8_t *h);
extern int32_t func_004e6eb0(const char *name, void *rec);
extern void func_004e6640(const char *name, char *out);
extern int32_t func_0042e7c8(int32_t a);
extern int32_t func_004e5a68(void *rec, void *name);
extern void func_004cd3f0(const char *fmt, ...);
extern void func_004e5a38(void *obj, const char *msg);
extern uint8_t *func_004e5e28(void);
extern void func_004e5e60(uint8_t *h);
extern uint32_t func_004e36e8(void);
extern void func_00422218(uint32_t a, uint32_t b);
extern int32_t func_004e3810(uint32_t h, uint32_t sector, uint32_t n, uint32_t buf, const uint8_t *mode);
extern void func_004e3770(uint32_t h);
extern void func_004e42a0(void);
extern void func_004e4190(void);
extern void func_004e3ae0(uint32_t h);
extern void func_004e3c10(uint32_t h);
extern int32_t func_004e63b0(uint8_t *h);
extern void func_004e5bd8(void);
extern void func_004e62c8(uint8_t *h);
extern int32_t func_004e6528(uint8_t *h);
extern int32_t func_004e6538(uint8_t *h);
extern int32_t func_004e6558(uint8_t *h);
extern int32_t func_004e6560(uint8_t *h);

/* 0x004e3a10: polls the descriptor fd = h[0x30] with func_004e48b0(fd, 1, &state) while the state word stays 1; a negative result stores it at +0x34 and marks the handle as
 * failed (+2 = 9); more than 0x9000 rounds reports the timeout text at 0x0075AAC0. */
void func_004e3a10(uint8_t *h)
{
    int32_t state = 1;
    int32_t n = 0;
    int32_t r;

    while (state == 1) {
        func_004e4618(0x1100);
        r = func_004e48b0(*(int32_t *)(h + 0x30), 1, &state);
        func_004e4618(0x1101);
        if (r < 0) {
            *(int32_t *)(h + 0x34) = r;
            *(int8_t *)(h + 2) = 9;
            return;
        }
        func_004e39b8();
        n++;
        if (0x9000 < n) {
            func_004244c8(P4_STR(0x0075AAC0));
            return;
        }
    }
}

/* 0x004e3c78: when the sceIo layer is up (0x00731C4C) and the handle is in the failed state 2, fetches the pending error (func_0042f4b0, with the flag at 0x00731C48
 * set the bracket func_004f0668 / func_004f06e0 is used) while 0x00731C54 is raised; an error that is not 0, -1 and not 0x20 is stored at +0x34, printed (0x0075AB18) and
 * returns 1, else 0. */
int32_t func_004e3c78(uint8_t *h)
{
    int32_t r;
    int32_t v;

    if (P4_GW(0x00731C4C) == 0) {
        return 0;
    }
    if (*(int8_t *)(h + 2) != 2) {
        return 0;
    }
    P4_GW(0x00731C54) = 1;
    if (P4_GW(0x00731C48) == 1) {
        func_004f0668();
        func_004e4618(0x1400);
        r = func_0042f4b0();
        func_004e4618(0x1401);
        func_004f06e0();
        v = r + 1;
    } else {
        func_004e4618(0x1500);
        r = func_0042f4b0();
        func_004e4618(0x1501);
        v = r + 1;
    }
    P4_GW(0x00731C54) = 0;
    if ((uint32_t)v < 2) {
        return 0;
    }
    if (r == 0x20) {
        return 0;
    }
    *(int32_t *)(h + 0x34) = r;
    func_004244c8(P4_STR(0x0075AB18), r);
    return 1;
}

/* 0x004e4698: stores the sign extended low byte of a0 at 0x00731C44 and 0x00731C74; a0 == 1 prints 0x0075ACE8. Returns the stored value. */
int32_t func_004e4698(int32_t a0)
{
    P4_GW(0x00731C44) = (uint32_t)(int32_t)(int8_t)a0;
    P4_GW(0x00731C74) = P4_GW(0x00731C44);
    if (a0 == 1) {
        return func_004244c8(P4_STR(0x0075ACE8));
    }
    return (int32_t)P4_GW(0x00731C44);
}

/* 0x004e4770: when func_004f1278(0x00731C3C) succeeds, runs func_0042f3e0(a0) between the trace marks 0x9600 / 0x9601; its non zero result is dropped, a zero one
 * gives func_0042f4b0(); the word at 0x00731C3C is cleared; returns that value (0 otherwise). */
int32_t func_004e4770(int32_t a0)
{
    uint32_t *g = (uint32_t *)P4_G2H(0x00731C3C);
    int32_t r;
    int32_t v = 0;

    if (func_004f1278(g) == 0) {
        return 0;
    }
    func_004e4618(0x9600);
    r = func_0042f3e0(a0);
    func_004e4618(0x9601);
    if (r != 0) {
        *g = 0;
        return v;
    }
    v = func_0042f4b0();
    *g = 0;
    return v;
}

/* 0x004e47f8: sceIo seek under the func_004e4700 / func_004e4738 lock; with 0x00731C70 == 1 func_00429358 (text 0x0075AD00 on failure), else func_004270f8 (0x0075AD20).
 * The 64-bit backend preserves the offset and result; the legacy backend sign-extends its low word. */
int64_t func_004e47f8(int32_t a0, int64_t a1, int32_t a2)
{
    int64_t r;

    func_004e4700();
    if (P4_GW(0x00731C70) == 1) {
        r = func_00429358(a0, a1, a2);
        if (r < 0) {
            func_004244c8(P4_STR(0x0075AD00), r);
        }
    } else {
        r = func_004270f8(a0, (int32_t)a1, a2);
        if (r < 0) {
            func_004244c8(P4_STR(0x0075AD20), r);
        }
    }
    func_004e4738();
    return r;
}

/* 0x004e48b0: func_00427868(a0, a1, a2) under the lock; a negative result prints 0x0075AD40. */
int32_t func_004e48b0(int32_t a0, int32_t a1, void *a2)
{
    int32_t r;

    func_004e4700();
    r = func_00427868(a0, a1, a2);
    if (r < 0) {
        func_004244c8(P4_STR(0x0075AD40), r);
    }
    func_004e4738();
    return r;
}

extern int32_t sceRead(int32_t fd, void *buf, int32_t n);

/* 0x004e49e8: sceRead(a0, a1, a2) under the lock; a negative result prints 0x0075ADA0. */
int32_t func_004e49e8(int32_t a0, void *a1, int32_t a2)
{
    int32_t r;

    func_004e4700();
    r = sceRead(a0, a1, a2);
    if (r < 0) {
        func_004244c8(P4_STR(0x0075ADA0), r);
    }
    func_004e4738();
    return r;
}

/* 0x004e58a0: one call into the sif rpc client at 0x00732D30: the n1 words at a1 are copied to 0x00923780, func_004258b0(client, a0 + 0x400, 0, 0x00923780, 4 * n1,
 * 0x00923880, 4 * n2, 0, 0) runs between func_004f0608 / func_004f0620, and the n2 reply words at 0x00923880 are copied to a3. Returns the first reply word. */
int32_t func_004e58a0(uint32_t a0, uint32_t *a1, int32_t n1, uint32_t *a3, int32_t n2)
{
    uint32_t *snd = (uint32_t *)P4_G2H(0x00923780);
    uint32_t *rcv = (uint32_t *)P4_G2H(0x00923880);
    int32_t i;

    func_004f0608();
    if (n1 > 0) {
        for (i = 0; i < n1; i++) {
            snd[i] = a1[i];
        }
    }
    func_004258b0(0x00732D30, a0 + 0x400, 0, 0x00923780, (uint32_t)n1 << 2, 0x00923880, (uint32_t)n2 << 2, 0, 0);
    if (n2 > 0) {
        for (i = 0; i < n2; i++) {
            a3[i] = rcv[i];
        }
    }
    func_004f0620();
    return (int32_t)rcv[0];
}

/* 0x004e5ce0: 1 when the name a0 can be resolved: func_004e6eb0(name, &rec) with rec[1] != 0 gives 1; else func_004e6640(name, 0x00733C80) and the sceCd search
 * func_004e5a68(&rec, 0x00733C80) (between the trace marks 0x8100 / 0x8101, func_0042e7c8(0) syncs) decide. Null gives 0. */
int32_t func_004e5ce0(const char *a0)
{
    uint32_t rec[9] = {0};

    if (a0 == 0) {
        return 0;
    }
    func_004e6eb0(a0, rec);
    if (rec[1] != 0) {
        return 1;
    }
    func_004e6640(a0, (void *)P4_G2H(0x00733C80));
    func_004e4618(0x8100);
    func_0042e7c8(0);
    func_004e4618(0x8101);
    if (func_004e5a68(rec, (void *)P4_G2H(0x00733C80)) == 0) {
        return 0;
    }
    return 1;
}

/* 0x004e5d68 (dvCiGetFileSize): resolves the name like func_004e5ce0; a null name prints 0x0075B068, a resolved record returns rec[1], a failed sceCd search prints the name
 * (0x0075B098) and 0x0075B0C0 through func_004e5a38(0, text). */
int32_t dvCiGetFileSize(const char *a0)
{
    uint32_t rec[9] = {0};
    const char *msg;

    if (a0 == 0) {
        msg = P4_STR(0x0075B068);
    } else {
        func_004e6eb0(a0, rec);
        if (rec[1] != 0) {
            return (int32_t)rec[1];
        }
        func_004e6640(a0, (void *)P4_G2H(0x00733C80));
        func_004e4618(0x8200);
        func_0042e7c8(0);
        func_004e4618(0x8201);
        if (func_004e5a68(rec, (void *)P4_G2H(0x00733C80)) != 0) {
            return (int32_t)rec[1];
        }
        func_004cd3f0(P4_STR(0x0075B098), (void *)P4_G2H(0x00733C80));
        msg = P4_STR(0x0075B0C0);
    }
    func_004e5a38(0, msg);
    return 0;
}

/* 0x004e5e80 (dvCiOpen): a null name, a non zero rw (a2) or an exhausted table (func_004e5e28) print their text (0x0075B0F0 / B118 / B140) and give 0. The record at +0x20 is
 * filled by func_004e6eb0(name, h + 0x20); when its size (+0x24) is 0 the name is searched on the disc (func_004e6640 / func_004e5a68 with the trace marks 0x8300 / 0x8301; a failure
 * prints 0x0075B198, frees the handle with func_004e5e60 and gives 0), else byte +1 says it came from the cache. The handle gets +8 = (size + 0x7FF) >> 11, +0 = 1, +4 = size,
 * the counters cleared, +0x1C cleared, and +8 = 0x7FFFFFFF when func_004e6eb0 returned 1. Returns the handle. */
uint8_t *dvCiOpen(const char *a0, void *a1, int32_t a2)
{
    uint8_t *h;
    int32_t r;
    uint32_t size;
    const char *msg;

    if (a0 == 0) {
        msg = P4_STR(0x0075B0F0);
    } else if (a2 != 0) {
        msg = P4_STR(0x0075B118);
    } else {
        h = func_004e5e28();
        if (h != 0) {
            r = func_004e6eb0(a0, h + 0x20);
            if (*(uint32_t *)(h + 0x24) == 0) {
                func_004e6640(a0, (void *)P4_G2H(0x00733C80));
                func_004cd3f0(P4_STR(0x0075B170), (void *)P4_G2H(0x00733C80));
                func_004e4618(0x8300);
                func_0042e7c8(0);
                func_004e4618(0x8301);
                if (func_004e5a68(h + 0x20, (void *)P4_G2H(0x00733C80)) == 0) {
                    func_004e5a38(0, P4_STR(0x0075B198));
                    func_004e5e60(h);
                    return 0;
                }
                h[1] = 0;
            } else {
                h[1] = 1;
            }
            size = *(uint32_t *)(h + 0x24);
            *(uint32_t *)(h + 8) = (size + 0x7FF) >> 11;
            h[0] = 1;
            *(uint32_t *)(h + 4) = size;
            *(uint32_t *)(h + 0xC) = 0;
            *(uint32_t *)(h + 0x18) = 0;
            *(uint32_t *)(h + 0x10) = 0;
            *(uint32_t *)(h + 0x14) = 0;
            h[2] = 0;
            *(uint32_t *)(h + 0x1C) = 0;
            if (r == 1) {
                *(uint32_t *)(h + 8) = 0x7FFFFFFF;
            }
            return h;
        }
        msg = P4_STR(0x0075B140);
    }
    func_004e5a38(0, msg);
    return 0;
}

/* 0x004e5ff8 (dvCiClose): a null handle does nothing; a handle in state >= 2 is stopped first (func_004e62c8); the used byte is cleared and the handle freed (func_004e5e60). */
void dvCiClose(uint8_t *h)
{
    if (h == 0) {
        return;
    }
    if (h[2] < 2) {
        h[0] = 0;
    } else {
        func_004e62c8(h);
        h[0] = 0;
    }
    func_004e5e60(h);
}

/* 0x004e6050 (dvCiSeek): mode 0 sets the position (+0xC) to a1, mode 1 adds it, mode 2 sets it to the length (+8) plus a1; the position is clamped to [0, length], +0x14 cleared,
 * and returned. A null handle prints 0x0075B1C8. */
int32_t func_004e6050(uint8_t *h, int32_t a1, int32_t mode)
{
    int32_t len;
    int32_t pos;

    if (h == 0) {
        func_004e5a38(0, P4_STR(0x0075B1C8));
        return 0;
    }
    len = *(int32_t *)(h + 8);
    if (mode == 0) {
        *(int32_t *)(h + 0xC) = a1;
    } else if (mode == 2) {
        *(int32_t *)(h + 0xC) = len + a1;
    } else if (mode == 1) {
        *(int32_t *)(h + 0xC) = *(int32_t *)(h + 0xC) + a1;
    }
    pos = *(int32_t *)(h + 0xC);
    *(int32_t *)(h + 0x14) = 0;
    if (!(pos < len)) {
        pos = len;
    }
    if (pos < 0) {
        pos = 0;
    }
    *(int32_t *)(h + 0xC) = pos;
    return pos;
}

/* 0x004e60e8 (dvCiTell): the position (+0xC); a null handle prints 0x0075B1C8 and gives 0. */
int32_t func_004e60e8(uint8_t *h)
{
    if (h == 0) {
        func_004e5a38(0, P4_STR(0x0075B1C8));
        return 0;
    }
    return *(int32_t *)(h + 0xC);
}

/* 0x004e6120 (dvCiReqRd): starts reading a1 sectors into a2. A null handle, a negative count or a null buffer print (0x0075B1C8 / B1E0 / B200) and give 0; a handle in state 2
 * gives 0; a count of 0 marks the handle done (+2 = 1). Otherwise a sceCd request (func_004e36e8) is made: the sector count is limited to what is left (+8 - +0xC), the mode bytes
 * at 0x00733DC0 are copied to +0x44..+0x46, func_00422218 gets the buffer range, func_004e3810(request, +0x20 + +0xC, count, buffer, mode) starts it (a failure releases the request and
 * gives 0). Normally the state becomes 2 and the count is returned; when 0x00733DC4 is 1 the read is waited for (func_0042e7c8(0), func_004e42a0, func_004e4190, func_004e3ae0 and
 * func_004e3770 of the request), +0x14 = a1, the position advances by the count and the state becomes 1. */
int32_t dvCiReqRd(uint8_t *h, int32_t a1, uint32_t a2)
{
    uint32_t req;
    int32_t remain;
    int32_t n;
    int32_t flag;
    const char *msg;

    if (h == 0) {
        msg = P4_STR(0x0075B1C8);
    } else if (a1 < 0) {
        msg = P4_STR(0x0075B1E0);
    } else if (a2 == 0) {
        msg = P4_STR(0x0075B200);
    } else {
        if ((int8_t)h[2] == 2) {
            return 0;
        }
        if (a1 == 0) {
            h[2] = 1;
            return 0;
        }
        req = func_004e36e8();
        *(uint32_t *)(h + 0x1C) = req;
        if (req == 0) {
            return 0;
        }
        remain = *(int32_t *)(h + 8) - *(int32_t *)(h + 0xC);
        n = a1;
        if (!(a1 < remain)) {
            n = remain;
        }
        h[0x44] = *(uint8_t *)P4_G2H(0x00733DC0);
        h[0x45] = *(uint8_t *)P4_G2H(0x00733DC1);
        h[0x46] = *(uint8_t *)P4_G2H(0x00733DC2);
        *(int32_t *)(h + 0x10) = n;
        *(uint32_t *)(h + 0x18) = a2;
        func_00422218(a2, a2 + ((uint32_t)n << 11) - 1);
        if (func_004e3810(*(uint32_t *)(h + 0x1C), *(uint32_t *)(h + 0x20) + *(uint32_t *)(h + 0xC), *(uint32_t *)(h + 0x10), *(uint32_t *)(h + 0x18), h + 0x44) == 0) {
            func_004e3770(*(uint32_t *)(h + 0x1C));
            return 0;
        }
        flag = (int32_t)P4_GW(0x00733DC4);
        if (flag != 1) {
            h[2] = 2;
            return *(int32_t *)(h + 0x10);
        }
        func_0042e7c8(0);
        func_004e42a0();
        func_004e4190();
        func_004e3ae0(*(uint32_t *)(h + 0x1C));
        func_004e3770(*(uint32_t *)(h + 0x1C));
        *(uint32_t *)(h + 0x1C) = 0;
        *(int32_t *)(h + 0x14) = a1;
        *(int32_t *)(h + 0xC) = *(int32_t *)(h + 0xC) + *(int32_t *)(h + 0x10);
        h[2] = 1;
        return *(int32_t *)(h + 0x10);
    }
    func_004e5a38(h, msg);
    return 0;
}

/* 0x004e62c8 (dvCiStopTr): stops a running transfer: a null handle prints 0x0075B1C8; a handle below state 2 is left alone; else the request (+0x1C) is aborted (func_004e3c10),
 * and func_004e4190, func_004e5bd8 and func_004e63b0(handle) are repeated until the state leaves 2 (more than 0x9FFFF rounds print 0x0075B228). +0x14 and the state are cleared. */
void func_004e62c8(uint8_t *h)
{
    int32_t n = 0;

    if (h == 0) {
        func_004e5a38(0, P4_STR(0x0075B1C8));
        return;
    }
    if (h[2] < 2) {
        return;
    }
    if (*(uint32_t *)(h + 0x1C) != 0) {
        func_004e3c10(*(uint32_t *)(h + 0x1C));
    }
    for (;;) {
        func_004e4190();
        func_004e5bd8();
        func_004e63b0(h);
        if ((int8_t)h[2] != 2) {
            *(uint32_t *)(h + 0x14) = 0;
            break;
        }
        n++;
        if (0x9FFFF < n) {
            func_004cd3f0(P4_STR(0x0075B228));
            *(uint32_t *)(h + 0x14) = 0;
            break;
        }
    }
    h[2] = 0;
}

/* 0x004e63b0 (dvCiGetStat): the sign extended state byte; a null handle prints 0x0075B1C8 and gives 0. */
int32_t func_004e63b0(uint8_t *h)
{
    if (h == 0) {
        func_004e5a38(0, P4_STR(0x0075B1C8));
        return 0;
    }
    return (int8_t)h[2];
}

/* 0x004e63f0 (dvCiGetNumTr): the number of bytes transferred (+0x14 sectors of 2048) in state 1, else 0; a null handle prints 0x0075B1C8. */
int32_t func_004e63f0(uint8_t *h)
{
    if (h == 0) {
        func_004e5a38(0, P4_STR(0x0075B1C8));
        return 0;
    }
    if ((int8_t)h[2] != 1) {
        return 0;
    }
    return (int32_t)(*(uint32_t *)(h + 0x14) << 11);
}

/* 0x004e6480: the word at 0x007330CC. */
int32_t func_004e6480(void)
{
    return (int32_t)P4_GW(0x007330CC);
}

/* 0x004e6578: maps the sceCd error code a1 (when a0 is non null): 0xC8 func_004e6528(), 0xC9 func_004e6538(), 0xCA and 0xCC func_004e6558(), 0xCB and 0xCD func_004e6560(),
 * 0x12B 1; other codes and a null a0 give 0. */
int32_t func_004e6578(void *a0, int32_t a1)
{
    if (a0 == 0) {
        return 0;
    }
    switch (a1) {
    case 0xC8:
        return func_004e6528((uint8_t *)a0);
    case 0xC9:
        return func_004e6538((uint8_t *)a0);
    case 0xCA:
    case 0xCC:
        return func_004e6558((uint8_t *)a0);
    case 0xCB:
    case 0xCD:
        return func_004e6560((uint8_t *)a0);
    case 0x12B:
        return 1;
    }
    return 0;
}

/* 0x004e7258: 1 when the name a0's record (func_004e6eb0(name, &rec)) has a non zero first word, else 0. */
int32_t func_004e7258(const char *a0)
{
    uint32_t rec[12] = {0};

    func_004e6eb0(a0, rec);
    return rec[0] != 0;
}

/* ---- the sceCd request slot and libadxe dvCi helpers (0x004E36E8..0x004E6EB0) ---------------------------------------------------------------
 * The single request slot lives at 0x00731C00: +0 used, +1 kind (1 sceCd read, 2 sceIo), +2 status (0 idle, 1 started, 2 running, 3 done, 9 error), +3 = -1,
 * +4 abort flag, +8 / +0xC / +0x10 the sceCd read arguments (+0x14 its mode bytes) or +0x18 descriptor, +0x20 / +0x28 the 64 bit byte count / position of a
 * sceIo read, +0x30 the descriptor. The counters at 0x00731C60 / 0x00731C64 count the slot allocations / releases and 0x00731C5C the completed requests. */
extern void func_004e3a10(uint8_t *req);
extern void *func_0043f9c8(void *d, int32_t c, uint32_t n);
extern int32_t func_00426cf0(const char *name, int32_t flags, int32_t mode);
extern int32_t func_00426f80(int32_t fd);

/* 0x004e39b8: the busy wait of the request poll (about 512 rounds of nops). */
void func_004e39b8(void)
{
    volatile int32_t v = 0x3FFF;

    v = v - 0x20;
    while (v >= 0) {
        v = v - 0x20;
    }
}

/* 0x004e36e8: takes the request slot when it is free: clears it (func_004e36b8), marks it used, counts the allocation (0x00731C60), status 0, byte +3 = -1 and +4 = 0;
 * returns the slot or 0. */
uint32_t func_004e36e8(void)
{
    uint8_t *req = (uint8_t *)P4_G2H(0x00731C00);
    uint32_t r = 0;

    func_004f0638();
    if (*(int8_t *)req == 0) {
        func_004e36b8();
        req[0] = 1;
        r = P4_H2G(req);
        req[3] = (uint8_t)-1;
        req[2] = 0;
        P4_GW(0x00731C60) = P4_GW(0x00731C60) + 1;
        *(uint32_t *)(req + 4) = 0;
    }
    func_004f06b0();
    return r;
}

/* 0x004e3770: releases the request slot a0: another pointer prints 0x0075AA60 (func_004244c8(text, a0, slot)); a slot whose status is not 0, 3 or 9 prints 0x0075AAA0; then the
 * slot is cleared (func_004e36b8), unused and the release counted (0x00731C64). */
void func_004e3770(uint32_t h)
{
    uint8_t *a0 = (uint8_t *)P4_G2H(h);
    uint8_t *req = (uint8_t *)P4_G2H(0x00731C00);
    int32_t st;

    func_004f0638();
    if (a0 != req) {
        func_004244c8(P4_STR(0x0075AA60), a0, req);
    } else {
        st = (int8_t)a0[2];
        if (st != 0 && st != 3 && st != 9) {
            func_004244c8(P4_STR(0x0075AAA0));
        }
        func_004e36b8();
        req[0] = 0;
        P4_GW(0x00731C64) = P4_GW(0x00731C64) + 1;
    }
    func_004f06b0();
}

/* 0x004e3810: starts a sceCd read on the idle request a0 (status 0, 3 or 9) unless func_0042e7c8(1) says the drive is busy (trace marks 0x1200 / 0x1201): status 1, kind 1,
 * +8 = a1, +0xC = a2, +0x10 = a3, the 4 mode bytes at a4 copied to +0x14, +4 = 0; returns 1 when it was started, else 0. */
int32_t func_004e3810(uint32_t h, uint32_t a1, uint32_t a2, uint32_t a3, const uint8_t *a4)
{
    uint8_t *a0 = (uint8_t *)P4_G2H(h);
    int32_t r16;
    int32_t r = 0;
    int32_t st;

    func_004e4618(0x1200);
    r16 = func_0042e7c8(1);
    func_004e4618(0x1201);
    if (r16 == 1) {
        return 0;
    }
    func_004f0638();
    st = (int8_t)a0[2];
    if (st == 0 || st == 3 || st == 9) {
        a0[2] = 1;
        r = 1;
        a0[1] = 1;
        *(uint32_t *)(a0 + 8) = a1;
        memcpy(a0 + 0x14, a4, 4);
        *(uint32_t *)(a0 + 0xC) = a2;
        *(uint32_t *)(a0 + 0x10) = a3;
        *(uint32_t *)(a0 + 4) = 0;
    }
    func_004f06b0();
    return r;
}

/* 0x004e3900: starts a sceIo read on the idle request a0: +0x30 = fd (a1), kind 2, status 1, +0x18 = a4 (buffer), +0x20 = a3 (64 bit byte count), +0x28 = a2 (64 bit position), +4 = 0; returns 1 when started. */
int32_t func_004e3900(uint32_t h, int32_t a1, int64_t a2, int64_t a3, uint32_t a4)
{
    uint8_t *a0 = (uint8_t *)P4_G2H(h);
    int32_t r = 0;
    int32_t st;

    func_004f0638();
    st = (int8_t)a0[2];
    if (st == 0 || st == 3 || st == 9) {
        *(int32_t *)(a0 + 0x30) = a1;
        a0[1] = 2;
        a0[2] = 1;
        r = 1;
        *(uint32_t *)(a0 + 0x18) = a4;
        *(int64_t *)(a0 + 0x20) = a3;
        *(int64_t *)(a0 + 0x28) = a2;
        *(uint32_t *)(a0 + 4) = 0;
    }
    func_004f06b0();
    return r;
}

/* 0x004e3ae0: waits for the request a0 under the guard word 0x00731C3C (func_004f0608 / func_004f0620; up to 1000 rounds of func_004e39b8 while the guard is taken), then,
 * when the request status is 2, runs its handler by kind (kind 1 func_004e39e8(request), kind 2 func_004e3a10(request)); a status other than 9 afterwards becomes 3 and
 * the completion counter 0x00731C5C is incremented. The guard is released. */
void func_004e3ae0(uint32_t h)
{
    uint8_t *a0 = (uint8_t *)P4_G2H(h);
    int16_t i = 0;
    int32_t st;
    uint8_t *req = (uint8_t *)P4_G2H(0x00731C00);

    for (;;) {
        if (!(i < 0x3E8)) {
            st = (int8_t)a0[2];
            break;
        }
        func_004f0608();
        if (P4_GW(0x00731C3C) != 1) {
            P4_GW(0x00731C3C) = 1;
            func_004f0620();
            st = (int8_t)a0[2];
            break;
        }
        func_004f0620();
        func_004e39b8();
        i = (int16_t)(i + 1);
    }
    if (st != 2) {
        P4_GW(0x00731C3C) = 0;
        return;
    }
    if ((int8_t)req[1] == 1) {
        func_004e39e8(a0);
    }
    if ((int8_t)req[1] == st) {
        func_004e3a10(a0);
    }
    if ((int8_t)a0[2] != 9) {
        a0[2] = 3;
        P4_GW(0x00731C5C) = P4_GW(0x00731C5C) + 1;
    }
    P4_GW(0x00731C3C) = 0;
}

/* 0x004e3c10: flags the request a0 for abort (+4 = 1) under the request lock; a null request prints 0x0075AAE0 and an already flagged one 0x0075AB00. */
void func_004e3c10(uint32_t h)
{
    uint8_t *a0 = (uint8_t *)P4_G2H(h);
    func_004f0638();
    if (a0 == 0) {
        func_004244c8(P4_STR(0x0075AAE0));
    } else if (*(int32_t *)(a0 + 4) != 1) {
        *(int32_t *)(a0 + 4) = 1;
    } else {
        func_004244c8(P4_STR(0x0075AB00));
    }
    func_004f06b0();
}

/* 0x004e4700: the sceIo lock (func_004f0668) when 0x00731C74 == 1. */
void func_004e4700(void)
{
    if (P4_GW(0x00731C74) == 1) {
        func_004f0668();
    }
}

/* 0x004e4738: the sceIo unlock (func_004f06e0) when 0x00731C74 == 1. */
void func_004e4738(void)
{
    if (P4_GW(0x00731C74) == 1) {
        func_004f06e0();
    }
}

/* 0x004e4928: sceIo open under the lock; a negative descriptor prints 0x0075AD60. */
int32_t func_004e4928(const char *a0, int32_t a1)
{
    int32_t r;

    func_004e4700();
    r = func_00426cf0(a0, a1, 0);
    if (r < 0) {
        func_004244c8(P4_STR(0x0075AD60), r);
    }
    func_004e4738();
    return r;
}

/* 0x004e4990: sceIo close under the lock; a negative result prints 0x0075AD80. */
int32_t func_004e4990(int32_t a0)
{
    int32_t r;

    func_004e4700();
    r = func_00426f80(a0);
    if (r < 0) {
        func_004244c8(P4_STR(0x0075AD80), r);
    }
    func_004e4738();
    return r;
}

/* 0x004e5a38: calls the error callback at 0x007330C4 (object 0x007330C8) as callback(object, a1, a0) when one is set. */
void func_004e5a38(void *a0, const char *a1)
{
    void (*cb)(void *, const char *, void *) = (void (*)(void *, const char *, void *))p4_fn_g2h_sig(P4_GW(0x007330C4), P4_SIG(3, 0, 1, 1, 1, 0, 0, 0));

    if (cb != 0) {
        cb((void *)P4_G2H(P4_GW(0x007330C8)), a1, a0);
    }
}

/* 0x004e5a68: the sceCd file search func_0042e2b8(a0, a1, mode at 0x007330CC) between the trace marks 0x8000 / 0x8001. */
int32_t func_004e5a68(void *a0, void *a1)
{
    int32_t r;

    func_004e4618(0x8000);
    r = func_0042e2b8(a0, a1, (int32_t)P4_GW(0x007330CC));
    func_004e4618(0x8001);
    return r;
}

/* 0x004e5e28: the last free (used byte 0) handle of the dvCi table at 0x00733140 (40 entries of 0x48), or 0. */
uint8_t *func_004e5e28(void)
{
    uint8_t *p = (uint8_t *)P4_G2H(0x00733140);
    uint8_t *r = 0;
    int32_t i;

    for (i = 0x27; i >= 0; i--, p += 0x48) {
        if (*(int8_t *)p == 0) {
            r = p;
        }
    }
    return r;
}

/* 0x004e5e60: clears a dvCi handle (0x48 bytes). */
void func_004e5e60(uint8_t *h)
{
    func_0043f9c8(h, 0, 0x48);
}

/* 0x004e6528: the sector count (+0x14) shifted up by 11 bits as a 64 bit value, high word (sector count >> 21). */
int32_t func_004e6528(uint8_t *h)
{
    return (int32_t)(((int64_t)*(int32_t *)(h + 0x14) << 11) >> 32);
}

/* 0x004e6538: the sector count (+0x14) * 2048, low word. */
int32_t func_004e6538(uint8_t *h)
{
    return (int32_t)((uint32_t)*(int32_t *)(h + 0x14) << 11);
}

/* 0x004e6560: func_004e6490(handle). */
int32_t func_004e6560(uint8_t *h)
{
    return func_004e6490((const char *)h);
}

/* 0x004e6640: builds the disc path of the name a0 in a1: the directory prefix at 0x00733DD0, a "/" (0x0075B250) unless the name starts with one or a backslash,
 * the name, ";1" (0x0075B258) unless it already ends with it; func_004e5c68 then converts it. */
void func_004e6640(const char *a0, char *a1)
{
    size_t n;

    strcpy(a1, (const char *)P4_G2H(0x00733DD0));
    if (a0[0] != '/' && a0[0] != '\\') {
        strcat(a1, P4_STR(0x0075B250));
    }
    strcat(a1, a0);
    n = strlen(a1);
    if (func_004426e8(a1 + n - 2, P4_STR(0x0075B258)) != 0) {
        strcat(a1, P4_STR(0x0075B258));
    }
    func_004e5c68(a1);
}

/* 0x004e6eb0: the file record of the name a0 into rec[0] (sector) and rec[1] (size in bytes). The name equal to the text at 0x0075B320 gives {0, -1}, prints 0x0075B328 and returns 1.
 * Otherwise func_004e6db0() == 1 gives {sector, sectors << 11} from func_004e6df0(name, &sector, &sectors) and 0; else the record is cleared and, when the list
 * word 0x00733DB0 is set, func_004e6cc0(rec, name, 0x00733DB0 word, 0x00733DB4 word) fills it; 0. */
int32_t func_004e6eb0(const char *a0, void *rec_)
{
    uint32_t *rec = (uint32_t *)rec_;
    uint32_t sector;
    uint32_t sectors;
    uint32_t head;

    if (func_004426e8(P4_STR(0x0075B320), a0) == 0) {
        rec[1] = (uint32_t)-1;
        rec[0] = 0;
        func_004cd3f0(P4_STR(0x0075B328));
        return 1;
    }
    if (func_004e6db0(a0) == 1) {
        func_004e6df0(a0, &sector, &sectors);
        rec[1] = sectors << 11;
        rec[0] = sector;
        return 0;
    }
    rec[0] = 0;
    rec[1] = 0;
    head = P4_GW(0x00733DB0);
    if (head != 0) {
        func_004e6cc0(rec, a0, (uint8_t *)P4_G2H(head), (int32_t)P4_GW(0x00733DB4));
    }
    return 0;
}

/* 0x004e36b8: clears the request slot (0x38 bytes at 0x00731C00). */
void func_004e36b8(void)
{
    func_0043f9c8((void *)P4_G2H(0x00731C00), 0, 0x38);
}

/* 0x004e39e8: the sceCd wait of a read request: func_0042e7c8(0) between the trace marks 0x1300 / 0x1301. */
void func_004e39e8(uint8_t *a0)
{
    (void)a0;
    func_004e4618(0x1300);
    func_0042e7c8(0);
    func_004e4618(0x1301);
}

/* 0x004e5c68: turns the disc path a0 into the sceCd form: '/' becomes a backslash and lower case letters upper case. */
void func_004e5c68(char *a0)
{
    size_t n = strlen(a0);
    size_t i;
    uint32_t u;

    for (i = 0; i < n; i++) {
        u = (uint8_t)a0[i];
        if ((int8_t)a0[i] == '/') {
            a0[i] = '\\';
            u = '\\';
        }
        if (u - 0x61 < 0x1A) {
            a0[i] = (char)(u - 0x20);
        }
    }
}

/* 0x004e6490: the size of the file a0: func_004e6eb0(name, &rec) with rec[1] != 0 gives rec[1]; else func_004e6640(name, 0x00733C80), the sceCd search
 * func_004e5a68(&rec, 0x00733C80) (trace marks 0x8400 / 0x8401, func_0042e7c8(0) first) and rec[1] when it found the file; otherwise 0x0075B098 is printed with the path and 0 returned. */
int32_t func_004e6490(const char *a0)
{
    uint32_t rec[16] = {0};

    func_004e6eb0(a0, rec);
    if (rec[1] != 0) {
        return (int32_t)rec[1];
    }
    func_004e6640(a0, (char *)P4_G2H(0x00733C80));
    func_004e4618(0x8400);
    func_0042e7c8(0);
    func_004e4618(0x8401);
    if (func_004e5a68(rec, (void *)P4_G2H(0x00733C80)) != 0) {
        return (int32_t)rec[1];
    }
    func_004cd3f0(P4_STR(0x0075B098), (void *)P4_G2H(0x00733C80));
    return 0;
}

/* 0x004e6cc0: searches the table at a2 of a3 entries of 8 bytes for the name a1: entry i is compared (func_004e6778(name, a2 + (word 0x00733DB8 << 3) + i * (word 0x00733DBC + 1)),
 * 0 = equal) and the first equal entry's two words are copied to a0; none clears a0. */
void func_004e6cc0(uint32_t *a0, const char *a1, uint8_t *a2, int32_t a3)
{
    int32_t i = 0;
    uint8_t *w = a2;
    uint32_t *g = (uint32_t *)P4_G2H(0x00733DB0);

    if (a3 > 0) {
        do {
            uint32_t stride = g[3];
            uint32_t off = g[2];
            uint32_t idx = stride * (uint32_t)i + (uint32_t)i;

            i++;
            if (func_004e6778(a1, (const char *)(a2 + ((off << 3) + idx))) == 0) {
                a0[0] = *(uint32_t *)w;
                a0[1] = *(uint32_t *)(w + 4);
                return;
            }
            w += 8;
        } while (i < a3);
    }
    a0[1] = 0;
    a0[0] = 0;
}

/* 0x004e6db0: 1 when the name a0 has the form of a memory file name: 17 characters with a '.' at position 8. */
int32_t func_004e6db0(const char *a0)
{
    if (strlen(a0) != 0x11) {
        return 0;
    }
    return (int8_t)a0[8] == '.';
}

/* 0x004e6df0: parses "<sector>.<sectors>" (hex) into *a1 / *a2 (a2 may be null), prints the name, sector and size through func_004cd378 (0x0075B2E0 / B2F8 / B308; the size is read
 * through a2 even when it is null, as the retail code does) and returns 1. */
int32_t func_004e6df0(const char *a0, uint32_t *a1, uint32_t *a2)
{
    uint32_t pg = P4_H2G(a0);
    const char *p;

    *a1 = (uint32_t)func_00444458(a0, &pg, 0x10);
    p = (const char *)P4_G2H(pg);
    if (*p != 0) {
        p = p + 1;
    }
    if (a2 != 0) {
        pg = P4_H2G(p);
        *a2 = (uint32_t)func_00444458(p, &pg, 0x10);
        p = (const char *)P4_G2H(pg);
    }
    func_004cd378(P4_STR(0x0075B2E0), a0);
    func_004cd378(P4_STR(0x0075B2F8), *a1);
    func_004cd378(P4_STR(0x0075B308), *a2);
    return 1;
}

/* 0x004e6778: the file name compare of the disc list: -1 when the lengths differ or func_004e6720 (the character compare, non zero for a difference) finds a differing pair, else 0. */
int32_t func_004e6778(const char *a0, const char *a1)
{
    int32_t n0 = (int32_t)strlen(a0);
    int32_t n1 = (int32_t)strlen(a1);
    int32_t i;

    if (n0 != n1) {
        return -1;
    }
    if (n0 > 0) {
        i = 0;
        do {
            int32_t c0 = (int8_t)a0[i];
            int32_t c1 = (int8_t)a1[i];

            i++;
            if (func_004e6720(c0, c1) != 0) {
                return -1;
            }
        } while (i < n0);
    }
    return 0;
}

/* 0x004e66e8: the disc name character form of the (signed byte) c: a backslash becomes '/', lower case letters upper case, anything else is unchanged. */
int32_t func_004e66e8(int32_t c)
{
    int32_t v = (int8_t)c;
    int32_t u = v;

    if ((uint32_t)(v - 0x61) < 0x1A) {
        u = (int8_t)(v - 0x20);
    }
    return (v ^ 0x5C) != 0 ? u : 0x2F;
}

/* 0x004e6720: -1 when the characters a and b differ in the form of func_004e66e8, else 0. */
int32_t func_004e6720(int32_t a, int32_t b)
{
    int32_t x = func_004e66e8((int8_t)a);
    int32_t y = func_004e66e8((int8_t)b);

    return (x ^ y) != 0 ? -1 : 0;
}
