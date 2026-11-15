/* Golden inheritance rates in port skill-ID order. */
#ifndef P4PC_GOLDEN_INHERITANCE_H
#define P4PC_GOLDEN_INHERITANCE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Convert an FTD of one-byte rates. Every Golden skill maps to a distinct
 * port slot; untouched slots are zero. Input and output must not overlap.
 * The caller supplies at least capacity bytes and receives the number of
 * occupied port slots including gaps. */
int p4_golden_inheritance_convert(const P4GoldenDomainMap *map, const uint8_t *ftd, size_t size,
                                  uint8_t *out, size_t capacity, size_t *written,
                                  char *reason, size_t reason_capacity);
/* Read the pinned English archive member and convert it; does not write files. */
int p4_golden_inheritance_load(const char *directory, const P4GoldenDomainMap *map,
                              uint8_t *out, size_t capacity, size_t *written,
                              char *reason, size_t reason_capacity);
/* The digest accepted by load; suitable for provenance only after load succeeds. */
const char *p4_golden_inheritance_source_sha256(void);
#ifdef __cplusplus
}
#endif
#endif
