/* Plaintext data-section reader for the owner's Steam P4G.exe. */
#ifndef P4PC_GOLDEN_EXP_H
#define P4PC_GOLDEN_EXP_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Locate the unique 99-u32 increasing, bounded run starting at zero and
 * require the supplied lowercase SHA-256 before copying the 396 bytes. */
int p4_golden_exp_section(const uint8_t *section, size_t size, const char *expected,
                          uint8_t out[396], char *reason, size_t capacity);
/* Require the supported Steam executable and EXP hashes; parse PE headers and
 * read its file-backed .sdata only. Outputs are valid only on success. */
int p4_golden_exp_read(const char *path, uint8_t out[396], char exe_digest[65], char *reason, size_t capacity);
/* Compiled source specifications only. Offsets are section-relative; only
 * file-backed .sdata/.xpdata spans are accepted, each with its own digest. */
typedef struct P4GoldenPEDataSpan {
    const char *section, *sha256;
    uint32_t offset, size, output_offset;
} P4GoldenPEDataSpan;
/* Read EXP and additional proven image spans under one executable hash pass.
 * Outputs are valid only on success. No executable code section is read. */
int p4_golden_exp_image_read(const char *path, uint8_t experience[396],
    const P4GoldenPEDataSpan *spans, size_t count, uint8_t *data, size_t data_size,
    char exe_digest[65], char *reason, size_t capacity);
#ifdef P4_GOLDEN_EXP_TEST
/* Synthetic file identities are accepted only in the isolated reader fixture. */
int p4_golden_exp_test_image_read(const char *path, uint8_t experience[396], char exe_digest[65],
    const char *expected_exe, const char *expected_exp, const P4GoldenPEDataSpan *spans,
    size_t count, uint8_t *data, size_t data_size, char *reason, size_t capacity);
#endif
#ifdef __cplusplus
}
#endif
#endif
