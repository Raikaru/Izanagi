/* The side-ailment battle readers (game_side_ailment_battle.c) over the real
 * service, a frozen registry and guest DatUnits, with the native calculators
 * they call stubbed: inertness (no RNG draw without a running battle), the
 * rider arithmetic of 00238940, the single slot, Guard, immunity, SMT V decay,
 * commit and cure, the crit bonus, the lost turn and defeat. A rule witness,
 * not a battle qualification. */
#include "p4pc/game_side_ailment.h"
#include "p4pc/game_side_ailment_native.h"
#include "p4pc/game_elements_native.h"
#include "p4pc/game_table_use.h"
#include "p4pc/package_boot.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{
    fprintf(stderr, "unexpected native refusal: %s\n", reason);
    abort();
}
static int use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(view == context && ids);
    assert(domain == P4_TABLE_SOURCE_SIDE_AILMENT || domain == P4_TABLE_SOURCE_SIDE_RIDER);
    return P4_TABLE_OK;
}

/* Native stubs: per-unit Luck, guard, buff 7, Ailment Boost, status affinity
 * word and a scripted RpRandom. */
static uint8_t *units[3];
static uint8_t luck[3];
static int guarding[3], exposed[3], boosted;
static uint32_t affinity[3];
static uint32_t rolls[8], roll_count, roll_next;
static int index_of(const void *unit)
{
    for (int i = 0; i < 3; ++i) if (units[i] == unit) return i;
    assert(!"unknown unit");
    return -1;
}
int32_t datCalcIsGuarding(uint16_t *unit) { return guarding[index_of(unit)]; }
uint8_t datCalcGetBattleStat(void *unit, int32_t stat) { assert(stat == 4); return luck[index_of(unit)]; }
int8_t datCalcGetBuffLevel(void *unit, int32_t buff) { assert(buff == 7); return (int8_t)exposed[index_of(unit)]; }
int32_t datCalcHasSkill(void *unit, int32_t skill) { (void)unit; assert(skill == 0x21E); return boosted; }
uint32_t RpRandom(void) { assert(roll_next < roll_count); return rolls[roll_next++]; }
/* Routing off: the retail skill element decides physical delivery. */
static int32_t skill_elements[0x40];
int32_t datCalcSkillElement(void *unit, uint16_t skill) { (void)unit; assert(skill < 0x40); return skill_elements[skill]; }
static int routing;
int p4_game_elements_native_enabled(void) { return routing; }
/* Routing on: Analyze learns a landing on an enemy. */
static unsigned learned;
static uint32_t learned_leaf;
static const void *learned_unit;
void p4_game_elements_native_learn_leaf(void *defender, uint32_t leaf)
{
    ++learned; learned_leaf = leaf; learned_unit = defender;
}
int32_t p4_game_elements_native_presentation_leaf(void *unit, uint32_t skill_id) { (void)unit; (void)skill_id; abort(); }
uint32_t p4_game_elements_native_affinity_leaf(void *unit, uint32_t leaf, uint32_t channel, uint32_t component)
{
    assert((leaf == 24 || leaf == 25) && channel == P4_CHANNEL_STATUS && component == P4_ACTION_COMPONENT_STATUS);
    return affinity[index_of(unit)];
}
static void script(uint32_t value) { rolls[0] = value; roll_count = 1; roll_next = 0; }
static unsigned drawn(void) { return roll_next; }

static P4GameTables *tables_with_riders(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    for (uint32_t leaf = 24; leaf <= 25; ++leaf) {
        P4ElementDesc d = {.id = leaf, .flags = P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, .family = leaf,
            .order = leaf, .revision = 1, .display_name = "Ailment", .short_name = "Ail"};
        strcpy(d.key, leaf == 24 ? "test:freeze" : "test:shock");
        assert(p4_game_tables_element(tables, "test", &d) == P4_TABLE_OK);
    }
    const P4SideAilmentDef freeze = {24, P4_SIDE_FLAGS, 0x16, 2, 100, 0, 0, 1, 1, "test:freeze-ailment"};
    /* Shock halves an enemy's riders against the party (enemy_scale 50). */
    const P4SideAilmentDef shock = {25, P4_SIDE_LOSE_TURN, 0x16, 3, 100, 50, 50, 1, 1, "test:shock-ailment"};
    assert(p4_game_tables_side_ailment(tables, "test", &freeze) == P4_TABLE_OK);
    assert(p4_game_tables_side_ailment(tables, "test", &shock) == P4_TABLE_OK);
    const P4SideRiderDesc bufu = {0x1A, 24, 10, 1, 1, "test:bufu-freeze"};
    const P4SideRiderDesc zio = {0x27, 25, 10, 1, 1, "test:zio-shock"};
    const P4SideRiderDesc reign = {0x2E, 25, 100, 1, 1, "test:reign-shock"};
    const P4SideRiderDesc niflheim = {0x21, 24, 100, 1, 1, "test:niflheim-freeze"};
    /* One skill with two riders: Freeze (single slot) first, then Shock. */
    const P4SideRiderDesc both_freeze = {0x33, 24, 50, 1, 1, "test:both-freeze"};
    const P4SideRiderDesc both_shock = {0x33, 25, 50, 1, 1, "test:both-shock"};
    assert(p4_game_tables_side_rider(tables, "test", &zio) == P4_TABLE_OK);
    assert(p4_game_tables_side_rider(tables, "test", &reign) == P4_TABLE_OK);
    assert(p4_game_tables_side_rider(tables, "test", &both_shock) == P4_TABLE_OK);
    assert(p4_game_tables_side_rider(tables, "test", &niflheim) == P4_TABLE_OK);
    assert(p4_game_tables_side_rider(tables, "test", &both_freeze) == P4_TABLE_OK);
    assert(p4_game_tables_side_rider(tables, "test", &bufu) == P4_TABLE_OK);
    assert(!p4_game_tables_bind_runtime_use(tables, use, tables));
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    return tables;
}
static void reset_units(void)
{
    for (int i = 0; i < 3; ++i) {
        memset(units[i], 0, 0x40);
        luck[i] = 10; guarding[i] = 0; exposed[i] = 0; affinity[i] = 100;
    }
    boosted = 0;
}
int main(void)
{
    if (p4t_guest_init() || p4t_guest_map(P4_GPADDR(0x811c), 4)) return 77;
    P4_GPDATA(float, 0x811c) = 0.8f; /* fGpffff811c, the enemy-source ailment scale */
    uint8_t *storage = p4t_galloc(3 * 0x40, 16); assert(storage);
    for (int i = 0; i < 3; ++i) units[i] = storage + i * 0x40;
    uint8_t *source = units[0], *target = units[1], *other = units[2];
    const uint32_t gtarget = P4_H2G(target);
    uint32_t leaf;
    reset_units();

    /* Nothing published: inert, reads no unit and draws nothing. */
    script(0);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(!drawn() && !p4_side_ailment_native_occupied(target) && !p4_side_ailment_native_lose_turn(target));
    assert(!p4_side_ailment_native_crit_bonus(target, 0) && !p4_side_ailment_native_spend_turn(target));
    p4_side_ailment_native_commit(target, 0xFF, 0, source, 0x1A);
    p4_side_ailment_native_defeat(target);
    p4_side_ailment_native_turn_end(target);
    p4_side_ailment_native_action_begin();

    P4GameTables *tables = tables_with_riders();
    size_t first, count;
    assert(p4_game_tables_find_side_riders(tables, 0x33, &first, &count) && count == 2);
    assert(p4_game_tables_side_rider_at(tables, first)->leaf == 24 && p4_game_tables_side_rider_at(tables, first + 1)->leaf == 25);
    assert(!p4_game_tables_find_side_riders(tables, 0x1B, &first, &count) && !count);
    assert(p4_game_side_ailment_prepare(tables) == P4_TABLE_OK);
    p4_game_side_ailment_publish();

    /* Published but outside a battle: still no draw. */
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(!drawn());
    p4_game_side_ailment_native_battle_begin();
    /* The per-run gate is off: still inert. */
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(!drawn() && !p4_side_ailment_active());
    p4_game_side_ailment_native_battle_end();
    assert(p4_side_ailment_set_enabled(1) == P4_TABLE_OK);
    p4_game_side_ailment_native_battle_begin();
    assert(p4_side_ailment_active());

    /* A skill without a rider draws nothing. */
    p4_side_ailment_native_riders(0x1B, source, target, 1, 0, 0);
    assert(!drawn());

    /* Party source, Luck 20 vs 10, normal: 10 x 1 x 120/110 -> 10, x1 -> 10. */
    luck[0] = 20;
    script(10);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(drawn() == 1 && !p4_side_ailment_landed(gtarget, &leaf)); /* 10 is not below 10 */
    script(9);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(drawn() == 1 && p4_side_ailment_landed(gtarget, &leaf) && leaf == 24);
    assert(p4_side_ailment_native_occupied(target) && !p4_side_ailment_native_lose_turn(target));
    /* A marked landing fills the slot for the rest of the action. */
    script(0);
    p4_side_ailment_native_riders(0x27, source, target, 1, 0, 0);
    assert(!drawn());
    p4_side_ailment_native_action_begin();
    assert(!p4_side_ailment_native_occupied(target));

    /* Enemy source x0.8: 10 x 120/110 x 0.8 -> 8. */
    *(uint16_t *)source = 4;
    script(8);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(!p4_side_ailment_landed(gtarget, &leaf));
    script(7);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(p4_side_ailment_landed(gtarget, &leaf));
    p4_side_ailment_native_action_begin();
    *(uint16_t *)source = 0;

    /* enemy_scale 50: an enemy hitting a party member rolls half the chance
     * (10 x 120/110 x 0.8 -> 8.7, x 0.5 -> 4); an enemy hitting an enemy and a
     * party source are unscaled (8 and 10). */
    *(uint16_t *)source = 4;
    script(4);
    p4_side_ailment_native_riders(0x27, source, target, 1, 0, 0);
    assert(drawn() == 1 && !p4_side_ailment_landed(gtarget, &leaf));
    script(3);
    p4_side_ailment_native_riders(0x27, source, target, 1, 0, 0);
    assert(drawn() == 1 && p4_side_ailment_landed(gtarget, &leaf) && leaf == 25);
    p4_side_ailment_native_action_begin();
    /* A base of 100 no longer lands unrolled: 100 x 120/110 x 0.8 x 0.5 -> 43. */
    script(43);
    p4_side_ailment_native_riders(0x2E, source, target, 1, 0, 0);
    assert(drawn() == 1 && !p4_side_ailment_landed(gtarget, &leaf));
    script(42);
    p4_side_ailment_native_riders(0x2E, source, target, 1, 0, 0);
    assert(drawn() == 1 && p4_side_ailment_landed(gtarget, &leaf));
    p4_side_ailment_native_action_begin();
    *(uint16_t *)target = 4;
    script(8);
    p4_side_ailment_native_riders(0x27, source, target, 1, 0, 0);
    assert(drawn() == 1 && !p4_side_ailment_landed(gtarget, &leaf));
    script(7);
    p4_side_ailment_native_riders(0x27, source, target, 1, 0, 0);
    assert(drawn() == 1 && p4_side_ailment_landed(gtarget, &leaf));
    p4_side_ailment_native_action_begin();
    *(uint16_t *)target = 0;
    *(uint16_t *)source = 0;
    script(10);
    p4_side_ailment_native_riders(0x27, source, target, 1, 0, 0);
    assert(drawn() == 1 && !p4_side_ailment_landed(gtarget, &leaf));
    script(9);
    p4_side_ailment_native_riders(0x27, source, target, 1, 0, 0);
    assert(drawn() == 1 && p4_side_ailment_landed(gtarget, &leaf));
    p4_side_ailment_native_action_begin();
    script(0);
    p4_side_ailment_native_riders(0x2E, source, target, 1, 0, 0); /* party base 100: no roll */
    assert(!drawn() && p4_side_ailment_landed(gtarget, &leaf));
    p4_side_ailment_native_action_begin();
    assert(!learned); /* routing off */

    /* Analyze: with routing on, only a landing on an enemy teaches its
     * (leaf, status) pair; a failed roll or a party target teaches nothing. */
    routing = 1;
    script(9);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(p4_side_ailment_landed(gtarget, &leaf) && !learned);
    p4_side_ailment_native_action_begin();
    *(uint16_t *)target = 4;
    script(10);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(!p4_side_ailment_landed(gtarget, &leaf) && !learned);
    script(9);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(p4_side_ailment_landed(gtarget, &leaf) && learned == 1 && learned_leaf == 24 && learned_unit == target);
    p4_side_ailment_native_action_begin();
    *(uint16_t *)target = 0;
    routing = 0;
    learned = 0;

    /* Weak 125, applied twice as 00238940 does: 10 x 1.25 x 120/110 -> 13, x1.25 -> 16;
     * exposed x2 -> 32 and Ailment Boost x1.25 -> 40. */
    affinity[1] = P4_AFF_WEAK | 125u;
    script(16);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(!p4_side_ailment_landed(gtarget, &leaf));
    exposed[1] = 1; boosted = 1;
    script(39);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(p4_side_ailment_landed(gtarget, &leaf));
    p4_side_ailment_native_action_begin();
    /* options & 1 leaves out buff 7 and Ailment Boost: back to 16. */
    script(16);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 1);
    assert(!p4_side_ailment_landed(gtarget, &leaf));
    exposed[1] = 0; boosted = 0; affinity[1] = 100;

    /* Immune, nulled or repelling, Guard and a retail ailment refuse without a draw. */
    script(0);
    affinity[1] = 0x20000000u | 100u;
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    affinity[1] = P4_AFF_REPEL | 100u;
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    affinity[1] = 100;
    guarding[1] = 1;
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    guarding[1] = 0;
    *(uint32_t *)(target + 0xC) = 0x04; /* Fear holds the single slot. */
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    *(uint32_t *)(target + 0xC) = 0x100000; /* Down does not. */
    assert(!drawn());

    /* A miss, null or repelled outcome (not 1) rolls nothing. */
    p4_side_ailment_native_riders(0x1A, source, target, 4, 0, 0);
    p4_side_ailment_native_riders(0x1A, source, target, 0x200, 0, 0);
    assert(!drawn());
    /* A base of 100 lands without a roll. */
    p4_side_ailment_native_riders(0x21, source, target, 1, 0, 0);
    assert(!drawn() && p4_side_ailment_landed(gtarget, &leaf) && leaf == 24);
    p4_side_ailment_native_action_begin();
    /* Two riders: a retail ailment this hit inflicts (Fear) skips the
     * single-slot Freeze rider; the Shock rider still rolls and lands. */
    script(0);
    p4_side_ailment_native_riders(0x33, source, target, 1, 0x04, 0);
    assert(drawn() == 1 && p4_side_ailment_landed(gtarget, &leaf) && leaf == 25);
    p4_side_ailment_native_action_begin();
    /* Without it, the first (Freeze) rider lands and ends the loop. */
    script(0);
    p4_side_ailment_native_riders(0x33, source, target, 1, 0, 0);
    assert(drawn() == 1 && p4_side_ailment_landed(gtarget, &leaf) && leaf == 24);
    p4_side_ailment_native_action_begin();

    /* Commit: the mark becomes the held ailment. */
    script(0);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(drawn() == 1 && p4_side_ailment_landed(gtarget, &leaf));
    p4_side_ailment_native_commit(target, 0, 0, source, 0x1A);
    assert(p4_side_ailment_native_occupied(target) && p4_side_ailment_native_lose_turn(target));
    assert(!p4_side_ailment_landed(gtarget, &leaf) && p4_side_ailment_landings(gtarget, 24) == 1);
    /* A held ailment fills the slot. */
    script(0);
    p4_side_ailment_native_riders(0x27, source, target, 1, 0, 0);
    assert(!drawn());
    /* Crit bonus on an unresisted hit only. */
    assert(p4_side_ailment_native_crit_bonus(target, 100) == 100);
    assert(p4_side_ailment_native_crit_bonus(target, P4_AFF_RESIST | 50u) == 0);
    assert(p4_side_ailment_native_crit_bonus(other, 100) == 0);
    /* Re Patra's mask does not cover 0x16; Patra's does. */
    p4_side_ailment_native_commit(target, 0x100001, 0, source, 0x1A);
    assert(p4_side_ailment_native_occupied(target));
    p4_side_ailment_native_commit(target, 0x16, 0, source, 0x1A);
    assert(!p4_side_ailment_native_occupied(target));

    /* SMT V decay: one prior landing at 100 leaves 70: 10 x 0.7 x 120/110 -> 7, x0.7 -> 4. */
    script(4);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(!p4_side_ailment_landed(gtarget, &leaf));
    script(3);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(p4_side_ailment_landed(gtarget, &leaf));
    p4_side_ailment_native_commit(target, 0, 0, source, 0x1A);
    assert(p4_side_ailment_landings(gtarget, 24) == 2);

    /* The lost turn: reported once at the bad state, recovered at its end. */
    assert(p4_side_ailment_native_spend_turn(target) == 1 && !p4_side_ailment_native_spend_turn(other));
    p4_side_ailment_native_turn_end(other);
    p4_side_ailment_native_turn_end(target);
    assert(!p4_side_ailment_native_occupied(target));

    /* Defeat clears it; four prior landings at 100 decay to nothing. */
    assert(p4_side_ailment_set(gtarget, 24, P4_SIDE_REASON_MOD) == P4_TABLE_OK);
    p4_side_ailment_native_defeat(target);
    assert(!p4_side_ailment_native_occupied(target));
    assert(p4_side_ailment_set(gtarget, 24, P4_SIDE_REASON_MOD) == P4_TABLE_OK);
    (void)p4_side_ailment_clear(gtarget, P4_SIDE_REASON_MOD);
    assert(p4_side_ailment_landings(gtarget, 24) == 4);
    script(0);
    p4_side_ailment_native_riders(0x1A, source, target, 1, 0, 0);
    assert(!drawn());

    /* Break on hit: a damaging hit clears a held Freeze, but not the Freeze the
     * same packet lands; a zero-damage packet leaves it. */
    assert(p4_side_ailment_set(gtarget, 24, P4_SIDE_REASON_MOD) == P4_TABLE_OK);
    p4_side_ailment_native_commit(target, 0, 0, source, 0x05);
    assert(p4_side_ailment_native_occupied(target));
    p4_side_ailment_native_commit(target, 0, -12, source, 0x05);
    assert(!p4_side_ailment_native_occupied(target));
    assert(p4_side_ailment_mark_landed(gtarget, 24) == P4_TABLE_OK);
    p4_side_ailment_native_commit(target, 0, -12, source, 0x05);
    assert(p4_side_ailment_native_occupied(target));
    (void)p4_side_ailment_clear(gtarget, P4_SIDE_REASON_MOD);

    /* Transfer: a damaging physical hit on a Shock holder moves it to the
     * attacker at 50% x the attacker's susceptibility (100%). */
    skill_elements[0x27] = 3; /* Zio is Elec */
    assert(p4_side_ailment_set(gtarget, 25, P4_SIDE_REASON_MOD) == P4_TABLE_OK);
    script(0);
    p4_side_ailment_native_commit(target, 0, -12, source, 0x27); /* An Elec hit never moves it. */
    p4_side_ailment_native_commit(target, 0, 0, source, 0x05);   /* No damage, no draw. */
    p4_side_ailment_native_commit(target, 0, -12, target, 0x05); /* Its own reflected hit. */
    assert(!drawn() && p4_side_ailment_native_occupied(target));
    assert(p4_side_ailment_protect(P4_H2G(source), 1) == P4_TABLE_OK);
    p4_side_ailment_native_commit(target, 0, -12, source, 0x05);
    assert(!drawn() && p4_side_ailment_protect(P4_H2G(source), 0) == P4_TABLE_OK);
    affinity[0] = P4_AFF_NULL | 100u; /* An attacker immune to it: no draw. */
    p4_side_ailment_native_commit(target, 0, -12, source, 0x05);
    assert(!drawn());
    affinity[0] = 100;
    script(50);
    p4_side_ailment_native_commit(target, 0, -12, source, 0x05);
    assert(drawn() == 1 && p4_side_ailment_native_occupied(target) && !p4_side_ailment_native_occupied(source));
    script(49);
    p4_side_ailment_native_commit(target, 0, -12, source, 0x05);
    assert(drawn() == 1 && !p4_side_ailment_native_occupied(target) && p4_side_ailment_native_lose_turn(source));
    assert(p4_side_ailment_landings(P4_H2G(source), 25) == 1);
    /* A protected unit takes no rider. */
    (void)p4_side_ailment_clear(P4_H2G(source), P4_SIDE_REASON_MOD);
    assert(p4_side_ailment_protect(gtarget, 1) == P4_TABLE_OK);
    script(0);
    p4_side_ailment_native_riders(0x21, source, target, 1, 0, 0);
    assert(!drawn() && !p4_side_ailment_native_occupied(target));

    p4_game_side_ailment_native_battle_end();
    p4_game_side_ailment_session_end();
    p4_game_tables_destroy(tables);
    puts("game_side_ailment_battle: inertness, rider arithmetic, single slot, decay, commit, cure, crit, lost turn passed");
    return 0;
}
