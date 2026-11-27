/* Pure decision tests: these do not exercise native extraction or combat. */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "../src/platform/battle_sim/expert.h"

static struct bsim_observation scenario(void)
{
    struct bsim_observation o = {0};
    o.unit_count = 2;
    o.actor_index = 0;
    o.units[0] = (struct bsim_unit_observation){.action_uid = 1, .unit_uid = 1, .native_id = 1,
        .hp = 100, .sp = 50, .max_hp = 100, .max_sp = 100};
    o.units[1] = (struct bsim_unit_observation){.action_uid = 2, .unit_uid = 2, .native_id = 20, .side = 1};
    o.candidate_count = 2;
    o.candidates[0] = (struct bsim_candidate_observation){.wheel = 3, .skill = 1, .hp_effect = 1,
        .power = 20, .element = 1, .targets = 2, .first_target = 1, .persona_slot = UINT8_MAX};
    o.candidates[1] = (struct bsim_candidate_observation){.wheel = 2, .persona_slot = UINT8_MAX};
    return o;
}

static struct bsim_choice choose(const struct bsim_observation *o)
{
    struct bsim_choice c;
    assert(bsim_choose(o, 1, &c));
    return c;
}

static void determinism(void)
{
    struct bsim_observation o = scenario();
    o.units[1].weak = 2;
    const struct bsim_choice first = choose(&o);
    for (unsigned i = 0; i < 100; i++) {
        const struct bsim_choice next = choose(&o);
        assert(first.candidate == next.candidate && first.target == next.target && first.score == next.score);
    }
}

static void hidden_state_does_not_enter_scorer(void)
{
    struct state { struct bsim_observation visible; uint32_t rng, enemy_hp, enemy_affinity; } s = {0};
    s.visible = scenario();
    const struct bsim_choice before = choose(&s.visible);
    s.rng = 0xffffffffu; s.enemy_hp = 9999; s.enemy_affinity = 0xaaaaaaaa;
    const struct bsim_choice after = choose(&s.visible);
    assert(before.candidate == after.candidate && before.target == after.target && before.score == after.score);
}

static void priorities(void)
{
    struct bsim_observation o = scenario();
    o.units[1].charged = 1;
    assert(choose(&o).candidate == 1);
    o.units[1].charged = 0;
    o.units[0].hp = 57;
    o.units[0].max_hp = 329;
    o.candidate_count = 4;
    o.candidates[2] = (struct bsim_candidate_observation){.wheel = 6, .item = 769, .skill = 288,
        .hp_effect = 5, .power = 20, .targets = 1, .persona_slot = UINT8_MAX};
    o.candidates[3] = (struct bsim_candidate_observation){.wheel = 6, .item = 771, .skill = 290,
        .hp_effect = 5, .power = 100, .targets = 1, .persona_slot = UINT8_MAX};
    assert(choose(&o).candidate == 3);
    o.candidate_count = 5;
    o.candidates[4] = (struct bsim_candidate_observation){.wheel = 6, .item = 773, .skill = 292,
        .hp_effect = 11, .power = 30, .targets = 1, .persona_slot = UINT8_MAX};
    assert(choose(&o).candidate == 3); /* 100 HP beats 30% of 329 (98 HP). */
    o.candidates[4].power = 50;
    assert(choose(&o).candidate == 4);
    o.candidate_count = 4;
    o.units[0].hp = o.units[0].max_hp;
    o.units[1].buffs[0] = 1;
    o.candidates[2] = (struct bsim_candidate_observation){.wheel = 4, .skill = 224,
        .buff_mask = 0x1000, .targets = 2, .first_target = 1, .persona_slot = UINT8_MAX};
    assert(choose(&o).candidate == 0); /* Dispelling an enemy buff is not a priority tier. */
    o.units[1].buffs[0] = 0;
    o.units[0].buffs[0] = -1;
    o.candidates[2].buff_mask = 0x2000;
    o.candidates[2].targets = 1; o.candidates[2].first_target = 0;
    assert(choose(&o).candidate == 2); /* Removing a party debuff is. */
    o.units[0].buffs[0] = 0;
    assert(choose(&o).candidate == 0);
    o.candidates[2].held_targets = 2;
    assert(choose(&o).candidate == 0);
    o.candidates[2] = (struct bsim_candidate_observation){.wheel = 4, .skill = 218,
        .buff_mask = 5, .targets = 1, .persona_slot = UINT8_MAX};
    o.units[0].sp = 10;
    o.is_boss = 0;
    assert(choose(&o).candidate == 0);
    o.is_boss = 1;
    assert(choose(&o).candidate == 0); /* Boss context alone must not force setup over damage. */
    o.units[1].weak = 1u << 1;
    assert(choose(&o).candidate == 0); /* An observed knockdown route beats a setup. */
    o.units[1].weak = 0;
    o.setup_pending = 1;
    assert(choose(&o).candidate == 0);
    o.setup_pending = 0;
    o.units[0].sp = 50;
    o.candidates[3] = (struct bsim_candidate_observation){.wheel = 4, .skill = 222,
        .buff_mask = 0x40, .targets = 1, .persona_slot = UINT8_MAX};
    o.units[0].expired_buffs = 0x40;
    assert(choose(&o).candidate == 0);
    o.durable = 1;
    assert(choose(&o).candidate == 3); /* Durable setup remains available; an expired buff gets its bonus. */
    o.durable = 0;
    o.units[0].expired_buffs = 0;
    o.persona_count = 3;
    o.lead_slot = 0;
    o.personas[0].slot = 0;
    o.personas[1].slot = 2;
    o.personas[2].slot = 3;
    o.personas[0].stats[1] = 10;
    o.personas[1].stats[1] = 9;
    o.personas[2].stats[1] = 10;
    o.candidates[2] = (struct bsim_candidate_observation){.wheel = 4, .skill = 20, .hp_effect = 1,
        .power = 1000, .element = 1, .targets = 2, .first_target = 1, .persona_slot = 2};
    o.candidates[3] = (struct bsim_candidate_observation){.wheel = 4, .skill = 10, .hp_effect = 1,
        .power = 1000, .element = 1, .targets = 2, .first_target = 1, .persona_slot = 3};
    assert(choose(&o).candidate == 3);
    o.personas[1].stats[1] = 10;
    assert(choose(&o).candidate == 2);
    o.candidate_count = 3;
    o.candidates[2] = (struct bsim_candidate_observation){.wheel = 4, .skill = 50, .hp_effect = 1,
        .power = 1000, .buff_mask = 1, .targets = 1, .persona_slot = UINT8_MAX};
    assert(choose(&o).candidate == 0);
}

static void boss_context_does_not_ration_sp(void)
{
    struct bsim_observation o = scenario();
    o.units[0].sp = 60;
    o.candidates[0].power = 300;
    o.candidates[0].cost_kind = 2;
    o.candidates[0].cost = 20;
    o.candidates[1] = o.candidates[0];
    o.candidates[1].power = 100;
    o.candidates[1].cost_kind = 0;
    o.candidates[1].cost = 0;
    assert(choose(&o).candidate == 0);
    o.is_boss = 1;
    assert(choose(&o).candidate == 0); /* No SP horizon: a boss does not make a costly skill lose. */
}

static void strategic_candidate_rules(void)
{
    struct bsim_observation o = scenario();
    struct bsim_choice c;
    o.units[0].hp = 20;
    o.units[1].charged = 1;
    o.candidate_count = 3;
    o.candidates[2] = (struct bsim_candidate_observation){.wheel = 6, .item = 771, .skill = 290,
        .hp_effect = 5, .power = 100, .targets = 1, .persona_slot = UINT8_MAX};
    assert(bsim_choose(&o, 0, &c) && c.candidate == 2); /* policy 8 heals above charge Guard */
    assert(bsim_choose(&o, 1, &c) && c.candidate == 2); /* Emergency healing beats a fresh charge. */
    o.units[0].hp = 100;
    assert(bsim_choose(&o, 1, &c) && c.candidate == 1); /* Guard remains preferred without an emergency. */
    o.units[1].charged = 0;
    o.durable = 1;
    o.candidates[2] = (struct bsim_candidate_observation){.wheel = 4, .skill = 218,
        .buff_mask = 5, .targets = 1, .persona_slot = UINT8_MAX};
    assert(bsim_choose(&o, 0, &c) && c.candidate == 2);
    o.setup_pending = 1;
    assert(bsim_choose(&o, 0, &c) && c.candidate == 0);
}

int main(void)
{
    determinism();
    hidden_state_does_not_enter_scorer();
    priorities();
    boss_context_does_not_ration_sp();
    strategic_candidate_rules();
    return 0;
}
