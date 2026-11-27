/* Usable normal mod behavior inside the sandbox, through the same host
 * patterns as src/platform/mods/lua_runtime.c: an 8 MiB cap, per-invocation
 * fuel charged by the interrupt (GC safepoints included), a frozen `p4` API
 * table registered in config.initialize, handlers retained in the registry,
 * callbacks run with lua_pcall on a host-created thread, and a retained main
 * chunk executed under startup fuel.
 *
 * The mod is ordinary idiomatic Luau: metatable classes, closures, type
 * annotations, compound assignment, a long-lived coroutine task, buffers,
 * string.pack/format, utf8, bit32, table.sort with a comparator and script-side
 * pcall. Every handler result is checked against an independent C model. */
#include "witness.h"

#define MEMORY_LIMIT (8u * 1024u * 1024u)
#define STARTUP_FUEL 65536u
#define CALLBACK_FUEL 8192u

struct mod {
    p4_luau *vm;
    lua_State *thread;
    int thread_ref;
    int handlers_ref;
    int main_ref;
    unsigned fuel;
    int active;
    int budget_failed;
    char log[4096];
    size_t log_size;
    char error[2048];
};

/* As production: charge every safepoint while a callback frame is active. */
static int interrupt(void *user, int gc)
{
    struct mod *mod = user;
    (void)gc;
    if (!mod->active) return mod->budget_failed;
    if (mod->fuel == 0) mod->budget_failed = 1;
    else --mod->fuel;
    return mod->budget_failed;
}

static int api_log(lua_State *L)
{
    struct mod *mod = p4_luau_user(L);
    size_t size;
    const char *message = luaL_checklstring(L, 1, &size);
    if (size > 1024 || mod->log_size + size + 1 >= sizeof mod->log) luaL_error(L, "mod log message too large");
    memcpy(mod->log + mod->log_size, message, size);
    mod->log_size += size;
    mod->log[mod->log_size++] = '\n';
    mod->log[mod->log_size] = 0;
    return 0;
}
static int api_budget(lua_State *L)
{
    struct mod *mod = p4_luau_user(L);
    lua_pushinteger(L, (int)mod->fuel);
    return 1;
}
static int api_on(lua_State *L)
{
    struct mod *mod = p4_luau_user(L);
    const char *name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    lua_getref(L, mod->handlers_ref);
    lua_getfield(L, -1, name);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        lua_newtable(L);
        lua_pushvalue(L, -1);
        lua_setfield(L, -3, name);
    }
    int count = lua_objlen(L, -1);
    lua_pushvalue(L, 2);
    lua_rawseti(L, -2, count + 1);
    return 0;
}

static void initialize(lua_State *L, void *user)
{
    struct mod *mod = user;
    lua_newtable(L);
    mod->handlers_ref = lua_ref(L, -1); /* registry-only: stays mutable */
    lua_pop(L, 1);
    lua_newtable(L);
    lua_pushstring(L, "1.0");
    lua_setfield(L, -2, "api_version");
    lua_pushstring(L, "witness.mod");
    lua_setfield(L, -2, "id");
    lua_pushcfunction(L, api_log, "p4.log");
    lua_setfield(L, -2, "log");
    lua_pushcfunction(L, api_budget, "p4.budget");
    lua_setfield(L, -2, "budget");
    lua_newtable(L);
    lua_pushcfunction(L, api_on, "p4.events.on");
    lua_setfield(L, -2, "on");
    lua_setfield(L, -2, "events");
    lua_setglobal(L, "p4");
}

static const char main_source[] =
    "--!strict\n"
    "local Mod = {}\n"
    "Mod.__index = Mod\n"
    "function Mod.new(name: string)\n"
    "  return setmetatable({ name = name, hits = 0, last = 0 }, Mod)\n"
    "end\n"
    "function Mod.hit(self, amount: number): number\n"
    "  self.hits += amount\n"
    "  self.last = amount\n"
    "  return self.hits\n"
    "end\n"
    "local party = { Mod.new('yu'), Mod.new('yosuke'), Mod.new('chie') }\n"
    "local seen = setmetatable({}, { __index = function() return 0 end })\n"
    "p4.events.on('battle_start', function(level: number)\n"
    "  for i, member in ipairs(party) do\n"
    "    member:hit(level * i)\n"
    "    seen[member.name] += 1\n"
    "  end\n"
    "  assert(seen.yu == seen.chie and party[2].last == level * 2)\n"
    "  return party[1].hits + party[2].hits * 1000 + party[3].hits * 1000000\n"
    "end)\n"
    "local task = coroutine.create(function()\n"
    "  local acc = 0\n"
    "  while true do\n"
    "    local frame = coroutine.yield(acc)\n"
    "    acc += frame\n"
    "  end\n"
    "end)\n"
    "assert(coroutine.resume(task))\n"
    "p4.events.on('tick', function(frame: number)\n"
    "  local ok, acc = coroutine.resume(task, frame)\n"
    "  assert(ok, acc)\n"
    "  return acc\n"
    "end)\n"
    "p4.events.on('encode', function(text: string)\n"
    "  local b = buffer.create(#text + 4)\n"
    "  buffer.writeu32(b, 0, utf8.len(text))\n"
    "  buffer.writestring(b, 4, text)\n"
    "  local packed = string.pack('<I4', buffer.readu32(b, 0))\n"
    "  return string.format('%d/%d/%s/%08x', utf8.len(text), #packed,\n"
    "    buffer.readstring(b, 4, #text):upper(), bit32.bxor(0x0f0f0f0f, buffer.readu32(b, 0)))\n"
    "end)\n"
    "p4.events.on('rank', function()\n"
    "  local scores = {}\n"
    "  for _, member in ipairs(party) do\n"
    "    table.insert(scores, { name = member.name, score = member.hits % 97 })\n"
    "  end\n"
    "  table.sort(scores, function(a, b)\n"
    "    if a.score ~= b.score then return a.score > b.score end\n"
    "    return a.name < b.name\n"
    "  end)\n"
    "  local names = {}\n"
    "  for i, entry in ipairs(scores) do names[i] = entry.name end\n"
    "  return table.concat(names, ',')\n"
    "end)\n"
    "p4.events.on('fail', function(code: number)\n"
    "  error('handler failure ' .. code)\n"
    "end)\n"
    "p4.events.on('spin', function() while true do end end)\n"
    "p4.events.on('ping', function() return 'pong' end)\n"
    "p4.events.on('budget', function() return p4.budget() end)\n"
    "p4.events.on('frozen', function()\n"
    "  local api: any = p4\n"
    "  local ok = pcall(function() api.log = nil end)\n"
    "  return not ok and type(p4.log) == 'function'\n"
    "end)\n"
    "ModState = { version = 2, members = #party }\n"
    "p4.log(string.format('loaded %s api %s', p4.id, p4.api_version))\n";

static int protected_call(struct mod *mod, int nargs, int nresults, unsigned fuel)
{
    lua_State *T = mod->thread;
    int base = lua_gettop(T) - nargs - 1;
    mod->error[0] = 0;
    mod->fuel = fuel;
    mod->active = 1;
    int status = lua_pcall(T, nargs, nresults, 0);
    mod->active = 0;
    if (mod->budget_failed) {
        snprintf(mod->error, sizeof mod->error, "budget: %s",
            status != LUA_OK && lua_type(T, -1) == LUA_TSTRING ? lua_tostring(T, -1) : "exhausted");
        lua_settop(T, base);
        return LUA_ERRRUN;
    }
    if (status != LUA_OK) {
        snprintf(mod->error, sizeof mod->error, "%s",
            lua_type(T, -1) == LUA_TSTRING ? lua_tostring(T, -1) : "non-string Luau error");
        lua_pop(T, 1);
    }
    return status;
}

/* Pushes the single handler registered for an event onto the callback thread. */
static int push_handler(struct mod *mod, const char *event)
{
    lua_State *T = mod->thread;
    lua_getref(T, mod->handlers_ref);
    lua_getfield(T, -1, event);
    int ok = lua_istable(T, -1) && lua_objlen(T, -1) == 1;
    if (ok) lua_rawgeti(T, -1, 1);
    else lua_pushnil(T);
    lua_remove(T, -2);
    lua_remove(T, -2);
    return ok;
}

static int fire_number(struct mod *mod, const char *event, double argument, double *result)
{
    lua_State *T = mod->thread;
    CHECK(push_handler(mod, event));
    lua_pushnumber(T, argument);
    int status = protected_call(mod, 1, 1, CALLBACK_FUEL);
    if (status != LUA_OK) return status;
    CHECK(lua_type(T, -1) == LUA_TNUMBER);
    *result = lua_tonumber(T, -1);
    lua_pop(T, 1);
    return LUA_OK;
}

static int fire_string(struct mod *mod, const char *event, const char *argument, char *out, size_t size)
{
    lua_State *T = mod->thread;
    CHECK(push_handler(mod, event));
    if (argument) lua_pushstring(T, argument);
    int status = protected_call(mod, argument ? 1 : 0, 1, CALLBACK_FUEL);
    if (status != LUA_OK) return status;
    CHECK(lua_type(T, -1) == LUA_TSTRING);
    snprintf(out, size, "%s", lua_tostring(T, -1));
    lua_pop(T, 1);
    return LUA_OK;
}

static int mod_open(struct mod *mod)
{
    memset(mod, 0, sizeof *mod);
    mod->handlers_ref = mod->main_ref = mod->thread_ref = LUA_NOREF;
    p4_luau_config config = {MEMORY_LIMIT, interrupt, mod, initialize, 1024u * 1024u};
    mod->vm = p4_luau_create(&config);
    CHECK(mod->vm != NULL);
    lua_State *L = p4_luau_state(mod->vm);
    struct thread_create created = {NULL, LUA_NOREF};
    CHECK(lua_cpcall(L, witness_create_thread, &created) == LUA_OK);
    mod->thread = created.state;
    mod->thread_ref = created.reference;

    CHECK_VM(mod->vm, p4_luau_check_source(mod->vm, "@mods/witness/main.luau", main_source,
        sizeof main_source - 1) == LUA_OK);
    CHECK_VM(mod->vm, p4_luau_load_source(mod->vm, "@mods/witness/main.luau", main_source,
        sizeof main_source - 1) == LUA_OK);
    mod->main_ref = lua_ref(L, -1);
    lua_pop(L, 1);
    lua_getref(mod->thread, mod->main_ref);
    if (protected_call(mod, 0, 0, STARTUP_FUEL) != LUA_OK) {
        fprintf(stderr, "main chunk failed: %s\n", mod->error);
        return 1;
    }
    CHECK(strstr(mod->log, "loaded witness.mod api 1.0\n") != NULL);
    CHECK(lua_gettop(L) == 0 && lua_gettop(mod->thread) == 0);
    return 0;
}

static void mod_close(struct mod *mod)
{
    p4_luau_destroy(mod->vm);
    mod->vm = NULL;
}

/* Independent model of the script's party arithmetic. */
static void expected_rank(double level_sum, char *out, size_t size)
{
    static const char *const names[3] = {"yu", "yosuke", "chie"};
    long long score[3];
    int order[3] = {0, 1, 2};
    for (int i = 0; i < 3; ++i) score[i] = ((long long)level_sum * (i + 1)) % 97;
    for (int i = 0; i < 3; ++i)
        for (int j = i + 1; j < 3; ++j) {
            int a = order[i], b = order[j];
            int swap = score[b] > score[a] || (score[b] == score[a] && strcmp(names[b], names[a]) < 0);
            if (swap) { order[i] = b; order[j] = a; }
        }
    snprintf(out, size, "%s,%s,%s", names[order[0]], names[order[1]], names[order[2]]);
}

static int normal_behavior(struct mod *mod)
{
    lua_State *L = p4_luau_state(mod->vm);
    double result = 0, level_sum = 0, frame_sum = 0;
    char text[256], expected[256];
    size_t mid_used = 0;
    for (unsigned i = 1; i <= 2000; ++i) {
        double level = (double)(i % 50 + 1);
        level_sum += level;
        frame_sum += i;
        CHECK(fire_number(mod, "battle_start", level, &result) == LUA_OK);
        CHECK(result == level_sum * 3002001.0);
        CHECK(fire_number(mod, "tick", (double)i, &result) == LUA_OK);
        CHECK(result == frame_sum);
        if (i % 64 == 0) {
            CHECK(fire_string(mod, "rank", NULL, text, sizeof text) == LUA_OK);
            expected_rank(level_sum, expected, sizeof expected);
            if (strcmp(text, expected) != 0) {
                fprintf(stderr, "rank after %u battles: %s, expected %s\n", i, text, expected);
                return 1;
            }
        }
        /* Production's per-frame pressure relief: a bounded collector step. */
        lua_gc(L, LUA_GCSTEP, 0);
        if (i == 200) mid_used = witness_collect(mod->vm);
    }
    /* Steady state: thousands of callbacks retain nothing new. */
    size_t end_used = witness_collect(mod->vm);
    if (end_used > mid_used + 16u * 1024u) {
        fprintf(stderr, "steady-state growth: %zu -> %zu bytes\n", mid_used, end_used);
        return 1;
    }

    CHECK(fire_string(mod, "encode", "Inaba", text, sizeof text) == LUA_OK);
    CHECK(strcmp(text, "5/4/INABA/0f0f0f0a") == 0);
    CHECK(push_handler(mod, "frozen"));
    CHECK(protected_call(mod, 0, 1, CALLBACK_FUEL) == LUA_OK);
    CHECK(lua_toboolean(mod->thread, -1));
    lua_pop(mod->thread, 1);
    CHECK(fire_number(mod, "budget", 0, &result) == LUA_OK);
    CHECK(result > 0 && result < CALLBACK_FUEL);

    /* A later chunk sees the mod's globals but not its locals. */
    CHECK_VM(mod->vm, witness_run(mod->vm, "@mods/witness/console.luau",
        "return ModState.version, ModState.members, type(Mod)", 3) == LUA_OK);
    CHECK(lua_tointeger(L, 1) == 2 && lua_tointeger(L, 2) == 3);
    CHECK(lua_type(L, 3) == LUA_TSTRING && strcmp(lua_tostring(L, 3), "nil") == 0);
    lua_settop(L, 0);
    return 0;
}

/* Handler failures are isolated: diagnostics carry the chunk position, and the
 * VM, the long-lived coroutine task and other handlers keep working. */
static int failures_are_contained(struct mod *mod)
{
    double result = 0, before = 0;
    char text[64];
    CHECK(fire_number(mod, "tick", 0, &before) == LUA_OK);
    CHECK(fire_number(mod, "fail", 3, &result) == LUA_ERRRUN);
    CHECK(strstr(mod->error, "main.luau:") != NULL && strstr(mod->error, "handler failure 3") != NULL);
    CHECK(fire_number(mod, "tick", 5, &result) == LUA_OK && result == before + 5);

    CHECK(fire_string(mod, "spin", NULL, text, sizeof text) == LUA_ERRRUN);
    CHECK(mod->budget_failed);
    CHECK(strstr(mod->error, "Execution budget exhausted") != NULL);
    /* Still latched: nothing runs until the host resets the budget. */
    CHECK(fire_string(mod, "ping", NULL, text, sizeof text) == LUA_ERRRUN);
    CHECK(witness_run(mod->vm, "@mods/witness/latched.luau", "return 1", 1) == LUA_ERRRUN);
    mod->budget_failed = 0;
    p4_luau_reset_interrupt(mod->vm);
    CHECK(fire_string(mod, "ping", NULL, text, sizeof text) == LUA_OK && strcmp(text, "pong") == 0);
    CHECK(fire_number(mod, "tick", 1, &result) == LUA_OK && result == before + 6);
    CHECK(lua_gettop(mod->thread) == 0 && lua_gettop(p4_luau_state(mod->vm)) == 0);
    return 0;
}

/* Two mods with the same script share nothing. */
static int mods_are_isolated(struct mod *first)
{
    struct mod second;
    double result = 0;
    if (mod_open(&second)) return 1;
    CHECK(fire_number(&second, "battle_start", 7, &result) == LUA_OK);
    CHECK(result == 7.0 * 3002001.0);
    CHECK(fire_number(&second, "tick", 9, &result) == LUA_OK && result == 9);
    CHECK(strcmp(second.log, "loaded witness.mod api 1.0\n") == 0);
    mod_close(&second);
    CHECK(fire_number(first, "battle_start", 1, &result) == LUA_OK);
    CHECK(result > 7.0 * 3002001.0);
    return 0;
}

int main(void)
{
    struct mod mod;
    SECTION(mod_open(&mod));
    SECTION(normal_behavior(&mod));
    SECTION(failures_are_contained(&mod));
    SECTION(mods_are_isolated(&mod));
    CHECK(p4_luau_memory_used(mod.vm) <= MEMORY_LIMIT);
    mod_close(&mod);
    puts("sandbox witness mod: production-shaped mod load, 2000-frame event workload, coroutine task, "
         "frozen API, contained failures, budget reset and isolated mods passed");
    return 0;
}
