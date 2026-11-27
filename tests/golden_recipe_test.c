/* Synthetic special-fusion rows only. */
#include "p4pc/golden_recipe.h"
#include "p4pc/golden_fusion_pool.h"
#include "p4pc/game_mode.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)
static P4GameMode mode = P4_GAME_MODE_GOLDEN;
P4GameMode p4_game_mode(void) { return mode; }
int main(void)
{
    const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenDomainMap map;
    uint8_t row[44] = {0};
    P4GoldenRecipe out, unchanged;
    char reason[128];
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    row[0] = 40; row[12] = 10; row[14] = 20;
    CHECK(!p4_golden_recipe_convert(&map, row, sizeof row, 2, &out, reason, sizeof reason));
    CHECK(out.result == 40 && out.material_count == 2 && out.materials[0] == 10 &&
          out.materials[1] == 20 && out.materials[2] == 0);
    for (unsigned i = 2; i < 8; ++i) row[12 + i * 2] = (uint8_t)(i + 1);
    CHECK(!p4_golden_recipe_convert(&map, row, sizeof row, 8, &out, reason, sizeof reason));
    CHECK(out.material_count == 8 && out.materials[7] == 8 && out.materials[8] == 0);
    for (unsigned i = 8; i < 12; ++i) row[12 + i * 2] = (uint8_t)(i + 1);
    row[14] = row[12]; /* duplicates and their order must survive */
    CHECK(!p4_golden_recipe_convert(&map, row, sizeof row, 12, &out, reason, sizeof reason));
    CHECK(out.material_count == 12 && out.materials[0] == 10 && out.materials[1] == 10 &&
          out.materials[11] == 12);
    unchanged = out;
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row, 13, &out, reason, sizeof reason) == -1);
    CHECK(!memcmp(&out, &unchanged, sizeof out));
    row[2] = 1;
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row, 12, &out, reason, sizeof reason) == -1);
    row[2] = 0; row[12] = 0;
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row, 12, &out, reason, sizeof reason) == -1);
    CHECK(!memcmp(&out, &unchanged, sizeof out));
    row[12] = 10; row[13] = 1; /* 0x10a is outside the synthetic persona domain */
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row, 12, &out, reason, sizeof reason) == -1);
    CHECK(!memcmp(&out, &unchanged, sizeof out));
    row[13] = 0;
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row - 1, 12, &out, reason, sizeof reason) == -1);
    CHECK(!memcmp(&out, &unchanged, sizeof out));
    CHECK(p4_golden_recipe_convert(&map, row, sizeof row, 1, &out, reason, sizeof reason) == -1);
    CHECK(!memcmp(&out, &unchanged, sizeof out));
    {
        P4GoldenRecipe recipes[2] = {unchanged, unchanged};
        recipes[1].material_count = 8;
        for (unsigned i = 8; i < 12; ++i) recipes[1].materials[i] = 0;
        CHECK(p4_golden_recipes_count() == -1);
        CHECK(!p4_golden_recipes_install(recipes, 2));
        CHECK(p4_golden_recipes_count() == 2 && p4_golden_recipe_result(0) == 40 &&
              p4_golden_recipe_material_count(0) == 12 && p4_golden_recipe_material(0, 11) == 12 &&
              p4_golden_recipe_material(0, 12) == -1);
        CHECK(p4_golden_recipe_twelfth() == 0 && p4_golden_recipe_group_index(8, 0) == 1 &&
              p4_golden_recipe_group_index(8, 1) == -1);
        recipes[1] = unchanged;
        recipes[1].result = 41;
        CHECK(!p4_golden_recipes_install(recipes, 2));
        CHECK(p4_golden_recipe_twelfth() == 0 && p4_golden_recipe_cycle_twelfth(1) == 1 &&
              p4_golden_recipe_result((unsigned)p4_golden_recipe_twelfth()) == 41 &&
              p4_golden_recipe_cycle_twelfth(1) == 0 && p4_golden_recipe_cycle_twelfth(-1) == 1);
        recipes[0].result = 0;
        CHECK(p4_golden_recipes_install(recipes, 2) == -1 && p4_golden_recipe_result(0) == 40);
        mode = P4_GAME_MODE_PS2;
        CHECK(p4_golden_recipes_count() == -1 && p4_golden_recipe_result(0) == -1 &&
              p4_golden_recipes_install(recipes, 2) == -1);
        mode = P4_GAME_MODE_GOLDEN;
        p4_golden_recipes_clear();
        CHECK(p4_golden_recipes_count() == -1);
    }
    {
        P4GoldenFusionPool pool = {{0}, 0};
        for (unsigned i = 0; i < 96; ++i) CHECK(p4_golden_fusion_pool_add(&pool, (uint16_t)(i + 1)) == 1);
        CHECK(pool.count == 96 && p4_golden_fusion_pool_add(&pool, 50) == 0);
        CHECK(p4_golden_fusion_pool_add(&pool, 97) == -1 && pool.count == 96);
        p4_golden_fusion_pool_remove(&pool, 12);
        CHECK(pool.count == 95 && pool.ids[12] == 14 && pool.ids[94] == 96);
        p4_golden_fusion_pool_remove(&pool, 95);
        CHECK(pool.count == 95 && p4_golden_fusion_pool_add(&pool, 97) == 1);
    }
    puts("golden_recipe: ok");
    return 0;
}
