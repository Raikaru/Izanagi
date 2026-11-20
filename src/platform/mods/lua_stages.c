/* p4.damage.on, p4.recovery.on, p4.cost.on, p4.chance.on,
 * p4.status.wearoff.on, p4.buff.on, p4.reward.on, p4.unit.on and p4.hit.on
 * {kind, priority, fn}: Luau modifiers registered in the native stages
 * (include/p4pc/game_damage_stage.h, game_recovery_stage.h,
 * game_cost_stage.h, game_chance_stage.h, game_wearoff_stage.h,
 * game_buff_stage.h, game_reward_stage.h, game_unit_stage.h,
 * game_hit_stage.h). fn runs synchronously in snapshot mode (no native
 * calls, no guest or settings mutation) on a frozen copy of one eligible hit
 * and returns a factor ("multiply") or an addend ("add"), or nil to leave the
 * hit alone. Registration is a start-up declaration; a disabled modifier keeps
 * its place in the stage order and never enters Lua. A Lua error or an
 * exhausted budget never applies a value: the mod enters native recovery for
 * that hit. One binding serves every stage; only the hit table differs. */
#include "native_bindings.h"
#include "p4pc/game_buff_stage.h"
#include "p4pc/game_chance_stage.h"
#include "p4pc/game_cost_stage.h"
#include "p4pc/game_damage_stage.h"
#include "p4pc/game_hit_stage.h"
#include "p4pc/game_recovery_stage.h"
#include "p4pc/game_reward_stage.h"
#include "p4pc/game_unit_stage.h"
#include "p4pc/game_wearoff_stage.h"
#include <math.h>
#include <string.h>

typedef enum StageId {
    STAGE_DAMAGE, STAGE_RECOVERY, STAGE_COST, STAGE_CHANCE, STAGE_WEAROFF, STAGE_BUFF, STAGE_REWARD, STAGE_UNIT,
    STAGE_HIT, STAGE_COUNT
} StageId;
typedef struct LuaStage {
    P4LuaMod *mod;
    struct LuaStage *next; /* every registered modifier, for VM teardown */
    uint32_t handle;       /* stage handle; 0 once removed */
    int callback, self, enabled, multiply;
    StageId stage;
    int offense;
    P4GameOffensePolicy policy;
} LuaStage;
typedef struct StageCall {
    const LuaStage *handler;
    const void *hit;
    double value;
    int apply;
} StageCall;
static LuaStage *registered;

static const char *side_name(uint32_t side) { return side == P4_DAMAGE_SIDE_ENEMY ? "enemy" : "party"; }
static const char *hit_name(uint32_t kind)
{
    return kind == P4_DAMAGE_HIT_CRITICAL ? "critical" : kind == P4_DAMAGE_HIT_WEAK ? "weak" :
        kind == P4_DAMAGE_HIT_NORMAL ? "normal" : "other";
}
static const char *origin_name(uint32_t origin)
{
    return origin == P4_DAMAGE_ORIGIN_SKILL ? "skill" : origin == P4_DAMAGE_ORIGIN_ITEM ? "item" :
        origin == P4_DAMAGE_ORIGIN_ATTACK ? "attack" : "unknown";
}
static void set_unsigned(lua_State *L, const char *field, uint32_t value)
{
    lua_pushunsigned(L, value); lua_setfield(L, -2, field);
}
static void set_string(lua_State *L, const char *field, const char *value)
{
    lua_pushstring(L, value); lua_setfield(L, -2, field);
}
static void set_boolean(lua_State *L, const char *field, uint32_t value)
{
    lua_pushboolean(L, value != 0); lua_setfield(L, -2, field);
}
/* A persona field is nil where the hit has no persona source. */
static void set_persona(lua_State *L, const char *persona_field, const char *level_field, uint32_t persona, uint32_t level)
{
    if (!persona) return;
    set_unsigned(L, persona_field, persona);
    set_unsigned(L, level_field, level);
}
static void push_damage(lua_State *L, const P4GameDamageHit *hit)
{
    lua_createtable(L, 0, 17);
    set_unsigned(L, "skill", hit->skill_id);
    set_string(L, "source_side", side_name(hit->source_side));
    set_unsigned(L, "source_id", hit->source_id);
    set_unsigned(L, "source_status", hit->source_status);
    set_string(L, "target_side", side_name(hit->target_side));
    set_unsigned(L, "target_id", hit->target_id);
    set_unsigned(L, "target_status", hit->target_status);
    set_string(L, "origin_side", side_name(hit->origin_side));
    set_unsigned(L, "origin_id", hit->origin_id);
    set_string(L, "hit_kind", hit_name(hit->hit_kind));
    if (hit->element_valid) set_unsigned(L, "element", hit->element_id);
    set_boolean(L, "reflected", hit->reflected);
    set_boolean(L, "resolved", hit->resolved);
    lua_pushinteger(L, hit->stock); lua_setfield(L, -2, "stock");
    set_string(L, "origin", origin_name(hit->origin));
    set_persona(L, "source_persona", "source_persona_level", hit->source_persona, hit->source_persona_level);
}
static void push_recovery(lua_State *L, const P4GameRecoveryHit *hit)
{
    lua_createtable(L, 0, 10);
    set_unsigned(L, "skill", hit->skill_id);
    set_string(L, "source_side", side_name(hit->source_side));
    set_unsigned(L, "source_id", hit->source_id);
    set_string(L, "target_side", side_name(hit->target_side));
    set_unsigned(L, "target_id", hit->target_id);
    set_boolean(L, "resolved", hit->resolved);
    set_string(L, "origin", origin_name(hit->origin));
    lua_pushinteger(L, hit->stock); lua_setfield(L, -2, "stock");
    set_persona(L, "source_persona", "source_persona_level", hit->source_persona, hit->source_persona_level);
}
static void push_cost(lua_State *L, const P4GameCostHit *hit)
{
    lua_createtable(L, 0, 7);
    set_unsigned(L, "skill", hit->skill_id);
    set_string(L, "side", side_name(hit->side));
    set_unsigned(L, "unit", hit->unit_id);
    set_string(L, "resource", hit->resource == P4_COST_RESOURCE_HP ? "hp" : "sp");
    lua_pushinteger(L, hit->stock); lua_setfield(L, -2, "stock");
    set_persona(L, "persona", "persona_level", hit->persona, hit->persona_level);
}
static void push_chance(lua_State *L, const P4GameChanceHit *hit)
{
    lua_createtable(L, 0, 12);
    set_unsigned(L, "skill", hit->skill_id);
    set_string(L, "source_side", side_name(hit->source_side));
    set_unsigned(L, "source_id", hit->source_id);
    set_string(L, "target_side", side_name(hit->target_side));
    set_unsigned(L, "target_id", hit->target_id);
    set_string(L, "component", hit->component == P4_CHANCE_COMPONENT_INSTANT_KO ? "instant_ko" : "ailment");
    set_unsigned(L, "statuses", hit->statuses);
    set_boolean(L, "item", hit->item);
    lua_pushinteger(L, hit->stock); lua_setfield(L, -2, "stock");
    set_unsigned(L, "afflictions", hit->afflictions);
    set_persona(L, "source_persona", "source_persona_level", hit->source_persona, hit->source_persona_level);
}
/* A wear-off roll: whose turn, which ailment, and the native chance. */
static void push_wearoff(lua_State *L, const P4GameWearoffRoll *roll)
{
    lua_createtable(L, 0, 9);
    set_string(L, "side", side_name(roll->side));
    set_unsigned(L, "unit", roll->id);
    set_string(L, "status", p4_game_wearoff_status_name(roll->status) ? p4_game_wearoff_status_name(roll->status) : "other");
    set_unsigned(L, "level", roll->level);
    set_unsigned(L, "luck", roll->luck);
    set_unsigned(L, "turns", roll->turns);
    set_unsigned(L, "limit", roll->limit);
    lua_pushinteger(L, roll->stock); lua_setfield(L, -2, "stock");
    set_unsigned(L, "afflictions", roll->afflictions);
}
/* A buff multiplier: the unit, the stat, its rank, Charge, the native value. */
static void push_buff(lua_State *L, const P4GameBuffQuery *query)
{
    lua_createtable(L, 0, 6);
    set_string(L, "side", side_name(query->side));
    set_unsigned(L, "unit", query->id);
    set_string(L, "stat", p4_game_buff_stat_name(query->stat) ? p4_game_buff_stat_name(query->stat) : "other");
    lua_pushinteger(L, query->rank); lua_setfield(L, -2, "rank");
    set_boolean(L, "charged", query->charged);
    lua_pushnumber(L, query->stock); lua_setfield(L, -2, "stock");
}
/* A battle award: EXP for one recipient, or the team's money. */
static void push_reward(lua_State *L, const P4GameRewardQuery *query)
{
    static const char *const recipients[] = {NULL, "hero", "party", "persona"};
    lua_createtable(L, 0, 6);
    set_string(L, "reward", query->reward == P4_REWARD_MONEY ? "money" : "exp");
    if (query->reward == P4_REWARD_EXP && query->recipient >= P4_REWARD_HERO && query->recipient <= P4_REWARD_PERSONA) {
        set_string(L, "recipient", recipients[query->recipient]);
        set_unsigned(L, "id", query->id);
        set_unsigned(L, "level", query->level);
        if (query->recipient == P4_REWARD_PERSONA) set_boolean(L, "equipped", query->equipped);
    }
    lua_pushinteger(L, query->stock); lua_setfield(L, -2, "stock");
}
/* A unit value: whose, which (max HP/SP or a battle stat), the native value. */
static void push_unit(lua_State *L, const P4GameUnitQuery *query)
{
    lua_createtable(L, 0, 5);
    set_string(L, "side", side_name(query->side));
    set_unsigned(L, "unit", query->id);
    set_string(L, "stat", p4_game_unit_stat_name(query->stat) ? p4_game_unit_stat_name(query->stat) : "other");
    set_unsigned(L, "level", query->level);
    lua_pushinteger(L, query->stock); lua_setfield(L, -2, "stock");
}
/* A rolled accuracy or critical chance: which, the skill, both units, the native chance. */
static void push_hit(lua_State *L, const P4GameHitQuery *query)
{
    lua_createtable(L, 0, 7);
    set_string(L, "roll", query->roll == P4_HIT_CRITICAL ? "critical" : "accuracy");
    set_unsigned(L, "skill", query->skill_id);
    set_string(L, "source_side", side_name(query->source_side));
    set_unsigned(L, "source_id", query->source_id);
    set_string(L, "target_side", side_name(query->target_side));
    set_unsigned(L, "target_id", query->target_id);
    lua_pushinteger(L, query->stock); lua_setfield(L, -2, "stock");
}

static const char *const stage_names[STAGE_COUNT] = {"damage", "recovery", "cost", "chance", "status.wearoff", "buff",
    "reward", "unit", "hit"};
static int invoke_stage(lua_State *L)
{
    StageCall *call = lua_touserdata(L, 1);
    lua_getref(L, call->handler->callback);
    switch (call->handler->stage) {
    case STAGE_DAMAGE: push_damage(L, call->hit); break;
    case STAGE_RECOVERY: push_recovery(L, call->hit); break;
    case STAGE_COST: push_cost(L, call->hit); break;
    case STAGE_WEAROFF: push_wearoff(L, call->hit); break;
    case STAGE_BUFF: push_buff(L, call->hit); break;
    case STAGE_REWARD: push_reward(L, call->hit); break;
    case STAGE_UNIT: push_unit(L, call->hit); break;
    case STAGE_HIT: push_hit(L, call->hit); break;
    default: push_chance(L, call->hit); break;
    }
    lua_setreadonly(L, -1, 1);
    lua_call(L, 1, 1);
    if (lua_isnil(L, -1)) return 0;
    if (lua_type(L, -1) != LUA_TNUMBER)
        luaL_error(L, "p4.%s callback must return a number or nil", stage_names[call->handler->stage]);
    call->value = lua_tonumber(L, -1);
    call->apply = 1;
    return 0;
}
static int modifier(void *user, const void *hit, double *value)
{
    const LuaStage *handler = user;
    if (!handler->enabled) return 0;
    P4LuaMod *mod = handler->mod;
    StageCall call = {handler, hit, 0.0, 0};
    if (p4_lua_invoke(mod, invoke_stage, &call, 0, P4_LUA_SYNCHRONOUS | P4_LUA_SNAPSHOT))
        p4_mod_failure(mod->owner, p4_lua_error(mod));
    /* An identity value leaves the hit alone; only a change is semantic use. */
    if (!call.apply || call.value == (handler->multiply ? 1.0 : 0.0)) return 0;
    p4_lua_semantic_use(mod);
    *value = call.value;
    return 1;
}
static int damage_modifier(void *user, const P4GameDamageHit *hit, double *value) { return modifier(user, hit, value); }
static int recovery_modifier(void *user, const P4GameRecoveryHit *hit, double *value) { return modifier(user, hit, value); }
static int cost_modifier(void *user, const P4GameCostHit *hit, double *value) { return modifier(user, hit, value); }
static int chance_modifier(void *user, const P4GameChanceHit *hit, double *value) { return modifier(user, hit, value); }
static int wearoff_modifier(void *user, const P4GameWearoffRoll *roll, double *value) { return modifier(user, roll, value); }
static int buff_modifier(void *user, const P4GameBuffQuery *query, double *value) { return modifier(user, query, value); }
static int reward_modifier(void *user, const P4GameRewardQuery *query, double *value) { return modifier(user, query, value); }
static int unit_modifier(void *user, const P4GameUnitQuery *query, double *value) { return modifier(user, query, value); }
static int hit_modifier(void *user, const P4GameHitQuery *query, double *value) { return modifier(user, query, value); }

/* Each stage's own descriptor and status codes; all of them share the damage
 * stage's P4_DAMAGE_OK/BUSY/NOMEM/INVALID values. */
static int stage_register(LuaStage *handler, const char *owner, int32_t priority, uint32_t load_order)
{
    switch (handler->stage) {
    case STAGE_DAMAGE: {
        const P4GameDamageModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_DAMAGE_MULTIPLY : P4_DAMAGE_ADD, damage_modifier, handler};
        return p4_game_damage_register(&desc, &handler->handle);
    }
    case STAGE_RECOVERY: {
        const P4GameRecoveryModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_RECOVERY_MULTIPLY : P4_RECOVERY_ADD, recovery_modifier, handler};
        return p4_game_recovery_register(&desc, &handler->handle);
    }
    case STAGE_COST: {
        const P4GameCostModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_COST_MULTIPLY : P4_COST_ADD, cost_modifier, handler};
        return p4_game_cost_register(&desc, &handler->handle);
    }
    case STAGE_WEAROFF: {
        const P4GameWearoffModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_WEAROFF_MULTIPLY : P4_WEAROFF_ADD, wearoff_modifier, handler};
        return p4_game_wearoff_register(&desc, &handler->handle);
    }
    case STAGE_BUFF: {
        const P4GameBuffModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_BUFF_MULTIPLY : P4_BUFF_ADD, buff_modifier, handler};
        return p4_game_buff_register(&desc, &handler->handle);
    }
    case STAGE_REWARD: {
        const P4GameRewardModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_REWARD_MULTIPLY : P4_REWARD_ADD, reward_modifier, handler};
        return p4_game_reward_register(&desc, &handler->handle);
    }
    case STAGE_UNIT: {
        const P4GameUnitModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_UNIT_MULTIPLY : P4_UNIT_ADD, unit_modifier, handler};
        return p4_game_unit_register(&desc, &handler->handle);
    }
    case STAGE_HIT: {
        const P4GameHitModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_HIT_MULTIPLY : P4_HIT_ADD, hit_modifier, handler};
        return p4_game_hit_register(&desc, &handler->handle);
    }
    default: {
        const P4GameChanceModifierDesc desc = {owner, priority, load_order,
            handler->multiply ? P4_CHANCE_MULTIPLY : P4_CHANCE_ADD, chance_modifier, handler};
        return p4_game_chance_register(&desc, &handler->handle);
    }
    }
}
static int stage_unregister(const LuaStage *handler)
{
    switch (handler->stage) {
    case STAGE_DAMAGE: return p4_game_damage_unregister(handler->handle);
    case STAGE_RECOVERY: return p4_game_recovery_unregister(handler->handle);
    case STAGE_COST: return p4_game_cost_unregister(handler->handle);
    case STAGE_WEAROFF: return p4_game_wearoff_unregister(handler->handle);
    case STAGE_BUFF: return p4_game_buff_unregister(handler->handle);
    case STAGE_REWARD: return p4_game_reward_unregister(handler->handle);
    case STAGE_UNIT: return p4_game_unit_unregister(handler->handle);
    case STAGE_HIT: return p4_game_hit_unregister(handler->handle);
    default: return p4_game_chance_unregister(handler->handle);
    }
}

static void forget(LuaStage *handler)
{
    for (LuaStage **link = &registered; *link; link = &(*link)->next) {
        if (*link != handler) continue;
        *link = handler->next;
        break;
    }
    handler->next = NULL;
    handler->handle = 0;
}
static int enable_stage(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaStage *handler = p4_lua_userdata(L, 1, P4_LUA_TAG_STAGE, "stage modifier");
    if (handler->mod != p4_luau_user(L) || !handler->handle)
        luaL_error(L, "cannot enable a removed or foreign %s modifier", stage_names[handler->stage]);
    if (handler->offense) {
        if (lua_toboolean(L, lua_upvalueindex(1))) {
            if (p4_game_offense_policy_enable(handler, &handler->policy) != P4_DAMAGE_OK)
                luaL_error(L, "offense policy refused: another provider is active");
            handler->enabled = 1;
            p4_lua_semantic_use(handler->mod);
        } else {
            p4_game_offense_policy_disable(handler);
            handler->enabled = 0;
        }
        return 0;
    }
    handler->enabled = lua_toboolean(L, lua_upvalueindex(1));
    return 0;
}
static int remove_stage(lua_State *L)
{
    p4_lua_require_mutation(L);
    LuaStage *handler = p4_lua_userdata(L, 1, P4_LUA_TAG_STAGE, "stage modifier");
    if (handler->mod != p4_luau_user(L)) luaL_error(L, "foreign %s modifier", stage_names[handler->stage]);
    if (!handler->handle) return 0;
    if (handler->offense) p4_game_offense_policy_disable(handler);
    else if (stage_unregister(handler) != P4_DAMAGE_OK)
        luaL_error(L, "%s modifier removal refused while a hit is composed", stage_names[handler->stage]);
    forget(handler);
    if (handler->callback != LUA_NOREF) lua_unref(L, handler->callback);
    lua_unref(L, handler->self);
    handler->callback = handler->self = LUA_NOREF;
    return 0;
}
static int add_stage(lua_State *L)
{
    const StageId stage = (StageId)lua_tointeger(L, lua_upvalueindex(1));
    const char *name = stage_names[stage];
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    if (!p4_lua_frame(L)->startup) luaL_error(L, "p4.%s.on is a start-up declaration, refused inside callbacks", name);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "kind");
    const char *kind_name = lua_type(L, -1) == LUA_TSTRING ? lua_tostring(L, -1) : "";
    int multiply;
    if (!strcmp(kind_name, "multiply")) multiply = 1;
    else if (!strcmp(kind_name, "add")) multiply = 0;
    else { luaL_error(L, "p4.%s.on: kind must be \"multiply\" or \"add\"", name); }
    lua_pop(L, 1);
    lua_getfield(L, 1, "priority");
    const int32_t priority = lua_isnil(L, -1) ? 0 : (int32_t)p4_lua_integer(L, -1, INT32_MIN, INT32_MAX);
    lua_pop(L, 1);
    lua_getfield(L, 1, "fn");
    if (lua_type(L, -1) != LUA_TFUNCTION) luaL_error(L, "p4.%s.on: fn must be a function", name);
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaStage *handler = lua_newuserdatataggedwithmetatable(L, sizeof *handler, P4_LUA_TAG_STAGE);
    *handler = (LuaStage){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF, .enabled = 1, .multiply = multiply,
        .stage = stage};
    const int result = stage_register(handler, mod->info->id, priority, (uint32_t)mod->owner);
    if (result != P4_DAMAGE_OK) {
        handler->handle = 0;
        luaL_error(L, "%s modifier refused (%s)", name, result == P4_DAMAGE_BUSY ? "a hit is being composed" :
            result == P4_DAMAGE_NOMEM ? "out of memory" : "invalid owner or kind");
    }
    handler->callback = lua_ref(L, -2);
    handler->self = lua_ref(L, -1);
    handler->next = registered;
    registered = handler;
    ++mod->subscriptions;
    return 1;
}

/* A declarative policy is initially disabled. Ownership is acquired only on
 * enable, so dormant mods do not block one another; conflicts fail rather than
 * silently changing the active run's rules. */
static float policy_number(lua_State *L, const char *field)
{
    lua_getfield(L, 1, field);
    if (lua_type(L, -1) != LUA_TNUMBER) luaL_error(L, "offense policy: %s must be a number", field);
    const double value = lua_tonumber(L, -1);
    const float result = (float)value;
    if (!isfinite(value) || !isfinite(result) || result <= 0)
        luaL_error(L, "offense policy: %s must be finite and positive", field);
    lua_pop(L, 1);
    return result;
}
static int add_offense_policy(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    if (!p4_lua_frame(L)->startup) luaL_error(L, "p4.damage.offense_policy is a start-up declaration");
    luaL_checktype(L, 1, LUA_TTABLE);
    P4GameOffensePolicy policy = {policy_number(L, "boost"), policy_number(L, "amp"), policy_number(L, "cap")};
    if (mod->subscriptions >= 128) luaL_error(L, "mod lifetime subscription limit (128) exceeded");
    LuaStage *handler = lua_newuserdatataggedwithmetatable(L, sizeof *handler, P4_LUA_TAG_STAGE);
    *handler = (LuaStage){.mod = mod, .callback = LUA_NOREF, .self = LUA_NOREF,
        .stage = STAGE_DAMAGE, .handle = 1, .offense = 1, .policy = policy};
    handler->self = lua_ref(L, -1);
    handler->next = registered;
    registered = handler;
    ++mod->subscriptions;
    return 1;
}

void p4_lua_stages_initialize(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_pushcclosure(L, enable_stage, "p4.stage.enable", 1); lua_setfield(L, -2, "enable");
    lua_pushboolean(L, 0); lua_pushcclosure(L, enable_stage, "p4.stage.disable", 1); lua_setfield(L, -2, "disable");
    lua_pushcfunction(L, remove_stage, "p4.stage.remove"); lua_setfield(L, -2, "remove");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 stage modifier"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_STAGE);
    static const char *const debug_names[STAGE_COUNT] = {"p4.damage.on", "p4.recovery.on", "p4.cost.on", "p4.chance.on",
        "p4.status.wearoff.on", "p4.buff.on", "p4.reward.on", "p4.unit.on", "p4.hit.on"};
    for (int stage = 0; stage < STAGE_COUNT; ++stage) {
        lua_newtable(L);
        lua_pushinteger(L, stage);
        lua_pushcclosure(L, add_stage, debug_names[stage], 1);
        lua_setfield(L, -2, "on");
        if (stage == STAGE_DAMAGE) {
            lua_pushcfunction(L, add_offense_policy, "p4.damage.offense_policy");
            lua_setfield(L, -2, "offense_policy");
        }
        lua_setreadonly(L, -1, 1);
        if (stage == STAGE_WEAROFF) {
            /* Share p4.status with any other cooperative initializer. */
            lua_newtable(L);
            lua_insert(L, -2);
            lua_setfield(L, -2, "wearoff");
            p4_lua_namespace_merge(L, "status");
        } else {
            lua_setfield(L, -2, stage_names[stage]);
        }
    }
}
/* VM teardown and session end: outside callbacks, so never during a hit. */
void p4_lua_stages_close(P4LuaMod *mod)
{
    for (LuaStage **link = &registered; *link;) {
        LuaStage *handler = *link;
        if (handler->mod != mod) { link = &handler->next; continue; }
        if (handler->offense) p4_game_offense_policy_disable(handler);
        else stage_unregister(handler);
        *link = handler->next;
        handler->next = NULL;
        handler->handle = 0;
    }
}
