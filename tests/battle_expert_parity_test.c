/* Differential decision tests on synthetic snapshots, not native battle parity. */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include "../src/platform/battle_sim/expert.h"
int bsim_choose_c(const struct bsim_observation *, int, struct bsim_choice *);

int main(void)
{
    for (unsigned seed = 0; seed < 400; seed++) {
        struct bsim_observation o = {0};
        o.unit_count = 2; o.actor_index = 0; o.candidate_count = 7;
        o.is_boss = seed & 1; o.durable = (seed >> 1) & 1; o.setup_pending = (seed >> 2) & 1;
        o.units[0] = (struct bsim_unit_observation){.action_uid = 1, .unit_uid = 1, .native_id = 1,
            .hp = 1 + seed % 100, .max_hp = 100, .sp = seed % 101, .max_sp = 100};
        o.units[0].expired_buffs = (seed & 256) ? 1 : 0;
        o.units[1] = (struct bsim_unit_observation){.action_uid = 2, .unit_uid = 2, .native_id = 20,
            .side = 1, .charged = (seed >> 3) & 1, .weak = (seed & 16) ? 2 : 0,
            .tried = (seed & 32) ? 2 : 0, .bad = (seed & 64) ? 2 : 0};
        o.units[1].buffs[0] = (seed & 128) ? 1 : 0;
        o.persona_count = 2; o.lead_slot = 0;
        o.personas[0].slot = 0; o.personas[0].stats[1] = 10;
        o.personas[1].slot = 1; o.personas[1].stats[1] = (seed & 128) ? 9 : 11;
        o.candidates[0] = (struct bsim_candidate_observation){.wheel = 3, .skill = 1, .hp_effect = 1,
            .power = 20, .element = 1, .targets = 2, .first_target = 1, .persona_slot = UINT8_MAX};
        o.candidates[1] = (struct bsim_candidate_observation){.wheel = 2, .persona_slot = UINT8_MAX};
        o.candidates[2] = (struct bsim_candidate_observation){.wheel = 6, .item = 769, .skill = 288,
            .hp_effect = 5, .power = 20, .targets = 1, .persona_slot = UINT8_MAX};
        o.candidates[3] = (struct bsim_candidate_observation){.wheel = 4, .skill = 218,
            .buff_mask = 5, .targets = 1, .persona_slot = UINT8_MAX};
        o.candidates[4] = (struct bsim_candidate_observation){.wheel = 4, .skill = 224,
            .buff_mask = 0x1000, .targets = 2, .first_target = 1, .persona_slot = UINT8_MAX};
        o.candidates[5] = (struct bsim_candidate_observation){.wheel = 4, .skill = 3, .hp_effect = 1,
            .power = 1000, .element = 1, .targets = 2, .first_target = 1, .persona_slot = 1,
            .cost_kind = 2, .cost = 12};
        o.candidates[6] = (struct bsim_candidate_observation){.wheel = 4, .skill = 222,
            .buff_mask = 0x40, .targets = 1, .persona_slot = UINT8_MAX};
        for (int expert = 0; expert <= 1; expert++) {
            struct bsim_choice a, b;
            const int ca = bsim_choose_c(&o, expert, &a), cb = bsim_choose(&o, expert, &b);
            assert(ca == cb);
            if (ca) assert(a.candidate == b.candidate && a.target == b.target && a.score == b.score);
        }
    }
    return 0;
}
