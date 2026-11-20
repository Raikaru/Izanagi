/* Golden Social Link rank-up skill conversion. */
#ifndef P4PC_GOLDEN_CMM_H
#define P4PC_GOLDEN_CMM_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Map both skill IDs in each entry of a rank-up DAT. Output is its 480-byte
 * payload in port ID order, with no DAT header or padding. */
int p4_golden_cmm_rankup_convert(const P4GoldenDomainMap *map, const uint8_t *dat, size_t size,
                                  uint8_t out[480], char *reason, size_t capacity);
/* Read and validate the measured English archive member before conversion. */
int p4_golden_cmm_rankup_load(const char *directory, const P4GoldenDomainMap *map,
                               uint8_t out[480], char *reason, size_t capacity);
/* The digest accepted by load; suitable for provenance only after load succeeds. */
const char *p4_golden_cmm_rankup_source_sha256(void);
#ifdef __cplusplus
}
#endif
#endif
