/* Restricted globals and denied capabilities.
 *
 * Contract (include/p4pc/luau_c.h): p4_luau_create opens only base, coroutine,
 * table, string, math, utf8, bit32 and buffer, registers host globals through
 * config.initialize, then applies upstream luaL_sandbox + luaL_sandboxthread.
 * No OS/io/debug/require/loaders, and no boundary API accepts bytecode. */
#include "witness.h"

struct host { unsigned probes; };

static int host_probe(lua_State *L)
{
    struct host *host = p4_luau_user(L);
    ++host->probes;
    lua_pushinteger(L, (int)host->probes);
    return 1;
}

/* Same shape as the production mod runtime: one plain host function plus a
 * host API table registered before the sandbox freezes globals. */
static void initialize(lua_State *L, void *user)
{
    (void)user;
    lua_pushcfunction(L, host_probe, "host_probe");
    lua_setglobal(L, "host_probe");
    lua_newtable(L);
    lua_pushcfunction(L, host_probe, "p4.probe");
    lua_setfield(L, -2, "probe");
    lua_pushstring(L, "witness");
    lua_setfield(L, -2, "id");
    lua_setglobal(L, "p4");
}

static p4_luau *create(struct host *host)
{
    p4_luau_config config = {4u * 1024u * 1024u, NULL, host, initialize, 0};
    return p4_luau_create(&config);
}

/* The exact frozen global set: everything the upstream libraries listed in the
 * contract register (including luaopen_table's Lua 5.1 compat global `unpack`,
 * a separate closure over table.unpack's implementation), plus this host's two
 * names, and nothing else. */
static int exact_global_set(p4_luau *vm)
{
    static const char expected[] =
        "_G _VERSION assert bit32 buffer coroutine error gcinfo getfenv getmetatable "
        "host_probe ipairs math newproxy next p4 pairs pcall print rawequal rawget rawlen "
        "rawset select setfenv setmetatable string table tonumber tostring type typeof "
        "unpack utf8 xpcall";
    lua_State *L = p4_luau_state(vm);
    CHECK_VM(vm, witness_run(vm, "@globals-set.luau",
        "assert(next(getfenv(1)) == nil, 'fresh thread environment is not empty')\n"
        "local names = {}\n"
        "for k in pairs(_G) do\n"
        "  assert(type(k) == 'string', 'non-string global key')\n"
        "  table.insert(names, k)\n"
        "end\n"
        "table.sort(names)\n"
        "return table.concat(names, ' ')", 1) == LUA_OK);
    const char *actual = lua_tostring(L, -1);
    if (actual == NULL || strcmp(actual, expected) != 0) {
        fprintf(stderr, "global set mismatch\n  expected: %s\n  actual:   %s\n", expected,
            actual ? actual : "(not a string)");
        return 1;
    }
    lua_pop(L, 1);
    CHECK(lua_gettop(L) == 0);
    return 0;
}

static int libraries_frozen_and_inert(p4_luau *vm)
{
    CHECK_VM(vm, witness_run(vm, "@globals-libraries.luau",
        "assert(_VERSION == 'Luau')\n"
        "assert(table.isfrozen(_G), '_G is mutable')\n"
        "local smt = getmetatable('')\n"
        "assert(type(smt) == 'table' and smt.__index == string and table.isfrozen(smt))\n"
        "local penv = getmetatable(getfenv(1))\n"
        "assert(type(penv) == 'table' and penv.__index == _G and table.isfrozen(penv))\n"
        "for _, name in ipairs({'coroutine', 'table', 'string', 'math', 'utf8', 'bit32', 'buffer', 'p4'}) do\n"
        "  local lib = _G[name]\n"
        "  assert(type(lib) == 'table' and table.isfrozen(lib), name .. ' is not a frozen table')\n"
        "  for k, v in pairs(lib) do\n"
        "    local t = type(v)\n"
        "    local ok = t == 'function' or (t == 'number' and name == 'math')\n"
        "      or (t == 'string' and (name == 'utf8' or name == 'p4'))\n"
        "    assert(ok, name .. '.' .. tostring(k) .. ' exposes a ' .. t)\n"
        "  end\n"
        "end\n"
        "assert(string.dump == nil)\n"
        "return true", 1) == LUA_OK);
    lua_pop(p4_luau_state(vm), 1);
    return 0;
}

/* Filesystem, process, debug, module loading and runtime code loading are all
 * unreachable, both from the frozen globals and through the thread proxy. */
static int denied_capabilities(p4_luau *vm)
{
    CHECK_VM(vm, witness_run(vm, "@globals-denied.luau",
        "local env = getfenv(1)\n"
        "for _, name in ipairs({'os', 'io', 'debug', 'package', 'require', 'load', 'loadstring',\n"
        "    'loadfile', 'dofile', 'collectgarbage', 'module', 'integer', 'vector', 'class',\n"
        "    'ffi', 'jit', 'getgenv', 'shared', 'task'}) do\n"
        "  assert(rawget(_G, name) == nil, name .. ' is a frozen global')\n"
        "  assert(env[name] == nil, name .. ' is reachable from the thread environment')\n"
        "end\n"
        "local ok = pcall(setfenv, print, {})\n"
        "assert(not ok, 'setfenv changed a C function environment')\n"
        "return true", 1) == LUA_OK);
    lua_pop(p4_luau_state(vm), 1);
    return 0;
}

/* Each mutation of a shared/builtin table must fail with Luau's readonly error,
 * and afterwards every builtin must still be intact. */
static int readonly_mutations(p4_luau *vm)
{
    CHECK_VM(vm, witness_run(vm, "@globals-readonly.luau",
        "local attempts = {\n"
        "  function() table.insert = nil end,\n"
        "  function() rawset(table, 'insert', nil) end,\n"
        "  function() _G.print = nil end,\n"
        "  function() rawset(_G, 'os', {}) end,\n"
        "  function() setmetatable(_G, {}) end,\n"
        "  function() getmetatable('').__index = {} end,\n"
        "  function() rawset(getmetatable(''), '__index', {}) end,\n"
        "  function() getmetatable('').__index.upper = string.lower end,\n"
        "  function() setmetatable(string, {}) end,\n"
        "  function() p4.probe = nil end,\n"
        "  function() rawset(getmetatable(getfenv(1)), '__index', {}) end,\n"
        "  function() table.insert(math, 1) end,\n"
        "  function() table.clear(coroutine) end,\n"
        "  function() buffer.create = nil end,\n"
        "}\n"
        "for i, attempt in ipairs(attempts) do\n"
        "  local ok, err = pcall(attempt)\n"
        "  assert(not ok, 'mutation ' .. i .. ' succeeded')\n"
        "  assert(string.find(tostring(err), 'readonly', 1, true), 'mutation ' .. i .. ': ' .. tostring(err))\n"
        "end\n"
        "assert(type(table.insert) == 'function' and _G.print == print and rawget(_G, 'os') == nil)\n"
        "assert(getmetatable('').__index == string and ('a'):upper() == 'A')\n"
        "assert(type(p4.probe) == 'function' and type(buffer.create) == 'function')\n"
        "assert(getmetatable(getfenv(1)).__index == _G and getmetatable(_G) == nil)\n"
        "return true", 1) == LUA_OK);
    lua_pop(p4_luau_state(vm), 1);
    return 0;
}

/* Script globals live in this VM's thread proxy: they persist across chunks of
 * the same VM, never enter the frozen _G, and never reach another VM. Shadowing
 * a library name cannot tamper with the builtin string metatable. */
static int per_vm_environment(p4_luau *first, struct host *first_host)
{
    lua_State *L = p4_luau_state(first);
    CHECK_VM(first, witness_run(first, "@globals-write.luau",
        "shadow = 41\n"
        "string = { upper = function() return 'tampered' end }\n"
        "assert(rawget(_G, 'shadow') == nil and rawget(getfenv(1), 'shadow') == 41)\n"
        "return host_probe()", 1) == LUA_OK);
    CHECK(lua_tointeger(L, -1) == (int)first_host->probes);
    lua_pop(L, 1);
    CHECK_VM(first, witness_run(first, "@globals-read.luau",
        "assert(string.upper() == 'tampered')\n"
        "assert(('abc'):upper() == 'ABC', 'builtin string metatable was tampered')\n"
        "string = nil -- removes only the thread-environment shadow\n"
        "assert(string.upper('a') == 'A')\n"
        "return shadow", 1) == LUA_OK);
    CHECK(lua_tointeger(L, -1) == 41);
    lua_pop(L, 1);

    struct host second_host = {0};
    p4_luau *second = create(&second_host);
    CHECK(second != NULL);
    lua_State *S = p4_luau_state(second);
    CHECK_VM(second, witness_run(second, "@globals-other-vm.luau",
        "assert(shadow == nil, 'global leaked between VMs')\n"
        "assert(string.upper('a') == 'A', 'library shadow leaked between VMs')\n"
        "return host_probe()", 1) == LUA_OK);
    CHECK(lua_tointeger(S, -1) == 1 && second_host.probes == 1);
    lua_pop(S, 1);
    p4_luau_destroy(second);

    /* A host-created thread (the production mod runtime's per-native-thread
     * pattern) shares this VM's sandboxed environment, not a fresh one. */
    {
        static const char body[] =
            "assert(shadow == 41 and rawget(_G, 'shadow') == nil)\n"
            "return p4.probe()";
        struct thread_create created = {NULL, LUA_NOREF};
        CHECK(lua_cpcall(L, witness_create_thread, &created) == LUA_OK);
        lua_State *T = created.state;
        int thread_ref = created.reference;
        CHECK_VM(first, p4_luau_load_source(first, "@globals-thread.luau", body, sizeof body - 1) == LUA_OK);
        lua_xmove(L, T, 1);
        unsigned before = first_host->probes;
        CHECK(lua_pcall(T, 0, 1, 0) == LUA_OK);
        CHECK(lua_tointeger(T, -1) == (int)before + 1);
        lua_settop(T, 0);
        lua_unref(L, thread_ref);
    }
    CHECK(lua_gettop(L) == 0);
    return 0;
}

/* p4_luau_load_source always compiles. Bytes shaped like a bytecode blob are
 * compiled as source text, never handed to luau_load. */
static int no_bytecode_admission(p4_luau *vm)
{
    lua_State *L = p4_luau_state(vm);
    /* Leading version byte of a Luau bytecode blob: a lexer error as source. */
    static const char blob[] = {6, 3, 1, 0, 'r', 'e', 't', 'u', 'r', 'n', 0};
    CHECK(p4_luau_load_source(vm, "@bytecode-probe.luau", blob, sizeof blob) == LUA_ERRSYNTAX);
    CHECK_VM(vm, witness_error_has(vm, "bytecode-probe.luau:1"));
    CHECK(lua_gettop(L) == 0);
    CHECK(p4_luau_check_source(vm, "@bytecode-probe.luau", blob, sizeof blob) == LUA_ERRSYNTAX);
    /* A leading zero is how a compiler error blob starts. luau_load would report
     * the trailing text as a compile error; the lexer treats NUL as end of input,
     * so the compiled chunk is empty and returns nothing. */
    static const char error_blob[] = "\0:1: injected loader message";
    CHECK(p4_luau_load_source(vm, "@error-blob.luau", error_blob, sizeof error_blob - 1) == LUA_OK);
    CHECK(p4_luau_error(vm)[0] == 0);
    CHECK(p4_luau_run(vm, 0, LUA_MULTRET) == LUA_OK);
    CHECK(lua_gettop(L) == 0);
    CHECK(!witness_error_has(vm, "injected"));
    return 0;
}

/* Exposed libraries keep their bounds checks; host memory is never addressable. */
static int library_bounds(p4_luau *vm)
{
    CHECK_VM(vm, witness_run(vm, "@globals-bounds.luau",
        "local function fails(pattern, f, ...)\n"
        "  local ok, err = pcall(f, ...)\n"
        "  assert(not ok, 'out-of-bounds access succeeded')\n"
        "  assert(string.find(tostring(err), pattern, 1, true), tostring(err))\n"
        "end\n"
        "local b = buffer.create(8)\n"
        "fails('out of bounds', buffer.readu32, b, 5)\n"
        "fails('out of bounds', buffer.readu8, b, -1)\n"
        "fails('out of bounds', buffer.writef64, b, 1, 0)\n"
        "fails('out of bounds', buffer.readstring, b, 0, 9)\n"
        "fails('out of bounds', buffer.copy, b, 4, buffer.create(8), 0, 8)\n"
        "fails('too short', string.unpack, 'i4', 'ab')\n"
        "local p = newproxy(true)\n"
        "getmetatable(p).__index = function(_, k) return k end\n"
        "assert(p.anything == 'anything')\n"
        "return true", 1) == LUA_OK);
    lua_pop(p4_luau_state(vm), 1);
    return 0;
}

/* Normal environment and coroutine semantics remain inside the sandbox. */
static int environment_semantics(p4_luau *vm)
{
    CHECK_VM(vm, witness_run(vm, "@globals-env.luau",
        "local env = setmetatable({}, {__index = getfenv(1)})\n"
        "local function f() return marker end\n"
        "setfenv(f, env)\n"
        "env.marker = 5\n"
        "assert(f() == 5 and marker == nil)\n"
        "local co = coroutine.create(function(a)\n"
        "  local b = coroutine.yield(a + host_probe() * 0)\n"
        "  return b * 2, shadow\n"
        "end)\n"
        "local ok, v = coroutine.resume(co, 20)\n"
        "assert(ok and v == 20)\n"
        "local ok2, w, s = coroutine.resume(co, 11)\n"
        "assert(ok2 and w == 22 and s == 41 and coroutine.status(co) == 'dead')\n"
        "return true", 1) == LUA_OK);
    lua_pop(p4_luau_state(vm), 1);
    return 0;
}

int main(void)
{
    struct host host = {0};
    p4_luau *vm = create(&host);
    CHECK(vm != NULL);
    SECTION(exact_global_set(vm));
    SECTION(libraries_frozen_and_inert(vm));
    SECTION(denied_capabilities(vm));
    SECTION(readonly_mutations(vm));
    SECTION(per_vm_environment(vm, &host));
    SECTION(no_bytecode_admission(vm));
    SECTION(library_bounds(vm));
    SECTION(environment_semantics(vm));
    CHECK(lua_gettop(p4_luau_state(vm)) == 0);
    p4_luau_destroy(vm);
    puts("sandbox witness globals: exact frozen global set, denied os/io/debug/loaders, readonly "
         "builtins, per-VM environments, no bytecode admission and library bounds passed");
    return 0;
}
