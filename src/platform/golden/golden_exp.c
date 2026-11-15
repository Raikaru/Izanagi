/* Plaintext EXP data from the pinned Steam executable. No executable code is
 * interpreted or executed. Only the PE headers and file-backed .sdata are
 * parsed; the complete file is streamed solely to establish its identity. */
#include "p4pc/golden_exp.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXP_BYTES (99u * 4u)
#define SDATA_LIMIT (16u * 1024u * 1024u)

static uint16_t u16(const unsigned char *p) { return (uint16_t)(p[0] | (unsigned)p[1] << 8); }
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

static int read_expected(const char *path, uint8_t out[EXP_BYTES], char exe_digest[65],
                         const char *allowed_exe, const char *allowed_exp, char *reason, size_t capacity)
{
    unsigned char header[64], pe[24], section_header[40], block[65536];
    uint32_t pe_offset, section_offset = 0, section_size = 0;
    unsigned sections, matches = 0;
    FILE *f;
    P4ContentDigest hash;
    char digest[65];
    uint8_t *data = NULL;
    int result = -1;
    size_t n;
    if (!path || !out || !exe_digest) return fail(reason, capacity, "missing executable path or output");
    f = fopen(path, "rb");
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
    sections = u16(pe + 6);
    if (!sections || sections > 96 || fseek(f, (long)u16(pe + 20), SEEK_CUR)) {
        fail(reason, capacity, "invalid Steam PE section table"); goto done;
    }
    for (unsigned i = 0; i < sections; ++i) {
        if (fread(section_header, 1, sizeof section_header, f) != sizeof section_header) {
            fail(reason, capacity, "truncated Steam PE section table"); goto done;
        }
        if (!memcmp(section_header, ".sdata\0\0", 8)) {
            ++matches;
            if ((u32(section_header + 36) & 0x20000040u) != 0x40u) {
                fail(reason, capacity, "Steam PE section is not non-executable initialized data"); goto done;
            }
            section_size = u32(section_header + 16);
            section_offset = u32(section_header + 20);
        }
    }
    if (matches != 1 || section_size < EXP_BYTES || section_size > SDATA_LIMIT || section_offset > 0x7fffffff ||
        fseek(f, (long)section_offset, SEEK_SET)) {
        fail(reason, capacity, "invalid Steam PE data section"); goto done;
    }
    data = malloc(section_size);
    if (!data || fread(data, 1, section_size, f) != section_size) {
        fail(reason, capacity, "cannot read Steam PE data section"); goto done;
    }
    if (p4_golden_exp_section(data, section_size, allowed_exp, out, reason, capacity)) goto done;
    memcpy(exe_digest, digest, sizeof digest);
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
        "10074a688452572b931bb748d6912b7e5fb0f71270d6343f4f1084797eaf6fb7", reason, capacity);
}

#ifdef P4_GOLDEN_EXP_TEST
/* Synthetic fixtures only: never compiled into the production reader. */
int p4_golden_exp_test_read(const char *path, uint8_t out[EXP_BYTES], char exe_digest[65],
                            const char *expected_exe, const char *expected_exp, char *reason, size_t capacity)
{
    return read_expected(path, out, exe_digest, expected_exe, expected_exp, reason, capacity);
}
#endif
