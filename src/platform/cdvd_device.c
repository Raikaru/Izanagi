/* sdkCdvd device methods that have no `// FUN_` marker in the decomp (the gap before func_003de2c0).
 *
 * func_003de280 is the device's "slot i" method (vtable offset 0x14, called by func_003dd290): the
 * device object has its channel-record count at +4 and the array of 0x70-byte records at +0x58
 * (func_003dea20); the method returns the address of record `i`, or NULL when i is out of range
 * (unsigned compare, so a negative index fails too).
 *
 * Direct transliteration of the retail code. */
#include <stdint.h>

#include "p4pc/gmem.h"

void *func_003de280(uint8_t *dev, uint32_t i)
{
    if (i >= *(uint32_t *)(dev + 4))
        return 0;
    return (uint8_t *)P4_G2H(*(uint32_t *)(dev + 0x58)) + i * 0x70;
}

/* func_003dd620 (retail 0x003dd620, also in the gap): sets a device's name. `dev` is the device
 * object (name pointer at +0xC), `name` the new name. Fails with 7 when func_003ddc20 finds a
 * registered device that already has this name; otherwise frees the old name through the RenderWare
 * free slot (0x008873EC), measures the new one with the strlen slot (0x008873D4), allocates
 * len + 1 bytes (malloc slot 0x008873E8, hint 0x000401BE), copies the name with the strcpy slot
 * (0x008873AC) and returns 1.
 *
 * Direct transliteration of the retail code. */
extern void *p4cdvd_find_device(const char *name) __asm__("func_003ddc20");

typedef void (*P4CdFreeFn)(void *p);
typedef int (*P4CdStrlenFn)(const char *s);
typedef void *(*P4CdMallocFn)(unsigned size, unsigned flags);
typedef void (*P4CdStrcpyFn)(char *dst, const char *src);

int func_003dd620(uint8_t *dev, const char *name)
{
    uint32_t old = *(uint32_t *)(dev + 0xC);
    char *copy;

    if (p4cdvd_find_device(name) != 0)
        return 7;
    if (old != 0)
        ((P4CdFreeFn)p4_fn_g2h_sig(P4_G32(0x008873ECu), P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(P4_G2H(old));
    copy = (char *)((P4CdMallocFn)p4_fn_g2h_sig(P4_G32(0x008873E8u), P4_SIG(2, 1, 0, 0, 0, 0, 0, 0)))(
        (unsigned)((P4CdStrlenFn)p4_fn_g2h_sig(P4_G32(0x008873D4u), P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(name) + 1, 0x401BE);
    *(uint32_t *)(dev + 0xC) = P4_H2G(copy);
    ((P4CdStrcpyFn)p4_fn_g2h_sig(P4_G32(0x008873ACu), P4_SIG(2, 0, 1, 1, 0, 0, 0, 0)))(copy, name);
    return 1;
}

/* func_003de2f0 (452 bytes): opens a file for the sdkCdvd device method table (no marker in the decomp: it follows the func_003de2d0 thunk in the same asm unit, so it lives here). arg0 = the device, arg1 = the
 * file record, arg2 = the name, arg3 = the open mode bits (1 read, 2 write, 4 create, 8 keep the position, 0x10 reopen). The device method at +0x1C is called first (its result is the
 * prefix argument), func_003dd390(arg2) gives the name part and func_003dd430(buf, 0x100, prefix, name, '/') builds the path (0: returns 5). The open flags are 1 / 2 from bits 1 / 2 (3 when both), 0x200 from bit 4
 * and 0x400 from bit 2 without bit 8. arg1[+0x30] = 0 and func_00426cf0(path, flags) gives the handle (arg1[+0x60]; negative: returns 3). The size, the sign-extended result of
 * func_004270f8(handle, 0, 2), goes to the 64-bit word at arg1 + 0; the position (arg1 + 0x10) is that size with bit 8, else the file is rewound (func_004270f8(handle, 0, 0)) and it is 0.
 * Bit 0x10 closes the handle (func_00426f80) and reopens the path with flags | 0x8000 (failure returns 3; success sets arg1[+0x30] = 1, arg1[+0x38] = 3). arg1[+0x64] = arg1[+0x6C];
 * when arg1[+0x30] is 0, arg1[+0x38] = 2. arg1[+0x3C] = 1 and 1 is returned. (The port's open takes a mode argument the PS2 call does not have; 0644 is passed.) */

extern uint8_t *func_003dd390(uint8_t *s);
extern uint8_t *func_003dd430(uint8_t *d, int32_t n, uint8_t *a, uint8_t *b, int32_t c);
extern int func_00426cf0(const char *name, int flags, int mode);
extern int func_00426f80(int fd);
extern int func_004270f8(int fd, int offset, int whence);

typedef uint8_t *(*P4DevMethod_003de2f0)(uint8_t *dev);   /* the device's path prefix: a host pointer */

int32_t func_003de2f0(uint8_t *arg0, uint8_t *arg1, uint32_t arg2, uint32_t arg3)
{
    uint8_t buf[0x100];
    uint32_t oflags = 0;
    uint8_t *prefix;
    uint32_t name;
    int32_t r;
    uint32_t i;

    for (i = 0; i < 0x100; i++) {
        buf[i] = 0;
    }
    prefix = ((P4DevMethod_003de2f0)p4_fn_g2h_sig(*(uint32_t *)(arg0 + 0x1C), P4_SIG(1, 1, 1, 0, 0, 0, 0, 0)))(arg0);
    name = P4_H2G(func_003dd390((uint8_t *)P4_G2H(arg2)));
    if (func_003dd430(buf, 0x100, prefix, (uint8_t *)P4_G2H(name), 0x2F) == 0) {
        return 5;
    }
    if ((arg3 & 1) != 0) {
        oflags |= 1;
    }
    if ((arg3 & 2) != 0) {
        oflags |= 2;
    }
    if ((arg3 & 4) != 0) {
        oflags |= 0x200;
    }
    if ((arg3 & 2) != 0 && (arg3 & 8) == 0) {
        oflags |= 0x400;
    }
    *(uint32_t *)(arg1 + 0x30) = 0;
    *(int32_t *)(arg1 + 0x60) = func_00426cf0((const char *)buf, oflags, 0x1A4);
    if (*(int32_t *)(arg1 + 0x60) < 0) {
        return 3;
    }
    r = func_004270f8(*(int32_t *)(arg1 + 0x60), 0, 2);
    *(int64_t *)arg1 = (int64_t)r;
    if ((arg3 & 8) != 0) {
        *(int64_t *)(arg1 + 0x10) = *(int64_t *)arg1;
    } else {
        func_004270f8(*(int32_t *)(arg1 + 0x60), 0, 0);
        *(int64_t *)(arg1 + 0x10) = 0;
    }
    if ((arg3 & 0x10) != 0) {
        func_00426f80(*(int32_t *)(arg1 + 0x60));
        *(int32_t *)(arg1 + 0x60) = func_00426cf0((const char *)buf, oflags | 0x8000, 0x1A4);
        if (*(int32_t *)(arg1 + 0x60) < 0) {
            return 3;
        }
        *(uint32_t *)(arg1 + 0x30) = 1;
        *(uint32_t *)(arg1 + 0x38) = 3;
    }
    *(uint32_t *)(arg1 + 0x64) = *(uint32_t *)(arg1 + 0x6C);
    if (*(uint32_t *)(arg1 + 0x30) == 0) {
        *(uint32_t *)(arg1 + 0x38) = 2;
    }
    *(uint32_t *)(arg1 + 0x3C) = 1;
    return 1;
}
