/* Layouts: btl_action_internal.h and btl_unit_internal.h, NTSC-U.
 * Only native committed owners call these adapters, under interest guards. */
#include "p4pc/battle_events.h"
#include "p4pc/battle_events_native.h"
#include "p4pc/gmem.h"

static P4BattleActor actor_snapshot(uint32_t action)
{
    P4BattleActor actor = {0};
    if (!action) return actor;
    const uint32_t unit = P4_G32(action + 0x30);
    actor.action_uid = (uint64_t)P4_G32(action) | (uint64_t)P4_G32(action + 4) << 32;
    actor.unit_uid = P4_G32(unit + 0xa8);
    actor.native_id = P4_G16(unit + 0xa4);
    actor.side = P4_G8(unit + 0xa2);
    actor.present = 1;
    return actor;
}

void p4_battle_emit_admission(uint32_t action, uint32_t queue_kind,
                              uint32_t native_action_count, uint32_t requested_state)
{
    const P4BattleAdmission event = {
        actor_snapshot(action), native_action_count, (uint16_t)requested_state,
        (uint8_t)queue_kind, 0
    };
    p4_battle_publish_admission(&event);
}

void p4_battle_emit_cost(uint32_t action, uint32_t kind, uint32_t requested_cost,
                         uint32_t hp_before, uint32_t sp_before, uint32_t item_id,
                         uint32_t item_count_before, uint32_t item_count_after)
{
    const uint32_t unit = P4_G32(action + 0x30);
    const uint32_t record = P4_G32(unit + 0xa64);
    const P4BattleCost event = {
        actor_snapshot(action), requested_cost, P4_G16(action + 0x6e), (uint16_t)item_id,
        (uint16_t)hp_before, P4_G16(record + 8),
        (uint16_t)sp_before, P4_G16(record + 10),
        (uint16_t)item_count_before, (uint16_t)item_count_after, (uint8_t)kind, {0}
    };
    p4_battle_publish_cost(&event);
}

void p4_battle_hit_begin(P4BattleHit *event, uint32_t work)
{
    const uint32_t source = P4_G32(work), target = P4_G32(work + 4);
    const uint32_t record = P4_G32(P4_G32(target + 0x30) + 0xa64);
    *event = (P4BattleHit){
        .source = actor_snapshot(source),
        .target = actor_snapshot(target),
        .requested_hp_delta = (int32_t)P4_G32(work + 8),
        .requested_sp_delta = (int32_t)P4_G32(work + 12),
        .requested_money_delta = (int32_t)P4_G32(work + 28),
        .added_status = P4_G32(work + 16),
        .removed_status = P4_G32(work + 20),
        .buff_changes = P4_G32(work + 24),
        .status_before = P4_G32(record + 12),
        .skill_id = source ? P4_G16(source + 0x6e) : 0,
        .action_kind = source ? P4_G16(source + 0x6c) : 0,
        .hp_before = P4_G16(record + 8),
        .sp_before = P4_G16(record + 10),
        .target_flags = P4_G16(work + 42),
        .result_flags = P4_G16(work + 38),
        .effect = P4_G16(work + 40)
    };
}

void p4_battle_hit_end(P4BattleHit *event, uint32_t target_action)
{
    const uint32_t record = P4_G32(P4_G32(target_action + 0x30) + 0xa64);
    event->hp_after = P4_G16(record + 8);
    event->sp_after = P4_G16(record + 10);
    event->status_after = P4_G32(record + 12);
    p4_battle_publish_hit(event);
}

void p4_battle_emit_command(uint32_t action, uint32_t action_kind)
{
    const P4BattleCommand event = {
        actor_snapshot(action), (uint16_t)action_kind, P4_G16(action + 0x6e),
        action_kind == 3 ? P4_G16(action + 0x70) : 0,
        P4_G16(action + 0x6a), P4_G16(action + 0x0c),
        action_kind == 5 || action_kind == 10, 0
    };
    p4_battle_publish_command(&event);
}

void p4_battle_emit_request(uint32_t stage, uint32_t encounter_id, uint32_t guest_object, uint32_t value)
{
    const P4BattleRequest event = {stage, encounter_id, guest_object, value};
    p4_battle_publish_request(&event);
}
