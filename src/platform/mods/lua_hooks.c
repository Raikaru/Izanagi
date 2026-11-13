#include "lua_runtime.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/mod_numbers.h"
#include "p4pc/platform.h"
#include <string.h>

typedef struct LuaHook {
    P4LuaMod *mod;
    P4ModHook *hook;
    int callback, self;
} LuaHook;
struct P4LuaContext {
    P4LuaMod *mod;
    P4ModCall *call;
    uint64_t key;
    int table, self, continuing;
};
typedef struct HookInvocation {
    LuaHook *hook;
    P4ModCall *call;
    P4LuaContext *context;
} HookInvocation;

/* The hook invocation this frame is running on this thread, or NULL outside
 * one and once it has returned. Bindings that act only inside one particular
 * hook check its function, phase and arguments here instead of trusting
 * values a script passes in; *table is the context table whose `args` the
 * invocation copies back after a before-hook. */
const P4ModCall *p4_lua_hook_call(lua_State *L, int *table)
{
    P4LuaContext *context = p4_lua_frame(L)->context;
    if (context == NULL || context->key != p4pc_thread_context_key() || context->call == NULL ||
        context->table == LUA_NOREF)
        return NULL;
    if (table != NULL) *table = context->table;
    return context->call;
}

static void push_value(lua_State *L, char kind, const P4ModValue *value)
{
    switch (kind) {
    case 'v': lua_pushnil(L); break;
    case 'b': case 'h': case 'i': lua_pushinteger(L, (int32_t)value->i); break;
    case 'B': case 'H': case 'I': case 'p': lua_pushunsigned(L, (uint32_t)value->u); break;
    case 'q': case 'Q': p4_lua_push_wide(L, value->u); break;
    case 'f': case 'd': lua_pushnumber(L, p4_mod_f64_quiet(value->d)); break;
    default: luaL_error(L, "unsupported native scalar kind");
    }
}
static void get_value(lua_State *L, int index, char kind, P4ModValue *value)
{
    switch (kind) {
    case 'v': value->u = 0; break;
    case 'b': value->i = p4_lua_integer(L, index, INT8_MIN, INT8_MAX); break;
    case 'h': value->i = p4_lua_integer(L, index, INT16_MIN, INT16_MAX); break;
    case 'i': value->i = p4_lua_integer(L, index, INT32_MIN, INT32_MAX); break;
    case 'B': value->u = (uint64_t)p4_lua_integer(L, index, 0, UINT8_MAX); break;
    case 'H': value->u = (uint64_t)p4_lua_integer(L, index, 0, UINT16_MAX); break;
    case 'I': case 'p': value->u = (uint64_t)p4_lua_integer(L, index, 0, UINT32_MAX); break;
    case 'q': case 'Q': value->u = p4_lua_wide(L, index); break;
    case 'f': case 'd':
        luaL_checktype(L, index, LUA_TNUMBER);
        value->d = p4_mod_f64_quiet(lua_tonumber(L, index));
        break;
    default: luaL_error(L, "unsupported native scalar kind");
    }
}

static void context_args(lua_State *L, P4LuaContext *context)
{
    const char *signature = context->call->function->signature;
    lua_getref(L, context->table);
    lua_getfield(L, -1, "args");
    luaL_checktype(L, -1, LUA_TTABLE);
    for (size_t i = 1; signature[i] != '\0'; ++i) {
        lua_rawgeti(L, -1, (int)i);
        get_value(L, -1, signature[i], &context->call->args[i - 1]);
        lua_pop(L, 1);
    }
    lua_pop(L, 2);
}

static int continue_original(lua_State *L)
{
    P4LuaContext *context = p4_lua_userdata(L, lua_upvalueindex(1), P4_LUA_TAG_CONTEXT, "game continuation");
    P4LuaFrame *frame = p4_lua_frame(L);
    if (context->key != p4pc_thread_context_key() || frame->context != context ||
        context->call == NULL || context->continuing ||
        context->call->phase != P4_MOD_REPLACE || context->call->continued)
        luaL_error(L, "next is valid once, only in its current replacement invocation");
    context->continuing = 1;
    context_args(L, context);
    fenv_t script_environment;
    p4_lua_native_enter(L, &script_environment);
    int status = p4_mod_call_next(context->call);
    p4_lua_native_leave(L, &script_environment);
    context->continuing = 0;
    if (status != 0) luaL_error(L, "original continuation refused");
    push_value(L, context->call->function->signature[0], context->call->result);
    lua_getref(L, context->table);
    lua_pushvalue(L, -2);
    lua_setfield(L, -2, "result");
    lua_pop(L, 1);
    return 1;
}

static int invoke_hook(lua_State *L)
{
    HookInvocation *invocation = lua_touserdata(L, 1);
    LuaHook *hook = invocation->hook;
    P4ModCall *call = invocation->call;
    P4LuaContext *context = lua_newuserdatatagged(L, sizeof *context, P4_LUA_TAG_CONTEXT);
    *context = (P4LuaContext){.mod = hook->mod, .call = call,
        .key = p4pc_thread_context_key(), .table = LUA_NOREF, .self = LUA_NOREF};
    context->self = lua_ref(L, -1);
    invocation->context = context;
    p4_lua_frame(L)->context = context;
    lua_newtable(L);
    context->table = lua_ref(L, -1);
    lua_pushunsigned(L, call->function->address); lua_setfield(L, -2, "address");
    lua_pushstring(L, call->function->signature); lua_setfield(L, -2, "signature");
    static const char *const phases[] = {"before", "after", "replace"};
    lua_pushstring(L, phases[call->phase]); lua_setfield(L, -2, "phase");
    lua_newtable(L);
    for (size_t i = 1; call->function->signature[i] != '\0'; ++i) {
        push_value(L, call->function->signature[i], &call->args[i - 1]);
        lua_rawseti(L, -2, (int)i);
    }
    lua_setfield(L, -2, "args");
    push_value(L, call->function->signature[0], call->result);
    lua_setfield(L, -2, "result");
    lua_getref(L, context->self);
    lua_pushcclosure(L, continue_original, "p4.context.next", 1);
    lua_setfield(L, -2, "next");
    lua_getref(L, hook->callback);
    lua_getref(L, context->table);
    lua_call(L, 1, 0);
    if (call->phase == P4_MOD_BEFORE) context_args(L, context);
    if (call->phase != P4_MOD_BEFORE) {
        lua_getref(L, context->table);
        lua_getfield(L, -1, "result");
        get_value(L, -1, call->function->signature[0], call->result);
    }
    return 0;
}

static int hook_callback(P4ModCall *call, void *user)
{
    LuaHook *hook = user;
    P4LuaMod *mod = hook->mod;
    HookInvocation invocation = {.hook = hook, .call = call};
    int status = p4_lua_invoke(mod, invoke_hook, &invocation, 0, P4_LUA_SEMANTIC);
    if (invocation.context != NULL) {
        P4LuaContext *context = invocation.context;
        int table = context->table, self = context->self;
        context->call = NULL;
        context->table = context->self = LUA_NOREF;
        if (table != LUA_NOREF) lua_unref(mod->state, table);
        if (self != LUA_NOREF) lua_unref(mod->state, self);
    }
    if (status) p4_mod_failure(mod->owner, p4_lua_error(mod));
    return status;
}

static int hook_enable(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaHook *hook = p4_lua_userdata(L, 1, P4_LUA_TAG_HOOK, "hook subscription");
    if (hook->mod != p4_luau_user(L) ||
        p4_mod_hook_enable(hook->hook, lua_toboolean(L, lua_upvalueindex(1))) != 0)
        luaL_error(L, "cannot enable removed/conflicting subscription");
    return 0;
}
static int hook_remove(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaHook *hook = p4_lua_userdata(L, 1, P4_LUA_TAG_HOOK, "hook subscription");
    if (hook->mod != p4_luau_user(L)) luaL_error(L, "foreign subscription");
    if (hook->hook == NULL) return 0;
    if (p4_mod_hook_remove(hook->hook) != 0) luaL_error(L, "subscription removal refused");
    hook->hook = NULL;
    lua_unref(L, hook->callback);
    lua_unref(L, hook->self);
    hook->callback = hook->self = LUA_NOREF;
    return 0;
}

static int add_hook(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    uint32_t address = p4_lua_function_address(L, 1);
    const char *signature = luaL_checkstring(L, 2);
    const char *phase_name = luaL_checkstring(L, 3);
    int phase;
    if (strcmp(phase_name, "before") == 0) phase = P4_MOD_BEFORE;
    else if (strcmp(phase_name, "after") == 0) phase = P4_MOD_AFTER;
    else if (strcmp(phase_name, "replace") == 0) phase = P4_MOD_REPLACE;
    else { luaL_error(L, "phase must be before, after or replace"); }
    luaL_checktype(L, 4, LUA_TFUNCTION);
    P4ModFunction *function = p4_mod_function_find(address, signature);
    if (function == NULL) luaL_error(L, "unknown address or incompatible scalar signature");
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaHook *hook = lua_newuserdatataggedwithmetatable(L, sizeof *hook, P4_LUA_TAG_HOOK);
    *hook = (LuaHook){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF};
    hook->callback = lua_ref(L, 4);
    hook->self = lua_ref(L, -1);
    hook->hook = p4_mod_hook_add(mod->owner, function, phase, hook_callback, hook);
    if (hook->hook == NULL) {
        lua_unref(L, hook->callback);
        lua_unref(L, hook->self);
        hook->callback = hook->self = LUA_NOREF;
        luaL_error(L, "hook registration refused (active dispatch or conflicting replacement)");
    }
    ++mod->subscriptions;
    return 1;
}

static int call_original(lua_State *L)
{
    uint32_t address = p4_lua_function_address(L, 1);
    const char *signature = luaL_checkstring(L, 2);
    P4ModFunction *function = p4_mod_function_find(address, signature);
    if (function == NULL) luaL_error(L, "unknown address or incompatible scalar signature");
    size_t count = strlen(signature) - 1;
    if ((size_t)lua_gettop(L) != count + 2) luaL_error(L, "wrong native argument count");
    P4ModValue arguments[12], result = {.u = 0};
    for (size_t i = 0; i < count; ++i) get_value(L, (int)i + 3, signature[i + 1], &arguments[i]);
    fenv_t script_environment;
    p4_lua_native_enter(L, &script_environment);
    p4_mod_original(function, arguments, &result);
    p4_lua_native_leave(L, &script_environment);
    if (signature[0] == 'v') return 0;
    push_value(L, signature[0], &result);
    return 1;
}
static int describe_function(lua_State *L)
{
    uint32_t address = p4_lua_function_address(L, 1);
    P4ModFunction *function = p4_mod_function_find(address, NULL);
    if (function == NULL) { lua_pushnil(L); return 1; }
    lua_newtable(L);
    lua_pushunsigned(L, function->address); lua_setfield(L, -2, "address");
    lua_pushstring(L, function->signature); lua_setfield(L, -2, "signature");
    lua_pushboolean(L, function->entry->enabled != P4_MOD_INACTIVE); lua_setfield(L, -2, "hooked");
    lua_setreadonly(L, -1, 1);
    return 1;
}

void p4_lua_hooks(P4LuaMod *mod)
{
    lua_State *L = mod->state;
    lua_newtable(L);
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_pushcclosure(L, hook_enable, "p4.hook.enable", 1); lua_setfield(L, -2, "enable");
    lua_pushboolean(L, 0); lua_pushcclosure(L, hook_enable, "p4.hook.disable", 1); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, hook_remove, "p4.hook.remove"); lua_setfield(L, -2, "remove");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 hook"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_HOOK);
    lua_getglobal(L, "p4");
    lua_pushcfunction(L, add_hook, "p4.hook"); lua_setfield(L, -2, "hook");
    lua_pushcfunction(L, call_original, "p4.call"); lua_setfield(L, -2, "call");
    lua_pushcfunction(L, describe_function, "p4.describe"); lua_setfield(L, -2, "describe");
    lua_pushcfunction(L, p4_lua_function_reference, "p4.fn"); lua_setfield(L, -2, "fn");
    lua_setreadonly(L, -1, 1);
    lua_pop(L, 1);
}
