/* Link the original C scorer beside the Zig export for differential tests. */
#define bsim_choose bsim_choose_c
#include "../src/platform/battle_sim/expert.c"
