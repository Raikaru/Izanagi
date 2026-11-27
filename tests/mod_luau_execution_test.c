/* battle.execution@1 on the real VM and reducer, with synthetic guest actors.
 * No game, renderer, retail bytes or pretend execution provider. */
#undef NDEBUG
#include "lua_runtime.h"
#include "native_bindings.h"
#include "guest_test.h"
#include "p4pc/battle_action_exec_native.h"
#include "p4pc/gmem.h"
#include "p4pc/mods.h"
#include "p4pc/platform.h"
#include "p4pc/image.h"
#include "p4pc/mod_events.h"
#include "p4pc/mod_failure.h"
#include "p4pc/mod_memory.h"
#include "p4pc/os.h"
#include "p4pc/tables.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t mod_test_unsigned(uint32_t);
int16_t mod_test_narrow(int8_t, uint16_t);
float mod_test_mixed(int, float, unsigned char *, double);
uint32_t *mod_test_pointer(uint32_t *);
int mod_test_recursive(int);
void mod_test_void(uint32_t *);
const p4pc_native_t p4pc_native_table[] = {
    {0x100100, (void *)mod_test_unsigned, "mod_test_unsigned", NULL},
    {0x100120, (void *)mod_test_narrow, "mod_test_narrow", NULL},
    {0x100140, (void *)mod_test_mixed, "mod_test_mixed", NULL},
    {0x100160, (void *)mod_test_pointer, "mod_test_pointer", NULL},
    {0x100180, (void *)mod_test_recursive, "mod_test_recursive", NULL},
    {0x1001a0, (void *)mod_test_void, "mod_test_void", NULL}};
const size_t p4pc_native_count = sizeof p4pc_native_table / sizeof *p4pc_native_table;
const p4pc_patch_t p4pc_patch_table[] = {{0, NULL, NULL}};
const size_t p4pc_patch_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {{NULL, NULL}};
const size_t p4pc_hostfn_count = 0;
const p4pc_image_guest_range_t *p4pc_image_guest_ranges(size_t *count) { *count = 0; return NULL; }
int p4pc_heap_guest_span(uint32_t *begin, uint32_t *end) { (void)begin; (void)end; return -1; }
int p4pc_heap_guest_host_overlap(uint32_t address, size_t size) { (void)address; (void)size; return 0; }
int32_t func_004216d0(void);
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "execution fixture line %d: %s\n", __LINE__, #x); exit(1); } } while (0)
static void recover_failure(size_t owner, const char *reason, void *user)
{
    CHECK(owner == *(size_t *)user);
    fprintf(stderr, "execution_callback_recovery: %s\n", reason);
    fflush(stderr);
    _Exit(42);
}

static int order;
static void other_battle_part(lua_State *L)
{
    lua_newtable(L);
    lua_pushboolean(L, 1); lua_setfield(L, -2, order == 2 ? "execution" : "retained");
    p4_lua_namespace_merge(L, "battle");
}
static void initialize(lua_State *L)
{
    if (order != 1) other_battle_part(L);
    p4_lua_battle_execution_initialize(L);
    if (order == 1) other_battle_part(L);
}
static const P4LuaNativeBindings bindings = {initialize, p4_lua_battle_execution_close};
static void run(P4LuaMod *mod, const char *script)
{
    int result = p4_lua_compile(mod, script, strlen(script), "@execution-fixture");
    if (!result) result = p4_lua_execute(mod);
    if (result) fprintf(stderr, "%s\n%s\n", p4_lua_error(mod), script);
    CHECK(!result);
}
static int snapshot_probe(lua_State *L)
{
    lua_getglobal(L, "snapshotProbe"); lua_call(L, 0, 0); return 0;
}
static void put(const char *path, const char *text)
{
    FILE *f = fopen(path, "wb"); CHECK(f && fputs(text, f) >= 0 && !fclose(f));
}
static uint32_t action, unit, data, battle;
static P4ModMemoryRegion *data_region;
static void actor(void)
{
    battle = P4_H2G(p4t_galloc(0x200, 16));
    action = P4_H2G(p4t_galloc(0x458, 16));
    unit = P4_H2G(p4t_galloc(0xa70, 16));
    data = P4_H2G(p4t_galloc(0x200, 16));
    /* guest_test's arena is outside retail RAM. Declare precisely the native
     * synthetic DatUnit span; do not weaken the production memory allowlist. */
    data_region = p4_mod_memory_declare(data, 0x200, P4_MOD_MEMORY_READ | P4_MOD_MEMORY_WRITE);
    CHECK(data_region);
    P4_G32(action) = 0x89abcdef; P4_G32(action + 4) = 0x01234567;
    P4_G32(action + 0x30) = unit;
    P4_G32(unit + 0xa64) = data; P4_G32(unit + 0xa8) = 0x60000001;
    P4_G16(unit + 0xa4) = 311; P4_G8(unit + 0xa2) = 1;
    P4_G32(battle + 0x174) = action;
    P4_GPDATA(uint32_t, 0xb3ac) = battle;
}
static int witness(int argc, char **argv)
{
    if (argc != 2 && (argc != 3 || strcmp(argv[2], "--callback-failure"))) return 2;
#ifdef P4_GUEST64
    for (size_t i = 0; i < p4pc_native_count; ++i) p4_fn_register(p4pc_native_table[i].addr, p4pc_native_table[i].fn);
#endif
    CHECK(func_004216d0() == 1);
    CHECK(!p4t_guest_map(P4_GPADDR(0xb3ac), 4));
    CHECK(!p4_lua_bind_native(&bindings));
    char mods[1200], file[1300];
    p4_os_mkdir(argv[1]); snprintf(mods, sizeof mods, "%s/mods", argv[1]); p4_os_mkdir(mods);
    static const char *const ids[] = {"test.execution", "test.execution2"};
    for (unsigned i = 0; i < 2; ++i) {
        snprintf(file, sizeof file, "%s/%s", mods, ids[i]); p4_os_mkdir(file);
        char manifest[256];
        snprintf(manifest, sizeof manifest, "[mod]\nid=%s\nname=Execution fixture\nversion=1.0\napi=1.6\nrequires=battle.execution@1\npriority=%u\norder=0\n", ids[i], i);
        snprintf(file, sizeof file, "%s/%s/mod.ini", mods, ids[i]); put(file, manifest);
        snprintf(file, sizeof file, "%s/%s/main.luau", mods, ids[i]); put(file, "-- fixture\n");
    }
    CHECK(!p4_mods_stage(mods, "test.execution,test.execution2") && !p4_package_registry_publish());
    order = 2; CHECK(!p4_lua_create(p4_packages_provider(0)));
    order = 0; P4LuaMod *a = p4_lua_create(p4_packages_provider(0));
    order = 1; P4LuaMod *b = p4_lua_create(p4_packages_provider(1));
    CHECK(a && b);
    static const char setup[] =
        "assert(p4.has('battle.execution@1') and not p4.has('battle.execution@2'))\n"
        "assert(p4.battle.retained and table.isfrozen(p4.battle) and table.isfrozen(p4.battle.execution))\n"
        "assert(not pcall(function() p4.battle.execution = false end))\n"
        "assert(not pcall(p4.battle.execution.snapshot, 1))\n"
        "assert(not pcall(p4.battle.execution.snapshot, '123'))\n"
        "assert(not pcall(p4.battle.execution.snapshot, 'ABCDEF0000000000'))\n"
        "assert(p4.battle.execution.snapshot('ffffffffffffffff') == nil)\n"
        "seen = {}; lost = 0\n"
        "watch = p4.battle.execution.on(function(e)\n"
        " assert(table.isfrozen(e)); table.insert(seen, e.kind)\n"
        " assert(not pcall(function() watch:disable() end))\n"
        " assert(not pcall(p4.call, 0x100100, 'II', 1))\n"
        " if e.kind == 'lost' then lost += 1 end\n"
        " local x = e.execution\n"
        " if x then\n"
        "  assert(table.isfrozen(x) and table.isfrozen(x.actor))\n"
        "  assert(x.actor.id == 311 and x.actor.uid == 0x60000001)\n"
        "  assert(x.action_uid == '0123456789abcdef' and #x.token == 16)\n"
        "  saved = x.token\n"
        "  if e.kind == 'admitted' then\n"
        "   assert(p4.mem.read('u8', x.actor.data + 0x100) == 0)\n"
        "   local n = if p4.id == 'test.execution' then 0 else 1\n"
        "   assert(p4.mem.read('u8', x.actor.data + 0x101) == n)\n"
        "   p4.mem.write('u8', x.actor.data + 0x101, n + 1)\n"
        "  end\n"
        "  if e.kind == 'terminal' then finished = x.outcome end\n"
        " end\n"
        "end)\n";
    run(a, setup); run(b, setup);
    actor();
    if (argc == 3) {
        CHECK(!p4_mod_failure_handler(recover_failure, &a->owner));
        run(a, "p4.battle.execution.on(function() error('execution deliberate callback failure') end)");
        p4_action_exec_battle_begin();
        CHECK(0); /* native recovery must not resume with a silently dead observer */
    }
    p4_action_exec_battle_begin();
    p4_action_exec_native_admit(action);
    P4_G8(data + 0x100) = 1; /* synchronous initializer effect follows both listeners */
    char script[512];
    snprintf(script, sizeof script,
        "assert(seen[1] == 'battle_start' and seen[2] == 'admitted')\n"
        "assert(p4.battle.execution.unit(%u).uid == 0x60000001)\n"
        "assert(p4.battle.execution.current(%u).token == saved)\n"
        "assert(not pcall(p4.battle.execution.on, function() end))", data, action);
    run(a, script); run(b, script);
    snprintf(script, sizeof script,
        "function snapshotProbe() assert(not pcall(p4.battle.execution.unit, %u)) end", data);
    run(a, script);
    CHECK(!p4_lua_invoke(a, snapshot_probe, NULL, 0, P4_LUA_SNAPSHOT));
    run(b, "watch:disable(); assert(not pcall(function() watch:enable() end))");
    P4_G16(action + 0x6c) = 1; P4_G16(action + 0x6e) = 149;
    p4_action_exec_native_command(action);
    uint64_t previous = p4_action_exec_enter_action(action);
    p4_action_exec_packet_add(1000, 0, 0x700);
    p4_action_exec_native_end(action);
    p4_action_exec_end_processed(action, 0);
    p4_action_exec_standby(action);
    p4_action_exec_leave(previous);
    run(a, "assert(seen[#seen] == 'committed' and finished == nil and p4.battle.execution.snapshot(saved).pending_packets == 1)");
    p4_action_exec_packet_remove(1000, 1);
    run(a, "assert(finished == 'resolved' and p4.battle.execution.snapshot(saved).sealed)");
    /* No running actor: unknown work still must notify, never leave a policy open. */
    p4_action_exec_packet_add(1001, 0, 0x702);
    run(a, "assert(lost == 1 and seen[#seen] == 'lost')");
    p4_action_exec_packet_remove(1001, 1);
    p4_action_exec_release();
    run(a, "assert(seen[#seen] == 'battle_end'); watch:disable(); watch:enable()");
    run(b, "watch:enable()");
    P4_G8(data + 0x100) = 0;
    P4_G8(data + 0x101) = 0;
    p4_action_exec_battle_begin();
    p4_action_exec_native_admit(action);
    p4_lua_destroy(a); /* b remains attached; no userdata pointer survives a */
    p4_action_exec_resume(999); /* orphan/lost marker and uncertainty, no invented execution */
    run(b, "assert(lost == 1 and p4.battle.execution.current(999) == nil)");
    p4_action_exec_release();
    run(b, "assert(seen[#seen] == 'battle_end'); watch:remove(); watch:remove(); assert(not pcall(function() watch:enable() end))");
    p4_lua_destroy(b);
    p4_mod_memory_revoke(data_region);
    p4_mod_events_clear(); p4_package_registry_retire();
    puts("mod_luau_execution: PASS (real reducer/VM, synchronous immutable facts, tokens, composition, live identities, ownership and loss/lifetime refusal)");
    return 0;
}
int main(int argc, char **argv)
{
    p4_os_init(); p4_os_setenv("P4PC_VIRTUAL_CLOCK", "1", 1); p4_os_setenv("P4PC_TURBO", "0", 1);
    return p4t_on_guest_stack(witness, argc, argv);
}
