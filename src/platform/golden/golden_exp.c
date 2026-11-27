/* Plaintext EXP and proven image data from the pinned Steam executable. No
 * executable code is interpreted or executed. Only PE headers and file-backed
 * .sdata/.xpdata are parsed; the complete file is streamed only for identity. */
#include "p4pc/golden_exp.h"
#include "p4pc/content_digest.h"
#include "golden_pe_data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/hostpath.h"

#define EXP_BYTES (99u * 4u)
#define SDATA_LIMIT (16u * 1024u * 1024u)

static uint16_t u16(const unsigned char *p)
{
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}

static uint32_t u32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}

/* A candidate must be the unique bounded increasing run starting at zero. */
int p4_golden_exp_section(const uint8_t *section, size_t size, const char *expected,
                          uint8_t out[EXP_BYTES], char *reason, size_t capacity)
{
    size_t found = 0, offset = 0, words = size / 4;
    char digest[65];
    if (!section || !expected || !out) return fail(reason, capacity, "missing EXP section or output");
    for (size_t i = 0; i + 99 <= words; ++i) {
        size_t j;
        if (u32(section + i * 4) != 0 || (i && u32(section + (i - 1) * 4) < u32(section + i * 4))) continue;
        for (j = 1; j < 99; ++j)
            if (u32(section + (i + j - 1) * 4) >= u32(section + (i + j) * 4)) break;
        if (j != 99 || (i + 99 < words && u32(section + (i + 98) * 4) < u32(section + (i + 99) * 4))) continue;
        ++found;
        offset = i * 4;
    }
    if (found != 1) return fail(reason, capacity, "EXP section has no unique bounded run");
    p4_content_digest(section + offset, EXP_BYTES, digest);
    if (strcmp(digest, expected)) return fail(reason, capacity, "unsupported EXP table digest");
    memcpy(out, section + offset, EXP_BYTES);
    if (reason && capacity) reason[0] = 0;
    return 0;
}

typedef struct DataSection {
    uint32_t offset, size, virtual_size, flags;
    unsigned matches;
} DataSection;
static int read_expected(const char *path, uint8_t out[EXP_BYTES], char exe_digest[65],
    const char *allowed_exe, const char *allowed_exp,
    const P4GoldenPEDataSpan *spans, size_t count, uint8_t *output, size_t output_size,
    char *reason, size_t capacity)
{
    unsigned char header[64], pe[24], section_header[40], block[65536];
    static const char data_names[2][8] = {".sdata", ".xpdata"};
    DataSection sections[2] = {{0}};
    uint32_t pe_offset;
    unsigned section_count;
    FILE *f;
    P4ContentDigest hash;
    char digest[65];
    uint8_t *data = NULL;
    int result = -1;
    size_t n;
    if (!path || !out || !exe_digest || (count && (!spans || !output)))
        return fail(reason, capacity, "missing executable path or output");
    for (size_t i = 0; i < count; ++i) {
        const P4GoldenPEDataSpan *s = spans + i;
        if (!s->section || (strcmp(s->section, ".sdata") && strcmp(s->section, ".xpdata")) ||
            !s->sha256 || strlen(s->sha256) != 64 || !s->size ||
            s->output_offset > output_size || s->size > output_size - s->output_offset)
            return fail(reason, capacity, "invalid plaintext image span");
        for (size_t j = 0; j < i; ++j)
            if ((uint64_t)s->output_offset < (uint64_t)spans[j].output_offset + spans[j].size &&
                (uint64_t)spans[j].output_offset < (uint64_t)s->output_offset + s->size)
                return fail(reason, capacity, "overlapping plaintext image outputs");
    }
    f = p4_host_fopen(path, "rb");
    if (!f) return fail(reason, capacity, "cannot open Steam P4G.exe");
    p4_content_digest_begin(&hash);
    while ((n = fread(block, 1, sizeof block, f)) != 0) p4_content_digest_update(&hash, block, n);
    if (ferror(f)) { fail(reason, capacity, "cannot hash Steam P4G.exe"); goto done; }
    p4_content_digest_end(&hash, digest);
    if (strcmp(digest, allowed_exe)) { fail(reason, capacity, "unsupported Steam P4G.exe digest"); goto done; }
    rewind(f);
    if (fread(header, 1, sizeof header, f) != sizeof header || header[0] != 'M' || header[1] != 'Z') {
        fail(reason, capacity, "invalid PE DOS header"); goto done;
    }
    pe_offset = u32(header + 60);
    if (pe_offset > 0x100000 || fseek(f, (long)pe_offset, SEEK_SET) || fread(pe, 1, sizeof pe, f) != sizeof pe ||
        memcmp(pe, "PE\0\0", 4) || u16(pe + 4) != 0x8664) {
        fail(reason, capacity, "invalid Steam PE header"); goto done;
    }
    section_count = u16(pe + 6);
    if (!section_count || section_count > 96 || fseek(f, (long)u16(pe + 20), SEEK_CUR)) {
        fail(reason, capacity, "invalid Steam PE section table"); goto done;
    }
    for (unsigned i = 0; i < section_count; ++i) {
        if (fread(section_header, 1, sizeof section_header, f) != sizeof section_header) {
            fail(reason, capacity, "truncated Steam PE section table"); goto done;
        }
        for (unsigned j = 0; j < 2; ++j) {
            if (!memcmp(section_header, data_names[j], 8)) {
                DataSection *s = sections + j;
                ++s->matches;
                s->virtual_size = u32(section_header + 8);
                s->size = u32(section_header + 16);
                s->offset = u32(section_header + 20);
                s->flags = u32(section_header + 36);
            }
        }
    }
    DataSection *sdata = sections;
    if (sdata->matches == 1 && (sdata->flags & 0x20000040u) != 0x40u) {
        fail(reason, capacity, "Steam PE section is not non-executable initialized data"); goto done;
    }
    if (sdata->matches != 1 ||
        sdata->size < EXP_BYTES || sdata->size > SDATA_LIMIT || sdata->offset > 0x7fffffff ||
        fseek(f, (long)sdata->offset, SEEK_SET)) {
        fail(reason, capacity, "invalid Steam PE data section"); goto done;
    }
    data = malloc(sdata->size);
    if (!data || fread(data, 1, sdata->size, f) != sdata->size) {
        fail(reason, capacity, "cannot read Steam PE data section"); goto done;
    }
    if (p4_golden_exp_section(data, sdata->size, allowed_exp, out, reason, capacity)) goto done;
    for (size_t i = 0; i < count; ++i) {
        const P4GoldenPEDataSpan *span = spans + i;
        unsigned index = !strcmp(span->section, ".xpdata");
        const DataSection *s = sections + index;
        if (s->matches != 1 || (s->flags & 0x20000040u) != 0x40u ||
            span->offset > s->size || span->size > s->size - span->offset ||
            span->offset > s->virtual_size || span->size > s->virtual_size - span->offset ||
            (uint64_t)s->offset + span->offset > 0x7fffffff) {
            fail(reason, capacity, "image span is not file-backed initialized data"); goto done;
        }
        uint8_t *destination = output + span->output_offset;
        if (!index) memcpy(destination, data + span->offset, span->size);
        else if (fseek(f, (long)(s->offset + span->offset), SEEK_SET) ||
                 fread(destination, 1, span->size, f) != span->size) {
            fail(reason, capacity, "cannot read plaintext image span"); goto done;
        }
        p4_content_digest(destination, span->size, digest);
        if (strcmp(digest, span->sha256)) {
            fail(reason, capacity, "unsupported plaintext image span digest"); goto done;
        }
    }
    memcpy(exe_digest, allowed_exe, 65);
    result = 0;
done:
    free(data);
    fclose(f);
    return result;
}

int p4_golden_exp_read(const char *path, uint8_t out[EXP_BYTES], char exe_digest[65], char *reason, size_t capacity)
{
    return read_expected(path, out, exe_digest,
        "0f327c7669e6cac72aaf355181b1e51edd12fee4d71709fc1802dbf0da70546e",
        "10074a688452572b931bb748d6912b7e5fb0f71270d6343f4f1084797eaf6fb7",
        NULL, 0, NULL, 0, reason, capacity);
}
int p4_golden_exp_image_read(const char *path, uint8_t out[EXP_BYTES],
    const P4GoldenPEDataSpan *spans, size_t count, uint8_t *data, size_t data_size,
    char exe_digest[65], char *reason, size_t capacity)
{
    return read_expected(path, out, exe_digest,
        "0f327c7669e6cac72aaf355181b1e51edd12fee4d71709fc1802dbf0da70546e",
        "10074a688452572b931bb748d6912b7e5fb0f71270d6343f4f1084797eaf6fb7",
        spans, count, data, data_size, reason, capacity);
}

#ifdef P4_GOLDEN_EXP_TEST
/* Synthetic fixtures only: never compiled into the production reader. */
int p4_golden_exp_test_read(const char *path, uint8_t out[EXP_BYTES], char exe_digest[65],
                            const char *expected_exe, const char *expected_exp, char *reason, size_t capacity)
{
    return read_expected(path, out, exe_digest, expected_exe, expected_exp, NULL, 0, NULL, 0, reason, capacity);
}
int p4_golden_exp_test_image_read(const char *path, uint8_t out[EXP_BYTES], char exe_digest[65],
    const char *expected_exe, const char *expected_exp, const P4GoldenPEDataSpan *spans,
    size_t count, uint8_t *data, size_t data_size, char *reason, size_t capacity)
{
    return read_expected(path, out, exe_digest, expected_exe, expected_exp,
                         spans, count, data, data_size, reason, capacity);
}
#endif
