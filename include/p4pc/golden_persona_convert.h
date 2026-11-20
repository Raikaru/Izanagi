/* Golden PERSONA.TBL skill-bearing blocks. */
#ifndef P4PC_GOLDEN_PERSONA_CONVERT_H
#define P4PC_GOLDEN_PERSONA_CONVERT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Convert block 1 (growth events), 2 (party events), or 4 (replacement
 * pairs); map all active skill references. Input and output must not overlap. */
int p4_golden_persona_convert(const P4GoldenDomainMap *map, unsigned block, const uint8_t *input,
                               size_t size, uint8_t *out, size_t capacity,
                               char *reason, size_t reason_capacity);
#ifdef __cplusplus
}
#endif
#endif
