#ifndef P4_TEST_GOLDEN_LITERAL_CONSUMERS_H
#define P4_TEST_GOLDEN_LITERAL_CONSUMERS_H
#include "../src/platform/golden/golden_mode_identity.h"
void p4t_literal_synthetic(P4GoldenModeIdentityArtifacts *artifacts);
void p4t_literal_seed_ps2(void);
void p4t_literal_consumers(const P4GoldenModeIdentityArtifacts *artifacts, int golden);
#endif
