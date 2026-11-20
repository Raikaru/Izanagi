/* Golden fixed-width battle names converted without truncation. */
#ifndef P4PC_GOLDEN_NAMES_H
#define P4PC_GOLDEN_NAMES_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Convert an entire fixed-width name bank into port-ID order. Names must be
 * NUL-terminated and zero-padded and fit the destination width including NUL.
 * Input and output must not overlap. */
int p4_golden_names_convert(const P4GoldenDomainMap *map, P4GoldenDomain domain,
                            const uint8_t *input, size_t size, unsigned rows,
                            unsigned source_width, unsigned dest_width,
                            uint8_t *out, size_t capacity, size_t *written,
                            char *reason, size_t reason_capacity);
#ifdef __cplusplus
}
#endif
#endif
