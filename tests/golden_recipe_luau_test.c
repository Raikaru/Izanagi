/* Real Luau execution over synthetic recipes; no game data. */
#include "p4pc/luau_c.h"
#include "p4pc/golden_recipe.h"
#include "p4pc/game_mode.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static P4GameMode mode = P4_GAME_MODE_GOLDEN;
P4GameMode p4_game_mode(void) { return mode; }
void p4_lua_data_fusion_register(lua_State *L);
static void initialize(lua_State *L, void *user)
{
    (void)user;
    lua_newtable(L);
    p4_lua_data_fusion_register(L);
    lua_setglobal(L, "data");
}
static int run(p4_luau *vm, const char *source)
{
    int rc = p4_luau_load_source(vm, "@golden-recipe-test", source, strlen(source));
    if (rc == LUA_OK) rc = p4_luau_run(vm, 0, 0);
    if (rc != LUA_OK) fprintf(stderr, "%s\n", p4_luau_error(vm));
    return rc;
}
int main(void)
{
    P4GoldenRecipe recipes[3] = {{0}};
    recipes[0].result = 100; recipes[0].material_count = 12;
    for (int i = 0; i < 12; ++i) recipes[0].materials[i] = (uint16_t)(i + 1);
    recipes[0].materials[11] = 1;
    recipes[1].result = 101; recipes[1].material_count = 4;
    for (int i = 0; i < 4; ++i) recipes[1].materials[i] = (uint16_t)(20 + i);
    recipes[2] = recipes[0]; recipes[2].result = 102;
    recipes[2].materials[0] = 60;
    CHECK(!p4_golden_recipes_install(recipes, 3));
    p4_luau_config config = {1024 * 1024, NULL, NULL, initialize, 65536};
    p4_luau *vm = p4_luau_create(&config);
    CHECK(vm != NULL);
    CHECK(run(vm, "local a=data.fusion_recipes(); assert(#a==3 and a[1].result==100 and #a[1].materials==12); "
                  "assert(a[1].materials[1]==1 and a[1].materials[12]==1 and a[2].result==101 and a[3].materials[1]==60); "
                  "a[1].materials[1]=99; assert(data.fusion_recipe(1).materials[1]==1); "
                  "assert(data.fusion_recipe(2).materials[4]==23); "
                  "assert(not pcall(data.fusion_recipe,0)); assert(not pcall(data.fusion_recipe,1.5)); "
                  "assert(not pcall(data.fusion_recipe,4))") == LUA_OK);
    p4_golden_recipes_clear();
    CHECK(run(vm, "assert(not pcall(data.fusion_recipes)); assert(not pcall(data.fusion_recipe,1))") == LUA_OK);
    p4_luau_destroy(vm);
    mode = P4_GAME_MODE_PS2;
    CHECK(p4_golden_recipes_install(recipes, 3) == -1);
    vm = p4_luau_create(&config);
    CHECK(vm != NULL);
    CHECK(run(vm, "assert(data.fusion_recipe==nil and data.fusion_recipes==nil)") == LUA_OK);
    p4_luau_destroy(vm);
    puts("golden_recipe_luau: ok");
    return 0;
}
