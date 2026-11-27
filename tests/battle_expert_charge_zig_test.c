/* Exercise the Zig policy through its public observer and decision interfaces. */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/battle_events.h"
#include "battle_sim/expert.h"

#if UINTPTR_MAX <= UINT32_MAX
int main(void) { return 77; }
#else
extern uint8_t *p4_guest_base;
static P4BattleObserver observer;
static const uint32_t battle = 0x2000000, action = 0x2001000, unit = 0x2002000;
static const uint32_t record = 0x2003000, task = 0x2004000, panel = 0x200509c, actor = 0x2006000;
#define G8(a) (*(uint8_t *)(p4_guest_base + (a)))
#define G16(a) (*(uint16_t *)(void *)(p4_guest_base + (a)))
#define G32(a) (*(uint32_t *)(void *)(p4_guest_base + (a)))
extern int p4_battle_policy_begin(uint32_t, uint32_t);
extern int p4_battle_policy_input(uint32_t);
extern void p4_battle_policy_before_update(void);
uint32_t p4_gport_alloc(uint32_t size, uint32_t align) { (void)size; (void)align; return 0x2007000; }
uint32_t p4_battle_observer_add(const P4BattleObserver *o) { observer = *o; return 1; }
int p4_game_table_extent_rows(uint32_t slot, uint32_t stride, uint32_t *base, uint32_t *rows)
{ (void)slot; (void)stride; *base = 0x2008000; *rows = 2; return 1; }
uint32_t p4_battle_sim_update_count(void) { return 1; }
int p4_battle_sim_opening_active(void) { return 0; }
void p4_battle_sim_command_ready(uint32_t a) { (void)a; }
void p4pc_pad_inject(unsigned bits, unsigned frames) { (void)bits; (void)frames; }
void *p4_battle_sim_decision_trace(void) { return 0; }
void bsim_trace_decision(void *s, const struct bsim_observation *o, const struct bsim_choice *c, unsigned p, unsigned u)
{ (void)s; (void)o; (void)c; (void)p; (void)u; }
/* Unused legacy-policy imports are linked into the same Zig object. */
void btlFormationEntrySetSelected(int a, void *b) { (void)a; (void)b; abort(); }
int btlTargetCheckCommandAvailability(void *a, int64_t b) { (void)a; (void)b; abort(); }
int datPersonaGetLeadSlot(void) { abort(); }
uint16_t datPersonaCountValidMembers(void) { abort(); }
void *datPersonaRosterSlot(int16_t a) { (void)a; abort(); }
int datCalcDefaultSkillId(void *a) { (void)a; abort(); }
int datCalcThresholdTrigger(void *a, int b) { (void)a; (void)b; abort(); }
uint32_t p4_game_tables_items_native_row_count(void) { abort(); }
void *p4_game_tables_items_native_battle_rows(void) { abort(); }
uint16_t datCalcUnderlyingSkillId(int64_t a) { (void)a; abort(); }
uint16_t datCalcMaxHpById(int16_t a) { (void)a; abort(); }
uint16_t datCalcMaxSpById(int16_t a) { (void)a; abort(); }
uint16_t btlSkillClassifyTargets(void *a, void *b, uint16_t c, uint32_t d)
{ (void)a; (void)b; (void)c; (void)d; abort(); }
void btlEffectCollectTargetUnits(void *a, void *b, uint32_t c, uint32_t d, uint32_t e)
{ (void)a; (void)b; (void)c; (void)d; (void)e; abort(); }
int datCalcSkillElement(void *a, uint16_t b) { (void)a; (void)b; abort(); }
uint32_t datCalcSkillCost(void *a, int b) { (void)a; (void)b; abort(); }
int datCalcHasBuffMask(void *a, int b) { (void)a; (void)b; abort(); }
int bsim_observe(struct bsim_observation *o, uint32_t p, uint32_t a, uint32_t s, uint32_t b, uint32_t r,
                 const struct bsim_knowledge *k, unsigned n)
{
    (void)p; (void)a; (void)s; (void)b; (void)r; (void)k; (void)n;
    memset(o, 0, sizeof *o);
    o->unit_count = o->candidate_count = 2;
    o->units[0] = (struct bsim_unit_observation){.action_uid = 1, .unit_uid = 1, .native_id = 1,
        .hp = 100, .max_hp = 100, .sp = 50, .max_sp = 50};
    o->units[1] = (struct bsim_unit_observation){.action_uid = 10, .unit_uid = 7, .native_id = 13,
        .side = 1, .charged = (G32(record + 0x14) & 0xc0000u) != 0};
    o->candidates[0] = (struct bsim_candidate_observation){.wheel = 3, .skill = 1, .hp_effect = 1,
        .power = 20, .element = 1, .targets = 2, .first_target = 1, .persona_slot = UINT8_MAX};
    o->candidates[1] = (struct bsim_candidate_observation){.wheel = 2, .persona_slot = UINT8_MAX};
    return 1;
}
static void expect_wheel(unsigned wheel)
{
    P4BattleCommand command = {.actor = {.action_uid = 1}};
    observer.command(&command, observer.context);
    assert(p4_battle_policy_input(panel + 0x40) == 2);
    assert(G16(panel + 0x44) == wheel);
}
int main(void)
{
    if (sizeof(void *) != 8) return 77;
    p4_guest_base = calloc(1, 0x4000000);
    assert(p4_guest_base);
    G32(0x7690f0 - 0x4c54) = battle;
    G32(battle + 0x174) = action;
    G32(battle + 0xdd4) = task;
    G32(task + 0x38) = panel - 0x9c;
    G32(panel + 0x178) = actor;
    G16(panel + 0x46) = 8;
    G16(panel + 0x48) = 8;
    G32(actor) = 1;
    G32(action) = 10;
    G32(action + 0x30) = unit;
    G8(unit + 0xa2) = 1;
    G16(unit + 0xa4) = 13;
    G32(unit + 0xa8) = 7;
    G32(unit + 0xa64) = record;
    assert(p4_battle_policy_begin(9, 0));
    G32(record + 0x14) = 0x80000;
    expect_wheel(2);
    P4BattleAdmission admission = {.actor = {.present = 1, .side = 1, .native_id = 13,
                                             .unit_uid = 7, .action_uid = 10}};
    observer.admission(&admission, observer.context);
    G16(action + 0x0c) = 0x21;
    p4_battle_policy_before_update();
    expect_wheel(3);
    G32(record + 0x14) = 0xc0000;
    expect_wheel(2);
    G32(record + 0x14) = 0x80000;
    expect_wheel(3);
    G32(record + 0x14) = 0;
    p4_battle_policy_before_update();
    G32(record + 0x14) = 0x80000;
    expect_wheel(2);
    G16(action + 0x0c) = 2;
    observer.admission(&admission, observer.context);
    G32(record + 0x14) = 0;
    p4_battle_policy_before_update();
    G32(record + 0x14) = 0x80000;
    G16(action + 0x0c) = 0x21;
    p4_battle_policy_before_update();
    expect_wheel(2);
    free(p4_guest_base);
    return 0;
}
#endif
