#ifndef P4PC_BATTLE_EVENTS_NATIVE_H
#define P4PC_BATTLE_EVENTS_NATIVE_H
#include <stdint.h>
#include "p4pc/battle_events.h"

/* Engine-body adapters only, not a script API. Call under the appropriate
 * observer-interest bit after native admission, passing its actual actor.
 * The adapter copies identities into the pointer-free public event. */
void p4_battle_emit_admission(uint32_t action, uint32_t queue_kind,
                              uint32_t native_action_count, uint32_t requested_state);
void p4_battle_emit_cost(uint32_t action, uint32_t kind, uint32_t requested_cost,
                         uint32_t hp_before, uint32_t sp_before, uint32_t item_id,
                         uint32_t item_count_before, uint32_t item_count_after);
void p4_battle_hit_begin(P4BattleHit *event, uint32_t work);
void p4_battle_hit_end(P4BattleHit *event, uint32_t target_action);
void p4_battle_emit_command(uint32_t action, uint32_t action_kind);
void p4_battle_emit_request(uint32_t stage, uint32_t encounter_id, uint32_t guest_object, uint32_t value);
#endif
