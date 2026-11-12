/* vag_test: the SPU ADPCM decoder (src/platform/sound/vag.c) against vectors worked by hand
 * from the format: sample = (signed nibble << 12 >> shift) + (h1 * f0 + h2 * f1 + 32) >> 6 with
 * the filter pairs (0,0), (60,0), (115,-52), (98,-55), (122,-60). Needs no disc. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "vag.h"

static int failures;

static void fail(const char *what)
{
    fprintf(stderr, "vag_test: FAIL %s\n", what);
    failures++;
}

/* A block whose 28 nibbles are nib[0..27] (low nibble of byte 2 first). */
static void make_block(uint8_t b[16], int shift, int filter, int flags, const int *nib)
{
    int i;

    b[0] = (uint8_t)(filter << 4 | shift);
    b[1] = (uint8_t)flags;
    for (i = 0; i < 14; i++) {
        b[2 + i] = (uint8_t)((nib[i * 2] & 15) | (nib[i * 2 + 1] & 15) << 4);
    }
}

static void expect(const char *name, const int16_t *got, const int *want, int n)
{
    int i;

    for (i = 0; i < n; i++) {
        if (got[i] != want[i]) {
            fprintf(stderr, "vag_test: %s sample %d: got %d want %d\n", name, i, got[i], want[i]);
            fail(name);
            return;
        }
    }
}

int main(void)
{
    uint8_t blk[16];
    int16_t out[28 * 4];
    int32_t h1, h2;
    int nib[28];
    int i;

    /* shift 12, filter 0: the samples are the signed nibbles */
    for (i = 0; i < 28; i++) {
        nib[i] = i < 16 ? i : i - 16;
    }
    {
        int want[28] = {0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1,
                        0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5};
        make_block(blk, 12, 0, 0, nib);
        h1 = h2 = 0;
        p4vag_block(blk, &h1, &h2, out);
        expect("shift 12", out, want, 28);
        if (h1 != -5 || h2 != -6) {
            fail("history after shift 12");
        }
    }

    /* shift 8: the same nibbles times 16 */
    {
        int want[28];

        for (i = 0; i < 28; i++) {
            want[i] = (nib[i] < 8 ? nib[i] : nib[i] - 16) * 16;
        }
        make_block(blk, 8, 0, 0, nib);
        h1 = h2 = 0;
        p4vag_block(blk, &h1, &h2, out);
        expect("shift 8", out, want, 28);
    }

    /* filter 1 (60/64 of the previous sample): nibble 4 every sample.
     * s0 = 4; s1 = 4 + (4*60+32)/64 = 8; s2 = 4 + (8*60+32)/64 = 12; s3 = 4 + (12*60+32)/64 = 15;
     * s4 = 4 + (15*60+32)/64 = 18 */
    for (i = 0; i < 28; i++) {
        nib[i] = 4;
    }
    {
        int want[5] = {4, 8, 12, 15, 18};

        make_block(blk, 12, 1, 0, nib);
        h1 = h2 = 0;
        p4vag_block(blk, &h1, &h2, out);
        expect("filter 1", out, want, 5);
    }

    /* filter 2 (115, -52): nibbles 7, 0, 0, 0.
     * s0 = 7; s1 = (7*115+32)/64 = 13; s2 = (13*115 - 7*52 + 32)/64 = 18;
     * s3 = (18*115 - 13*52 + 32)/64 = 22 */
    memset(nib, 0, sizeof nib);
    nib[0] = 7;
    {
        int want[4] = {7, 13, 18, 22};

        make_block(blk, 12, 2, 0, nib);
        h1 = h2 = 0;
        p4vag_block(blk, &h1, &h2, out);
        expect("filter 2", out, want, 4);
    }

    /* clamp: shift 0 nibble 7 is 28672; filter 1 adds (28672*60+32)>>6 = 26880, which exceeds
     * 32767 */
    for (i = 0; i < 28; i++) {
        nib[i] = 7;
    }
    {
        int want[3] = {28672, 32767, 32767};

        make_block(blk, 0, 1, 0, nib);
        h1 = h2 = 0;
        p4vag_block(blk, &h1, &h2, out);
        expect("clamp", out, want, 3);
    }

    /* history carries across blocks: two filter-1 blocks of nibble 4 continue the ramp */
    {
        uint8_t two[32];
        P4VagInfo info;
        uint32_t n;
        int want[30] = {4, 8, 12, 15, 18};

        for (i = 0; i < 28; i++) {
            nib[i] = 4;
        }
        make_block(two, 12, 1, 0, nib);
        make_block(two + 16, 12, 1, P4VAG_FLAG_END, nib);
        n = p4vag_decode(two, sizeof two, out, &info);
        if (n != 56 || info.frames != 56 || info.bytes != 32 || info.has_loop) {
            fail("two-block decode shape");
        }
        expect("block history", out, want, 5);
        /* the second block's first sample continues from the first block's last two */
        {
            int32_t a = out[27], b = out[26];
            int32_t s = 4 + ((a * 60 + 0 * b + 32) >> 6);

            if (out[28] != s) {
                fail("history does not carry into the next block");
            }
        }
    }

    /* loop flags: start on the first block, end+repeat on the third */
    {
        uint8_t three[48];
        P4VagInfo info;

        memset(nib, 0, sizeof nib);
        make_block(three, 12, 0, P4VAG_FLAG_LOOP_START, nib);
        make_block(three + 16, 12, 0, 0, nib);
        make_block(three + 32, 12, 0, P4VAG_FLAG_END | P4VAG_FLAG_REPEAT, nib);
        if (p4vag_decode(three, sizeof three, NULL, &info) != 84 || !info.has_loop || info.loop_start != 0) {
            fail("loop flags");
        }
        /* a loop start in the middle */
        three[1] = 0;
        three[17] = P4VAG_FLAG_LOOP_START;
        p4vag_decode(three, sizeof three, NULL, &info);
        if (info.loop_start != 28 || !info.has_loop) {
            fail("loop start in the second block");
        }
        /* an end without repeat stops decoding and has no loop; trailing blocks are ignored */
        three[17] = P4VAG_FLAG_END;
        if (p4vag_decode(three, sizeof three, NULL, &info) != 56 || info.has_loop || info.bytes != 32) {
            fail("end without repeat");
        }
    }

    if (failures != 0) {
        return 1;
    }
    printf("vag_test: ok\n");
    return 0;
}
