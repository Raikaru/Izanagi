/* Consumer-visible GC interruption and allocator recovery; run against either VM. */
#include "lua.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct allocation { max_align_t alignment; size_t size; };
struct tracker { size_t bytes, blocks, limit; int bad; };
static struct tracker tracker;
static int interrupt_at, interrupts, destroyed, marker;

static void *allocate(void *ud, void *ptr, size_t old_size, size_t new_size)
{
    struct tracker *t = ud;
    struct allocation *a = ptr ? (struct allocation *)ptr - 1 : NULL;
    size_t old = a ? a->size : 0;
    (void)old_size; /* Fresh allocations may carry a type tag. */
    if (!new_size) {
        if (a) { t->bytes -= old; --t->blocks; free(a); }
        return NULL;
    }
    if (new_size > SIZE_MAX - sizeof(*a) ||
        (t->limit && (new_size > t->limit || t->bytes - old > t->limit - new_size)))
        return NULL;
    int fresh = a == NULL;
    struct allocation *next = realloc(a, sizeof(*a) + new_size);
    if (!next) return NULL;
    if (fresh) ++t->blocks;
    next->size = new_size;
    t->bytes = t->bytes - old + new_size;
    return next + 1;
}

static void destroy(lua_State *L, void *data)
{
    (void)L;
    if (*(int *)data != 1234) tracker.bad = 1;
    ++destroyed;
}

static void interrupt(lua_State *L, int gc)
{
    if (gc >= 0 && ++interrupts == interrupt_at) {
        lua_pushlightuserdata(L, &marker);
        lua_error(L);
    }
}

static int step(lua_State *L)
{
    lua_gc(L, LUA_GCSTEP, 1);
    return 0;
}

static int allocate_large(lua_State *L)
{
    lua_newbuffer(L, 1024 * 1024);
    return 0;
}

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "gc_interrupt_recovery:%d: %s\n", __LINE__, #x); \
    failed = 1; goto cleanup; } } while (0)

int main(void)
{
    int failed = 0;
    lua_State *L = lua_newstate(allocate, &tracker);
    CHECK(L != NULL);
    for (int phase = 1; phase <= 2; ++phase) {
        int before = destroyed;
        *(int *)lua_newuserdatadtor(L, sizeof(int), destroy) = 1234;
        lua_pop(L, 1);
        interrupts = 0;
        interrupt_at = phase;
        lua_callbacks(L)->interrupt = interrupt;
        CHECK(lua_cpcall(L, step, NULL) == LUA_ERRRUN);
        CHECK(interrupts == phase);
        CHECK(lua_touserdata(L, -1) == &marker);
        lua_pop(L, 1);
        lua_callbacks(L)->interrupt = NULL;
        lua_gc(L, LUA_GCCOLLECT, 0);
        CHECK(destroyed == before + 1);
        CHECK(!tracker.bad);
    }
    /* A rejected allocation must not poison collection or lose allocator state. */
    tracker.limit = tracker.bytes + 128 * 1024;
    CHECK(lua_cpcall(L, allocate_large, NULL) == LUA_ERRMEM);
    lua_pop(L, 1);
    tracker.limit = 0;
    *(int *)lua_newuserdatadtor(L, sizeof(int), destroy) = 1234;
    lua_pop(L, 1);
    lua_gc(L, LUA_GCCOLLECT, 0);
    CHECK(destroyed == 3 && !tracker.bad);
    /* Keep enough reachable work for a zero budget not to finish a cycle. */
    lua_gc(L, LUA_GCSTOP, 0);
    lua_createtable(L, 10000, 0);
    for (int i = 1; i <= 10000; ++i) {
        lua_createtable(L, 1, 0);
        lua_rawseti(L, -2, i);
    }
    lua_gc(L, LUA_GCCOLLECT, 0);
    int top = lua_gettop(L);
    *(int *)lua_newuserdatadtor(L, sizeof(int), destroy) = 1234;
    lua_pop(L, 1);
    CHECK(lua_gc(L, LUA_GCSTEP, 0) == 0);
    CHECK(destroyed == 3 && lua_gettop(L) == top);
    /* Signed data is converted to size_t before KB scaling in Luau 0.744. */
    CHECK(lua_gc(L, LUA_GCSTEP, -1) == 1);
    CHECK(destroyed == 4 && lua_gettop(L) == top);
    CHECK(lua_rawgeti(L, -1, 10000) == LUA_TTABLE);
    lua_pop(L, 1);
    *(int *)lua_newuserdatadtor(L, sizeof(int), destroy) = 1234;
    lua_pop(L, 1);
    CHECK(lua_gc(L, LUA_GCSTEP, -2) == 1);
    CHECK(destroyed == 5 && lua_gettop(L) == top && !tracker.bad);
    lua_pop(L, 1);
cleanup:
    if (L) { lua_callbacks(L)->interrupt = NULL; tracker.limit = 0; lua_close(L); }
    if (tracker.bytes || tracker.blocks || tracker.bad) {
        fprintf(stderr, "allocator imbalance: %zu bytes, %zu blocks, bad=%d\n",
                tracker.bytes, tracker.blocks, tracker.bad);
        failed = 1;
    }
    if (!failed) puts("gc interrupt and allocation recovery: PASS");
    return failed;
}
