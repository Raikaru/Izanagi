/* p4.status (src/platform/mods/lua_status.c) on the actual Luau VM and native
 * event core, over a scripted side-ailment service. Live units come from a
 * guest battle action list, as in the game. The namespace is shared with the
 * real wear-off stage (lua_stages.c), built in both initialization orders. */
#include "lua_runtime.h"
#include "native_bindings.h"
#include "guest_test.h"
#include "p4pc/game_side_ailment.h"
#include "p4pc/game_wearoff_stage.h"
#include "p4pc/image.h"
#include "p4pc/mods.h"
#include "p4pc/mod_events.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "p4pc/tables.h"
#include <stdint.h>
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
    {0x1001a0, (void *)mod_test_void, "mod_test_void", NULL}
};
const size_t p4pc_native_count = sizeof p4pc_native_table / sizeof p4pc_native_table[0];
const p4pc_patch_t p4pc_patch_table[] = {{0, NULL, NULL}};
const size_t p4pc_patch_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {{NULL, NULL}};
const size_t p4pc_hostfn_count = 0;
const p4pc_image_guest_range_t *p4pc_image_guest_ranges(size_t *count) { *count = 0; return NULL; }
int p4pc_heap_guest_span(uint32_t *begin, uint32_t *end) { (void)begin; (void)end; return -1; }
int p4pc_heap_guest_host_overlap(uint32_t address, size_t size) { (void)address; (void)size; return 0; }
int32_t func_004216d0(void);
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_luau_status line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

/* Scripted service: one held leaf per unit, a gate, protection windows. */
static const P4SideAilmentDef defs[2] = {
    {.leaf = 24, .flags = 15, .cure_mask = 0x16, .default_from = 2, .crit_bonus = 100, .key = "test.status:freeze"},
    {.leaf = 25, .flags = 7, .cure_mask = 0x16, .default_from = 3, .crit_bonus = 100, .transfer = 25,
     .key = "test.status:shock"}};
enum { UNITS = 3 };
static uint32_t units[UNITS], held[UNITS], landings[UNITS], guarded[UNITS];
static int in_battle, gate, gate_calls, protect_calls, delivering;
static P4SideAilmentObserver observer;
static int slot_of(uint32_t unit)
{
    for (int i = 0; i < UNITS; ++i) if (units[i] == unit) return i;
    return -1;
}
size_t p4_side_ailment_count(void) { return 2; }
const P4SideAilmentDef *p4_side_ailment_at(size_t index) { return index < 2 ? &defs[index] : NULL; }
const P4SideAilmentDef *p4_side_ailment_find_leaf(uint32_t leaf)
{
    return leaf == 24 ? &defs[0] : leaf == 25 ? &defs[1] : NULL;
}
const P4SideAilmentDef *p4_side_ailment_find_key(const char *key)
{
    for (size_t i = 0; i < 2; ++i) if (!strcmp(defs[i].key, key)) return &defs[i];
    return NULL;
}
int p4_side_ailment_query(uint32_t unit, const P4SideAilmentDef **def)
{
    const int i = slot_of(unit);
    if (i < 0 || !in_battle || !held[i]) return 0;
    *def = p4_side_ailment_find_leaf(held[i]);
    return 1;
}
static void deliver(uint32_t unit, uint32_t leaf, uint32_t event, uint32_t reason)
{
    if (!observer) return;
    delivering = 1;
    observer(unit, leaf, event, reason);
    delivering = 0;
}
int p4_side_ailment_set(uint32_t unit, uint32_t leaf, uint32_t reason)
{
    const int i = slot_of(unit);
    if (!in_battle || !gate || delivering) return P4_TABLE_NOT_READY;
    if (i < 0 || guarded[i]) return P4_TABLE_INVALID;
    if (held[i]) return P4_TABLE_CONFLICT;
    held[i] = leaf;
    ++landings[i];
    deliver(unit, leaf, P4_SIDE_EVENT_LANDED, reason);
    return P4_TABLE_OK;
}
uint32_t p4_side_ailment_clear(uint32_t unit, uint32_t reason)
{
    const int i = slot_of(unit);
    if (i < 0 || !held[i] || delivering) return P4_SIDE_DEFAULT_NONE;
    const uint32_t leaf = held[i];
    held[i] = 0;
    deliver(unit, leaf, P4_SIDE_EVENT_CLEARED, reason);
    return leaf;
}
uint32_t p4_side_ailment_landings(uint32_t unit, uint32_t leaf)
{
    const int i = slot_of(unit);
    return i >= 0 && leaf == 24 ? landings[i] : 0;
}
void p4_side_ailment_set_observer(P4SideAilmentObserver sink) { observer = sink; }
int p4_side_ailment_set_enabled(int on)
{
    ++gate_calls;
    if (in_battle) return P4_TABLE_NOT_READY;
    gate = on;
    return P4_TABLE_OK;
}
int p4_side_ailment_protect(uint32_t unit, int on)
{
    ++protect_calls;
    const int i = slot_of(unit);
    if (!in_battle || !gate) return P4_TABLE_NOT_READY;
    if (i < 0) return P4_TABLE_INVALID;
    guarded[i] = (uint32_t)on;
    return P4_TABLE_OK;
}
int p4_side_ailment_protected(uint32_t unit)
{
    const int i = slot_of(unit);
    return i >= 0 && in_battle && guarded[i];
}
const char *p4_game_tables_error(int result) { (void)result; return "scripted refusal"; }

static void run(P4LuaMod *mod, const char *source, int success)
{
    int result = p4_lua_compile(mod, source, strlen(source), "@status-fixture");
    if (!result) result = p4_lua_execute(mod);
    if ((result == 0) != success) fprintf(stderr, "status fixture (%d): %s\n%s\n", result, p4_lua_error(mod), source);
    CHECK((result == 0) == success);
}
static int snapshot_call(lua_State *L)
{
    lua_getglobal(L, (const char *)lua_touserdata(L, 1));
    lua_call(L, 0, 0);
    return 0;
}
static void put(const char *file, const char *text)
{
    FILE *f = fopen(file, "wb");
    CHECK(f && fputs(text, f) >= 0 && !fclose(f));
}
/* p4.status is one table filled by two initializers: the side-ailment service
 * and the wear-off stage. Mod a's VM runs the stage first and mod b's the
 * service first; DUPLICATE adds a third part that also defines query. */
enum { STAGES_FIRST, STATUS_FIRST, DUPLICATE };
static int order;
static void duplicate_query(lua_State *L)
{
    lua_newtable(L);
    lua_pushboolean(L, 1);
    lua_setfield(L, -2, "query");
    p4_lua_namespace_merge(L, "status");
}
static void initialize(lua_State *L)
{
    if (order == DUPLICATE) duplicate_query(L);
    if (order != STATUS_FIRST) p4_lua_stages_initialize(L);
    p4_lua_status_initialize(L);
    if (order == STATUS_FIRST) p4_lua_stages_initialize(L);
}
static void before_destroy(P4LuaMod *mod)
{
    p4_lua_status_close(mod);
    p4_lua_stages_close(mod);
}
static const P4LuaNativeBindings bindings = {initialize, before_destroy};

/* A battle of three units: guest BtlBattleState -> BtlAction list -> BtlUnit -> DatUnit. */
static void battle(void)
{
    uint8_t *state = p4t_galloc(0x200, 16);
    uint32_t previous = 0;
    for (int i = UNITS - 1; i >= 0; --i) {
        uint8_t *dat = p4t_galloc(0x200, 16), *unit = p4t_galloc(0xb00, 16), *action = p4t_galloc(0x460, 16);
        units[i] = P4_H2G(dat);
        P4_GDATA(uint32_t, P4_H2G(unit) + 0xa64) = units[i];
        P4_GDATA(uint32_t, P4_H2G(action) + 0x30) = P4_H2G(unit);
        P4_GDATA(uint32_t, P4_H2G(action) + 0x450) = previous;
        previous = P4_H2G(action);
    }
    P4_GDATA(uint32_t, P4_H2G(state) + 0x174) = previous;
    P4_GPDATA(uint32_t, 0xb3ac) = P4_H2G(state);
}

static int witness(int argc, char **argv)
{
    if (argc != 2) return 2;
#ifdef P4_GUEST64
    for (size_t i = 0; i < p4pc_native_count; i++) p4_fn_register(p4pc_native_table[i].addr, p4pc_native_table[i].fn);
#endif
    CHECK(func_004216d0() == 1);
    CHECK(p4t_guest_map(P4_GPADDR(0xb3ac), 4) == 0);
    CHECK(p4_lua_bind_native(&bindings) == 0);
    char mods[1200], file[1300];
    p4_os_mkdir(argv[1]);
    CHECK(snprintf(mods, sizeof mods, "%s/mods", argv[1]) > 0);
    p4_os_mkdir(mods);
    static const char *const ids[] = {"test.status", "test.status2"};
    for (unsigned i = 0; i < 2; ++i) {
        snprintf(file, sizeof file, "%s/%s", mods, ids[i]);
        p4_os_mkdir(file);
        char manifest[256];
        snprintf(manifest, sizeof manifest,
            "[mod]\nid=%s\nname=Status fixture\nversion=1.0\napi=1.6\nrequires=status@1, events.status@1, status.wearoff@1\n"
            "priority=%u\norder=0\n", ids[i], i);
        snprintf(file, sizeof file, "%s/%s/mod.ini", mods, ids[i]);
        put(file, manifest);
        snprintf(file, sizeof file, "%s/%s/main.luau", mods, ids[i]);
        put(file, "-- fixture\n");
    }
    CHECK(!p4_mods_stage(mods, "test.status,test.status2") && !p4_package_registry_publish());
    /* A field two initializers both define fails the VM rather than replacing one. */
    order = DUPLICATE;
    CHECK(!p4_lua_create(p4_packages_provider(0)));
    order = STAGES_FIRST;
    P4LuaMod *a = p4_lua_create(p4_packages_provider(0));
    order = STATUS_FIRST;
    P4LuaMod *b = p4_lua_create(p4_packages_provider(1));
    CHECK(a && b);
    battle();
    char script[512];

    /* Either order: one readonly p4.status with every field of both keys. */
    static const char shared[] =
        "assert(p4.has('status@1') and p4.has('status.wearoff@1') and p4.has('events.status@1'))\n"
        "local names = {}\n"
        "for name in p4.status do table.insert(names, name) end\n"
        "table.sort(names)\n"
        "assert(table.concat(names, ' ') == 'clear enable key landings protect protected query set wearoff')\n"
        "assert(table.isfrozen(p4.status) and table.isfrozen(p4.status.wearoff))\n"
        "assert(not pcall(function() p4.status.wearoff = nil end) and not pcall(function() p4.status.query = nil end))\n"
        "assert(not pcall(function() p4.status.extra = true end) and not pcall(function() p4.status.wearoff.on = nil end))\n";
    run(a, shared, 1);
    run(b, shared, 1);

    /* A build without a status source refuses the subscription. */
    run(a, "assert(not pcall(p4.events.on, 'status', function() end))", 1);
    p4_lua_status_install();

    /* The gate is per mod: on with the first hold, off with the last release. */
    run(a, "assert(p4.status.enable(true) == true and p4.status.enable(true) == true)", 1);
    CHECK(gate && gate_calls == 1 && a->semantic_used);
    run(b, "assert(p4.status.enable(true) == true)", 1);
    CHECK(gate_calls == 1);
    run(a, "assert(p4.status.enable(false) == true)", 1);
    CHECK(gate && gate_calls == 1); /* b still holds it */
    run(a, "assert(p4.status.enable(true) == true)", 1);
    run(a, "assert(not pcall(p4.status.enable, 1))", 1);

    /* Units: misuse raises, refusals return false and a reason. */
    in_battle = 1;
    snprintf(script, sizeof script,
        "u0, u1, u2 = %u, %u, %u\n"
        "assert(not pcall(p4.status.set, 0x1234, 'test.status:freeze'))\n"
        "assert(not pcall(p4.status.set, u0, 'test.status:burn'))\n"
        "assert(not pcall(p4.status.protect, 0x1234, true))\n"
        "assert(not pcall(p4.status.clear, 0x1234))\n"
        "assert(p4.status.query(u0) == nil and p4.status.key(24) == 'test.status:freeze' and p4.status.key(9) == nil)\n",
        (unsigned)units[0], (unsigned)units[1], (unsigned)units[2]);
    run(a, script, 1);
    snprintf(script, sizeof script, "u0, u1, u2 = %u, %u, %u", (unsigned)units[0], (unsigned)units[1], (unsigned)units[2]);
    run(b, script, 1);

    /* The status event: the source is installed with the first subscription. */
    CHECK(!observer);
    run(a, "seen = {}\n"
           "watch = p4.events.on('status', function(e) table.insert(seen, string.format('%d %d %d', e.leaf, e.kind, e.reason))\n"
           "    assert(p4.status.set(e.unit, 'test.status:shock') == false) end)", 1);
    CHECK(observer);
    run(b, "watch2 = p4.events.on('status', function(e) end)", 1);
    run(a, "assert(p4.status.set(u0, 'test.status:freeze') == true)\n"
           "assert(p4.status.query(u0) == 'test.status:freeze' and p4.status.landings(u0, 'test.status:freeze') == 1)\n"
           "assert(seen[1] == '24 1 3')\n"
           "local ok, why = p4.status.set(u0, 'test.status:shock'); assert(ok == false and why:find('already'))\n"
           "assert(p4.status.clear(u0) == 'test.status:freeze' and p4.status.clear(u0) == nil and seen[2] == '24 2 3')\n", 1);
    run(a, "watch:remove()", 1);
    CHECK(observer);
    run(b, "watch2:disable()", 1);
    CHECK(!observer);
    run(b, "watch2:enable()", 1);
    CHECK(observer);
    run(b, "watch2:remove()", 1);
    CHECK(!observer);

    /* Protection is per mod and per unit. */
    run(a, "assert(p4.status.protect(u1, true) == true and p4.status.protected(u1))\n"
           "local ok, why = p4.status.set(u1, 'test.status:freeze'); assert(ok == false and why:find('protected'))", 1);
    CHECK(protect_calls == 1);
    run(b, "assert(p4.status.protect(u1, true) == true)", 1);
    CHECK(protect_calls == 1);
    run(a, "assert(p4.status.protect(u1, false) == true and p4.status.protected(u1))", 1);
    CHECK(protect_calls == 1); /* b still holds it */
    run(a, "assert(p4.status.protect(u2, true) == true)", 1);

    /* Snapshot callbacks may read, never mutate. */
    run(a, "function snap_set() p4.status.set(u0, 'test.status:freeze') end\n"
           "function snap_read() assert(p4.status.query(u0) == nil and p4.status.protected(u2)) end", 1);
    CHECK(p4_lua_invoke(a, snapshot_call, (void *)"snap_set", 0, P4_LUA_SNAPSHOT) != 0);
    CHECK(p4_lua_invoke(a, snapshot_call, (void *)"snap_read", 0, P4_LUA_SNAPSHOT) == 0);
    CHECK(!held[0]);

    /* Inside a battle the gate keeps its holders. */
    run(a, "local ok, why = p4.status.enable(false); assert(ok == true)", 1);
    run(b, "local ok, why = p4.status.enable(false); assert(ok == false and why:find('outside battle'))", 1);
    CHECK(gate);

    /* Both halves of the shared namespace work in both VMs: a wear-off modifier
     * reads p4.status.key, and the stage composes a's and b's modifiers. (Last,
     * because a non-identity result is semantic use.) */
    run(a, "fade = p4.status.wearoff.on({kind = 'add', fn = function(roll)\n"
           "    assert(roll.side == 'enemy' and roll.status == 'fear' and roll.stock == 25)\n"
           "    return if p4.status.key(24) == 'test.status:freeze' then 10 else nil end})", 1);
    run(b, "fade = p4.status.wearoff.on({kind = 'add', fn = function(roll) return 20 end})", 1);
    const P4GameWearoffRoll roll = {.side = P4_DAMAGE_SIDE_ENEMY, .id = 0x2b, .status = 0x04, .level = 22, .luck = 9,
        .turns = 2, .limit = 3, .stock = 25, .afflictions = 1};
    CHECK(p4_game_wearoff_count() == 2 && p4_game_wearoff_apply(&roll) == 55);
    run(a, "fade:remove()", 1);
    run(b, "fade:remove()", 1);
    CHECK(p4_game_wearoff_count() == 0 && p4_game_wearoff_apply(&roll) == 25);

    /* Teardown releases only the closing mod's holds. */
    p4_lua_destroy(a);
    CHECK(!guarded[2] && guarded[1] && gate);
    in_battle = 0;
    p4_lua_destroy(b);
    CHECK(!gate);
    p4_mod_events_clear();
    p4_package_registry_retire();
    puts("mod_luau_status: PASS (one p4.status with the wear-off stage in either order, per-mod gate and protection "
         "holders, refusals, misuse, snapshot, status event source)");
    return 0;
}
int main(int argc, char **argv)
{
    p4_os_init();
    p4_os_setenv("P4PC_VIRTUAL_CLOCK", "1", 1);
    p4_os_setenv("P4PC_TURBO", "0", 1);
    return p4t_on_guest_stack(witness, argc, argv);
}
