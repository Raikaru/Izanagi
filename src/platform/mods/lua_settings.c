/* p4.settings: the mod's persisted settings (docs/modding/lua.md "Settings"),
 * and p4.mods.required: what this mod's declared requirements provide. */
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

/* A declared setting's effective choice: the admission value while restart-bound. */
static unsigned effective_choice(const P4PackageProvider *provider, const char *id, const P4ModSettingDecl *decl)
{
    char value[P4_MOD_SETTING_VALUE_MAX], reason[192];
    int found = decl->restart ? p4_mod_setting_boot(provider, decl->key, value, sizeof value) : 0;
    if (found == 0) found = p4_mod_setting_stored(id, decl->key, value, sizeof value, reason, sizeof reason);
    if (found < 0) {
        fprintf(stderr, "p4pc: mod %s: settings: %s\n", id, reason);
        found = 0;
    }
    return p4_mod_setting_choice(decl, found > 0 ? value : NULL);
}

static int get_setting(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    const char *key = checked_key(L);
    const P4ModSettingDecl *decl = declared(L, mod, key);
    if (decl) {
        unsigned choice = effective_choice(mod->provider, mod->info->id, decl);
        if (decl->kind == P4_MOD_SETTING_TOGGLE) lua_pushboolean(L, choice == 1);
        else lua_pushstring(L, decl->choices[choice]);
        return 1;
    }
    char value[P4_MOD_SETTING_VALUE_MAX], reason[192];
    int found = p4_mod_setting_stored(mod->info->id, key, value, sizeof value, reason, sizeof reason);
    if (found < 0) {
        fprintf(stderr, "p4pc: mod %s: settings: %s\n", mod->info->id, reason);
        found = 0;
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
    /* An admitted mod that requires this setting locks it: admission checked
     * the value its dependent relies on, and nothing may change it underneath. */
    const P4PackageProvider *requirer = decl ? p4_packages_requirer(mod->provider, key) : NULL;
    if (requirer) {
        lua_pushboolean(L, 0);
        lua_pushfstring(L, "locked: required by %s", p4_package_provider_info(requirer)->name);
        return 2;
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

/* p4.mods.required(id): a declared requirement's admitted version and the
 * effective values of the settings this mod requires of it. Read-only, legal in
 * every callback. An undeclared id is a script error: the manifest fixes them. */
static int required(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    const char *id = luaL_checkstring(L, 1);
    size_t field_count = 0;
    const P4PackageField *fields = p4_package_provider_fields(mod->provider, &field_count);
    int listed = 0;
    for (size_t i = 0; i < field_count && !listed; ++i)
        listed = !strcmp(fields[i].section, "require") && !strcmp(fields[i].argument, id);
    if (!listed) luaL_error(L, "p4.mods.required: %s is not a declared requirement", id);
    const P4PackageProvider *need = NULL;
    for (size_t i = 0; i < p4_packages_count() && !need; ++i) {
        const P4PackageInfo *info = p4_packages_info(i);
        if (info && info->kind == P4_PACKAGE_MOD && !strcmp(info->id, id)) need = p4_packages_provider(i);
    }
    if (!need) luaL_error(L, "p4.mods.required: %s is not admitted", id);
    /* Resolve every value before building Lua tables: a Lua allocation error
     * must not leak the parsed declarations. */
    size_t need_count = 0, decl_count = 0, value_count = 0;
    const P4PackageField *need_fields = p4_package_provider_fields(need, &need_count);
    P4ModSettingDecl *decls = NULL;
    char reason[256];
    struct { const char *key; int toggle; char choice[P4_MOD_SETTING_CHOICE_MAX]; } values[P4_PACKAGE_REQUIRE_SETTINGS];
    if (p4_mod_setting_declarations(need_fields, need_count, &decls, &decl_count, reason, sizeof reason))
        luaL_error(L, "p4.mods.required: %s settings: %s", id, reason);
    for (size_t i = 0; i < field_count && value_count < P4_PACKAGE_REQUIRE_SETTINGS; ++i) {
        const P4PackageField *field = &fields[i];
        if (strcmp(field->section, "require") || strcmp(field->argument, id) || strncmp(field->key, "setting.", 8))
            continue;
        const P4ModSettingDecl *decl = p4_mod_setting_find(decls, decl_count, field->key + 8);
        if (!decl) continue; /* admission refuses a requirement that lacks it */
        values[value_count].key = field->key + 8;
        values[value_count].toggle = decl->kind == P4_MOD_SETTING_TOGGLE;
        strcpy(values[value_count++].choice, decl->choices[effective_choice(need, id, decl)]);
    }
    free(decls);
    lua_createtable(L, 0, 2);
    lua_pushstring(L, p4_package_provider_info(need)->version);
    lua_setfield(L, -2, "version");
    lua_createtable(L, 0, (int)value_count);
    for (size_t i = 0; i < value_count; ++i) {
        if (values[i].toggle) lua_pushboolean(L, !strcmp(values[i].choice, "on"));
        else lua_pushstring(L, values[i].choice);
        lua_setfield(L, -2, values[i].key);
    }
    lua_setfield(L, -2, "settings");
    return 1;
}
void p4_lua_mods(lua_State *L)
{
    static const luaL_Reg methods[] = {{"required", required}, {NULL, NULL}};
    p4_lua_library(L, methods);
}
