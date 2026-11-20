/* p4.ui.native_row (capability ui.native_row@1): a mod row in a native menu
 * whose layout is hard-coded per row count, over include/p4pc/native_rows.h.
 * The first host is the Velvet Room fusion card list ("velvet_fusion"): one
 * mod card after the spreads, drawn from the mod's own sprites, that opens the
 * mod's registered page. Presentation only: refused in save snapshots, never
 * semantic use. The row is removed when the VM closes (p4_lua_ui_close). */
#include "native_bindings.h"
#include "p4pc/native_rows.h"
#include <stdio.h>
#include <string.h>

typedef struct LuaRow {
    P4LuaMod *mod;
    int handle, removed;
} LuaRow;

static LuaRow *row_handle(lua_State *L)
{
    LuaRow *row = p4_lua_userdata(L, 1, P4_LUA_TAG_UI_NATIVE_ROW, "native row");
    if (row->mod != p4_luau_user(L)) luaL_error(L, "foreign native row");
    if (row->removed) luaL_error(L, "native row was removed");
    return row;
}

static int sprite_field(lua_State *L, int table, const char *name)
{
    lua_rawgetfield(L, table, name);
    if (lua_isnil(L, -1)) luaL_error(L, "p4.ui.native_row needs the %s sprite", name);
    int handle = p4_lua_ui_sprite_handle(L, -1);
    lua_pop(L, 1);
    return handle;
}

/* p4.ui.native_row{host, key, page, symbol, label, small}: page is this mod's
 * registered page or detail handle; nil plus a reason when the host's mod row
 * is taken or the art does not fit the host's families. */
int p4_lua_ui_native_row(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod = p4_luau_user(L);
    if (mod->ui_closing) luaL_error(L, "p4.ui.native_row after native UI shutdown");
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_rawgetfield(L, 1, "host");
    if (lua_type(L, -1) != LUA_TSTRING || strcmp(lua_tostring(L, -1), "velvet_fusion"))
        luaL_error(L, "p4.ui.native_row host must be \"velvet_fusion\"");
    lua_pop(L, 1);
    lua_rawgetfield(L, 1, "key");
    size_t key_length = 0;
    const char *key = lua_type(L, -1) == LUA_TSTRING ? lua_tolstring(L, -1, &key_length) : NULL;
    if (!key || !key_length || key_length > 47) luaL_error(L, "p4.ui.native_row key must be 1..47 bytes");
    char row_key[64];
    snprintf(row_key, sizeof row_key, "%s", key);
    lua_pop(L, 1);
    /* The page handle's own namespaced key ("lua/<mod id>/<local key>"). */
    lua_rawgetfield(L, 1, "page");
    if (lua_isnil(L, -1)) luaL_error(L, "p4.ui.native_row needs this mod's page");
    lua_getfield(L, -1, "key");
    const char *page = lua_tostring(L, -1);
    char prefix[160];
    snprintf(prefix, sizeof prefix, "lua/%s/", mod->info->id);
    if (!page || strncmp(page, prefix, strlen(prefix))) luaL_error(L, "p4.ui.native_row page must be this mod's page");
    char page_key[128];
    if (snprintf(page_key, sizeof page_key, "%s", page) >= (int)sizeof page_key) luaL_error(L, "page key too long");
    lua_pop(L, 2);
    P4NativeRowArt art = {sprite_field(L, 1, "symbol"), sprite_field(L, 1, "label"), sprite_field(L, 1, "small")};
    int handle = p4_native_rows_add(P4_NATIVE_ROWS_VELVET_FUSION, (uint32_t)mod->owner, row_key, &art, page_key);
    if (handle < 0) {
        lua_pushnil(L);
        lua_pushstring(L, "refused: one mod card per Velvet list, art in the Velvet card families "
                          "(72x77 symbol, label up to 99x31, small up to 72x18)");
        return 2;
    }
    LuaRow *row = lua_newuserdatataggedwithmetatable(L, sizeof *row, P4_LUA_TAG_UI_NATIVE_ROW);
    *row = (LuaRow){mod, handle, 0};
    return 1;
}

static int row_enable(lua_State *L)
{
    LuaRow *row = row_handle(L);
    p4_lua_require_mutation(L);
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    lua_pushboolean(L, p4_native_rows_enable(row->handle, lua_toboolean(L, 2)) == 0);
    return 1;
}

static int row_present(lua_State *L)
{
    row_handle(L);
    lua_pushboolean(L, p4_native_rows_present(P4_NATIVE_ROWS_VELVET_FUSION));
    return 1;
}

static int row_remove(lua_State *L)
{
    LuaRow *row = row_handle(L);
    p4_lua_require_mutation(L);
    lua_pushboolean(L, p4_native_rows_remove(row->handle) == 0);
    row->removed = 1;
    return 1;
}

void p4_lua_rows_initialize(lua_State *L)
{
    lua_newtable(L);
    lua_newtable(L);
    static const luaL_Reg methods[] = {
        {"enable", row_enable}, {"present", row_present}, {"remove", row_remove}, {NULL, NULL}
    };
    luaL_register(L, NULL, methods);
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "__index");
    lua_pushliteral(L, "p4 native row"); lua_setfield(L, -2, "__metatable");
    lua_setreadonly(L, -1, 1);
    lua_setuserdatametatable(L, P4_LUA_TAG_UI_NATIVE_ROW);
}

void p4_lua_rows_close(P4LuaMod *mod)
{
    p4_native_rows_remove_owner((uint32_t)mod->owner);
}
