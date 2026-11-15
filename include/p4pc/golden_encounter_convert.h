/* Golden ENCOUNT.TBL projections into PS2-shaped rows. */
#ifndef P4PC_GOLDEN_ENCOUNTER_CONVERT_H
#define P4PC_GOLDEN_ENCOUNTER_CONVERT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Convert block 0 (formations), 1 (selectors), 2 (opaque), 3 (weighted
 * groups), or 4 (treasure records); inputs and outputs must not overlap. */
int p4_golden_encounter_convert(const P4GoldenDomainMap *map, unsigned block,
                                 const uint8_t *input, size_t size, uint8_t *out, size_t capacity,
                                 size_t *written, char *reason, size_t reason_capacity);
#ifdef __cplusplus
}
#endif
#endif
