/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI "mfCi" memory file device (libadxe mfci), translated from the retail asm 0x004E9420..0x004EA690.
 *
 * Handle (0x38 bytes, 40 in the table at 0x00738D68): +0 used, +1 state (0 idle, 1 done, 2 reading), +4 sector size, +8 file size, +0xC length in sectors,
 * +0x10 position, +0x14 bytes transferred, +0x18 sectors of the running read, +0x1C name (copied with func_00442830), +0x30 offset, +0x34 byte count.
 * The function at 0x004e9420..0x004e96d0 read a second object (+0x20 ring base, +0x24 count, a 16 entry table of 0x20 bytes at +0x38).
 * Every function follows the retail asm call for call; the callees are the retail ones (func_004e9e20 / func_004e9e38 lock pair, func_004e9e68 error print). */
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "p4pc/ps2_libc.h"

#define P4_GW(a) (*(volatile uint32_t *)P4_G2H(a))
#define P4_STR(a) ((const char *)P4_G2H(a))

extern void func_004e9878(const char *fmt, ...);
extern void func_004e9e68(void *obj, const char *msg);
extern void func_004e9e20(void);
extern void func_004e9e38(void);
extern void func_004e9fa8(uint8_t *h);
extern uint8_t *func_004ea040(void);
extern int32_t func_004ea0b8(uint8_t *h);
extern void func_004ea098(uint8_t *h);
extern void func_004ea490(uint8_t *h);
extern int32_t func_004ea5d0(uint8_t *h);
extern int32_t func_004ea020(uint8_t *h);
extern int func_00446ed8(char *buf, const char *fmt, va_list ap);
extern uint64_t func_004e9e98(const char *s, uint32_t *end, int32_t base);
extern void func_004f0608(void);
extern void func_004f0620(void);
extern uint8_t *func_004e9eb0(const char *name, uint32_t *size);
extern void *func_0043f810(void *d, const void *s, uint32_t n);
extern void *func_0043f9c8(void *d, int32_t c, uint32_t n);

/* 0x004e9420: the sign extended state byte (+1) of the object a0; a null object prints 0x0075BAA8 (func_004e9878) and gives -1. */
int32_t func_004e9420(uint8_t *a0)
{
    if (a0 == 0) {
        func_004e9878(P4_STR(0x0075BAA8));
        return -1;
    }
    return (int8_t)a0[1];
}

/* 0x004e9480: the first word of entry (a0[8] + a1) mod 16 of the ring table at a0 + 0x38 (entries of 0x20 bytes); a null object prints 0x0075BB08, an index outside 0..a0[9] 0x0075BB38;
 * both give -1. */
int32_t func_004e9480(uint8_t *a0, int32_t a1)
{
    int32_t v;

    if (a0 == 0) {
        func_004e9878(P4_STR(0x0075BB08));
        return -1;
    }
    if (a1 < 0 || !(a1 < *(int32_t *)(a0 + 0x24))) {
        func_004e9878(P4_STR(0x0075BB38));
        return -1;
    }
    v = *(int32_t *)(a0 + 0x20) + a1;
    return (int32_t)*(uint32_t *)(a0 + 0x38 + (v % 16) * 0x20);
}

/* 0x004e9510: the word at +4 of the entry of the table at a0 + 0x38 (16 entries of 0x20 bytes) whose first word equals a1; a null object prints 0x0075BB60, no entry 0x0075BB90; both give 0. */
int32_t func_004e9510(uint8_t *a0, int32_t a1)
{
    uint32_t *p;
    int32_t i;

    if (a0 == 0) {
        func_004e9878(P4_STR(0x0075BB60));
        return 0;
    }
    p = (uint32_t *)(a0 + 0x38);
    i = 0;
    if ((int32_t)*p != a1) {
        i = 1;
        for (;;) {
            if (!(i < 16)) {
                break;
            }
            p += 8;
            if ((int32_t)*p == a1) {
                break;
            }
            i++;
        }
    }
    if (i == 16) {
        func_004e9878(P4_STR(0x0075BB90));
        return 0;
    }
    return (int32_t)*(uint32_t *)(a0 + (i << 5) + 0x3C);
}

/* 0x004e9640: as func_004e9510 but gives the word at +0x1C of the entry; the texts are 0x0075BC20 (null object) and 0x0075BC50 (no entry). */
int32_t func_004e9640(uint8_t *a0, int32_t a1)
{
    uint32_t *p;
    int32_t i;

    if (a0 == 0) {
        func_004e9878(P4_STR(0x0075BC20));
        return 0;
    }
    p = (uint32_t *)(a0 + 0x38);
    i = 0;
    if ((int32_t)*p != a1) {
        i = 1;
        for (;;) {
            if (!(i < 16)) {
                break;
            }
            p += 8;
            if ((int32_t)*p == a1) {
                break;
            }
            i++;
        }
    }
    if (i == 16) {
        func_004e9878(P4_STR(0x0075BC50));
        return 0;
    }
    return (int32_t)*(uint32_t *)(a0 + (i << 5) + 0x54);
}

/* 0x004ea128 (mfCiOpen): a null name, a non zero rw (a2) or an exhausted table (func_004ea040) print their text (0x0075BE80 / BEA8 / BED0 through func_004e9e68(0, text)) and give 0;
 * otherwise the name is copied to +0x1C (func_00442830), func_004ea0b8(handle) sets the handle up and it is returned. */
uint8_t *mfCiOpen(const char *a0, void *a1, int32_t a2)
{
    uint8_t *h;
    const char *msg;

    if (a0 == 0) {
        msg = P4_STR(0x0075BE80);
    } else if (a2 != 0) {
        msg = P4_STR(0x0075BEA8);
    } else {
        h = func_004ea040();
        if (h != 0) {
            strcpy((char *)(h + 0x1C), a0);
            func_004ea0b8(h);
            return h;
        }
        msg = P4_STR(0x0075BED0);
    }
    func_004e9e68(0, msg);
    return 0;
}

/* 0x004ea1c8 (mfCiClose): a null handle does nothing; func_004ea490(handle) stops it and a used handle (byte 1) is cleared and released (func_004ea098). */
void mfCiClose(uint8_t *h)
{
    if (h == 0) {
        return;
    }
    func_004ea490(h);
    if (*(int8_t *)h == 1) {
        h[0] = 0;
        func_004ea098(h);
    }
}

/* 0x004ea210: seek under the lock: mode 0 sets the position (+0x10) to a1, mode 1 adds it, mode 2 sets it to the length (+0xC) plus a1; clamped to [0, length], returned.
 * A null handle prints 0x0075BF00. */
int32_t func_004ea210(uint8_t *h, int32_t a1, int32_t mode)
{
    int32_t len;
    int32_t pos;

    if (h == 0) {
        func_004e9e68(0, P4_STR(0x0075BF00));
        return 0;
    }
    func_004e9e20();
    len = *(int32_t *)(h + 0xC);
    if (mode == 0) {
        *(int32_t *)(h + 0x10) = a1;
    } else if (mode == 2) {
        *(int32_t *)(h + 0x10) = len + a1;
    } else if (mode == 1) {
        *(int32_t *)(h + 0x10) = *(int32_t *)(h + 0x10) + a1;
    }
    pos = *(int32_t *)(h + 0x10);
    if (!(pos < len)) {
        pos = len;
    }
    if (pos < 0) {
        pos = 0;
    }
    *(int32_t *)(h + 0x10) = pos;
    func_004e9e38();
    return *(int32_t *)(h + 0x10);
}

/* 0x004ea2d0 (dvCiTell): the position (+0x10); a null handle prints 0x0075BF20 and gives 0. */
int32_t dvCiTell(uint8_t *h)
{
    if (h == 0) {
        func_004e9e68(0, P4_STR(0x0075BF20));
        return 0;
    }
    return *(int32_t *)(h + 0x10);
}

/* 0x004ea308 (mfCiReqRd): reads a1 sectors into a2. A null handle, a negative count or a null buffer print (0x0075BF40 / BF60 / BF80 through func_004e9e68(handle, text)) and give 0; a
 * count of 0 marks the handle done (+1 = 1); a handle in state 2 gives 0. Otherwise, under the lock, +0x14 = 0, the count is limited to the sectors left (+0xC - +0x10, stored at +0x18), the
 * byte offset +0x30 = position * sector size and the byte count +0x34 = count * sector size are made (a zero byte count marks the handle done and gives 0), the state becomes 2 and the
 * file memory is fetched (func_004e9eb0(handle + 0x1C, &size)); the bytes available (size - offset, at most the byte count) are copied to a2 (func_0043f810) and the rest of the byte count
 * zeroed (func_0043f9c8). Then the position advances, +0x14 = count * sector size, the state is 1 and the count is returned. */
int32_t mfCiReqRd(uint8_t *h, int32_t a1, uint8_t *a2)
{
    uint32_t size = 0;
    uint8_t *mem;
    int32_t pos;
    int32_t rem;
    int32_t n;
    int32_t ssz;
    int32_t off;
    int32_t bytes;
    int32_t avail;
    const char *msg;

    if (h == 0) {
        msg = P4_STR(0x0075BF40);
    } else if (a1 < 0) {
        msg = P4_STR(0x0075BF60);
    } else if (a2 == 0) {
        msg = P4_STR(0x0075BF80);
    } else {
        if (a1 == 0) {
            h[1] = 1;
            return 0;
        }
        if ((int8_t)h[1] == 2) {
            return 0;
        }
        func_004e9e20();
        *(uint32_t *)(h + 0x14) = 0;
        pos = *(int32_t *)(h + 0x10);
        rem = *(int32_t *)(h + 0xC) - pos;
        ssz = *(int32_t *)(h + 4);
        n = a1;
        if (a1 < rem) {
            n = a1;
        } else {
            n = rem;
        }
        off = pos * ssz;
        *(int32_t *)(h + 0x18) = n;
        bytes = n * ssz;
        if (bytes == 0) {
            h[1] = 1;
            func_004e9e38();
            return 0;
        }
        *(int32_t *)(h + 0x30) = off;
        *(int32_t *)(h + 0x34) = bytes;
        h[1] = 2;
        mem = func_004e9eb0((const char *)(h + 0x1C), &size);
        avail = (int32_t)size - *(int32_t *)(h + 0x30);
        if (avail > *(int32_t *)(h + 0x34)) {
            avail = *(int32_t *)(h + 0x34);
        }
        func_004e9e38();
        func_0043f810(a2, mem + *(int32_t *)(h + 0x30), *(uint32_t *)(h + 0x34));
        func_0043f9c8(a2 + avail, 0, *(uint32_t *)(h + 0x34) - avail);
        func_004e9e20();
        n = *(int32_t *)(h + 0x18);
        ssz = *(int32_t *)(h + 4);
        pos = *(int32_t *)(h + 0x10);
        h[1] = 1;
        *(int32_t *)(h + 0x10) = pos + n;
        *(int32_t *)(h + 0x14) = n * ssz;
        func_004e9e38();
        return *(int32_t *)(h + 0x18);
    }
    func_004e9e68(h, msg);
    return 0;
}

/* 0x004ea490: stops the handle under the lock (func_004e9e20 ... func_004e9e38), +1 = 0; a null handle prints 0x0075BFA8. */
void func_004ea490(uint8_t *h)
{
    if (h == 0) {
        func_004e9e68(0, P4_STR(0x0075BFA8));
        return;
    }
    func_004e9e20();
    h[1] = 0;
    func_004e9e38();
}

/* 0x004ea4e0: the sign extended state byte (+1); a null handle prints 0x0075BFA8 and gives 0. */
int32_t func_004ea4e0(uint8_t *h)
{
    if (h == 0) {
        func_004e9e68(0, P4_STR(0x0075BFA8));
        return 0;
    }
    return (int8_t)h[1];
}

/* 0x004ea518: the sector size (+4); a null handle prints 0x0075BFC0 and gives 0. */
int32_t func_004ea518(uint8_t *h)
{
    if (h == 0) {
        func_004e9e68(0, P4_STR(0x0075BFC0));
        return 0;
    }
    return *(int32_t *)(h + 4);
}

/* 0x004ea550: sets the sector size (+4) to a1 and rescales the handle: the length (+0xC) becomes (file size (+8) + a1 - 1) / a1, the position (+0x10) becomes
 * position * old sector size / a1 and +0x14 becomes the sectors of the running read (+0x18) * a1. A zero sector size is the divide by zero trap of the retail code (break 7).
 * A null handle prints 0x0075BFD8. */
void func_004ea550(uint8_t *h, int32_t a1)
{
    int32_t old;
    int32_t t;

    if (h == 0) {
        func_004e9e68(0, P4_STR(0x0075BFD8));
        return;
    }
    old = *(int32_t *)(h + 4);
    t = *(int32_t *)(h + 8) + a1 - 1;
    *(int32_t *)(h + 4) = a1;
    if (a1 == 0) {
        __builtin_trap();
    }
    *(int32_t *)(h + 0xC) = t / a1;
    t = *(int32_t *)(h + 0x10) * old;
    *(int32_t *)(h + 0x10) = t / a1;
    *(int32_t *)(h + 0x14) = *(int32_t *)(h + 0x18) * a1;
}

/* 0x004ea5d0: the bytes transferred (+0x14); a null handle prints 0x0075BFA8 and gives 0. */
int32_t func_004ea5d0(uint8_t *h)
{
    if (h == 0) {
        func_004e9e68(0, P4_STR(0x0075BFA8));
        return 0;
    }
    return *(int32_t *)(h + 0x14);
}

/* 0x004ea690 (mfCiOptFn1): maps the option code a1 (a0 non null): 0xC9 func_004ea5d0(a0), 0xCB and 0xCD func_004ea020(); other codes and a null a0 give 0. */
int32_t mfCiOptFn1(uint8_t *a0, int32_t a1)
{
    if (a0 == 0) {
        return 0;
    }
    if (a1 == 0xCB || a1 == 0xCD) {
        return func_004ea020(a0);
    }
    if (a1 == 0xC9) {
        return func_004ea5d0(a0);
    }
    return 0;
}

/* ---- mfCi helpers (0x004E9878..0x004EA0B8) -------------------------------------------------------------------------------------------------- */

/* 0x004e9878: the mfCi debug printf: vsprintf(0x007344D0, fmt, args), then the callback word 0x007344C8 (object 0x007344CC) is called as callback(object, text) when set. */
void func_004e9878(const char *fmt, ...)
{
    va_list ap;
    void (*cb)(void *, const char *);

    va_start(ap, fmt);
    func_00446ed8((char *)P4_G2H(0x007344D0), fmt, ap);
    va_end(ap);
    cb = (void (*)(void *, const char *))p4_fn_g2h_sig(P4_GW(0x007344C8), P4_SIG(2, 0, 1, 1, 0, 0, 0, 0));
    if (cb != 0) {
        cb((void *)P4_G2H(P4_GW(0x007344CC)), (const char *)P4_G2H(0x007344D0));
    }
}

/* 0x004e9e20: the mfCi lock (func_004f0608). */
void func_004e9e20(void)
{
    func_004f0608();
}

/* 0x004e9e38: the mfCi unlock (func_004f0620). */
void func_004e9e38(void)
{
    func_004f0620();
}

/* 0x004e9e68: calls the error callback at 0x00738CF4 (object 0x00738CF8) as callback(object, a1, a0) when one is set. */
void func_004e9e68(void *a0, const char *a1)
{
    void (*cb)(void *, const char *, void *) = (void (*)(void *, const char *, void *))p4_fn_g2h_sig(P4_GW(0x00738CF4), P4_SIG(3, 0, 1, 1, 1, 0, 0, 0));

    if (cb != 0) {
        cb((void *)P4_G2H(P4_GW(0x00738CF8)), a1, a0);
    }
}

/* 0x004e9eb0: parses the memory file name a0 ("<address>.<size>", 17 characters, both parts hex) and returns the address; the size goes to *a1 when a1 is set. A name of another length
 * (text 0x0075BE00) or without the '.' at position 8 (0x0075BE40) is reported through sprintf into 0x00739628 and func_004e9e68(0, text). */
uint8_t *func_004e9eb0(const char *a0, uint32_t *a1)
{
    char *buf = (char *)P4_G2H(0x00739628);
    uint32_t pg = P4_H2G(a0);
    const char *p;
    uint32_t addr;

    if (strlen(a0) != 0x11) {
        func_00442088(buf, P4_STR(0x0075BE00), a0);
        func_004e9e68(0, buf);
    }
    if ((int8_t)a0[8] != '.') {
        func_00442088(buf, P4_STR(0x0075BE40), a0);
        func_004e9e68(0, buf);
    }
    addr = (uint32_t)func_004e9e98(a0, &pg, 0x10);
    p = (const char *)P4_G2H(pg);
    if (*(int8_t *)p != 0) {
        p = p + 1;
    }
    if (a1 != 0) {
        pg = P4_H2G(p);
        *a1 = (uint32_t)func_004e9e98(p, &pg, 0x10);
    }
    return (uint8_t *)P4_G2H(addr);
}

/* 0x004ea040: the first free (used byte 0) handle of the mfCi table at 0x00738D68 (40 entries of 0x38), or 0. */
uint8_t *func_004ea040(void)
{
    uint8_t *p = (uint8_t *)P4_G2H(0x00738D68);
    int32_t i;

    for (i = 0; i < 0x28; i++, p += 0x38) {
        if (*(int8_t *)p == 0) {
            return p;
        }
    }
    return 0;
}

/* 0x004ea098: clears an mfCi handle (0x38 bytes). */
void func_004ea098(uint8_t *h)
{
    func_0043f9c8(h, 0, 0x38);
}

/* 0x004ea0b8: sets a freshly opened handle up: sector size +4 = 0x800, the file size +8 = func_004ea020(h + 0x1C), used byte 1, the length +0xC = (size + sector size - 1) / sector size
 * (divide by zero trap when the sector size read back is 0), position +0x10, +0x18, +0x14 and state +1 cleared. Returns the length. */
int32_t func_004ea0b8(uint8_t *h)
{
    int32_t size;
    int32_t ssz;

    *(int32_t *)(h + 4) = 0x800;
    size = func_004ea020(h + 0x1C);
    ssz = *(int32_t *)(h + 4);
    *(int32_t *)(h + 8) = size;
    if (ssz == 0) {
        __builtin_trap();
    }
    h[0] = 1;
    *(int32_t *)(h + 0x10) = 0;
    *(int32_t *)(h + 0x18) = 0;
    *(int32_t *)(h + 0x14) = 0;
    h[1] = 0;
    *(int32_t *)(h + 0xC) = (size + ssz - 1) / ssz;
    return *(int32_t *)(h + 0xC);
}

/* 0x004e9e98: tail-jumps to the unsigned-64 conversion without truncating. */
uint64_t func_004e9e98(const char *a0, uint32_t *a1, int32_t a2)
{
    return func_00444458(a0, a1, a2);
}
