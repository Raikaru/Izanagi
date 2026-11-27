/* battle.execution@1: synchronous, immutable observations of the native C
 * reducer. Native calls/yields are forbidden while a listener runs; guest
 * reads/writes and Lua policy state remain available before initializer effects.
 * Subscription admission is outside battle, never a fabricated late admission. */
#include "native_bindings.h"
#include "p4pc/battle_action_exec_native.h"
#include "p4pc/mod_failure.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define EXEC_META "p4.battle.execution.subscription"
typedef struct LuaExecution {
    struct LuaExecution *next;
    P4LuaMod *mod;
    int callback, self, enabled, linked;
} LuaExecution;
typedef struct ExecutionCall {
    LuaExecution *handler;
    const P4ActionExec *execution;
    unsigned event;
} ExecutionCall;
static LuaExecution *listeners;
static unsigned delivering, installed;

static void number(lua_State *L, const char *key, uint32_t value)
{
    lua_pushunsigned(L, value); lua_setfield(L, -2, key);
}
static void boolean(lua_State *L, const char *key, int value)
{
    lua_pushboolean(L, value); lua_setfield(L, -2, key);
}
static void token(lua_State *L, const char *key, uint64_t value)
{
    char text[17];
    snprintf(text, sizeof text, "%016" PRIx64, value);
    lua_pushstring(L, text); lua_setfield(L, -2, key);
}
static void unit_view(lua_State *L, const P4ActionExecUnit *unit)
{
    lua_createtable(L, 0, 5);
    number(L, "action", unit->action); number(L, "data", unit->data);
    number(L, "uid", unit->uid); number(L, "id", unit->id);
    lua_pushstring(L, unit->side ? "enemy" : "party"); lua_setfield(L, -2, "side");
    lua_setreadonly(L, -1, 1);
}
static void execution_view(lua_State *L, const P4ActionExec *e)
{
    static const char *const outcomes[] = {"running", "resolved", "hitless", "direct_end", "removed",
        "flushed", "battle_release", "replaced", "unproven"};
    lua_createtable(L, 0, 15);
    token(L, "token", e->token); token(L, "action_uid", e->action_uid);
    const P4ActionExecUnit actor = {e->action, e->unit_data, e->unit_uid, e->native_id, e->side};
    unit_view(L, &actor); lua_setfield(L, -2, "actor");
    number(L, "command", e->command); number(L, "skill", e->skill);
    number(L, "pending_packets", e->pending_packets); number(L, "result_packets", e->result_packets);
    number(L, "uncertainty", e->uncertainty);
    boolean(L, "base_turn", e->base_turn); boolean(L, "committed", e->committed);
    boolean(L, "direct_end", e->direct_end); boolean(L, "end_seen", e->end_seen);
    boolean(L, "sealed", e->sealed); boolean(L, "base_boundary", e->base_boundary);
    lua_pushstring(L, e->outcome < sizeof outcomes / sizeof *outcomes ? outcomes[e->outcome] : "unproven");
    lua_setfield(L, -2, "outcome");
    lua_setreadonly(L, -1, 1);
}
static int invoke_execution(lua_State *L)
{
    static const char *const events[] = {"", "admitted", "committed", "base_boundary", "terminal",
        "uncertain", "battle_start", "battle_end", "lost"};
    const ExecutionCall *call = lua_touserdata(L, 1);
    lua_getref(L, call->handler->callback);
    lua_createtable(L, 0, 5);
    lua_pushstring(L, events[call->event]); lua_setfield(L, -2, "kind");
    token(L, "epoch", p4_action_exec_battle_epoch);
    boolean(L, "battle_active", p4_action_exec_battle_active != 0);
    number(L, "reason", call->execution->uncertainty);
    if (call->execution->token) { execution_view(L, call->execution); lua_setfield(L, -2, "execution"); }
    lua_setreadonly(L, -1, 1);
    lua_call(L, 1, 0);
    return 0;
}
static void observe(unsigned event, const P4ActionExec *execution, void *context)
{
    (void)context;
    ++delivering;
    for (LuaExecution *handler = listeners; handler; handler = handler->next) {
        if (!handler->enabled) continue;
        ExecutionCall call = {handler, execution, event};
        if (p4_lua_invoke(handler->mod, invoke_execution, &call, 0, P4_LUA_SYNCHRONOUS))
            p4_mod_failure(handler->mod->owner, p4_lua_error(handler->mod));
    }
    --delivering;
}
static LuaExecution *check_handler(lua_State *L)
{
    LuaExecution *handler = luaL_checkudata(L, 1, EXEC_META);
    if (handler->mod != p4_luau_user(L)) luaL_error(L, "foreign battle execution subscription");
    return handler;
}
static void mutable(lua_State *L)
{
    p4_lua_require_mutation(L);
    if (delivering) luaL_error(L, "battle execution subscriptions cannot change during delivery");
}
static int enable_handler(lua_State *L)
{
    mutable(L);
    LuaExecution *handler = check_handler(L);
    const int on = lua_toboolean(L, lua_upvalueindex(1));
    if (!handler->linked) luaL_error(L, "removed battle execution subscription");
    if (on && !handler->enabled && p4_action_exec_battle_active)
        luaL_error(L, "battle execution subscription enables only outside battle (missed admissions)");
    handler->enabled = on;
    return 0;
}
static void unlink_handler(LuaExecution *handler)
{
    LuaExecution **link = &listeners;
    while (*link && *link != handler) link = &(*link)->next;
    if (*link) *link = handler->next;
    handler->linked = handler->enabled = 0;
}
static int remove_handler(lua_State *L)
{
    mutable(L);
    LuaExecution *handler = check_handler(L);
    if (!handler->linked) return 0;
    unlink_handler(handler);
    lua_unref(L, handler->callback); lua_unref(L, handler->self);
    handler->callback = handler->self = LUA_NOREF;
    return 0;
}
static int add_handler(lua_State *L)
{
    mutable(L);
    luaL_checktype(L, 1, LUA_TFUNCTION);
    if (p4_action_exec_battle_active)
        luaL_error(L, "battle execution subscription registers only outside battle (no invented admission)");
    P4LuaMod *mod = p4_luau_user(L);
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaExecution *handler = lua_newuserdata(L, sizeof *handler);
    *handler = (LuaExecution){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF};
    luaL_getmetatable(L, EXEC_META); lua_setmetatable(L, -2);
    lua_pushvalue(L, 1); handler->callback = lua_ref(L, -1); lua_pop(L, 1);
    handler->self = lua_ref(L, -1);
    if (!installed && !p4_action_exec_observe(observe, NULL)) {
        lua_unref(L, handler->callback); lua_unref(L, handler->self);
        handler->callback = handler->self = LUA_NOREF;
        luaL_error(L, "battle execution observer is busy");
    }
    installed = 1;
    LuaExecution **link = &listeners;
    while (*link) link = &(*link)->next;
    *link = handler;
    handler->enabled = handler->linked = 1;
    ++mod->subscriptions;
    return 1;
}
static uint64_t check_token(lua_State *L, int index)
{
    size_t size;
    luaL_checktype(L, index, LUA_TSTRING);
    const char *text = luaL_checklstring(L, index, &size);
    if (size != 16) luaL_argerror(L, index, "execution token must be 16 lowercase hexadecimal characters");
    uint64_t result = 0;
    for (size_t i = 0; i < size; ++i) {
        const char c = text[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            luaL_argerror(L, index, "execution token must be 16 lowercase hexadecimal characters");
        result = (result << 4) | (uint64_t)(c <= '9' ? c - '0' : c - 'a' + 10);
    }
    return result;
}
static int snapshot(lua_State *L)
{
    P4ActionExec execution;
    if (p4_action_exec_snapshot(check_token(L, 1), &execution)) execution_view(L, &execution);
    else lua_pushnil(L);
    return 1;
}
static int current(lua_State *L)
{
    P4ActionExec execution;
    const uint32_t action = (uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX);
    if (p4_action_exec_snapshot(p4_action_exec_current(action), &execution)) execution_view(L, &execution);
    else lua_pushnil(L);
    return 1;
}
static int live_unit(lua_State *L)
{
    if (p4_lua_frame(L)->snapshot)
        luaL_error(L, "live battle identity is unavailable in snapshot callbacks");
    P4ActionExecUnit unit;
    if (p4_action_exec_native_unit((uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX), &unit)) unit_view(L, &unit);
    else lua_pushnil(L);
    return 1;
}
void p4_lua_battle_execution_initialize(lua_State *L)
{
    luaL_newmetatable(L, EXEC_META);
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_pushcclosure(L, enable_handler, "execution.enable", 1); lua_setfield(L, -2, "enable");
    lua_pushboolean(L, 0); lua_pushcclosure(L, enable_handler, "execution.disable", 1); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, remove_handler, "execution.remove"); lua_setfield(L, -2, "remove");
    lua_setreadonly(L, -1, 1); lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "battle execution subscription"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1); lua_pop(L, 1);
    lua_newtable(L);
    static const luaL_Reg methods[] = {{"on", add_handler}, {"snapshot", snapshot}, {"current", current},
        {"unit", live_unit}, {NULL, NULL}};
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "execution");
    p4_lua_namespace_merge(L, "battle");
}
void p4_lua_battle_execution_close(P4LuaMod *mod)
{
    LuaExecution *handler = listeners;
    while (handler) {
        LuaExecution *next = handler->next;
        if (handler->mod == mod) unlink_handler(handler);
        handler = next;
    }
    /* VM teardown owns its Lua refs. Core observation remains installed so
     * later admissions/epochs cannot be confused with unobserved executions. */
}
