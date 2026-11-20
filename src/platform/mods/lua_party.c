/* p4.party: playable characters' Persona records (docs/modding/lua.md "Party records"). */
#include "native_bindings.h"
#include "p4pc/mod_party.h"
#include "p4pc/mod_events.h"
#include <stdint.h>
#include <stdio.h>

static int party_record(lua_State *L)
{
    P4ModPartyRecord record;
    int id = (int)p4_lua_integer(L, 1, 1, 10);
    if (p4_mod_party_read(id, &record)) {
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 5);
    lua_pushinteger(L, record.persona); lua_setfield(L, -2, "persona");
    lua_pushinteger(L, record.level); lua_setfield(L, -2, "level");
    lua_pushnumber(L, record.exp); lua_setfield(L, -2, "exp");
    lua_createtable(L, 5, 0);
    for (int i = 0; i < 5; ++i) {
        lua_pushinteger(L, record.stats[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setfield(L, -2, "stats");
    lua_createtable(L, 8, 0);
    int slot = 0;
    for (int i = 0; i < 8; ++i)
        if (record.skills[i]) {
            lua_pushinteger(L, record.skills[i]);
            lua_rawseti(L, -2, ++slot);
        }
    lua_setfield(L, -2, "skills");
    return 1;
}

static int integer_field(lua_State *L, const char *name, int64_t low, int64_t high)
{
    lua_getfield(L, 2, name);
    int64_t value = p4_lua_integer(L, -1, low, high);
    lua_pop(L, 1);
    return (int)value;
}
static int party_set(lua_State *L)
{
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.party.set is refused in synchronous UI/action callbacks and save snapshots");
    P4LuaMod *mod = p4_luau_user(L);
    int id = (int)p4_lua_integer(L, 1, 2, 10);
    luaL_checktype(L, 2, LUA_TTABLE);
    P4ModPartyRecord record = {0};
    record.persona = (uint16_t)integer_field(L, "persona", 1, 0xffff);
    record.level = (uint8_t)integer_field(L, "level", 1, 99);
    lua_getfield(L, 2, "exp");
    record.exp = (uint32_t)p4_lua_integer(L, -1, 0, INT32_MAX);
    lua_pop(L, 1);
    lua_getfield(L, 2, "stats");
    luaL_checktype(L, -1, LUA_TTABLE);
    if (lua_objlen(L, -1) != 5) luaL_error(L, "p4.party.set: stats holds five values (St Ma En Ag Lu)");
    for (int i = 0; i < 5; ++i) {
        lua_rawgeti(L, -1, i + 1);
        record.stats[i] = (uint8_t)p4_lua_integer(L, -1, 1, 99);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    lua_getfield(L, 2, "skills");
    luaL_checktype(L, -1, LUA_TTABLE);
    int count = lua_objlen(L, -1);
    if (count > 8) luaL_error(L, "p4.party.set: at most eight skills");
    for (int i = 0; i < count; ++i) {
        lua_rawgeti(L, -1, i + 1);
        record.skills[i] = (uint16_t)p4_lua_integer(L, -1, 1, 0xffff);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    char reason[192];
    if (p4_mod_party_queue(mod->owner, id, &record, reason, sizeof reason)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    /* Queued for the frame safe point. Changing a character's record is actual
     * use of the mod for this save. */
    p4_lua_semantic_use(mod);
    lua_pushboolean(L, 1);
    return 1;
}

void p4_lua_party_close(P4LuaMod *mod)
{
    p4_mod_party_clear_owner(mod->owner);
}

void p4_lua_party_initialize(lua_State *L)
{
    p4_mod_events_set_party(p4_mod_party_apply_pending);
    p4_mod_party_set_actions(p4_mod_party_first_action); /* a write waits for actions asked for before it */
    static const luaL_Reg methods[] = {{"record", party_record}, {"set", party_set}, {NULL, NULL}};
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "party");
}
