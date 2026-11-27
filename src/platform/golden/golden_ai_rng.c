/* Independent scalar implementation of the clean-room ai-commands contract.
 * SFMT-19937 algorithm: Mutsuo Saito and Makoto Matsumoto,
 * https://www.math.sci.hiroshima-u.ac.jp/m-mat/MT/SFMT/ .
 * No reference implementation source is incorporated. */
#include "golden_ai_rng.h"
#include <assert.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#pragma STDC FENV_ACCESS ON
#pragma STDC FP_CONTRACT OFF

uint32_t p4_golden_ai_rng_next(P4GoldenAiRng *rng)
{
    assert(rng && rng->front < 31 && rng->rear < 31);
    uint32_t sum = rng->words[rng->front] + rng->words[rng->rear];
    rng->words[rng->front] = sum;
    if (++rng->front == 31) rng->front = 0;
    if (++rng->rear == 31) rng->rear = 0;
    return sum >> 1;
}
void p4_golden_ai_rng_seed(P4GoldenAiRng *rng, uint32_t seed)
{
    rng->words[0] = seed;
    for (unsigned i = 1; i < 31; ++i)
        rng->words[i] = rng->words[i - 1] * UINT32_C(0x41c64e6d) + UINT32_C(0x3039);
    rng->front = 3;
    rng->rear = 0;
    for (unsigned i = 0; i < 310; ++i) (void)p4_golden_ai_rng_next(rng);
}
void p4_golden_sfmt_certify(P4GoldenSfmt *rng)
{
    uint32_t parity = (rng->words[0] & 1u) ^ (rng->words[3] & UINT32_C(0x13c9e684));
    for (unsigned shift = 16; shift; shift >>= 1) parity ^= parity >> shift;
    if (!(parity & 1u)) rng->words[0] ^= 1u;
}
void p4_golden_sfmt_seed(P4GoldenSfmt *rng, uint32_t seed)
{
    rng->words[0] = seed;
    for (unsigned i = 1; i < 624; ++i) {
        uint32_t previous = rng->words[i - 1];
        rng->words[i] = UINT32_C(0x6c078965) * (previous ^ (previous >> 30)) + i;
    }
    rng->index = 624;
    rng->anomalies = 0;
    p4_golden_sfmt_certify(rng);
}
void p4_golden_sfmt_recur(uint32_t out[4], const uint32_t a[4], const uint32_t b[4],
                          const uint32_t c[4], const uint32_t d[4])
{
    static const uint32_t masks[4] = {0x001fffef, 0x001ecb7f, 0x001affff, 0x001ffff6};
    /* Temporaries make aliasing out==a safe without a 128-bit host type. */
    uint32_t result[4];
    for (unsigned lane = 0; lane < 4; ++lane) {
        uint32_t left = a[lane] << 8;
        uint32_t right = c[lane] >> 8;
        if (lane) left |= a[lane - 1] >> 24;
        if (lane != 3) right |= c[lane + 1] << 24;
        result[lane] = a[lane] ^ left ^ right ^ (d[lane] << 18) ^ ((b[lane] >> 11) & masks[lane]);
    }
    memcpy(out, result, sizeof result);
}
uint32_t p4_golden_sfmt_next(P4GoldenSfmt *rng)
{
    assert(rng && rng->index >= 0);
    if (rng->index >= 624) {
        const uint32_t *c = rng->words + 616, *d = rng->words + 620;
        for (unsigned i = 0; i < 156; ++i) {
            uint32_t *a = rng->words + 4 * i;
            const uint32_t *b = rng->words + 4 * ((i + 122) % 156);
            p4_golden_sfmt_recur(a, a, b, c, d);
            c = d;
            d = a;
        }
        rng->index = 0;
    }
    return rng->words[rng->index++];
}
static int64_t rng2_word(uint32_t word, int64_t argument, uint64_t *anomalies)
{
    /* Unsigned addition defines wrap at INT64_MAX without signed overflow.
     * memcpy preserves the signed interpretation without implementation-defined casts. */
    uint64_t bits = (uint64_t)argument + UINT64_C(1);
    int64_t bound;
    memcpy(&bound, &bits, sizeof bound);
    volatile float random_float = (float)word;
    volatile float bound_float = (float)bound;
    volatile float fraction = random_float * 0x1p-32f;
    volatile float product = fraction * bound_float;
    /* Switch FCVTZS saturation; Steam's invalid CVTTSS2SI is not portable. */
    if (isnan(product) || product >= 0x1p63f || product < -0x1p63f) {
        if (anomalies) ++*anomalies;
        return isnan(product) ? 0 : product > 0 ? INT64_MAX : INT64_MIN;
    }
    return (int64_t)product;
}
int64_t p4_golden_rng2_word(uint32_t word, int64_t argument)
{
    return rng2_word(word, argument, NULL);
}
int64_t p4_golden_rng2(P4GoldenSfmt *rng, int64_t argument)
{
    return rng2_word(p4_golden_sfmt_next(rng), argument, &rng->anomalies);
}
uint16_t p4_golden_assist_argument(P4GoldenSfmt *rng)
{
    (void)p4_golden_sfmt_next(rng);
    volatile float word = (float)p4_golden_sfmt_next(rng);
    volatile float fraction = word * 0x1p-32f;
    volatile float scaled = fraction * 3.0f;
    return (uint16_t)(uint32_t)scaled;
}
