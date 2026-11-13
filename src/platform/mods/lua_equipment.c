/* p4.equipment: the equipment exchange (docs/modding/lua.md "Equipment
 * exchange"). get reads a slot; check validates a request now; equip
 * validates and queues; the frame safe point revalidates, then equips or
 * refuses; result takes the outcome. */
#include "native_bindings.h"
#include "p4pc/mod_equipment.h"
#include "p4pc/mod_events.h"

static void native_only(lua_State *L, const char *what)
{
    p4_lua_require_mutation(L);
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "%s is refused in synchronous UI/action callbacks and save snapshots", what);
}

static P4ModEquip request(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    P4ModEquip equip = {0};
    lua_getfield(L, 1, "character");
    equip.character = (uint8_t)p4_lua_integer(L, -1, 1, 8);
    lua_getfield(L, 1, "slot");
    equip.slot = (uint8_t)p4_lua_integer(L, -1, 0, 2);
    lua_getfield(L, 1, "item");
    equip.item = (uint16_t)p4_lua_integer(L, -1, 1, 0x7ffd);
    lua_pop(L, 3);
    return equip;
}

/* Legal in every callback mode: it copies one halfword. */
static int equipment_get(lua_State *L)
{
    int character = (int)p4_lua_integer(L, 1, 1, 8), slot = (int)p4_lua_integer(L, 2, 0, 2);
    int32_t item = 0;
    char reason[192];
    if (p4_mod_equipment_get(character, slot, &item, reason, sizeof reason)) luaL_error(L, "%s", reason);
    lua_pushinteger(L, item > 0 ? item : 0);
    return 1;
}

static int equipment_check(lua_State *L)
{
    native_only(L, "p4.equipment.check");
    P4ModEquip equip = request(L);
    int16_t previous = 0;
    char reason[192];
    if (p4_mod_equipment_check(&equip, &previous, reason, sizeof reason)) {
        lua_pushnil(L);
        lua_pushstring(L, reason);
        return 2;
    }
    lua_pushinteger(L, previous > 0 ? previous : 0);
    return 1;
}

static int equipment_equip(lua_State *L)
{
    native_only(L, "p4.equipment.equip");
    P4LuaMod *mod = p4_luau_user(L);
    P4ModEquip equip = request(L);
    char reason[192];
    if (p4_mod_equipment_queue(mod->owner, &equip, reason, sizeof reason)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    /* A queued exchange will change the player's equipment and items. */
    p4_lua_semantic_use(mod);
    lua_pushboolean(L, 1);
    return 1;
}

static int equipment_result(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    P4ModEquip equip;
    char reason[192];
    int outcome = p4_mod_equipment_take_result(mod->owner, &equip, reason, sizeof reason);
    if (outcome == P4_MOD_EQUIP_NONE) {
        lua_pushnil(L);
        return 1;
    }
    lua_createtable(L, 0, 6);
    lua_pushstring(L, outcome == P4_MOD_EQUIP_EQUIPPED ? "equipped" : "refused"); lua_setfield(L, -2, "outcome");
    if (outcome == P4_MOD_EQUIP_REFUSED) { lua_pushstring(L, reason); lua_setfield(L, -2, "reason"); }
    lua_pushinteger(L, equip.character); lua_setfield(L, -2, "character");
    lua_pushinteger(L, equip.slot); lua_setfield(L, -2, "slot");
    lua_pushinteger(L, equip.item); lua_setfield(L, -2, "item");
    lua_pushinteger(L, equip.previous > 0 ? equip.previous : 0); lua_setfield(L, -2, "previous");
    lua_setreadonly(L, -1, 1);
    return 1;
}

static int equipment_pending(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    lua_pushboolean(L, p4_mod_equipment_pending(mod->owner));
    return 1;
}

void p4_lua_equipment_close(P4LuaMod *mod)
{
    p4_mod_equipment_clear_owner(mod->owner);
}

void p4_lua_equipment_initialize(lua_State *L)
{
    p4_mod_events_set_equipment(p4_mod_equipment_apply_pending);
    static const luaL_Reg methods[] = {{"get", equipment_get}, {"check", equipment_check},
        {"equip", equipment_equip}, {"result", equipment_result}, {"pending", equipment_pending}, {NULL, NULL}};
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "equipment");
}
