#ifndef P4_GOLDEN_AI_RNG_H
#define P4_GOLDEN_AI_RNG_H
#include <stdint.h>

/* Session-owned streams. Commands never seed either stream. */
typedef struct P4GoldenAiRng {
    uint32_t words[31];
    unsigned front, rear;
} P4GoldenAiRng;
typedef struct P4GoldenSfmt {
    uint32_t words[624];
    int32_t index;
    uint64_t anomalies; /* Switch-defined invalid conversion events. */
} P4GoldenSfmt;
void p4_golden_ai_rng_seed(P4GoldenAiRng *rng, uint32_t seed);
uint32_t p4_golden_ai_rng_next(P4GoldenAiRng *rng);
void p4_golden_sfmt_certify(P4GoldenSfmt *rng);
void p4_golden_sfmt_seed(P4GoldenSfmt *rng, uint32_t seed);
void p4_golden_sfmt_recur(uint32_t out[4], const uint32_t a[4], const uint32_t b[4],
                          const uint32_t c[4], const uint32_t d[4]);
uint32_t p4_golden_sfmt_next(P4GoldenSfmt *rng);
/* Switch saturation profile, including NaN -> 0. */
int64_t p4_golden_rng2_word(uint32_t word, int64_t argument);
int64_t p4_golden_rng2(P4GoldenSfmt *rng, int64_t argument);
/* Assist presentation consumes two SFMT draws even if skill classification
 * overrides this argument. The first is discarded; the second can round to 3. */
uint16_t p4_golden_assist_argument(P4GoldenSfmt *rng);
#endif
