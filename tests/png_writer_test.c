/* Real streaming writer + bounded miniz decode: opaque/transparent pixels,
 * incompressible multi-IDAT output and a useful fast compression ratio. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4png.h"
#include "../third_party/miniz/miniz.h"

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "png_writer:%d: %s\n", __LINE__, #condition); goto out; \
} } while (0)

static uint32_t be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

static int roundtrip(uint32_t w, uint32_t h, int uniform, int streamed) {
    char path[96];
    FILE *file = NULL;
    uint8_t *pixels = NULL, *png = NULL, *raw = NULL;
    const size_t stride = (size_t)w * 4, pixel_bytes = stride * h;
    const size_t raw_bytes = (stride + 1) * h;
    size_t length, pos, packed = 0, chunks = 0;
    uint32_t random = 0x12345678u;
    mz_ulong original;
    long end;
    int done = 0, rc = 1;
    snprintf(path, sizeof path, "png_writer_%u_%ux%u.png", (unsigned)(sizeof(void *) * 8), w, h);
    pixels = malloc(pixel_bytes);
    raw = malloc(raw_bytes);
    CHECK(pixels && raw);
    for (size_t i = 0; i < pixel_bytes; ++i) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        pixels[i] = uniform ? (uint8_t)(i % 4 == 3 ? 255 : 34 * (i % 4 + 1)) : (uint8_t)random;
    }
    original = mz_adler32(MZ_ADLER32_INIT, pixels, pixel_bytes);
    CHECK(p4png_write_rgba(path, w, h, pixels) == 0);
    CHECK(mz_adler32(MZ_ADLER32_INIT, pixels, pixel_bytes) == original);
    file = fopen(path, "rb");
    CHECK(file && fseek(file, 0, SEEK_END) == 0);
    end = ftell(file);
    CHECK(end >= 45 && fseek(file, 0, SEEK_SET) == 0);
    length = (size_t)end;
    png = malloc(length);
    CHECK(png && fread(png, 1, length, file) == length);
    {
        int closed = fclose(file);
        file = NULL;
        CHECK(closed == 0);
    }
    CHECK(memcmp(png, "\211PNG\r\n\032\n", 8) == 0);
    pos = 8;
    while (pos + 12 <= length) {
        size_t n = be32(png + pos);
        uint8_t *kind = png + pos + 4, *data = png + pos + 8;
        CHECK(n <= length - pos - 12);
        CHECK((uint32_t)mz_crc32(MZ_CRC32_INIT, kind, n + 4) == be32(data + n));
        if (memcmp(kind, "IHDR", 4) == 0) {
            CHECK(n == 13 && be32(data) == w && be32(data + 4) == h);
            CHECK(data[8] == 8 && data[9] == 6 && data[10] == 0 && data[11] == 0 && data[12] == 0);
        } else if (memcmp(kind, "IDAT", 4) == 0) {
            /* Compact in place only after checking this chunk's original CRC. */
            memmove(png + packed, data, n);
            packed += n;
            ++chunks;
        } else if (memcmp(kind, "IEND", 4) == 0) {
            CHECK(n == 0 && pos + 12 == length);
            done = 1;
            break;
        }
        pos += n + 12;
    }
    CHECK(done && chunks && packed);
    CHECK(!streamed || chunks > 1);
    CHECK(!uniform || length < pixel_bytes / 16);
    CHECK(tinfl_decompress_mem_to_mem(raw, raw_bytes, png, packed, TINFL_FLAG_PARSE_ZLIB_HEADER) == raw_bytes);
    for (uint32_t y = 0; y < h; ++y) {
        CHECK(raw[(stride + 1) * y] == 0);
        CHECK(memcmp(raw + (stride + 1) * y + 1, pixels + stride * y, stride) == 0);
    }
    printf("png_writer: PASS %ux%u %u-bit %zu bytes, %zu IDAT chunks, identical RGBA\n",
           w, h, (unsigned)(sizeof(void *) * 8), length, chunks);
    rc = 0;
out:
    if (file) fclose(file);
    free(raw);
    free(png);
    free(pixels);
    remove(path);
    return rc;
}

int main(void) {
    const uint8_t pixel[4] = {12, 34, 56, 0};
    if (p4png_write_rgba("png_writer_invalid.png", 0, 1, pixel) != -1 ||
        p4png_write_rgba("png_writer_invalid.png", 1, 0, pixel) != -1 ||
        p4png_write_rgba("png_writer_invalid.png", 1, 1, NULL) != -1) {
        fprintf(stderr, "png_writer: invalid geometry/data accepted\n");
        return 1;
    }
    return roundtrip(1, 1, 0, 0) || roundtrip(17, 31, 0, 0) ||
           roundtrip(640, 448, 1, 0) || roundtrip(1024, 512, 0, 1);
}
