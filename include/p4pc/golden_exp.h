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
#ifdef __cplusplus
}
#endif
#endif
