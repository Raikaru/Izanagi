#include "p4content_image.h"
#include "../../../third_party/miniz/miniz.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Container layout: Microsoft DDS programming guide and Khronos KTX 2.0.
 * https://learn.microsoft.com/windows/win32/direct3ddds/dds-header
 * https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html
 * These are byte readers, not casts of untrusted data to native structs. */
static const P4ContentImageFormatInfo formats[] = {
    {"rgba8", 1, 1, 4, 0},
    {"bc1", 4, 4, 8, 1}, {"bc2", 4, 4, 16, 1},
    {"bc3", 4, 4, 16, 1}, {"bc7", 4, 4, 16, 1},
    {"etc2-rgb", 4, 4, 8, 1}, {"etc2-rgba", 4, 4, 16, 1},
    {"etc2-a1", 4, 4, 8, 1},
    {"astc-4x4", 4, 4, 16, 1}, {"astc-5x4", 5, 4, 16, 1},
    {"astc-5x5", 5, 5, 16, 1}, {"astc-6x5", 6, 5, 16, 1},
    {"astc-6x6", 6, 6, 16, 1}, {"astc-8x5", 8, 5, 16, 1},
    {"astc-8x6", 8, 6, 16, 1}, {"astc-8x8", 8, 8, 16, 1},
    {"astc-10x5", 10, 5, 16, 1}, {"astc-10x6", 10, 6, 16, 1},
    {"astc-10x8", 10, 8, 16, 1}, {"astc-10x10", 10, 10, 16, 1},
    {"astc-12x10", 12, 10, 16, 1}, {"astc-12x12", 12, 12, 16, 1}
};

const P4ContentImageFormatInfo *p4_content_image_format(P4ContentImageFormat format)
{
    return (unsigned)format < sizeof formats / sizeof formats[0] ? &formats[format] : NULL;
}

static int image_dimensions(uint32_t width, uint32_t height)
{
    return width != 0 && height != 0 && width <= P4_CONTENT_IMAGE_LIMIT &&
           height <= P4_CONTENT_IMAGE_LIMIT;
}

uint64_t p4_content_image_level_size(P4ContentImageFormat format,
                                    uint32_t width, uint32_t height)
{
    const P4ContentImageFormatInfo *f = p4_content_image_format(format);
    if (f == NULL || !image_dimensions(width, height)) return 0;
    return ((uint64_t)width + f->block_width - 1) / f->block_width *
           (((uint64_t)height + f->block_height - 1) / f->block_height) * f->bytes_per_block;
}

static int image_error(char *error, size_t size, const char *format, ...)
{
    if (error != NULL && size != 0) {
        va_list args;
        va_start(args, format);
        vsnprintf(error, size, format, args);
        va_end(args);
    }
    return -1;
}

static uint16_t little16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}

static uint32_t little32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static uint64_t little64(const uint8_t *p)
{
    return little32(p) | (uint64_t)little32(p + 4) << 32;
}

static uint32_t big32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static int range(const P4ContentImageInput *input, uint64_t offset, uint64_t size)
{
    return offset <= input->size && size <= input->size - offset;
}

static int read_exact(const P4ContentImageInput *input, uint64_t offset, void *out, size_t size)
{
    return range(input, offset, size) &&
           (size == 0 || input->read_at(input->user, offset, out, size) == 0) ? 0 : -1;
}

static unsigned full_levels(uint32_t width, uint32_t height)
{
    unsigned result = 1;
    while (width > 1 || height > 1) {
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
        ++result;
    }
    return result;
}

static int png_depth(uint8_t depth, uint8_t type)
{
    if (type == 0) return depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16;
    if (type == 3) return depth == 1 || depth == 2 || depth == 4 || depth == 8;
    return (type == 2 || type == 4 || type == 6) && (depth == 8 || depth == 16);
}

static int probe_png(const P4ContentImageInput *input, P4ContentImageInfo *info,
                      char *error, size_t error_size)
{
    uint8_t header[33], chunk[8];
    uint64_t offset = 8;
    unsigned chunks = 0;
    int have_data = 0, data_ended = 0;
    if (read_exact(input, 0, header, sizeof header) != 0)
        return image_error(error, error_size, "truncated PNG header");
    if (big32(header + 8) != 13 || memcmp(header + 12, "IHDR", 4) != 0 ||
        (uint32_t)mz_crc32(0, header + 12, 17) != big32(header + 29))
        return image_error(error, error_size, "invalid PNG IHDR/CRC");
    info->width = big32(header + 16);
    info->height = big32(header + 20);
    if (!image_dimensions(info->width, info->height) || !png_depth(header[24], header[25]) ||
        header[26] != 0 || header[27] != 0 || header[28] > 1)
        return image_error(error, error_size, "unsupported PNG dimensions/color encoding");
    info->container = P4_IMAGE_PNG;
    info->format = P4_IMAGE_RGBA8;
    info->level_count = 1;
    if (header[25] == 4 || header[25] == 6) info->flags |= P4_IMAGE_CHANNEL_A;
    if (header[28] != 0) info->flags |= P4_IMAGE_INTERLACED;
    while (offset < input->size) {
        uint32_t length;
        uint64_t span;
        unsigned i;
        if (++chunks > 4096 || read_exact(input, offset, chunk, sizeof chunk) != 0)
            return image_error(error, error_size, "PNG chunk limit or truncated chunk header");
        length = big32(chunk);
        span = (uint64_t)length + 12;
        if (length > 0x7fffffffu || !range(input, offset, span))
            return image_error(error, error_size, "PNG chunk extends past the file");
        for (i = 4; i < 8; ++i) {
            if (!((chunk[i] >= 'A' && chunk[i] <= 'Z') || (chunk[i] >= 'a' && chunk[i] <= 'z')))
                return image_error(error, error_size, "invalid PNG chunk name");
        }
        if (chunk[6] & 32) return image_error(error, error_size, "invalid PNG reserved chunk bit");
        if (memcmp(chunk + 4, "acTL", 4) == 0 || memcmp(chunk + 4, "fcTL", 4) == 0 ||
            memcmp(chunk + 4, "fdAT", 4) == 0)
            return image_error(error, error_size, "animated PNG is not a replacement image; key each game frame");
        if (memcmp(chunk + 4, "IDAT", 4) == 0) {
            if (data_ended) return image_error(error, error_size, "nonconsecutive PNG IDAT chunks");
            have_data = 1;
        } else if (have_data) {
            data_ended = 1;
        }
        if (memcmp(chunk + 4, "IHDR", 4) == 0 && offset != 8)
            return image_error(error, error_size, "duplicate PNG IHDR");
        if ((chunk[4] & 32) == 0 && memcmp(chunk + 4, "IHDR", 4) != 0 &&
            memcmp(chunk + 4, "PLTE", 4) != 0 && memcmp(chunk + 4, "IDAT", 4) != 0 &&
            memcmp(chunk + 4, "IEND", 4) != 0)
            return image_error(error, error_size, "unsupported critical PNG chunk");
        if (memcmp(chunk + 4, "tRNS", 4) == 0) info->flags |= P4_IMAGE_CHANNEL_A;
        if (memcmp(chunk + 4, "sRGB", 4) == 0) info->flags |= P4_IMAGE_SRGB_HINT;
        offset += span;
        if (memcmp(chunk + 4, "IEND", 4) == 0) {
            if (length != 0 || !have_data || offset != input->size)
                return image_error(error, error_size, "invalid PNG IEND or trailing data");
            info->levels[0].width = info->width;
            info->levels[0].height = info->height;
            info->levels[0].row_bytes = info->width * 4;
            info->levels[0].size = input->size;
            info->levels[0].uncompressed_size = (uint64_t)info->width * info->height * 4;
            return 0;
        }
    }
    return image_error(error, error_size, "PNG has no IEND");
}

static int dxgi_format(uint32_t format, P4ContentImageInfo *info)
{
    switch (format) {
    case 27: case 28: case 29: info->format = P4_IMAGE_RGBA8; break;
    case 87: case 90: case 91:
    case 88: case 92: case 93:
        info->format = P4_IMAGE_RGBA8;
        info->swizzle[0] = 2; info->swizzle[2] = 0;
        if (format == 88 || format == 92 || format == 93) info->swizzle[3] = P4_IMAGE_SWIZZLE_ONE;
        break;
    case 70: case 71: case 72: info->format = P4_IMAGE_BC1; break;
    case 73: case 74: case 75: info->format = P4_IMAGE_BC2; break;
    case 76: case 77: case 78: info->format = P4_IMAGE_BC3; break;
    case 97: case 98: case 99: info->format = P4_IMAGE_BC7; break;
    default: return -1;
    }
    if (format == 29 || format == 91 || format == 93 || format == 72 ||
        format == 75 || format == 78 || format == 99) info->flags |= P4_IMAGE_SRGB_HINT;
    return 0;
}

static int probe_dds(const P4ContentImageInput *input, P4ContentImageInfo *info,
                      char *error, size_t error_size)
{
    uint8_t header[148];
    uint32_t flags, pixel_flags, fourcc, pitch, width, height, i;
    uint64_t offset = 128;
    const P4ContentImageFormatInfo *format;
    if (read_exact(input, 0, header, 128) != 0 || little32(header + 4) != 124 ||
        little32(header + 76) != 32) return image_error(error, error_size, "invalid DDS header");
    flags = little32(header + 8);
    info->height = little32(header + 12);
    info->width = little32(header + 16);
    pitch = little32(header + 20);
    info->level_count = little32(header + 28);
    if (info->level_count == 0) info->level_count = 1;
    if (!image_dimensions(info->width, info->height) ||
        info->level_count > full_levels(info->width, info->height) ||
        little32(header + 24) > 1 || (flags & 0x800000u) ||
        (little32(header + 112) & (0xfe00u | 0x200000u)))
        return image_error(error, error_size, "DDS must be a bounded non-array 2D image with valid mips");
    pixel_flags = little32(header + 80);
    fourcc = little32(header + 84);
    if (pixel_flags & 4) {
        if (fourcc == 0x30315844u) { /* DX10 */
            uint32_t alpha_mode;
            if (read_exact(input, 128, header + 128, 20) != 0 ||
                little32(header + 132) != 3 || (little32(header + 136) & 4) ||
                little32(header + 140) != 1)
                return image_error(error, error_size, "unsupported DDS DX10 dimension/array/cubemap");
            if (dxgi_format(little32(header + 128), info) != 0)
                return image_error(error, error_size, "unsupported DDS DXGI format %u", little32(header + 128));
            alpha_mode = little32(header + 144);
            if (alpha_mode > 3) return image_error(error, error_size, "unsupported DDS alpha mode");
            if (alpha_mode == 2) info->flags |= P4_IMAGE_PREMULTIPLIED;
            if (alpha_mode == 3) info->swizzle[3] = P4_IMAGE_SWIZZLE_ONE;
            offset = 148;
        } else if (fourcc == 0x31545844u) {
            info->format = P4_IMAGE_BC1;
        } else if (fourcc == 0x32545844u || fourcc == 0x33545844u) {
            info->format = P4_IMAGE_BC2;
            if (fourcc == 0x32545844u) info->flags |= P4_IMAGE_PREMULTIPLIED;
        } else if (fourcc == 0x34545844u || fourcc == 0x35545844u) {
            info->format = P4_IMAGE_BC3;
            if (fourcc == 0x34545844u) info->flags |= P4_IMAGE_PREMULTIPLIED;
        } else {
            return image_error(error, error_size, "unsupported DDS FourCC");
        }
    } else {
        uint32_t red = little32(header + 92), green = little32(header + 96);
        uint32_t blue = little32(header + 100), alpha = little32(header + 104);
        if (!(pixel_flags & 0x40) || little32(header + 88) != 32 || green != 0xff00u ||
            (alpha != 0 && alpha != 0xff000000u))
            return image_error(error, error_size, "DDS requires RGBA/BGRA8 or BC1/2/3/7");
        info->format = P4_IMAGE_RGBA8;
        if (red == 0xff0000u && blue == 0xffu) {
            info->swizzle[0] = 2; info->swizzle[2] = 0;
        } else if (red != 0xffu || blue != 0xff0000u) {
            return image_error(error, error_size, "unsupported DDS channel masks");
        }
        if (alpha == 0) info->swizzle[3] = P4_IMAGE_SWIZZLE_ONE;
    }
    info->container = P4_IMAGE_DDS;
    info->flags |= P4_IMAGE_CHANNEL_A;
    info->premultiply_alpha = info->swizzle[3];
    if (info->premultiply_alpha == P4_IMAGE_SWIZZLE_ONE)
        info->flags &= ~(uint32_t)P4_IMAGE_PREMULTIPLIED;
    format = p4_content_image_format(info->format);
    width = info->width; height = info->height;
    for (i = 0; i < info->level_count; ++i) {
        P4ContentImageLevel *level = &info->levels[i];
        level->width = width; level->height = height;
        level->row_bytes = (width + format->block_width - 1) / format->block_width * format->bytes_per_block;
        if (!format->compressed && i == 0 && (flags & 8) && pitch != 0) {
            if (pitch < level->row_bytes) return image_error(error, error_size, "DDS pitch is shorter than a row");
            level->row_bytes = pitch;
        }
        level->offset = offset;
        level->size = (uint64_t)level->row_bytes * ((height + format->block_height - 1) / format->block_height);
        level->uncompressed_size = level->size;
        if (!range(input, offset, level->size)) return image_error(error, error_size, "truncated DDS mip %u", i);
        offset += level->size;
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
    }
    if (offset != input->size) return image_error(error, error_size, "unexpected trailing DDS data");
    return 0;
}

typedef struct ImageRange { uint64_t offset, size; } ImageRange;

static int add_range(const P4ContentImageInput *input, ImageRange *regions, unsigned *count,
                      uint64_t offset, uint64_t size, uint64_t alignment)
{
    unsigned i;
    if (size == 0) return offset == 0 ? 0 : -1;
    if (!range(input, offset, size) || offset % alignment != 0) return -1;
    for (i = 0; i < *count; ++i) {
        if (offset < regions[i].offset + regions[i].size &&
            regions[i].offset < offset + size) return -1;
    }
    regions[*count].offset = offset;
    regions[(*count)++].size = size;
    return 0;
}

static int vk_format(uint32_t vk, P4ContentImageInfo *info, uint8_t *model)
{
    if (vk == 37 || vk == 43 || vk == 44 || vk == 50) {
        info->format = P4_IMAGE_RGBA8;
        *model = 1;
        if (vk == 44 || vk == 50) { info->swizzle[0] = 2; info->swizzle[2] = 0; }
        if (vk == 43 || vk == 50) info->flags |= P4_IMAGE_SRGB_HINT;
    } else if (vk >= 131 && vk <= 138) {
        info->format = vk <= 134 ? P4_IMAGE_BC1 : vk <= 136 ? P4_IMAGE_BC2 : P4_IMAGE_BC3;
        *model = vk <= 134 ? 128 : vk <= 136 ? 129 : 130;
        if (vk == 131 || vk == 132) info->swizzle[3] = P4_IMAGE_SWIZZLE_ONE;
        if (!(vk & 1)) info->flags |= P4_IMAGE_SRGB_HINT;
    } else if (vk == 145 || vk == 146) {
        info->format = P4_IMAGE_BC7; *model = 134;
        if (vk == 146) info->flags |= P4_IMAGE_SRGB_HINT;
    } else if (vk >= 147 && vk <= 152) {
        info->format = vk <= 148 ? P4_IMAGE_ETC2_RGB : vk <= 150 ? P4_IMAGE_ETC2_A1 : P4_IMAGE_ETC2_RGBA;
        *model = 161;
        if (info->format == P4_IMAGE_ETC2_RGB) info->swizzle[3] = P4_IMAGE_SWIZZLE_ONE;
        if (!(vk & 1)) info->flags |= P4_IMAGE_SRGB_HINT;
    } else if (vk >= 157 && vk <= 184) {
        /* Vulkan's ASTC sequence places 8x8 before 10x5/10x6. */
        static const uint8_t astc[] = {
            P4_IMAGE_ASTC_4x4, P4_IMAGE_ASTC_5x4, P4_IMAGE_ASTC_5x5,
            P4_IMAGE_ASTC_6x5, P4_IMAGE_ASTC_6x6, P4_IMAGE_ASTC_8x5,
            P4_IMAGE_ASTC_8x6, P4_IMAGE_ASTC_8x8, P4_IMAGE_ASTC_10x5,
            P4_IMAGE_ASTC_10x6, P4_IMAGE_ASTC_10x8, P4_IMAGE_ASTC_10x10,
            P4_IMAGE_ASTC_12x10, P4_IMAGE_ASTC_12x12
        };
        info->format = (P4ContentImageFormat)astc[(vk - 157) / 2]; *model = 162;
        if (!(vk & 1)) info->flags |= P4_IMAGE_SRGB_HINT;
    } else {
        return -1;
    }
    if (info->format != P4_IMAGE_ETC2_RGB) info->flags |= P4_IMAGE_CHANNEL_A;
    return 0;
}

static int basis_channels(P4ContentImageInfo *info, const uint8_t *dfd, unsigned samples)
{
    unsigned first = dfd[31] & 15, second = samples > 1 ? dfd[47] & 15 : 0;
    if (info->basis_model == 163) {
        if ((samples != 1 && samples != 2) || (first != 0 && first != 3) ||
            (samples == 2 && second != (first == 0 ? 15u : 4u))) return -1;
        if (samples == 2) info->flags |= P4_IMAGE_CHANNEL_A;
        if (first == 0) {
            info->swizzle[3] = samples == 2 ? 3 : P4_IMAGE_SWIZZLE_ONE;
        } else {
            info->swizzle[1] = samples == 2 ? 3 : P4_IMAGE_SWIZZLE_ZERO;
            info->swizzle[2] = P4_IMAGE_SWIZZLE_ZERO;
            info->swizzle[3] = P4_IMAGE_SWIZZLE_ONE;
        }
    } else {
        if (samples != 1 || (first != 0 && first != 3 && first != 4 && first != 5 && first != 6)) return -1;
        if (first == 3 || first == 5) info->flags |= P4_IMAGE_CHANNEL_A;
        if (first != 3) info->swizzle[3] = P4_IMAGE_SWIZZLE_ONE;
        if (first == 4 || first == 5 || first == 6) {
            if (first != 6) info->swizzle[1] = first == 5 ? 3 : P4_IMAGE_SWIZZLE_ZERO;
            info->swizzle[2] = P4_IMAGE_SWIZZLE_ZERO;
        }
    }
    return 0;
}

static int read_key(const P4ContentImageInput *input, uint64_t offset, uint32_t size,
                     char prefix[32], uint32_t *length)
{
    uint8_t buffer[64];
    uint32_t position = 0;
    memset(prefix, 0, 32);
    while (position < size) {
        size_t count = size - position > sizeof buffer ? sizeof buffer : size - position;
        const uint8_t *end;
        size_t bytes, copy;
        if (read_exact(input, offset + position, buffer, count) != 0) return -1;
        end = (const uint8_t *)memchr(buffer, 0, count);
        bytes = end != NULL ? (size_t)(end - buffer) : count;
        copy = position < 31 ? 31 - position : 0;
        if (copy > bytes) copy = bytes;
        if (copy != 0) memcpy(prefix + position, buffer, copy);
        if (end != NULL) { *length = position + (uint32_t)bytes; return *length != 0 ? 0 : -1; }
        position += (uint32_t)count;
    }
    return -1;
}

static int ktx_keys(const P4ContentImageInput *input, P4ContentImageInfo *info,
                     const uint8_t base[4], char *error, size_t error_size)
{
    static const char channels[] = "rgba01";
    uint64_t offset = info->kvd_offset, end = offset + info->kvd_size;
    unsigned entries = 0;
    int orientation = 0, swizzle = 0;
    while (offset < end) {
        uint8_t length_bytes[4], value[5];
        char key[32];
        uint32_t size, key_length, value_length;
        uint64_t padded, value_offset;
        if (++entries > 1024 || end - offset < 4 || read_exact(input, offset, length_bytes, 4) != 0)
            return image_error(error, error_size, "invalid KTX2 key/value index");
        size = little32(length_bytes);
        padded = ((uint64_t)size + 3) & ~(uint64_t)3;
        if (size == 0 || padded > end - offset - 4 ||
            read_key(input, offset + 4, size, key, &key_length) != 0)
            return image_error(error, error_size, "invalid KTX2 key/value entry");
        value_length = size - key_length - 1;
        value_offset = offset + 4 + key_length + 1;
        if (key_length == 14 && memcmp(key, "KTXorientation", 14) == 0) {
            if (orientation++ || value_length != 3 || read_exact(input, value_offset, value, 3) != 0 ||
                (value[0] != 'r' && value[0] != 'l') || (value[1] != 'd' && value[1] != 'u') || value[2] != 0)
                return image_error(error, error_size, "invalid/duplicate KTX2 orientation");
            if (value[0] == 'l') info->flags |= P4_IMAGE_FLIP_X;
            if (value[1] == 'u') info->flags |= P4_IMAGE_FLIP_Y;
        } else if (key_length == 10 && memcmp(key, "KTXswizzle", 10) == 0) {
            unsigned i;
            if (swizzle++ || value_length != 5 || read_exact(input, value_offset, value, 5) != 0 || value[4] != 0)
                return image_error(error, error_size, "invalid/duplicate KTX2 swizzle");
            for (i = 0; i < 4; ++i) {
                const char *channel = strchr(channels, value[i]);
                unsigned component;
                if (value[i] == 0 || channel == NULL) return image_error(error, error_size, "invalid KTX2 swizzle channel");
                component = (unsigned)(channel - channels);
                info->swizzle[i] = component < 4 ? base[component] : (uint8_t)component;
            }
        }
        offset += 4 + padded;
    }
    return 0;
}

static int probe_ktx2(const P4ContentImageInput *input, P4ContentImageInfo *info,
                       char *error, size_t error_size)
{
    uint8_t header[80], dfd[60], entry[24], base[4], model = 0;
    ImageRange regions[P4_CONTENT_MIP_LIMIT + 4];
    unsigned region_count = 0, i, samples;
    uint32_t vk, width, height, declared_levels, block_bytes, block_width, block_height;
    const P4ContentImageFormatInfo *format;
    if (read_exact(input, 0, header, sizeof header) != 0)
        return image_error(error, error_size, "truncated KTX2 header");
    vk = little32(header + 12);
    info->width = little32(header + 20); info->height = little32(header + 24);
    declared_levels = little32(header + 40);
    info->level_count = declared_levels != 0 ? declared_levels : 1;
    info->supercompression = little32(header + 44);
    if (!image_dimensions(info->width, info->height) || little32(header + 16) != 1 ||
        little32(header + 28) != 0 || little32(header + 32) != 0 || little32(header + 36) != 1 ||
        info->level_count > full_levels(info->width, info->height) || info->supercompression > 2)
        return image_error(error, error_size, "unsupported KTX2 dimensions/array/cubemap/mips/supercompression");
    info->container = P4_IMAGE_KTX2;
    if (vk == 0) info->format = P4_IMAGE_BASIS;
    else if (vk_format(vk, info, &model) != 0)
        return image_error(error, error_size, "unsupported KTX2 VkFormat %u", vk);
    if (declared_levels == 0 && info->format != P4_IMAGE_RGBA8)
        return image_error(error, error_size, "block-compressed KTX2 cannot have levelCount zero");
    info->dfd_offset = little32(header + 48); info->dfd_size = little32(header + 52);
    info->kvd_offset = little32(header + 56); info->kvd_size = little32(header + 60);
    info->sgd_offset = little64(header + 64); info->sgd_size = little64(header + 72);
    if (info->dfd_size < 44 || info->dfd_size > 4096 || info->kvd_size > P4_CONTENT_KTX_METADATA_LIMIT ||
        add_range(input, regions, &region_count, 0, 80 + (uint64_t)info->level_count * 24, 1) != 0 ||
        add_range(input, regions, &region_count, info->dfd_offset, info->dfd_size, 4) != 0 ||
        add_range(input, regions, &region_count, info->kvd_offset, info->kvd_size, 4) != 0 ||
        add_range(input, regions, &region_count, info->sgd_offset, info->sgd_size, 8) != 0 ||
        (info->supercompression == 1 ? info->sgd_size < 20 : info->sgd_size != 0))
        return image_error(error, error_size, "invalid/overlapping KTX2 metadata ranges");
    memset(dfd, 0, sizeof dfd);
    if (read_exact(input, info->dfd_offset, dfd, info->dfd_size < sizeof dfd ? (size_t)info->dfd_size : sizeof dfd) != 0 ||
        little32(dfd) != info->dfd_size || little32(dfd + 4) != 0 || little16(dfd + 8) != 2 ||
        (uint32_t)little16(dfd + 10) + 4 != info->dfd_size ||
        (little16(dfd + 10) - 24) % 16 != 0 || dfd[18] != 0 || dfd[19] != 0 ||
        dfd[14] > 2 || (dfd[15] & ~1u))
        return image_error(error, error_size, "unsupported KTX2 basic data-format descriptor");
    samples = (little16(dfd + 10) - 24) / 16;
    if (vk == 0) {
        info->basis_model = dfd[12];
        if ((info->basis_model != 163 && info->basis_model != 166) ||
            (info->basis_model == 163 ? info->supercompression != 1 : info->supercompression == 1) ||
            basis_channels(info, dfd, samples) != 0)
            return image_error(error, error_size, "unsupported KTX2 Basis model/channel layout");
        block_width = block_height = 4;
        block_bytes = info->basis_model == 163 ? 8 : 16;
    } else {
        if (dfd[12] != model || info->supercompression == 1 ||
            ((dfd[14] == 2) != ((info->flags & P4_IMAGE_SRGB_HINT) != 0)))
            return image_error(error, error_size, "inconsistent KTX2 format/transfer/supercompression");
        format = p4_content_image_format(info->format);
        block_width = format->block_width; block_height = format->block_height;
        block_bytes = format->bytes_per_block;
    }
    if (dfd[16] != block_width - 1 || dfd[17] != block_height - 1 ||
        (dfd[20] != block_bytes && !(info->supercompression != 0 && dfd[20] == 0)) ||
        (info->basis_model == 163 && samples == 2 ? dfd[21] != 0 && dfd[21] != 8 : dfd[21] != 0))
        return image_error(error, error_size, "inconsistent KTX2 block dimensions/planes");
    for (i = 22; i < 28; ++i) {
        if (dfd[i] != 0) return image_error(error, error_size, "unsupported KTX2 extra data plane");
    }
    if (dfd[14] == 2) info->flags |= P4_IMAGE_SRGB_HINT;
    info->premultiply_alpha = info->swizzle[3];
    info->premultiply_rgb_mask = 0;
    for (i = 0; i < 3; ++i) {
        if (info->swizzle[i] < 4) info->premultiply_rgb_mask |= (uint8_t)(1u << info->swizzle[i]);
    }
    if ((dfd[15] & 1) && info->premultiply_alpha != P4_IMAGE_SWIZZLE_ONE)
        info->flags |= P4_IMAGE_PREMULTIPLIED;
    memcpy(base, info->swizzle, sizeof base);
    if (ktx_keys(input, info, base, error, error_size) != 0) return -1;
    width = info->width; height = info->height;
    for (i = 0; i < info->level_count; ++i) {
        P4ContentImageLevel *level = &info->levels[i];
        uint64_t expected, alignment = info->supercompression == 0 ? block_bytes : 1;
        if (read_exact(input, 80 + (uint64_t)i * 24, entry, sizeof entry) != 0)
            return image_error(error, error_size, "truncated KTX2 mip index");
        level->width = width; level->height = height;
        level->row_bytes = (width + block_width - 1) / block_width * block_bytes;
        level->offset = little64(entry); level->size = little64(entry + 8);
        level->uncompressed_size = little64(entry + 16);
        expected = (uint64_t)level->row_bytes * ((height + block_height - 1) / block_height);
        if (alignment < 4 && info->supercompression == 0) alignment = 4;
        if (level->size == 0 || add_range(input, regions, &region_count, level->offset, level->size, alignment) != 0 ||
            (info->supercompression == 1 ? level->uncompressed_size != 0 : level->uncompressed_size != expected) ||
            (info->supercompression == 0 && level->size != expected))
            return image_error(error, error_size, "invalid/overlapping KTX2 mip %u", i);
        width = width > 1 ? width / 2 : 1;
        height = height > 1 ? height / 2 : 1;
    }
    return 0;
}

int p4_content_image_probe(const P4ContentImageInput *input,
                           P4ContentImageInfo *info, char *error, size_t error_size)
{
    static const uint8_t png[] = {137, 80, 78, 71, 13, 10, 26, 10};
    static const uint8_t ktx2[] = {0xab, 0x4b, 0x54, 0x58, 0x20, 0x32, 0x30, 0xbb, 13, 10, 26, 10};
    uint8_t magic[12];
    P4ContentImageInfo result;
    int status;
    if (error != NULL && error_size != 0) error[0] = 0;
    if (input == NULL || info == NULL || input->read_at == NULL || input->size > INT64_MAX ||
        read_exact(input, 0, magic, sizeof magic) != 0)
        return image_error(error, error_size, "invalid/truncated texture input");
    memset(&result, 0, sizeof result);
    result.swizzle[0] = 0; result.swizzle[1] = 1;
    result.swizzle[2] = 2; result.swizzle[3] = 3;
    result.premultiply_alpha = 3; result.premultiply_rgb_mask = 7;
    if (memcmp(magic, png, sizeof png) == 0) status = probe_png(input, &result, error, error_size);
    else if (memcmp(magic, "DDS ", 4) == 0) status = probe_dds(input, &result, error, error_size);
    else if (memcmp(magic, ktx2, sizeof ktx2) == 0) status = probe_ktx2(input, &result, error, error_size);
    else return image_error(error, error_size, "texture must be PNG, DDS or KTX2 (magic mismatch)");
    if (status != 0) return -1;
    result.full_level_count = full_levels(result.width, result.height);
    *info = result;
    return 0;
}

int p4_content_image_read_level(const P4ContentImageInput *input,
                                const P4ContentImageInfo *info, unsigned level,
                                void *output, size_t capacity,
                                char *error, size_t error_size)
{
    if (input == NULL || input->read_at == NULL || info == NULL || output == NULL ||
        level >= info->level_count || level >= P4_CONTENT_MIP_LIMIT || info->levels[level].size > capacity ||
        read_exact(input, info->levels[level].offset, output, (size_t)info->levels[level].size) != 0)
        return image_error(error, error_size, "invalid destination or failed texture mip read");
    return 0;
}

static void transformed(uint8_t output[4], const uint8_t source[4], const P4ContentImageInfo *info)
{
    uint8_t values[6];
    unsigned i;
    memcpy(values, source, 4);
    values[P4_IMAGE_SWIZZLE_ZERO] = 0; values[P4_IMAGE_SWIZZLE_ONE] = 255;
    if (info->flags & P4_IMAGE_PREMULTIPLIED) {
        unsigned alpha = values[info->premultiply_alpha];
        for (i = 0; i < 4; ++i) {
            if (info->premultiply_rgb_mask & (1u << i)) {
                unsigned color = alpha != 0 ? (values[i] * 255u + alpha / 2) / alpha : 0;
                values[i] = (uint8_t)(color > 255 ? 255 : color);
            }
        }
    }
    for (i = 0; i < 4; ++i) output[i] = values[info->swizzle[i]];
}

int p4_content_image_transform_rgba(uint8_t *pixels, size_t bytes,
                                    uint32_t width, uint32_t height, size_t stride,
                                    const P4ContentImageInfo *info)
{
    size_t index, count;
    unsigned i;
    if (pixels == NULL || info == NULL || !image_dimensions(width, height) || stride < (size_t)width * 4 ||
        stride > SIZE_MAX / height || bytes < stride * height || info->premultiply_alpha > 5 ||
        (info->premultiply_rgb_mask & ~15u)) return -1;
    for (i = 0; i < 4; ++i) if (info->swizzle[i] > 5) return -1;
    if (!(info->flags & (P4_IMAGE_FLIP_X | P4_IMAGE_FLIP_Y | P4_IMAGE_PREMULTIPLIED)) &&
        info->swizzle[0] == 0 && info->swizzle[1] == 1 && info->swizzle[2] == 2 && info->swizzle[3] == 3) return 0;
    if (!(info->flags & (P4_IMAGE_FLIP_X | P4_IMAGE_FLIP_Y))) {
        uint32_t row;
        for (row = 0; row < height; ++row) {
            uint8_t *pixel = pixels + (size_t)row * stride;
            uint8_t *end = pixel + width * 4;
            while (pixel != end) {
                transformed(pixel, pixel, info);
                pixel += 4;
            }
        }
        return 0;
    }
    count = (size_t)width * height;
    for (index = 0; index < count; ++index) {
        size_t x = index % width, y = index / width;
        size_t other_x = info->flags & P4_IMAGE_FLIP_X ? width - 1 - x : x;
        size_t other_y = info->flags & P4_IMAGE_FLIP_Y ? height - 1 - y : y;
        size_t other = other_y * width + other_x;
        uint8_t a[4], b[4], *left, *right;
        if (index > other) continue;
        left = pixels + y * stride + x * 4;
        right = pixels + other_y * stride + other_x * 4;
        transformed(a, left, info);
        if (index != other) transformed(b, right, info);
        memcpy(right, a, 4);
        if (index != other) memcpy(left, b, 4);
    }
    return 0;
}
