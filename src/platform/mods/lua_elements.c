/* p4.elements: read-only queries over the active element registry (API 1.5).
 *
 * Game-only: registered with p4.ui through the native bindings, so the VM tests'
 * core never links the battle tables. Every answer comes from the frozen active
 * snapshot through the pure resolvers (no runtime_use row consumption, no guest
 * writes, no native calls), so the functions are legal in synchronous UI
 * callbacks. An unknown or out-of-range ID answers nil, never an error. */
#include "native_bindings.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_elements_native.h"
#include "p4pc/game_items.h"
#include "p4pc/game_skills.h"
#include "p4pc/game_tables.h"

/* The element family the retail game treats as Almighty. */
#define ALMIGHTY_FAMILY 5u

static int id_argument(lua_State *L, uint32_t *id)
{
    const double value = luaL_checknumber(L, 1);
    if (!(value >= 0.0 && value <= 4294967295.0) || value != (double)(uint32_t)value) return 0;
    *id = (uint32_t)value;
    return 1;
}
static const char *kind_name(const P4ElementDesc *desc)
{
    if (desc->flags & P4_ELEMENT_PHYSICAL) return "physical";
    if (desc->family == ALMIGHTY_FAMILY) return "almighty";
    if (desc->flags & P4_ELEMENT_OFFENSIVE) return "magic";
    return "status";
}
static int push_element(lua_State *L, const P4GameTables *tables, const P4GameActionElement *element)
{
    const P4ElementDesc *desc = element->valid ? p4_game_tables_find_element(tables, element->element_id) : NULL;
    if (!desc) {
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 4);
    lua_pushstring(L, desc->key); lua_setfield(L, -2, "key");
    lua_pushstring(L, desc->display_name); lua_setfield(L, -2, "name");
    lua_pushstring(L, desc->short_name); lua_setfield(L, -2, "short");
    lua_pushstring(L, kind_name(desc)); lua_setfield(L, -2, "kind");
    return 1;
}
static int elements_active(lua_State *L)
{
    lua_pushboolean(L, p4_game_elements_active() != NULL);
    return 1;
}
static int elements_revision(lua_State *L)
{
    lua_pushnumber(L, (double)p4_game_elements_active_generation());
    return 1;
}
static int elements_of_skill(lua_State *L)
{
    uint32_t id;
    const P4GameTables *tables = p4_game_elements_active();
    const P4GameSkills *skills = p4_game_elements_active_skills();
    P4GameSkillDefinition definition;
    if (!id_argument(L, &id) || !tables || !skills || p4_game_skills_definition_find(skills, id, &definition) ||
        !definition.active || !definition.metadata) {
        lua_pushnil(L);
        return 1;
    }
    /* A weapon-derived skill (Attack and its follow-ups) takes its user's
     * weapon element unless a provider bound the skill itself. */
    if ((definition.metadata[0] & 2u) &&
        !p4_game_tables_find_action_element(tables, P4_ACTION_ELEMENT_SKILL, id, P4_ACTION_COMPONENT_DAMAGE)) {
        lua_createtable(L, 0, 1);
        lua_pushboolean(L, 1); lua_setfield(L, -2, "weapon");
        return 1;
    }
    P4GameActionElement element;
    if (p4_game_elements_resolve_action(tables, skills, p4_game_elements_active_items(), NULL, id,
            P4_ACTION_COMPONENT_DAMAGE, 0, &element)) {
        lua_pushnil(L);
        return 1;
    }
    return push_element(L, tables, &element);
}
static int elements_of_weapon(lua_State *L)
{
    uint32_t id;
    const P4GameTables *tables = p4_game_elements_active();
    const P4GameItems *items = p4_game_elements_active_items();
    P4GameActionElement element;
    if (!id_argument(L, &id) || !tables || !items ||
        p4_game_elements_resolve_weapon(tables, items, id, P4_ACTION_COMPONENT_DAMAGE, &element)) {
        lua_pushnil(L);
        return 1;
    }
    return push_element(L, tables, &element);
}
void p4_lua_elements_initialize(lua_State *L)
{
    static const luaL_Reg methods[] = {
        {"active", elements_active}, {"revision", elements_revision},
        {"of_skill", elements_of_skill}, {"of_weapon", elements_of_weapon}, {NULL, NULL}
    };
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "elements");
}
