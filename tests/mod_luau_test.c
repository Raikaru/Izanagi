/* Real Luau VM/bindings + production cooperative kernel, synthetic data only. */
#include "lua_runtime.h"
#include "native_bindings.h"
#include "p4pc/mod_script.h"
#include "guest_test.h"
#include "p4pc/image.h"
#include "p4pc/mod_events.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "p4pc/tables.h"
#include "p4pc/ps2_libc.h"
#include "vfs.h"
#include "../src/platform/memcard/mod_save_internal.h"
#include "p4pc/mod_setting_store.h"
#include "p4pc/mod_lifecycle.h"
#include "p4pc/game_damage_stage.h"
#include "p4pc/mod_artifacts.h"
#include "p4pc/game_recovery_stage.h"
#include "p4pc/game_cost_stage.h"
#include "p4pc/game_chance_stage.h"
#include "p4pc/game_wearoff_stage.h"
#include "p4pc/game_buff_stage.h"
#include "p4pc/game_reward_stage.h"
#include "p4pc/game_unit_stage.h"
#include "p4pc/game_hit_stage.h"
#include "p4pc/game_passive_grant.h"
#ifdef P4_MOD_NATIVE_RECOVERY
#include "p4pc/crt.h"
#include "failure_native.h"
#include "../src/platform/config/p4cfg.h"
#include "p4pc/mods.h"
#include "p4pc/package_memory.h"
#include "p4pc/mod_bindings.h"
#include "p4pc/package_boot.h"
#endif
#include <errno.h>
#include <fenv.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif
#include "p4pc/game_elements_ai_native.h"
#include "p4pc/table_result.h"

uint32_t mod_test_unsigned(uint32_t value);
int16_t mod_test_narrow(int8_t value, uint16_t add);
float mod_test_mixed(int value, float real, unsigned char *byte, double wide);
uint32_t *mod_test_pointer(uint32_t *value);
int mod_test_recursive(int depth);
void mod_test_void(uint32_t *value);
void mod_test_set_action_code(unsigned char *action, unsigned char *slot, uint16_t code);
const p4pc_native_t p4pc_native_table[] = {
    {0x100100, (void *)mod_test_unsigned, "mod_test_unsigned", NULL},
    {0x100120, (void *)mod_test_narrow, "mod_test_narrow", NULL},
    {0x100140, (void *)mod_test_mixed, "mod_test_mixed", NULL},
    {0x100160, (void *)mod_test_pointer, "mod_test_pointer", NULL},
    {0x100180, (void *)mod_test_recursive, "mod_test_recursive", NULL},
    {0x1001a0, (void *)mod_test_void, "mod_test_void", NULL},
    {0x1de640, (void *)mod_test_set_action_code, "mod_test_set_action_code", NULL}
};
const size_t p4pc_native_count = sizeof p4pc_native_table / sizeof p4pc_native_table[0];
const p4pc_patch_t p4pc_patch_table[] = {{0, NULL, NULL}};
const size_t p4pc_patch_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {{NULL, NULL}};
const size_t p4pc_hostfn_count = 0;
/* This fixture never loads an ELF or initializes the EE heap. Their production
 * evidence getters have separate loader/real-allocator witnesses. The actual
 * kernel supplies stack exclusion, context keys and scheduling here. */
const p4pc_image_guest_range_t *p4pc_image_guest_ranges(size_t *count) { *count = 0; return NULL; }
int p4pc_heap_guest_span(uint32_t *begin, uint32_t *end) { (void)begin; (void)end; return -1; }
int p4pc_heap_guest_host_overlap(uint32_t address, size_t size) { (void)address; (void)size; return 0; }

int32_t func_004215e0(void *parameter);
int32_t func_00421600(int32_t id, uint32_t arg);
int32_t func_00421670(int32_t id, int32_t priority);
int32_t func_00421690(int32_t priority);
/* Stub enemy AI carrier for p4.battle.substitute: 227 is authorized, 128 has
 * another target scope, anything else is not an active skill. */
static void *substitute_action;
static uint32_t substitute_skill;
static unsigned substitute_calls;
int p4_game_elements_ai_native_substitute(void *source_action, uint32_t skill)
{
    ++substitute_calls;
    substitute_action = source_action;
    substitute_skill = skill;
    return skill == 227u ? P4_TABLE_OK : skill == 128u ? P4_TABLE_CONFLICT : P4_TABLE_NOT_FOUND;
}
/* The p4.passive game facts (tests/mod_passive_facts.c). */
extern int passive_facts_catalog_ready;
int32_t func_004216d0(void);
static int failures;
static unsigned canonical_copies, watched_invalidations;
static const void *watched_scratch;
static unsigned char watched_value;
static void observe_bytes(void *destination, const void *source, uint32_t count, int after)
{
    if (after) return;
    if (source) ++canonical_copies;
    if (!source && destination == watched_scratch && count == 128) {
        /* Reading here must happen before the real guest stack arena unmaps. */
        watched_value = ((const volatile unsigned char *)destination)[40];
        ++watched_invalidations;
    }
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_luau: FAIL line%d: %s\n", __LINE__, #x); ++failures; } } while (0)
static int event_teardown_mode;
static int event_teardown(const P4ModEvent *event, void *user)
{
    (void)event; (void)user;
    CHECK(!p4_mod_events_quiescent());
    if (event_teardown_mode == 1) p4_mod_events_clear();
    else p4_mod_events_remove_owner(P4_MOD_NATIVE_OWNER);
    return 0;
}
static void run(P4LuaMod *mod, const char *source, int success)
{
    int status = p4_lua_compile(mod, source, strlen(source), "@mod-luau-test");
    if (!status) status = p4_lua_execute(mod);
    if ((status == 0) != success) fprintf(stderr, "mod_luau unexpected status %d for: %s\n", status, source);
    CHECK((status == 0) == success);
}
static void saved_provenance(int expected)
{
    unsigned char image[4100]={0};
    image[57]=0x20;image[60]=1;memset(image+65,0xff,4);
    P4ModSaveImage raw={image,sizeof image,"native-vm-synthetic"};
    P4ModSaveCapture *capture=NULL;char error[1024]={0};
    int result=p4_mod_save_capture_image(&raw,&capture,error,sizeof error);
    if(result)fprintf(stderr,"synthetic VM provenance capture: %s\n",error);
    CHECK(!result&&((capture!=NULL)==expected));
    CHECK(!capture||!capture->witness);
    p4_mod_save_capture_free(capture);
}
static int nested_callback(lua_State *L)
{
    P4LuaMod *mod = p4_luau_user(L);
    lua_getglobal(L, "quota_inner");
    if (p4_lua_protected(mod, L, 0, 0, 0)) luaL_error(L, "nested callback budget failed");
    return 0;
}
static int synchronous_callback(lua_State *L)
{
    lua_getglobal(L, "action_body");
    lua_call(L, 0, 0);
    return 0;
}
static int empty_callback(lua_State *L)
{
    (void)L;
    return 0;
}
static int yield_body(P4ModCall *call, void *user)
{
    (void)call; (void)user;
    CHECK(func_00421690(60) == 0);
    return 0;
}
static uint32_t worker_results[2];
static uint64_t worker_keys[2];
static void worker(void *argument)
{
    unsigned which = (unsigned)(uintptr_t)argument;
    worker_keys[which] = p4pc_thread_context_key();
    worker_results[which] = mod_test_unsigned(which ? 20u : 10u);
}
static int32_t spawn(void)
{
    struct {
        int32_t status;
        uint32_t function, stack;
        int32_t stack_size;
        uint32_t gp;
        int32_t initial_priority, current_priority;
        uint32_t attributes, option;
    } parameters = {.function = P4_FN_H2G(worker), .initial_priority = 60, .current_priority = 60};
    return func_004215e0(&parameters);
}
static void coroutine_witness(P4LuaMod *mod)
{
    run(mod,
        "trace={}; replacement=p4.hook(0x100100,'II','replace',function(c)\n"
        " local value=c.args[1]; table.insert(trace,'enter'..value)\n"
        " if value==10 then saved_first=c else assert(not pcall(saved_first.next)) end\n"
        " local result=c.next(); assert(result==value+1)\n"
        " table.insert(trace,'exit'..value); c.result=result*2\n"
        "end)", 1);
    P4ModFunction *function = p4_mod_function_find(0x100100, "II");
    P4ModHook *observer = p4_mod_hook_add(P4_MOD_NATIVE_OWNER, function, P4_MOD_BEFORE, yield_body, NULL);
    CHECK(observer != NULL);
    int32_t main_id = func_004216d0();
    CHECK(func_00421670(main_id, 50) >= 0);
    int32_t first = spawn(), second = spawn();
    CHECK(first > 1 && second > first);
    CHECK(func_00421600(first, 0) == first);
    CHECK(func_00421600(second, 1) == second);
    CHECK(func_00421670(main_id, 70) == 50);
    CHECK(worker_results[0] == 22 && worker_results[1] == 42);
    run(mod, "assert(table.concat(trace,',')=='enter10,enter20,exit10,exit20'); assert(not pcall(saved_first.next))", 1);
    uint64_t old_key = worker_keys[0];
    CHECK(func_00421670(main_id, 50) == 70);
    CHECK(func_00421600(first, 0) == first);
    CHECK(func_00421670(main_id, 70) == 50);
    CHECK(worker_keys[0] != old_key && worker_results[0] == 22);
    CHECK(func_00421670(main_id, 50) == 70);
    CHECK(p4_mod_hook_remove(observer) == 0);
    run(mod, "replacement:remove(); replacement=nil; saved_first=nil", 1);
}

static unsigned native_initializations, native_destructions;
static void native_initialize(lua_State *L)
{
    ++native_initializations;
    lua_pushboolean(L, 1); lua_setfield(L, -2, "fixture_native");
    p4_lua_stages_initialize(L); /* the real binding over the real amount stages */
    p4_lua_passive_initialize(L); /* the real binding over the real grant table */
    /* The real p4.battle.substitute binding over the stub carrier below. */
    lua_newtable(L);
    lua_pushcfunction(L, p4_lua_battle_substitute, "substitute");
    lua_setfield(L, -2, "substitute");
    lua_setfield(L, -2, "battle");
}
static void native_before_destroy(P4LuaMod *mod)
{
    ++native_destructions;
    p4_lua_stages_close(mod);
    p4_lua_passive_close(mod);
    /* Real Lua state remains alive; no fake settings backend is installed. */
    lua_getglobal(mod->state, "p4");
    lua_rawgetfield(mod->state, -1, "fixture_native");
    CHECK(lua_toboolean(mod->state, -1));
    lua_pop(mod->state, 2);
}
static const P4LuaNativeBindings fixture_bindings = {native_initialize, native_before_destroy};
/* p4.damage: order, frozen snapshot hits, disable/remove, identity values and
 * teardown, through p4_game_damage_apply exactly as datCalcSkillAmount calls it. */
static void damage_witness(P4LuaMod *mod, const P4PackageProvider *other_provider)
{
    P4GameDamageHit hit = {.skill_id = 16, .source_side = P4_DAMAGE_SIDE_PARTY, .source_id = 1,
        .target_side = P4_DAMAGE_SIDE_ENEMY, .target_id = 258, .origin_side = P4_DAMAGE_SIDE_ENEMY,
        .origin_id = 258, .hit_kind = P4_DAMAGE_HIT_WEAK, .element_valid = 1, .element_id = 1,
        .resolved = 1, .stock = 100, .source_status = 0x2, .target_status = 0x10,
        .origin = P4_DAMAGE_ORIGIN_SKILL, .source_persona = 0x2a, .source_persona_level = 31};
    CHECK(p4_game_damage_count() == 0 && p4_game_damage_apply(&hit) == 100);
    run(mod, "p4.damage.on{kind='divide', fn=function() end}", 0);
    run(mod, "p4.damage.on{kind='add'}", 0);
    run(mod, "p4.damage.on{kind='add', priority=0.5, fn=function() end}", 0);
    /* A start-up declaration: refused inside any callback, not only snapshots. */
    run(mod, "hooked=p4.hook(0x100100,'II','before',function(c)\n"
             " refused=not pcall(p4.damage.on, {kind='add', fn=function() end}) end)", 1);
    (void)mod_test_unsigned(10);
    run(mod, "assert(refused); hooked:remove(); hooked=nil; refused=nil", 1);
    CHECK(p4_game_damage_count() == 0);
    run(mod,
        "seen={}\n"
        "late=p4.damage.on{kind='add', priority=5, fn=function(h) table.insert(seen,'add'); return 7 end}\n"
        "double=p4.damage.on{kind='multiply', fn=function(h)\n"
        " table.insert(seen,'mul')\n"
        " assert(h.source_side=='party' and h.source_id==1 and h.target_side=='enemy' and h.target_id==258)\n"
        " assert(h.origin_side=='enemy' and h.origin_id==258 and h.hit_kind=='weak' and h.element==1)\n"
        " assert(h.resolved and not h.reflected and h.stock==100 and h.source_status==2 and h.target_status==0x10)\n"
        " assert(h.origin=='skill' and h.source_persona==0x2a and h.source_persona_level==31)\n"
        " assert(not pcall(function() h.stock=1 end))\n"
        " assert(not pcall(p4.damage.on, {kind='add', fn=function() end}))\n"
        " assert(not pcall(p4.call, 0x100100, 'II', 1))\n"
        " if h.skill==17 then return nil elseif h.skill==18 then return 1 end\n"
        " return 2 end}\n", 1);
    CHECK(p4_game_damage_count() == 2);
    CHECK(p4_game_damage_apply(&hit) == 207); /* priority 0 multiply, then priority 5 add */
    run(mod, "assert(table.concat(seen,',')=='mul,add'); seen={}", 1);
    hit.skill_id = 17;
    CHECK(p4_game_damage_apply(&hit) == 107);
    hit.skill_id = 18;
    CHECK(p4_game_damage_apply(&hit) == 107);
    run(mod, "double:disable(); seen={}", 1);
    hit.skill_id = 16;
    CHECK(p4_game_damage_apply(&hit) == 107);
    run(mod, "assert(table.concat(seen,',')=='add'); double:enable(); late:remove(); late:remove()\n"
             "assert(not pcall(late.enable, late))", 1);
    CHECK(p4_game_damage_count() == 1 && p4_game_damage_apply(&hit) == 200);
    P4LuaMod *other = p4_lua_create(other_provider);
    CHECK(other != NULL);
    if (other) {
        run(other, "half=p4.damage.on{kind='multiply', priority=-1, fn=function(h) if h.skill==18 then return 1 end return 0.5 end}", 1);
        CHECK(!other->semantic_used && p4_game_damage_count() == 2);
        hit.skill_id = 18;
        CHECK(p4_game_damage_apply(&hit) == 100 && !other->semantic_used); /* identity is not semantic use */
        hit.skill_id = 16;
        CHECK(p4_game_damage_apply(&hit) == 100 && other->semantic_used); /* 100 x 0.5 x 2 */
        p4_lua_destroy(other); /* VM teardown unregisters its modifiers */
        CHECK(p4_game_damage_count() == 1);
    }
    run(mod, "double:remove(); double=nil; late=nil; seen=nil", 1);
    CHECK(p4_game_damage_count() == 0);
    /* A hit without a persona source reports nil persona fields and its command. */
    run(mod, "probe=p4.damage.on{kind='multiply', fn=function(h)\n"
             " assert(h.origin=='attack' and h.source_persona==nil and h.source_persona_level==nil) end}", 1);
    hit.origin = P4_DAMAGE_ORIGIN_ATTACK; hit.source_persona = hit.source_persona_level = 0;
    CHECK(p4_game_damage_apply(&hit) == 100);
    run(mod, "probe:remove(); probe=nil", 1);
    CHECK(p4_game_damage_count() == 0);
}
/* --stage-failure recovery|chance|cost: a modifier that raises must reach the
 * native failure owner (p4_mod_failure), as a damage modifier's error does;
 * each stage pushes its own hit table. Exits 42 from the owner, 3 otherwise. */
static const char *stage_failure_mode;
static void expected_stage_failure(size_t owner, const char *reason, void *user)
{
    (void)owner;
    (void)user;
    _Exit(reason && strstr(reason, "intentional stage failure") ? 42 : 43);
}
static void stage_failure(P4LuaMod *mod)
{
    static const char fn[] = "{kind='multiply', fn=function(h) error('intentional stage failure') end}";
    char source[160];
    snprintf(source, sizeof source, "boom=p4.%s.on%s", stage_failure_mode, fn);
    run(mod, source, 1);
    if (!strcmp(stage_failure_mode, "recovery")) {
        const P4GameRecoveryHit heal = {.skill_id = 0xc0, .source_side = P4_DAMAGE_SIDE_PARTY, .source_id = 1,
            .target_side = P4_DAMAGE_SIDE_PARTY, .target_id = 2, .resolved = 1, .origin = P4_DAMAGE_ORIGIN_SKILL,
            .stock = 100};
        (void)p4_game_recovery_apply(&heal);
    } else if (!strcmp(stage_failure_mode, "chance")) {
        const P4GameChanceHit chance = {.skill_id = 0x53, .source_side = P4_DAMAGE_SIDE_PARTY, .source_id = 1,
            .target_side = P4_DAMAGE_SIDE_ENEMY, .target_id = 40, .component = P4_CHANCE_COMPONENT_AILMENT,
            .statuses = 0x10, .stock = 45};
        (void)p4_game_chance_apply(&chance);
    } else if (!strcmp(stage_failure_mode, "cost")) {
        const P4GameCostHit cost = {.skill_id = 2, .side = P4_DAMAGE_SIDE_PARTY, .unit_id = 1,
            .resource = P4_COST_RESOURCE_SP, .stock = 12};
        (void)p4_game_cost_apply(&cost);
    }
    fprintf(stderr, "mod_luau: the %s modifier's error did not reach the failure owner\n", stage_failure_mode);
    _Exit(3);
}
/* p4.recovery, p4.cost and p4.chance: the same binding over their own stages
 * and hit tables, the same refusals, order, identity rule and teardown. */
static void stages_witness(P4LuaMod *mod, const P4PackageProvider *other_provider)
{
    if (stage_failure_mode) stage_failure(mod);
    run(mod, "p4.cost.on{kind='divide', fn=function() end}", 0);
    run(mod, "p4.recovery.on{kind='add'}", 0);
    run(mod, "p4.chance.on{kind='add', priority=0.5, fn=function() end}", 0);
    run(mod,
        "heal=p4.recovery.on{kind='multiply', priority=20, fn=function(h)\n"
        " assert(h.skill==0xc0 and h.source_side=='party' and h.source_id==1 and h.target_side=='party' and h.target_id==2)\n"
        " assert(h.resolved and h.origin=='skill' and h.stock==100 and h.source_persona==0x2a and h.source_persona_level==31)\n"
        " assert(not pcall(function() h.stock=1 end))\n"
        " return 1.15 end}\n"
        "cost=p4.cost.on{kind='multiply', priority=20, fn=function(h)\n"
        " assert(h.skill==2 and h.side=='party' and h.unit==1 and h.resource=='sp' and h.stock==12)\n"
        " if h.persona==nil then assert(h.persona_level==nil) return nil end\n"
        " assert(h.persona==0x55 and h.persona_level==60)\n"
        " return 0.81 end}\n"
        "chance=p4.chance.on{kind='multiply', priority=20, fn=function(h)\n"
        " assert(h.skill==0x53 and h.component=='ailment' and h.statuses==0x10 and h.item==false and h.stock==45)\n"
        " assert(h.source_side=='party' and h.target_side=='enemy' and h.target_id==40 and h.source_persona==0x2a)\n"
        " assert(h.afflictions==2)\n"
        " return 1.1 end}\n", 1);
    CHECK(p4_game_recovery_count() == 1 && p4_game_cost_count() == 1 && p4_game_chance_count() == 1);
    const P4GameRecoveryHit heal = {.skill_id = 0xc0, .source_side = P4_DAMAGE_SIDE_PARTY, .source_id = 1,
        .target_side = P4_DAMAGE_SIDE_PARTY, .target_id = 2, .resolved = 1, .origin = P4_DAMAGE_ORIGIN_SKILL,
        .stock = 100, .source_persona = 0x2a, .source_persona_level = 31};
    CHECK(p4_game_recovery_apply(&heal) == 114); /* 100 x 1.15 truncated once */
    P4GameCostHit cost = {.skill_id = 2, .side = P4_DAMAGE_SIDE_PARTY, .unit_id = 1,
        .resource = P4_COST_RESOURCE_SP, .stock = 12, .persona = 0x55, .persona_level = 60};
    CHECK(p4_game_cost_apply(&cost) == 10); /* 12 x 0.81 = 9.72, rounded half up */
    cost.persona = cost.persona_level = 0;
    CHECK(p4_game_cost_apply(&cost) == 12);
    const P4GameChanceHit chance = {.skill_id = 0x53, .source_side = P4_DAMAGE_SIDE_PARTY, .source_id = 1,
        .target_side = P4_DAMAGE_SIDE_ENEMY, .target_id = 40, .component = P4_CHANCE_COMPONENT_AILMENT,
        .statuses = 0x10, .stock = 45, .source_persona = 0x2a, .source_persona_level = 31, .afflictions = 2};
    CHECK(p4_game_chance_apply(&chance) == 49); /* 45 x 1.1 = 49.5, truncated */
    P4LuaMod *other = p4_lua_create(other_provider);
    CHECK(other != NULL);
    if (other) {
        run(other, "more=p4.cost.on{kind='add', priority=30, fn=function(h) return 2 end}", 1);
        cost.persona = 0x55; cost.persona_level = 60;
        CHECK(p4_game_cost_count() == 2 && p4_game_cost_apply(&cost) == 12); /* 12 x 0.81 + 2 = 11.72 */
        p4_lua_destroy(other);
        CHECK(p4_game_cost_count() == 1);
    }
    run(mod, "heal:remove(); cost:remove(); chance:disable()", 1);
    CHECK(p4_game_chance_apply(&chance) == 45 && p4_game_chance_count() == 1);
    run(mod, "chance:remove(); heal=nil; cost=nil; chance=nil", 1);
    CHECK(!p4_game_recovery_count() && !p4_game_cost_count() && !p4_game_chance_count());
}
/* p4.status.wearoff, p4.buff, p4.reward, p4.unit and p4.hit: what a mod's
 * callback sees for each stage's frozen query, each stage's clamp, the
 * readonly p4.status namespace and teardown. */
static void formula_stages_witness(P4LuaMod *mod)
{
    run(mod, "p4.status.wearoff.on{kind='divide', fn=function() end}", 0);
    run(mod, "p4.buff.on{kind='add'}", 0);
    run(mod, "p4.status.extra=1", 0);
    run(mod, "p4.status.wearoff=nil", 0);
    run(mod,
        "assert(p4.has('status.wearoff@1') and p4.has('status.afflictions@1') and p4.has('buff@1'))\n"
        "assert(p4.has('reward@1') and p4.has('unit@1') and p4.has('hit@1'))\n"
        "wear=p4.status.wearoff.on{kind='add', fn=function(r)\n"
        " assert(r.side=='party' and r.unit==1 and r.status=='confusion' and r.level==30 and r.luck==20)\n"
        " assert(r.turns==2 and r.limit==3 and r.stock==40 and r.afflictions==1)\n"
        " assert(not pcall(function() r.stock=1 end))\n"
        " return 15 * (r.turns - 1) end}\n"
        "buff=p4.buff.on{kind='multiply', fn=function(q)\n"
        " assert(q.side=='enemy' and q.unit==40 and q.stat=='defense' and q.rank==-1 and q.charged==false and q.stock==1.5)\n"
        " return 2 end}\n"
        "reward=p4.reward.on{kind='multiply', fn=function(q)\n"
        " if q.reward=='money' then assert(q.recipient==nil and q.id==nil and q.stock==190) return 2 end\n"
        " assert(q.recipient=='persona' and q.id==0x2a and q.level==31 and q.equipped==true and q.stock==100)\n"
        " return 0 end}\n"
        "unit=p4.unit.on{kind='multiply', fn=function(q)\n"
        " if q.side=='enemy' then assert(q.unit==40 and q.stat=='max_hp' and q.level==10 and q.stock==40000) end\n"
        " if q.side=='party' then assert(q.unit==2 and q.stat=='strength' and q.level==30 and q.stock==60) end\n"
        " return 2 end}\n"
        "hit=p4.hit.on{kind='add', fn=function(q)\n"
        " assert(q.skill==1 and q.source_side=='party' and q.source_id==1 and q.target_side=='enemy' and q.target_id==40)\n"
        " assert((q.roll=='critical' and q.stock==35) or (q.roll=='accuracy' and q.stock==95))\n"
        " return 10 end}\n", 1);
    CHECK(p4_game_wearoff_count() == 1 && p4_game_buff_count() == 1 && p4_game_reward_count() == 1 &&
        p4_game_unit_count() == 1 && p4_game_hit_count() == 1);
    const P4GameWearoffRoll roll = {.side = P4_DAMAGE_SIDE_PARTY, .id = 1, .status = 0x10, .level = 30, .luck = 20,
        .turns = 2, .limit = 3, .stock = 40, .afflictions = 1};
    CHECK(p4_game_wearoff_apply(&roll) == 55);
    const P4GameBuffQuery buff = {.side = P4_DAMAGE_SIDE_ENEMY, .id = 40, .stat = P4_BUFF_DEFENSE, .rank = -1,
        .stock = 1.5};
    CHECK(p4_game_buff_apply(&buff) == 3.0f);
    const P4GameRewardQuery award = {.reward = P4_REWARD_EXP, .recipient = P4_REWARD_PERSONA, .id = 0x2a, .level = 31,
        .equipped = 1, .stock = 100};
    const P4GameRewardQuery money = {.reward = P4_REWARD_MONEY, .stock = 190};
    CHECK(p4_game_reward_apply(&award) == 0 && p4_game_reward_apply(&money) == 380);
    CHECK(p4_game_reward_apply(&money) == 380); /* the results screen asks again: the same answer */
    const P4GameUnitQuery hp = {.side = P4_DAMAGE_SIDE_ENEMY, .id = 40, .stat = P4_UNIT_MAX_HP, .level = 10,
        .stock = 40000};
    const P4GameUnitQuery strength = {.side = P4_DAMAGE_SIDE_PARTY, .id = 2, .stat = P4_UNIT_STRENGTH, .level = 30,
        .stock = 60};
    CHECK(p4_game_unit_apply(&hp) == P4_UNIT_ENEMY_POOL_MAX && p4_game_unit_apply(&strength) == P4_UNIT_STAT_MAX);
    const P4GameHitQuery critical = {.roll = P4_HIT_CRITICAL, .skill_id = 1, .source_side = P4_DAMAGE_SIDE_PARTY,
        .source_id = 1, .target_side = P4_DAMAGE_SIDE_ENEMY, .target_id = 40, .stock = 35};
    P4GameHitQuery accuracy = critical;
    accuracy.roll = P4_HIT_ACCURACY;
    accuracy.stock = 95;
    CHECK(p4_game_hit_apply(&critical) == 45 && p4_game_hit_apply(&accuracy) == P4_HIT_MAX);
    run(mod, "wear:disable()", 1);
    CHECK(p4_game_wearoff_apply(&roll) == 40 && p4_game_wearoff_count() == 1);
    run(mod, "wear:remove(); buff:remove(); reward:remove(); unit:remove(); hit:remove()\n"
             "wear=nil; buff=nil; reward=nil; unit=nil; hit=nil", 1);
    CHECK(!p4_game_wearoff_count() && !p4_game_buff_count() && !p4_game_reward_count() && !p4_game_unit_count() &&
        !p4_game_hit_count());
}
/* p4.passive: a grant reaches its target's native passive checks and nothing
 * else; owners hold grants independently and order never matters; queries
 * are pure; snapshot callbacks cannot grant; a mod's grants end with it. */
static void passive_witness(P4LuaMod *mod, const P4PackageProvider *other_provider)
{
    const uint16_t member[8] = {0, 2}, other_member[8] = {0, 3}, enemy[8] = {4, 40};
    CHECK(!p4_game_passive_live && !P4_PASSIVE_UNIT_GRANTED(member, 0x1F6) && !P4_PASSIVE_LEADER_GRANTED(0x205));
    run(mod, "assert(p4.has('passive.grant@1'))", 1);
    run(mod, "p4.passive.grant({party = 'yosuke'}, 1)", 0); /* an active skill */
    run(mod, "p4.passive.grant({party = 'yosuke'}, 600)", 0); /* not a registered skill */
    run(mod, "p4.passive.grant({party = 'marie'}, 0x1F6)", 0);
    run(mod, "p4.passive.grant({party = 9}, 0x1F6)", 0);
    run(mod, "p4.passive.grant({party = 2, enemy = 40}, 0x1F6)", 0);
    run(mod, "p4.passive.grant({}, 0x1F6)", 0);
    run(mod, "p4.passive.grant({enemy = 0x150}, 0x1F6)", 0);
    run(mod, "p4.passive.grant({item = 0x8000}, 0x1F6)", 0);
    run(mod, "p4.passive.extra = 1", 0);
    CHECK(!p4_game_passive_count());
    /* With the catalog loaded a new grant's Persona, item or enemy must be a
     * row of it; before that only the ID ranges are known. Revocations and
     * queries take any in-range target, so such a grant can still be taken back. */
    run(mod, "local ok, why = pcall(p4.passive.grant, {persona = 0x2b}, 0x1F6)\n"
             "assert(not ok and string.find(why, 'no persona of the loaded tables', 1, true))\n"
             "assert(not pcall(p4.passive.grant, {item = 7}, 0x1F6) and not pcall(p4.passive.grant, {enemy = 42}, 0x1F6))", 1);
    CHECK(!p4_game_passive_count());
    passive_facts_catalog_ready = 0;
    run(mod, "assert(p4.passive.grant({persona = 0x2b}, 0x1F6))", 1);
    passive_facts_catalog_ready = 1;
    run(mod, "assert(p4.passive.granted({persona = 0x2b}, 0x1F6) and p4.passive.revoke({persona = 0x2b}, 0x1F6))", 1);
    CHECK(!p4_game_passive_count());
    run(mod,
        "assert(p4.passive.grant({party = 'yosuke'}, 0x1F6) == true)\n"
        "assert(p4.passive.grant({party = 2}, {id = 0x1F6}) == false)\n"
        "assert(p4.passive.grant({enemy = 40}, 0x1FB))\n"
        "assert(p4.passive.grant({item = {id = 5}}, 0x1F8))\n"
        "assert(p4.passive.grant({persona = 0x2a}, 0x1F7))\n"
        "assert(p4.passive.granted({party = 2}, 0x1F6) and not p4.passive.granted({party = 3}, 0x1F6))\n"
        "local list = p4.passive.list()\n"
        "assert(#list == 4 and list[1].target.party == 'yosuke' and list[1].skill == 0x1F6 and list[1].owner == p4.id)\n"
        "assert(list[2].target.persona == 0x2a and list[3].target.item == 5 and list[4].target.enemy == 40)\n"
        "assert(list[1].kind == 'party' and list[1].id == 2 and list[2].kind == 'persona' and list[2].id == 0x2a)\n"
        "assert(list[3].kind == 'item' and list[3].id == 5 and list[4].kind == 'enemy' and list[4].id == 40)\n", 1);
    CHECK(p4_game_passive_live == 4);
    CHECK(P4_PASSIVE_UNIT_GRANTED(member, 0x1F6) && P4_PASSIVE_UNIT_GRANTED(member, 0x1F7) &&
        P4_PASSIVE_UNIT_GRANTED(member, 0x1F8) && !P4_PASSIVE_UNIT_GRANTED(member, 0x1FB));
    CHECK(!P4_PASSIVE_UNIT_GRANTED(other_member, 0x1F6) && !P4_PASSIVE_UNIT_GRANTED(other_member, 0x1F7) &&
        !P4_PASSIVE_UNIT_GRANTED(other_member, 0x1F8));
    CHECK(P4_PASSIVE_UNIT_GRANTED(enemy, 0x1FB) && !P4_PASSIVE_COUNTER_GRANTED(enemy) && P4_PASSIVE_COUNTER_GRANTED(member));
    /* Two mods: overlapping tuples are held separately; neither can revoke or
     * clear the other's; tearing one down leaves the other's grants. */
    P4LuaMod *other = p4_lua_create(other_provider);
    CHECK(other != NULL);
    if (other) {
        run(other, "assert(p4.passive.grant({party = 2}, 0x1F6) and p4.passive.grant({persona = 0x2a}, 0x1F7))\n"
                   "assert(p4.passive.revoke({enemy = 40}, 0x1FB) == false)\n"
                   "local mine = 0\n"
                   "for _, g in p4.passive.list() do if g.owner == p4.id then mine += 1 end end\n"
                   "assert(mine == 2 and #p4.passive.list() == 6)", 1);
        run(mod, "local owners = {}\n"
                 "for _, g in p4.passive.list() do\n"
                 " if g.kind == 'party' and g.id == 2 and g.skill == 0x1F6 then table.insert(owners, g.owner) end\n"
                 "end\n"
                 "assert(#owners == 2 and owners[1] ~= owners[2])\n"
                 "assert(p4.passive.revoke({party = 'yosuke'}, 0x1F6) == true)\n"
                 "assert(p4.passive.revoke({party = 2}, 0x1F6) == false)\n"
                 "assert(p4.passive.granted({party = 2}, 0x1F6))", 1);
        CHECK(P4_PASSIVE_UNIT_GRANTED(member, 0x1F6)); /* the other mod's grant remains */
        run(other, "assert(p4.passive.clear() == 2)", 1);
        CHECK(!P4_PASSIVE_UNIT_GRANTED(member, 0x1F6) && P4_PASSIVE_UNIT_GRANTED(member, 0x1F7) &&
            p4_game_passive_count() == 3); /* the first mod's Persona grant stays */
        run(other, "assert(p4.passive.grant({party = 2}, 0x1F6) and p4.passive.grant({enemy = 40}, 0x1FB))", 1);
        CHECK(p4_game_passive_count() == 5);
        p4_lua_destroy(other);
        CHECK(!P4_PASSIVE_UNIT_GRANTED(member, 0x1F6) && P4_PASSIVE_UNIT_GRANTED(enemy, 0x1FB) &&
            p4_game_passive_count() == 3);
    }
    /* A full table refuses only a new grant, with a readable error a mod can
     * catch; a grant already held still answers false; room comes back. */
    for (uint32_t i = 0; p4_game_passive_count() < P4_PASSIVE_GRANT_MAX; ++i)
        CHECK(p4_game_passive_grant("test.filler", 0xFFFFu, P4_PASSIVE_PERSONA, 0x1000u + i, 0x1F6) == 1);
    run(mod, "local ok, why = pcall(p4.passive.grant, {party = 1}, 0x1F6)\n"
             "assert(not ok and string.find(why, 'grant table is full', 1, true))\n"
             "assert(p4.passive.grant({enemy = 40}, 0x1FB) == false)", 1);
    CHECK(p4_game_passive_revoke_owner(0xFFFFu) == P4_PASSIVE_GRANT_MAX - 3 && p4_game_passive_count() == 3);
    run(mod, "refused = nil\n"
             "probe = p4.unit.on{kind='add', fn=function(q) refused = not pcall(p4.passive.grant, {party = 1}, 0x1F6); return nil end}", 1);
    const P4GameUnitQuery hp = {.side = P4_DAMAGE_SIDE_PARTY, .id = 1, .stat = P4_UNIT_MAX_HP, .level = 10, .stock = 100};
    CHECK(p4_game_unit_apply(&hp) == 100);
    run(mod, "assert(refused == true and not p4.passive.granted({party = 1}, 0x1F6)); probe:remove(); probe = nil", 1);
    run(mod, "assert(p4.passive.clear() == 3 and #p4.passive.list() == 0)", 1);
    CHECK(!p4_game_passive_live && !P4_PASSIVE_UNIT_GRANTED(member, 0x1F7));
    run(mod, "assert(p4.passive.grant({party = 'protagonist'}, 0x205))", 1);
    CHECK(P4_PASSIVE_LEADER_GRANTED(0x205) && p4_game_passive_count() == 1); /* revoked at this mod's teardown */
}
static void adopt_native_context(void)
{
    #ifdef P4_GUEST64
    for (size_t i = 0; i < p4pc_native_count; ++i)
        p4_fn_register(p4pc_native_table[i].addr, p4pc_native_table[i].fn);
    #endif
    /* Adopt the real boot context before any VM/native call can change its key. */
    CHECK(func_004216d0() == 1);
}
static int benchmark_requested;
static int benchmark_noop(P4ModCall *call,void *user)
{
    (void)call;(void)user;return 0;
}
static uint64_t benchmark_cpu(void)
{
    uint64_t value=0;
    CHECK(!p4_os_thread_cpu_ns(&value));
    return value;
}
static uint32_t benchmark_calls(unsigned count)
{
    uint32_t sum=0;
    for(unsigned i=0;i<count;++i) {
        /* Advance the real production frame counter, not a test budget bypass. */
        p4pc_vsync_wait();
        sum+=mod_test_unsigned(10);
    }
    CHECK(sum==11u*count);
    return sum;
}
static int hook_cost(P4LuaMod *mod,uint64_t setup_start)
{
    enum { BATCHES=10,CALLS=20000,WARM=1000 };
    const uint64_t budget_ns=10000;
    uint64_t quantum=0,baseline=0;
    CHECK(!p4_os_thread_cpu_resolution_ns(&quantum)&&quantum);
    printf("hook_cost synthetic=true reference_eligible=false pointer_bits=%u setup_cpu_ns=%llu counter_quantum_ns=%llu\n",
        (unsigned)(8*sizeof(void *)),(unsigned long long)(benchmark_cpu()-setup_start),
        (unsigned long long)quantum);
    static const char *const names[]={"unhooked","native","luau","luau-before-after"};
    for(unsigned mode=0;mode<4;++mode) {
        uint64_t configure_start=benchmark_cpu();
        P4ModHook *native=NULL;
        if(mode==1) {
            native=p4_mod_hook_add(P4_MOD_NATIVE_OWNER,p4_mod_function_find(0x100100,"II"),
                P4_MOD_BEFORE,benchmark_noop,NULL);
            CHECK(native!=NULL);
        } else if(mode>=2) {
            run(mod,"bench_before=p4.hook(0x100100,'II','before',function(c) end)",1);
            if(mode==3)run(mod,"bench_after=p4.hook(0x100100,'II','after',function(c) end)",1);
        }
        if(failures)break;
        uint64_t configure_ns=benchmark_cpu()-configure_start,warm_start=benchmark_cpu();
        uint32_t checksum=benchmark_calls(WARM);
        uint64_t warm_ns=benchmark_cpu()-warm_start,samples[BATCHES],total=0;
        for(unsigned batch=0;batch<BATCHES;++batch) {
            uint64_t start=benchmark_cpu();
            checksum+=benchmark_calls(CALLS);
            uint64_t end=benchmark_cpu();
            CHECK(end>=start);
            samples[batch]=end-start;total+=samples[batch];
        }
        printf("hook_cost case=%s calls=%u batches=%u calls_per_batch=%u warm_calls=%u configure_cpu_ns=%llu warm_cpu_ns=%llu checksum=%u samples_cpu_ns=",
            names[mode],BATCHES*CALLS,BATCHES,CALLS,WARM,
            (unsigned long long)configure_ns,(unsigned long long)warm_ns,checksum);
        for(unsigned i=0;i<BATCHES;++i)printf("%s%llu",i?",":"",(unsigned long long)samples[i]);
        for(unsigned i=1;i<BATCHES;++i) {
            uint64_t value=samples[i];unsigned j=i;
            while(j&&samples[j-1]>value){samples[j]=samples[j-1];--j;}
            samples[j]=value;
        }
        if(!mode)baseline=total;
        uint64_t added=total>baseline?total-baseline:0;
        unsigned callbacks=mode==3?2u:1u;
        /* Each independent interval can be off by one counter quantum. */
        uint64_t uncertainty=2u*BATCHES*quantum;
        printf(" total_cpu_ns=%llu median_ns_per_call=%llu p95_ns_per_call=%llu added_ns_per_callback=%llu uncertainty_ns_per_callback=%llu budget_ns_per_callback=%llu\n",
            (unsigned long long)total,(unsigned long long)((samples[(BATCHES-1)/2]+samples[BATCHES/2])/2/CALLS),
            (unsigned long long)(samples[BATCHES-1]/CALLS),
            (unsigned long long)(added/(BATCHES*CALLS*callbacks)),
            (unsigned long long)(uncertainty/(BATCHES*CALLS*callbacks)),
            (unsigned long long)budget_ns);
        if(mode>=2)CHECK(added<=budget_ns*BATCHES*CALLS*callbacks+uncertainty);
        if(native)CHECK(!p4_mod_hook_remove(native));
        if(mode>=2)run(mod,"bench_before:remove(); bench_before=nil",1);
        if(mode==3)run(mod,"bench_after:remove(); bench_after=nil",1);
    }
    return failures?1:0;
}
static int frame_cpu_witness(int argc,char **argv)
{
    (void)argc;(void)argv;
    CHECK(func_004216d0()==1);
    volatile uint32_t work=1;
    for(unsigned frame=0;frame<2101;++frame) {
        for(unsigned i=0;i<4096;++i)work=work*1664525u+1013904223u;
        p4pc_vsync_wait();
    }
    CHECK(!p4_packages_count());
    printf("frame_cpu_fixture synthetic=true reference_eligible=false checksum=%u result=%s\n",
        (unsigned)work,failures?"FAIL":"PASS");
    return failures?1:0;
}

/* [setting] declarations: bounded parse and refusals, without a package. */
static void setting_declarations_witness(void)
{
    P4PackageField good[] = {
        {"setting", "flag", "kind", "toggle", 9}, {"setting", "flag", "label", "FLAG", 10},
        {"setting", "style", "choices", "A | B|C", 4}, {"setting", "style", "default", "B", 5},
        {"setting", "style", "group", "LOOK", 6}, {"setting", "style", "kind", "choice", 3},
        {"setting", "style", "label", "STYLE", 7}};
    P4ModSettingDecl *decls = NULL;
    size_t count = 0;
    char reason[256];
    CHECK(!p4_mod_setting_declarations(good, 7, &decls, &count, reason, sizeof reason) && count == 2);
    if (count == 2) {
        /* Manifest line order, not key order: style starts on line 3. */
        CHECK(!strcmp(decls[0].key, "style") && decls[0].choice_count == 3 && decls[0].default_choice == 1 &&
              !strcmp(decls[0].choices[0], "A") && !strcmp(decls[0].choices[2], "C") && !strcmp(decls[0].group, "LOOK"));
        CHECK(!strcmp(decls[1].key, "flag") && decls[1].kind == P4_MOD_SETTING_TOGGLE &&
              decls[1].default_choice == 0 && !decls[1].restart);
        CHECK(p4_mod_setting_choice(&decls[0], "C") == 2 && p4_mod_setting_choice(&decls[0], "D") == 1);
    }
    free(decls);
    static const P4PackageField bad[][2] = {
        {{"setting", "x", "kind", "toggle", 1}, {"setting", "x", "unknown", "1", 2}},
        {{"setting", "x", "kind", "slider", 1}, {"setting", "x", "label", "X", 2}},
        {{"setting", "x", "kind", "toggle", 1}, {"setting", "x", "default", "maybe", 2}},
        {{"setting", "Bad", "kind", "toggle", 1}, {"setting", "Bad", "label", "X", 2}},
        {{"setting", "x", "kind", "choice", 1}, {"setting", "x", "label", "X", 2}},
        {{"setting", "x", "kind", "toggle", 1}, {"setting", "x", "apply", "later", 2}},
        {{"setting", "x", "kind", "toggle", 1}, {"setting", "x", "help", "no label", 2}}};
    for (size_t i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        decls = NULL;
        CHECK(p4_mod_setting_declarations(bad[i], 2, &decls, &count, reason, sizeof reason) == -1 && !decls);
    }
    const P4PackageField repeated[] = {{"setting", "x", "kind", "choice", 1}, {"setting", "x", "label", "X", 2},
                                       {"setting", "x", "choices", "A|A", 3}};
    CHECK(p4_mod_setting_declarations(repeated, 3, &decls, &count, reason, sizeof reason) == -1);
    CHECK(p4_mod_setting_key_valid("run.count-2") && !p4_mod_setting_key_valid(".x") &&
          !p4_mod_setting_key_valid("a b") && !p4_mod_setting_value_valid(" x") && p4_mod_setting_value_valid("x y"));
}

static int witness(int argc, char **argv)
{
    if (argc != 2) return 2;
    /* The failure owner must be installed while no hook or event exists. */
    if (stage_failure_mode && p4_mod_failure_handler(expected_stage_failure, NULL)) return 2;
    uint64_t setup_start=benchmark_requested?benchmark_cpu():0;
    CHECK(p4_lua_bind_native(&fixture_bindings) == 0);
    adopt_native_context();
    char path[1200];
    p4_os_mkdir(argv[1]);
    int length = snprintf(path, sizeof path, "%s/DATA", argv[1]);
    if (length < 0 || (size_t)length >= sizeof path) return 2;
    p4_os_mkdir(path);
    length = snprintf(path, sizeof path, "%s/DATA/TEST.TXT", argv[1]);
    if (length < 0 || (size_t)length >= sizeof path) return 2;
    FILE *fixture = fopen(path, "wb");
    if (!fixture) return 2;
    int wrote = fputs("mod-resource", fixture);
    int closed = fclose(fixture);
    if (wrote < 0 || closed) return 2;
    char mod_root[1200];
    length = snprintf(mod_root, sizeof mod_root, "%s/mods", argv[1]);
    if (length < 0 || (size_t)length >= sizeof mod_root) return 2;
    p4_os_mkdir(mod_root);
    static const char *const ids[] = {"test.luau", "test.isolation"};
    /* test.luau declares four settings; boot=on is stored before admission.
     * test.isolation requires test.luau 1.0 with lock on, which locks that row. */
    static const char settings[] =
        "[setting flag]\nkind=toggle\nlabel=FLAG\n"
        "[setting style]\nkind=choice\nchoices=A|B|C\nlabel=STYLE\n"
        "[setting boot]\nkind=toggle\nlabel=BOOT\napply=restart\n"
        "[setting lock]\nkind=toggle\nlabel=LOCK\ndefault=on\n";
    static const char requires[] = "[require test.luau]\nversion=1.0\nsetting.lock=on\n";
    for (unsigned i = 0; i < 2; ++i) {
        length = snprintf(path, sizeof path, "%s/%s", mod_root, ids[i]);
        if (length < 0 || (size_t)length >= sizeof path) return 2;
        p4_os_mkdir(path);
        for (unsigned source = 0; source < 2; ++source) {
            length = snprintf(path, sizeof path, "%s/%s/%s", mod_root, ids[i],
                source ? "main.luau" : "mod.ini");
            if (length < 0 || (size_t)length >= sizeof path) return 2;
            fixture = fopen(path, "wb");
            if (!fixture) return 2;
            wrote = source ? fputs("-- Native VM witness supplies source below.\n", fixture) :
                fprintf(fixture, "[mod]\nid=%s\nname=Native VM witness\nversion=1.0\napi=1.6\npriority=%u\norder=0\n%s",
                        ids[i], i, i ? requires : settings);
            closed = fclose(fixture);
            if (wrote < 0 || closed) return 2;
        }
    }
    setting_declarations_witness();
    char settings_root[1200];
    length = snprintf(settings_root, sizeof settings_root, "%s/settings", argv[1]);
    if (length < 0 || (size_t)length >= sizeof settings_root) return 2;
    p4_os_mkdir(settings_root);
    /* The data directory outlives a run (reused build trees): start from no stored values. */
    for (unsigned i = 0; i < 4; ++i) {
        static const char *const stale[] = {"test.luau.ini", "test.luau.ini.tmp", "test.isolation.ini",
                                            "test.isolation.ini.tmp"};
        length = snprintf(path, sizeof path, "%s/mod-settings/%s", settings_root, stale[i]);
        if (length < 0 || (size_t)length >= sizeof path) return 2;
        if (remove(path) && errno != ENOENT) p4_os_rmdir(path);
    }
    CHECK(p4_mod_setting_store_root(settings_root) == 0);
    char reason[256];
    CHECK(p4_mod_setting_store("test.luau", "boot", "on", reason, sizeof reason) == 0);
    CHECK(p4_package_handler_register(&p4_mod_settings_boot_provider) == 0);
    if (p4_mods_stage(mod_root, "test.luau,test.isolation") || p4_package_registry_publish()) return 2;
    if (vfs_mount(NULL, argv[1])) return 2;
    const P4PackageProvider *provider = p4_packages_provider(0);
    const P4PackageProvider *other_provider = p4_packages_provider(1);
    CHECK(provider && other_provider && p4_mods_count() == 2);
    CHECK(p4pc_bytes_observer(observe_bytes) == 0);
    CHECK(p4_lua_create(NULL) == NULL);
    P4LuaMod *mod = p4_lua_create(provider);
    CHECK(mod != NULL);
    if (!mod) return 1;
    if(benchmark_requested) {
        int result=hook_cost(mod,setup_start);
        p4_lua_destroy(mod);
        p4_package_registry_retire();
        return result;
    }
    saved_provenance(0);
    run(mod,
        "local b=p4.mem.alloc(16); p4.mem.write('u32',b,7); p4.mem.write_string(b,16,'owned scratch'); b:release()\n"
        "assert(not pcall(p4.mem.write,'u32',0x100000,1))\n"
        "local h=p4.hook(0x100100,'II','before',function(c) end); h:remove()\n"
        "local e=p4.events.on('frame_start',function(e) end); e:remove()\n",1);
    CHECK(!mod->semantic_used);
    saved_provenance(0); /* Compile/registration/owned scratch are not gameplay. */
    run(mod,"semantic_hook=p4.hook(0x100100,'II','before',function(c) end)",1);
    saved_provenance(0);
    CHECK(mod_test_unsigned(10)==11&&mod->semantic_used);
    saved_provenance(1); /* Actual script dispatch records its selected pin. */
    run(mod,"semantic_hook:remove(); semantic_hook=nil",1);
    /* p4.settings: declared toggle/choice/restart-bound values and undeclared data keys. */
    run(mod,
        "assert(p4.settings.get('flag')==false and p4.settings.get('style')=='A' and p4.settings.get('boot')==true)\n"
        "assert(p4.settings.set('flag',true) and p4.settings.get('flag')==true)\n"
        "assert(p4.settings.set('style','C') and p4.settings.get('style')=='C')\n"
        "assert(not pcall(p4.settings.set,'style','D') and not pcall(p4.settings.set,'flag','yes'))\n"
        "assert(p4.settings.set('boot',false) and p4.settings.get('boot')==true)\n"
        "assert(p4.settings.set('run.count',3) and p4.settings.get('run.count')==3)\n"
        "assert(p4.settings.set('note','hello world') and p4.settings.get('note')=='hello world')\n"
        "assert(p4.settings.set('note',nil) and p4.settings.get('note')==nil)\n"
        "assert(p4.settings.set('done',false) and p4.settings.get('done')==false)\n"
        "assert(not pcall(p4.settings.set,'Bad Key',1) and not pcall(p4.settings.set,'x',0/0))\n"
        "assert(not pcall(p4.settings.set,'x',' edge') and not pcall(p4.settings.set,'x',{}))\n"
        /* An admitted dependent locks the row it requires; reads stay legal. */
        "local ok, why = p4.settings.set('lock', false)\n"
        "assert(ok == false and why == 'locked: required by Native VM witness' and p4.settings.get('lock') == true)\n"
        "assert(not pcall(p4.mods.required, 'test.isolation'))\n", 1);
    CHECK(p4_packages_requirer(provider, "lock") == other_provider && p4_packages_requirer(provider, "flag") == NULL);
    CHECK(p4_packages_requirer(provider, NULL) == other_provider && p4_packages_requirer(other_provider, NULL) == NULL);
    /* p4.contract: frozen plain values published while main.luau runs. */
    run(mod,
        "p4.contract.publish('profile', 3, {mode='hard', list={1,2}, nested={deep=true}})\n"
        "assert(not pcall(p4.contract.publish,'profile',4,true))\n"
        "assert(not pcall(p4.contract.publish,'other',1,nil) and not pcall(p4.contract.publish,'Bad Name',1,true))\n"
        "assert(not pcall(p4.contract.publish,'zero',0,true) and not pcall(p4.contract.publish,'f',1,function() end))\n"
        "assert(not pcall(p4.contract.publish,'big',1,string.rep('x',70000)))\n"
        "for i=1,15 do p4.contract.publish('c'..i,1,i) end\n"
        "assert(not pcall(p4.contract.publish,'c16',1,16))\n"
        "assert(not pcall(p4.contract.read,'test.isolation','profile'))\n", 1);
    run(mod, "late_hook=p4.hook(0x100100,'II','before',function(c) late_ok=pcall(p4.contract.publish,'late',1,true) end)", 1);
    CHECK(mod_test_unsigned(10) == 11);
    run(mod, "assert(late_ok==false); late_hook:remove(); late_hook=nil", 1);
    {
        /* The file round-trips from disk, and the admitted restart value stays put. */
        char value[P4_MOD_SETTING_VALUE_MAX];
        CHECK(p4_mod_setting_store_root(NULL) == 0 && p4_mod_setting_store_root(settings_root) == 0);
        CHECK(p4_mod_setting_stored("test.luau", "flag", value, sizeof value, reason, sizeof reason) == 1 && !strcmp(value, "on"));
        CHECK(p4_mod_setting_stored("test.luau", "style", value, sizeof value, reason, sizeof reason) == 1 && !strcmp(value, "C"));
        CHECK(p4_mod_setting_stored("test.luau", "boot", value, sizeof value, reason, sizeof reason) == 1 && !strcmp(value, "off"));
        CHECK(p4_mod_setting_stored("test.luau", "run.count", value, sizeof value, reason, sizeof reason) == 1 && !strcmp(value, "n:3"));
        CHECK(p4_mod_setting_stored("test.luau", "note", value, sizeof value, reason, sizeof reason) == 0);
        CHECK(p4_mod_setting_boot(provider, "boot", value, sizeof value) == 1 && !strcmp(value, "on"));
        CHECK(p4_mod_setting_boot(provider, "run.count", value, sizeof value) == 0);
        CHECK(p4_mod_setting_boot(other_provider, "flag", value, sizeof value) == 0);
    }
    run(mod, "assert(p4.settings.get('style')=='C' and p4.settings.get('run.count')==3)", 1);
    /* No settings file (P4PC_CONFIG=none): defaults, and writes refused. */
    CHECK(p4_mod_setting_store_root(NULL) == 0);
    run(mod, "local ok, why = p4.settings.set('flag', false); assert(ok == false and why)\n"
             "assert(p4.settings.get('flag') == false and p4.settings.get('style') == 'A')", 1);
    /* A failed replace changes neither the cached value nor the old file. */
    CHECK(p4_mod_setting_store_root(settings_root) == 0);
    length = snprintf(path, sizeof path, "%s/mod-settings/test.luau.ini.tmp", settings_root);
    CHECK(length > 0 && (size_t)length < sizeof path && p4_os_mkdir(path) == 0);
    run(mod, "local ok, why = p4.settings.set('style', 'A'); assert(ok == false and why)\n"
             "assert(p4.settings.get('style') == 'C')", 1);
    CHECK(p4_os_rmdir(path) == 0);
    CHECK(p4_mod_setting_store_root(NULL) == 0 && p4_mod_setting_store_root(settings_root) == 0);
    run(mod, "assert(p4.settings.get('style') == 'C' and p4.settings.get('flag') == true)", 1);
    /* p4.lifecycle: no session without a settings directory, then the real one. */
    CHECK(p4_mod_lifecycle_root(NULL) == 0);
    run(mod, "local s, why = p4.lifecycle.session(); assert(s == nil and why)\n"
             "local ok, why2 = p4.lifecycle.tag('R1:B1'); assert(ok == false and why2)", 1);
    CHECK(p4_mod_lifecycle_root(settings_root) == 0);
    run(mod, "local s = p4.lifecycle.session(); assert(type(s) == 'number' and s >= 1 and p4.lifecycle.session() == s)\n"
             "assert(p4.lifecycle.tag('R1:B1') == true and p4.lifecycle.tag(nil) == true)\n"
             "local ok, why = p4.lifecycle.tag(string.rep('x', 64)); assert(ok == false and why)\n"
             "assert(p4.lifecycle.crashed(s) == false and p4.lifecycle.consume(s) == true)\n"
             "assert(not pcall(p4.lifecycle.crashed, 0) and not pcall(p4.lifecycle.tag, {}))", 1);
    CHECK(p4_mod_lifecycle_root(NULL) == 0);
#if defined(__SSE__)
    {
        /* A script entered from the game's EE float mode (state_dump.c: MXCSR
         * round-toward-zero, flush and denormal zeroing) runs in the default
         * mode, and the game gets its mode back. Runtime values, not folded
         * constants; the win32/64w twins use mingw's fesetenv, which keeps MXCSR. */
        const unsigned ee = 0xE0C0u, game = _mm_getcsr() | ee;
        _mm_setcsr(game);
        run(mod, "local a, b = tonumber('0.1'), tonumber('0.2')\n"
                 "assert(a + b == 0.30000000000000004, 'rounds to nearest')\n"
                 "assert(tonumber('1e-310') * tonumber('1') > 0, 'keeps denormals')\n"
                 "assert(string.format('%.2f %.2f', tonumber('1.15'), tonumber('0.87')) == '1.15 0.87')", 1);
        CHECK((_mm_getcsr() & ee) == ee);
        _mm_setcsr(game & ~ee);
    }
#endif
    /* p4.artifacts: refused without a settings directory, then mod-confined files. */
    CHECK(p4_mod_artifact_root(NULL) == 0);
    run(mod, "local ok, why = p4.artifacts.write('a.txt', 'x'); assert(ok == false and why)\n"
             "assert(p4.artifacts.folder() == nil and p4.artifacts.read('a.txt') == nil)", 1);
    CHECK(p4_mod_artifact_root(settings_root) == 0);
    run(mod, "assert(p4.artifacts.write('runs/r1/spoiler.txt', 'seed 0') == true)\n"
             "local b = buffer.create(3); buffer.writeu8(b, 2, 7)\n"
             "assert(p4.artifacts.write('runs/r1/layer.bin', b) == true)\n"
             "assert(buffer.tostring(p4.artifacts.read('runs/r1/spoiler.txt')) == 'seed 0')\n"
             "assert(buffer.readu8(p4.artifacts.read('runs/r1/layer.bin', 3), 2) == 7)\n"
             "local none, why = p4.artifacts.read('runs/r1/layer.bin', 2); assert(none == nil and why)\n"
             "assert(p4.artifacts.read('runs/r1/missing.txt') == nil)\n"
             "assert(string.find(p4.artifacts.folder(), 'mod%-artifacts/test%.luau$'))\n"
             "assert(not pcall(p4.artifacts.write, '../x', 'y') and not pcall(p4.artifacts.write, 'nul', 'y'))\n"
             "assert(not pcall(p4.artifacts.write, 'x', {}) and not pcall(p4.artifacts.read, '.x'))\n"
             "local ok, why2 = p4.artifacts.write('RUNS/x', 'y'); assert(ok == false and why2)", 1);
    CHECK(p4_mod_artifact_root(NULL) == 0);
    /* p4.budget: what this invocation may still spend (here the startup fuel). */
    run(mod, "local first = p4.budget(); assert(first > 4096 and first <= 65536)\n"
             "local n = 0; for i = 1, 200 do n += i end\n"
             "assert(p4.budget() < first - 150)", 1);
    run(mod,
        "assert(p4.language=='Luau' and p4.language_version=='0.744' and p4.api_version=='" P4_PACKAGE_API_TEXT "')\n"
        "assert(p4.fixture_native and p4.ui==nil and p4.scene==nil and p4.camera==nil and p4.clock==nil and p4.elements==nil and p4.audio==nil and p4.battle.menu==nil and p4.encounter==nil and p4.hud==nil and p4.calendar==nil and p4.pace==nil)\n"
        "for _,key in {'os','io','debug','package','require','load','loadstring','loadfile','dofile','vector','integer'} do assert(_G[key]==nil,key) end\n"
        "assert(type(pcall)=='function' and type(xpcall)=='function' and type(getfenv)=='function' and type(coroutine.resume)=='function')\n"
        "assert(string.dump==nil and p4.mem._pin==nil)\n"
        "scratch=p4.mem.alloc(128); a=scratch.address; assert(scratch.size==128)\n"
        "p4.mem.write('u32',a+1,0xfedcba98); assert(p4.mem.read('u32',a+1)==0xfedcba98)\n"
        "p4.mem.write('u64',a+8,p4.u64(0xffffffff,0xfffffffe)); local hi,lo=p4.split64(p4.mem.read('u64',a+8)); assert(hi==0xffffffff and lo==0xfffffffe)\n"
        "p4.mem.write('i64',a+8,p4.u64(0x80000000,0)); hi,lo=p4.split64(p4.mem.read('i64',a+8)); assert(hi==0x80000000 and lo==0)\n"
        "p4.mem.write('i8',a,-128); assert(p4.mem.read('i8',a)==-128)\n"
        "p4.mem.write('i16',a,-32768); assert(p4.mem.read('i16',a)==-32768)\n"
        "p4.mem.write('i32',a,-2147483648); assert(p4.mem.read('i32',a)==-2147483648)\n"
        "p4.mem.write('f32',a+16,1.25); assert(p4.mem.read('f32',a+16)==1.25)\n"
        "p4.mem.write('f64',a+24,-7.125); assert(p4.mem.read('f64',a+24)==-7.125)\n"
        "p4.mem.write_string(a+32,4,'abc'); assert(p4.mem.read_string(a+32,4)=='abc')\n"
        "layout=p4.mem.layout{name='Record',size=16,fields={value={offset=0,type='i32'},items={offset=4,type='u16',count=4},name={offset=12,type='string',capacity=4}}}\n"
        "view=layout:view(a+40); view.value=-17; view.items[3]=65535; view.name='new'\n"
        "assert(view.value==-17 and #view.items==4 and view.items.count==4 and view.items[3]==65535 and view.name=='new')\n"
        "assert(view.items[5]==nil and not pcall(function() rawset(view,'value',1) end))\n"
        "assert(p4.call(0x100120,'hbH',-7,12)==5 and p4.call(0x100160,'pp',a)==a)\n"
        "local fn=p4.fn(0x100100); assert(fn.address==0x100100 and p4.call(fn,'II',10)==11)\n"
        "functions=p4.mem.layout{name='Functions',size=4,fields={target={offset=0,type='fn'}}}:view(a+96)\n"
        "functions.target=fn; assert(functions.target.address==0x100100)\n"
        "assert(not pcall(function() p4.mem.write('u8',a+97,1) end)); functions.target=nil\n", 1);
    run(mod, "assert(buffer.tostring(p4.files.read('cdrom0:\\\\data\\\\test.txt;1'))=='mod-resource')", 1);
    run(mod, "for i=1,100 do assert(not pcall(p4.files.read,'DATA/TEST.TXT',3)) end; assert(buffer.len(p4.files.read('DATA/TEST.TXT'))==12)", 1);
    run(mod, "p4.files.read('../outside')", 0);
    run(mod, "assert(p4.has('core') and p4.has('core@1') and p4.has('capabilities@1') and not p4.has('core@999'))\n"
             "assert(not p4.has('made.up') and p4.capabilities().core == 1 and p4.capabilities()['capabilities'] == 1)\n"
             "assert(not pcall(p4.has, 'Core') and not pcall(p4.has, 'core@0') and not pcall(p4.has, {}))", 1);
    run(mod, "p4.mem.read('u32',0x1fffffe)", 0);
    run(mod, "p4.mem.write('u8',a,256)", 0);
    run(mod, "assert(p4.has('table-source.item-derive@1') and p4.has('table-source.skill-source@1') and p4.has('table-source.status@1') and p4.has('manifest.element-ui@2'))", 1);
    run(mod, "assert(p4.has('calendar.pass@2') and p4.has('calendar.hold@1') and p4.has('manifest.table-derived@2'))", 1);
    run(mod, "assert(p4.has('tables.overlay@2') and p4.has('artifacts@1') and p4.has('budget@1') and p4.has('sha256@1'))", 1);
    run(mod, "assert(p4.has('battle.menu@1'))", 1);
    run(mod, "assert(p4.has('pace@1') and p4.has('party.record@1'))", 1);
    run(mod, "assert(p4.has('manifest.resource-model@1') and p4.has('manifest.resource-model@2') and p4.has('manifest.resource-model@3') and not p4.has('manifest.resource-model@4'))", 1);
    run(mod, "assert(p4.has('stage.damage@2') and p4.has('stage.recovery@1') and p4.has('stage.cost@1') and p4.has('stage.chance@1'))", 1);
    run(mod, "assert(p4.has('battle.substitute@1'))", 1);
    run(mod, "assert(p4.has('manifest.table-delta@1'))", 1);
    run(mod, "assert(p4.has('table-source.skill-derive@1') and p4.has('table-source.skill-delivery@1'))", 1);
    run(mod, "assert(p4.has('manifest.element-header@1') and p4.has('manifest.element-prompt@1'))", 1);
    run(mod, "assert(p4.has('tables.source@1') and p4.has('scene.player-position@1'))", 1);
    run(mod, "assert(p4.has('authorize.weapon_fusion@1') and p4.has('persona.stock@1') and p4.has('social.rank@1')"
             " and p4.has('forge@1') and p4.has('velvet.command@1') and p4.has('manifest.companion-setting@1')"
             " and p4.has('party.hero_level@1') and p4.has('forge.vessel@1'))", 1);
    run(mod, "p4.mem.write('i16',a,32768)", 0);
    run(mod, "p4.mem.write('u32',a,0/0)", 0);
    run(mod, "p4.mem.write('u64',a,9007199254740992)", 0);
    run(mod, "p4.mem.write('u64',a,buffer.create(7))", 0);
    run(mod, "p4.mem.read('u8',newproxy(true))", 0);
    run(mod, "p4.mem.write_string(a+32,4,'long')", 0);
    run(mod, "assert(p4.mem.read_string(a+32,4)=='abc')", 1);
    run(mod, "p4.mem.write('u32',a+32,0x41414141); p4.mem.read_string(a+32,4)", 0);
    run(mod, "layout:view(a+40,true).value=5", 0);
    run(mod, "view.items[5]=1", 0);
    run(mod, "p4.mem.layout{name='Bad',size=2,fields={bad={offset=1,type='u32'}}}", 0);
    run(mod, "p4.call(0x100120,'iii',-7,12)", 0);
    run(mod, "p4.call(0x100120,'hbH',-129,12)", 0);
    run(mod, "p4.call(0x100121,'hbH',-7,12)", 0);
    run(mod, "p4.call(0x100120,'hbH',-7)", 0);
    run(mod, "while true do end", 0);
    run(mod, "pcall(function() while true do end end)", 0);
    lua_pushcfunction(mod->state, nested_callback, "test_nested");
    lua_setglobal(mod->state, "test_nested");
    run(mod, "quota_inner=function() end; while true do test_nested() end", 0);
    run(mod, "quota_inner=function() for i=1,15000 do end end; while true do test_nested() end", 0);
    run(mod, "test_nested=nil; quota_inner=nil", 1);
    run(mod, "local x=string.rep('x',16*1024*1024)", 0);
    run(mod, "error({bad=1})", 0);
    const char binary[] = "\033Lua\0not-source";
    CHECK(p4_lua_compile(mod, binary, sizeof binary - 1, "binary-refused") == -1);
    run(mod, "action_body=function() p4.call(0x100100,'II',1) end", 1);
    CHECK(p4_lua_invoke(mod, synchronous_callback, NULL, 0, P4_LUA_SYNCHRONOUS) == -1);
    run(mod, "action_body=function() p4.settings.set('run.count', 4) end", 1);
    CHECK(p4_lua_invoke(mod, synchronous_callback, NULL, 0, P4_LUA_SYNCHRONOUS) == -1);
    run(mod, "assert(p4.settings.get('run.count') == 3)", 1);
    run(mod,
        "before=p4.hook(0x100100,'II','before',function(c) c.args[1]+=2 end)\n"
        "after=p4.hook(0x100100,'II','after',function(c) c.result*=2 end)\n"
        "replacement=p4.hook(0x100100,'II','replace',function(c) saved=c; c.next(); c.result+=5 end)\n"
        "assert(p4.call(0x100100,'II',10)==11)\n"
        "recursive_calls=0; recursive=p4.hook(0x100180,'ii','before',function(c) recursive_calls+=1 end)\n", 1);
    CHECK(mod_test_unsigned(10) == 36);
    CHECK(mod_test_recursive(3) == 3);
    run(mod, "assert(recursive_calls==4)", 1);
    run(mod, "saved.next()", 0);
    run(mod, "replacement:disable()", 1);
    CHECK(mod_test_unsigned(10) == 26);
    run(mod, "before:remove(); replacement:enable()", 1);
    CHECK(mod_test_unsigned(10) == 32);
    {
        /* p4.battle.substitute: only inside a before-hook on 0x001de640, for
         * that call's own action; the carrier decides, success rewrites args[3]. */
        unsigned char *action = p4t_galloc(0x40, 16), *slot = p4t_galloc(4, 4);
        run(mod, "assert(not pcall(p4.battle.substitute, 0x1000, 227))\n"
                 "sub_hook=p4.hook(0x1de640,'vppH','before',function(c)\n"
                 "  assert(not pcall(p4.battle.substitute, c.args[1] + 4, 227))\n"
                 "  local ok, why = p4.battle.substitute(c.args[1], 128); assert(ok == false and why)\n"
                 "  local nf, nwhy = p4.battle.substitute(c.args[1], 4000); assert(nf == false and nwhy)\n"
                 "  assert(c.args[3] == 151 and p4.battle.substitute(c.args[1], 227) == true and c.args[3] == 227)\n"
                 "end)\n"
                 "sub_after=p4.hook(0x1de640,'vppH','after',function(c) assert(not pcall(p4.battle.substitute, c.args[1], 227)) end)\n"
                 "elsewhere=p4.hook(0x100100,'II','after',function(c) assert(not pcall(p4.battle.substitute, 0x1000, 227)) end)", 1);
        substitute_calls = 0;
        mod_test_set_action_code(action, slot, 151);
        CHECK(substitute_calls == 3 && substitute_action == action && substitute_skill == 227);
        CHECK(*(uint16_t *)slot == 227);
        CHECK(mod_test_unsigned(10) == 32);
        run(mod, "sub_hook:remove(); sub_after:remove(); elsewhere:remove(); sub_hook=nil; sub_after=nil; elsewhere=nil", 1);
        mod_test_set_action_code(action, slot, 151);
        CHECK(*(uint16_t *)slot == 151 && substitute_calls == 3);
        /* A replace hook on the same function is not a before-hook either. */
        run(mod, "sub_replace=p4.hook(0x1de640,'vppH','replace',function(c)\n"
                 "  assert(not pcall(p4.battle.substitute, c.args[1], 227)); sub_replaced=true end)", 1);
        mod_test_set_action_code(action, slot, 228);
        CHECK(*(uint16_t *)slot == 151 && substitute_calls == 3);
        run(mod, "assert(sub_replaced); sub_replace:remove(); sub_replace=nil; sub_replaced=nil", 1);
    }
    P4LuaMod *other = p4_lua_create(other_provider);
    CHECK(other != NULL);
    if (other) {
        run(other, "assert(scratch==nil and saved==nil and type(buffer.create)=='function')", 1);
        run(other, "assert(p4.settings.get('flag')==nil and p4.settings.get('run.count')==nil)", 1);
        run(other, "local r = p4.mods.required('test.luau')\n"
                   "assert(r.version == '1.0' and r.settings.lock == true and next(r.settings, 'lock') == nil)\n"
                   "assert(not pcall(p4.mods.required, 'test.missing') and not pcall(p4.mods.required, 'test.isolation'))", 1);
        run(other, "local v, version = p4.contract.read('test.luau','profile')\n"
                   "assert(version == 3 and v.mode == 'hard' and v.list[2] == 2 and v.nested.deep == true)\n"
                   "assert(not pcall(function() v.mode = 'easy' end) and not pcall(function() v.nested.deep = false end))\n"
                   "local again = p4.contract.read('test.luau','profile'); assert(again ~= v and again.mode == 'hard')\n"
                   "assert(p4.contract.read('test.luau','missing') == nil and p4.contract.read('test.luau','c15') == 15)\n"
                   "assert(p4.contract.read('test.luau','late') == nil and not pcall(p4.contract.read,'test.ghost','profile'))", 1);
        run(other, "p4.hook(0x100100,'II','replace',function(c) c.result=0 end)", 0);
        lua_getglobal(mod->state, "a");
        uint32_t address = lua_tounsigned(mod->state, -1);
        lua_pop(mod->state, 1);
        char source[160];
        snprintf(source, sizeof source, "p4.mem.read('u8',%u)", (unsigned)address);
        run(other, source, 0);
        p4_lua_destroy(other);
    }
    run(mod, "before:remove(); after:remove(); replacement:remove(); recursive:remove()", 1);
    coroutine_witness(mod);
    run(mod, "frames=0; ev=p4.events.on('frame_start',function(e) assert(e.name=='frame_start'); frames+=e.frame end)", 1);
    p4_mod_event_frame(42);
    run(mod, "assert(frames==42); ev:disable()", 1);
    CHECK(!(p4_mod_event_mask & P4_MOD_FRAME_START_BIT));
    p4_mod_event_frame(2);
    run(mod, "assert(frames==42); ev:enable()", 1);
    p4_mod_event_frame(3);
    run(mod, "assert(frames==45); ev:remove()", 1);
    run(mod, "local d,p,r=p4.input.action('confirm'); assert(not d and not p and not r)", 1);
    uint8_t packet[32] = {0};
    packet[2] = 0xff; packet[3] = 0xbf;
    packet[4] = 255; packet[5] = 128; packet[6] = 0; packet[7] = 192;
    p4_mod_input_sample(packet, 1);
    run(mod, "local d,p,r=p4.input.action('confirm'); assert(d and p and not r); local x,y,rx,ry=p4.input.sticks(); assert(x==-1 and y==0.5 and rx==127/128 and ry==0)", 1);
    p4_mod_input_sample(packet, 2);
    run(mod, "local d,p,r=p4.input.action('confirm'); assert(d and not p and not r)", 1);
    packet[3] = 0xff;
    p4_mod_input_sample(packet, 3);
    run(mod, "local d,p,r=p4.input.action('confirm'); assert(not d and not p and r)", 1);
    int rounding = fegetround();
    fesetround(FE_DOWNWARD);
    run(mod, "p4.mem.write('f32',a+16,1.000000059604644775390625); assert(p4.mem.read('u32',a+16)==0x3f800000)", 1);
    CHECK(fegetround() == FE_DOWNWARD);
    fesetround(rounding);
    run(mod, "scratch:release(); assert(view.value==-17); view.value=-18; assert(view.value==-18); scratch=nil", 1);
    lua_gc(mod->state, LUA_GCCOLLECT, 0);
    run(mod, "assert(view.value==-18)", 1);
    other = p4_lua_create(other_provider);
    CHECK(other != NULL);
    if (other) {
        run(other, "for i=1,128 do p4.hook(0x100100,'II','before',function(c) end):remove() end", 1);
        CHECK(other->subscriptions == 128);
        run(other, "p4.hook(0x100100,'II','before',function(c) end)", 0);
        /* Empty callbacks still consume frame fuel; a real flip renews it and
         * exhaustion remains sticky until a new host-side source invocation. */
        run(other, "", 1);
        int exhausted = 0;
        for (unsigned i = 0; i < 4096; ++i)
            exhausted |= p4_lua_invoke(other, empty_callback, NULL, 0, 0);
        CHECK(!exhausted);
        p4pc_vsync_wait();
        for (unsigned i = 0; i < 8192; ++i)
            exhausted |= p4_lua_invoke(other, empty_callback, NULL, 0, 0);
        CHECK(!exhausted);
        CHECK(p4_lua_invoke(other, empty_callback, NULL, 0, 0) != 0);
        p4pc_vsync_wait();
        CHECK(p4_lua_invoke(other, empty_callback, NULL, 0, 0) != 0);
        run(other, "assert(true)", 1);
        /* A finished large job leaves garbage that callbacks allocating almost
         * nothing never collect; later flips must reclaim it before a burst. */
        run(other, "local junk = {} for i = 1, 4200 do junk[i] = table.create(64, i) end junk = nil", 1);
        CHECK(p4_luau_memory_used(other->vm) > (4u << 20));
        for (unsigned i = 0; i < 32; ++i) {
            p4pc_vsync_wait();
            CHECK(p4_lua_invoke(other, empty_callback, NULL, 0, 0) == 0);
        }
        CHECK(p4_luau_memory_used(other->vm) < (4u << 20));
        p4_lua_destroy(other);
    }
    p4pc_vsync_wait(); /* a fresh frame budget for the stage callbacks */
    damage_witness(mod, other_provider);
    p4pc_vsync_wait();
    stages_witness(mod, other_provider);
    p4pc_vsync_wait();
    formula_stages_witness(mod);
    passive_witness(mod, other_provider);
    lua_getglobal(mod->state, "a");
    watched_scratch = P4_G2H(lua_tounsigned(mod->state, -1));
    lua_pop(mod->state, 1);
    p4_lua_destroy(mod);
    /* A destroyed owner's contracts are gone; readers see nil, never stale bytes. */
    other = p4_lua_create(other_provider);
    CHECK(other != NULL);
    if (other) {
        run(other, "assert(p4.contract.read('test.luau','profile') == nil)", 1);
        p4_lua_destroy(other);
    }
    CHECK(canonical_copies > 0 && watched_invalidations == 1 && watched_value == 0xee);
    CHECK(p4pc_bytes_observer(NULL) == 0);
    p4_mod_events_clear();
    p4_mod_hooks_clear();
    CHECK(mod_test_unsigned(10) == 11);
    p4_package_registry_retire();
    CHECK(p4_mods_count() == 0);
    CHECK(native_initializations > 0 && native_initializations == native_destructions);
    printf("mod_luau: %s (sandbox, quotas, LE-wide buffers, pinned layouts, known function words, hooks, real non-LIFO coroutines/restarts, events/input, FP restore)\n",
        failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
#ifdef P4_MOD_NATIVE_RECOVERY
static unsigned managed_entries, good_source_reads, bad_source_reads;
static void managed_read(const P4PackageFileOrigin *origin, const void *bytes,
                         size_t size, uint64_t offset, void *user)
{
    (void)bytes; (void)size; (void)offset; (void)user;
    if (strcmp(origin->relative, "main.luau")) return;
    if (!strcmp(origin->package->id, "mod.good")) ++good_source_reads;
    else if (!strcmp(origin->package->id, "mod.bad")) ++bad_source_reads;
    else CHECK(0);
}
static int managed_witness(int argc, char **argv)
{
    CHECK(argc == 3 && ++managed_entries == 1);
    if (!p4_mod_witness_enabled()) return 2;
    CHECK(p4pc_thread_context_key() == 0);
    p4cfg_load();
    p4_os_setenv("P4PC_MOD_DIR",argv[2],1);
    if(p4_mod_native_failure_init()||p4_mod_script_failure_init())return 2;
    const char *enabled = p4cfg_str("mods.enabled", "");
    printf("managed entry mod_witness=true reference_eligible=false enabled=%s\n", enabled);
    if (
        p4_package_reads_observe(managed_read, NULL) ||
        p4_mods_init() ||
        p4_package_registry_publish() || vfs_mount(NULL, NULL)) {
        fputs("managed loader fixture could not prepare\n", stderr);
        return 2;
    }
    p4_package_boot_admission_complete();
    int bad = 0;
    for (size_t i = 0; i < p4_mods_count(); ++i)
        bad |= !strcmp(p4_mods_info(i)->id, "mod.bad");
    int empty = !strcmp(argv[1], "--managed-empty");
    CHECK(good_source_reads == (empty ? 0u : 1u));
    CHECK(p4_mod_event_mask == 0); /* All selected chunks compiled; none executed. */
    int fd = vfs_open("DATA/TEST.TXT", 0);
    char bytes[5] = {0};
    CHECK(empty ? fd < 0 : fd >= 0);
    if (fd >= 0) {
        CHECK(vfs_read(fd, bytes, sizeof bytes - 1) == (bad ? 3 : 4));
        CHECK(!strcmp(bytes, bad ? "bad" : "good"));
        CHECK(vfs_close(fd) == 0);
    }
    printf("managed compile barrier good_reads=%u bad_reads=%u mods=%zu mask=%u raw=%s\n",
           good_source_reads, bad_source_reads, p4_mods_count(), (unsigned)p4_mod_event_mask, bytes);
    fflush(stdout);
    adopt_native_context(); /* Pass1 also works before the native boot task exists. */
    p4_mod_scripts_start(); /* Runtime failure must use the real native backend. */
    CHECK(p4_mod_event_mask == (empty ? 0u : P4_MOD_FRAME_START_BIT));
    if (!strcmp(argv[1], "--managed-callback")) p4_mod_event_frame(1);
    if (!strcmp(argv[1], "--managed-hook")) CHECK(mod_test_unsigned(10) == 11);
    CHECK(!bad); /* A bad runtime chunk must never fall through after its error. */
    p4_mod_scripts_stop(); /* Fixture has no queued work; callback stacks are idle. */
    CHECK(p4_mod_event_mask == 0);
    p4_package_registry_retire();
    CHECK(p4_package_reads_unobserve(managed_read, NULL) == 0);
    P4PackageMemoryUsage memory = p4_packages_memory_usage();
    CHECK(memory.resident == 0 && memory.reserved == 0);
    if (!strcmp(argv[1], "--managed-exit-normal") || !strcmp(argv[1], "--managed-exit-failure")) {
        /* Deliberate automation completion before the title, as harnesses and
         * gates end: only code 0 is a normal exit for startup health. */
        printf("managed deliberate exit: %s\n", failures ? "FAIL" : "PASS");
        fflush(stdout);
        p4pc_exit(failures ? 1 : !strcmp(argv[1], "--managed-exit-normal") ? 0 : 3);
    }
    p4_package_boot_clean_exit(); /* Normal host-fixture completion, not a fake title frame. */
    printf("managed %s: %s\n", empty || !strcmp(argv[1], "--managed-preflight") ? "pre-code refusal" : "fresh-entry recovery",
           failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
#endif
int main(int argc, char **argv)
{
    #ifdef P4_MOD_NATIVE_RECOVERY
    if (p4_os_restart_init(argc, argv)) return 2;
    #endif
    if (argc == 2 && (!strcmp(argv[1], "--active-clear") || !strcmp(argv[1], "--active-owner-remove"))) {
        event_teardown_mode = !strcmp(argv[1], "--active-clear") ? 1 : 2;
        CHECK(p4_mod_events_quiescent());
        if (!p4_mod_event_subscribe(P4_MOD_NATIVE_OWNER, "frame_start", event_teardown, NULL)) return 2;
        p4_mod_event_frame(1);
        return failures ? 1 : 0;
    }
    p4_os_init();
    p4_os_setenv("P4PC_VIRTUAL_CLOCK", "1", 1);
    p4_os_setenv("P4PC_TURBO", "0", 1);
    if(argc==2&&!strcmp(argv[1],"--frame-cpu"))
        return p4t_on_guest_stack(frame_cpu_witness,argc,argv);
    if(argc==3&&!strcmp(argv[1],"--hook-cost")) {
        /* A CPU-time budget holds only natively: under qemu-user (an AArch64 build tested on an x86_64 host) every
           call costs the emulation, so the case reports itself skipped (CTest SKIP_RETURN_CODE 77) with the reason. */
        const char *emulated=getenv("P4PC_PERF_EMULATED");
        if(emulated&&*emulated) {
            printf("hook_cost skipped: its CPU-time budget is native-only and this run is emulated (%s)\n",emulated);
            return 77;
        }
        benchmark_requested=1;
        return p4t_on_guest_stack(witness,argc-1,argv+1);
    }
    #ifdef P4_MOD_NATIVE_RECOVERY
    if (argc == 3 && (!strcmp(argv[1], "--managed-preflight") || !strcmp(argv[1], "--managed-runtime") ||
                      !strcmp(argv[1], "--managed-callback") || !strcmp(argv[1], "--managed-hook") ||
                      !strcmp(argv[1], "--managed-empty") || !strcmp(argv[1], "--managed-exit-normal") ||
                      !strcmp(argv[1], "--managed-exit-failure")))
        return p4t_on_guest_stack(managed_witness, argc, argv);
    #endif
    if (argc == 4 && !strcmp(argv[1], "--stage-failure") &&
        (!strcmp(argv[2], "recovery") || !strcmp(argv[2], "chance") || !strcmp(argv[2], "cost"))) {
        stage_failure_mode = argv[2];
        argv[2] = argv[0];
        return p4t_on_guest_stack(witness, argc - 2, argv + 2);
    }
    return p4t_on_guest_stack(witness, argc, argv);
}
