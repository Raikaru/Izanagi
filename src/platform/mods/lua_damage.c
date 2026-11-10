/* p4.damage.on{kind, priority, fn}: final-damage modifiers registered in the
 * native stage (include/p4pc/game_damage_stage.h, the end of datCalcSkillAmount
 * 0x00235520). fn runs synchronously in snapshot mode (no native calls, no
 * guest or settings mutation) on a frozen copy of one eligible hit and returns
 * a factor ("multiply") or an addend ("add"), or nil to leave the hit alone.
 * Registration is a start-up declaration; a disabled modifier keeps its place
 * in the stage order and never enters Lua. A Lua error or an exhausted budget
 * never applies a value: the mod enters native recovery for that hit. */
#include "native_bindings.h"
#include "p4pc/game_damage_stage.h"
#include <string.h>

typedef struct LuaDamage {
    P4LuaMod *mod;
    struct LuaDamage *next; /* every registered modifier, for VM teardown */
    uint32_t handle;        /* stage handle; 0 once removed */
    int callback, self, enabled;
    P4GameDamageKind kind;
} LuaDamage;
typedef struct DamageCall {
    const LuaDamage *handler;
    const P4GameDamageHit *hit;
    double value;
    int apply;
} DamageCall;
static LuaDamage *registered;

static const char *side_name(uint32_t side) { return side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party"; }
static const char *hit_name(uint32_t kind)
{
    return kind == P4_DAMAGE_HIT_CRITICAL ? "critical" : kind == P4_DAMAGE_HIT_WEAK ? "weak" :
        kind == P4_DAMAGE_HIT_NORMAL ? "normal" : "other";
}
static int invoke_damage(lua_State *L)
{
    DamageCall *call = lua_touserdata(L, 1);
    const P4GameDamageHit *hit = call->hit;
    lua_getref(L, call->handler->callback);
    lua_createtable(L, 0, 14);
    lua_pushunsigned(L, hit->skill_id); lua_setfield(L, -2, "skill");
    lua_pushstring(L, side_name(hit->source_side)); lua_setfield(L, -2, "source_side");
    lua_pushunsigned(L, hit->source_id); lua_setfield(L, -2, "source_id");
    lua_pushunsigned(L, hit->source_status); lua_setfield(L, -2, "source_status");
    lua_pushstring(L, side_name(hit->target_side)); lua_setfield(L, -2, "target_side");
    lua_pushunsigned(L, hit->target_id); lua_setfield(L, -2, "target_id");
    lua_pushunsigned(L, hit->target_status); lua_setfield(L, -2, "target_status");
    lua_pushstring(L, side_name(hit->origin_side)); lua_setfield(L, -2, "origin_side");
    lua_pushunsigned(L, hit->origin_id); lua_setfield(L, -2, "origin_id");
    lua_pushstring(L, hit_name(hit->hit_kind)); lua_setfield(L, -2, "hit_kind");
    if (hit->element_valid) { lua_pushunsigned(L, hit->element_id); lua_setfield(L, -2, "element"); }
    lua_pushboolean(L, hit->reflected != 0); lua_setfield(L, -2, "reflected");
    lua_pushboolean(L, hit->resolved != 0); lua_setfield(L, -2, "resolved");
    lua_pushinteger(L, hit->stock); lua_setfield(L, -2, "stock");
    lua_setreadonly(L, -1, 1);
    lua_call(L, 1, 1);
    if (lua_isnil(L, -1)) return 0;
    if (lua_type(L, -1) != LUA_TNUMBER) luaL_error(L, "p4.damage callback must return a number or nil");
    call->value = lua_tonumber(L, -1);
    call->apply = 1;
    return 0;
}
static int damage_modifier(void *user, const P4GameDamageHit *hit, double *value)
{
    const LuaDamage *handler = user;
    if (!handler->enabled) return 0;
    P4LuaMod *mod = handler->mod;
    DamageCall call = {handler, hit, 0.0, 0};
    if (p4_lua_invoke(mod, invoke_damage, &call, 0, P4_LUA_SYNCHRONOUS | P4_LUA_SNAPSHOT))
        p4_mod_failure(mod->owner, p4_lua_error(mod));
    /* An identity value leaves the hit alone; only a change is semantic use. */
    if (!call.apply || call.value == (handler->kind == P4_DAMAGE_MULTIPLY ? 1.0 : 0.0)) return 0;
    p4_lua_semantic_use(mod);
    *value = call.value;
    return 1;
}

static void forget(LuaDamage *handler)
{
    for (LuaDamage **link = &registered; *link; link = &(*link)->next) {
        if (*link != handler) continue;
        *link = handler->next;
        break;
    }
    handler->next = NULL;
    handler->handle = 0;
}
static int enable_damage(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaDamage *handler = p4_lua_userdata(L, 1, P4_LUA_TAG_DAMAGE, "damage modifier");
    if (handler->mod != p4_luau_user(L) || !handler->handle)
        luaL_error(L, "cannot enable a removed or foreign damage modifier");
    handler->enabled = lua_toboolean(L, lua_upvalueindex(1));
    return 0;
}
static int remove_damage(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaDamage *handler = p4_lua_userdata(L, 1, P4_LUA_TAG_DAMAGE, "damage modifier");
    if (handler->mod != p4_luau_user(L)) luaL_error(L, "foreign damage modifier");
    if (!handler->handle) return 0;
    if (p4_game_damage_unregister(handler->handle) != P4_DAMAGE_OK)
        luaL_error(L, "damage modifier removal refused while a hit is composed");
    forget(handler);
    lua_unref(L, handler->callback);
    lua_unref(L, handler->self);
    handler->callback = handler->self = LUA_NOREF;
    return 0;
}
static int add_damage(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    if (!p4_lua_frame(L)->startup) luaL_error(L, "p4.damage.on is a start-up declaration, refused inside callbacks");
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "kind");
    const char *kind_name = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "";
    P4GameDamageKind kind;
    if (!strcmp(kind_name, "multiply")) kind = P4_DAMAGE_MULTIPLY;
    else if (!strcmp(kind_name, "add")) kind = P4_DAMAGE_ADD;
    else { luaL_error(L, "p4.damage.on: kind must be \"multiply\" or \"add\""); }
    lua_pop(L, 1);
    lua_getfield(L, 1, "priority");
    const int32_t priority = lua_isnil(L, -1) ? 0 : (int32_t)p4_lua_integer(L, -1, INT32_MIN, INT32_MAX);
    lua_pop(L, 1);
    lua_getfield(L, 1, "fn");
    if (lua_type(L, -1) != LUA_TFUNCTION) luaL_error(L, "p4.damage.on: fn must be a function");
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaDamage *handler = lua_newuserdatataggedwithmetatable(L, sizeof *handler, P4_LUA_TAG_DAMAGE);
    *handler = (LuaDamage){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF, .enabled = 1, .kind = kind};
    const P4GameDamageModifierDesc desc = {
        .owner = mod->info->id, .priority = priority, .load_order = (uint32_t)mod->owner,
        .kind = kind, .modifier = damage_modifier, .user = handler,
    };
    const int result = p4_game_damage_register(&desc, &handler->handle);
    if (result != P4_DAMAGE_OK) {
        handler->handle = 0;
        luaL_error(L, "damage modifier refused (%s)", result == P4_DAMAGE_BUSY ? "a hit is being composed" :
            result == P4_DAMAGE_NOMEM ? "out of memory" : "invalid owner or kind");
    }
    handler->callback = lua_ref(L, -2);
    handler->self = lua_ref(L, -1);
    handler->next = registered;
    registered = handler;
    ++mod->subscriptions;
    return 1;
}

void p4_lua_damage_initialize(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_pushcclosure(L, enable_damage, "p4.damage.enable", 1); lua_setfield(L, -2, "enable");
    lua_pushboolean(L, 0); lua_pushcclosure(L, enable_damage, "p4.damage.disable", 1); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, remove_damage, "p4.damage.remove"); lua_setfield(L, -2, "remove");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 damage modifier"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_DAMAGE);
    static const luaL_Reg methods[] = {{"on", add_damage}, {NULL, NULL}};
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "damage");
}
/* VM teardown and session end: outside callbacks, so never during a hit. */
void p4_lua_damage_close(P4LuaMod *mod)
{
    for (LuaDamage **link = &registered; *link;) {
        LuaDamage *handler = *link;
        if (handler->mod != mod) { link = &handler->next; continue; }
        p4_game_damage_unregister(handler->handle);
        *link = handler->next;
        handler->next = NULL;
        handler->handle = 0;
    }
}
