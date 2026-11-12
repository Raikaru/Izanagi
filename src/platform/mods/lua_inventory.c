/* p4.inventory, p4.give, p4.take and p4.party (capabilities inventory@1,
 * give.item@1, give.money@1, party.persona@1, party.hero_level@1, give.skill@1, give.exp@1,
 * give.skill.choice@1) over include/p4pc/mod_inventory.h and mod_party.h. An
 * item or skill is an ID, a p4.data row (anything with a numeric id) or a
 * name/key the data catalog resolves; an unknown or ambiguous name raises the
 * catalog's own message. A party member is 1-8, a character name ("Yosuke",
 * "protagonist") or a unit table with a character field. */
#include "native_bindings.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/mod_inventory.h"
#include "p4pc/mod_party.h"
#include <string.h>

static uint32_t catalog_argument(lua_State *L, int index, const char *function, P4DataKind kind)
{
    const char *what = p4_data_kind_name(kind);
    if (lua_type(L, index) == LUA_TNUMBER) return (uint32_t)p4_lua_integer(L, index, 0, INT16_MAX);
    if (lua_istable(L, index)) {
        lua_getfield(L, index, "id");
        if (lua_type(L, -1) != LUA_TNUMBER) luaL_error(L, "%s: the %s table has no numeric id", function, what);
        const uint32_t id = (uint32_t)p4_lua_integer(L, -1, 0, INT16_MAX);
        lua_pop(L, 1);
        return id;
    }
    const char *query = luaL_checkstring(L, index);
    const char *why = NULL;
    if (p4_data_catalog_ready(&why)) luaL_error(L, "%s: %s names are unavailable: %s", function, what, why);
    P4DataRows rows;
    size_t row;
    char message[512];
    if (p4_data_catalog_rows(kind, &rows)) luaL_error(L, "%s: the %s table is not loaded", function, what);
    if (p4_data_lookup(&rows, query, &row, message, sizeof message)) luaL_error(L, "%s: %s", function, message);
    return p4_data_catalog_id(kind, row);
}
static uint32_t item_argument(lua_State *L, int index, const char *function)
{
    return catalog_argument(L, index, function, P4_DATA_ITEM);
}
static uint32_t member_argument(lua_State *L, int index, const char *function)
{
    if (lua_type(L, index) == LUA_TNUMBER) return (uint32_t)p4_lua_integer(L, index, 1, P4_MOD_PARTY_MEMBERS);
    if (lua_istable(L, index)) lua_getfield(L, index, "character");
    else lua_pushvalue(L, index);
    uint32_t member;
    const char *name = lua_tostring(L, -1);
    if (!name || p4_data_character_parse(name, &member))
        luaL_error(L, "%s: \"%s\" is not a party member; use protagonist, yosuke, chie, yukiko, rise, kanji, naoto, "
                   "teddie or 1-8", function, name ? name : luaL_typename(L, index));
    lua_pop(L, 1);
    return member;
}
static void require_action(lua_State *L, const char *function)
{
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "%s is refused in synchronous UI/action callbacks, decisions and save snapshots", function);
}
/* (applied change), (requested change, "queued ...") or (0, refusal reason). */
static int push_change(lua_State *L, int result, int32_t changed, const char *reason, P4LuaMod *mod)
{
    if (result < 0) {
        lua_pushinteger(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    if (changed) p4_lua_semantic_use(mod); /* the game's state changes: save provenance */
    lua_pushinteger(L, changed < 0 ? -changed : changed);
    if (!result) return 1;
    lua_pushstring(L, reason);
    return 2;
}
static int inventory_count(lua_State *L)
{
    const uint32_t item = item_argument(L, 1, "p4.inventory.count");
    uint32_t count;
    const char *reason;
    if (p4_mod_item_count(item, &count, &reason)) luaL_error(L, "p4.inventory.count: %s", reason);
    lua_pushinteger(L, (int)count);
    return 1;
}
static int inventory_money(lua_State *L)
{
    lua_pushunsigned(L, p4_mod_money());
    return 1;
}
static int change_item(lua_State *L, int sign, const char *function)
{
    require_action(L, function);
    const uint32_t item = item_argument(L, 1, function);
    const int32_t count = lua_isnoneornil(L, 2) ? 1 : (int32_t)p4_lua_integer(L, 2, 1, P4_MOD_ITEM_STACK);
    int32_t changed;
    const char *reason = NULL;
    const int result = p4_mod_item_add(item, sign * count, &changed, &reason);
    return push_change(L, result, changed, reason, p4_luau_user(L));
}
static int give_item(lua_State *L) { return change_item(L, 1, "p4.give.item"); }
static int take_item(lua_State *L) { return change_item(L, -1, "p4.take.item"); }
static int change_money(lua_State *L, int sign, const char *function)
{
    require_action(L, function);
    const int32_t amount = (int32_t)p4_lua_integer(L, 1, 1, P4_MOD_MONEY_MAX);
    int32_t changed;
    const char *reason = NULL;
    const int result = p4_mod_money_add(sign * amount, &changed, &reason);
    return push_change(L, result, changed, reason, p4_luau_user(L));
}
static int give_money(lua_State *L) { return change_money(L, 1, "p4.give.money"); }
static int take_money(lua_State *L) { return change_money(L, -1, "p4.take.money"); }

/* p4.party.persona(who) -> { id, key?, name?, level, exp, skills } or nil, reason */
static int party_persona(lua_State *L)
{
    const uint32_t member = member_argument(L, 1, "p4.party.persona");
    P4ModPersona persona;
    const char *reason = NULL;
    if (p4_mod_party_persona(member, &persona, &reason)) {
        lua_pushnil(L);
        lua_pushstring(L, reason);
        return 2;
    }
    lua_createtable(L, 0, 6);
    lua_pushinteger(L, persona.id); lua_setfield(L, -2, "id");
    P4DataRef ref;
    if (!p4_data_ref(P4_DATA_PERSONA, persona.id, &ref)) {
        lua_pushstring(L, ref.key); lua_setfield(L, -2, "key");
        if (ref.name[0]) { lua_pushstring(L, ref.name); lua_setfield(L, -2, "name"); }
    }
    lua_pushinteger(L, persona.level); lua_setfield(L, -2, "level");
    lua_pushunsigned(L, persona.exp); lua_setfield(L, -2, "exp");
    lua_createtable(L, P4_MOD_PERSONA_SKILLS, 0);
    int n = 0;
    for (uint32_t i = 0; i < P4_MOD_PERSONA_SKILLS; ++i)
        if (persona.skills[i]) { lua_pushinteger(L, persona.skills[i]); lua_rawseti(L, -2, ++n); }
    lua_setfield(L, -2, "skills");
    return 1;
}
/* p4.party.level(who) -> level, or nil and a reason: the protagonist's own; a member's Persona's. */
static int party_level(lua_State *L)
{
    const uint32_t member = member_argument(L, 1, "p4.party.level");
    const char *reason = NULL;
    uint8_t level = 0;
    uint32_t exp;
    P4ModPersona persona;
    const int result = member == 1 ? p4_mod_party_hero(&level, &exp, &reason) : p4_mod_party_persona(member, &persona, &reason);
    if (result) {
        lua_pushnil(L);
        lua_pushstring(L, reason);
        return 2;
    }
    lua_pushinteger(L, member == 1 ? level : persona.level);
    return 1;
}
/* p4.party.hero_level() -> level, or nil and a reason: the protagonist's stored level. */
static int party_hero_level(lua_State *L)
{
    const char *reason = NULL;
    uint8_t level = 0;
    if (p4_mod_party_hero_level(&level, &reason)) {
        lua_pushnil(L);
        lua_pushstring(L, reason);
        return 2;
    }
    lua_pushinteger(L, level);
    return 1;
}
/* (true), (true, "queued ...") or (false, refusal reason). */
static int push_party(lua_State *L, int result, const char *reason)
{
    lua_pushboolean(L, result >= 0);
    if (result >= 0) p4_lua_semantic_use(p4_luau_user(L)); /* the game's state changes: save provenance */
    if (!result) return 1;
    lua_pushstring(L, reason);
    return 2;
}
/* give.skill.choice@1: the done callbacks of choices still waiting for their outcome. A slot
 * lives from the request to its delivery, or until its mod closes (p4_lua_inventory_close). */
typedef struct ChoiceCallback {
    P4LuaMod *mod;
    int callback;
} ChoiceCallback;
static ChoiceCallback choice_callbacks[P4_MOD_SKILL_CHOICES];
typedef struct ChoiceInvocation {
    int callback;
    const P4ModSkillChoice *outcome;
} ChoiceInvocation;

static int invoke_choice(lua_State *L)
{
    const ChoiceInvocation *invocation = lua_touserdata(L, 1);
    const P4ModSkillChoice *outcome = invocation->outcome;
    lua_getref(L, invocation->callback);
    lua_createtable(L, 0, 5);
    lua_pushinteger(L, (int)outcome->member); lua_setfield(L, -2, "member");
    lua_pushinteger(L, (int)outcome->skill); lua_setfield(L, -2, "skill");
    lua_pushboolean(L, outcome->learned); lua_setfield(L, -2, "learned");
    if (outcome->forgot) { lua_pushinteger(L, outcome->forgot); lua_setfield(L, -2, "forgot"); }
    if (outcome->reason) { lua_pushstring(L, outcome->reason); lua_setfield(L, -2, "reason"); }
    lua_setreadonly(L, -1, 1);
    lua_call(L, 1, 0);
    return 0;
}
/* game_party.c, at the services frame boundary: the outcome, once, to the requesting mod. */
static void deliver_choice(const P4ModSkillChoice *outcome, void *user)
{
    ChoiceCallback *slot = user;
    P4LuaMod *mod = slot->mod;
    ChoiceInvocation invocation = {slot->callback, outcome};
    *slot = (ChoiceCallback){NULL, LUA_NOREF};
    const int status = p4_lua_invoke(mod, invoke_choice, &invocation, 0, P4_LUA_SEMANTIC);
    lua_unref(mod->state, invocation.callback);
    if (status) p4_mod_failure(mod->owner, p4_lua_error(mod));
}

/* p4.give.skill(who, skill, { replace = skill }?) or, give.skill.choice@1,
 * p4.give.skill(who, skill, { choice = true, done = function(outcome) end }) */
static int give_skill(lua_State *L)
{
    require_action(L, "p4.give.skill");
    const uint32_t member = member_argument(L, 1, "p4.give.skill");
    const uint32_t skill = catalog_argument(L, 2, "p4.give.skill", P4_DATA_SKILL);
    uint32_t replace = 0;
    int choice = 0, done = 0;
    if (!lua_isnoneornil(L, 3)) {
        luaL_checktype(L, 3, LUA_TTABLE);
        lua_getfield(L, 3, "replace");
        if (!lua_isnil(L, -1)) replace = catalog_argument(L, lua_gettop(L), "p4.give.skill", P4_DATA_SKILL);
        lua_getfield(L, 3, "choice");
        if (!lua_isnil(L, -1) && !lua_isboolean(L, -1)) luaL_error(L, "p4.give.skill: choice must be true or false");
        choice = lua_toboolean(L, -1);
        lua_getfield(L, 3, "done");
        done = !lua_isnil(L, -1);
        if (done && !lua_isfunction(L, -1)) luaL_error(L, "p4.give.skill: done must be a function");
        if (choice && replace) luaL_error(L, "p4.give.skill: replace and choice cannot be combined");
        if (done && !choice) luaL_error(L, "p4.give.skill: done needs choice = true");
    }
    const char *reason = NULL;
    P4LuaMod *mod = p4_luau_user(L);
    if (!choice) return push_party(L, p4_mod_skill_give(member, skill, replace, mod->info->id, &reason), reason);
    ChoiceCallback *slot = NULL;
    for (size_t i = 0; done && !slot && i < P4_MOD_SKILL_CHOICES; ++i)
        if (!choice_callbacks[i].mod) slot = &choice_callbacks[i];
    if (done && !slot) return push_party(L, -1, "too many skill choices are waiting");
    if (slot) *slot = (ChoiceCallback){mod, lua_ref(L, -1)}; /* the done function, on top */
    const int result = p4_mod_skill_choose(member, skill, mod->owner, mod->info->id, slot ? deliver_choice : NULL, slot,
                                           &reason);
    if (result < 0 && slot) {
        lua_unref(L, slot->callback);
        *slot = (ChoiceCallback){NULL, LUA_NOREF};
    }
    return push_party(L, result, reason);
}
/* p4.give.exp(who, amount, { persona = true }?): the protagonist's own level unless persona is set. */
static int give_exp(lua_State *L)
{
    require_action(L, "p4.give.exp");
    const uint32_t member = member_argument(L, 1, "p4.give.exp");
    const uint32_t amount = (uint32_t)p4_lua_integer(L, 2, 1, P4_MOD_EXP_MAX);
    int persona = 0;
    if (!lua_isnoneornil(L, 3)) {
        luaL_checktype(L, 3, LUA_TTABLE);
        lua_getfield(L, 3, "persona");
        persona = lua_toboolean(L, -1);
    }
    const char *reason = NULL;
    const P4LuaMod *mod = p4_luau_user(L);
    return push_party(L, p4_mod_exp_give(member, member == 1 && !persona, amount, mod->info->id, &reason), reason);
}

void p4_lua_inventory_initialize(lua_State *L)
{
    static const luaL_Reg inventory[] = {{"count", inventory_count}, {"money", inventory_money}, {NULL, NULL}};
    static const luaL_Reg give[] = {{"item", give_item}, {"money", give_money}, {"skill", give_skill}, {"exp", give_exp},
                                    {NULL, NULL}};
    static const luaL_Reg take[] = {{"item", take_item}, {"money", take_money}, {NULL, NULL}};
    p4_lua_library(L, inventory); lua_setfield(L, -2, "inventory");
    p4_lua_library(L, give); lua_setfield(L, -2, "give");
    p4_lua_library(L, take); lua_setfield(L, -2, "take");
    /* p4.party may already hold other party services: one read-only table with theirs and these. */
    lua_newtable(L);
    lua_getfield(L, -2, "party");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            lua_pushvalue(L, -2);
            lua_insert(L, -2);
            lua_settable(L, -5);
        }
    }
    lua_pop(L, 1);
    lua_pushcfunction(L, party_persona, "p4.party.persona");
    lua_setfield(L, -2, "persona");
    lua_pushcfunction(L, party_level, "p4.party.level");
    lua_setfield(L, -2, "level");
    lua_pushcfunction(L, party_hero_level, "p4.party.hero_level");
    lua_setfield(L, -2, "hero_level");
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "party");
}

void p4_lua_inventory_close(P4LuaMod *mod)
{
    p4_mod_skill_choice_release(mod->owner);
    for (size_t i = 0; i < P4_MOD_SKILL_CHOICES; ++i) {
        if (choice_callbacks[i].mod != mod) continue;
        lua_unref(mod->state, choice_callbacks[i].callback);
        choice_callbacks[i] = (ChoiceCallback){NULL, LUA_NOREF};
    }
}
