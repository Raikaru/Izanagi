/* Runtime-only Golden recipe bank; source ownership remains with the caller. */
#include "p4pc/golden_recipe.h"
#include "p4pc/game_mode.h"
#include "p4pc/game_personas.h"
#include <string.h>

static P4GoldenRecipe bank[P4_GOLDEN_RECIPE_MAX];
static size_t bank_count;
static int installed;
static unsigned twelfth_row;
int p4_golden_recipes_active(void) { return p4_game_mode() == P4_GAME_MODE_GOLDEN; }

int p4_golden_recipes_install(const P4GoldenRecipe *recipes, size_t count)
{
    unsigned group_counts[13] = {0};
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN || !recipes || !count || count > P4_GOLDEN_RECIPE_MAX) return -1;
    for (size_t i = 0; i < count; ++i) {
        const P4GoldenRecipe *r = &recipes[i];
        if (!r->result || r->result > P4_PERSONA_MAX_ID || r->material_count < 2 ||
            r->material_count > P4_GOLDEN_RECIPE_MATERIAL_MAX) return -1;
        if (++group_counts[r->material_count] > 12u && r->material_count != 12) return -1;
        for (unsigned j = 0; j < P4_GOLDEN_RECIPE_MATERIAL_MAX; ++j) {
            if (j < r->material_count) {
                if (!r->materials[j] || r->materials[j] > P4_PERSONA_MAX_ID) return -1;
            } else if (r->materials[j]) return -1;
        }
    }
    memcpy(bank, recipes, count * sizeof *recipes);
    bank_count = count;
    twelfth_row = 0;
    installed = 1;
    return 0;
}
void p4_golden_recipes_clear(void) { installed = 0; bank_count = 0; twelfth_row = 0; }
int p4_golden_recipes_count(void)
{
    return p4_game_mode() == P4_GAME_MODE_GOLDEN && installed ? (int)bank_count : -1;
}
int p4_golden_recipe_result(unsigned index)
{
    return p4_golden_recipes_count() >= 0 && index < bank_count ? bank[index].result : -1;
}
int p4_golden_recipe_material_count(unsigned index)
{
    return p4_golden_recipes_count() >= 0 && index < bank_count ? bank[index].material_count : -1;
}
int p4_golden_recipe_material(unsigned index, unsigned material)
{
    return p4_golden_recipes_count() >= 0 && index < bank_count && material < bank[index].material_count
        ? bank[index].materials[material] : -1;
}
int p4_golden_recipe_twelfth(void)
{
    return p4_golden_recipe_group_index(12, twelfth_row);
}
int p4_golden_recipe_cycle_twelfth(int direction)
{
    unsigned count = 0;
    if (p4_golden_recipes_count() < 0) return -1;
    for (size_t i = 0; i < bank_count; ++i) if (bank[i].material_count == 12) ++count;
    if (!count) return -1;
    if (direction > 0) twelfth_row = (twelfth_row + 1) % count;
    else if (direction < 0) twelfth_row = twelfth_row ? twelfth_row - 1 : count - 1;
    return p4_golden_recipe_twelfth();
}
int p4_golden_recipe_group_index(unsigned material_count, unsigned row)
{
    int count = p4_golden_recipes_count();
    for (int i = 0; i < count; ++i) {
        if (bank[i].material_count == material_count && row-- == 0) return i;
    }
    return -1;
}
