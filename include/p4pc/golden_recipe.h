/* Golden special-fusion recipe projection. */
#ifndef P4PC_GOLDEN_RECIPE_H
#define P4PC_GOLDEN_RECIPE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Convert one specified 44-byte Golden recipe into a 32-byte PS2 recipe.
 * material_count is determined by the source member's name. Returns 0 on
 * success, 1 when the valid recipe requires more than eight materials, or
 * -1 on malformed input. The output is untouched on refusal. */
int p4_golden_recipe_convert(const P4GoldenDomainMap *map, const uint8_t *row, size_t size,
                             unsigned material_count, uint8_t out[32], char *reason, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
