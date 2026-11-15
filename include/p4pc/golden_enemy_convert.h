/* Golden UNIT.TBL enemy metadata conversion. */
#ifndef P4PC_GOLDEN_ENEMY_CONVERT_H
#define P4PC_GOLDEN_ENEMY_CONVERT_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Maps the skill and item references of all 368 60-byte enemy rows. Input
 * and output must not overlap. All validation precedes output writes. */
int p4_golden_enemy_convert(const P4GoldenDomainMap *map, const uint8_t *input, size_t size,
                            uint8_t *out, size_t capacity, char *reason, size_t reason_capacity);
#ifdef __cplusplus
}
#endif
#endif
