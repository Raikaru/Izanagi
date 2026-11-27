/* Allocator cleanup: the VM behind p4_luau must report exact block sizes to
 * its lua_Alloc and release every block on lua_close and on every out-of-memory
 * path, including a failing lua_newstate. The p4_luau bridge's cap accounting
 * (retained = used - oldsize) is only sound under that contract, and its own
 * allocator is private, so this witness drives the same VM through the public
 * lua.h allocator hook with the production library set and sandbox calls.
 *
 * Every allocation is recorded by address. A free or resize must name a live
 * block with its recorded size; after lua_close nothing may remain live and
 * every userdata destructor must have run exactly once. Then the same workload
 * is repeated with a single injected allocation failure at each request index. */
#include "witness.h"
#if defined(_WIN32) && !defined(_WIN64)
#include <malloc.h>
#endif

#define TRACK_CAPACITY (1u << 17)

struct block { void *ptr; size_t size; };
struct tracker {
    struct block *blocks;
    size_t count;
    size_t live_bytes;
    unsigned long requests;  /* nonzero-size allocation/resize requests */
    unsigned long fail_at;   /* 1-based request index to refuse; 0 = never */
    unsigned mismatched_size;
    unsigned unknown_block;
    unsigned misaligned;
    unsigned overflow;
    unsigned refused;        /* injected refusals actually delivered */
    int refused_phase;       /* phase in which the refusal happened */
};

/* Where the VM is in run_once, so each refusal can be attributed. */
enum phase { PHASE_NEWSTATE, PHASE_LIBRARIES, PHASE_WORKLOAD, PHASE_CLOSE, PHASE_COUNT };
static int current_phase;

static size_t slot_of(const void *ptr)
{
    uint64_t h = (uint64_t)(uintptr_t)ptr;
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdull;
    h ^= h >> 33;
    return (size_t)h & (TRACK_CAPACITY - 1);
}
static size_t find(const struct tracker *t, const void *ptr)
{
    size_t i = slot_of(ptr);
    while (t->blocks[i].ptr != NULL && t->blocks[i].ptr != ptr) i = (i + 1) & (TRACK_CAPACITY - 1);
    return i;
}
static void insert(struct tracker *t, void *ptr, size_t size)
{
    if (t->count >= TRACK_CAPACITY / 2) { ++t->overflow; return; }
    size_t i = find(t, ptr);
    if (t->blocks[i].ptr == NULL) ++t->count;
    t->blocks[i].ptr = ptr;
    t->blocks[i].size = size;
}
/* Linear-probing deletion with backward shift; no tombstones. */
static void erase(struct tracker *t, size_t i)
{
    size_t j = i;
    for (;;) {
        j = (j + 1) & (TRACK_CAPACITY - 1);
        if (t->blocks[j].ptr == NULL) break;
        size_t k = slot_of(t->blocks[j].ptr);
        int movable = j > i ? (k <= i || k > j) : (k <= i && k > j);
        if (movable) {
            t->blocks[i] = t->blocks[j];
            i = j;
        }
    }
    t->blocks[i].ptr = NULL;
    t->blocks[i].size = 0;
    --t->count;
}

static void raw_free(void *ptr)
{
#if defined(_WIN32) && !defined(_WIN64)
    _aligned_free(ptr);
#else
    free(ptr);
#endif
}
static void *raw_realloc(void *ptr, size_t size)
{
#if defined(_WIN32) && !defined(_WIN64)
    return _aligned_realloc(ptr, size, 16);
#else
    return realloc(ptr, size);
#endif
}

static void *tracked_alloc(void *ud, void *ptr, size_t osize, size_t nsize)
{
    struct tracker *t = ud;
    size_t recorded = 0;
    size_t slot = 0;
    if (ptr != NULL) {
        slot = find(t, ptr);
        if (t->blocks[slot].ptr != ptr) ++t->unknown_block;
        else {
            recorded = t->blocks[slot].size;
            if (recorded != osize) ++t->mismatched_size;
        }
    }
    if (nsize == 0) {
        if (ptr != NULL) {
            if (t->blocks[slot].ptr == ptr) {
                erase(t, slot);
                t->live_bytes -= recorded;
            }
            raw_free(ptr);
        }
        return NULL;
    }
    if (++t->requests == t->fail_at) {
        ++t->refused;
        t->refused_phase = current_phase;
        return NULL;
    }
    void *result = raw_realloc(ptr, nsize);
    if (result == NULL) return NULL;
    if (ptr != NULL && t->blocks[slot].ptr == ptr) {
        erase(t, slot);
        t->live_bytes -= recorded;
    }
    insert(t, result, nsize);
    t->live_bytes += nsize;
    if (sizeof(void *) == 8 && ((uintptr_t)result & 15u) != 0) ++t->misaligned;
    return result;
}

struct work {
    int strict;              /* no injected failure: every step must succeed */
    unsigned dtor_created;
    unsigned dtor_called;
    unsigned steps_failed;   /* inner protected steps that reported an error */
};
static struct work *current_work;

static void destructor(lua_State *L, void *data)
{
    (void)L;
    if (*(uint32_t *)data == 0x5eed5eedu) ++current_work->dtor_called;
}

/* Production library set and sandbox, exactly as luau_c.cpp's initialize. */
static int open_sandbox(lua_State *L)
{
    static const luaL_Reg libraries[] = {
        {"", luaopen_base}, {LUA_COLIBNAME, luaopen_coroutine},
        {LUA_TABLIBNAME, luaopen_table}, {LUA_STRLIBNAME, luaopen_string},
        {LUA_MATHLIBNAME, luaopen_math}, {LUA_UTF8LIBNAME, luaopen_utf8},
        {LUA_BITLIBNAME, luaopen_bit32}, {LUA_BUFFERLIBNAME, luaopen_buffer},
    };
    for (size_t i = 0; i < sizeof libraries / sizeof libraries[0]; ++i) {
        lua_pushcfunction(L, libraries[i].func, NULL);
        lua_pushstring(L, libraries[i].name);
        lua_call(L, 1, 0);
    }
    luaL_sandbox(L);
    luaL_sandboxthread(L);
    return 0;
}

static int raise_after_allocating(lua_State *L)
{
    lua_createtable(L, 64, 64);
    lua_pushstring(L, "allocated before the error");
    lua_error(L);
    return 0;
}
static int yielder(lua_State *L)
{
    lua_pushinteger(L, 7);
    return lua_yield(L, 1);
}
static int upvalue_sum(lua_State *L)
{
    lua_pushinteger(L, lua_tointeger(L, lua_upvalueindex(1)) + lua_tointeger(L, lua_upvalueindex(2)));
    return 1;
}

static int workload(lua_State *L)
{
    struct work *work = current_work;
    char text[64];
    lua_settop(L, 0);
    lua_createtable(L, 0, 0); /* 1: anchor */

    for (int i = 0; i < 400; ++i) {
        lua_createtable(L, 2, 2);
        snprintf(text, sizeof text, "entry-%d", i);
        lua_pushstring(L, text);
        lua_setfield(L, -2, "name");
        lua_pushinteger(L, i);
        lua_rawseti(L, -2, 1);
        if (i % 2) lua_rawseti(L, 1, i / 2 + 1);
        else lua_pop(L, 1);
    }
    for (int i = 0; i < 200; ++i) {
        uint32_t *data = lua_newuserdatadtor(L, 48, destructor);
        *data = 0x5eed5eedu;
        ++work->dtor_created;
        if (i % 3 == 0) lua_rawseti(L, 1, 1000 + i);
        else lua_pop(L, 1);
    }
    for (int i = 0; i < 24; ++i) {
        (void)lua_newbuffer(L, 4096);
        if (i % 2) lua_rawseti(L, 1, 2000 + i);
        else lua_pop(L, 1);
    }
    for (int i = 0; i < 40; ++i) {
        lua_pushinteger(L, i);
        lua_pushinteger(L, 2 * i);
        lua_pushcclosurek(L, upvalue_sum, "upvalue_sum", 2, NULL);
        lua_call(L, 0, 1);
        if (work->strict && lua_tointeger(L, -1) != 3 * i) ++work->steps_failed;
        lua_pop(L, 1);
    }
    int refs[32];
    for (int i = 0; i < 32; ++i) {
        lua_createtable(L, 0, 1);
        refs[i] = lua_ref(L, -1);
        lua_pop(L, 1);
    }
    for (int i = 0; i < 32; i += 2) lua_unref(L, refs[i]);

    /* Library code reached through the sandboxed globals. */
    lua_getglobal(L, "string");
    lua_getfield(L, -1, "rep");
    lua_pushstring(L, "ab");
    lua_pushinteger(L, 2000);
    lua_call(L, 2, 1);
    if (work->strict && lua_objlen(L, -1) != 4000) ++work->steps_failed;
    lua_pop(L, 2);
    lua_getglobal(L, "table");
    lua_getfield(L, -1, "sort");
    lua_createtable(L, 500, 0);
    for (int i = 0; i < 500; ++i) {
        lua_pushinteger(L, 500 - i);
        lua_rawseti(L, -2, i + 1);
    }
    lua_pushvalue(L, -1);
    lua_insert(L, -3);
    lua_call(L, 1, 0);
    lua_rawgeti(L, -1, 1);
    if (work->strict && lua_tointeger(L, -1) != 1) ++work->steps_failed;
    lua_pop(L, 3);

    /* Error unwinding after allocation inside a protected call. */
    lua_pushcfunction(L, raise_after_allocating, "raise_after_allocating");
    int status = lua_pcall(L, 0, 0, 0);
    if (status != LUA_ERRRUN && !(status == LUA_ERRMEM && !work->strict)) ++work->steps_failed;
    lua_pop(L, 1);

    /* Coroutines: one finished, one left suspended and unreferenced, one pinned. */
    for (int i = 0; i < 3; ++i) {
        lua_State *T = lua_newthread(L);
        lua_pushcfunction(T, yielder, "yielder");
        status = lua_resume(T, L, 0);
        if (status != LUA_YIELD && !(status == LUA_ERRMEM && !work->strict)) ++work->steps_failed;
        if (i == 0 && status == LUA_YIELD) {
            lua_settop(T, 0);
            status = lua_resume(T, L, 0);
            if (status != LUA_OK && !(status == LUA_ERRMEM && !work->strict)) ++work->steps_failed;
        }
        if (i == 2) lua_rawseti(L, 1, 3000);
        else lua_pop(L, 1);
    }

    lua_gc(L, LUA_GCCOLLECT, 0);
    lua_gc(L, LUA_GCSTEP, 64);
    lua_setglobal(L, "anchor"); /* retained by the thread environment until close */
    return 0;
}

static int run_once(struct tracker *tracker, unsigned long fail_at, struct work *work, int *opened)
{
    /* A verified previous run left no live entry, so the table is all empty. */
    struct block *blocks = tracker->blocks;
    memset(tracker, 0, sizeof *tracker);
    tracker->blocks = blocks;
    tracker->fail_at = fail_at;
    memset(work, 0, sizeof *work);
    work->strict = fail_at == 0;
    current_work = work;
    *opened = 0;

    current_phase = PHASE_NEWSTATE;
    lua_State *L = lua_newstate(tracked_alloc, tracker);
    if (L == NULL) return LUA_ERRMEM;
    *opened = 1;
    current_phase = PHASE_LIBRARIES;
    int status = lua_cpcall(L, open_sandbox, NULL);
    current_phase = PHASE_WORKLOAD;
    if (status == LUA_OK) status = lua_cpcall(L, workload, NULL);
    current_phase = PHASE_CLOSE;
    lua_close(L);
    return status;
}

static int verify_closed(const struct tracker *tracker, const struct work *work, unsigned long fail_at, int status)
{
    if (tracker->live_bytes != 0 || tracker->count != 0 || tracker->mismatched_size != 0 ||
        tracker->unknown_block != 0 || tracker->misaligned != 0 || tracker->overflow != 0 ||
        work->dtor_called != work->dtor_created || work->steps_failed != 0 ||
        !(status == LUA_OK || (fail_at != 0 && status == LUA_ERRMEM))) {
        fprintf(stderr,
            "fail_at %lu: status %d, live %zu bytes in %zu blocks, size mismatches %u, unknown blocks %u, "
            "misaligned %u, tracker overflow %u, destructors %u/%u, failed steps %u\n",
            fail_at, status, tracker->live_bytes, tracker->count, tracker->mismatched_size,
            tracker->unknown_block, tracker->misaligned, tracker->overflow, work->dtor_called,
            work->dtor_created, work->steps_failed);
        return 1;
    }
    return 0;
}

int main(void)
{
    struct tracker tracker = {0};
    struct work work;
    int opened = 0;
    tracker.blocks = calloc(TRACK_CAPACITY, sizeof(struct block));
    CHECK(tracker.blocks != NULL);

    int status = run_once(&tracker, 0, &work, &opened);
    CHECK(opened);
    SECTION(verify_closed(&tracker, &work, 0, status));
    CHECK(work.dtor_created == 200);
    unsigned long total = tracker.requests;
    CHECK(tracker.refused == 0);

    /* Refuse each request index in turn (a bounded stride only if the workload
     * ever grows past 20000 requests; the final request is always tried). The
     * VM is deterministic up to the refused request, so every index must be
     * reached and refused exactly once, and cleanup must hold afterwards. */
    static const char *const phase_names[PHASE_COUNT] = {"lua_newstate", "libraries+sandbox",
        "workload", "lua_close"};
    unsigned long tried = 0, refused_in[PHASE_COUNT] = {0}, failed_runs = 0, absorbed_runs = 0;
    unsigned long stride = total <= 20000 ? 1 : total / 20000 + 1;
    for (unsigned long k = 1; k <= total; k = k == total ? total + 1 : (k + stride > total ? total : k + stride)) {
        status = run_once(&tracker, k, &work, &opened);
        if (tracker.refused != 1) {
            fprintf(stderr, "fail_at %lu of %lu: request index was not reached (refused %u)\n", k, total,
                tracker.refused);
            return 1;
        }
        ++refused_in[tracker.refused_phase];
        if (!opened) CHECK(tracker.refused_phase == PHASE_NEWSTATE);
        SECTION(verify_closed(&tracker, &work, k, status));
        /* OK: caught by an inner lua_pcall/lua_resume or a protected VM resize. */
        if (status == LUA_OK) ++absorbed_runs;
        else ++failed_runs;
        ++tried;
    }
    /* Injected failures must land in state creation, in library opening plus
     * the sandbox calls, and in the workload; lua_close must never allocate. */
    for (int phase = 0; phase < PHASE_COUNT; ++phase) {
        int expected = phase != PHASE_CLOSE;
        if ((refused_in[phase] != 0) != expected) {
            fprintf(stderr, "%lu refusals during %s\n", refused_in[phase], phase_names[phase]);
            return 1;
        }
    }
    CHECK(failed_runs > 0);
    free(tracker.blocks);
    printf("sandbox witness allocator: exact sizes, alignment, full release and destructor completion "
           "after %lu of %lu request indices refused (%lu newstate, %lu libraries, %lu workload; "
           "%lu surfaced as LUA_ERRMEM, %lu absorbed) passed\n",
           tried, total, refused_in[PHASE_NEWSTATE], refused_in[PHASE_LIBRARIES],
           refused_in[PHASE_WORKLOAD], failed_runs, absorbed_runs);
    return 0;
}
