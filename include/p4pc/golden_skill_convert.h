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
/* Portable declaration transport from already mapped SKILL0 rows. FF is
 * signed NONE (FFFFFFFF), independently of its inheritance bucket 18.
 * Unknown discriminator bytes fail before either output is modified. */
int p4_golden_skill_semantics(const uint8_t *elements, size_t rows, uint8_t *element_ids,
    size_t element_capacity, uint8_t *groups, size_t group_capacity);
/* Output contains the PS2-shaped 40-byte native portion in port-ID order;
 * unused IDs are zero. The caller must retain the entire input as the
 * Golden-only SKILL1 sidecar: high flags and byte7 are not native fields. */
int p4_golden_skill_metadata(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                             uint8_t *out, size_t capacity, size_t *written, char *reason, size_t reason_capacity);
#ifdef __cplusplus
}
#endif
#endif
