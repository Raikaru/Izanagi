/* p4.forge: the weapon fusion transaction (docs/modding/lua.md "Weapon fusion
 * transaction"). commit validates and queues; the frame safe point asks the
 * weapon_fusion authorizer, then forges or refuses; result takes the outcome.
 * vessel (forge.vessel@1) reads how many copies commit would accept as a vessel. */
#include "native_bindings.h"
#include "p4pc/mod_events.h"
#include "p4pc/mod_forge.h"
#include <stdio.h>

static int integer_field(lua_State *L, const char *name, int64_t low, int64_t high)
{
    lua_getfield(L, 1, name);
    int64_t value = p4_lua_integer(L, -1, low, high);
    lua_pop(L, 1);
    return (int)value;
}

static int forge_commit(lua_State *L)
{
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.forge.commit is refused in synchronous UI/action callbacks and save snapshots");
    P4LuaMod *mod = p4_luau_user(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    P4ModForge forge = {0};
    forge.character = (uint8_t)integer_field(L, "character", 1, 8);
    forge.persona = (uint16_t)integer_field(L, "persona", 1, 0xffff);
    forge.vessel = (uint16_t)integer_field(L, "vessel", 1, 0x7ffd);
    forge.result = (uint16_t)integer_field(L, "item", 1, 0x7ffd);
    forge.fee = (uint32_t)integer_field(L, "fee", 0, 9999999);
    lua_getfield(L, 1, "signature");
    forge.signature = lua_toboolean(L, -1) ? 1 : 0;
    lua_pop(L, 1);
    char reason[192];
    if (p4_mod_forge_queue(mod->owner, &forge, reason, sizeof reason)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    /* A queued forge will change the player's stock and inventory. */
    p4_lua_semantic_use(mod);
    lua_pushboolean(L, 1);
    return 1;
}

static int forge_result(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    P4ModForge forge;
    char reason[192];
    int outcome = p4_mod_forge_take_result(mod->owner, &forge, reason, sizeof reason);
    if (outcome == P4_MOD_FORGE_NONE) {
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 8);
    lua_pushstring(L, outcome == P4_MOD_FORGE_FORGED ? "forged" : "refused"); lua_setfield(L, -2, "outcome");
    if (outcome == P4_MOD_FORGE_REFUSED) { lua_pushstring(L, reason); lua_setfield(L, -2, "reason"); }
    lua_pushinteger(L, forge.character); lua_setfield(L, -2, "character");
    lua_pushinteger(L, forge.persona); lua_setfield(L, -2, "persona");
    lua_pushinteger(L, forge.vessel); lua_setfield(L, -2, "vessel");
    lua_pushinteger(L, forge.result); lua_setfield(L, -2, "item");
    lua_pushnumber(L, forge.fee); lua_setfield(L, -2, "fee");
    lua_pushboolean(L, forge.signature); lua_setfield(L, -2, "signature");
    lua_setreadonly(L, -1, 1);
    return 1;
}

static int forge_pending(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    lua_pushboolean(L, p4_mod_forge_pending(mod->owner));
    return 1;
}

/* p4.forge.vessel(character, item) -> the unequipped count of a weapon the
 * character may equip, or 0. IDs only, in commit's own ranges. */
static int forge_vessel(lua_State *L)
{
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.forge.vessel is refused in synchronous UI/action callbacks and save snapshots");
    const int character = (int)p4_lua_integer(L, 1, 1, 8);
    const uint32_t item = (uint32_t)p4_lua_integer(L, 2, 1, 0x7ffd);
    lua_pushinteger(L, (int)p4_mod_forge_vessel_count(character, item));
    return 1;
}

void p4_lua_forge_close(P4LuaMod *mod)
{
    p4_mod_forge_clear_owner(mod->owner);
}

void p4_lua_forge_initialize(lua_State *L)
{
    p4_mod_events_set_forge(p4_mod_forge_apply_pending);
    static const luaL_Reg methods[] = {{"commit", forge_commit}, {"result", forge_result},
        {"pending", forge_pending}, {"vessel", forge_vessel}, {NULL, NULL}};
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "forge");
}
