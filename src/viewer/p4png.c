/* p4png: bounded streaming RGBA PNG writer. Filter type 0 preserves the readback
 * verbatim; miniz's level-1 deflate emits compressed IDAT chunks without copying
 * an image/row buffer. One reusable compressor is allocated for the whole image. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4png.h"

#include "../../third_party/miniz/miniz.h"

static void p4png_put_be32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

typedef struct {
    FILE *f;
    uint32_t crc;      /* CRC of the current chunk, type included */
    tdefl_compressor *deflate;
} Sink;

/* Starts a chunk: length, type, and the CRC state seeded with the type. */
static int p4png_chunk_begin(Sink *s, const char type[4], uint32_t len) {
    uint8_t hdr[8];
    p4png_put_be32(hdr, len);
    memcpy(hdr + 4, type, 4);
    if (fwrite(hdr, 1, 8, s->f) != 8) {
        return -1;
    }
    s->crc = (uint32_t)mz_crc32(0, (const uint8_t *)type, 4);
    return 0;
}

static int p4png_chunk_end(Sink *s) {
    uint8_t tail[4];
    p4png_put_be32(tail, s->crc);
    return fwrite(tail, 1, 4, s->f) == 4 ? 0 : -1;
}

/* Writes chunk payload bytes and folds them into the chunk CRC. */
static int p4png_put(Sink *s, const void *p, size_t n) {
    if (n != 0) {
        if (fwrite(p, 1, n, s->f) != n) {
            return -1;
        }
        s->crc = (uint32_t)mz_crc32(s->crc, (const uint8_t *)p, n);
    }
    return 0;
}

/* Each compressor output buffer becomes a complete IDAT chunk; no seek or staging
 * copy is needed, and an output error propagates through the low-level codec. */
static mz_bool p4png_deflate_put(const void *data, int len, void *user) {
    Sink *s = user;
    return p4png_chunk_begin(s, "IDAT", (uint32_t)len) == 0 &&
           p4png_put(s, data, (size_t)len) == 0 && p4png_chunk_end(s) == 0;
}

int p4png_write_rgba(const char *path, uint32_t w, uint32_t h, const uint8_t *rgba) {
    static const uint8_t sig[8] = {137, 'P', 'N', 'G', 13, 10, 26, 10};
    uint8_t ihdr[13];
    uint64_t raw_len;
    tdefl_status status;
    Sink s;
    int rc = -1;

    if (w == 0 || h == 0 || w > 0x7FFFFFFFu / 4 || !rgba) {
        return -1;
    }

    /* Keep row offsets representable on the 32-bit reference build as well. */
    raw_len = (uint64_t)h * (1 + 4ull * w);
    if (raw_len > 0xFFFFFFFFu) {
        return -1;
    }

    s.f = fopen(path, "wb");
    if (!s.f) {
        return -1;
    }
    s.deflate = malloc(sizeof *s.deflate);
    if (!s.deflate) {
        fclose(s.f);
        return -1;
    }
    /* One hash probe plus greedy parsing is miniz's fast level-1 preset. */
    status = tdefl_init(s.deflate, p4png_deflate_put, &s,
                        TDEFL_WRITE_ZLIB_HEADER | TDEFL_GREEDY_PARSING_FLAG | 1);
    if (status != TDEFL_STATUS_OKAY) {
        goto out;
    }

    p4png_put_be32(ihdr, w);
    p4png_put_be32(ihdr + 4, h);
    ihdr[8] = 8;   /* bit depth */
    ihdr[9] = 6;   /* colour type: RGBA */
    ihdr[10] = 0;  /* compression: deflate */
    ihdr[11] = 0;  /* filter method 0 */
    ihdr[12] = 0;  /* no interlace */

    if (fwrite(sig, 1, sizeof sig, s.f) != sizeof sig) {
        goto out;
    }
    if (p4png_chunk_begin(&s, "IHDR", sizeof ihdr) || p4png_put(&s, ihdr, sizeof ihdr) ||
        p4png_chunk_end(&s)) {
        goto out;
    }

    for (uint32_t y = 0; y < h; y++) {
        const uint8_t filter = 0;
        if (tdefl_compress_buffer(s.deflate, &filter, 1, TDEFL_NO_FLUSH) != TDEFL_STATUS_OKAY ||
            tdefl_compress_buffer(s.deflate, rgba + (size_t)y * w * 4, (size_t)w * 4,
                                 TDEFL_NO_FLUSH) != TDEFL_STATUS_OKAY) {
            goto out;
        }
    }
    if (tdefl_compress_buffer(s.deflate, NULL, 0, TDEFL_FINISH) != TDEFL_STATUS_DONE) {
        goto out;
    }

    if (p4png_chunk_begin(&s, "IEND", 0) || p4png_chunk_end(&s)) {
        goto out;
    }
    rc = 0;

out:
    free(s.deflate);
    if (fclose(s.f) != 0) {
        rc = -1;
    }
    return rc;
}
