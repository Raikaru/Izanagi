/* Golden special-fusion recipe projection. */
#ifndef P4PC_GOLDEN_RECIPE_H
#define P4PC_GOLDEN_RECIPE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/golden_domains.h"
#ifdef __cplusplus
extern "C" {
#endif
#define P4_GOLDEN_RECIPE_MATERIAL_MAX 12u
typedef struct P4GoldenRecipe {
    uint16_t result;
    uint16_t materials[P4_GOLDEN_RECIPE_MATERIAL_MAX];
    uint8_t material_count;
} P4GoldenRecipe;
/* Convert one 44-byte Golden row. The count comes from the source member.
 * Returns -1 for malformed input, leaving the output untouched. */
int p4_golden_recipe_convert(const P4GoldenDomainMap *map, const uint8_t *row, size_t size,
                             unsigned material_count, P4GoldenRecipe *out, char *reason, size_t capacity);
/* Runtime tables are installed only in Golden mode. Installation validates and
 * copies the entire set atomically; the caller retains ownership of its input.
 * Getters return -1 when unavailable or out of range. */
#define P4_GOLDEN_RECIPE_MAX 17u
int p4_golden_recipes_install(const P4GoldenRecipe *recipes, size_t count);
void p4_golden_recipes_clear(void);
int p4_golden_recipes_active(void);
int p4_golden_recipes_count(void);
int p4_golden_recipe_result(unsigned index);
int p4_golden_recipe_material_count(unsigned index);
int p4_golden_recipe_material(unsigned index, unsigned material);
/* Current twelve-material group index, and cycle in source order with wrap. */
int p4_golden_recipe_twelfth(void);
int p4_golden_recipe_cycle_twelfth(int direction);
/* Index of the zero-based row in a material-count group, or -1. */
int p4_golden_recipe_group_index(unsigned material_count, unsigned row);
#ifdef __cplusplus
}
#endif
#endif
