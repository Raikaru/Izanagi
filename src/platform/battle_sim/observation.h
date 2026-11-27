#ifndef P4PC_BATTLE_SIM_OBSERVATION_H
#define P4PC_BATTLE_SIM_OBSERVATION_H

#include <stdint.h>

/* A snapshot contains only visible state, party resources, and remembered
 * outcomes. Enemy HP, affinities and RNG are deliberately not represented. */
enum { BSIM_UNITS_MAX = 12, BSIM_KNOWLEDGE_MAX = 64, BSIM_ITEMS_MAX = 1536,
       BSIM_PERSONAS_MAX = 12, BSIM_CANDIDATES_MAX = BSIM_ITEMS_MAX + 10 + 8 * BSIM_PERSONAS_MAX };
struct bsim_knowledge {
    uint32_t unit_uid, tried, weak, bad, vulnerable, probed, probed2;
    uint16_t native_id;
    uint8_t valid;
};
struct bsim_unit_observation {
    uint64_t action_uid;
    uint32_t unit_uid, status, tried, weak, bad, vulnerable, probed, probed2, expired_buffs;
    uint16_t native_id, hp, sp, max_hp, max_sp;
    int8_t buffs[5];
    uint8_t side, charged;
};
struct bsim_candidate_observation {
    uint32_t buff_mask, status_mask, cost;
    uint16_t skill, item, menu_index, targets, held_targets, first_target;
    int16_t power, element;
    uint8_t wheel, group, hp_effect, sp_effect, cost_kind, status_mode, persona_slot;
};
struct bsim_persona_observation {
    uint16_t id, skills[8];
    uint8_t slot, stats[5];
};
struct bsim_observation {
    struct bsim_unit_observation units[BSIM_UNITS_MAX];
    struct bsim_persona_observation personas[BSIM_PERSONAS_MAX];
    struct bsim_candidate_observation candidates[BSIM_CANDIDATES_MAX];
    uint32_t focus_uid, healer_uid[8];
    uint8_t healer_count, durable, setup_pending, switched;
    uint16_t unit_count, candidate_count, actor_index, persona_count, lead_slot;
    uint8_t is_boss;
};
/* Knowledge is copied from observed hit packets, never hidden affinity. */
int bsim_observe(struct bsim_observation *out, uint32_t panel, uint32_t actor,
                 uint32_t scratch, uint32_t skill_base, uint32_t skill_rows,
                 const struct bsim_knowledge *knowledge, unsigned knowledge_count);

#endif
