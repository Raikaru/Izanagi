/* p4.passive (capability passive.grant@1, docs/modding/lua.md "Passive
 * grants"): a mod's grants in the native passive grant table
 * (include/p4pc/game_passive_grant.h). A grant reaches every passive check
 * the game makes about its target, exactly as the skill in a slot would; the
 * eight-slot skill list itself is unchanged. Grants and revocations are
 * mutations (refused in snapshot callbacks) and take effect at the game's
 * next check; queries are pure. Grants outlive loads and New Game and end
 * when revoked or with the mod's script. */
#include "native_bindings.h"
#include "p4pc/game_data.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_passive_grant.h"
#include "p4pc/game_skills_native.h"

static const char *const scope_names[P4_PASSIVE_SCOPES] = {"party", "persona", "item", "enemy"};
#define TARGET_FORMS "a target is {party = ...}, {persona = ...}, {item = ...} or {enemy = ...}"

/* An ID, or a p4.data row (its `id`). */
static uint32_t row_id(lua_State *L, int index, const char *what, uint32_t high)
{
    uint32_t id;
    if (lua_type(L, index) == LUA_TTABLE) {
        lua_getfield(L, index, "id");
        if (lua_type(L, -1) != LUA_TNUMBER) luaL_error(L, "p4.passive: %s must be an ID or a p4.data row", what);
        id = (uint32_t)p4_lua_integer(L, -1, 0, high);
        lua_pop(L, 1);
        return id;
    }
    if (lua_type(L, index) != LUA_TNUMBER) luaL_error(L, "p4.passive: %s must be an ID or a p4.data row", what);
    return (uint32_t)p4_lua_integer(L, index, 0, high);
}
/* A new grant's Persona, item or enemy must be a row of the semantic catalog
 * once it is loaded (before that, only the ID ranges are known). Revocations
 * and queries take any in-range target, so a grant can always be taken back. */
static void catalog_target(lua_State *L, uint32_t scope, uint32_t key)
{
    const char *reason = NULL;
    if (scope == P4_PASSIVE_PARTY || p4_data_catalog_ready(&reason)) return;
    const P4DataKind kind = scope == P4_PASSIVE_PERSONA ? P4_DATA_PERSONA :
        scope == P4_PASSIVE_ITEM ? P4_DATA_ITEM : P4_DATA_ENEMY;
    size_t index;
    if (p4_data_catalog_index(kind, key, &index))
        luaL_error(L, "p4.passive: no %s of the loaded tables has the ID %u", scope_names[scope], (unsigned)key);
}
static void target_argument(lua_State *L, int index, uint32_t *scope, uint32_t *key)
{
    if (lua_type(L, index) != LUA_TTABLE) luaL_error(L, "p4.passive: " TARGET_FORMS);
    int found = 0;
    for (uint32_t candidate = 0; candidate < P4_PASSIVE_SCOPES; ++candidate) {
        lua_getfield(L, index, scope_names[candidate]);
        if (lua_isnil(L, -1)) { lua_pop(L, 1); continue; }
        if (found++) luaL_error(L, "p4.passive: a target names exactly one of party, persona, item or enemy");
        *scope = candidate;
        if (candidate == P4_PASSIVE_PARTY && lua_type(L, -1) == LUA_TSTRING) {
            if (p4_data_character_parse(lua_tostring(L, -1), key))
                luaL_error(L, "p4.passive: unknown party member \"%s\" (protagonist, yosuke, chie, yukiko, rise, "
                    "kanji, naoto or teddie)", lua_tostring(L, -1));
        } else if (candidate == P4_PASSIVE_PARTY) {
            *key = (uint32_t)p4_lua_integer(L, -1, 1, P4_PASSIVE_PARTY_MAX);
        } else {
            *key = row_id(L, lua_gettop(L), scope_names[candidate],
                candidate == P4_PASSIVE_ITEM ? P4_PASSIVE_ITEM_MAX : P4_PASSIVE_KEY_MAX);
            if (candidate == P4_PASSIVE_ENEMY && *key >= p4_game_enemies_native_limit())
                luaL_error(L, "p4.passive: no enemy has the ID %u", (unsigned)*key);
        }
        lua_pop(L, 1);
    }
    if (!found) luaL_error(L, "p4.passive: " TARGET_FORMS);
}
/* A passive skill of the loaded tables: registered and not an active skill. */
static uint32_t skill_argument(lua_State *L, int index)
{
    const uint32_t skill = row_id(L, index, "skill", P4_PASSIVE_SKILL_MAX);
    if (!skill || !p4_game_skill_registered(skill) || p4_game_skill_active(skill))
        luaL_error(L, "p4.passive: skill %u is not a passive skill", (unsigned)skill);
    return skill;
}
static void push_target(lua_State *L, uint32_t scope, uint32_t key)
{
    lua_createtable(L, 0, 1);
    if (scope == P4_PASSIVE_PARTY && p4_data_character_name(key)) lua_pushstring(L, p4_data_character_name(key));
    else lua_pushinteger(L, (int)key);
    lua_setfield(L, -2, scope_names[scope]);
}

static int passive_grant(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    uint32_t scope = 0, key = 0;
    target_argument(L, 1, &scope, &key);
    catalog_target(L, scope, key);
    const uint32_t skill = skill_argument(L, 2);
    const int result = p4_game_passive_grant(mod->info->id, (uint32_t)mod->owner, scope, key, skill);
    /* A full table refuses only a new tuple; nothing already held changes. */
    if (result == P4_PASSIVE_FULL)
        luaL_error(L, "p4.passive.grant: the grant table is full (%u grants held by all mods together)",
            (unsigned)P4_PASSIVE_GRANT_MAX);
    if (result < 0) luaL_error(L, "p4.passive.grant: refused (invalid target, skill or mod ID)");
    lua_pushboolean(L, result == 1);
    return 1;
}
static int passive_revoke(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    uint32_t scope = 0, key = 0;
    target_argument(L, 1, &scope, &key);
    const uint32_t skill = skill_argument(L, 2);
    lua_pushboolean(L, p4_game_passive_revoke((uint32_t)mod->owner, scope, key, skill) == 1);
    return 1;
}
static int passive_clear(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    lua_pushinteger(L, (int)p4_game_passive_revoke_owner((uint32_t)mod->owner));
    return 1;
}
static int passive_granted(lua_State *L)
{
    uint32_t scope = 0, key = 0;
    target_argument(L, 1, &scope, &key);
    const uint32_t skill = skill_argument(L, 2);
    lua_pushboolean(L, p4_game_passive_held(scope, key, skill));
    return 1;
}
/* Every held grant, in (target kind, target, skill, mod load order) order. */
static int passive_list(lua_State *L)
{
    const size_t count = p4_game_passive_count();
    lua_createtable(L, (int)count, 0);
    for (size_t at = 0; at < count; ++at) {
        P4PassiveGrant grant;
        if (p4_game_passive_at(at, &grant)) break;
        lua_createtable(L, 0, 5);
        push_target(L, grant.scope, grant.key);
        lua_setfield(L, -2, "target");
        lua_pushstring(L, scope_names[grant.scope]);
        lua_setfield(L, -2, "kind");
        lua_pushinteger(L, (int)grant.key);
        lua_setfield(L, -2, "id");
        lua_pushinteger(L, (int)grant.skill);
        lua_setfield(L, -2, "skill");
        lua_pushstring(L, grant.owner);
        lua_setfield(L, -2, "owner");
        lua_rawseti(L, -2, (int)at + 1);
    }
    return 1;
}

void p4_lua_passive_initialize(lua_State *L)
{
    static const luaL_Reg methods[] = {{"grant", passive_grant}, {"revoke", passive_revoke},
        {"clear", passive_clear}, {"granted", passive_granted}, {"list", passive_list}, {NULL, NULL}};
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "passive");
}
/* VM teardown: outside callbacks, so never inside a passive check. */
void p4_lua_passive_close(P4LuaMod *mod)
{
    p4_game_passive_revoke_owner((uint32_t)mod->owner);
}
