/* p4.persona.stock() and p4.social.rank(arcana): read-only copies of Yu's
 * Persona stock and the Social Link ranks (docs/modding/lua.md "Persona stock
 * and Social Links"). Legal in every callback mode, including synchronous UI
 * callbacks and snapshots: they only copy scalars. */
#include "native_bindings.h"
#include "p4pc/mod_persona.h"

static void push_array(lua_State *L, const void *values, int count, int width)
{
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; ++i) {
        unsigned value = width == 1 ? ((const uint8_t *)values)[i] : ((const uint16_t *)values)[i];
        lua_pushunsigned(L, value);
        lua_rawseti(L, -2, i + 1);
    }
}

static int persona_stock(lua_State *L)
{
    P4ModStockEntry stock[P4_MOD_STOCK_MAX];
    int count = p4_mod_persona_stock(stock, P4_MOD_STOCK_MAX);
    lua_createtable(L, count, 0);
    for (int i = 0; i < count; ++i) {
        const P4ModStockEntry *e = &stock[i];
        lua_createtable(L, 0, 10);
        lua_pushinteger(L, e->slot + 1); lua_setfield(L, -2, "slot");
        lua_pushboolean(L, e->lead); lua_setfield(L, -2, "lead");
        lua_pushinteger(L, e->persona); lua_setfield(L, -2, "persona");
        lua_pushinteger(L, e->arcana); lua_setfield(L, -2, "arcana");
        lua_pushinteger(L, e->level); lua_setfield(L, -2, "level");
        lua_pushnumber(L, e->exp); lua_setfield(L, -2, "exp");
        push_array(L, e->stats, 5, 1); lua_setfield(L, -2, "stats");
        int skills = 0;
        uint16_t learned[8];
        for (int s = 0; s < 8; ++s)
            if (e->skills[s]) learned[skills++] = e->skills[s];
        push_array(L, learned, skills, 2); lua_setfield(L, -2, "skills");
        lua_pushboolean(L, e->registered); lua_setfield(L, -2, "registered");
        lua_pushinteger(L, e->registered_level); lua_setfield(L, -2, "registered_level");
        lua_setreadonly(L, -1, 1);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setreadonly(L, -1, 1);
    return 1;
}

static int social_rank(lua_State *L)
{
    int arcana = (int)p4_lua_integer(L, 1, 0, 0xFF);
    lua_pushinteger(L, p4_mod_social_rank(arcana));
    return 1;
}

void p4_lua_persona_initialize(lua_State *L)
{
    static const luaL_Reg persona[] = {{"stock", persona_stock}, {NULL, NULL}};
    p4_lua_library(L, persona);
    lua_setfield(L, -2, "persona");
    static const luaL_Reg social[] = {{"rank", social_rank}, {NULL, NULL}};
    p4_lua_library(L, social);
    lua_setfield(L, -2, "social");
}
