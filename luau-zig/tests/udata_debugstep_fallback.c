/*
 * Regression: single-step debugstep hook count must not change when a
 * GETUDATAKS / SETUDATAKS / NAMECALLUDATA instruction falls back to its generic
 * form (GETTABLEKS / SETTABLEKS / NAMECALL).
 *
 * Upstream Luau (lvmexecute.cpp) rewrites the instruction and jumps straight
 * into the generic handler with VM_CONTINUE, so the debugstep hook fires once
 * for that instruction. The fallback is permanent, so the first execution and
 * every later execution of the same function produce the same number of hook
 * calls. This fixture compares those counts; it never inspects bytecode PCs.
 *
 * Setup: a useratom callback assigns atoms to the keys "x" and "m", so
 * luau_load rewrites `t.x`, `t.x = v` and `t:m()` into the userdata ops. The
 * receivers are plain tables, which forces the fallback on first execution.
 *
 * Exit status: 0 pass, 1 hook-count mismatch, 2 setup failure,
 * 77 the key string has no atom (the loader's rewrite predicate is false).
 *
 * The Zig candidate is built and run by `zig build udata-debugstep-regression`.
 * Without LUAU_ZIG_CANDIDATE, link against the pinned upstream Luau compiler
 * and VM to compare reference behavior.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lualib.h"

#ifdef LUAU_ZIG_CANDIDATE
/* luau-zig/tests/upstream_compiler_bridge.zig; storage is libc malloc. */
extern int zig_conformance_compile(const char* source, size_t length, int optimization, int debug, int type_info, int coverage,
    const char* case_name, char** bytes, size_t* size);

static char* compile_source(const char* source, size_t* outsize)
{
    char* bytes = NULL;
    if (zig_conformance_compile(source, strlen(source), 1, 1, 0, 0, "UdataDebugStepFallback", &bytes, outsize) != 0)
        return NULL;
    return bytes;
}
#else
#include "luacode.h"

static char* compile_source(const char* source, size_t* outsize)
{
    lua_CompileOptions options;
    memset(&options, 0, sizeof(options));
    options.optimizationLevel = 1;
    options.debugLevel = 1;
    return luau_compile(source, strlen(source), &options, outsize);
}
#endif

static long step_count;

static int16_t fixture_useratom(lua_State* L, const char* s, size_t l)
{
    (void)L;
    if (l == 1 && s[0] == 'x')
        return 0;
    if (l == 1 && s[0] == 'm')
        return 1;
    return -1;
}

static void fixture_debugstep(lua_State* L, lua_Debug* ar)
{
    (void)L;
    (void)ar;
    step_count++;
}

static int fixture_method(lua_State* L)
{
    (void)L;
    return 0;
}

/* Pushes a fresh plain table receiver { x = 1, m = fixture_method }. */
static void push_receiver(lua_State* L)
{
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, 1);
    lua_setfield(L, -2, "x");
    lua_pushcfunction(L, fixture_method, "m");
    lua_setfield(L, -2, "m");
}

struct Case
{
    const char* name;
    const char* source;
    const char* key;
    int extra_number_arg;
};

static const struct Case cases[] = {
    {"GETUDATAKS->GETTABLEKS", "return function(t) local v = t.x return v end", "x", 0},
    {"SETUDATAKS->SETTABLEKS", "return function(t, v) t.x = v end", "x", 1},
    {"NAMECALLUDATA->NAMECALL", "return function(t) t:m() end", "m", 0},
};

/* Calls the function at stack index fn_index once under single-step and returns the hook count. */
static long stepped_call(lua_State* L, int fn_index, const struct Case* c, int* failed)
{
    lua_pushvalue(L, fn_index);
    push_receiver(L);
    if (c->extra_number_arg)
        lua_pushnumber(L, 2);

    step_count = 0;
    lua_singlestep(L, 1);
    int status = lua_pcall(L, c->extra_number_arg ? 2 : 1, 0, 0);
    lua_singlestep(L, 0);
    long count = step_count;

    if (status != LUA_OK)
    {
        const char* msg = lua_tostring(L, -1);
        fprintf(stderr, "%s: call failed: %s\n", c->name, msg ? msg : "(non-string error)");
        lua_pop(L, 1);
        *failed = 1;
    }
    return count;
}

static int run_case(lua_State* L, const struct Case* c)
{
    size_t size = 0;
    char* bytecode = compile_source(c->source, &size);
    if (!bytecode)
    {
        fprintf(stderr, "%s: compile failed\n", c->name);
        return 2;
    }
    int load_status = luau_load(L, c->name, bytecode, size, 0);
    free(bytecode);
    if (load_status != 0)
    {
        const char* msg = lua_tostring(L, -1);
        fprintf(stderr, "%s: load failed: %s\n", c->name, msg ? msg : "(non-string error)");
        lua_pop(L, 1);
        return 2;
    }

    /* luau_load rewrites a string-keyed access only when the key's atom is >= 0. */
    int atom = -1;
    lua_pushstring(L, c->key);
    lua_tostringatom(L, -1, &atom);
    lua_pop(L, 1);
    if (atom < 0)
    {
        fprintf(stderr, "%s: key '%s' has no atom; userdata op rewrite not exercised\n", c->name, c->key);
        lua_pop(L, 1);
        return 77;
    }

    /* Run the chunk without single-step to obtain the function under test. */
    if (lua_pcall(L, 0, 1, 0) != LUA_OK || !lua_isfunction(L, -1))
    {
        fprintf(stderr, "%s: chunk did not return a function\n", c->name);
        lua_settop(L, 0);
        return 2;
    }
    int fn_index = lua_gettop(L);

    int failed = 0;
    long first = stepped_call(L, fn_index, c, &failed);
    long second = stepped_call(L, fn_index, c, &failed);
    long third = stepped_call(L, fn_index, c, &failed);
    lua_settop(L, 0);

    if (failed)
        return 2;

    printf("%s: debugstep calls first=%ld second=%ld third=%ld\n", c->name, first, second, third);

    if (second <= 0)
    {
        fprintf(stderr, "%s: debugstep hook never fired\n", c->name);
        return 2;
    }
    if (first != second || second != third)
    {
        fprintf(stderr, "%s: FAIL first execution (fallback) produced %ld debugstep calls, later executions %ld/%ld\n", c->name, first,
            second, third);
        return 1;
    }
    return 0;
}

int main(void)
{
    lua_State* L = luaL_newstate();
    if (!L)
    {
        fprintf(stderr, "luaL_newstate failed\n");
        return 2;
    }

    lua_Callbacks* cb = lua_callbacks(L);
    cb->useratom = fixture_useratom;
    cb->debugstep = fixture_debugstep;

    int result = 0;
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        int r = run_case(L, &cases[i]);
        /* Report the most severe outcome: setup failure, then mismatch, then skip. */
        if (r == 2 || (r == 1 && result != 2) || (r == 77 && result == 0))
            result = r;
    }

    lua_close(L);
    printf("%s\n", result == 0 ? "PASS" : result == 1 ? "FAIL" : result == 77 ? "SKIP" : "ERROR");
    return result;
}
