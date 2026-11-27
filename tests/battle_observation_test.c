/* Synthetic guest layout verifies the extraction boundary, not native combat. */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "p4pc/gmem.h"
#include "p4pc/game_items_native.h"
#include "../src/platform/battle_sim/expert.h"

/* The guest-facing functions observation.c calls are defined on every host so that the 32-bit build links; only a 64-bit
 * host runs the checks, over a synthetic guest laid out with the 64-bit guest mapping. */
static unsigned buff_lookups;
uint16_t datCalcMaxHpById(int16_t id) { (void)id; return 100; }
uint16_t datCalcMaxSpById(int16_t id) { (void)id; return 100; }
uint16_t datCalcUnderlyingSkillId(int64_t item) { return (uint16_t)item; }
int32_t datCalcDefaultSkillId(void *u) { (void)u; return 1; }
int32_t datCalcThresholdTrigger(void *u, int32_t s) { (void)u; (void)s; return 0; }
int32_t datCalcHasBuffMask(void *u, int32_t m) { (void)u; (void)m; buff_lookups++; return 0; }
uint32_t datCalcSkillCost(void *u, int32_t s) { (void)u; (void)s; return 0; }
int32_t datCalcSkillElement(void *u, uint16_t s) { (void)u; (void)s; return 1; }
int32_t btlTargetCheckCommandAvailability(void *a, int64_t k) { (void)a; return k == 1 || k == 7; }
uint16_t btlSkillClassifyTargets(void *a, void *selection, uint16_t skill, uint32_t reverse)
{
    (void)a; (void)skill; (void)reverse;
    uint32_t scratch = P4_H2G(selection);
    P4_G32(scratch) = 0x111000;
    P4_G16(scratch + 0x38) = 1;
    return 1;
}
void btlEffectCollectTargetUnits(void *a, void *s, uint32_t x, uint32_t y, uint32_t z)
{ (void)a; (void)s; (void)x; (void)y; (void)z; assert(0); }
int p4_battle_sim_is_boss(void) { return 1; }
void *datPersonaRosterSlot(int16_t slot) { (void)slot; return 0; }
int32_t datPersonaGetLeadSlot(void) { return 0; }
uint16_t datPersonaCountValidMembers(void) { return 0; }
uint16_t *datPersonaGetSkillsByPcId(uint16_t id) { (void)id; return 0; }
int p4_game_cost_native_active(void) { return 0; }
int p4_game_skill_active(uint32_t id) { (void)id; return 0; }
uint32_t p4_game_tables_items_native_row_count(void) { return 1536; }
P4GameItemBattleRow *p4_game_tables_items_native_battle_rows(uint8_t *panel) { return (P4GameItemBattleRow *)panel; }

#if UINTPTR_MAX <= UINT32_MAX
int main(void) { return 77; }
#else
static uint8_t memory[P4_GUEST_SIZE];
/* The 64-bit twins link the real guest runtime (BSIM_EXTERNAL_GUEST), which defines these. The original is also built natively in an
 * AArch64 build directory on an x86_64 host without it, so there the stubs are weak and the real definitions win. */
#ifdef BSIM_EXTERNAL_GUEST
#if defined(__ELF__) || defined(__APPLE__)
#define P4_TEST_GUEST_STUB __attribute__((weak))
#endif
#else
#define P4_TEST_GUEST_STUB
#endif
#ifndef P4_GUEST_SLOW_CC
#define P4_GUEST_SLOW_CC /* the 32-bit guest model has no slow paths; the stubs below are unused there */
#endif
#ifdef P4_TEST_GUEST_STUB
P4_TEST_GUEST_STUB uint8_t *p4_guest_base = memory;
P4_TEST_GUEST_STUB void *p4_g2h_slow(uint32_t a) P4_GUEST_SLOW_CC { (void)a; return 0; }
P4_TEST_GUEST_STUB uint32_t p4_h2g_slow(const void *p) P4_GUEST_SLOW_CC { (void)p; return 0; }
#endif

int main(void)
{
    p4_guest_base = memory;
    const uint32_t battle = 0x100000, actor = 0x110000, enemy = 0x111000;
    const uint32_t party_unit = 0x112000, enemy_unit = 0x113000;
    const uint32_t party_record = 0x114000, enemy_record = 0x115000;
    const uint32_t panel = 0x116000, scratch = 0x117000, skills = 0x120000, affinity = 0x130000;
    P4_GPDATA(uint32_t, 0xb3ac) = battle;
    P4_GPDATA(uint32_t, 0xb3c8) = affinity;
    P4_G32(battle + 0x174) = actor;
    P4_G32(actor) = 1; P4_G32(actor + 0x30) = party_unit; P4_G32(actor + 0x450) = enemy;
    P4_G32(enemy) = 2; P4_G32(enemy + 0x30) = enemy_unit;
    P4_G32(party_unit + 0xa64) = party_record; P4_G16(party_unit + 0xa4) = 1; P4_G32(party_unit + 0xa8) = 10;
    P4_G32(enemy_unit + 0xa64) = enemy_record; P4_G8(enemy_unit + 0xa2) = 1;
    P4_G16(enemy_unit + 0xa4) = 2; P4_G32(enemy_unit + 0xa8) = 20;
    P4_G16(party_record + 8) = 100; P4_G16(party_record + 10) = 50;
    P4_G8(skills + 0x28 + 0x11) = 1; P4_G16(skills + 0x28 + 0x12) = 20;
    P4_G32(skills + 0x28 + 0x20) = 0xffffffffu; /* Damage rows never ask the native buff predicate. */
    struct bsim_observation before, after;
    assert(bsim_observe(&before, panel, actor, scratch, skills, 2, 0, 0));
    assert(buff_lookups == 0);
    struct bsim_choice first, second;
    assert(bsim_choose(&before, 1, &first));
    /* Enemy HP, nonpublic status bits, affinities and RNG are not observable. */
    P4_G16(enemy_record + 8) = 9999;
    P4_G32(enemy_record + 12) = 0x40000000u;
    P4_G32(affinity + 0x20) = 0xffffffffu;
    P4_G32(0x140000) = 0xabcdef01u;
    assert(bsim_observe(&after, panel, actor, scratch, skills, 2, 0, 0));
    assert(bsim_choose(&after, 1, &second));
    assert(!memcmp(&before, &after, sizeof before));
    assert(first.candidate == second.candidate && first.target == second.target && first.score == second.score);
    FILE *left = tmpfile(), *right = tmpfile();
    assert(left && right);
    bsim_trace_decision(left, &before, &first, 9, 1);
    bsim_trace_decision(right, &after, &second, 9, 1);
    rewind(left); rewind(right);
    int a, b, bytes = 0;
    do {
        a = fgetc(left); b = fgetc(right);
        assert(a == b);
        bytes += a != EOF;
    } while (a != EOF);
    assert(bytes > 0);
    assert(!fclose(left) && !fclose(right));
    return 0;
}

#endif