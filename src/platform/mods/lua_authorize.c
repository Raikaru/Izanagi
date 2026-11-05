/* p4.authorize.on(operation, callback): game-owned operation authorizers.
 * Callbacks run in snapshot mode (no native calls, no guest or settings
 * mutation) and answer (allowed, reason?). A Lua error fails closed into the
 * native mod recovery owner; it never authorizes. */
#include "native_bindings.h"
#include "p4pc/mod_authorize.h"
#include <stdio.h>
#include <string.h>

typedef struct LuaAuthorizer {
    P4LuaMod *mod;
    P4ModAuthorizer *authorizer;
    P4ModFormation *formation; /* set instead of authorizer for p4.battle.formation */
    int callback, self;
} LuaAuthorizer;
typedef struct AuthorizeCall {
    LuaAuthorizer *handler;
    const P4ModEvent *context;
    int allowed;
    char *reason;
    size_t capacity;
} AuthorizeCall;

static int invoke_authorize(lua_State *L)
{
    AuthorizeCall *call = lua_touserdata(L, 1);
    const P4ModEvent *context = call->context;
    lua_getref(L, call->handler->callback);
    lua_createtable(L, 0, (int)context->count + 1);
    lua_pushstring(L, context->name); lua_setfield(L, -2, "name");
    for (size_t i = 0; i < context->count; ++i) {
        int64_t value = context->fields[i].value;
        if (value >= 0 && value <= UINT32_MAX) lua_pushunsigned(L, (uint32_t)value);
        else if (value >= INT32_MIN && value < 0) lua_pushinteger(L, (int32_t)value);
        else p4_lua_push_wide(L, (uint64_t)value);
        lua_setfield(L, -2, context->fields[i].name);
    }
    lua_setreadonly(L, -1, 1);
    lua_call(L, 1, 2);
    call->allowed = lua_toboolean(L, -2);
    if (!call->allowed) {
        size_t length = 0;
        const char *reason = lua_type(L, -1) == LUA_TSTRING ? lua_tolstring(L, -1, &length) : NULL;
        if (reason && length) snprintf(call->reason, call->capacity, "%.*s", (int)length, reason);
        else snprintf(call->reason, call->capacity, "refused by mod %s", call->handler->mod->info->id);
    }
    return 0;
}
static int authorize_callback(const P4ModEvent *context, char *reason, size_t capacity, void *user)
{
    LuaAuthorizer *handler = user;
    P4LuaMod *mod = handler->mod;
    AuthorizeCall call = {handler, context, 0, reason, capacity};
    if (p4_lua_invoke(mod, invoke_authorize, &call, 0, P4_LUA_SNAPSHOT)) return -1;
    /* A denial changes the game's outcome: that is semantic use of the mod. */
    if (!call.allowed) p4_lua_semantic_use(mod);
    return call.allowed;
}

static int enable_authorizer(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaAuthorizer *handler = p4_lua_userdata(L, 1, P4_LUA_TAG_AUTHORIZER, "authorizer");
    int enabled = lua_toboolean(L, lua_upvalueindex(1));
    if (handler->mod != p4_luau_user(L) || (handler->formation ?
            p4_mod_formation_enable(handler->formation, enabled) :
            p4_mod_authorizer_enable(handler->authorizer, enabled)) != 0)
        luaL_error(L, "cannot enable a removed authorizer or during authorization");
    return 0;
}
static int remove_authorizer(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaAuthorizer *handler = p4_lua_userdata(L, 1, P4_LUA_TAG_AUTHORIZER, "authorizer");
    if (handler->mod != p4_luau_user(L)) luaL_error(L, "foreign authorizer");
    if (handler->authorizer == NULL && handler->formation == NULL) return 0;
    if ((handler->formation ? p4_mod_formation_remove(handler->formation) :
            p4_mod_authorizer_remove(handler->authorizer)) != 0)
        luaL_error(L, "authorizer removal refused during authorization");
    handler->authorizer = NULL;
    handler->formation = NULL;
    lua_unref(L, handler->callback);
    lua_unref(L, handler->self);
    handler->callback = handler->self = LUA_NOREF;
    return 0;
}
static int add_authorizer(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    const char *operation = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaAuthorizer *handler = lua_newuserdatataggedwithmetatable(L, sizeof *handler, P4_LUA_TAG_AUTHORIZER);
    *handler = (LuaAuthorizer){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF};
    handler->callback = lua_ref(L, 2);
    handler->self = lua_ref(L, -1);
    handler->authorizer = p4_mod_authorizer_add(mod->owner, operation, authorize_callback, handler);
    if (handler->authorizer == NULL) {
        lua_unref(L, handler->callback);
        lua_unref(L, handler->self);
        handler->callback = handler->self = LUA_NOREF;
        luaL_error(L, "authorizer refused (unknown operation or active authorization)");
    }
    ++mod->subscriptions;
    return 1;
}

typedef struct FormationCall {
    LuaAuthorizer *handler;
    const uint8_t *ids;
    int count;
    uint8_t *out;
    int result;
} FormationCall;
static int invoke_formation(lua_State *L)
{
    FormationCall *call = lua_touserdata(L, 1);
    lua_getref(L, call->handler->callback);
    lua_createtable(L, 0, 2);
    lua_pushliteral(L, "formation"); lua_setfield(L, -2, "name");
    lua_createtable(L, call->count, 0);
    for (int i = 0; i < call->count; ++i) {
        lua_pushinteger(L, call->ids[i]);
        lua_rawseti(L, -2, i + 1);
    }
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "party");
    lua_setreadonly(L, -1, 1);
    lua_call(L, 1, 1);
    call->result = 0;
    if (lua_isnil(L, -1)) return 0;
    luaL_checktype(L, -1, LUA_TTABLE);
    int n = lua_objlen(L, -1);
    if (n < 1 || n > 4) luaL_error(L, "p4.battle.formation must return 1..4 character IDs or nil");
    for (int i = 0; i < n; ++i) {
        lua_rawgeti(L, -1, i + 1);
        call->out[i] = (uint8_t)p4_lua_integer(L, -1, 1, 10);
        lua_pop(L, 1);
    }
    call->result = n;
    return 0;
}
static int formation_callback(const uint8_t *ids, int count, uint8_t *out, void *user)
{
    LuaAuthorizer *handler = user;
    FormationCall call = {handler, ids, count, out, 0};
    if (p4_lua_invoke(handler->mod, invoke_formation, &call, 0, P4_LUA_SNAPSHOT)) return -1;
    if (call.result) p4_lua_semantic_use(handler->mod);
    return call.result;
}
static int add_formation(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    luaL_checktype(L, 1, LUA_TFUNCTION);
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaAuthorizer *handler = lua_newuserdatataggedwithmetatable(L, sizeof *handler, P4_LUA_TAG_AUTHORIZER);
    *handler = (LuaAuthorizer){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF};
    handler->callback = lua_ref(L, 1);
    handler->self = lua_ref(L, -1);
    handler->formation = p4_mod_formation_add(mod->owner, formation_callback, handler);
    if (handler->formation == NULL) {
        lua_unref(L, handler->callback);
        lua_unref(L, handler->self);
        handler->callback = handler->self = LUA_NOREF;
        luaL_error(L, "battle formation refused (another mod already projects the lineup)");
    }
    ++mod->subscriptions;
    return 1;
}

void p4_lua_authorize_initialize(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_pushcclosure(L, enable_authorizer, "p4.authorizer.enable", 1); lua_setfield(L, -2, "enable");
    lua_pushboolean(L, 0); lua_pushcclosure(L, enable_authorizer, "p4.authorizer.disable", 1); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, remove_authorizer, "p4.authorizer.remove"); lua_setfield(L, -2, "remove");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 authorizer"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_AUTHORIZER);
    static const luaL_Reg methods[] = {{"on", add_authorizer}, {NULL, NULL}};
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "authorize");
    static const luaL_Reg battle[] = {{"formation", add_formation}, {NULL, NULL}};
    p4_lua_library(L, battle);
    lua_setfield(L, -2, "battle");
}
void p4_lua_authorize_close(P4LuaMod *mod)
{
    p4_mod_authorizers_remove_owner(mod->owner);
    p4_mod_formation_remove_owner(mod->owner);
}
