/* data.luau runner (include/p4pc/game_data_edit.h).
 *
 * A mod's data.luau runs once per boot while the game builds its tables, in a
 * short-lived VM of its own: p4.id, p4.log, p4.has, p4.capabilities,
 * p4.settings.get (the values admitted this boot), the p4.data readers (over
 * the rows as the layers before it left them) and p4.data.edit. Nothing else
 * exists there: no hooks, memory, events or UI, since the game itself has not
 * started. p4.data.edit(fn) calls fn(d) at once; d.skill/persona/enemy/item
 * return views whose assignments become field writes of this mod's layer. */
#include "lua_data_internal.h"
#include "lua_runtime.h"
#include "p4pc/capabilities.h"
#include "p4pc/game_data_catalog.h"
#include "p4pc/game_data_edit.h"
#include "p4pc/mod_setting_store.h"
#include <fenv.h>
#include <stdio.h>
#include <string.h>

enum { VM_MEMORY = 16u * 1024u * 1024u, FUEL = 4000000u };

typedef struct Run {
    const P4PackageProvider *provider;
    const char *id;
    unsigned fuel;
    int make_reference;
} Run;

/* d.<kind>(query) returns node(kind, index): reads walk a fresh row, writes
 * become p4.data field writes; tables assign field by field. */
static const char prelude[] =
    "local set, learn, row, lookup = ...\n"
    "local function plain(v)\n"
    "    if type(v) == 'table' and (v.key ~= nil or v.id ~= nil) then return if v.key ~= nil then v.key else v.id end\n"
    "    return v\n"
    "end\n"
    "local function walk(value, path)\n"
    "    for part in string.gmatch(path, '[^.]+') do\n"
    "        if type(value) ~= 'table' then return nil end\n"
    "        local n = tonumber(part)\n"
    "        value = value[if n ~= nil then n else part]\n"
    "    end\n"
    "    return value\n"
    "end\n"
    "local function group(v) return type(v) == 'table' and v.key == nil and v.id == nil end\n"
    "local function assign(kind, index, path, v)\n"
    "    if group(v) and next(v) == nil then\n"
    "        set(kind, index, path, nil)\n"
    "    elseif group(v) then\n"
    "        for k, x in v do assign(kind, index, path .. '.' .. tostring(k), x) end\n"
    "    else\n"
    "        set(kind, index, path, plain(v))\n"
    "    end\n"
    "end\n"
    "local function call(f, ...)\n"
    "    local ok, message = pcall(f, ...)\n"
    "    if not ok then error(message, 3) end\n"
    "end\n"
    "local function learnset(index)\n"
    "    local list = row('persona', index).learnset\n"
    "    return setmetatable(list, {__index = {\n"
    "        add = function(_, event) call(learn, index, 'add', plain(event.skill), event.level) end,\n"
    "        remove = function(_, skill) call(learn, index, 'remove', plain(skill), 0) end,\n"
    "        move = function(_, skill, level) call(learn, index, 'move', plain(skill), level) end,\n"
    "        replace = function(_, skill, with) call(learn, index, 'replace', plain(skill), plain(with)) end,\n"
    "    }})\n"
    "end\n"
    "local node\n"
    "node = function(kind, index, prefix)\n"
    "    local function path(field) return if prefix then prefix .. '.' .. tostring(field) else tostring(field) end\n"
    "    return setmetatable({}, {\n"
    "        __index = function(_, field)\n"
    "            local at = path(field)\n"
    "            if kind == 'persona' and at == 'learnset' then return learnset(index) end\n"
    "            local value = walk(row(kind, index), at)\n"
    "            -- A floor's groups are rows of their own, edited through the floor.\n"
    "            local nested = kind == 'floor' and (at == 'encounters' or at == 'treasure') and value ~= nil\n"
    "            if nested or group(value) then return node(kind, index, at) end\n"
    "            return value\n"
    "        end,\n"
    "        __newindex = function(_, field, v)\n"
    "            local at = path(field)\n"
    "            if kind == 'persona' and at == 'learnset' then\n"
    "                error('a learnset changes through learnset:add, :remove, :move and :replace', 2)\n"
    "            end\n"
    "            local ok, message = pcall(assign, kind, index, at, v)\n"
    "            if not ok then error(message, 2) end\n"
    "        end,\n"
    "        __len = function() local t = walk(row(kind, index), prefix or '') return if type(t) == 'table' then #t else 0 end,\n"
    "        __iter = function() local t = walk(row(kind, index), prefix or '') return next, if type(t) == 'table' then t else {} end,\n"
    "        __tostring = function() return kind .. ' view' .. (if prefix then ' ' .. prefix else '') end,\n"
    "    })\n"
    "end\n"
    "return function()\n"
    "    local d = {}\n"
    "    for _, kind in {'skill', 'persona', 'enemy', 'item', 'floor', 'formation', 'encounters', 'treasure'} do\n"
    "        d[kind] = function(query)\n"
    "            local ok, index = pcall(lookup, kind, query)\n"
    "            if not ok then error(index, 2) end\n"
    "            return node(kind, index, nil)\n"
    "        end\n"
    "    end\n"
    "    return table.freeze(d)\n"
    "end\n";

static Run *run_of(lua_State *L) { return p4_luau_user(L); }
static P4DataKind kind_argument(lua_State *L, int index)
{
    P4DataKind kind;
    if (p4_data_kind_parse(luaL_checkstring(L, index), &kind)) luaL_error(L, "unknown kind");
    return kind;
}
/* Errors from the primitives carry no position: the prelude re-raises them
 * at the mod's own line. */
static int raise(lua_State *L, const char *message)
{
    lua_pushstring(L, message);
    lua_error(L);
    return 0;
}
static int resolve(lua_State *L, P4DataKind kind, int index, size_t *out)
{
    char message[600];
    if (lua_type(L, index) == LUA_TNUMBER) {
        const double value = lua_tonumber(L, index);
        if (value >= 0 && value <= 4294967295.0 && value == (double)(uint32_t)value &&
            !p4_data_catalog_index(kind, (uint32_t)value, out)) return 0;
        snprintf(message, sizeof message, "no %s has the ID %g", p4_data_kind_name(kind), value);
        return raise(L, message);
    }
    const char *query = luaL_checkstring(L, index);
    P4DataRows rows;
    if (p4_data_catalog_rows(kind, &rows)) return raise(L, "the catalog is unavailable");
    if (p4_data_lookup(&rows, query, out, message, sizeof message)) return raise(L, message);
    return 0;
}
static int prim_lookup(lua_State *L)
{
    size_t index;
    resolve(L, kind_argument(L, 1), 2, &index);
    lua_pushnumber(L, (double)index);
    return 1;
}
static int prim_row(lua_State *L)
{
    const P4DataKind kind = kind_argument(L, 1);
    if (p4_lua_data_push_row(L, kind, (size_t)luaL_checknumber(L, 2))) return raise(L, "that row is outside the tables");
    return 1;
}
static int prim_set(lua_State *L)
{
    const P4DataKind kind = kind_argument(L, 1);
    const size_t index = (size_t)luaL_checknumber(L, 2);
    const char *path = luaL_checkstring(L, 3);
    P4DataEditValue value = {0};
    if (lua_type(L, 4) == LUA_TNUMBER) value.number = lua_tonumber(L, 4);
    else if (lua_type(L, 4) == LUA_TSTRING) { value.text = 1; value.string = lua_tostring(L, 4); }
    else if (lua_isnil(L, 4)) { value.text = 1; value.string = "none"; }
    else if (lua_type(L, 4) == LUA_TBOOLEAN) { value.text = 1; value.string = lua_toboolean(L, 4) ? "true" : "false"; }
    else {
        char message[200];
        snprintf(message, sizeof message, "%s: expected a number, a name or a key, got a %s", path, luaL_typename(L, 4));
        return raise(L, message);
    }
    char error[768];
    if (p4_data_edit_set(kind, index, path, &value, error, sizeof error)) return raise(L, error);
    return 0;
}
static int prim_learn(lua_State *L)
{
    static const char *const ops[] = {"add", "remove", "move", "replace", NULL};
    const size_t persona = (size_t)luaL_checknumber(L, 1);
    const int op = luaL_checkoption(L, 2, NULL, ops);
    size_t skill, other;
    uint32_t value = 0;
    resolve(L, P4_DATA_SKILL, 3, &skill);
    if (op == P4_DATA_LEARN_REPLACE) {
        resolve(L, P4_DATA_SKILL, 4, &other);
        value = p4_data_catalog_id(P4_DATA_SKILL, other);
    } else if (op != P4_DATA_LEARN_REMOVE) {
        const double level = luaL_checknumber(L, 4);
        if (!(level >= 0 && level <= 255) || level != (double)(uint32_t)level) return raise(L, "learnset: level must be a whole number");
        value = (uint32_t)level;
    }
    char error[768];
    if (p4_data_edit_learn(persona, (P4DataLearnOp)op, p4_data_catalog_id(P4_DATA_SKILL, skill), value, error, sizeof error))
        return raise(L, error);
    return 0;
}

static int data_edit(lua_State *L)
{
    Run *run = run_of(L);
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_getref(L, run->make_reference);
    lua_call(L, 0, 1);       /* d */
    lua_pushvalue(L, 1);
    lua_insert(L, -2);       /* fn, d */
    lua_call(L, 1, 0);
    return 0;
}
static int core_log(lua_State *L)
{
    fprintf(stderr, "p4pc: mod %s (data.luau): %s\n", run_of(L)->id, luaL_checkstring(L, 1));
    return 0;
}
static int core_has(lua_State *L)
{
    size_t size;
    const char *text = luaL_checklstring(L, 1, &size);
    char key[P4_CAPABILITY_KEY_MAX];
    unsigned revision;
    if (p4_capability_parse(text, size, key, &revision)) luaL_error(L, "p4.has: \"%s\" is not a capability key", text);
    lua_pushboolean(L, p4_capability_revision(key) >= revision);
    return 1;
}
static int core_capabilities(lua_State *L)
{
    lua_createtable(L, 0, (int)p4_capability_count());
    for (size_t i = 0; i < p4_capability_count(); ++i) {
        unsigned revision;
        const char *key = p4_capability_at(i, &revision);
        lua_pushnumber(L, revision);
        lua_setfield(L, -2, key);
    }
    return 1;
}
static int settings_get(lua_State *L)
{
    char value[64];
    const int found = p4_mod_setting_boot(run_of(L)->provider, luaL_checkstring(L, 1), value, sizeof value);
    if (found <= 0) { lua_pushnil(L); return 1; }
    if (!strcmp(value, "on") || !strcmp(value, "off")) lua_pushboolean(L, value[1] == 'n');
    else lua_pushstring(L, value);
    return 1;
}
static void initialize(lua_State *L, void *user)
{
    const Run *run = user;
    static const luaL_Reg core[] = {{"log", core_log}, {"has", core_has}, {"capabilities", core_capabilities}, {NULL, NULL}};
    static const luaL_Reg settings[] = {{"get", settings_get}, {NULL, NULL}};
    static const luaL_Reg edit[] = {{"edit", data_edit}, {NULL, NULL}};
    lua_newtable(L);
    luaL_register(L, NULL, core);
    lua_pushstring(L, run->id);
    lua_setfield(L, -2, "id");
    p4_lua_library(L, settings);
    lua_setfield(L, -2, "settings");
    lua_newtable(L);
    luaL_register(L, NULL, p4_lua_data_read_methods());
    luaL_register(L, NULL, edit);
    lua_setreadonly(L, -1, 1);
    lua_setfield(L, -2, "data");
    lua_setreadonly(L, -1, 1);
    lua_setglobal(L, "p4");
}
static int interrupt(void *user, int gc)
{
    Run *run = user;
    return !gc && ++run->fuel > FUEL;
}

static int run_data(const P4PackageProvider *provider, const char *source, size_t size, char *error, size_t capacity)
{
    const P4PackageInfo *info = p4_package_provider_info(provider);
    Run run = {provider, info ? info->id : "?", 0, LUA_NOREF};
    p4_luau_config config = {
        .memory_limit = VM_MEMORY, .interrupt = interrupt, .user = &run, .initialize = initialize,
        .source_limit = P4_DATA_EDIT_LUAU_MAX
    };
    fenv_t native;
    fegetenv(&native);
    fesetenv(FE_DFL_ENV);
    int result = -1;
    p4_luau *vm = p4_luau_create(&config);
    if (!vm) snprintf(error, capacity, "data.luau: cannot create a Luau VM");
    else {
        lua_State *L = p4_luau_state(vm);
        if (p4_luau_load_source(vm, "@p4/data-edit", prelude, sizeof prelude - 1) != LUA_OK) {
            snprintf(error, capacity, "data.luau: the edit view failed to load: %s", p4_luau_error(vm));
        } else {
            p4_lua_data_edit_push_primitives(L);
            if (p4_luau_run(vm, 4, 1) != LUA_OK) {
                snprintf(error, capacity, "data.luau: the edit view failed to start: %s", p4_luau_error(vm));
            } else {
                run.make_reference = lua_ref(L, -1);
                lua_pop(L, 1);
                char name[96];
                snprintf(name, sizeof name, "@%s/data.luau", run.id);
                if (p4_luau_load_source(vm, name, source, size) != LUA_OK || p4_luau_run(vm, 0, 0) != LUA_OK)
                    snprintf(error, capacity, "%s", run.fuel > FUEL ? "data.luau ran too long (an endless loop?)" : p4_luau_error(vm));
                else result = 0;
            }
        }
        p4_luau_destroy(vm);
    }
    fesetenv(&native);
    return result;
}
void p4_lua_data_edit_install(void) { p4_data_edits_runner(run_data); }
const char *p4_lua_data_edit_prelude(size_t *size)
{
    *size = sizeof prelude - 1;
    return prelude;
}
void p4_lua_data_edit_push_primitives(lua_State *L)
{
    lua_pushcfunction(L, prim_set, "p4.data.set");
    lua_pushcfunction(L, prim_learn, "p4.data.learn");
    lua_pushcfunction(L, prim_row, "p4.data.row");
    lua_pushcfunction(L, prim_lookup, "p4.data.lookup");
}
