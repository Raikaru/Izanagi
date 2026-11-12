/* CodeBreaker framing follows the public-domain mymc ps2save.py by Ross Ridge:
 * https://github.com/ps2dev/mymc/blob/master/ps2save.py
 * The published 256-byte table below is a file-format constant: the initial
 * RC4 S permutation, NOT a password to feed through RC4 key scheduling. */
#include "import_cbs.h"
#include "../../../third_party/miniz/miniz_tinfl.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CBS_LIMIT (8u * 1024u * 1024u)
#define CBS_RECORD_LIMIT 64u
#define CBS_BODY_LIMIT (CBS_LIMIT + CBS_RECORD_LIMIT * 64u)
/* A valid zlib encoding may be slightly larger than its uncompressed input.
 * Bound the encrypted allocation too, independently of untrusted sizes. */
#define CBS_COMPRESSED_LIMIT (CBS_BODY_LIMIT + 65536u)

static const unsigned char cbs_rc4_state[256] = {
0x5f, 0x1f, 0x85, 0x6f, 0x31, 0xaa, 0x3b, 0x18,
		    0x21, 0xb9, 0xce, 0x1c, 0x07, 0x4c, 0x9c, 0xb4,
		    0x81, 0xb8, 0xef, 0x98, 0x59, 0xae, 0xf9, 0x26,
		    0xe3, 0x80, 0xa3, 0x29, 0x2d, 0x73, 0x51, 0x62,
		    0x7c, 0x64, 0x46, 0xf4, 0x34, 0x1a, 0xf6, 0xe1,
		    0xba, 0x3a, 0x0d, 0x82, 0x79, 0x0a, 0x5c, 0x16,
		    0x71, 0x49, 0x8e, 0xac, 0x8c, 0x9f, 0x35, 0x19,
		    0x45, 0x94, 0x3f, 0x56, 0x0c, 0x91, 0x00, 0x0b,
		    0xd7, 0xb0, 0xdd, 0x39, 0x66, 0xa1, 0x76, 0x52,
		    0x13, 0x57, 0xf3, 0xbb, 0x4e, 0xe5, 0xdc, 0xf0,
		    0x65, 0x84, 0xb2, 0xd6, 0xdf, 0x15, 0x3c, 0x63,
		    0x1d, 0x89, 0x14, 0xbd, 0xd2, 0x36, 0xfe, 0xb1,
		    0xca, 0x8b, 0xa4, 0xc6, 0x9e, 0x67, 0x47, 0x37,
		    0x42, 0x6d, 0x6a, 0x03, 0x92, 0x70, 0x05, 0x7d,
		    0x96, 0x2f, 0x40, 0x90, 0xc4, 0xf1, 0x3e, 0x3d,
		    0x01, 0xf7, 0x68, 0x1e, 0xc3, 0xfc, 0x72, 0xb5,
		    0x54, 0xcf, 0xe7, 0x41, 0xe4, 0x4d, 0x83, 0x55,
		    0x12, 0x22, 0x09, 0x78, 0xfa, 0xde, 0xa7, 0x06,
		    0x08, 0x23, 0xbf, 0x0f, 0xcc, 0xc1, 0x97, 0x61,
		    0xc5, 0x4a, 0xe6, 0xa0, 0x11, 0xc2, 0xea, 0x74,
		    0x02, 0x87, 0xd5, 0xd1, 0x9d, 0xb7, 0x7e, 0x38,
		    0x60, 0x53, 0x95, 0x8d, 0x25, 0x77, 0x10, 0x5e,
		    0x9b, 0x7f, 0xd8, 0x6e, 0xda, 0xa2, 0x2e, 0x20,
		    0x4f, 0xcd, 0x8f, 0xcb, 0xbe, 0x5a, 0xe0, 0xed,
		    0x2c, 0x9a, 0xd4, 0xe2, 0xaf, 0xd0, 0xa9, 0xe8,
		    0xad, 0x7a, 0xbc, 0xa8, 0xf2, 0xee, 0xeb, 0xf5,
		    0xa6, 0x99, 0x28, 0x24, 0x6c, 0x2b, 0x75, 0x5d,
		    0xf8, 0xd3, 0x86, 0x17, 0xfb, 0xc0, 0x7b, 0xb3,
		    0x58, 0xdb, 0xc7, 0x4b, 0xff, 0x04, 0x50, 0xe9,
		    0x88, 0x69, 0xc9, 0x2a, 0xab, 0xfd, 0x5b, 0x1b,
		    0x8a, 0xd9, 0xec, 0x27, 0x44, 0x0e, 0x33, 0xc8,
		    0x6b, 0x93, 0x32, 0x48, 0xb6, 0x30, 0x43, 0xa5
};

static uint32_t cbs_u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static void cbs_crypt(unsigned char *data, size_t size)
{
    unsigned char s[256];
    unsigned i = 0, j = 0;
    size_t n;
    memcpy(s, cbs_rc4_state, sizeof(s));
    for (n = 0; n < size; ++n) {
        unsigned char t;
        i = (i + 1u) & 255u;
        j = (j + s[i]) & 255u;
        t = s[i]; s[i] = s[j]; s[j] = t;
        data[n] ^= s[(s[i] + s[j]) & 255u];
    }
}

static int cbs_name(char dst[33], const unsigned char *src)
{
    size_t n = 0;
    while (n < 32 && src[n]) {
        if (src[n] < 32 || src[n] == 127 || src[n] == '/' ||
            src[n] == '\\' || src[n] == ':')
            return 0;
        dst[n] = (char)src[n];
        ++n;
    }
    dst[n] = 0;
    return n && strcmp(dst, ".") && strcmp(dst, "..");
}

int p4save_decode_cbs(const unsigned char *blob, size_t size,
                      P4SaveArchive *out, char *error, size_t cap)
{
    unsigned char *encrypted = NULL, *body = NULL;
    size_t header, compressed, expanded, in_size, out_size, pos, total = 0;
    size_t i;
    tinfl_decompressor inflater;
    const char *why = "Invalid CBS arguments";
    if (error && cap) error[0] = 0;
    if (!blob || !out) goto fail;
    /* Caller contract: out is zero-initialized and owns no previous data. */
    if (size < 124) { why = "Truncated CBS header"; goto fail; }
    if (memcmp(blob, "CFU\0", 4)) { why = "Bad CBS magic"; goto fail; }
    header = cbs_u32(blob + 8);
    expanded = cbs_u32(blob + 12);
    compressed = cbs_u32(blob + 16);
    if (header < 124 || header > size) {
        why = "Invalid CBS data offset"; goto fail;
    }
    /* Real encoders use either body size or entire archive size at +16. */
    if (compressed != size - header && compressed != size) {
        why = "CBS compressed size mismatch"; goto fail;
    }
    compressed = size - header;
    if (!expanded || expanded > CBS_BODY_LIMIT ||
        !compressed || compressed > CBS_COMPRESSED_LIMIT) {
        why = "CBS size exceeds output/input limit"; goto fail;
    }
    if (!cbs_name(out->name, blob + 20)) {
        why = "Invalid CBS directory name"; goto fail;
    }
    memcpy(out->created, blob + 52, 8);
    memcpy(out->modified, blob + 60, 8);
    /* Directory modes at +76 are unreliable in published encoders (mymc);
     * preserve timestamps but don't reject saves solely for this field.
     * Shared P4SaveArchive intentionally does not expose permission modes. */
    encrypted = (unsigned char *)malloc(compressed);
    body = (unsigned char *)malloc(expanded);
    if (!encrypted || !body) { why = "Out of memory decoding CBS"; goto fail; }
    memcpy(encrypted, blob + header, compressed);
    cbs_crypt(encrypted, compressed);
    tinfl_init(&inflater);
    in_size = compressed;
    out_size = expanded;
    if (tinfl_decompress(&inflater, encrypted, &in_size, body, body, &out_size,
            TINFL_FLAG_PARSE_ZLIB_HEADER |
            TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF) != TINFL_STATUS_DONE ||
        out_size != expanded || in_size != compressed) {
        why = "CBS zlib size, stream or Adler-32 integrity failure"; goto fail;
    }
    free(encrypted);
    encrypted = NULL;
    pos = 0;
    while (pos < expanded) {
        P4SaveFile *file;
        size_t length;
        unsigned mode;
        if (out->count == CBS_RECORD_LIMIT || expanded - pos < 64) {
            why = "CBS file count limit or truncated record"; goto fail;
        }
        length = cbs_u32(body + pos + 16);
        mode = body[pos + 20] | (unsigned)body[pos + 21] << 8;
        if ((mode & 0x30u) != 0x10u) {
            why = "CBS record is not a regular file"; goto fail;
        }
        if (length > expanded - pos - 64 || length > CBS_LIMIT - total) {
            why = "CBS record size exceeds data or output limit"; goto fail;
        }
        file = &out->files[out->count];
        if (!cbs_name(file->name, body + pos + 32)) {
            why = "Invalid CBS file name"; goto fail;
        }
        for (i = 0; i < out->count; ++i) {
            if (!strcmp(out->files[i].name, file->name)) {
                why = "Duplicate CBS file name"; goto fail;
            }
        }
        memcpy(file->created, body + pos, 8);
        memcpy(file->modified, body + pos + 8, 8);
        file->data = (unsigned char *)malloc(length ? length : 1);
        if (!file->data) { why = "Out of memory copying CBS file"; goto fail; }
        file->size = length;
        memcpy(file->data, body + pos + 64, length);
        ++out->count;
        total += length;
        pos += 64 + length;
    }
    free(body);
    if (!out->count) { why = "CBS contains no files"; body = NULL; goto fail; }
    return 1;
fail:
    free(encrypted);
    free(body);
    if (out) {
        for (i = 0; i < out->count; ++i) free(out->files[i].data);
        memset(out, 0, sizeof(*out));
    }
    if (error && cap) snprintf(error, cap, "%s", why);
    return 0;
}
