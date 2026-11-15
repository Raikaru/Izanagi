/* p4.artifacts: mod-owned files beside p4pc.ini (API 1.9, docs/modding/lua.md "Artifacts"). */
#include "lua_runtime.h"
#include "p4pc/mod_artifacts.h"
#include <stdio.h>
#include <string.h>

static int failed(lua_State *L, int value, const char *reason)
{
    if (value) lua_pushboolean(L, 0);
    else lua_pushnil(L);
    lua_pushstring(L, reason);
    return 2;
}
static const char *checked_path(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    if (!p4_mod_artifact_path_valid(path))
        luaL_argerror(L, 1, "artifact paths are 1..8 '/'-separated [A-Za-z0-9_.-] names, 128 bytes at most");
    return path;
}
static int artifact_write(lua_State *L)
{
    p4_lua_require_mutation(L);
    /* A synchronous disk write: only from frames that may take native effects. */
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.artifacts.write is refused in synchronous UI/action callbacks and save snapshots");
    P4LuaMod *mod = p4_luau_user(L);
    const char *path = checked_path(L);
    size_t size = 0;
    const void *bytes = NULL;
    if (lua_type(L, 2) == LUA_TSTRING) bytes = lua_tolstring(L, 2, &size);
    else bytes = luaL_checkbuffer(L, 2, &size);
    if (size > P4_MOD_ARTIFACT_FILE_MAX) luaL_argerror(L, 2, "artifacts are at most 4 MiB");
    char reason[256];
    if (p4_mod_artifact_write(mod->info->id, path, bytes, size, reason, sizeof reason)) return failed(L, 1, reason);
    lua_pushboolean(L, 1);
    return 1;
}
static int artifact_read(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    const char *path = checked_path(L);
    size_t maximum = lua_isnoneornil(L, 2) ? P4_MOD_ARTIFACT_FILE_MAX
                                          : (size_t)p4_lua_integer(L, 2, 0, P4_MOD_ARTIFACT_FILE_MAX);
    char reason[256] = {0};
    int64_t size = p4_mod_artifact_read(mod->info->id, path, NULL, maximum, reason, sizeof reason);
    if (size >= 0) {
        void *out = lua_newbuffer(L, (size_t)size);
        size = p4_mod_artifact_read(mod->info->id, path, out, (size_t)size, reason, sizeof reason);
        if (size >= 0) return 1;
        lua_pop(L, 1);
    }
    return failed(L, 0, size == -1 ? "no such artifact" : reason);
}
static int artifact_folder(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    char folder[1400];
    if (p4_mod_artifact_folder(mod->info->id, folder, sizeof folder)) lua_pushnil(L);
    else lua_pushstring(L, folder);
    return 1;
}

void p4_lua_artifacts(lua_State *L)
{
    static const luaL_Reg methods[] = {{"write", artifact_write}, {"read", artifact_read},
        {"folder", artifact_folder}, {NULL, NULL}};
    p4_lua_library(L, methods);
}
