/* Whole-table SKILL.TBL projections into port skill-ID order. */
#ifndef P4PC_GOLDEN_SKILL_CONVERT_H
#define P4PC_GOLDEN_SKILL_CONVERT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Output contains two-byte element rows indexed by port skill ID.
 * Input and output must not overlap. */
int p4_golden_skill_elements(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                             uint8_t *out, size_t capacity, size_t *written, char *reason, size_t reason_capacity);
/* Output contains 40-byte active metadata rows indexed by port skill ID;
 * unused IDs are zero. Input and output must not overlap. */
int p4_golden_skill_metadata(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                             uint8_t *out, size_t capacity, size_t *written, char *reason, size_t reason_capacity);
#ifdef __cplusplus
}
#endif
#endif
