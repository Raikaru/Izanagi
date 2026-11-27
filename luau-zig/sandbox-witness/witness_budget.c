/* CPU budget: the interrupt callback, the bridge's sticky latch and every way a
 * script could try to outlive or hide budget exhaustion.
 *
 * Contract (include/p4pc/luau_c.h): the interrupt runs at VM safepoints,
 * including GC; a nonzero return latches exhaustion until
 * p4_luau_reset_interrupt. p4_luau_run follows the lua_pcall stack contract but
 * reports a latched exhaustion as LUA_ERRRUN even if a script-side pcall
 * caught the original error. The callback receives config.user. */
#include "witness.h"
#include <limits.h>

struct budget {
    unsigned exec_calls;
    unsigned gc_calls;
    unsigned exec_limit;
    int trip_on_gc;
    unsigned wrong_user;
};
static struct budget *expected_budget;

static int interrupt(void *user, int gc)
{
    struct budget *budget = user;
    if (budget != expected_budget) ++budget->wrong_user;
    if (gc) {
        ++budget->gc_calls;
        return budget->trip_on_gc;
    }
    ++budget->exec_calls;
    return budget->exec_calls >= budget->exec_limit;
}

/* host_call(f): an unprotected re-entry into Lua from a host function. */
static int host_call(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_settop(L, 1);
    lua_call(L, 0, 0);
    return 0;
}
/* host_swallow(f): a host function that catches the error itself. */
static int host_swallow(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_settop(L, 1);
    int status = lua_pcall(L, 0, 0, 0);
    lua_pushboolean(L, status == LUA_OK);
    return 1;
}
static void initialize(lua_State *L, void *user)
{
    (void)user;
    lua_pushcfunction(L, host_call, "host_call");
    lua_setglobal(L, "host_call");
    lua_pushcfunction(L, host_swallow, "host_swallow");
    lua_setglobal(L, "host_swallow");
}

static void reset(p4_luau *vm, struct budget *budget, unsigned limit, int trip_on_gc)
{
    p4_luau_reset_interrupt(vm);
    budget->exec_calls = budget->gc_calls = 0;
    budget->exec_limit = limit;
    budget->trip_on_gc = trip_on_gc;
}

/* The latch alone keeps the VM failing: the callback is made permissive first,
 * so only the bridge's sticky state can produce these errors. */
static int expect_latched_then_recover(p4_luau *vm, struct budget *budget)
{
    lua_State *L = p4_luau_state(vm);
    budget->exec_calls = 0;
    budget->exec_limit = UINT_MAX;
    budget->trip_on_gc = 0;
    CHECK(witness_run(vm, "@budget-sticky.luau", "return 1", 1) == LUA_ERRRUN);
    CHECK_VM(vm, witness_error_has(vm, "Execution budget exhausted"));
    CHECK(lua_gettop(L) == 0);
    p4_luau_reset_interrupt(vm);
    CHECK_VM(vm, witness_run(vm, "@budget-recovered.luau", "return 6 * 7", 1) == LUA_OK);
    CHECK(lua_tointeger(L, -1) == 42);
    lua_pop(L, 1);
    return 0;
}

static int expect_exhausted(p4_luau *vm, struct budget *budget, unsigned limit, const char *name,
                            const char *source, const char *forbidden)
{
    lua_State *L = p4_luau_state(vm);
    reset(vm, budget, limit, 0);
    int status = witness_run(vm, name, source, LUA_MULTRET);
    if (status != LUA_ERRRUN || !witness_error_has(vm, "Execution budget exhausted")) {
        fprintf(stderr, "%s: expected budget exhaustion, got status %d error '%s'\n", name, status,
            p4_luau_error(vm));
        return 1;
    }
    CHECK(lua_gettop(L) == 0);
    CHECK(budget->exec_calls >= limit);
    if (forbidden && witness_error_has(vm, forbidden)) {
        fprintf(stderr, "%s: script text '%s' replaced the budget diagnostic: %s\n", name, forbidden,
            p4_luau_error(vm));
        return 1;
    }
    return expect_latched_then_recover(vm, budget);
}

static int script_escapes(p4_luau *vm, struct budget *budget)
{
    static const struct { const char *name, *source, *forbidden; } cases[] = {
        {"@budget-loop.luau", "local n = 0\nwhile true do n += 1 end", NULL},
        {"@budget-pcall.luau", "local ok = pcall(function() while true do end end)\nreturn 'escaped'", NULL},
        {"@budget-pcall-retry.luau", "while true do pcall(function() while true do end end) end", NULL},
        {"@budget-xpcall.luau",
         "xpcall(function() while true do end end, function(e) while true do end end)\nreturn 'escaped'", NULL},
        {"@budget-spoof.luau", "pcall(function() while true do end end)\nerror('spoofed result')", "spoofed"},
        {"@budget-resume.luau",
         "local co = coroutine.create(function() while true do end end)\n"
         "local ok, err = coroutine.resume(co)\nreturn ok, err", NULL},
        {"@budget-wrap.luau",
         "local f = coroutine.wrap(function() while true do end end)\nlocal ok = pcall(f)\nreturn ok", NULL},
        {"@budget-pingpong.luau",
         "local co = coroutine.wrap(function() while true do coroutine.yield() end end)\n"
         "while true do co() end", NULL},
        {"@budget-recursion.luau", "local function f(n) return 1 + f(n + 1) end\nreturn f(0)", NULL},
        {"@budget-sort.luau", "local t = {3, 1, 2}\ntable.sort(t, function(a, b) while true do end end)", NULL},
        {"@budget-gsub.luau", "return string.gsub('aaa', 'a', function() while true do end end)", NULL},
        {"@budget-index.luau",
         "local t = setmetatable({}, {__index = function() while true do end end})\nreturn t.x", NULL},
        {"@budget-tostring.luau",
         "return tostring(setmetatable({}, {__tostring = function() while true do end end}))", NULL},
        {"@budget-host-call.luau", "host_call(function() while true do end end)\nreturn 'escaped'", NULL},
        {"@budget-host-swallow.luau",
         "local ok = host_swallow(function() while true do end end)\nassert(ok == false)\n"
         "error('host swallowed budget')", "host swallowed"},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i)
        if (expect_exhausted(vm, budget, 2000, cases[i].name, cases[i].source, cases[i].forbidden)) return 1;
    return 0;
}

/* The interrupt is charged at every loop back-edge; GC safepoints are reported
 * with gc != 0 and never raise by themselves. */
static int accounting(p4_luau *vm, struct budget *budget)
{
    lua_State *L = p4_luau_state(vm);
    reset(vm, budget, UINT_MAX, 0);
    CHECK_VM(vm, witness_run(vm, "@budget-accounting.luau",
        "local t, n = nil, 0\nfor i = 1, 200000 do t = {i}; n += 1 end\nreturn n", 1) == LUA_OK);
    CHECK(lua_tointeger(L, -1) == 200000);
    lua_pop(L, 1);
    CHECK(budget->exec_calls >= 200000);
    CHECK(budget->gc_calls > 0);
    CHECK(budget->wrong_user == 0);

    /* A GC-phase trip latches without raising inside the collector; the next
     * execution safepoint raises, long before the loop could finish. */
    reset(vm, budget, UINT_MAX, 1);
    CHECK(witness_run(vm, "@budget-gc-trip.luau",
        "local t\nfor i = 1, 10000000 do t = {i} end\nreturn 'finished'", 1) == LUA_ERRRUN);
    CHECK_VM(vm, witness_error_has(vm, "Execution budget exhausted"));
    CHECK(budget->gc_calls > 0 && budget->exec_calls < 10000000);
    CHECK(lua_gettop(L) == 0);
    return expect_latched_then_recover(vm, budget);
}

/* p4_luau_run keeps the lua_pcall stack contract on exhaustion: the function and
 * its arguments are removed, values below them are untouched. */
static int stack_contract(p4_luau *vm, struct budget *budget)
{
    static const char source[] = "local a, b = ...\nwhile true do a += b end";
    lua_State *L = p4_luau_state(vm);
    reset(vm, budget, 2000, 0);
    lua_pushstring(L, "sentinel");
    CHECK_VM(vm, p4_luau_load_source(vm, "@budget-args.luau", source, sizeof source - 1) == LUA_OK);
    lua_pushinteger(L, 1);
    lua_pushinteger(L, 2);
    CHECK(p4_luau_run(vm, 2, LUA_MULTRET) == LUA_ERRRUN);
    CHECK_VM(vm, witness_error_has(vm, "Execution budget exhausted"));
    CHECK(lua_gettop(L) == 1);
    CHECK(lua_type(L, 1) == LUA_TSTRING && strcmp(lua_tostring(L, 1), "sentinel") == 0);
    lua_settop(L, 0);
    /* Compile-only admission neither runs nor clears the latch. */
    CHECK(p4_luau_check_source(vm, "@budget-check.luau", "return 1", 8) == LUA_OK);
    return expect_latched_then_recover(vm, budget);
}

/* The production mod runtime runs callbacks with lua_pcall on host-created
 * threads, not p4_luau_run. The latch is VM-wide: exhaustion on that thread
 * blocks the main state until the host resets it. */
static int host_thread(p4_luau *vm, struct budget *budget)
{
    static const char source[] = "while true do end";
    lua_State *L = p4_luau_state(vm);
    struct thread_create created = {NULL, LUA_NOREF};
    CHECK(lua_cpcall(L, witness_create_thread, &created) == LUA_OK);
    lua_State *T = created.state;
    reset(vm, budget, 2000, 0);
    CHECK_VM(vm, p4_luau_load_source(vm, "@budget-thread.luau", source, sizeof source - 1) == LUA_OK);
    lua_xmove(L, T, 1);
    CHECK(lua_pcall(T, 0, 0, 0) == LUA_ERRRUN);
    CHECK(lua_type(T, -1) == LUA_TSTRING && strstr(lua_tostring(T, -1), "Execution budget exhausted"));
    lua_settop(T, 0);
    CHECK(expect_latched_then_recover(vm, budget) == 0);
    lua_unref(L, created.reference);
    CHECK(lua_gettop(L) == 0);
    return 0;
}

/* Latches and callbacks are per VM; a VM without an interrupt runs normally. */
static int isolation(p4_luau *vm, struct budget *budget)
{
    p4_luau_config config = {4u * 1024u * 1024u, NULL, NULL, NULL, 0};
    p4_luau *unbounded = p4_luau_create(&config);
    CHECK(unbounded != NULL);
    reset(vm, budget, 2000, 0);
    CHECK(witness_run(vm, "@budget-isolation.luau", "while true do end", 0) == LUA_ERRRUN);
    CHECK_VM(unbounded, witness_run(unbounded, "@unbounded.luau",
        "local n = 0\nfor i = 1, 1000000 do n += i end\nreturn n", 1) == LUA_OK);
    CHECK(lua_tonumber(p4_luau_state(unbounded), -1) == 500000500000.0);
    lua_pop(p4_luau_state(unbounded), 1);
    p4_luau_destroy(unbounded);
    return expect_latched_then_recover(vm, budget);
}

int main(void)
{
    struct budget budget = {0, 0, UINT_MAX, 0, 0};
    expected_budget = &budget;
    p4_luau_config config = {4u * 1024u * 1024u, interrupt, &budget, initialize, 0};
    p4_luau *vm = p4_luau_create(&config);
    CHECK(vm != NULL);
    SECTION(script_escapes(vm, &budget));
    SECTION(accounting(vm, &budget));
    SECTION(stack_contract(vm, &budget));
    SECTION(host_thread(vm, &budget));
    SECTION(isolation(vm, &budget));
    CHECK(budget.wrong_user == 0);
    p4_luau_destroy(vm);
    puts("sandbox witness budget: loop/recursion/pcall/xpcall/coroutine/C-boundary/host-thread "
         "exhaustion, GC-phase latch, sticky reset and per-VM isolation passed");
    return 0;
}
