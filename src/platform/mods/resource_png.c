#include "resource_png.h"
#include "p4content_image.h"
#define LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_NO_COMPILE_ALLOCATORS
#include "../../../third_party/lodepng/lodepng.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The decoder is invoked only in game-thread early preparation. Its shared
 * limit includes compressed inputs, codec workspace and retained outputs. */
#ifdef __ANDROID__
#define RESOURCE_CPU_LIMIT (32u * 1024u * 1024u)
#else
#define RESOURCE_CPU_LIMIT (128u * 1024u * 1024u)
#endif
typedef union PngAllocation {
    max_align_t alignment;
    size_t size;
} PngAllocation;
static size_t allocated;

void *lodepng_malloc(size_t size)
{
    if (size > SIZE_MAX - sizeof(PngAllocation)) return NULL;
    size_t bytes = sizeof(PngAllocation) + size;
    if (bytes > RESOURCE_CPU_LIMIT - allocated) return NULL;
    PngAllocation *block = malloc(bytes);
    if (!block) return NULL;
    block->size = bytes;
    allocated += bytes;
    return block + 1;
}
void lodepng_free(void *pointer)
{
    if (!pointer) return;
    PngAllocation *block = (PngAllocation *)pointer - 1;
    allocated -= block->size;
    free(block);
}
void *lodepng_realloc(void *pointer, size_t size)
{
    if (!pointer) return lodepng_malloc(size);
    if (!size) { lodepng_free(pointer); return NULL; }
    if (size > SIZE_MAX - sizeof(PngAllocation)) return NULL;
    size_t bytes = sizeof(PngAllocation) + size;
    PngAllocation *old = (PngAllocation *)pointer - 1;
    size_t old_size = old->size;
    /* Account for both allocations during a potentially moving realloc. */
    if (bytes > RESOURCE_CPU_LIMIT - allocated) return NULL;
    allocated += bytes;
    PngAllocation *replacement = realloc(old, bytes);
    if (!replacement) { allocated -= bytes; return NULL; }
    allocated -= old_size;
    replacement->size = bytes;
    return replacement + 1;
}
static int read_at(void *user, uint64_t offset, void *out, size_t size)
{
    return p4_package_file_read_at(user, offset, out, size);
}
static int fail(char *error, size_t capacity, const char *message)
{
    if (error && capacity) snprintf(error, capacity, "%s", message);
    return -1;
}
static uint32_t be32(const uint8_t *p)
{
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}
static int all_chunk_crcs(const uint8_t *bytes, size_t size)
{
    size_t offset = 8;
    while (offset < size) {
        if (size - offset < 12) return 0;
        uint32_t length = be32(bytes + offset);
        if ((size_t)length > size - offset - 12) return 0;
        if (lodepng_chunk_check_crc(bytes + offset)) return 0;
        offset += (size_t)length + 12;
    }
    return offset == size;
}
int p4_resource_png_decode(P4PackageFile *file, P4ResourcePng *out,
                           char *error, size_t error_size)
{
    if (error && error_size) *error = 0;
    if (!file || !out) return fail(error, error_size, "invalid image input");
    uint64_t length = p4_package_file_size(file);
    if (!length || length > SIZE_MAX || length > RESOURCE_CPU_LIMIT)
        return fail(error, error_size, "encoded image exceeds the resource CPU budget");
    P4ContentImageInput input = {file, length, read_at};
    P4ContentImageInfo info;
    if (p4_content_image_probe(&input, &info, error, error_size)) return -1;
    if (info.container != P4_IMAGE_PNG)
        return fail(error, error_size, "resource image requires PNG format");
    uint64_t decoded_size = (uint64_t)info.width * info.height * 4u;
    if (!decoded_size || decoded_size > SIZE_MAX || decoded_size > RESOURCE_CPU_LIMIT)
        return fail(error, error_size, "decoded image exceeds the resource CPU budget");
    uint8_t *encoded = lodepng_malloc((size_t)length);
    if (!encoded) return fail(error, error_size, "resource CPU budget exhausted reading PNG");
    if (p4_package_file_read_at(file, 0, encoded, (size_t)length)) {
        lodepng_free(encoded);
        return fail(error, error_size, "cannot read the selected PNG source");
    }
    if (!all_chunk_crcs(encoded, (size_t)length)) {
        lodepng_free(encoded);
        return fail(error, error_size, "PNG chunk CRC mismatch");
    }
    LodePNGState state;
    lodepng_state_init(&state);
    state.decoder.ignore_crc = 1; /* all chunks were checked above, including ignored ones */
    unsigned width = 0, height = 0;
    uint8_t *rgba = NULL;
    unsigned code = lodepng_decode(&rgba, &width, &height, &state, encoded, (size_t)length);
    lodepng_state_cleanup(&state);
    lodepng_free(encoded);
    if (code || width != info.width || height != info.height || !rgba) {
        lodepng_free(rgba);
        return fail(error, error_size, "invalid, corrupt or over-budget PNG payload");
    }
    P4ResourcePng result = {rgba, width, height, (size_t)decoded_size};
    *out = result;
    return 0;
}
void p4_resource_png_free(P4ResourcePng *image)
{
    if (!image) return;
    lodepng_free(image->rgba);
    memset(image, 0, sizeof *image);
}
