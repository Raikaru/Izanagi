/* p4.contract: read-only plain values a mod publishes while main.luau runs, for
 * the mods that declared it in [require] or [companion] (docs/modding/lua.md
 * "Published contracts"). C owns frozen copies; every read builds fresh
 * read-only tables in the reader's own VM. Game thread only. */
#include "lua_runtime.h"
#include "p4pc/mod_setting_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONTRACT_MAX_BYTES (64u * 1024u)
#define CONTRACTS_PER_MOD 16u

typedef struct Contract {
    struct Contract *next;
    const P4LuaMod *mod;
    char owner[64], name[P4_MOD_SETTING_KEY_MAX];
    uint32_t version;
    void *value;
} Contract;
static Contract *contracts;

static const char *checked_name(lua_State *L, int index)
{
    const char *name = luaL_checkstring(L, index);
    if (!p4_mod_setting_key_valid(name)) luaL_argerror(L, index, "contract names are [a-z0-9][a-z0-9_.-]{0,47}");
    return name;
}
static int publish(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    const P4LuaFrame *frame = p4_lua_frame(L);
    if (!frame->startup || frame->snapshot) luaL_error(L, "p4.contract.publish is only legal while main.luau runs");
    const char *name = checked_name(L, 1);
    uint32_t version = (uint32_t)p4_lua_integer(L, 2, 1, UINT32_MAX);
    if (lua_isnoneornil(L, 3)) luaL_argerror(L, 3, "contract values cannot be nil");
    unsigned count = 0;
    for (const Contract *c = contracts; c; c = c->next) {
        if (c->mod != mod) continue;
        if (!strcmp(c->name, name)) luaL_error(L, "contract %s is already published", name);
        ++count;
    }
    if (count == CONTRACTS_PER_MOD) luaL_error(L, "a mod publishes at most %u contracts", CONTRACTS_PER_MOD);
    Contract *contract = calloc(1, sizeof *contract);
    if (!contract) luaL_error(L, "cannot allocate contract %s", name);
    char error[256];
    contract->value = p4_lua_value_copy(L, 3, CONTRACT_MAX_BYTES, "contract value", error, sizeof error);
    if (!contract->value) {
        free(contract);
        luaL_error(L, "%s", error);
    }
    contract->mod = mod;
    snprintf(contract->owner, sizeof contract->owner, "%s", mod->info->id);
    snprintf(contract->name, sizeof contract->name, "%s", name);
    contract->version = version;
    contract->next = contracts;
    contracts = contract;
    return 0;
}
/* The manifest fixes whose contracts a mod may read. */
static int declared(const P4LuaMod *mod, const char *owner)
{
    size_t count = 0;
    const P4PackageField *fields = p4_package_provider_fields(mod->provider, &count);
    for (size_t i = 0; i < count; ++i)
        if ((!strcmp(fields[i].section, "require") || !strcmp(fields[i].section, "companion")) &&
            !strcmp(fields[i].argument, owner)) return 1;
    return 0;
}
static int read_contract(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    const char *owner = luaL_checkstring(L, 1);
    const char *name = checked_name(L, 2);
    if (!declared(mod, owner))
        luaL_error(L, "p4.contract.read: %s is not a declared requirement or companion", owner);
    for (const Contract *c = contracts; c; c = c->next) {
        if (strcmp(c->owner, owner) || strcmp(c->name, name)) continue;
        if (!lua_checkstack(L, 2)) luaL_error(L, "p4.contract.read cannot grow interpreter stack");
        p4_lua_value_push(L, c->value, 1);
        lua_pushnumber(L, (double)c->version);
        return 2;
    }
    lua_pushnil(L);
    return 1;
}
void p4_lua_contract_destroy(P4LuaMod *mod)
{
    for (Contract **link = &contracts; *link;) {
        Contract *c = *link;
        if (c->mod != mod) {
            link = &c->next;
            continue;
        }
        *link = c->next;
        p4_lua_value_free(c->value);
        free(c);
    }
}
void p4_lua_contract(lua_State *L)
{
    static const luaL_Reg methods[] = {{"publish", publish}, {"read", read_contract}, {NULL, NULL}};
    p4_lua_library(L, methods);
}
