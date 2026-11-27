#include "p4pc/golden_battle_rows_native.h"
#include "p4pc/game_mode.h"
#include <limits.h>

int p4_golden_battle_rows_enabled(void)
{
    return p4_game_mode() == P4_GAME_MODE_GOLDEN;
}
int32_t p4_golden_motion_quotient(int32_t numerator, int32_t percent, float rate)
{
    /* Keep both divisions and the multiplication in binary32. The host's
     * out-of-range float-to-int cast is undefined, unlike FCVTZS. */
    float divisor = (float)percent / 100.0f;
    divisor = divisor * rate;
    float quotient = (float)numerator / divisor;
    if (quotient != quotient) return 0;
    if (quotient >= 2147483648.0f) return INT32_MAX;
    if (quotient <= -2147483648.0f) return INT32_MIN;
    return (int32_t)quotient;
}

static float live_frame_step = 1.0f;
static uint64_t frame_previous_ns;
static int frame_has_baseline;

void p4_golden_frame_step_reset(void)
{
    live_frame_step = 1.0f;
    frame_previous_ns = 0;
    frame_has_baseline = 0;
}

void p4_golden_frame_step_publish(uint64_t scheduler_ns)
{
    if (frame_has_baseline) {
        /* Steam's counter arithmetic: narrow 60*delta before multiplying by
         * the binary64 reciprocal frequency, then publish/clamp binary32. */
        float scaled = (float)(int64_t)((scheduler_ns - frame_previous_ns) * UINT64_C(60));
        float step = (float)((double)scaled * (1.0 / 1000000000.0));
        if (step < 0.01f) step = 0.01f;
        if (step > 3.0f) step = 3.0f;
        live_frame_step = step;
    }
    frame_previous_ns = scheduler_ns;
    frame_has_baseline = 1;
}

float p4_golden_live_frame_step(void)
{
    return live_frame_step;
}

typedef struct P4GoldenCueCacheRow {
    int16_t cue, variant;
    float age;
} P4GoldenCueCacheRow;
static P4GoldenCueCacheRow cue_cache[16];

void p4_golden_cue_cache_reset(void)
{
    for (unsigned i = 0; i < 16; ++i) {
        cue_cache[i].cue = cue_cache[i].variant = -1;
        cue_cache[i].age = 3.0f;
    }
}

void p4_golden_cue_cache_advance(void)
{
    float step = 0.5f * p4_golden_live_frame_step();
    for (unsigned i = 0; i < 16; ++i) {
        if (cue_cache[i].age >= 0.0f) {
            cue_cache[i].age += step;
            if (cue_cache[i].age >= 3.0f) {
                cue_cache[i].cue = cue_cache[i].variant = -1;
                cue_cache[i].age = -1.0f;
            }
        }
    }
}

int p4_golden_cue_cache_match(uint32_t channel, int32_t cue, int32_t variant)
{
    return cue_cache[channel].cue == cue && cue_cache[channel].variant == variant;
}

void p4_golden_cue_cache_store(uint32_t channel, int32_t cue, int32_t variant)
{
    cue_cache[channel].cue = (int16_t)cue;
    cue_cache[channel].variant = (int16_t)variant;
    cue_cache[channel].age = 0.0f;
}
