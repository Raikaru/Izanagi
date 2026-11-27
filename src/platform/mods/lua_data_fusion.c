/* Golden-only special-fusion getters for p4.data. */
#include "p4pc/luau_c.h"
#include "p4pc/golden_recipe.h"

static void push_fusion_recipe(lua_State *L, unsigned index)
{
    int result = p4_golden_recipe_result(index);
    int count = p4_golden_recipe_material_count(index);
    if (result < 0 || count < 0) luaL_error(L, "p4.data.fusion_recipe: recipe unavailable");
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, result);
    lua_setfield(L, -2, "result");
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; ++i) {
        lua_pushnumber(L, p4_golden_recipe_material(index, (unsigned)i));
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "materials");
}
static int data_fusion_recipe(lua_State *L)
{
    double value = luaL_checknumber(L, 1);
    int count = p4_golden_recipes_count();
    if (count < 0) luaL_error(L, "p4.data.fusion_recipe: Golden recipes are unavailable");
    if (!(value >= 1 && value <= count) || value != (double)(unsigned)value)
        luaL_error(L, "p4.data.fusion_recipe: invalid recipe index");
    push_fusion_recipe(L, (unsigned)value - 1);
    return 1;
}
static int data_fusion_recipes(lua_State *L)
{
    int count = p4_golden_recipes_count();
    if (count < 0) luaL_error(L, "p4.data.fusion_recipes: Golden recipes are unavailable");
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; ++i) {
        push_fusion_recipe(L, (unsigned)i);
        lua_rawseti(L, -2, i + 1);
    }
    return 1;
}
void p4_lua_data_fusion_register(lua_State *L)
{
    static const luaL_Reg methods[] = {{"fusion_recipe", data_fusion_recipe},
                                       {"fusion_recipes", data_fusion_recipes}, {NULL, NULL}};
    if (p4_golden_recipes_active()) luaL_register(L, NULL, methods);
}
