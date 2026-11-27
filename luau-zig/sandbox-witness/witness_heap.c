/* Host-heap cleanup across the whole p4_luau lifecycle.
 *
 * The bridge object, compiler scratch and compiled blob are host (libc) memory
 * and not charged to the VM cap, so p4_luau_memory_used cannot see them. This
 * witness measures the process heap's in-use bytes around many complete
 * lifecycles: failed creation at too-small caps, compile-only checks, syntax
 * errors, oversized sources, normal runs, runtime errors, budget exhaustion and
 * reset, memory exhaustion, and destroy. Any per-cycle leak of a heap block
 * accumulates far beyond the tolerance.
 *
 * Platforms without an in-use heap statistic exit 77 (SKIP), never PASS. */
#include "witness.h"
#include <limits.h>

#if defined(_WIN32)
#include <windows.h>
static int heap_in_use(size_t *out)
{
    HANDLE heaps[256];
    DWORD count = GetProcessHeaps(256, heaps);
    if (count == 0 || count > 256) return 0;
    size_t total = 0;
    for (DWORD i = 0; i < count; ++i) {
        if (!HeapLock(heaps[i])) continue;
        PROCESS_HEAP_ENTRY entry;
        entry.lpData = NULL;
        while (HeapWalk(heaps[i], &entry))
            if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY) total += entry.cbData;
        DWORD error = GetLastError();
        HeapUnlock(heaps[i]);
        if (error != ERROR_NO_MORE_ITEMS) return 0;
    }
    *out = total;
    return 1;
}
#elif defined(__GLIBC__)
#include <malloc.h>
static int heap_in_use(size_t *out)
{
#if __GLIBC__ > 2 || (__GLIBC__ == 2 && __GLIBC_MINOR__ >= 33)
    struct mallinfo2 info = mallinfo2();
    *out = info.uordblks + info.hblkhd;
#else
    struct mallinfo info = mallinfo();
    *out = (size_t)(unsigned)info.uordblks + (size_t)(unsigned)info.hblkhd;
#endif
    return 1;
}
#elif defined(__APPLE__)
#include <malloc/malloc.h>
static int heap_in_use(size_t *out)
{
    malloc_statistics_t stats;
    malloc_zone_statistics(NULL, &stats);
    *out = stats.size_in_use;
    return 1;
}
#else
static int heap_in_use(size_t *out)
{
    (void)out;
    return 0;
}
#endif

#define LIMIT (512u * 1024u)
#define WARMUP 64u
#define CYCLES 2000u

struct budget { unsigned calls; unsigned limit; };
static int interrupt(void *user, int gc)
{
    struct budget *budget = user;
    if (!gc) ++budget->calls;
    return budget->calls >= budget->limit;
}
static int host_add(lua_State *L)
{
    lua_pushinteger(L, luaL_checkinteger(L, 1) + luaL_checkinteger(L, 2));
    return 1;
}
static void initialize(lua_State *L, void *user)
{
    (void)user;
    lua_newtable(L);
    lua_pushcfunction(L, host_add, "host.add");
    lua_setfield(L, -2, "add");
    lua_setglobal(L, "host");
}

/* No stdio inside measured cycles: the first failing line is reported later. */
static int failed_line;
#define EXPECT(condition) do { if (!(condition)) { failed_line = __LINE__; return 1; } } while (0)

static int cycle(unsigned index, size_t baseline)
{
    struct budget budget = {0, UINT_MAX};
    p4_luau_config config = {LIMIT, interrupt, &budget, initialize, 4096};

    /* Creation that fails part-way through initialization at varying points. */
    p4_luau_config small = config;
    small.memory_limit = 1 + (index * 7919u) % (baseline - 1);
    EXPECT(p4_luau_create(&small) == NULL);

    p4_luau *vm = p4_luau_create(&config);
    EXPECT(vm != NULL);
    lua_State *L = p4_luau_state(vm);

    static const char typed[] = "local x: number = 1\nreturn x";
    EXPECT(p4_luau_check_source(vm, "@heap-check.luau", typed, sizeof typed - 1) == LUA_OK);
    EXPECT(p4_luau_check_source(vm, "@heap-check-bad.luau", "local = 1", 9) == LUA_ERRSYNTAX);
    EXPECT(p4_luau_load_source(vm, "@heap-bad.luau", "return (", 8) == LUA_ERRSYNTAX);
    EXPECT(p4_luau_load_source(vm, "@heap-limit.luau", "", config.source_limit + 1) == LUA_ERRSYNTAX);

    EXPECT(witness_run(vm, "@heap-normal.luau",
        "local t = {}\nfor i = 1, 200 do t[i] = string.format('%d:%s', i, ('x'):rep(i % 7)) end\n"
        "table.sort(t)\nreturn host.add(#t, #table.concat(t))", 1) == LUA_OK);
    EXPECT(lua_tointeger(L, -1) > 200);
    lua_pop(L, 1);

    EXPECT(witness_run(vm, "@heap-error.luau", "error({code = 1})", 0) == LUA_ERRRUN);
    EXPECT(witness_run(vm, "@heap-error2.luau", "local t = nil\nreturn t.field", 0) == LUA_ERRRUN);

    budget.calls = 0;
    budget.limit = 500;
    EXPECT(witness_run(vm, "@heap-budget.luau", "local co = coroutine.wrap(function() while true do end end)\n"
        "pcall(co)\nreturn 1", 1) == LUA_ERRRUN);
    p4_luau_reset_interrupt(vm);
    budget.calls = 0;
    budget.limit = UINT_MAX;

    EXPECT(witness_run(vm, "@heap-oom.luau", "local h\nwhile true do h = {h, buffer.create(1024)} end", 0) == LUA_ERRMEM);
    lua_gc(L, LUA_GCCOLLECT, 0);
    EXPECT(witness_run(vm, "@heap-after.luau", "return host.add(40, 2)", 1) == LUA_OK);
    EXPECT(lua_tointeger(L, -1) == 42);
    lua_pop(L, 1);
    EXPECT(lua_gettop(L) == 0);
    p4_luau_destroy(vm);
    return 0;
}

int main(void)
{
    size_t before = 0, after = 0;
    if (!heap_in_use(&before)) {
        puts("SKIP sandbox witness heap: no in-use heap statistic on this platform");
        return WITNESS_SKIP;
    }
    struct budget budget = {0, UINT_MAX};
    p4_luau_config config = {LIMIT, interrupt, &budget, initialize, 4096};
    p4_luau *probe = p4_luau_create(&config);
    CHECK(probe != NULL);
    size_t baseline = p4_luau_memory_used(probe);
    /* Collection is host-driven: expose only ordinary weak tables to scripts.
     * Strongly held keys and values survive, while unreachable ones clear. */
    lua_State *probe_state = p4_luau_state(probe);
    CHECK(witness_run(probe, "@weak-setup.luau",
        "keeper = { value = 42 }; weak = setmetatable({}, { __mode = 'v' }); "
        "weak.live = keeper; key = {}; weakkeys = setmetatable({}, { __mode = 'k' }); "
        "weakkeys[key] = 42; do local dead = { value = 99 }; weak.dead = dead; weakkeys[dead] = 99 end", 0) == LUA_OK);
    lua_gc(probe_state, LUA_GCCOLLECT, 0);
    CHECK(witness_run(probe, "@weak-check.luau",
        "assert(weak.live == keeper and keeper.value == 42); assert(weak.dead == nil); "
        "assert(weakkeys[key] == 42); local n = 0; for _ in weakkeys do n += 1 end; assert(n == 1)", 0) == LUA_OK);
    CHECK(witness_run(probe, "@gc-api.luau",
        "assert(collectgarbage == nil); assert(type(gcinfo()) == 'number' and gcinfo() >= 0)", 0) == LUA_OK);
    for (unsigned i = 0; i < 64; ++i) {
        CHECK(witness_run(probe, "@weak-stress-setup.luau",
            "local shared = { value = 42 }; keeper = { value = 42, data = buffer.create(8), proxy = newproxy(true) }; "
            "buffer.writeu8(keeper.data, 0, 42); keeper.closure = function() return shared.value end; "
            "keeper.thread = coroutine.create(function() coroutine.yield(shared.value); return shared.value + 1 end); "
            "weak.live = keeper; do local dead = { value = 99 }; weak.dead = dead end; "
            "for n = 1, 200 do local garbage = { n, ('x'):rep(n % 23) } end", 0) == LUA_OK);
        lua_gc(probe_state, LUA_GCCOLLECT, 0);
        CHECK(witness_run(probe, "@weak-stress-check.luau",
            "assert(weak.live == keeper and keeper.value == 42); assert(weak.dead == nil); "
            "assert(buffer.readu8(keeper.data, 0) == 42 and type(keeper.proxy) == 'userdata'); "
            "assert(keeper.closure() == 42); local ok, value = coroutine.resume(keeper.thread); assert(ok and value == 42)", 0) == LUA_OK);
        lua_gc(probe_state, LUA_GCCOLLECT, 0);
        CHECK(witness_run(probe, "@weak-stress-resume.luau",
            "local ok, value = coroutine.resume(keeper.thread); assert(ok and value == 43); "
            "assert(keeper.closure() == 42 and buffer.readu8(keeper.data, 0) == 42); keeper = nil", 0) == LUA_OK);
        lua_gc(probe_state, LUA_GCCOLLECT, 0);
        CHECK(witness_run(probe, "@weak-stress-drop.luau", "assert(weak.live == nil)", 0) == LUA_OK);
    }
    p4_luau_destroy(probe);
    CHECK(baseline > 1);

    /* Settle lazy libc/runtime state and allocator caches first. */
    for (unsigned i = 0; i < WARMUP; ++i)
        if (cycle(i, baseline)) {
            fprintf(stderr, "warm-up cycle %u failed at line %d\n", i, failed_line);
            return 1;
        }
    CHECK(heap_in_use(&before));
    for (unsigned i = 0; i < CYCLES; ++i)
        if (cycle(WARMUP + i, baseline)) {
            fprintf(stderr, "cycle %u failed at line %d\n", i, failed_line);
            return 1;
        }
    CHECK(heap_in_use(&after));
    /* Any leaked heap block is at least 16 bytes per cycle. */
    size_t tolerance = (size_t)CYCLES * 16u;
    if (after > before + tolerance) {
        fprintf(stderr, "host heap grew by %zu bytes over %u lifecycles (tolerance %zu)\n",
            after - before, CYCLES, tolerance);
        return 1;
    }
    printf("sandbox witness heap: %u complete p4_luau lifecycles left the host heap at %zu -> %zu bytes "
           "in use (tolerance %zu)\n", CYCLES, before, after, tolerance);
    return 0;
}
