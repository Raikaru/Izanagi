/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* ACSFDR fader setters (retail 0x004bd9d0, 0x004bd9d8, 0x004bd9e8). Fader layout: see acs.c. */
#include <stdint.h>

#define HW32(p, off) (*(uint32_t *)((uint8_t *)(p) + (off)))

/* 0x004bd9d0: the fade curve byte (+0x18): 1 logarithmic, anything else power. */
void func_004bd9d0(uint8_t *f, int32_t curve)
{
    f[0x18] = (uint8_t)curve;
}

/* 0x004bd9d8: the fade-start hook: function +0x2c, argument +0x30, event +0x34 (func_004bdd48 calls it). */
void func_004bd9d8(uint8_t *f, uint32_t fn, uint32_t obj, uint32_t ev)
{
    HW32(f, 0x34) = ev;
    HW32(f, 0x2c) = fn;
    HW32(f, 0x30) = obj;
}

/* 0x004bd9e8: the fade-end hook: function +0x38, argument +0x3c, event +0x40 (func_004bdd78 calls it). */
void func_004bd9e8(uint8_t *f, uint32_t fn, uint32_t obj, uint32_t ev)
{
    HW32(f, 0x40) = ev;
    HW32(f, 0x38) = fn;
    HW32(f, 0x3c) = obj;
}
