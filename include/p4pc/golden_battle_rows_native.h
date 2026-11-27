/* Golden-only scalar seams for the original guest battle-row consumers. */
#ifndef P4PC_GOLDEN_BATTLE_ROWS_NATIVE_H
#define P4PC_GOLDEN_BATTLE_ROWS_NATIVE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
int p4_golden_battle_rows_enabled(void);
uint32_t p4_golden_skill_flags(uint32_t port_skill);
uint32_t p4_golden_skill_selector(uint32_t port_skill);
/* Nonzero n chooses logical unit-array element n-1, unchecked against the PS2
 * getter's smaller domain. Scalar guest address; never a native host pointer. */
uint32_t p4_golden_skill_unit_guest(uint32_t selector);
uint32_t p4_golden_effect_flags(uint32_t port_skill);
uint32_t p4_golden_effect_interval(uint32_t port_skill);
int32_t p4_golden_effect_timing(uint32_t port_skill);
/* Result markers use action address+UID; delay lifetime follows packet allocation
 * and release (its UID is assigned later by enqueue). No guest layout changes. */
void p4_golden_result_clear(uint32_t action);
void p4_golden_result_intercept(uint32_t action);
int p4_golden_result_intercepted(uint32_t action);
void p4_golden_result_copy(uint32_t destination, uint32_t source);
void p4_golden_packet_delay_set(uint32_t packet, uint32_t frames);
void p4_golden_packet_delay_clear(uint32_t packet);
/* -1: no Golden delay; 1: consumed a waiting frame; 0: ready to dispatch. */
int p4_golden_packet_delay_step(uint32_t packet);
/* M8f owns the single retained49-entry BED400 common generation. Return its
 * logical48 formation-work guest handle only after all49 handles are ready;
 * return0 while absent/not ready. Never return raw BED bytes or a PS2 slot.
 * The provider owns resource clear/reload lifetime; R2 retains no asset copy. */
typedef struct P4GoldenCommonResourceOwner {
    uint32_t (*interception)(void *context, uint32_t battle);
    void *context;
} P4GoldenCommonResourceOwner;
int p4_golden_common_resource_owner_install(const P4GoldenCommonResourceOwner *owner);
int p4_golden_common_resource_owner_remove(const P4GoldenCommonResourceOwner *owner);
/* Fails closed with the named M8f dependency if no ready owner is installed. */
uint32_t p4_golden_common_interception_resource(uint32_t battle);
/* Live Golden simulation-frame step supplied by the shared host timing seam. */
float p4_golden_live_frame_step(void);
/* Application lifetime, not battle/packet lifetime. The first publication
 * captures the real scheduler baseline; later publications precede callbacks. */
void p4_golden_frame_step_reset(void);
void p4_golden_frame_step_publish(uint64_t scheduler_ns);
void p4_golden_cue_cache_reset(void);
void p4_golden_cue_cache_advance(void);
int p4_golden_cue_cache_match(uint32_t channel, int32_t cue, int32_t variant);
void p4_golden_cue_cache_store(uint32_t channel, int32_t cue, int32_t variant);
/* Native M8 consumers receive the complete original row, never a projection.
 * These bounded views return NULL for an out-of-domain active row. */
const uint8_t *p4_golden_skill_raw(uint32_t port_skill);
const uint8_t *p4_golden_effect_raw(uint32_t port_skill);
/* AArch64 FCVTZS after the specified binary32 percentage/rate divisions. */
int32_t p4_golden_motion_quotient(int32_t numerator, int32_t percent, float rate);
#ifdef __cplusplus
}
#endif
#endif
