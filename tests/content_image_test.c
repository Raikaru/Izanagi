#include "p4content_image.h"
#include "../third_party/miniz/miniz.h"

#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); return 1; \
} } while (0)

typedef struct MemoryFile {
    const uint8_t *bytes;
    size_t size;
    unsigned reads;
    int fail;
} MemoryFile;

static int read_at(void *user, uint64_t offset, void *output, size_t size)
{
    MemoryFile *file = (MemoryFile *)user;
    ++file->reads;
    if (file->fail || offset > file->size || size > file->size - (size_t)offset) return -1;
    memcpy(output, file->bytes + (size_t)offset, size);
    return 0;
}

static void le16(uint8_t *p, uint16_t n) { p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8); }
static void le32(uint8_t *p, uint32_t n)
{
    p[0] = (uint8_t)n; p[1] = (uint8_t)(n >> 8);
    p[2] = (uint8_t)(n >> 16); p[3] = (uint8_t)(n >> 24);
}
static void le64(uint8_t *p, uint64_t n) { le32(p, (uint32_t)n); le32(p + 4, (uint32_t)(n >> 32)); }
static void be32(uint8_t *p, uint32_t n)
{
    p[0] = (uint8_t)(n >> 24); p[1] = (uint8_t)(n >> 16);
    p[2] = (uint8_t)(n >> 8); p[3] = (uint8_t)n;
}

static size_t png_chunk(uint8_t *out, const char *type, const uint8_t *data, size_t size)
{
    be32(out, (uint32_t)size);
    memcpy(out + 4, type, 4);
    if (size != 0) memcpy(out + 8, data, size);
    be32(out + 8 + size, (uint32_t)mz_crc32(0, out + 4, size + 4));
    return size + 12;
}

/* One original RGBA pixel, with an independently constructed stored DEFLATE
 * block and Adler-32, so the same fixture can exercise the real decoder. */
static size_t make_png(uint8_t *out, int animated)
{
    static const uint8_t signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    static const uint8_t deflate[] = {
        0x78, 0x01, 0x01, 0x05, 0x00, 0xfa, 0xff,
        0x00, 0x11, 0x22, 0x33, 0x44, 0x01, 0x59, 0x00, 0xab
    };
    uint8_t ihdr[13] = {0}, animation[8] = {0};
    size_t offset = 8;
    memcpy(out, signature, sizeof signature);
    be32(ihdr, 1); be32(ihdr + 4, 1);
    ihdr[8] = 8; ihdr[9] = 6;
    offset += png_chunk(out + offset, "IHDR", ihdr, sizeof ihdr);
    if (animated) {
        be32(animation, 1);
        offset += png_chunk(out + offset, "acTL", animation, sizeof animation);
    }
    offset += png_chunk(out + offset, "IDAT", deflate, sizeof deflate);
    offset += png_chunk(out + offset, "IEND", NULL, 0);
    return offset;
}

static size_t make_dds(uint8_t *out, int dx10)
{
    size_t offset = dx10 ? 148 : 128;
    memset(out, 0, 256);
    memcpy(out, "DDS ", 4);
    le32(out + 4, 124); le32(out + 8, 0x21007);
    le32(out + 12, 3); le32(out + 16, 5); le32(out + 28, 3);
    le32(out + 76, 32); le32(out + 80, 4);
    le32(out + 84, dx10 ? 0x30315844u : 0x31545844u);
    le32(out + 108, 0x401008);
    if (dx10) {
        le32(out + 128, 98); le32(out + 132, 3);
        le32(out + 140, 1); le32(out + 144, 1);
        return offset + 32 + 16 + 16; /* BC7, including full tiny blocks. */
    }
    return offset + 16 + 8 + 8; /* BC1 ceil(5/4) * ceil(3/4), then 2x1 and 1x1. */
}

static size_t kv_entry(uint8_t *out, const char *key, const char *value)
{
    size_t key_size = strlen(key) + 1, value_size = strlen(value) + 1;
    size_t size = key_size + value_size, padded = (size + 3) & ~(size_t)3;
    le32(out, (uint32_t)size);
    memcpy(out + 4, key, key_size);
    memcpy(out + 4 + key_size, value, value_size);
    memset(out + 4 + size, 0, padded - size);
    return padded + 4;
}

static size_t make_ktx2(uint8_t *out, int metadata)
{
    static const uint8_t signature[] = {0xab, 0x4b, 0x54, 0x58, 0x20, 0x32, 0x30, 0xbb, 13, 10, 26, 10};
    size_t data = 220;
    unsigned i;
    memset(out, 0, 512);
    memcpy(out, signature, sizeof signature);
    le32(out + 12, 37); le32(out + 16, 1);
    le32(out + 20, 3); le32(out + 24, 2);
    le32(out + 36, 1); le32(out + 40, 2);
    le32(out + 48, 128); le32(out + 52, 92);
    le32(out + 128, 92); le16(out + 136, 2); le16(out + 138, 88);
    out[140] = 1; out[141] = 1; out[142] = 1; out[148] = 4;
    for (i = 0; i < 4; ++i) {
        uint8_t *sample = out + 156 + i * 16;
        le16(sample, (uint16_t)(i * 8)); sample[2] = 7;
        sample[3] = (uint8_t)(i == 3 ? 15 : i);
        le32(sample + 12, 255);
    }
    if (metadata) {
        size_t size = kv_entry(out + data, "KTXorientation", "lu");
        size += kv_entry(out + data + size, "KTXswizzle", "bgra");
        le32(out + 56, (uint32_t)data); le32(out + 60, (uint32_t)size);
        data += size;
    }
    /* Smallest mip first, as the KTX2 writer specifies. */
    le64(out + 104, data); le64(out + 112, 4); le64(out + 120, 4);
    le64(out + 80, data + 4); le64(out + 88, 24); le64(out + 96, 24);
    for (i = 0; i < 28; ++i) out[data + i] = (uint8_t)(11 + i * 37);
    return data + 28;
}

static int rejected(P4ContentImageInput *input, P4ContentImageInfo *info)
{
    uint8_t before[sizeof *info];
    char error[2] = {'x', 'x'};
    memcpy(before, info, sizeof before);
    CHECK(p4_content_image_probe(input, info, error, sizeof error) == -1);
    CHECK(error[0] != 0 && error[1] == 0);
    CHECK(memcmp(before, info, sizeof before) == 0);
    return 0;
}

static int containers(void)
{
    uint8_t bytes[512], mip[64];
    MemoryFile file = {bytes, 0, 0, 0};
    P4ContentImageInput input = {&file, 0, read_at};
    P4ContentImageInfo info;
    char error[192];
    memset(&info, 0xa5, sizeof info);
    file.size = (size_t)(input.size = make_png(bytes, 0));
    CHECK(p4_content_image_probe(&input, &info, error, sizeof error) == 0);
    CHECK(info.container == P4_IMAGE_PNG && info.width == 1 && info.height == 1);
    CHECK(info.flags & P4_IMAGE_CHANNEL_A);
    bytes[29] ^= 1;
    CHECK(rejected(&input, &info) == 0);
    file.size = (size_t)(input.size = make_png(bytes, 1));
    CHECK(rejected(&input, &info) == 0);
    file.size = (size_t)(input.size = make_png(bytes, 0));
    bytes[file.size++] = 0; ++input.size;
    CHECK(rejected(&input, &info) == 0);

    file.size = (size_t)(input.size = make_dds(bytes, 0));
    CHECK(p4_content_image_probe(&input, &info, error, sizeof error) == 0);
    CHECK(info.container == P4_IMAGE_DDS && info.format == P4_IMAGE_BC1 && info.level_count == 3);
    CHECK(info.levels[0].size == 16 && info.levels[1].offset == 144 && info.levels[2].offset == 152);
    memset(mip, 0xa5, sizeof mip);
    CHECK(p4_content_image_read_level(&input, &info, 0, mip, 15, error, sizeof error) == -1);
    CHECK(mip[0] == 0xa5);
    CHECK(p4_content_image_read_level(&input, &info, 0, mip, sizeof mip, error, sizeof error) == 0);
    CHECK(memcmp(mip, bytes + 128, 16) == 0);
    --input.size;
    CHECK(rejected(&input, &info) == 0);
    file.size = (size_t)(input.size = make_dds(bytes, 1));
    CHECK(p4_content_image_probe(&input, &info, error, sizeof error) == 0);
    CHECK(info.format == P4_IMAGE_BC7 && info.levels[0].size == 32 && info.levels[2].size == 16);
    le32(bytes + 140, 2);
    CHECK(rejected(&input, &info) == 0);
    le32(bytes + 140, 1); le32(bytes + 136, 4);
    CHECK(rejected(&input, &info) == 0);
    le32(bytes + 136, 0); le32(bytes + 128, 95); /* BC6H/HDR is not RGBA8. */
    CHECK(rejected(&input, &info) == 0);

    file.size = (size_t)(input.size = make_ktx2(bytes, 0));
    CHECK(p4_content_image_probe(&input, &info, error, sizeof error) == 0);
    CHECK(info.container == P4_IMAGE_KTX2 && info.format == P4_IMAGE_RGBA8 && info.full_level_count == 2);
    CHECK(info.levels[0].offset == 224 && info.levels[1].offset == 220);
    CHECK(p4_content_image_read_level(&input, &info, 1, mip, sizeof mip, error, sizeof error) == 0);
    CHECK(memcmp(mip, bytes + 220, 4) == 0);
    le64(bytes + 104, 224); /* Two valid-size ranges cannot alias. */
    CHECK(rejected(&input, &info) == 0);
    file.size = (size_t)(input.size = make_ktx2(bytes, 0));
    le64(bytes + 80, UINT64_MAX - 1);
    CHECK(rejected(&input, &info) == 0);
    file.size = (size_t)(input.size = make_ktx2(bytes, 0));
    le32(bytes + 48, 80);
    CHECK(rejected(&input, &info) == 0);
    file.size = (size_t)(input.size = make_ktx2(bytes, 0));
    le32(bytes + 32, 1);
    CHECK(rejected(&input, &info) == 0);
    file.size = (size_t)(input.size = make_ktx2(bytes, 0));
    le32(bytes + 40, 0); /* Legal only for an uncompressed base image. */
    CHECK(p4_content_image_probe(&input, &info, error, sizeof error) == 0 && info.level_count == 1);
    file.size = (size_t)(input.size = make_ktx2(bytes, 1));
    CHECK(p4_content_image_probe(&input, &info, error, sizeof error) == 0);
    CHECK((info.flags & (P4_IMAGE_FLIP_X | P4_IMAGE_FLIP_Y)) == (P4_IMAGE_FLIP_X | P4_IMAGE_FLIP_Y));
    CHECK(info.swizzle[0] == 2 && info.swizzle[1] == 1 && info.swizzle[2] == 0 && info.swizzle[3] == 3);
    le32(bytes + 12, 44); /* Compose the BGRA storage map with KTXswizzle. */
    le16(bytes + 156, 16); le16(bytes + 188, 0); /* Matching R/B DFD bit offsets. */
    CHECK(p4_content_image_probe(&input, &info, error, sizeof error) == 0);
    CHECK(info.swizzle[0] == 0 && info.swizzle[2] == 2);
    file.fail = 1;
    CHECK(rejected(&input, &info) == 0);
    return 0;
}

static int transforms(void)
{
    P4ContentImageInfo info;
    uint8_t pixels[32], original[32];
    unsigned i, mode;
    memset(&info, 0, sizeof info);
    info.swizzle[0] = 0; info.swizzle[1] = 1; info.swizzle[2] = 2; info.swizzle[3] = 3;
    info.premultiply_alpha = 3; info.premultiply_rgb_mask = 7;
    memset(original, 0xa5, sizeof original);
    for (i = 0; i < 6; ++i) {
        uint8_t *p = original + (i / 3) * 16 + (i % 3) * 4;
        p[0] = (uint8_t)(i + 1); p[1] = (uint8_t)(i + 21); p[2] = (uint8_t)(i + 41); p[3] = 255;
    }
    for (mode = 0; mode < 4; ++mode) {
        memcpy(pixels, original, sizeof pixels);
        info.flags = mode;
        CHECK(p4_content_image_transform_rgba(pixels, sizeof pixels, 3, 2, 16, &info) == 0);
        for (i = 0; i < 6; ++i) {
            unsigned x = i % 3, y = i / 3;
            unsigned sx = mode & P4_IMAGE_FLIP_X ? 2 - x : x;
            unsigned sy = mode & P4_IMAGE_FLIP_Y ? 1 - y : y;
            CHECK(memcmp(pixels + y * 16 + x * 4, original + sy * 16 + sx * 4, 4) == 0);
        }
        for (i = 12; i < 16; ++i) CHECK(pixels[i] == 0xa5 && pixels[i + 16] == 0xa5);
    }
    info.flags = P4_IMAGE_PREMULTIPLIED;
    info.swizzle[0] = 2; info.swizzle[2] = 0;
    pixels[0] = 32; pixels[1] = 64; pixels[2] = 128; pixels[3] = 128;
    pixels[4] = 255; pixels[5] = 255; pixels[6] = 255; pixels[7] = 0;
    CHECK(p4_content_image_transform_rgba(pixels, 8, 2, 1, 8, &info) == 0);
    CHECK(pixels[0] == 255 && pixels[1] == 128 && pixels[2] == 64 && pixels[3] == 128);
    CHECK(pixels[4] == 0 && pixels[5] == 0 && pixels[6] == 0 && pixels[7] == 0);
    info.flags = 0;
    info.swizzle[0] = P4_IMAGE_SWIZZLE_ONE; info.swizzle[1] = P4_IMAGE_SWIZZLE_ZERO;
    info.swizzle[2] = 0; info.swizzle[3] = P4_IMAGE_SWIZZLE_ONE;
    pixels[0] = 19;
    CHECK(p4_content_image_transform_rgba(pixels, 4, 1, 1, 4, &info) == 0);
    CHECK(pixels[0] == 255 && pixels[1] == 0 && pixels[2] == 19 && pixels[3] == 255);
    memcpy(original, pixels, sizeof original);
    info.swizzle[0] = 6;
    CHECK(p4_content_image_transform_rgba(pixels, sizeof pixels, 1, 1, 4, &info) == -1);
    CHECK(memcmp(original, pixels, sizeof original) == 0);
    CHECK(p4_content_image_transform_rgba(pixels, sizeof pixels, 1, 1, SIZE_MAX, &info) == -1);
    return 0;
}

int main(void)
{
    CHECK(p4_content_image_level_size(P4_IMAGE_BC1, 5, 3) == 16);
    CHECK(p4_content_image_level_size(P4_IMAGE_BC7, 1, 1) == 16);
    CHECK(p4_content_image_level_size(P4_IMAGE_RGBA8, 3, 2) == 24);
    CHECK(p4_content_image_format(P4_IMAGE_BASIS) == NULL);
    if (containers() != 0 || transforms() != 0) return 1;
    puts("texture container ranges, orientation, component/alpha conventions: OK");
    return 0;
}
