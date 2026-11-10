/* p4.tables: per-run session overlays of the loaded battle banks (API 1.9,
 * docs/modding/lua.md "Session bank overlays"). A thin binding of the table
 * service's p4_game_tables_overlay_* owner. */
#include "native_bindings.h"
#include "p4pc/game_table_overlay.h"
#include <stdio.h>
#include <string.h>

static int push_bank(lua_State *L, const P4TableOverlayInfo *bank)
{
    uint32_t base = 0, bytes = 0;
    int loaded = p4_game_tables_overlay_extent(bank, &base, &bytes);
    lua_createtable(L, 0, 6);
    lua_pushstring(L, bank->key); lua_setfield(L, -2, "key");
    lua_pushstring(L, bank->file); lua_setfield(L, -2, "file");
    lua_pushunsigned(L, bank->block); lua_setfield(L, -2, "block");
    if (loaded) { lua_pushunsigned(L, bytes); lua_setfield(L, -2, "bytes"); }
    lua_pushboolean(L, bank->overlayable); lua_setfield(L, -2, "overlayable");
    const char *owner = p4_game_tables_overlay_owner(bank);
    if (owner) { lua_pushstring(L, owner); lua_setfield(L, -2, "owner"); }
    lua_setreadonly(L, -1, 1);
    return 1;
}
static const P4TableOverlayInfo *checked_bank(lua_State *L, int index)
{
    const char *key = luaL_checkstring(L, index);
    const P4TableOverlayInfo *bank = p4_game_tables_overlay_find(key);
    if (!bank) luaL_argerror(L, index, "unknown battle bank");
    return bank;
}
static int tables_banks(lua_State *L)
{
    lua_createtable(L, P4_TABLE_OVERLAY_BANKS, 0);
    for (unsigned i = 0; i < P4_TABLE_OVERLAY_BANKS; ++i) {
        push_bank(L, p4_game_tables_overlay_info(i));
        lua_rawseti(L, -2, (int)i + 1);
    }
    lua_setreadonly(L, -1, 1);
    return 1;
}
static int tables_bank(lua_State *L)
{
    return push_bank(L, checked_bank(L, 1));
}
static int tables_stock(lua_State *L)
{
    const P4TableOverlayInfo *bank = checked_bank(L, 1);
    uint32_t base = 0, bytes = 0;
    if (!p4_game_tables_overlay_extent(bank, &base, &bytes)) luaL_error(L, "battle bank %s is not loaded", bank->key);
    void *out = lua_newbuffer(L, bytes);
    if (p4_game_tables_overlay_stock(bank, out, bytes)) luaL_error(L, "battle bank %s is not readable", bank->key);
    return 1;
}
/* Overlays change the game: only from a frame boundary callback (an event,
 * load or frame_start), never inside a hooked function, init or a snapshot. */
static void require_safe_point(lua_State *L)
{
    p4_lua_require_mutation(L);
    const P4LuaFrame *frame = p4_lua_frame(L);
    if (frame->no_native_calls)
        luaL_error(L, "p4.tables.overlay is refused in synchronous UI/action callbacks and save snapshots");
    if (frame->startup) luaL_error(L, "p4.tables.overlay is refused during mod initialization");
    for (; frame; frame = frame->previous)
        if (frame->context) luaL_error(L, "p4.tables.overlay is refused inside a hooked function");
}
static int tables_overlay(lua_State *L)
{
    require_safe_point(L);
    P4LuaMod *mod = p4_luau_user(L);
    luaL_checktype(L, 1, LUA_TTABLE);
    P4TableOverlayBank layer[P4_TABLE_OVERLAY_BANKS];
    size_t count = 0;
    lua_pushnil(L);
    while (lua_next(L, 1)) {
        if (lua_type(L, -2) != LUA_TSTRING) luaL_error(L, "p4.tables.overlay keys are bank keys");
        if (count == P4_TABLE_OVERLAY_BANKS) luaL_error(L, "p4.tables.overlay names too many banks");
        size_t size = 0;
        const void *bytes = luaL_checkbuffer(L, -1, &size);
        /* Keys and buffers stay referenced by the argument table during the call. */
        layer[count++] = (P4TableOverlayBank){lua_tostring(L, -2), bytes, size};
        lua_pop(L, 1);
    }
    char reason[P4_TABLE_OVERLAY_REASON];
    if (p4_game_tables_overlay_apply(mod->owner, mod->info->id, layer, count, reason, sizeof reason)) {
        fprintf(stderr, "p4pc: mod %s: table overlay refused: %s\n", mod->info->id, reason);
        lua_pushboolean(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    if (count) p4_lua_semantic_use(mod);
    lua_pushboolean(L, 1);
    return 1;
}
static int tables_clear(lua_State *L)
{
    require_safe_point(L);
    P4LuaMod *mod = p4_luau_user(L);
    char reason[P4_TABLE_OVERLAY_REASON];
    if (p4_game_tables_overlay_apply(mod->owner, mod->info->id, NULL, 0, reason, sizeof reason)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}
static int tables_active(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    lua_newtable(L);
    for (unsigned i = 0; i < P4_TABLE_OVERLAY_BANKS; ++i) {
        const P4TableOverlayInfo *bank = p4_game_tables_overlay_info(i);
        const char *owner = p4_game_tables_overlay_owner(bank);
        if (!owner || strcmp(owner, mod->info->id)) continue;
        lua_pushboolean(L, 1);
        lua_setfield(L, -2, bank->key);
    }
    lua_setreadonly(L, -1, 1);
    return 1;
}

void p4_lua_tables_initialize(lua_State *L)
{
    if (p4_game_tables_overlay_session_register())
        fprintf(stderr, "p4pc: mods: table overlay session restore could not be registered\n");
    static const luaL_Reg methods[] = {{"banks", tables_banks}, {"bank", tables_bank}, {"stock", tables_stock},
        {"overlay", tables_overlay}, {"clear", tables_clear}, {"active", tables_active}, {NULL, NULL}};
    p4_lua_library(L, methods);
    lua_setfield(L, -2, "tables");
}
void p4_lua_tables_close(P4LuaMod *mod)
{
    p4_game_tables_overlay_clear_owner(mod->owner);
}
