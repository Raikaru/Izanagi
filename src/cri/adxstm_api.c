/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* ADXSTM and ADX-server entry points the re4 tree has no retail-addressed C for, translated from the
 * retail asm. Every public ADXSTM function is a wrapper that calls the ADX lock pair (0x004d18c8 and
 * 0x004d18d0, both empty) around an inner function; the re4 natives cover most inners under their public
 * names (config/native_names.txt binds the wrappers to them). The inners below have no native.
 *
 * ADXSTM object (96 bytes, decomp re4/adx_stmc.c): +1 status, +2 read-pending flag, +8 file handle, +0x10
 * file size, +0x14 sectors, +0x18 and +0x1c the request chunk limits, +0x2c sectors per request, +0x30
 * end-of-stream sector, +0x34 bytes read, +0x4c retry word, +0x58 position, +0x5c sector limit.
 */
#include <stdint.h>
#include "cri_mem.h"

#define W32(p, off) (*(int32_t *)((uint8_t *)(p) + (off)))
#define U32(p, off) (*(uint32_t *)((uint8_t *)(p) + (off)))

extern void func_004d18c8(void);                                     /* ADXSTM lock (empty) */
extern void func_004d18d0(void);                                     /* ADXSTM unlock (empty) */
extern void func_004c54a8(void);                                     /* SVM lock */
extern void func_004c54c0(void);                                     /* SVM unlock */
extern int32_t func_004e2360(const char *name);                      /* CVFS name query */

/* 0x004d20f0: start the stream without the lock: bytes read and the retry word cleared, status 3 (end)
 * when the range is empty, else 2 (exec); the read-pending flag, the chunk and the request flag (+0x47)
 * set up for the server. */
void func_004d20f0(uint8_t *stm)
{
    W32(stm, 0x34) = 0;
    W32(stm, 0x4c) = 0;
    stm[1] = (W32(stm, 0x14) == 0) ? 3 : 2;
    stm[2] = 0;
    W32(stm, 0x24) = 0;
    W32(stm, 0x28) = 0;
    stm[0x47] = 1;
}

/* 0x004d2160 / 0x004d2128: ADXSTM start under the SVM lock, with the read limit lifted (0xfffff
 * sectors); returns 1. */
int32_t func_004d2160(uint8_t *stm)
{
    func_004c54a8();
    func_004d20f0(stm);
    W32(stm, 0x5c) = 0xfffff;
    func_004c54c0();
    return 1;
}

int32_t func_004d2128(uint8_t *stm)
{
    int32_t r;

    func_004d18c8();
    r = func_004d2160(stm);
    func_004d18d0();
    return r;
}

/* 0x004d2bd8 / 0x004d2b80: the request chunk limits: +0x1c to *a, +0x18 to *b; returns 1. */
int32_t func_004d2bd8(uint8_t *stm, uint32_t *a, uint32_t *b)
{
    *a = U32(stm, 0x1c);
    *b = U32(stm, 0x18);
    return 1;
}

int32_t func_004d2b80(uint8_t *stm, uint32_t *a, uint32_t *b)
{
    int32_t r;

    func_004d18c8();
    r = func_004d2bd8(stm, a, b);
    func_004d18d0();
    return r;
}

/* 0x004d2ce0 / 0x004d2c98: sectors per request (+0x2c); returns 1. */
int32_t func_004d2ce0(uint8_t *stm, uint32_t nsct)
{
    U32(stm, 0x2c) = nsct;
    return 1;
}

int32_t func_004d2c98(uint8_t *stm, uint32_t nsct)
{
    int32_t r;

    func_004d18c8();
    r = func_004d2ce0(stm, nsct);
    func_004d18d0();
    return r;
}

/* 0x004d2d30 / 0x004d2cf8: the file size in bytes (+0x10). */
uint32_t func_004d2d30(uint8_t *stm)
{
    return U32(stm, 0x10);
}

uint32_t func_004d2cf8(uint8_t *stm)
{
    uint32_t r;

    func_004d18c8();
    r = func_004d2d30(stm);
    func_004d18d0();
    return r;
}

/* 0x004d2db0 / 0x004d2d78: a tail jump to the CVFS name query 0x004e2360 under the lock. */
int32_t func_004d2db0(const char *name)
{
    return func_004e2360(name);
}

int32_t func_004d2d78(const char *name)
{
    int32_t r;

    func_004d18c8();
    r = func_004d2db0(name);
    func_004d18d0();
    return r;
}

/* 0x004d31e8: the signed byte at +2 of an ADXT handle (the input kind) through 0x004d3220, under the lock. */
extern int32_t func_004d3220(uint8_t *adxt);

int32_t func_004d31e8(uint8_t *adxt)
{
    int32_t r;

    func_004d18c8();
    r = func_004d3220(adxt);
    func_004d18d0();
    return r;
}

/* 0x004c6a70: ADX error callback entry: the function and object are kept at 0x00715ca0/0x00715ca4 and
 * handed on to the SVM error-callback setter. */
extern void func_004f0e80(void (*func)(void *obj, const char *msg), void *obj);

void func_004c6a70(uint32_t func, uint32_t obj)
{
    *(volatile uint32_t *)P4_G2H(0x00715ca0) = func;
    *(volatile uint32_t *)P4_G2H(0x00715ca4) = obj;
    func_004f0e80((void (*)(void *, const char *))p4_fn_g2h_sig(func, P4_SIG(2, 0, 1, 1, 0, 0, 0, 0)), (void *)P4_G2H(obj));   /* a code address and its argument word */
}

extern void ADXSTM_BindFileNw(void *stm, const char *name, void *dir, int32_t start, int32_t length);
extern void ADXT_ExecFsSvr(void);
extern void ADXSTMF_ExecHndl(void *stm);
extern int32_t func_004f1278(int32_t *flag);
extern int32_t adxstmf_execsvr_flg;

/* 004d1ec8 / 004d1e58: blocking bind. Publication is driven by the actual
 * file-server order, never by a synthetic ADXF alarm. */
void func_004d1ec8(void *stm, const char *name, void *dir, int32_t start, int32_t length)
{
    ADXSTM_BindFileNw(stm, name, dir, start, length);
    do {
        ADXT_ExecFsSvr();
    } while (P8(stm, 0x45) != 0);
}

void func_004d1e58(void *stm, const char *name, void *dir, int32_t start, int32_t length)
{
    func_004d18c8();
    func_004d1ec8(stm, name, dir, start, length);
    func_004d18d0();
}

/* 004d21e8 / 004d21a0: start at most the requested number of sectors. */
int32_t func_004d21e8(uint8_t *stm, int32_t sectors)
{
    func_004c54a8();
    func_004d20f0(stm);
    W32(stm, 0x5c) = sectors;
    func_004c54c0();
    return 1;
}

int32_t func_004d21a0(uint8_t *stm, int32_t sectors)
{
    int32_t result;
    func_004d18c8();
    result = func_004d21e8(stm, sectors);
    func_004d18d0();
    return result;
}

/* The matched leaf takes a guest address, not a host pointer. */
extern uint32_t func_004d2d70(uint32_t stm);

int32_t func_004d2d38(void *stm)
{
    int32_t result;
    func_004d18c8();
    result = func_004d2d70(P4_H2G(stm));
    func_004d18d0();
    return result;
}

/* 004d2dc8: locked wrapper of the byte store at 004d2e08. */
void func_004d2dc8(void *stm, int32_t sw)
{
    func_004d18c8();
    P8(stm, 0x44) = (uint8_t)sw;
    func_004d18d0();
}

/* 004d2fe8 / 004d2fa8: assign a stream joint and its full buffer policy. */
int32_t func_004d2fe8(void *stm, void *sj)
{
    int32_t bytes;
    SETPP(stm, 4, sj);
    func_004c54a8();
    bytes = sj_num(sj, 0) + sj_num(sj, 1);
    W32(stm, 0x40) = bytes;
    func_004c54c0();
    W32(stm, 0x1c) = bytes;
    W32(stm, 0x18) = bytes;
    return bytes;
}

int32_t func_004d2fa8(void *stm, void *sj)
{
    int32_t result;
    func_004d18c8();
    result = func_004d2fe8(stm, sj);
    func_004d18d0();
    return result;
}

/* 004d3258 / 004d3228: force a pending lazy open to settle, sharing the
 * controller server's test-and-set flag (the retail word at 00723f1c). */
void func_004d3258(void *stm)
{
    if (P32(stm, 0x50) == 0 || P32(stm, 8) != 0) {
        return;
    }
    P8(stm, 0x45) = 1;
    do {
        if (func_004f1278(&adxstmf_execsvr_flg) == 1) {
            ADXSTMF_ExecHndl(stm);
            adxstmf_execsvr_flg = 0;
        }
    } while (P8(stm, 0x45) != 0);
}

void func_004d3228(void *stm)
{
    func_004d18c8();
    func_004d3258(stm);
    func_004d18d0();
}
