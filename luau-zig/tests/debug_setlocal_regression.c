/* Consumer debugger regression extending the pinned Debugger local checks. */
#include "lua.h"
#include "lualib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef LUAU_ZIG_CANDIDATE
extern int zig_conformance_compile(const char*, size_t, int, int, int, int,
    const char*, char**, size_t*);
static char* compile_source(const char* source, size_t* size)
{
    char* bytes = NULL;
    if (zig_conformance_compile(source, strlen(source), 0, 2, 0, 0, "", &bytes, size))
        return NULL;
    return bytes;
}
#else
#include "luacode.h"
static char* compile_source(const char* source, size_t* size)
{
    lua_CompileOptions options = {0};
    options.debugLevel = 2;
    return luau_compile(source, strlen(source), &options, size);
}
#endif

#define REQUIRE(L, x) do { if (!(x)) luaL_error((L), "debug_setlocal line %d: %s", __LINE__, #x); } while (0)

static int probe(lua_State* L)
{
    int top = lua_gettop(L);
    lua_pushinteger(L, 17);
    REQUIRE(L, lua_setlocal(L, -1, 1) == NULL);
    REQUIRE(L, lua_gettop(L) == top + 1 && lua_tointeger(L, -1) == 17);
    REQUIRE(L, lua_setlocal(L, 999, 1) == NULL);
    REQUIRE(L, lua_gettop(L) == top + 1 && lua_tointeger(L, -1) == 17);
    /* A valid C frame and a valid Lua frame with no such local consume it. */
    REQUIRE(L, lua_setlocal(L, 0, 1) == NULL);
    REQUIRE(L, lua_gettop(L) == top);
    lua_pushinteger(L, 19);
    REQUIRE(L, lua_setlocal(L, 1, 999) == NULL);
    REQUIRE(L, lua_gettop(L) == top);
    const char* name = lua_getlocal(L, 1, 1);
    REQUIRE(L, name && strcmp(name, "x") == 0 && lua_tointeger(L, -1) == 11);
    lua_pop(L, 1);
    lua_pushinteger(L, 23);
    name = lua_setlocal(L, 1, 1);
    REQUIRE(L, name && strcmp(name, "x") == 0 && lua_gettop(L) == top);
    return 0;
}

int main(void)
{
    const char* source = "local x = 11; probe(); assert(x == 23); return x";
    lua_State* L = luaL_newstate();
    if (!L) return 1;
    luaL_openlibs(L);
    lua_pushinteger(L, 41);
    if (lua_setlocal(L, 0, 1) != NULL || lua_gettop(L) != 1 || lua_tointeger(L, -1) != 41) {
        fprintf(stderr, "invalid base frame changed stack\n");
        lua_close(L);
        return 1;
    }
    lua_pop(L, 1);
    lua_pushcfunction(L, probe, "probe");
    lua_setglobal(L, "probe");
    size_t size = 0;
    char* bytes = compile_source(source, &size);
    if (!bytes) { lua_close(L); return 1; }
    int result = luau_load(L, "=debug_setlocal", bytes, size, 0);
    free(bytes);
    if (!result) result = lua_pcall(L, 0, 1, 0);
    if (result || lua_tointeger(L, -1) != 23) {
        fprintf(stderr, "debug setlocal: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return 1;
    }
    lua_close(L);
    puts("debug setlocal stack and assignment: PASS");
    return 0;
}
