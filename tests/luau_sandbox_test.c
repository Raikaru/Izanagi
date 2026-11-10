#include "p4pc/luau_c.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #condition); return 1; } } while (0)
struct budget { unsigned calls; unsigned limit; int value; unsigned reads; };
static int interrupt(void *user, int gc)
{
    struct budget *budget = user;
    if (!gc) ++budget->calls;
    return budget->calls >= budget->limit;
}
static int context(lua_State *L)
{
    lua_pushboolean(L, p4_luau_user(L) != NULL);
    return 1;
}
static int view_index(lua_State *L)
{
    struct budget *budget = p4_luau_user(L);
    if (strcmp(luaL_checkstring(L, 2), "value") != 0) return 0;
    ++budget->reads;
    lua_pushinteger(L, budget->value);
    return 1;
}
static int view_newindex(lua_State *L)
{
    struct budget *budget = p4_luau_user(L);
    if (strcmp(luaL_checkstring(L, 2), "value") != 0) luaL_error(L, "unknown view field");
    budget->value = luaL_checkinteger(L, 3);
    return 0;
}

static void initialize(lua_State *L, void *user)
{
    (void)user;
    lua_pushcfunction(L, context, "host_context");
    lua_setglobal(L, "host_context");
    (void)lua_newuserdata(L, 1);
    lua_newtable(L);
    lua_pushcfunction(L, view_index, "view_index");
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, view_newindex, "view_newindex");
    lua_setfield(L, -2, "__newindex");
    lua_setreadonly(L, -1, 1);
    lua_setmetatable(L, -2);
    lua_setglobal(L, "view");
}
static int run(p4_luau *vm, const char *source)
{
    int status = p4_luau_load_source(vm, "@sandbox-test.luau", source, strlen(source));
    if (status == LUA_OK) status = p4_luau_run(vm, 0, 1);
    return status;
}
int main(void)
{
    struct budget budget = {0, 10000, -17, 0};
    p4_luau_config config = {1024 * 1024, interrupt, &budget, initialize, 65536};
    p4_luau *vm = p4_luau_create(&config);
    CHECK(vm != NULL);
    CHECK(((uintptr_t)p4_luau_state(vm) & 15u) == 0);
    CHECK(p4_luau_memory_used(vm) <= config.memory_limit);
    {
        /* A type error is valid compiler input. Neither this host callback nor
         * error() may execute during compile-only admission. */
        const char source[] = "local n: string = 42\nhost_context()\nerror('must not run')";
        size_t used = p4_luau_memory_used(vm);
        unsigned calls = budget.calls;
        int top = lua_gettop(p4_luau_state(vm));
        CHECK(p4_luau_check_source(vm, "@compile-only.luau", source, sizeof(source) - 1) == LUA_OK);
        CHECK(p4_luau_error(vm)[0] == 0);
        CHECK(p4_luau_check_source(vm, "@compile-bad.luau", "local =", 7) == LUA_ERRSYNTAX);
        CHECK(strstr(p4_luau_error(vm), "compile-bad.luau:1") != NULL);
        CHECK(p4_luau_check_source(vm, "@compile-limit", "", config.source_limit + 1) == LUA_ERRSYNTAX);
        CHECK(p4_luau_check_source(vm, "@compile-null", NULL, 1) == LUA_ERRSYNTAX);
        CHECK(p4_luau_check_source(vm, NULL, "", 0) == LUA_ERRSYNTAX);
        CHECK(p4_luau_check_source(vm, "@compile-empty", NULL, 0) == LUA_OK);
        CHECK(p4_luau_error(vm)[0] == 0);
        CHECK(lua_gettop(p4_luau_state(vm)) == top);
        CHECK(p4_luau_memory_used(vm) == used);
        CHECK(budget.calls == calls);
    }
    /* GETIMPORT must not snapshot mutable host-backed fields at load, even
     * within the first chunk or when a later chunk was loaded before a write. */
    {
        const char first[] = "assert(view.value == -17); view.value = -18; return view.value";
        const char second[] = "assert(view.value == -18); view.value = -19; return view.value";
        lua_State *L = p4_luau_state(vm);
        CHECK(p4_luau_load_source(vm, "@first-view.luau", first, sizeof(first) - 1) == LUA_OK);
        CHECK(p4_luau_load_source(vm, "@second-view.luau", second, sizeof(second) - 1) == LUA_OK);
        lua_insert(L, -2);
        CHECK(p4_luau_run(vm, 0, 1) == LUA_OK);
        CHECK(lua_tointeger(L, -1) == -18 && budget.value == -18 && budget.reads >= 2);
        lua_pop(L, 1);
        CHECK(p4_luau_run(vm, 0, 1) == LUA_OK);
        CHECK(lua_tointeger(L, -1) == -19 && budget.value == -19);
        lua_pop(L, 1);
        CHECK(run(vm, "return view.value") == LUA_OK);
        CHECK(lua_tointeger(L, -1) == -19);
        lua_pop(L, 1);
    }
    CHECK(run(vm,
        "assert(os == nil and io == nil and debug == nil and require == nil)\n"
        "assert(load == nil and loadstring == nil and loadfile == nil and dofile == nil)\n"
        "assert(integer == nil and vector == nil and class == nil)\n"
        "assert(type(pcall) == 'function' and type(getfenv) == 'function')\n"
        "assert(type(setfenv) == 'function' and type(newproxy) == 'function')\n"
        "assert(type(coroutine.create) == 'function' and type(buffer.create) == 'function')\n"
        "assert(not pcall(function() table.insert = false end))\n"
        "local co = coroutine.create(function() assert(host_context()); return 42 end)\n"
        "local ok, value = coroutine.resume(co); assert(ok); return value") == LUA_OK);
    CHECK(lua_tointeger(p4_luau_state(vm), -1) == 42);
    lua_pop(p4_luau_state(vm), 1);
    CHECK(p4_luau_load_source(vm, "@bad.luau", "local =", 7) != LUA_OK);
    CHECK(strstr(p4_luau_error(vm), "bad.luau:1") != NULL);
    CHECK(lua_gettop(p4_luau_state(vm)) == 0);
    CHECK(run(vm, "error('source-position')") == LUA_ERRRUN);
    CHECK(strstr(p4_luau_error(vm), "sandbox-test.luau:1") != NULL);
    CHECK(run(vm, "error({reason = 'opaque'})") == LUA_ERRRUN);
    CHECK(strstr(p4_luau_error(vm), "non-string error value") != NULL);
    CHECK(p4_luau_load_source(vm, "@too-big", "", config.source_limit + 1) == LUA_ERRSYNTAX);
    CHECK(run(vm, "return buffer.create(2 * 1024 * 1024)") == LUA_ERRMEM);
    CHECK(p4_luau_memory_used(vm) <= config.memory_limit);
    budget.calls = 0;
    budget.limit = 32;
    CHECK(run(vm, "local n = 0; while true do n += 1 end") == LUA_ERRRUN);
    CHECK(strstr(p4_luau_error(vm), "Execution budget exhausted") != NULL);
    CHECK(lua_gettop(p4_luau_state(vm)) == 0);
    CHECK(p4_luau_check_source(vm, "@still-interrupted", "return 42", 9) == LUA_OK);
    CHECK(run(vm, "return pcall(function() return 1 end)") == LUA_ERRRUN);
    p4_luau_reset_interrupt(vm);
    budget.calls = 0;
    CHECK(run(vm, "return pcall(function() while true do end end)") == LUA_ERRRUN);
    CHECK(strstr(p4_luau_error(vm), "Execution budget exhausted") != NULL);
    p4_luau_reset_interrupt(vm);
    budget.calls = 0;
    budget.limit = 10000;
    CHECK(run(vm, "return 6 * 7") == LUA_OK);
    CHECK(lua_tointeger(p4_luau_state(vm), -1) == 42);
    p4_luau_destroy(vm);
    config.memory_limit = 1;
    CHECK(p4_luau_create(&config) == NULL);
    puts("luau sandbox: compile-only admission, execution, isolation, diagnostics, memory cap and sticky interrupt passed");
    return 0;
}
