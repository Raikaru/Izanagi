/* Retail CRI middleware leaf functions that exist in the decomp only as MWCC asm blocks in
 * units the build does not compile (the legacy src/cri/acssvr.c, mwsfdsst.c, mwsfdsvr.c,
 * sfd_tst.c, sfx_alp.c; the linked src/cri/re4 units call these by name). Each is written from
 * the retail instructions and sits under its own name, so the callers link to it. */
#include <stdint.h>

/* 0x004c0ab8: ACSS server work size. a0 is not read. It is a 32-bit argument: the callers (re4
 * adx_cs_8c78.c) pass the server pointer as a 4-byte int, and a uint64_t here would take two
 * argument words on i386 and shift p onto the wrong one. p points at the parameter block: [0] count,
 * [1] channels, [2] rate, [3] n3, [4] n4, [6] double-buffer flag, [7] mode A, [8] extra-buffer
 * flag, [9] mode B. 32-bit integer arithmetic, as retail. */
int32_t ACSSVR_CalcWorkCprm(uint32_t a0, int32_t *p)
{
    int32_t size, span, tmp, total;

    (void)a0;
    if (p[7] != 0) {
        if (p[9] != 0) {
            span = (((p[2] / 1000) * 25000) / 0x2c) * (p[0] + 1);
            tmp = span + 0x800;
            if (tmp < 0) {
                tmp = span + 0xfff;
            }
            size = ((tmp >> 0xb) * 0x800 + 0x85a4) * p[3] + 0x50;
        } else {
            span = ((p[1] * (p[2] / 1000) * 25000) / 0x2c) * (p[0] + 1);
            tmp = span + 0x800;
            if (tmp < 0) {
                tmp = span + 0xfff;
            }
            size = ((tmp >> 0xb) * 0x800 + p[1] * 0x40c0 + 0x24) * p[3] + 0x50;
        }
    } else if (p[9] != 0) {
        span = (((p[2] / 1000) * 25000) / 0x2c) * (p[0] + 1);
        tmp = span + 0x800;
        if (tmp < 0) {
            tmp = span + 0xfff;
        }
        size = ((tmp >> 0xb) * 0x800 + 0x24) * p[4] + 0x85cc;
    } else {
        span = ((p[1] * (p[2] / 1000) * 25000) / 0x2c) * (p[0] + 1);
        tmp = span + 0x800;
        if (tmp < 0) {
            tmp = span + 0xfff;
        }
        size = ((tmp >> 0xb) * 0x800 + 0x24) * p[4] + p[1] * 0x40c0 + 0x4c;
    }
    if (p[8] != 0) {
        size += 0x3000;
    }
    total = size + 0x44;
    if (p[6] != 0) {
        total = (size + 0x44) * 2;
    }
    total += 0x1d0;
    if (p[7] != 0) {
        total += p[3] * 4;
    }
    return total;
}

/* 0x00510630 and 0x00510660: ADX and AIX stream work sizes. The three outputs are the sizes;
 * the return value is their sum. A fourth argument some callers pass is not read. */
int32_t MWSST_CalcAdxWorkSize(int32_t *out0, int32_t *out1, uint32_t *out2)
{
    *out0 = 0x249e4;
    *out1 = 40000;
    *out2 = 0x800;
    return *out0 + *out1 + 0x800;
}

int32_t MWSST_CalcAixWorkSize(int32_t *out0, int32_t *out1, uint32_t *out2)
{
    *out0 = 0x6952c;
    *out1 = 81000;
    *out2 = 0x2000;
    return *out0 + *out1 + 0x2000;
}

/* 0x004f9a88: a counted busy loop of about 100 microseconds on the EE. The port has no device
 * to wait for; the loop keeps a compiler barrier so it is not removed. */
void RFTGT_Wait100MicroSec(void)
{
    uint32_t i;

    for (i = 1; i < 0x1d80; i++) {
        __asm__ volatile("" : : : "memory");
    }
}

/* 0x0052a688 and 0x0052a668: SFX alpha luminance parameters (+8, +0xc, +0x10) and the valid
 * flag (+4). The getter returns +8 in *out_c, +0xc in *out_a and +0x10 in *out_b. */
void SFXA_GetLumiPrm(int32_t obj, uint32_t *out_a, uint32_t *out_b, uint32_t *out_c)
{
    uint8_t *o = (uint8_t *)P4_G2H(obj);

    *out_c = *(uint32_t *)(o + 8);
    *out_a = *(uint32_t *)(o + 0xc);
    *out_b = *(uint32_t *)(o + 0x10);
}

void SFXA_SetLumiPrm(int32_t obj, uint32_t a, uint32_t b, uint32_t c)
{
    uint8_t *o = (uint8_t *)P4_G2H(obj);

    *(uint32_t *)(o + 0x10) = b;
    *(uint32_t *)(o + 8) = c;
    *(uint32_t *)(o + 0xc) = a;
    *(uint32_t *)(o + 4) = 1;
}

/* 0x00527960: whether word +0x1f3c of the 0x44-byte record `index` is non-zero. */
uint8_t func_00527960(int32_t base, int32_t index)
{
    return *(int32_t *)((uint8_t *)P4_G2H(base) + index * 0x44 + 0x1f3c) != 0;
}
