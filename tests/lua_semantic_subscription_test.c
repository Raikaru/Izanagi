/* Real Luau allocator and semantic queue; synthetic observer and catalog only. */
#include "lua_runtime.h"
#include "native_bindings.h"
#include "p4pc/battle_notify.h"
#include "p4pc/battle_semantic.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/mod_entry.h"
#include "p4pc/guest.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) && !defined(_WIN64)
#include <malloc.h>
#endif
/* Mark the actual VM operations, not artificial error sites. The allocator below
 * fails only if the marked operation really requests a new page or table. */
static int phase, ref_number, first_ref;
static void *marked_userdata(lua_State *L, size_t size)
{
    phase = 1;
    void *result = lua_newuserdata(L, size);
    phase = 0;
    return result;
}
static int marked_ref(lua_State *L, int index)
{
    phase = ++ref_number + 1;
    int result = lua_ref(L, index);
    if (ref_number == 1) first_ref = result;
    phase = 0;
    return result;
}
#undef lua_newuserdata
#define lua_newuserdata(L, size) marked_userdata(L, size)
#define lua_ref(L, index) marked_ref(L, index)
#include "../src/platform/mods/lua_semantic_events.c"
#undef lua_newuserdata
#undef lua_ref

static P4LuaMod mod;
static int snapshot, observer, refuse_observer, calls, (*semantic_pump)(void);
static int next_id, order[8];
uint32_t p4_mod_safe_point;
#ifdef P4_GUEST64
uint8_t *p4_guest_base;
void *p4_g2h_slow(uint32_t address) { (void)address; abort(); }
#endif
uint32_t datPersonaGetExpById(int16_t id) { (void)id; return 0; }
uint8_t datLevelFromExp(int32_t exp) { (void)exp; return 0; }
uint32_t p4_battle_observer_add(const P4BattleObserver *p) { assert(p); if (refuse_observer) return 0; ++observer; return 1; }
int p4_battle_observer_remove(uint32_t token) { assert(token == 1 && observer == 1); --observer; return 0; }
void p4_battle_notify_sink(const P4BattleNotifySink *sink, uint32_t mask) { (void)sink; (void)mask; }
int p4_data_ref(P4DataKind kind, uint32_t id, P4DataRef *out) { (void)kind; (void)id; (void)out; return -1; }
const char *p4_data_character_name(uint32_t id) { (void)id; return NULL; }
const char *p4_data_ailment_name(uint32_t id) { (void)id; return NULL; }
void *p4_luau_user(lua_State *L) { (void)L; return &mod; }
void p4_lua_require_mutation(lua_State *L) { if (snapshot) luaL_error(L, "snapshot"); }
const char *p4_lua_error(const P4LuaMod *m) { (void)m; return "callback failed"; }
_Noreturn void p4_mod_failure(size_t owner, const char *reason) { (void)owner; (void)reason; abort(); }
void p4_mod_events_set_semantic(int (*pump_fn)(void)) { semantic_pump = pump_fn; }
int p4_lua_invoke(P4LuaMod *m, lua_CFunction fn, void *data, int results, int flags)
{
    (void)flags;
    lua_pushcfunction(m->state, fn, "test.invoke");
    lua_pushlightuserdata(m->state, data);
    return lua_pcall(m->state, 1, results, 0);
}

typedef struct Allocator { int target, failed; } Allocator;
static void *allocate(void *context, void *ptr, size_t oldsize, size_t size)
{
    Allocator *a = context;
    if (!size) {
#if defined(_WIN32) && !defined(_WIN64)
        _aligned_free(ptr);
#else
        free(ptr);
#endif
        return NULL;
    }
    if (a->target && phase == a->target && (!ptr || size > oldsize)) {
        a->failed = 1;
        return NULL;
    }
#if defined(_WIN32) && !defined(_WIN64)
    return _aligned_realloc(ptr, size, 16);
#else
    return realloc(ptr, size);
#endif
}
static int callback(lua_State *L)
{
    assert(calls < (int)(sizeof order / sizeof *order));
    order[calls++] = lua_tointeger(L, lua_upvalueindex(1));
    return 0;
}
static int fallback(lua_State *L) { luaL_error(L, "not semantic"); }
static void push_on(lua_State *L)
{
    lua_getglobal(L, "p4"); lua_getfield(L, -1, "events"); lua_getfield(L, -1, "on");
    lua_remove(L, -2); lua_remove(L, -2);
}
static int attempt(lua_State *L)
{
    push_on(L);
    lua_pushliteral(L, "turn_start");
    lua_pushinteger(L, next_id);
    lua_pushcclosure(L, callback, "test.callback", 1);
    ref_number = 0; first_ref = LUA_NOREF; phase = 0;
    return lua_pcall(L, 2, 1, 0);
}
static void method(lua_State *L, const char *name)
{
    lua_getfield(L, -1, name); lua_pushvalue(L, -2);
    assert(lua_pcall(L, 1, 0, 0) == LUA_OK);
}
static void emit(void)
{
    const P4BattleAdmission event = {0};
    p4_semantic_admission(&event);
}
static void assert_empty(void)
{
    P4SemanticEvent event;
    uint32_t lost = 0;
    emit();
    assert(!p4_semantic_drain(&event, 1, &lost) && !lost && !observer);
}

/* Vary live userdata and occupied registry slots so that VM page exhaustion
 * and registry resizing happen inside each of the three marked operations. */
static int trial(int target, int fill)
{
    Allocator allocator = {0};
    lua_State *L = lua_newstate(allocate, &allocator);
    assert(L);
    memset(&mod, 0, sizeof mod); mod.state = L; mod.owner = 4;
    lua_gc(L, LUA_GCSTOP, 0);
    lua_newtable(L); /* p4 */
    lua_newtable(L); lua_pushcfunction(L, fallback, "test.fallback"); lua_setfield(L, -2, "on");
    lua_setfield(L, -2, "events");
    p4_lua_semantic_events_initialize(L);
    lua_setglobal(L, "p4");
    /* Root filler userdata in one table; separately occupy registry slots. */
    lua_createtable(L, fill, 0);
    for (int i = 1; i <= fill; ++i) {
        (void)lua_newuserdatatagged(L, sizeof(void *), 0);
        lua_rawseti(L, -2, i);
    }
    lua_pushcfunction(L, callback, "test.filler");
    for (int i = 0; i < fill; ++i) (void)lua_ref(L, -1);
    lua_pop(L, 1);
    allocator.target = target;
    const int status = attempt(L);
    allocator.target = 0; phase = 0;
    if (allocator.failed) {
        assert(status != LUA_OK && mod.subscriptions == 0);
        if (first_ref > LUA_REFNIL) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, first_ref);
            assert(lua_type(L, -1) == LUA_TNUMBER); /* ref was returned to free list */
            lua_pop(L, 1);
        }
        lua_pop(L, 1); /* original VM error */
        assert_empty(); assert(!subscriptions);
        next_id = 1;
        assert(attempt(L) == LUA_OK && mod.subscriptions == 1 && observer == 1);
        next_id = 2;
        assert(attempt(L) == LUA_OK && mod.subscriptions == 2 && observer == 1);
        calls = 0;
        emit(); emit(); assert(semantic_pump() == 0 && calls == 4);
        assert(order[0] == 1 && order[1] == 2 && order[2] == 1 && order[3] == 2);
        method(L, "remove"); lua_pop(L, 1);
        method(L, "disable"); assert_empty();
        method(L, "enable"); assert(observer == 1);
        method(L, "remove"); assert_empty(); assert(!subscriptions);
        method(L, "remove");
        lua_pop(L, 1);
        /* Teardown of a live listener also releases interest. */
        assert(attempt(L) == LUA_OK && observer == 1);
        p4_lua_semantic_events_close(&mod); assert_empty(); assert(!subscriptions);
        lua_pop(L, 1);
        snapshot = 1; assert(attempt(L) != LUA_OK); snapshot = 0;
        lua_pop(L, 1); assert_empty(); assert(!subscriptions);
        refuse_observer = 1; assert(attempt(L) != LUA_OK); refuse_observer = 0;
        lua_pop(L, 1); assert_empty(); assert(!subscriptions);
    } else {
        assert(status == LUA_OK);
        method(L, "remove"); lua_pop(L, 1); assert_empty(); assert(!subscriptions);
    }
    lua_close(L);
    return allocator.failed;
}
int main(void)
{
    for (int target = 1; target <= 3; ++target) {
        int found = 0;
        for (int fill = 0; fill < 2048 && !found; ++fill) found = trial(target, fill);
        if (!found) { fprintf(stderr, "no VM allocation boundary for phase %d\n", target); return 1; }
    }
    puts("lua_semantic_subscription: VM allocation failures and transactional cleanup passed");
    return 0;
}
