/* p4.lifecycle: port session identity and crash markers (docs/modding/lua.md "Lifecycle"). */
#include "lua_runtime.h"
#include "p4pc/mod_lifecycle.h"
#include <stdint.h>
#include <stdio.h>

/* Durable writes: only from frames that may take native effects. */
static void require_effects(lua_State *L, const char *what)
{
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.lifecycle.%s is refused in synchronous UI/action callbacks and save snapshots", what);
}
static uint64_t checked_session(lua_State *L)
{
    /* Sessions count launches; the scalar API's bound is far beyond any device's. */
    return (uint64_t)p4_lua_integer(L, 1, 1, INT32_MAX);
}
static int failed(lua_State *L, int value, const char *reason)
{
    if (value) lua_pushboolean(L, 0);
    else lua_pushnil(L);
    lua_pushstring(L, reason);
    return 2;
}

static int lifecycle_session(lua_State *L)
{
    require_effects(L, "session");
    char reason[192];
    uint64_t session = p4_mod_lifecycle_session(reason, sizeof reason);
    if (!session) return failed(L, 0, reason);
    lua_pushnumber(L, (double)session);
    return 1;
}
static int lifecycle_tag(lua_State *L)
{
    require_effects(L, "tag");
    P4LuaMod *mod = p4_luau_user(L);
    const char *tag = lua_isnoneornil(L, 1) ? NULL : luaL_checkstring(L, 1);
    char reason[192];
    if (p4_mod_lifecycle_tag(mod->info->id, tag, reason, sizeof reason)) return failed(L, 1, reason);
    lua_pushboolean(L, 1);
    return 1;
}
static int lifecycle_crashed(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    uint64_t session = checked_session(L);
    char tag[P4_MOD_LIFECYCLE_TAG_MAX + 1], reason[192];
    int found = p4_mod_lifecycle_crashed(session, mod->info->id, tag, sizeof tag, reason, sizeof reason);
    if (found < 0) return failed(L, 0, reason);
    lua_pushboolean(L, found);
    if (!found) return 1;
    if (tag[0]) lua_pushstring(L, tag);
    else lua_pushnil(L);
    return 2;
}
static int lifecycle_consume(lua_State *L)
{
    require_effects(L, "consume");
    P4LuaMod *mod = p4_luau_user(L);
    uint64_t session = checked_session(L);
    char reason[192];
    if (p4_mod_lifecycle_consume(session, mod->info->id, reason, sizeof reason)) return failed(L, 1, reason);
    lua_pushboolean(L, 1);
    return 1;
}

void p4_lua_lifecycle(lua_State *L)
{
    static const luaL_Reg methods[] = {{"session", lifecycle_session}, {"tag", lifecycle_tag},
        {"crashed", lifecycle_crashed}, {"consume", lifecycle_consume}, {NULL, NULL}};
    p4_lua_library(L, methods);
}
