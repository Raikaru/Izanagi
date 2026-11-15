/* p4.battle: the controlled actor's own battle command panel (API 1.10).
 *
 * Game-only (registered with p4.ui through the native bindings). Reads are
 * legal anywhere; choose/point/aim are refused in synchronous UI callbacks and
 * save snapshots and act only through the game's own panel flow (battle_menu.c).
 * A wait has one driver: the first mod whose request is accepted. A mod's use
 * is recorded when the player confirms a panel it changed. substitute
 * (capability battle.substitute@1, lua_battle_substitute.c) acts only inside
 * a before-hook on 0x001DE640. */
#include "native_bindings.h"
#include "battle_menu.h"
#include "p4pc/game_items_native.h"
#include "p4pc/gmem.h"
#include "p4pc/packages.h"
#include <string.h>

#define BATTLE_STATE 0x0076449Cu /* the BtlBattleState pointer */
#define STATE_UNIT_LISTS 0x178u   /* head/tail pairs: party, then enemies */
#define UNIT_CHAR 0xA4u
#define UNIT_DATA 0xA64u
#define UNIT_NEXT 0xA6Cu
#define WORK_SKILLS 0x194u /* eight skill IDs, count at +0x1A4 */
#define WORK_SKILL_COUNT 0x1A4u
#define WORK_ITEM_COUNT 0x5A6u /* the Item panel's row count; the rows are where the tables put them */

static const char *const panels[] = {"", "commands", "skills", "items", "tactics", "persona"};
static const char *const commands[] = {"analyze", "tactics", "guard", "attack", "skill", "persona", "item", "escape", NULL};

static void require_native_frame(lua_State *L, const char *name)
{
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.battle.%s is refused in synchronous UI/action callbacks and save snapshots", name);
}
/* false plus the reason, or the driving mod's id when another mod drives the wait. */
static int refusal(lua_State *L, int result, const void *driver)
{
    lua_pushboolean(L, 0);
    if (result == -2 && driver) {
        const P4PackageProvider *provider = p4_mod_scripts_owner_provider(((const P4LuaMod *)driver)->owner);
        const P4PackageInfo *info = provider ? p4_package_provider_info(provider) : NULL;
        lua_pushstring(L, info ? info->id : "another mod");
    } else {
        lua_pushstring(L, "no matching battle panel waits for input");
    }
    return 2;
}
static int battle_menu(lua_State *L)
{
    P4BattleMenu menu;
    if (p4_battle_menu(&menu)) {
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 10);
    lua_pushstring(L, panels[menu.panel]); lua_setfield(L, -2, "panel");
    lua_pushinteger(L, menu.cursor + 1); lua_setfield(L, -2, "cursor");
    lua_pushinteger(L, menu.first + 1); lua_setfield(L, -2, "first");
    lua_pushinteger(L, menu.count); lua_setfield(L, -2, "count");
    lua_pushinteger(L, menu.page); lua_setfield(L, -2, "page");
    lua_pushunsigned(L, menu.unit); lua_setfield(L, -2, "unit");
    lua_pushunsigned(L, menu.action); lua_setfield(L, -2, "action");
    lua_pushinteger(L, P4_G16(menu.unit + UNIT_CHAR)); lua_setfield(L, -2, "id");
    if (menu.panel == P4_BATTLE_PANEL_COMMANDS && menu.cursor >= 0 && menu.cursor < P4_BATTLE_COMMANDS) {
        lua_pushstring(L, commands[menu.cursor]); lua_setfield(L, -2, "command");
    }
    /* The actor's skill rows are built with its turn, so the commands panel lists them too. */
    if (menu.panel == P4_BATTLE_PANEL_SKILLS || menu.panel == P4_BATTLE_PANEL_COMMANDS) {
        const int count = P4_G16(menu.work + WORK_SKILL_COUNT);
        lua_createtable(L, count > 0 && count <= 8 ? count : 0, 0);
        for (int i = 0; i < count && i < 8; ++i) {
            lua_pushinteger(L, P4_G16(menu.work + WORK_SKILLS + (uint32_t)i * 2u));
            lua_rawseti(L, -2, i + 1);
        }
        lua_setfield(L, -2, "skills");
    } else if (menu.panel == P4_BATTLE_PANEL_ITEMS) {
        /* Retail keeps the rows inside the panel work (+0x1A6). With items added by a
         * table mod, 00207140 builds them after the whole task work instead
         * (p4_game_tables_items_native_battle_refresh): up to 256 retail-range rows
         * plus one per added item. The count stays at +0x5A6 either way. */
        const P4GameItemBattleRow *rows =
            p4_game_tables_items_native_battle_rows((uint8_t *)P4_G2H(menu.work));
        const uint32_t total = p4_game_tables_items_native_row_count();
        const int capacity = 256 + (int)(total > P4_ITEM_RETAIL_COUNT ? total - P4_ITEM_RETAIL_COUNT : 0u);
        const int count = P4_G16(menu.work + WORK_ITEM_COUNT);
        const int shown = count > 0 && count <= capacity ? count : 0;
        lua_createtable(L, shown, 0);
        for (int i = 0; i < shown; ++i) {
            lua_pushinteger(L, rows[i].id);
            lua_rawseti(L, -2, i + 1);
        }
        lua_setfield(L, -2, "items");
    }
    return 1;
}
static int battle_choose(lua_State *L)
{
    const int command = luaL_checkoption(L, 1, NULL, commands);
    require_native_frame(L, "choose");
    const void *driver = NULL;
    const int result = p4_battle_menu_choose(p4_luau_user(L), command, &driver);
    if (result) return refusal(L, result, driver);
    lua_pushboolean(L, 1);
    return 1;
}
static int battle_point(lua_State *L)
{
    const int row = (int)p4_lua_integer(L, 1, 1, 256);
    require_native_frame(L, "point");
    const void *driver = NULL;
    const int result = p4_battle_menu_point(p4_luau_user(L), row - 1, &driver);
    if (result) return refusal(L, result, driver);
    lua_pushboolean(L, 1);
    return 1;
}
static int battle_aim(lua_State *L)
{
    const uint32_t unit = (uint32_t)p4_lua_integer(L, 1, 0, UINT32_MAX);
    require_native_frame(L, "aim");
    const void *driver = NULL;
    const int result = p4_battle_menu_aim(p4_luau_user(L), unit, &driver);
    if (result) return refusal(L, result, driver);
    lua_pushboolean(L, 1);
    return 1;
}
static void push_units(lua_State *L, uint32_t head)
{
    lua_newtable(L);
    int n = 0;
    for (uint32_t unit = head; unit && n < 16; unit = P4_G32(unit + UNIT_NEXT)) {
        lua_createtable(L, 0, 3);
        lua_pushunsigned(L, unit); lua_setfield(L, -2, "unit");
        lua_pushinteger(L, P4_G16(unit + UNIT_CHAR)); lua_setfield(L, -2, "id");
        lua_pushunsigned(L, P4_G32(unit + UNIT_DATA)); lua_setfield(L, -2, "data");
        lua_rawseti(L, -2, ++n);
    }
}
static int battle_units(lua_State *L)
{
    const uint32_t battle = P4_G32(BATTLE_STATE);
    if (!battle) {
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 2);
    push_units(L, P4_G32(battle + STATE_UNIT_LISTS)); lua_setfield(L, -2, "party");
    push_units(L, P4_G32(battle + STATE_UNIT_LISTS + 8u)); lua_setfield(L, -2, "enemies");
    return 1;
}
static void record_use(const void *owner)
{
    p4_lua_semantic_use((P4LuaMod *)owner);
}
void p4_lua_battle_initialize(lua_State *L)
{
    static const luaL_Reg methods[] = {
        {"menu", battle_menu}, {"choose", battle_choose}, {"point", battle_point}, {"aim", battle_aim},
        {"units", battle_units}, {"substitute", p4_lua_battle_substitute}, {NULL, NULL}
    };
    p4_battle_menu_prepare();
    p4_battle_menu_on_use(record_use);
    /* p4.battle may already hold other battle services (API 1.3 formation):
     * the namespace becomes one read-only table with theirs and these. */
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
    luaL_register(L, NULL, methods);
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "battle");
}
void p4_lua_battle_close(P4LuaMod *mod)
{
    p4_battle_menu_release(mod);
}
