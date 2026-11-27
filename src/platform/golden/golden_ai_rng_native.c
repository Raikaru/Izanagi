#include "golden_ai_rng_native.h"
#ifdef P4_GOLDEN_AI_QUALIFICATION
#include "golden_ai_qualification.h"
#endif
#include "p4pc/game_mode.h"
#include <stdlib.h>
static P4GoldenAiRng additive;
static P4GoldenSfmt sfmt;
static int seeded;
#ifdef P4_GOLDEN_AI_QUALIFICATION
static int qualification;
#endif
int p4_golden_ai_native_seed(uint32_t seed)
{
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN
#ifdef P4_GOLDEN_AI_QUALIFICATION
        && !qualification
#endif
    ) return 0;
    p4_golden_ai_rng_seed(&additive,seed);
    p4_golden_sfmt_seed(&sfmt,seed);
    seeded=1; return 1;
}
int p4_golden_ai_native_random(uint32_t *value)
{
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN
#ifdef P4_GOLDEN_AI_QUALIFICATION
        && !qualification
#endif
    ) return 0;
    if(!seeded || !value) abort();
    *value=p4_golden_ai_rng_next(&additive); return 1;
}
int p4_golden_ai_native_streams(P4GoldenAiRng **out_additive,P4GoldenSfmt **out_sfmt)
{
    if(!seeded) return -1;
    *out_additive=&additive; *out_sfmt=&sfmt; return 0;
}
#ifdef P4_GOLDEN_AI_QUALIFICATION
void p4_golden_ai_qualification_seed(uint32_t seed)
{
    p4_golden_ai_rng_seed(&additive,seed);
    p4_golden_sfmt_seed(&sfmt,seed);
    seeded=1;
    qualification=1;
}
#endif
