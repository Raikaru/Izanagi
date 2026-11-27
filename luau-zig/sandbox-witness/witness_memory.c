/* Memory cap: exhaustion is a clean LUA_ERRMEM at the configured cap, never an
 * overrun; the VM recovers after collection and stays usable.
 *
 * Contract (include/p4pc/luau_c.h): memory_limit is a hard VM allocator cap,
 * including state and libraries; p4_luau_create returns NULL when the cap is
 * too small; load/run failures restore the stack; p4_luau_memory_used reports
 * the bridge's exact accounting. */
#include "witness.h"

#define LIMIT (1024u * 1024u)
#define PAGE_SLACK (64u * 1024u) /* largest VM page plus one buffer object */

/* Samples the bridge's accounting at every safepoint without touching the VM. */
struct watch { p4_luau *vm; size_t limit; size_t peak; unsigned over; };

static int interrupt(void *user, int gc)
{
    struct watch *watch = user;
    (void)gc;
    if (watch->vm) {
        size_t used = p4_luau_memory_used(watch->vm);
        if (used > watch->peak) watch->peak = used;
        if (used > watch->limit) ++watch->over;
    }
    return 0;
}

static p4_luau *create(struct watch *watch, size_t limit)
{
    p4_luau_config config = {limit, interrupt, watch, NULL, 0};
    watch->vm = NULL;
    watch->limit = limit;
    watch->peak = 0;
    watch->over = 0;
    p4_luau *vm = p4_luau_create(&config);
    watch->vm = vm;
    return vm;
}

static const char retain_forever[] =
    "local h\nwhile true do h = {h, buffer.create(1024)} end";

/* A normal workload after recovery: exact, allocation-heavy, deterministic. */
static int usable(p4_luau *vm)
{
    lua_State *L = p4_luau_state(vm);
    CHECK_VM(vm, witness_run(vm, "@mem-usable.luau",
        "local parts = {}\nfor i = 1, 500 do parts[i] = tostring(i) end\n"
        "return #table.concat(parts, ',')", 1) == LUA_OK);
    CHECK(lua_tointeger(L, -1) == 1891); /* 9*1 + 90*2 + 401*3 digits + 499 commas */
    lua_pop(L, 1);
    CHECK(lua_gettop(L) == 0);
    return 0;
}

static int exhaustion_and_recovery(p4_luau *vm, struct watch *watch, size_t *settled_out)
{
    lua_State *L = p4_luau_state(vm);
    CHECK_VM(vm, witness_run(vm, "@mem-warm.luau",
        "local h\nfor i = 1, 16 do h = {h, buffer.create(1024)} end\nreturn true", 1) == LUA_OK);
    lua_pop(L, 1);
    size_t settled = witness_collect(vm);
    *settled_out = settled;

    watch->peak = 0;
    CHECK(witness_run(vm, "@mem-exhaust.luau", retain_forever, 0) == LUA_ERRMEM);
    CHECK_VM(vm, witness_error_has(vm, "not enough memory"));
    CHECK(lua_gettop(L) == 0);
    CHECK(p4_luau_memory_used(vm) <= LIMIT);
    CHECK(watch->over == 0);
    CHECK(watch->peak <= LIMIT);
    /* Exhaustion came from the cap itself, not an earlier unrelated failure. */
    if (watch->peak + PAGE_SLACK < LIMIT) {
        fprintf(stderr, "exhaustion at %zu bytes, cap %zu\n", watch->peak, (size_t)LIMIT);
        return 1;
    }
    size_t recovered = witness_collect(vm);
    if (recovered > settled + PAGE_SLACK || recovered >= LIMIT / 4) {
        fprintf(stderr, "not reclaimed: settled %zu, after exhaustion+collect %zu\n", settled, recovered);
        return 1;
    }
    return usable(vm);
}

/* Script-side pcall and coroutine.resume observe the memory error as a normal
 * Luau error value; the chunk itself completes. */
static int caught_in_script(p4_luau *vm, size_t settled)
{
    static const char *const sources[] = {
        "local ok, err = pcall(function()\n"
        "  local h\n  while true do h = {h, buffer.create(1024)} end\n"
        "end)\nreturn ok, err",
        "local co = coroutine.create(function()\n"
        "  local h\n  while true do h = {h, buffer.create(1024)} end\n"
        "end)\nlocal ok, err = coroutine.resume(co)\nreturn ok, err",
    };
    lua_State *L = p4_luau_state(vm);
    for (size_t i = 0; i < sizeof sources / sizeof sources[0]; ++i) {
        CHECK_VM(vm, witness_run(vm, i ? "@mem-coroutine.luau" : "@mem-pcall.luau", sources[i], 2) == LUA_OK);
        CHECK(lua_gettop(L) == 2);
        CHECK(lua_type(L, 1) == LUA_TBOOLEAN && !lua_toboolean(L, 1));
        CHECK(lua_type(L, 2) == LUA_TSTRING && strcmp(lua_tostring(L, 2), "not enough memory") == 0);
        lua_settop(L, 0);
        CHECK(p4_luau_memory_used(vm) <= LIMIT);
        size_t recovered = witness_collect(vm);
        CHECK(recovered <= settled + PAGE_SLACK);
        if (usable(vm)) return 1;
    }
    return 0;
}

/* Single requests beyond the cap fail before any overrun, through the buffer,
 * string, table and concatenation allocators. */
static int oversized_requests(p4_luau *vm, size_t settled)
{
    static const char *const sources[] = {
        "return buffer.create(2 * 1024 * 1024)",
        "return string.rep('x', 2 * 1024 * 1024)",
        "return table.create(1000000)",
        "local s = 'x'\nfor i = 1, 30 do s = s .. s end\nreturn #s",
    };
    lua_State *L = p4_luau_state(vm);
    for (size_t i = 0; i < sizeof sources / sizeof sources[0]; ++i) {
        if (witness_run(vm, "@mem-oversized.luau", sources[i], 1) != LUA_ERRMEM) {
            fprintf(stderr, "oversized request %zu was not LUA_ERRMEM: %s\n", i, p4_luau_error(vm));
            return 1;
        }
        CHECK(lua_gettop(L) == 0);
        CHECK(p4_luau_memory_used(vm) <= LIMIT);
        CHECK(witness_collect(vm) <= settled + PAGE_SLACK);
        if (usable(vm)) return 1;
    }
    return 0;
}

/* Accounting follows retained objects up and back down. */
static int accounting_tracks_retention(p4_luau *vm)
{
    size_t before = witness_collect(vm);
    CHECK_VM(vm, witness_run(vm, "@mem-keep.luau", "keep = buffer.create(256 * 1024)", 0) == LUA_OK);
    size_t holding = witness_collect(vm);
    CHECK(holding >= before + 256u * 1024u);
    CHECK_VM(vm, witness_run(vm, "@mem-drop.luau", "keep = nil", 0) == LUA_OK);
    size_t released = witness_collect(vm);
    CHECK(released + 256u * 1024u <= holding);
    return 0;
}

/* Too-small caps fail creation; at the smallest workable cap, loading a chunk
 * that needs more VM heap fails as LUA_ERRMEM with the stack restored. */
static int tight_caps(size_t baseline)
{
    struct watch watch;
    for (size_t limit = 1; limit < baseline; limit += baseline / 97 + 1) {
        p4_luau *vm = create(&watch, limit);
        if (vm != NULL) {
            fprintf(stderr, "created with cap %zu below steady usage %zu\n", limit, baseline);
            p4_luau_destroy(vm);
            return 1;
        }
    }
    CHECK(create(&watch, baseline - 1) == NULL);

    p4_luau *tight = NULL;
    size_t limit = baseline;
    for (; limit <= baseline + PAGE_SLACK && tight == NULL; limit += 64) tight = create(&watch, limit);
    CHECK(tight != NULL);
    limit = watch.limit;
    lua_State *L = p4_luau_state(tight);
    CHECK(p4_luau_memory_used(tight) <= limit);

    /* 4000 distinct string constants need far more than the remaining headroom. */
    size_t capacity = 16 + 4000 * 10;
    char *source = malloc(capacity);
    CHECK(source != NULL);
    size_t n = (size_t)snprintf(source, capacity, "return {");
    for (unsigned i = 0; i < 4000; ++i) n += (size_t)snprintf(source + n, capacity - n, "'k%05u',", i);
    n += (size_t)snprintf(source + n, capacity - n, "}");
    lua_pushboolean(L, 1);
    int status = p4_luau_load_source(tight, "@mem-tight.luau", source, n);
    free(source);
    CHECK_VM(tight, status == LUA_ERRMEM);
    CHECK_VM(tight, witness_error_has(tight, "not enough memory"));
    CHECK(lua_gettop(L) == 1 && lua_toboolean(L, 1));
    lua_settop(L, 0);
    CHECK(p4_luau_memory_used(tight) <= limit);
    /* Whatever still fits must either work or fail as a memory error. */
    status = witness_run(tight, "@mem-tight-small.luau", "return 1", 1);
    CHECK_VM(tight, status == LUA_OK || status == LUA_ERRMEM);
    lua_settop(L, 0);
    CHECK(p4_luau_memory_used(tight) <= limit);
    p4_luau_destroy(tight);
    return 0;
}

/* Caps and accounting are per VM. */
static int independent_caps(p4_luau *full)
{
    struct watch watch;
    CHECK(witness_run(full, "@mem-fill.luau", "fill = {}\nlocal h\nwhile true do h = {h, buffer.create(1024)}; fill[1] = h end", 0) == LUA_ERRMEM);
    size_t full_used = p4_luau_memory_used(full);
    p4_luau *other = create(&watch, LIMIT);
    CHECK(other != NULL);
    CHECK_VM(other, witness_run(other, "@mem-other.luau",
        "local b = buffer.create(512 * 1024)\nreturn buffer.len(b)", 1) == LUA_OK);
    CHECK(lua_tointeger(p4_luau_state(other), -1) == 512 * 1024);
    p4_luau_destroy(other);
    CHECK(p4_luau_memory_used(full) == full_used);
    /* At the cap even compiling `fill = nil` may not fit; clear the existing
     * thread-environment key from the host instead (no allocation). */
    lua_pushnil(p4_luau_state(full));
    lua_setglobal(p4_luau_state(full), "fill");
    CHECK(witness_collect(full) < LIMIT / 4);
    return usable(full);
}

int main(void)
{
    struct watch watch;
    p4_luau *vm = create(&watch, LIMIT);
    CHECK(vm != NULL);
    size_t baseline = p4_luau_memory_used(vm);
    CHECK(baseline > 0 && baseline < LIMIT / 4);
    size_t settled = 0;
    SECTION(exhaustion_and_recovery(vm, &watch, &settled));
    SECTION(caught_in_script(vm, settled));
    SECTION(oversized_requests(vm, settled));
    SECTION(accounting_tracks_retention(vm));
    SECTION(independent_caps(vm));
    CHECK(watch.over == 0);
    p4_luau_destroy(vm);
    SECTION(tight_caps(baseline));
    CHECK(p4_luau_create(NULL) == NULL);
    puts("sandbox witness memory: cap-bounded exhaustion, script-side pcall/coroutine memory errors, "
         "oversized requests, collection recovery, tight-cap creation/load and per-VM caps passed");
    return 0;
}
