#include "p4pc/vu0.h"

u_long128 mdlEffectInitParticleState(u8 *arg0)
{
    Code48InitialState *state = (Code48InitialState *)(void *)arg0;
    const u_long128 *quadSource;
    u_long128 quad;

    memset(state, 0, sizeof(*state));
    p4_vu0_sqc2(state, 0);
    p4_vu0_sqc2((u8 *)state + 16, 0);
    p4_vu0_sqc2((u8 *)state + 64, 0);
    state->vector40[1] = 5.0f;
    p4_vu0_sqc2((u8 *)state + 80, 0);
    quadSource = (const u_long128 *)(const void *)D_00713CE0;
    quad = *quadSource;
    state->quad20 = quad;
    state->scalar60 = 1.0f;
    state->scalar74 = 1.0f;
    state->word64 = 0xFFFFFFFF;
    state->word68 = 0x80;
    return quad;
}
