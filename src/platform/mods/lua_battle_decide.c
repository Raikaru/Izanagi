/* p4.battle.decide (capability battle.decide@1): a mod's decision for an
 * enemy's own turn at the AI-to-READY transition (battle_decide.c). The
 * callback runs in snapshot mode with a copied view of the battle; it returns
 * nil (keep the enemy's choice), { pass = true }, or { skill = id, target = unit? }.
 * Legality is the game's own target collector; an illegal choice is refused
 * with a log line and the enemy's choice stands. A Lua error enters the
 * native mod recovery owner, like an authorizer's. */
#include "native_bindings.h"
#include "p4pc/battle_decide.h"
#include <string.h>

#define DECIDER_META "p4.battle.decider"

typedef struct LuaDecider {
    P4LuaMod *mod;
    P4BattleDecider *decider;
    int callback, self;
} LuaDecider;
typedef struct DecideCall {
    LuaDecider *handler;
    const P4BattleDecideState *state;
    P4BattleDecision *decision;
} DecideCall;

/* d.candidates is valid only during the callback that received it. */
static const P4BattleDecideState *current_state;
static uint32_t current_generation;

static void push_unit(lua_State *L, const P4BattleDecideUnit *unit, uint32_t index)
{
    lua_createtable(L, 0, 12);
    lua_pushinteger(L, (int)index + 1); lua_setfield(L, -2, "slot");
    lua_pushstring(L, unit->side ? "enemy" : "party"); lua_setfield(L, -2, "side");
    lua_pushinteger(L, unit->id); lua_setfield(L, -2, "id");
    lua_pushunsigned(L, unit->uid); lua_setfield(L, -2, "uid");
    lua_pushboolean(L, unit->alive); lua_setfield(L, -2, "alive");
    lua_pushinteger(L, unit->hp); lua_setfield(L, -2, "hp");
    lua_pushinteger(L, unit->hp_max); lua_setfield(L, -2, "hp_max");
    lua_pushinteger(L, unit->sp); lua_setfield(L, -2, "sp");
    lua_pushinteger(L, unit->sp_max); lua_setfield(L, -2, "sp_max");
    lua_pushnumber(L, unit->hp_max ? (double)unit->hp / unit->hp_max : 0.0); lua_setfield(L, -2, "hp_fraction");
    lua_pushunsigned(L, unit->status); lua_setfield(L, -2, "status");
    lua_pushboolean(L, unit->charged); lua_setfield(L, -2, "charged");
    lua_setreadonly(L, -1, 1);
}
/* Pushes a read-only array of the unit views at units_index for these indices. */
static void push_units(lua_State *L, int units_index, const uint8_t *indices, uint32_t count)
{
    lua_createtable(L, (int)count, 0);
    for (uint32_t i = 0; i < count; ++i) {
        lua_rawgeti(L, units_index, indices[i] + 1);
        lua_rawseti(L, -2, (int)i + 1);
    }
    lua_setreadonly(L, -1, 1);
}
static int candidates(lua_State *L)
{
    if (!current_state || lua_tounsigned(L, lua_upvalueindex(2)) != current_generation)
        luaL_error(L, "d.candidates is only valid during the decide callback that received it");
    const uint16_t skill = (uint16_t)p4_lua_integer(L, 1, 0, UINT16_MAX);
    uint8_t indices[12];
    uint32_t count = 0;
    int all = 0;
    if (p4_battle_decide_candidates(current_state, skill, indices, &count, &all)) count = 0;
    push_units(L, lua_upvalueindex(1), indices, count);
    lua_pushboolean(L, all);
    return 2;
}
static uint32_t unit_slot(lua_State *L, int index, const P4BattleDecideState *state)
{
    if (lua_istable(L, index)) {
        lua_getfield(L, index, "slot");
        const int64_t slot = lua_isnumber(L, -1) ? (int64_t)lua_tointeger(L, -1) : 0;
        lua_pop(L, 1);
        if (slot < 1 || slot > (int64_t)state->count) luaL_error(L, "p4.battle.decide: target is not a unit of this decision");
        return (uint32_t)slot - 1;
    }
    return (uint32_t)p4_lua_integer(L, index, 1, state->count) - 1;
}
static int invoke_decide(lua_State *L)
{
    DecideCall *call = lua_touserdata(L, 1);
    const P4BattleDecideState *state = call->state;
    lua_getref(L, call->handler->callback);
    lua_createtable(L, 0, 10);
    const int d = lua_gettop(L);
    lua_createtable(L, (int)state->count, 0);
    const int units = lua_gettop(L);
    uint8_t party[P4_BATTLE_DECIDE_UNITS], enemies[P4_BATTLE_DECIDE_UNITS];
    uint32_t parties = 0, enemy_count = 0;
    for (uint32_t i = 0; i < state->count; ++i) {
        push_unit(L, &state->units[i], i);
        lua_rawseti(L, units, (int)i + 1);
        if (state->units[i].side) enemies[enemy_count++] = (uint8_t)i;
        else party[parties++] = (uint8_t)i;
    }
    push_units(L, units, party, parties); lua_setfield(L, d, "party");
    push_units(L, units, enemies, enemy_count); lua_setfield(L, d, "enemies");
    lua_rawgeti(L, units, (int)state->actor + 1); lua_setfield(L, d, "actor");
    lua_createtable(L, 0, 3);
    lua_pushstring(L, state->pick == P4_BATTLE_PICK_SKILL ? "skill" : state->pick == P4_BATTLE_PICK_PASS ? "pass" : "other");
    lua_setfield(L, -2, "kind");
    if (state->pick == P4_BATTLE_PICK_SKILL) { lua_pushinteger(L, state->skill); lua_setfield(L, -2, "skill"); }
    push_units(L, units, state->targets, state->target_count); lua_setfield(L, -2, "targets");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, d, "pick");
    lua_pushboolean(L, state->one_more != 0); lua_setfield(L, d, "one_more");
    lua_pushboolean(L, state->extra != 0); lua_setfield(L, d, "extra");
    lua_pushinteger(L, state->round); lua_setfield(L, d, "round");
    lua_pushvalue(L, units);
    lua_pushunsigned(L, current_generation);
    lua_pushcclosure(L, candidates, "d.candidates", 2);
    lua_setfield(L, d, "candidates");
    lua_setreadonly(L, units, 1);
    lua_setfield(L, d, "units");
    lua_setreadonly(L, d, 1);
    lua_call(L, 1, 1);
    P4BattleDecision *decision = call->decision;
    if (lua_isnil(L, -1)) return 0;
    if (!lua_istable(L, -1)) luaL_error(L, "p4.battle.decide: fn must return nil or a table");
    const int result = lua_gettop(L);
    lua_getfield(L, result, "pass");
    const int pass = lua_toboolean(L, -1);
    lua_getfield(L, result, "skill");
    lua_getfield(L, result, "target");
    if (pass) {
        if (!lua_isnil(L, -2) || !lua_isnil(L, -1)) luaL_error(L, "p4.battle.decide: { pass = true } takes no skill or target");
        decision->pick = P4_BATTLE_PICK_PASS;
        return 0;
    }
    if (lua_isnil(L, -2)) luaL_error(L, "p4.battle.decide: return { skill = id, target = unit? }, { pass = true } or nil");
    decision->skill = (uint16_t)p4_lua_integer(L, -2, 0, UINT16_MAX);
    if (!lua_isnil(L, -1)) {
        decision->target = unit_slot(L, lua_gettop(L), state);
        decision->has_target = 1;
    }
    decision->pick = P4_BATTLE_PICK_SKILL;
    return 0;
}
static int decide_callback(const P4BattleDecideState *state, P4BattleDecision *decision, void *user)
{
    LuaDecider *handler = user;
    DecideCall call = {handler, state, decision};
    current_state = state;
    ++current_generation;
    const int status = p4_lua_invoke(handler->mod, invoke_decide, &call, 0, P4_LUA_SNAPSHOT);
    current_state = NULL;
    if (status) { decision->pick = P4_BATTLE_PICK_OTHER; return -1; }
    return 0;
}
static void applied(void *user)
{
    /* A changed enemy turn is semantic use of the mod. */
    p4_lua_semantic_use(((LuaDecider *)user)->mod);
}

static LuaDecider *check_decider(lua_State *L)
{
    LuaDecider *handler = luaL_checkudata(L, 1, DECIDER_META);
    if (handler->mod != p4_luau_user(L)) luaL_error(L, "foreign battle decider");
    return handler;
}
static int enable_decider(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaDecider *handler = check_decider(L);
    if (!handler->decider || p4_battle_decider_enable(handler->decider, lua_toboolean(L, lua_upvalueindex(1))))
        luaL_error(L, "cannot enable a removed battle decider or during a decision");
    return 0;
}
static int remove_decider(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaDecider *handler = check_decider(L);
    if (!handler->decider) return 0;
    if (p4_battle_decider_remove(handler->decider)) luaL_error(L, "battle decider removal refused during a decision");
    handler->decider = NULL;
    lua_unref(L, handler->callback);
    lua_unref(L, handler->self);
    handler->callback = handler->self = LUA_NOREF;
    return 0;
}
static int add_decider(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "side");
    if (!lua_isnil(L, -1) && strcmp(luaL_checkstring(L, -1), "enemy"))
        luaL_error(L, "p4.battle.decide: side must be \"enemy\" (capability battle.decide@1)");
    lua_getfield(L, 1, "priority");
    const int32_t priority = lua_isnil(L, -1) ? 0 : (int32_t)p4_lua_integer(L, -1, INT32_MIN, INT32_MAX);
    lua_getfield(L, 1, "fn");
    luaL_checktype(L, -1, LUA_TFUNCTION);
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaDecider *handler = lua_newuserdata(L, sizeof *handler);
    *handler = (LuaDecider){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF};
    luaL_getmetatable(L, DECIDER_META);
    lua_setmetatable(L, -2);
    lua_pushvalue(L, -2);
    handler->callback = lua_ref(L, -1);
    lua_pop(L, 1);
    handler->self = lua_ref(L, -1);
    handler->decider = p4_battle_decider_add(mod->owner, priority, decide_callback, applied, handler);
    if (!handler->decider) {
        lua_unref(L, handler->callback);
        lua_unref(L, handler->self);
        handler->callback = handler->self = LUA_NOREF;
        luaL_error(L, "battle decider refused (during a decision or out of memory)");
    }
    ++mod->subscriptions;
    return 1;
}

void p4_lua_battle_decide_initialize(lua_State *L)
{
    luaL_newmetatable(L, DECIDER_META);
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_pushcclosure(L, enable_decider, "p4.battle.decider.enable", 1); lua_setfield(L, -2, "enable");
    lua_pushboolean(L, 0); lua_pushcclosure(L, enable_decider, "p4.battle.decider.disable", 1); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, remove_decider, "p4.battle.decider.remove"); lua_setfield(L, -2, "remove");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 battle decider"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_pop(L, 1);
    /* p4.battle may already hold other battle services (formation): the
     * namespace becomes one read-only table with theirs and this one. */
    lua_newtable(L);
    lua_getfield(L, -2, "battle");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_settable(L, -5);
        }
    }
    lua_pop(L, 1);
    lua_pushcfunction(L, add_decider, "p4.battle.decide");
    lua_setfield(L, -2, "decide");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "battle");
}
void p4_lua_battle_decide_close(P4LuaMod *mod)
{
    p4_battle_deciders_remove_owner(mod->owner);
}
