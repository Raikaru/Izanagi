/* p4.status (capability status@1, hardmode-design §4.F): the battle-scoped side-ailment
 * service (include/p4pc/game_side_ailment.h) for Luau. query, landings, key
 * and protected are read-only copies, legal in every callback. enable,
 * protect, set and clear are mutations (p4_lua_require_mutation) and record
 * semantic use. enable and protect holds are owned per mod: the native gate is
 * on while any mod holds it, a unit is protected while any mod holds it, and a
 * mod's holds are released at its VM teardown (a protection also ends with its
 * battle, which the service clears itself). Refusals return false and a reason;
 * misuse (an unknown key, a unit that is not a live battle DatUnit) raises. */
#include "native_bindings.h"
#include "p4pc/game_side_ailment.h"
#include "p4pc/gmem.h"
#include "p4pc/mod_events.h"

enum { GATE_HOLDERS = 32, PROTECT_HOLDS = 64 };
typedef struct ProtectHold { uint32_t unit; const P4LuaMod *mod; } ProtectHold;
static const P4LuaMod *gate_holders[GATE_HOLDERS];
static ProtectHold protects[PROTECT_HOLDS];

#define BATTLE_STATE 0xb3ac /* gp slot of the live battle state (iGpffffb3ac) */
#define STATE_ACTIONS 0x174 /* BtlBattleState.actions */
#define ACTION_NEXT 0x450   /* BtlAction.next */
#define ACTION_UNIT 0x30    /* BtlAction.unit */
#define UNIT_DATUNIT 0xa64  /* BtlUnit.datUnit */
#define ACTION_LIMIT 64u

/* A live battle DatUnit: one the battle's action list reaches. */
static int live_unit(uint32_t unit)
{
    const uint32_t state = P4_GPDATA(uint32_t, BATTLE_STATE);
    if (!unit || !state) return 0;
    uint32_t action = P4_GDATA(uint32_t, state + STATE_ACTIONS);
    for (unsigned n = 0; action && n < ACTION_LIMIT; ++n, action = P4_GDATA(uint32_t, action + ACTION_NEXT)) {
        const uint32_t battle_unit = P4_GDATA(uint32_t, action + ACTION_UNIT);
        if (battle_unit && P4_GDATA(uint32_t, battle_unit + UNIT_DATUNIT) == unit) return 1;
    }
    return 0;
}
static uint32_t check_unit(lua_State *L, int index)
{
    const uint32_t unit = (uint32_t)p4_lua_integer(L, index, 0, UINT32_MAX);
    if (!live_unit(unit)) luaL_argerror(L, index, "not a live battle DatUnit");
    return unit;
}
static const P4SideAilmentDef *check_key(lua_State *L, int index)
{
    const P4SideAilmentDef *def = p4_side_ailment_find_key(luaL_checkstring(L, index));
    if (!def) luaL_argerror(L, index, "not a published side-ailment key");
    return def;
}
static int refused(lua_State *L, const char *reason)
{
    lua_pushboolean(L, 0);
    lua_pushstring(L, reason);
    return 2;
}
static int accepted(lua_State *L)
{
    p4_lua_semantic_use(p4_luau_user(L));
    lua_pushboolean(L, 1);
    return 1;
}
static const char *unit_refusal(int result)
{
    switch (result) {
    case P4_TABLE_NOT_READY: return "side ailments are not active here (no battle, gate off, or inside a status event)";
    case P4_TABLE_INVALID: return "the unit is dead or protected";
    case P4_TABLE_CONFLICT: return "the unit already has an ailment";
    default: return p4_game_tables_error(result);
    }
}

static int gate_count(void)
{
    int count = 0;
    for (size_t i = 0; i < GATE_HOLDERS; ++i) count += gate_holders[i] != NULL;
    return count;
}
static int status_enable(lua_State *L)
{
    p4_lua_require_mutation(L);
    const P4LuaMod *mod = p4_luau_user(L);
    luaL_checktype(L, 1, LUA_TBOOLEAN);
    const int on = lua_toboolean(L, 1);
    size_t held = GATE_HOLDERS, free_slot = GATE_HOLDERS;
    for (size_t i = 0; i < GATE_HOLDERS; ++i) {
        if (gate_holders[i] == mod) held = i;
        else if (!gate_holders[i] && free_slot == GATE_HOLDERS) free_slot = i;
    }
    if (on == (held != GATE_HOLDERS)) { lua_pushboolean(L, 1); return 1; }
    if (on && free_slot == GATE_HOLDERS) luaL_error(L, "p4.status.enable holder limit (%d) exceeded", GATE_HOLDERS);
    /* Only the first hold turns the gate on and only the last release off. */
    if (gate_count() == !on && p4_side_ailment_set_enabled(on))
        return refused(L, "the side-ailment gate changes only outside battle");
    if (on) gate_holders[free_slot] = mod;
    else gate_holders[held] = NULL;
    return accepted(L);
}

/* The service ends protection at unit exit and battle begin/end: a hold on a
 * unit it no longer protects is stale. */
static void prune_protects(void)
{
    for (size_t i = 0; i < PROTECT_HOLDS; ++i)
        if (protects[i].mod && !p4_side_ailment_protected(protects[i].unit)) protects[i] = (ProtectHold){0};
}
static int protect_holders(uint32_t unit)
{
    int count = 0;
    for (size_t i = 0; i < PROTECT_HOLDS; ++i) count += protects[i].mod && protects[i].unit == unit;
    return count;
}
static int status_protect(lua_State *L)
{
    p4_lua_require_mutation(L);
    const P4LuaMod *mod = p4_luau_user(L);
    const uint32_t unit = check_unit(L, 1);
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    const int on = lua_toboolean(L, 2);
    prune_protects();
    size_t held = PROTECT_HOLDS, free_slot = PROTECT_HOLDS;
    for (size_t i = 0; i < PROTECT_HOLDS; ++i) {
        if (protects[i].mod == mod && protects[i].unit == unit) held = i;
        else if (!protects[i].mod && free_slot == PROTECT_HOLDS) free_slot = i;
    }
    if (on == (held != PROTECT_HOLDS)) { lua_pushboolean(L, 1); return 1; }
    if (on && free_slot == PROTECT_HOLDS) luaL_error(L, "p4.status.protect hold limit (%d) exceeded", PROTECT_HOLDS);
    if (protect_holders(unit) == !on && p4_side_ailment_protect(unit, on))
        return refused(L, "protection needs a battle with side ailments enabled");
    if (on) protects[free_slot] = (ProtectHold){unit, mod};
    else protects[held] = (ProtectHold){0};
    return accepted(L);
}
static int status_protected(lua_State *L)
{
    lua_pushboolean(L, p4_side_ailment_protected((uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX)));
    return 1;
}
static int status_query(lua_State *L)
{
    const P4SideAilmentDef *def = NULL;
    if (p4_side_ailment_query((uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX), &def)) lua_pushstring(L, def->key);
    else lua_pushnil(L);
    return 1;
}
static int status_landings(lua_State *L)
{
    const uint32_t unit = (uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX);
    lua_pushunsigned(L, p4_side_ailment_landings(unit, check_key(L, 2)->leaf));
    return 1;
}
static int status_key(lua_State *L)
{
    const P4SideAilmentDef *def = p4_side_ailment_find_leaf((uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX));
    if (def) lua_pushstring(L, def->key);
    else lua_pushnil(L);
    return 1;
}
static int status_set(lua_State *L)
{
    p4_lua_require_mutation(L);
    const uint32_t unit = check_unit(L, 1);
    const int result = p4_side_ailment_set(unit, check_key(L, 2)->leaf, P4_SIDE_REASON_MOD);
    if (result) return refused(L, unit_refusal(result));
    return accepted(L);
}
static int status_clear(lua_State *L)
{
    p4_lua_require_mutation(L);
    const uint32_t leaf = p4_side_ailment_clear(check_unit(L, 1), P4_SIDE_REASON_MOD);
    const P4SideAilmentDef *def = leaf == P4_SIDE_DEFAULT_NONE ? NULL : p4_side_ailment_find_leaf(leaf);
    if (!def) { lua_pushnil(L); return 1; }
    p4_lua_semantic_use(p4_luau_user(L));
    lua_pushstring(L, def->key);
    return 1;
}

/* The "status" event's source: the side-ailment observer while a mod listens. */
static void status_observed(uint32_t unit, uint32_t leaf, uint32_t kind, uint32_t reason)
{
    p4_mod_event_status(unit, leaf, kind, reason);
}
static void status_source(int on) { p4_side_ailment_set_observer(on ? status_observed : NULL); }
void p4_lua_status_install(void) { p4_mod_events_set_status_source(status_source); }

void p4_lua_status_initialize(lua_State *L)
{
    static const luaL_Reg methods[] = {
        {"enable", status_enable}, {"protect", status_protect}, {"protected", status_protected},
        {"query", status_query}, {"landings", status_landings}, {"key", status_key},
        {"set", status_set}, {"clear", status_clear}, {NULL, NULL}};
    p4_lua_library(L, methods);
    p4_lua_namespace_merge(L, "status"); /* shared with p4.status.wearoff (lua_stages.c) */
}
void p4_lua_status_close(P4LuaMod *mod)
{
    for (size_t i = 0; i < GATE_HOLDERS; ++i) {
        if (gate_holders[i] != mod) continue;
        gate_holders[i] = NULL;
        /* Inside a battle the gate stays on until the session ends. */
        if (!gate_count()) (void)p4_side_ailment_set_enabled(0);
    }
    prune_protects();
    for (size_t i = 0; i < PROTECT_HOLDS; ++i) {
        if (protects[i].mod != mod) continue;
        const uint32_t unit = protects[i].unit;
        protects[i] = (ProtectHold){0};
        if (!protect_holders(unit)) (void)p4_side_ailment_protect(unit, 0);
    }
}
