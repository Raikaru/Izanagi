/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI "htCi" host (sceIo) file device (libadxe htci.c), translated from the retail asm 0x004E7668..0x004E8110.
 *
 * Handle (0x30 bytes, 13 in the table at 0x00733FD0): +0 used, +1 name came from the cache, +2 state (0 idle, 1 done, 2 reading), +8 file size (64 bit), +0x10 length in sectors,
 * +0x14 transfer counter, +0x18 sceIo descriptor, +0x1C position (sectors), +0x20 buffer, +0x24 sectors of the running read, +0x28 sectors transferred, +0x2C sceCd request handle.
 * Every function follows the retail asm call for call; the callees are the retail ones. */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "p4pc/ps2_libc.h"

#define P4_GW(a) (*(volatile uint32_t *)P4_G2H(a))
#define P4_STR(a) ((const char *)P4_G2H(a))

extern int32_t func_004f1278(void *p);
extern void func_004e75a0(uint8_t *h);
extern void func_004e4618(int32_t code);
extern void func_004e8b98(const char *name, uint32_t *rec);
extern void func_004e82a0(const char *name, char *out);
extern int32_t func_004e4928(void *name, int32_t mode);
extern int32_t func_004e4990(int32_t fd);
extern int64_t func_004e47f8(int32_t fd, int64_t off, int32_t whence);
extern void func_004cd3f0(const char *fmt, ...);
extern void func_004cd378(const char *fmt, ...);
extern void func_004e7458(void *obj, const char *msg);
extern uint8_t *func_004e7a00(void);
extern void func_004e7a38(uint8_t *h);
extern int64_t func_004e77c0(const char *name);
extern int32_t func_004e7488(void);
extern uint32_t func_004e36e8(void);
extern int32_t func_004e7540(uint8_t *h);
extern int32_t func_004e3900(uint32_t req, int32_t fd, int64_t pos, int64_t bytes, uint32_t buf);
extern void func_004e42a0(void);
extern void func_004e4190(void);
extern void func_004e3770(uint32_t req);
extern void func_004e3c10(uint32_t req);
extern void func_004e7668(void);
extern int32_t func_004e7fc8(uint8_t *h);
extern void func_004e7ed0(uint8_t *h);
extern void func_004e7428(void);
extern int func_004244c8(const char *fmt, ...);
extern int func_00446ed8(char *buf, const char *fmt, va_list ap);
extern void *func_0043f9c8(void *d, int32_t c, uint32_t n);
extern void func_004f0638(void);
extern void func_004f06b0(void);
extern int32_t func_004e39b0(uint32_t req);
extern int32_t func_004e8428(const char *name, const char *entry);
extern int32_t func_004e83d0(int32_t a, int32_t b);
extern int32_t func_004e83b8(int32_t c);
extern int32_t func_004e81d8(const char *name);
extern int32_t func_004e8248(const char *name);
extern void func_004e8aa8(uint32_t *rec, const char *name, uint8_t *base, int32_t count);
extern void func_004e3770(uint32_t req);
extern int32_t func_004e8070(uint8_t *h);
extern int32_t func_004e8090(uint8_t *h);
extern int32_t func_004e80c0(uint8_t *h);
extern int32_t func_004e80e0(uint8_t *h);

/* 0x004e7668: when func_004f1278(0x00733F64) succeeds, closes (func_004e75a0) every handle of the table at 0x00733FD0 (13 entries of 0x30) whose used byte is 1 and clears 0x00733F64. */
void func_004e7668(void)
{
    uint8_t *h;
    int32_t i;

    if (func_004f1278((void *)P4_G2H(0x00733F64)) == 0) {
        return;
    }
    h = (uint8_t *)P4_G2H(0x00733FD0);
    for (i = 0xC; i >= 0; i--, h += 0x30) {
        if (*(int8_t *)h == 1) {
            func_004e75a0(h);
        }
    }
    P4_GW(0x00733F64) = 0;
}

/* 0x004e76f8: 1 when the name a0 resolves: func_004e8b98(name, &rec) with rec[1] != 0 gives 1; else func_004e82a0(name, 0x00734240) and func_004e4928(0x00734240, 1) open it (trace
 * marks 0x4200 / 0x4201); a negative descriptor gives 0, else it is closed (func_004e4990, marks 0x4300 / 0x4301) and 1 is returned. Null gives 0. */
int32_t func_004e76f8(const char *a0)
{
    uint32_t rec[8] = {0};
    int32_t fd;

    if (a0 == 0) {
        return 0;
    }
    func_004e8b98(a0, rec);
    if (rec[1] != 0) {
        return 1;
    }
    func_004e82a0(a0, (void *)P4_G2H(0x00734240));
    func_004e4618(0x4200);
    fd = func_004e4928((void *)P4_G2H(0x00734240), 1);
    func_004e4618(0x4201);
    if (fd < 0) {
        return 0;
    }
    func_004e4618(0x4300);
    func_004e4990(fd);
    func_004e4618(0x4301);
    return 1;
}

/* 0x004e77c0 (htCiGetFileSize): the size of the file a0 in bytes: a record from func_004e8b98 with rec[1] != 0 gives rec[1]; else the file is opened (func_004e4928, marks 0x4400 /
 * 0x4401), sought to its start and end (func_004e47f8(fd, 0, 0) and (fd, 0, 2), marks 0x4600 / 0x4601) and closed (func_004e4990, marks 0x5000 / 0x5001 or 0x4500 / 0x4501); the
 * end offset is the size. Failures print through func_004cd3f0 (0x0075B4A8 with the path, 0x0075B4F0, 0x0075B550) and func_004e7458(0, text) (0x0075B478 null name, B4C8, B528, B588 seek,
 * B5B8 close) and give 0. The uncached path preserves the seek's full 64-bit EE v0;
 * the cached word retains retail's signed lw extension. */
int64_t htCiGetFileSize(const char *a0)
{
    uint32_t rec[8] = {0};
    int32_t fd;
    int64_t sz;
    int32_t r;

    if (a0 == 0) {
        func_004e7458(0, P4_STR(0x0075B478));
        return 0;
    }
    func_004e8b98(a0, rec);
    if (rec[1] != 0) {
        return (int32_t)rec[1];
    }
    func_004e82a0(a0, (void *)P4_G2H(0x00734240));
    func_004e4618(0x4400);
    fd = func_004e4928((void *)P4_G2H(0x00734240), 1);
    func_004e4618(0x4401);
    if (fd < 0) {
        func_004cd3f0(P4_STR(0x0075B4A8), (void *)P4_G2H(0x00734240));
        func_004e7458(0, P4_STR(0x0075B4C8));
        return 0;
    }
    if (fd == 0x10004) {
        func_004cd3f0(P4_STR(0x0075B4F0));
        func_004e7458(0, P4_STR(0x0075B528));
        return 0;
    }
    if (fd == 0x13) {
        func_004cd3f0(P4_STR(0x0075B550));
        func_004e7458(0, P4_STR(0x0075B528));
        return 0;
    }
    func_004e4618(0x4600);
    func_004e47f8(fd, 0, 0);
    func_004e4618(0x4601);
    func_004e4618(0x4600);
    sz = func_004e47f8(fd, 0, 2);
    func_004e4618(0x4601);
    if (sz < 0) {
        func_004e7458(0, P4_STR(0x0075B588));
        func_004e4618(0x4500);
        func_004e4990(fd);
        func_004e4618(0x4501);
        return 0;
    }
    func_004e4618(0x5000);
    r = func_004e4990(fd);
    func_004e4618(0x5001);
    if (r >= 0) {
        return sz;
    }
    func_004e7458(0, P4_STR(0x0075B5B8));
    return 0;
}

/* 0x004e7a58 (htCiOpen): a null name, a non zero rw (a2) or an exhausted table (func_004e7a00) print (0x0075B5E8 / B610 / B638 through func_004e7458(0, text)) and give 0. The record
 * func_004e8b98(name, &rec) {descriptor, size}: with a zero size the file is opened here (func_004e82a0 / func_004cd378(0x0075B668, name), size by htCiGetFileSize, a 0 size gives 0, the
 * descriptor func_004e4928(0x00734240, *0x00734380) with marks 0x4900 / 0x4901; a negative one prints 0x0075B690 and frees the handle with func_004e7a38); otherwise the record is used
 * and +1 is set. The handle gets the size at +8 (64 bit), +0x10 = (size + 0x7FF) >> 11, the counters at +0x14, +0x1C, +0x20, +0x24 cleared, state 0, used 1. */
uint8_t *htCiOpen(const char *a0, void *a1, int32_t a2)
{
    uint32_t st[2] = {0, 0};
    uint8_t *h;
    int64_t size;
    int64_t t;
    const char *msg;

    if (a0 == 0) {
        msg = P4_STR(0x0075B5E8);
    } else if (a2 != 0) {
        msg = P4_STR(0x0075B610);
    } else {
        h = func_004e7a00();
        if (h != 0) {
            func_004e8b98(a0, st);
            if (st[1] == 0) {
                func_004e82a0(a0, (void *)P4_G2H(0x00734240));
                func_004cd378(P4_STR(0x0075B668), a0);
                size = func_004e77c0(a0);
                *(int64_t *)(h + 8) = size;
                if (size == 0) {
                    return 0;
                }
                func_004e4618(0x4900);
                *(int32_t *)(h + 0x18) = func_004e4928((void *)P4_G2H(0x00734240), (int32_t)P4_GW(0x00734380));
                func_004e4618(0x4901);
                if (*(int32_t *)(h + 0x18) < 0) {
                    func_004e7458(0, P4_STR(0x0075B690));
                    func_004e7a38(h);
                    return 0;
                }
                h[1] = 0;
            } else {
                h[1] = 1;
                *(uint32_t *)(h + 0x18) = st[0];
                *(int64_t *)(h + 8) = (int32_t)st[1];
            }
            t = *(int64_t *)(h + 8);
            *(uint32_t *)(h + 0x1C) = 0;
            *(uint32_t *)(h + 0x20) = 0;
            t += 0x7FF;
            *(uint32_t *)(h + 0x24) = 0;
            t = (int64_t)((uint64_t)t << 21) >> 32;
            *(uint32_t *)(h + 0x10) = (uint32_t)t;
            *(uint32_t *)(h + 0x14) = 0;
            h[2] = 0;
            h[0] = 1;
            return h;
        }
        msg = P4_STR(0x0075B638);
    }
    func_004e7458(0, msg);
    return 0;
}

/* 0x004e7bd0 (htCiClose): a null handle does nothing; a handle in state >= 2 is stopped first (func_004e7ed0); without the cache flag (+1) the descriptor +0x18 is closed (func_004e4990,
 * marks 0x5000 / 0x5001, a negative result prints 0x0075B6B8 through func_004cd378); the used byte is cleared and the handle freed (func_004e7a38). */
void func_004e7bd0(uint8_t *h)
{
    int32_t r;

    if (h == 0) {
        return;
    }
    if (h[2] >= 2) {
        func_004e7ed0(h);
    }
    if (*(int8_t *)(h + 1) == 0) {
        func_004e4618(0x5000);
        r = func_004e4990(*(int32_t *)(h + 0x18));
        func_004e4618(0x5001);
        if (r < 0) {
            func_004cd378(P4_STR(0x0075B6B8));
        }
    }
    h[0] = 0;
    func_004e7a38(h);
}

/* 0x004e7c70 (htCiSeek): mode 0 sets the position (+0x1C) to a1, mode 1 adds it, mode 2 sets it to the length (+0x10) plus a1; the position is clamped to [0, length], +0x28 cleared, and
 * returned. A null handle prints 0x0075B6D0 (func_004e7458(0, text)). */
int32_t func_004e7c70(uint8_t *h, int32_t a1, int32_t mode)
{
    int32_t len;
    int32_t pos;

    if (h == 0) {
        func_004e7458(0, P4_STR(0x0075B6D0));
        return 0;
    }
    len = *(int32_t *)(h + 0x10);
    if (mode == 0) {
        *(int32_t *)(h + 0x1C) = a1;
    } else if (mode == 2) {
        *(int32_t *)(h + 0x1C) = len + a1;
    } else if (mode == 1) {
        *(int32_t *)(h + 0x1C) = *(int32_t *)(h + 0x1C) + a1;
    }
    pos = *(int32_t *)(h + 0x1C);
    *(int32_t *)(h + 0x28) = 0;
    if (!(pos < len)) {
        pos = len;
    }
    if (pos < 0) {
        pos = 0;
    }
    *(int32_t *)(h + 0x1C) = pos;
    return pos;
}

/* 0x004e7d08 (htCiTell): the position (+0x1C); a null handle prints 0x0075B6D0 and gives 0. */
int32_t func_004e7d08(uint8_t *h)
{
    if (h == 0) {
        func_004e7458(0, P4_STR(0x0075B6D0));
        return 0;
    }
    return *(int32_t *)(h + 0x1C);
}

/* 0x004e7d40 (htCiReqRd): starts reading a1 sectors into a2. A null handle, a negative count or a null buffer print (0x0075B6D0 / B6E8 / B708 through func_004e7458(handle, text)) and give 0;
 * func_004e7488(handle) == 1 gives 0; a handle in state 2 gives 0; a count of 0 marks it done (+2 = 1). Otherwise a request (func_004e36e8, +0x2C) is made: the buffer is stored (+0x20),
 * the count limited to the sectors left (+0x10 - +0x1C, stored at +0x24), func_004e7540(handle) runs and func_004e3900(request, descriptor, position, bytes, buffer) starts it, the byte
 * count being min(size, count * 2048) (64 bit unsigned); a failure gives 0. With 0x00734380 == 1 the read is waited for (func_004e42a0, func_004e4190), the position advances and the
 * request is released (func_004e3770), +0x28 = count, state 1; otherwise +0x28 = 0 and the state is 2. +0x14 is cleared and the count is returned. */
int32_t htCiReqRd(uint8_t *h, int32_t a1, uint32_t a2)
{
    uint32_t req;
    int32_t remain;
    int32_t n;
    int32_t flag;
    uint64_t size;
    uint64_t bytes;
    const char *msg;

    if (h == 0) {
        msg = P4_STR(0x0075B6D0);
    } else if (a1 < 0) {
        msg = P4_STR(0x0075B6E8);
    } else if (a2 == 0) {
        msg = P4_STR(0x0075B708);
    } else {
        if (func_004e7488() == 1) {
            return 0;
        }
        if ((int8_t)h[2] == 2) {
            return 0;
        }
        if (a1 == 0) {
            h[2] = 1;
            return 0;
        }
        req = func_004e36e8();
        *(uint32_t *)(h + 0x2C) = req;
        if (req == 0) {
            return 0;
        }
        remain = *(int32_t *)(h + 0x10) - *(int32_t *)(h + 0x1C);
        size = *(uint64_t *)(h + 8);
        *(uint32_t *)(h + 0x20) = a2;
        n = a1;
        if (!(a1 < remain)) {
            n = remain;
        }
        *(int32_t *)(h + 0x24) = n;
        bytes = (uint64_t)(int64_t)(int32_t)((uint32_t)a1 << 11);
        if (size < bytes) {
            bytes = size;
        }
        func_004e7540(h);
        if (func_004e3900(*(uint32_t *)(h + 0x2C), *(int32_t *)(h + 0x18), *(int32_t *)(h + 0x1C), (int64_t)bytes, a2) == 0) {
            return 0;
        }
        flag = (int32_t)P4_GW(0x00734380);
        if (flag != 1) {
            h[2] = 2;
            *(uint32_t *)(h + 0x28) = 0;
            n = *(int32_t *)(h + 0x24);
        } else {
            func_004e42a0();
            func_004e4190();
            *(uint32_t *)(h + 0x1C) = *(uint32_t *)(h + 0x1C) + *(uint32_t *)(h + 0x24);
            func_004e3770(*(uint32_t *)(h + 0x2C));
            *(uint32_t *)(h + 0x2C) = 0;
            n = *(int32_t *)(h + 0x24);
            h[2] = flag;
            *(int32_t *)(h + 0x28) = n;
        }
        *(uint32_t *)(h + 0x14) = 0;
        return n;
    }
    func_004e7458(h, msg);
    return 0;
}

/* 0x004e7ed0 (htCiStopTr): stops a running transfer: a null handle prints 0x0075B730; a handle below state 2 is left alone; else the request (+0x2C) is aborted (func_004e3c10) and
 * func_004e4190, func_004e7668 and func_004e7fc8(handle) are repeated (func_004e7428 between rounds) until the state leaves 2 (more than 0x63FFF6 rounds print 0x0075B748). +0x28 and
 * the state are cleared. */
void func_004e7ed0(uint8_t *h)
{
    int32_t n = 0;

    if (h == 0) {
        func_004e7458(0, P4_STR(0x0075B730));
        return;
    }
    if (h[2] < 2) {
        return;
    }
    if (*(uint32_t *)(h + 0x2C) != 0) {
        func_004e3c10(*(uint32_t *)(h + 0x2C));
    }
    for (;;) {
        func_004e4190();
        func_004e7668();
        func_004e7fc8(h);
        if ((int8_t)h[2] != 2) {
            *(uint32_t *)(h + 0x28) = 0;
            break;
        }
        n++;
        if (0x63FFF6 < n) {
            func_004cd3f0(P4_STR(0x0075B748));
            *(uint32_t *)(h + 0x28) = 0;
            break;
        }
        func_004e7428();
    }
    h[2] = 0;
}

/* 0x004e7fc8 (htCiGetStat): the sign extended state byte; a null handle prints 0x0075B6D0 and gives 0. */
int32_t func_004e7fc8(uint8_t *h)
{
    if (h == 0) {
        func_004e7458(0, P4_STR(0x0075B6D0));
        return 0;
    }
    return (int8_t)h[2];
}

/* 0x004e8008 (htCiGetNumTr): sectors transferred (+0x28) * 2048 in state 1, else 0; a null handle prints 0x0075B6D0. */
int32_t func_004e8008(uint8_t *h)
{
    if (h == 0) {
        func_004e7458(0, P4_STR(0x0075B6D0));
        return 0;
    }
    if ((int8_t)h[2] != 1) {
        return 0;
    }
    return (int32_t)(*(uint32_t *)(h + 0x28) << 11);
}

/* 0x004e8058: tail jump to htCiGetFileSize. */
int64_t func_004e8058(const char *a0)
{
    return htCiGetFileSize(a0);
}

/* 0x004e8110: maps the error code a1 (when a0 is non null): 0xC8 func_004e8070(), 0xC9 func_004e8090(), 0xCA and 0xCC func_004e80c0(), 0xCB and 0xCD func_004e80e0(), 0x12B 1; other
 * codes and a null a0 give 0. */
int32_t func_004e8110(void *a0, int32_t a1)
{
    if (a0 == 0) {
        return 0;
    }
    switch (a1) {
    case 0xC8:
        return func_004e8070((uint8_t *)a0);
    case 0xC9:
        return func_004e8090((uint8_t *)a0);
    case 0xCA:
    case 0xCC:
        return func_004e80c0((uint8_t *)a0);
    case 0xCB:
    case 0xCD:
        return func_004e80e0((uint8_t *)a0);
    case 0x12B:
        return 1;
    }
    return 0;
}

/* ---- htCi helpers (0x004E7428..0x004E8B98) -------------------------------------------------------------------------------------------------- */

/* 0x004e7428: a short busy wait (about 256 rounds of nops). */
void func_004e7428(void)
{
    volatile int32_t v = 0x1FFF;

    v = v - 0x20;
    while (v >= 0) {
        v = v - 0x20;
    }
}

/* 0x004e7458: calls the error callback at 0x00733F5C (object 0x00733F60) as callback(object, a1, a0) when one is set. */
void func_004e7458(void *a0, const char *a1)
{
    void (*cb)(void *, const char *, void *) = (void (*)(void *, const char *, void *))p4_fn_g2h_sig(P4_GW(0x00733F5C), P4_SIG(3, 0, 1, 1, 1, 0, 0, 0));

    if (cb != 0) {
        cb((void *)P4_G2H(P4_GW(0x00733F60)), a1, a0);
    }
}

/* 0x004e7488: 1 when a handle of the table at 0x00733FD0 (13 entries of 0x30) is used (byte 0 is 1) and reading (state 2), else 0. */
int32_t func_004e7488(void)
{
    uint8_t *p = (uint8_t *)P4_G2H(0x00733FD0);
    int32_t i;

    for (i = 0; i < 13; i++, p += 0x30) {
        if (*(int8_t *)p == 1 && *(int8_t *)(p + 2) == 2) {
            return 1;
        }
    }
    return 0;
}

/* 0x004e7540: zero fills what a read of the handle a0 (buffer +0x20, sectors +0x24 from the position +0x1C) delivers beyond the end of the file (size +8, 64 bit): the
 * sectors * 2048 bytes at the buffer end minus the excess. Returns the excess (low word) when nothing was cleared. */
int32_t func_004e7540(uint8_t *a0)
{
    int32_t n = *(int32_t *)(a0 + 0x24);
    int32_t pos = *(int32_t *)(a0 + 0x1C);
    uint64_t size = *(uint64_t *)(a0 + 8);
    int32_t bytes = (int32_t)((uint32_t)n << 11);
    int64_t t = ((int64_t)pos + (int64_t)n) << 11;
    int32_t low = (int32_t)(t - (int64_t)size);

    if (size < (uint64_t)t) {
        func_0043f9c8((void *)P4_G2H(*(uint32_t *)(a0 + 0x20) + (uint32_t)bytes - (uint32_t)low), 0, (uint32_t)low);
        return 0;
    }
    return low;
}

/* 0x004e75a0: finishes the request (+0x2C) of the handle a0 when its sceCd status (func_004e39b0) has settled, under the request lock: status 0 clears the handle state, 3 counts the
 * sectors read (+0x28 = +0x24, the position +0x1C advances, state 1), 9 marks the state 3; the request is then released (func_004e3770) and +0x2C cleared. Other statuses leave it. */
void func_004e75a0(uint8_t *a0)
{
    uint32_t r;

    if (*(uint32_t *)(a0 + 0x2C) == 0) {
        return;
    }
    func_004f0638();
    r = (uint32_t)func_004e39b0(*(uint32_t *)(a0 + 0x2C));
    if (r < 10) {
        switch (r) {
        case 0:
            a0[2] = 0;
            goto release;
        case 3: {
            int32_t n = *(int32_t *)(a0 + 0x24);
            int32_t pos = *(int32_t *)(a0 + 0x1C);

            *(int32_t *)(a0 + 0x28) = n;
            a0[2] = 1;
            *(int32_t *)(a0 + 0x1C) = pos + n;
            goto release;
        }
        case 9:
            a0[2] = 3;
          release:
            func_004e3770(*(uint32_t *)(a0 + 0x2C));
            *(uint32_t *)(a0 + 0x2C) = 0;
            break;
        default:
            break;
        }
    }
    func_004f06b0();
}

/* 0x004e7a00: the last free (used byte 0) handle of the htCi table at 0x00733FD0 (13 entries of 0x30), or 0. */
uint8_t *func_004e7a00(void)
{
    uint8_t *p = (uint8_t *)P4_G2H(0x00733FD0);
    uint8_t *r = 0;
    int32_t i;

    for (i = 0xC; i >= 0; i--, p += 0x30) {
        if (*(int8_t *)p == 0) {
            r = p;
        }
    }
    return r;
}

/* 0x004e7a38: clears an htCi handle (0x30 bytes). */
void func_004e7a38(uint8_t *h)
{
    func_0043f9c8(h, 0, 0x30);
}

/* 0x004e8070: the upper word of min(file size (+8, 64 bit), sectors transferred (+0x28) * 2048 as a 64 bit value). */
int32_t func_004e8070(uint8_t *h)
{
    uint64_t size = *(uint64_t *)(h + 8);
    uint64_t b = (uint64_t)((int64_t)*(int32_t *)(h + 0x28) << 11);

    if (size < b) {
        b = size;
    }
    return (int32_t)(b >> 32);
}

/* 0x004e8090: the lower word of the same value. */
int32_t func_004e8090(uint8_t *h)
{
    uint64_t size = *(uint64_t *)(h + 8);
    uint64_t b = (uint64_t)((int64_t)*(int32_t *)(h + 0x28) << 11);

    if (size < b) {
        b = size;
    }
    return (int32_t)(uint32_t)b;
}

/* 0x004e80e0: the low word of func_004e8058(a0), explicitly sign-extended by retail. */
int32_t func_004e80e0(uint8_t *a0)
{
    return (int32_t)func_004e8058((const char *)a0);
}

/* 0x004e82a0: builds the host path of the name a0 in a1: func_004e81d8 decides whether the base directory is used (else the text at 0x00734490 is the start) and
 * func_004e8248 whether the name is relative; then the base directory (0x00734388) followed by a "/" (0x0075B770) unless it already ends with a separator or the name
 * starts with one, and the name; finally every backslash becomes a slash. */
void func_004e82a0(const char *a0, char *a1)
{
    const char *base = (const char *)P4_G2H(0x00734388);
    size_t n;
    size_t i;
    int8_t c;

    a1[0] = 0;
    if (func_004e81d8(a0) == 0) {
        strcpy(a1, (const char *)P4_G2H(0x00734490));
    }
    if (func_004e8248(a1) == 1) {
        strcat(a1, base);
        n = strlen(base);
        c = (int8_t)base[n - 1];
        if (c != '\\' && c != '/' && c != 0) {
            if ((int8_t)a0[0] != '\\' && (int8_t)a0[0] != '/') {
                strcat(a1, P4_STR(0x0075B770));
            }
        }
    }
    strcat(a1, a0);
    n = strlen(a1);
    for (i = 0; (int32_t)i < (int32_t)n; i++) {
        if (a1[i] == '\\') {
            a1[i] = '/';
        }
    }
}

/* 0x004e8b98: the file record of the name a0 into rec[0] / rec[1] from the list 0x00734370 {head, tail}: cleared, then func_004e8aa8(rec, name, head, tail) when the list is set. */
void func_004e8b98(const char *a0, uint32_t *rec)
{
    uint32_t head;

    rec[0] = 0;
    rec[1] = 0;
    head = P4_GW(0x00734370);
    if (head != 0) {
        func_004e8aa8(rec, a0, (uint8_t *)P4_G2H(head), (int32_t)P4_GW(0x00734374));
    }
}

/* 0x004cd378: the ADX debug printf of the second kind: unless the word 0x007212d0 is non zero, formats with vsprintf into a 128 byte buffer and prints it. */
void func_004cd378(const char *fmt, ...)
{
    char buf[128];
    va_list ap;

    if (P4_GW(0x007212D0) == 0) {
        va_start(ap, fmt);
        func_00446ed8(buf, fmt, ap);
        va_end(ap);
        func_004244c8(buf);
    }
}

/* 0x004e81d8: 1 when one of the first 16 characters of the name a0 is a ':' (a device prefix), else 0. */
int32_t func_004e81d8(const char *a0)
{
    size_t n = strlen(a0);
    uint32_t lim = (n < 0x11) ? (uint32_t)n : 0x10;
    uint32_t i;

    if (lim == 0) {
        return 0;
    }
    for (i = 0; i < lim; i++) {
        if ((int8_t)a0[i] == ':') {
            return 1;
        }
    }
    return 0;
}

/* 0x004e8248: 1 when the name a0 starts with "host" (the text at 0x0075B768), compared on its first 4 characters. */
int32_t func_004e8248(const char *a0)
{
    char loc[8] = {0};

    memcpy(loc, a0, 4);
    loc[5] = 0;
    return func_004426e8(loc, P4_STR(0x0075B768)) == 0;
}

/* 0x004e8aa8: as func_004e6cc0 for the host file list (words 0x00734378 / 0x0073437C scale the index, entries compared with func_004e8428). */
void func_004e8aa8(uint32_t *a0, const char *a1, uint8_t *a2, int32_t a3)
{
    int32_t i = 0;
    uint8_t *w = a2;
    uint32_t *g = (uint32_t *)P4_G2H(0x00734370);

    if (a3 > 0) {
        do {
            uint32_t stride = g[3];
            uint32_t off = g[2];
            uint32_t idx = stride * (uint32_t)i + (uint32_t)i;

            i++;
            if (func_004e8428(a1, (const char *)(a2 + ((off << 3) + idx))) == 0) {
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

/* 0x004e8428: the file name compare of the host list: as func_004e6778 with the character compare func_004e83d0. */
int32_t func_004e8428(const char *a0, const char *a1)
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
            if (func_004e83d0(c0, c1) != 0) {
                return -1;
            }
        } while (i < n0);
    }
    return 0;
}

/* 0x004e83b8: the (signed byte) character c with a backslash turned into '/'. */
int32_t func_004e83b8(int32_t c)
{
    int32_t v = (int8_t)c;

    return (v ^ 0x5C) != 0 ? v : 0x2F;
}

/* 0x004e83d0: -1 when the characters a and b differ in the form of func_004e83b8, else 0. */
int32_t func_004e83d0(int32_t a, int32_t b)
{
    int32_t x = func_004e83b8((int8_t)a);
    int32_t y = func_004e83b8((int8_t)b);

    return (x ^ y) != 0 ? -1 : 0;
}
