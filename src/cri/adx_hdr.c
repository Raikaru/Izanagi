/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* ADX file header and footer readers (libadx "adx_dcd"), translated from the retail asm
 * 0x004C5648..0x004C60A0.
 *
 * An ADX file starts with 0x80 0x00 and a big-endian offset that points at the end of the header
 * (the string "(c)CRI" sits at offset - 2). Everything in the header is big-endian. Fields read by
 * ADX_DecodeInfo: +4 coding type, +5 frame bytes, +6 bits per sample, +7 channels, +8 sampling
 * rate, +12 total samples. The optional blocks that follow, by header version (ADX_DecodeInfoExVer,
 * bytes 18 and 19): +16 the cutoff of the predictor's low-pass, +20 (version 4: +32) the loop
 * block, then "AINF" with the default volume and pan. The retail ADXT 9.44 differs from the
 * reconstructed 8.30 source (decomp/src/cri/libadxe/adx_dcd.c) in the version 4 offsets and the
 * ExIdly, ExLoopEncTime and Ainf readers, so these follow the asm.
 *
 * Returns are 0 for success, -1 when the buffer is too short for the field, -2 when it is not an
 * ADX header (ADX_DecodeFooter: not a footer). */
#include <stdint.h>

#include "p4pc/gmem.h"

static uint32_t be16(const uint8_t *p)
{
    return (uint32_t)p[0] << 8 | p[1];
}

static int32_t be16s(const uint8_t *p)
{
    return (int16_t)be16(p);
}

static uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

/* 0x004C5648: byte offset of the first 0x80 0x00 pair at an even offset, in *dlen. */
int32_t ADX_ScanInfoCode(const uint8_t *ibuf, int32_t ibuflen, int16_t *dlen)
{
    int32_t ptr;

    for (ptr = 0; ptr < ibuflen - 1; ptr += 2) {
        if (*(const uint16_t *)(ibuf + ptr) == 0x0080) {
            *dlen = (int16_t)ptr;
            return 0;
        }
    }
    *dlen = 0;
    return -1;
}

/* 0x004C5770 */
int32_t ADX_DecodeInfo(const uint8_t *ibuf, int32_t ibuflen, int16_t *dlen, int8_t *code, int8_t *bps,
                       int8_t *blksize, int8_t *nch, int32_t *sfreq, int32_t *total_nsmpl, int32_t *nsmpl_blk)
{
    if (ibuflen < 16) {
        return -1;
    }
    if (be16(ibuf) != 0x8000) {
        return -2;
    }
    *dlen = (int16_t)(be16(ibuf + 2) + 4);
    *code = (int8_t)ibuf[4];
    *blksize = (int8_t)ibuf[5];
    *bps = (int8_t)ibuf[6];
    *nch = (int8_t)ibuf[7];
    *sfreq = (int32_t)be32(ibuf + 8);
    *total_nsmpl = (int32_t)be32(ibuf + 12);
    if (*bps == 0) {
        *nsmpl_blk = 0;
    } else {
        *nsmpl_blk = ((int32_t)*blksize * 8 - 16) / (int32_t)*bps;
    }
    return 0;
}

/* 0x004C5870: the low-pass cutoff (Hz) the predictor coefficients are computed from. */
int32_t ADX_DecodeInfoExADPCM2(const uint8_t *ibuf, int32_t ibuflen, uint16_t *cof)
{
    if (ibuflen < 18) {
        return -1;
    }
    if (be16(ibuf) != 0x8000) {
        return -2;
    }
    if (be16s(ibuf + 2) < 14) {
        return -1;
    }
    *cof = (uint16_t)be16(ibuf + 16);
    return 0;
}

/* 0x004C58F0: header version and revision (bytes 18 and 19). */
int32_t ADX_DecodeInfoExVer(const uint8_t *ibuf, int32_t ibuflen, uint8_t *ver, uint8_t *rev)
{
    if (ibuflen < 20) {
        return -1;
    }
    if (be16(ibuf) != 0x8000) {
        return -2;
    }
    if (be16s(ibuf + 2) < 16) {
        return -1;
    }
    *ver = ibuf[18];
    *rev = ibuf[19];
    return 0;
}

/* 0x004C5968: the two initial delays of the channels (two 16-bit values each), from version 4 on;
 * zero below that. */
int32_t ADX_DecodeInfoExIdly(const uint8_t *ibuf, int32_t ibuflen, int16_t *idly_a, int16_t *idly_b)
{
    uint8_t ver, rev;

    if (ADX_DecodeInfoExVer(ibuf, ibuflen, &ver, &rev) != 0) {
        return -1;
    }
    if (ver < 4) {
        idly_a[0] = idly_a[1] = 0;
        idly_b[0] = idly_b[1] = 0;
        return 0;
    }
    if (ibuflen < 32) {
        return -1;
    }
    if (be16(ibuf) != 0x8000) {
        return -2;
    }
    if (be16s(ibuf + 2) < 28) {
        return -1;
    }
    idly_a[0] = (int16_t)be16(ibuf + 24);
    idly_b[0] = (int16_t)be16(ibuf + 26);
    idly_a[1] = (int16_t)be16(ibuf + 28);
    idly_b[1] = (int16_t)be16(ibuf + 30);
    return 0;
}

/* 0x004C5B68: the loop block. *nloop is 0 before anything is read. The block starts at +20, at
 * +32 in a version 4 header (which is also longer: the header must hold 60 bytes instead of 48).
 * lp_ins_nsmpl and nloop are stored before nloop is checked, as in the retail function. */
int32_t ADX_DecodeInfoExLoop(const uint8_t *ibuf, int32_t ibuflen, int32_t *lp_ins_nsmpl, int16_t *nloop,
                             int16_t *lp_type, int32_t *lp_spos, int32_t *lp_sofst, int32_t *lp_epos,
                             int32_t *lp_eofst)
{
    uint8_t ver, rev;
    int32_t need, ptr;

    *nloop = 0;
    if (ADX_DecodeInfoExVer(ibuf, ibuflen, &ver, &rev) != 0) {
        return -1;
    }
    need = ver == 4 ? 60 : 48;
    if (ibuflen < need) {
        return -1;
    }
    if (be16(ibuf) != 0x8000) {
        return -2;
    }
    if (be16s(ibuf + 2) < need - 4) {
        return -1;
    }
    ptr = ver == 4 ? 32 : 20;
    *lp_ins_nsmpl = be16s(ibuf + ptr);
    *nloop = (int16_t)be16(ibuf + ptr + 2);
    if (*nloop != 1) {
        return -2;
    }
    *lp_type = (int16_t)be16(ibuf + ptr + 6);
    *lp_spos = (int32_t)be32(ibuf + ptr + 8);
    *lp_sofst = (int32_t)be32(ibuf + ptr + 12);
    *lp_epos = (int32_t)be32(ibuf + ptr + 16);
    *lp_eofst = (int32_t)be32(ibuf + ptr + 20);
    return 0;
}

/* 0x004C5AA0: the loop block as sample positions inside the stream's first sector-aligned data:
 * *nloop is 1 with a loop block and then *spos and *epos are the loop start and end minus the
 * instruction samples modulo 2048, else all three are 0. Returns 0. */
int32_t ADX_DecodeInfoExLoopEncTime(const uint8_t *ibuf, int32_t ibuflen, int16_t *nloop, int32_t *spos,
                                    int32_t *epos)
{
    int32_t ins = 0, sofst = 0, eofst = 0, ps = 0, pe = 0;
    int16_t n = 0, type = 0;

    ADX_DecodeInfoExLoop(ibuf, ibuflen, &ins, &n, &type, &ps, &sofst, &pe, &eofst);
    if (n != 1) {
        n = 0;
    }
    *nloop = n;
    if (n != 0) {
        int32_t mod = ins - ((ins < 0 ? ins + 2047 : ins) >> 11 << 11);

        *spos = ps - mod;
        *epos = pe - mod;
    } else {
        *spos = 0;
        *epos = 0;
    }
    return 0;
}

/* 0x004C5DC8: the AINF block (default volume and pan), after the loop block when there is one.
 * *ainflen is 0 before anything is read. id receives the block's 16 identifier bytes, *vol its
 * default volume, pan[0] and pan[1] the channels' default pans. */
int32_t ADX_DecodeInfoAinf(const uint8_t *ibuf, int32_t ibuflen, int32_t *ainflen, uint8_t *id, int16_t *vol,
                           int16_t *pan)
{
    uint8_t ver, rev;
    int32_t need, ptr, i;

    *ainflen = 0;
    if (ADX_DecodeInfoExVer(ibuf, ibuflen, &ver, &rev) != 0) {
        return -1;
    }
    need = ver == 4 ? 72 : 60;
    if (ibuflen < need) {
        return -1;
    }
    if (be16(ibuf) != 0x8000) {
        return -2;
    }
    if (be16s(ibuf + 2) < need - 4) {
        return -1;
    }
    ptr = (ver == 4 ? 32 : 20) + 4;
    if (be16(ibuf + ptr - 2) != 0) {
        ptr += 20; /* a loop block precedes the AINF block */
    }
    if (be32(ibuf + ptr) != 0x41494e46u) { /* "AINF" */
        return -2;
    }
    *ainflen = (int32_t)be32(ibuf + ptr + 4);
    for (i = 0; i < 16; i++) {
        id[i] = ibuf[ptr + 8 + i];
    }
    *vol = (int16_t)be16(ibuf + ptr + 24);
    pan[0] = (int16_t)be16(ibuf + ptr + 28);
    pan[1] = (int16_t)be16(ibuf + ptr + 30);
    return 0;
}

/* 0x004C5FE8: a footer is 0x80 0x01 and a big-endian length; *dlen is that length + 4. */
int32_t ADX_DecodeFooter(const uint8_t *ibuf, int32_t ibuflen, int16_t *dlen)
{
    if (ibuflen < 16) {
        return -1;
    }
    if (be16(ibuf) != 0x8001) {
        return -2;
    }
    *dlen = (int16_t)(be16(ibuf + 2) + 4);
    return 0;
}

/* 0x004C6048: bytes of header plus AINF block (info) rounded up so that the audio data starts on
 * a multiple of `align` bytes after `ofst`: with a loop block 57 bytes more than without are
 * assumed to precede the data, 33 without. */
uint32_t ADX_CalcHdrInfoLen(int32_t loop, uint32_t info, uint32_t ofst, uint32_t align)
{
    uint32_t base = ofst + align;

    return loop == 0 ? ((info + base + 33) / align) * align - ofst : ((info + base + 57) / align) * align - ofst;
}

/* ---- the whole-header decoder (0x004c9f20..0x004ca1b0) ----------------------------------------
 * ADX_DecodeHeader reads a complete header into a 64-byte record:
 *   +0 +1 +2 +3 the first four bytes after the length (coding, frame bytes, bits, channels),
 *   +4 sampling rate, +8 total samples, +12 cutoff (16 bit), +14 loop count (16 bit),
 *   +16 loop records of 20 bytes, +36 play parameter records of 12 bytes (the AHX decoder takes its two
 *   channels' volume, pan and effect levels from them), +60 version, +61 revision.
 * The loop records are all read from the same place (the retail loop does not advance its source), and
 * the header is only checked against what is left of it before each part. */

/* 0x004c9f20 */
void func_004c9f20(const uint8_t *s, uint8_t *out)
{
    out[0] = s[0];
    out[1] = s[1];
    out[2] = s[2];
    out[3] = s[3];
    *(uint32_t *)(out + 4) = be32(s + 4);
    *(uint32_t *)(out + 8) = be32(s + 8);
    *(uint16_t *)(out + 12) = (uint16_t)be16(s + 12);
    out[60] = s[14];
    out[61] = s[15];
}

/* 0x004c9fe0 */
void func_004c9fe0(const uint8_t *s, uint8_t *out)
{
    *(uint16_t *)(out + 14) = (uint16_t)be16(s + 2);
}

/* 0x004ca000 */
void func_004ca000(const uint8_t *s, uint8_t *out)
{
    *(uint16_t *)(out + 0) = (uint16_t)be16(s);
    *(uint16_t *)(out + 2) = (uint16_t)be16(s + 2);
    *(uint32_t *)(out + 4) = be32(s + 4);
    *(uint32_t *)(out + 8) = be32(s + 8);
    *(uint32_t *)(out + 12) = be32(s + 12);
    *(uint32_t *)(out + 16) = be32(s + 16);
}

/* 0x004ca0f0: a play parameter record: a 16-bit value and, when it is positive, a second one; then five
 * byte pairs of which the second byte is only read when the first is positive. */
void func_004ca0f0(const uint8_t *s, uint8_t *out)
{
    int32_t k;

    *(uint16_t *)out = (uint16_t)be16(s);
    if ((int16_t)be16(s) > 0) {
        *(uint16_t *)(out + 2) = (uint16_t)be16(s + 2);
    }
    s += 4;
    for (k = 0; k < 4; k++) {
        out[4 + k * 2] = s[0];
        if ((int8_t)s[0] > 0) {
            out[5 + k * 2] = s[1];
        }
        s += 2;
    }
}

/* 0x004ca1b0 ADX_DecodeHeader(ibuf, len, *hdrlen, out): *hdrlen the header length field; with out the
 * parts of the header that fit are read into it. 0 for success, -1 for fewer than 4 bytes, -4 for no
 * ADX magic, -2 for a header longer than the data or shorter than its fixed part. */
int32_t ADX_DecodeHeader(const uint8_t *ibuf, int32_t len, int32_t *hdrlen, uint8_t *out)
{
    const uint8_t *s;
    int32_t rem, cnt, i;

    if (hdrlen != 0) {
        *hdrlen = 0;
    }
    if (len < 4) {
        return -1;
    }
    if (be16(ibuf) != 0x8000) {
        return -4;
    }
    rem = (int32_t)be16(ibuf + 2);
    s = ibuf + 4;
    if (hdrlen != 0) {
        *hdrlen = rem;
    }
    if (out == 0) {
        return 0;
    }
    if (len < rem + 4) {
        return -2;
    }
    rem -= 6;
    if (rem < 16) {
        return -2;
    }
    func_004c9f20(s, out);
    s += 16;
    rem -= 16;
    if ((int8_t)out[60] == 4) {
        s += 12;
        rem -= 12;
    }
    if (rem < 4) {
        return 0;
    }
    func_004c9fe0(s, out);
    s += 4;
    cnt = *(int16_t *)(out + 14);
    if (rem < cnt * 20) {
        return 0;
    }
    for (i = 0; i < cnt; i++) {
        func_004ca000(s, out + 16 + i * 20);
    }
    rem -= cnt * 20;
    for (i = 0; i < (int8_t)out[3]; i++) {
        if (rem < 12) {
            return 0;
        }
        rem -= 12;
        func_004ca0f0(s, out + 36 + i * 12);
        s += 12;
    }
    return 0;
}
