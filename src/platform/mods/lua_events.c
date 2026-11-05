#include "lua_runtime.h"
#include "p4pc/mod_events.h"
#include <string.h>

typedef struct LuaEvent {
    P4LuaMod *mod;
    P4ModEventSubscription *subscription;
    int callback, self;
} LuaEvent;
typedef struct EventInvocation {
    LuaEvent *handler;
    const P4ModEvent *event;
} EventInvocation;

static int invoke_event(lua_State *L)
{
    EventInvocation *invocation = lua_touserdata(L, 1);
    const P4ModEvent *event = invocation->event;
    lua_getref(L, invocation->handler->callback);
    lua_createtable(L, 0, (int)event->count + 1);
    lua_pushstring(L, event->name); lua_setfield(L, -2, "name");
    for (size_t i = 0; i < event->count; ++i) {
        int64_t value = event->fields[i].value;
        if (value >= 0 && value <= UINT32_MAX) lua_pushunsigned(L, (uint32_t)value);
        else if (value >= INT32_MIN && value < 0) lua_pushinteger(L, (int32_t)value);
        else p4_lua_push_wide(L, (uint64_t)value);
        lua_setfield(L, -2, event->fields[i].name);
    }
    lua_setreadonly(L, -1, 1);
    lua_call(L, 1, 0);
    return 0;
}
static int event_callback(const P4ModEvent *event, void *user)
{
    LuaEvent *handler = user;
    P4LuaMod *mod = handler->mod;
    EventInvocation invocation = {handler, event};
    int status = p4_lua_invoke(mod, invoke_event, &invocation, 0, P4_LUA_SEMANTIC);
    /* The orderly quit proceeds regardless: an exit callback error is logged
     * (p4_lua_protected already printed it), never turned into recovery UI. */
    if (status && strcmp(event->name, "exit")) p4_mod_failure(mod->owner, p4_lua_error(mod));
    return 0;
}

static int enable_event(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaEvent *handler = p4_lua_userdata(L, 1, P4_LUA_TAG_EVENT, "event subscription");
    if (handler->mod != p4_luau_user(L) ||
        p4_mod_event_enable(handler->subscription, lua_toboolean(L, lua_upvalueindex(1))) != 0)
        luaL_error(L, "cannot enable removed/conflicting event subscription");
    return 0;
}
static int remove_event(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaEvent *handler = p4_lua_userdata(L, 1, P4_LUA_TAG_EVENT, "event subscription");
    if (handler->mod != p4_luau_user(L)) luaL_error(L, "foreign event subscription");
    if (handler->subscription == NULL) return 0;
    if (p4_mod_event_remove(handler->subscription) != 0) luaL_error(L, "event removal refused");
    handler->subscription = NULL;
    lua_unref(L, handler->callback);
    lua_unref(L, handler->self);
    handler->callback = handler->self = LUA_NOREF;
    return 0;
}

static int subscribe(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaEvent *handler = lua_newuserdatataggedwithmetatable(L, sizeof *handler, P4_LUA_TAG_EVENT);
    *handler = (LuaEvent){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF};
    handler->callback = lua_ref(L, 2);
    handler->self = lua_ref(L, -1);
    handler->subscription = p4_mod_event_subscribe(mod->owner, name, event_callback, handler);
    if (handler->subscription == NULL) {
        lua_unref(L, handler->callback);
        lua_unref(L, handler->self);
        handler->callback = handler->self = LUA_NOREF;
        luaL_error(L, "event registration refused (unknown event, missing native seam or active dispatch)");
    }
    ++mod->subscriptions;
    return 1;
}

void p4_lua_events(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_pushcclosure(L, enable_event, "p4.event.enable", 1); lua_setfield(L, -2, "enable");
    lua_pushboolean(L, 0); lua_pushcclosure(L, enable_event, "p4.event.disable", 1); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, remove_event, "p4.event.remove"); lua_setfield(L, -2, "remove");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 event subscription"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_EVENT);
    static const luaL_Reg methods[] = {{"on", subscribe}, {NULL, NULL}};
    p4_lua_library(L, methods);
}
