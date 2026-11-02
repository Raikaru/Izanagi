/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* ADXT start entry points and ADX key/adjust helpers the re4 tree has no retail-addressed C for,
 * translated from the retail asm. The ADXT start functions only record what to play in the handle
 * (the file name or the memory joint, the range, status 1, the start request byte +0xac); the
 * server (re4 adxt_start_stm, 0x004d40a0) binds the stream when it sees the request.
 *
 * ADXT handle offsets used (see decomp re4/adx_t.h): +1 status, +2 input kind (0 file name, 1 file
 * range from a partition, 2 memory), +4 decoder, +8 stream controller, +0x60 error code, +0xac
 * start request, +0xb0 end of the file ring buffer (scratch for the file name), +0xb4 pending file
 * name, +0xb8 pending directory, +0xbc pending offset, +0xc0 pending sector count.
 *
 * The four wrappers 0x004d6d18, 0x004d6f78, 0x004d7030 and 0x004d7160 are the entries of the ACSSND
 * voice vtable (start A..D: file name, partition file, memory with a length, memory table entry).
 */
#include <stdint.h>
#include <string.h>

#define W32(p, off) (*(int32_t *)((uint8_t *)(p) + (off)))
#define U32(p, off) (*(uint32_t *)((uint8_t *)(p) + (off)))
#define M32(a) (*(volatile int32_t *)P4_G2H(a))
#define M16(a) (*(volatile int16_t *)P4_G2H(a))

extern void func_004c54d8(void);                                   /* ADX lock (empty) */
extern void func_004c54e0(void);                                   /* ADX unlock (empty) */
extern void func_004c54a8(void);                                   /* SVM lock */
extern void func_004c54c0(void);                                   /* SVM unlock */
extern void func_004c6a98(const char *msg);                        /* ADX error report */
extern void func_004c6af0(const char *msg, const char *arg);       /* ADXERR_CallErrFunc2 */
extern void func_004c6c68(int32_t a, int32_t b, char *buf, int32_t len); /* ADXERR_ItoA2 */
extern int32_t func_004c9a28(int32_t ptid, int32_t member, char *name, int32_t *dir, int32_t *start, int32_t *length);
extern void func_004c4088(uint32_t adxb, uint32_t fn, uint32_t obj);  /* ADXB_SetCbDec (decomp: register words) */
extern void func_004d3fa8(void *adxt, void *sj);                   /* decoder start on a joint (adxt_start_sjd) */
extern void func_004d65b8(void *adxt, int32_t sw);                 /* link switch */
extern void *func_004ed600(void *data, int32_t len);               /* SJMEM_Create */
extern int32_t func_004c5dc8(const uint8_t *ibuf, int32_t len, int32_t *ainflen, uint8_t *id, int16_t *vol, int16_t *pan);
extern void ADXT_Stop(void *adxt);

/* 0x004c99a0: the partition member lookup under the ADX lock. */
int32_t func_004c99a0(int32_t ptid, int32_t member, char *name, int32_t *dir, int32_t *start, int32_t *length)
{
    int32_t r;

    func_004c54d8();
    r = func_004c9a28(ptid, member, name, dir, start, length);
    func_004c54e0();
    return r;
}

/* 0x004d6ec0: start playing a file by name and sector range: stop the handle, keep the name in the
 * scratch area at the end of the ring buffer, record the range, status 1, input kind 0, link
 * switch cleared. Error text 0x00759370 for a NULL handle or name. */
void func_004d6ec0(uint8_t *adxt, const char *fname, int32_t ofst, int32_t nsct)
{
    if (adxt == 0 || fname == 0) {
        func_004c6a98((const char *)P4_G2H(0x00759370u));
        return;
    }
    ADXT_Stop(adxt);
    strcpy((char *)P4_G2H(U32(adxt, 0xb0)), fname);
    W32(adxt, 0xbc) = ofst;
    W32(adxt, 0xc0) = nsct;
    W32(adxt, 0xb4) = W32(adxt, 0xb0);
    adxt[0xac] = 1;
    W32(adxt, 0xb8) = 0;
    adxt[1] = 1;
    adxt[2] = 0;
    func_004d65b8(adxt, 0);
}

/* 0x004d6fb8 / 0x004d6f78: the whole file (offset 0, 0xfffff sectors). */
void func_004d6fb8(uint8_t *adxt, const char *fname)
{
    func_004d6ec0(adxt, fname, 0, 0xfffff);
}

void func_004d6f78(uint8_t *adxt, const char *fname)
{
    func_004c54d8();
    func_004d6fb8(adxt, fname);
    func_004c54e0();
}

/* 0x004d6d68 / 0x004d6d18: start a partition file (patid, fid): the lookup writes the name into the
 * ring buffer's scratch area and returns directory, offset and length. Found with a stream
 * controller: recorded, status 1, input kind 1. Found without one: "ADXT_StartAfs: can't open"
 * (0x00759348) with the "patid fid" text, error code 0xffff, status 6. Not found: nothing. Error text
 * 0x00759318 for a NULL handle. */
void func_004d6d68(uint8_t *adxt, int32_t patid, int32_t fid)
{
    int32_t dir;
    int32_t ofst;
    int32_t nsct;
    char msg[16];

    if (adxt == 0) {
        func_004c6a98((const char *)P4_G2H(0x00759318u));
        return;
    }
    ADXT_Stop(adxt);
    if (func_004c99a0(patid, fid, (char *)P4_G2H(U32(adxt, 0xb0)), &dir, &ofst, &nsct) != 0) {
        return;
    }
    if (W32(adxt, 8) == 0) {
        func_004c6c68(patid, fid, msg, 16);
        func_004c6af0((const char *)P4_G2H(0x00759348u), msg);
        *(uint16_t *)(adxt + 0x60) = 0xffff;
        adxt[1] = 6;
        return;
    }
    W32(adxt, 0xb8) = dir;
    W32(adxt, 0xb4) = W32(adxt, 0xb0);
    W32(adxt, 0xbc) = ofst;
    W32(adxt, 0xc0) = nsct;
    adxt[2] = 1;
    adxt[1] = 1;
    adxt[0xac] = 1;
    func_004d65b8(adxt, 0);
}

void func_004d6d18(uint8_t *adxt, int32_t patid, int32_t fid)
{
    func_004c54d8();
    func_004d6d68(adxt, patid, fid);
    func_004c54e0();
}

/* 0x004d7080 / 0x004d7030: start from memory (data, length >= 0): stop, take the SVM lock, make a
 * memory stream joint, start the decoder on it, input kind 2, link switch cleared. 0x007593a0 for a
 * bad argument, 0x007593d0 when the joint cannot be made. */
void func_004d7080(uint8_t *adxt, void *data, int32_t len)
{
    void *sj;

    if (adxt == 0 || data == 0 || len < 0) {
        func_004c6a98((const char *)P4_G2H(0x007593a0u));
        return;
    }
    ADXT_Stop(adxt);
    func_004c54a8();
    sj = func_004ed600(data, len);
    if (sj == 0) {
        func_004c54c0();
        func_004c6a98((const char *)P4_G2H(0x007593d0u));
        return;
    }
    func_004d3fa8(adxt, sj);
    adxt[2] = 2;
    func_004d65b8(adxt, 0);
    func_004c54c0();
}

void func_004d7030(uint8_t *adxt, void *data, int32_t len)
{
    func_004c54d8();
    func_004d7080(adxt, data, len);
    func_004c54e0();
}

static uint32_t bswap(uint32_t v)
{
    return (v >> 24) | (v << 24) | ((v & 0xff00) << 8) | ((v >> 8) & 0xff00);
}

/* 0x004d71b0 / 0x004d7160: start entry `no` of an ACX table (big-endian count at +4, then offset
 * and size pairs from +8): the entry is played from memory like func_004d7080 with the largest length.
 * 0x00759400 for a bad argument, 0x00759430 when the joint cannot be made; an entry number outside
 * the table does nothing after the stop. */
void func_004d71b0(uint8_t *adxt, uint8_t *acx, int32_t no)
{
    void *sj;

    if (adxt == 0 || acx == 0 || no < 0) {
        func_004c6a98((const char *)P4_G2H(0x00759400u));
        return;
    }
    ADXT_Stop(adxt);
    if (!(no < (int32_t)bswap(U32(acx, 4)) && no >= 0)) {
        return;
    }
    func_004c54a8();
    sj = func_004ed600(acx + bswap(U32(acx, 8 + no * 8)), 0x40000000);
    if (sj == 0) {
        func_004c54c0();
        func_004c6a98((const char *)P4_G2H(0x00759430u));
        return;
    }
    func_004d3fa8(adxt, sj);
    adxt[2] = 2;
    func_004d65b8(adxt, 0);
    func_004c54c0();
}

void func_004d7160(uint8_t *adxt, uint8_t *acx, int32_t no)
{
    func_004c54d8();
    func_004d71b0(adxt, acx, no);
    func_004c54e0();
}

/* 0x004c3178: the ADX key table's use count (the word at 0x00714c1c) goes up by one; returns 0. The
 * partner at 0x004c3198 takes it down. */
int32_t func_004c3178(void)
{
    M32(0x00714c1c) = M32(0x00714c1c) + 1;
    return 0;
}

/* 0x004c31b8: the three 16-bit ADX keys of a string. Each key starts from its seed word
 * (0x007577d8, 0x007579d8, 0x00757bd8) and for every character takes
 * table[(key * mult[char]) % 1024] (tables at 0x007575d8 and 0x007576d8, signed 16-bit, the
 * remainder keeps the dividend's sign). Keys are 0 for an empty string. The tables are set up by
 * func_004c3178 on first use. */
void func_004c31b8(const char *s, int32_t n, int16_t *k0, int16_t *k1, int16_t *k2)
{
    static const uint32_t seeds[3] = { 0x007577d8, 0x007579d8, 0x00757bd8 };
    int16_t *out[3];
    int32_t k;

    out[0] = k0;
    out[1] = k1;
    out[2] = k2;
    if (M32(0x00714c1c) == 0) {
        func_004c3178();
    }
    *k0 = 0;
    *k1 = 0;
    *k2 = 0;
    if (s == 0 && n <= 0) {
        return;
    }
    for (k = 0; k < 3; k++) {
        int16_t key = M16(seeds[k]);
        int32_t i;

        for (i = 0; i < n; i++) {
            int32_t t = (int32_t)key * (int32_t)M16(0x007576d8 + (int32_t)(int8_t)s[i] * 2);

            key = M16(0x007575d8 + (t % 1024) * 2);
        }
        *out[k] = key;
    }
}

/* 0x004c3d70: the decoder's keys from a string (ADXB +0xa0, +0xa2, +0xa4). */
void ADXB_SetExtString(uint8_t *adxb, const char *str)
{
    int16_t k0;
    int16_t k1;
    int16_t k2;

    func_004c31b8(str, (int32_t)strlen(str), &k0, &k1, &k2);
    *(uint16_t *)(adxb + 0xa0) = (uint16_t)k0;
    *(uint16_t *)(adxb + 0xa4) = (uint16_t)k2;
    *(uint16_t *)(adxb + 0xa2) = (uint16_t)k1;
}

/* 0x004c3dd8: the default keys (0x00714c34, 0x00714c36, 0x00714c38) from a string. */
void func_004c3dd8(const char *str)
{
    func_004c31b8(str, (int32_t)strlen(str), (int16_t *)P4_G2H(0x00714c34), (int16_t *)P4_G2H(0x00714c36),
                  (int16_t *)P4_G2H(0x00714c38));
}

/* 0x004cec00: the decoder's callback pair on its ADXB (+4): function at +0xfc, object at +0x100. */
void func_004cec00(uint8_t *sjd, uint32_t fn, uint32_t obj)
{
    func_004c4088(U32(sjd, 4), fn, obj);
}

/* 0x004cecc8: shift the decoder's sample window under the SVM lock: a positive count moves the start
 * (+0xa0) forward, a count of zero or less lengthens the end (+0xa4). Returns the count. */
int32_t ADXSJD_AdjustSmpl(uint8_t *sjd, int32_t n)
{
    func_004c54a8();
    if (n < 1) {
        W32(sjd, 0xa4) = W32(sjd, 0xa4) - n;
    } else {
        W32(sjd, 0xa0) = W32(sjd, 0xa0) + n;
    }
    func_004c54c0();
    return n;
}

/* 0x004d5370: the 16 identifier bytes of the AINF block of an ADX in memory, in the shared buffer at
 * 0x00922e30, or NULL without an AINF block. */
uint8_t *adxt_GetDataIdFromMem(const uint8_t *data)
{
    int32_t ainflen;
    int16_t vol[2];
    int16_t pan[8];
    uint8_t *id = (uint8_t *)P4_G2H(0x00922e30);

    func_004c5dc8(data, 0x800, &ainflen, id, vol, pan);
    return ainflen != 0 ? id : 0;
}
