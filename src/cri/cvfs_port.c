/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CVFS functions the re4 tree has no retail-addressed C for. 0x004e1a00 is cvFsDelDev. The device table is
 * at 0x00923440: 32 entries of 16 bytes, the device's interface pointer first and the 12-byte upper-case
 * name after it (re4/cri_cvfs.c cvfs_tbl). The retail function differs from the decomp's dead cvFsDelDev:
 * it matches on the first strlen(name) characters only, clears the first byte of the name (the interface
 * pointer stays) and stops at the first match.
 */
#include <stdint.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/ps2_libc.h"

extern void cvFsCallUsrErrFn(void *obj, const char *msg, void *hn);   /* 0x004e1578; retail's 0x004e15a8 passes a NULL handle */

/* 0x004e1a00: remove a device by name. NULL reports "cvFsDelDev #1:illegal device name" (0x00759e28). */
void func_004e1a00(const char *name)
{
    uint8_t *tbl = (uint8_t *)P4_G2H(0x00923444);
    uint32_t len;
    uint32_t i;

    if (name == 0) {
        cvFsCallUsrErrFn(0, (const char *)P4_G2H(0x00759e28), 0);
        return;
    }
    len = (uint32_t)strlen(name);
    for (i = 0; i < 0x20; i++) {
        if (func_00442c30(name, (const char *)(tbl + i * 0x10), len) == 0) {
            tbl[i * 0x10] = 0;
            return;
        }
    }
}

/* 004e3448..004e34dc: extended transfer count. Checked the native romwright
 * R5900 draft against the retail instructions. The option results contribute
 * their low 32 bits, high word first; the second call reloads the interface
 * and handle because the first callback may change them.
 */
uint64_t func_004e3448(void *object)
{
    typedef int32_t (*Option)(void *, int32_t, int32_t, int32_t);
    const uint32_t *words = object;
    uint32_t address, high, low;
    Option option;

    if (object == NULL) {
        cvFsCallUsrErrFn(NULL, P4_G2H(0x0075a9e0), NULL);
        return 0;
    }
    address = P4_G32(words[0] + 0x60);
    if (address == 0) {
        return 0;
    }
    option = (Option)p4_fn_g2h_sig(address, P4_SIG(4, 0, 1, 0, 0, 0, 0, 0));
    high = (uint32_t)option(P4_G2H(words[1]), 200, 0, 0);
    option = (Option)p4_fn_g2h_sig(P4_G32(words[0] + 0x60),
                                P4_SIG(4, 0, 1, 0, 0, 0, 0, 0));
    low = (uint32_t)option(P4_G2H(words[1]), 201, 0, 0);
    return ((uint64_t)high << 32) | low;
}
