/* p4.settings: the mod's persisted settings (docs/modding/lua.md "Settings"). */
#include "lua_runtime.h"
#include "p4pc/mod_setting_store.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const P4ModSettingDecl *declared(lua_State *L, P4LuaMod *mod, const char *key)
{
    if (!mod->settings_parsed) {
        size_t count = 0;
        const P4PackageField *fields = p4_package_provider_fields(mod->provider, &count);
        char reason[256];
        if (p4_mod_setting_declarations(fields, count, &mod->settings, &mod->setting_count, reason, sizeof reason))
            luaL_error(L, "invalid [setting] declarations: %s", reason);
        mod->settings_parsed = 1;
    }
    return p4_mod_setting_find(mod->settings, mod->setting_count, key);
}
static const char *checked_key(lua_State *L)
{
    const char *key = luaL_checkstring(L, 1);
    if (!p4_mod_setting_key_valid(key)) luaL_argerror(L, 1, "setting keys are [a-z0-9][a-z0-9_.-]{0,47}");
    return key;
}

static int get_setting(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    const char *key = checked_key(L);
    const P4ModSettingDecl *decl = declared(L, mod, key);
    char value[P4_MOD_SETTING_VALUE_MAX], reason[192];
    int found = decl && decl->restart ? p4_mod_setting_boot(mod->provider, key, value, sizeof value) : 0;
    if (found == 0) found = p4_mod_setting_stored(mod->info->id, key, value, sizeof value, reason, sizeof reason);
    if (found < 0) {
        fprintf(stderr, "p4pc: mod %s: settings: %s\n", mod->info->id, reason);
        found = 0;
    }
    if (decl) {
        unsigned choice = p4_mod_setting_choice(decl, found > 0 ? value : NULL);
        if (decl->kind == P4_MOD_SETTING_TOGGLE) lua_pushboolean(L, choice == 1);
        else lua_pushstring(L, decl->choices[choice]);
        return 1;
    }
    if (found <= 0 || value[1] != ':') { lua_pushnil(L); return 1; }
    if (value[0] == 'b' && (!strcmp(value + 2, "true") || !strcmp(value + 2, "false")))
        lua_pushboolean(L, value[2] == 't');
    else if (value[0] == 'n') {
        char *end;
        double number = strtod(value + 2, &end);
        if (*end || !isfinite(number)) lua_pushnil(L);
        else lua_pushnumber(L, number);
    } else if (value[0] == 's') lua_pushstring(L, value + 2);
    else lua_pushnil(L);
    return 1;
}

static int set_setting(lua_State *L)
{
    p4_lua_require_mutation(L);
    /* A synchronous disk write: only from frames that may take native effects. */
    if (p4_lua_frame(L)->no_native_calls)
        luaL_error(L, "p4.settings.set is refused in synchronous UI/action callbacks and save snapshots");
    P4LuaMod *mod = p4_luau_user(L);
    const char *key = checked_key(L);
    const P4ModSettingDecl *decl = declared(L, mod, key);
    char value[P4_MOD_SETTING_VALUE_MAX], reason[192];
    const char *stored = value;
    int type = lua_type(L, 2);
    if (type == LUA_TNIL || type == LUA_TNONE) stored = NULL;
    else if (decl && decl->kind == P4_MOD_SETTING_TOGGLE) {
        luaL_checktype(L, 2, LUA_TBOOLEAN);
        stored = lua_toboolean(L, 2) ? "on" : "off";
    } else if (decl) {
        const char *choice = luaL_checkstring(L, 2);
        unsigned i = 0;
        while (i < decl->choice_count && strcmp(decl->choices[i], choice)) ++i;
        if (i == decl->choice_count) luaL_argerror(L, 2, "not one of the declared choices");
        stored = decl->choices[i];
    } else if (type == LUA_TBOOLEAN) {
        snprintf(value, sizeof value, "b:%s", lua_toboolean(L, 2) ? "true" : "false");
    } else if (type == LUA_TNUMBER) {
        double number = lua_tonumber(L, 2);
        if (!isfinite(number)) luaL_argerror(L, 2, "setting numbers must be finite");
        snprintf(value, sizeof value, "n:%.17g", number);
    } else if (type == LUA_TSTRING) {
        size_t size;
        const char *text = lua_tolstring(L, 2, &size);
        if (size > 200 || memchr(text, 0, size) || snprintf(value, sizeof value, "s:%s", text) < 0 ||
            !p4_mod_setting_value_valid(value + 2))
            luaL_argerror(L, 2, "setting strings are 1..200 printable ASCII bytes without edge spaces");
    } else {
        luaL_argerror(L, 2, "settings store booleans, finite numbers or strings");
    }
    if (p4_mod_setting_store(mod->info->id, key, stored, reason, sizeof reason)) {
        lua_pushboolean(L, 0);
        lua_pushstring(L, reason);
        return 2;
    }
    lua_pushboolean(L, 1);
    return 1;
}

void p4_lua_settings(lua_State *L)
{
    static const luaL_Reg methods[] = {{"get", get_setting}, {"set", set_setting}, {NULL, NULL}};
    p4_lua_library(L, methods);
}
