/* Sony SPU ADPCM decoder, see vag.h. */
#include "vag.h"

/* Filter pairs (the previous sample's weight, the one before's), in 1/64. */
static const int32_t vag_f0[5] = {0, 60, 115, 98, 122};
static const int32_t vag_f1[5] = {0, 0, -52, -55, -60};

void p4vag_block(const uint8_t block[16], int32_t *h1, int32_t *h2, int16_t out[28])
{
    int shift = block[0] & 15;
    int filter = block[0] >> 4;
    int i;

    if (filter > 4) {
        filter = 4;
    }
    if (shift > 12) {
        shift = 9;
    }
    for (i = 0; i < P4VAG_BLOCK_SAMPLES; i++) {
        int nib = (block[2 + (i >> 1)] >> ((i & 1) * 4)) & 15;
        int32_t s = (int16_t)(nib << 12) >> shift;

        s += (*h1 * vag_f0[filter] + *h2 * vag_f1[filter] + 32) >> 6;
        if (s > 32767) {
            s = 32767;
        } else if (s < -32768) {
            s = -32768;
        }
        out[i] = (int16_t)s;
        *h2 = *h1;
        *h1 = s;
    }
}

uint32_t p4vag_decode(const uint8_t *src, size_t max_bytes, int16_t *dst, P4VagInfo *info)
{
    int32_t h1 = 0;
    int32_t h2 = 0;
    uint32_t frames = 0;
    uint32_t loop_start = 0;
    int has_loop = 0;
    size_t off = 0;
    int16_t tmp[P4VAG_BLOCK_SAMPLES];

    while (off + P4VAG_BLOCK_BYTES <= max_bytes) {
        const uint8_t *b = src + off;
        int flags = b[1];

        if (flags & P4VAG_FLAG_LOOP_START) {
            loop_start = frames;
        }
        p4vag_block(b, &h1, &h2, dst != NULL ? dst + frames : tmp);
        frames += P4VAG_BLOCK_SAMPLES;
        off += P4VAG_BLOCK_BYTES;
        if (flags & P4VAG_FLAG_END) {
            has_loop = (flags & P4VAG_FLAG_REPEAT) != 0;
            break;
        }
    }
    if (info != NULL) {
        info->frames = frames;
        info->loop_start = loop_start;
        info->has_loop = has_loop;
        info->bytes = (uint32_t)off;
    }
    return frames;
}
