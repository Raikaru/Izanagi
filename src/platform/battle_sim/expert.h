#ifndef P4PC_BATTLE_SIM_EXPERT_H
#define P4PC_BATTLE_SIM_EXPERT_H
#include "observation.h"
#include <stdint.h>

struct bsim_choice {
    int64_t score;
    uint16_t candidate, target;
};
/* Select exclusively from the copied observation; returns zero if there is no
 * legal command. target is the observation's unit index. */
int bsim_choose(const struct bsim_observation *observation, int expert, struct bsim_choice *choice);
/* Optional human-visible decision evidence; stream is a native FILE pointer. */
void bsim_trace_decision(void *stream, const struct bsim_observation *observation,
                         const struct bsim_choice *choice, unsigned policy, unsigned update);
#endif
