/* Declarative preparation uses real native progression/stock/record owners.
 * No combat formula, save serializer, RNG or acquisition history is cloned. */
#include "p4pc/battle_prepare.h"
#include "p4pc/battle_sim.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/gmem.h"
#include "p4pc/platform.h"
#include "p4pc/content_digest.h"
#include <inttypes.h>
#include <limits.h>
#include <string.h>

extern void *datUnitGetRecord(int16_t id);
extern void datCalcInitUnitById(int16_t id);
extern int32_t datCalcGetLevel(void *unit);
extern uint16_t datCalcGetHp(void *unit);
extern uint16_t datCalcGetSp(void *unit);
extern uint32_t datCalcGetBadStatus(void *unit);
extern uint32_t datCalcSetBadStatus(void *unit, uint32_t mask);
extern uint32_t datCalcClearBadStatus(void *unit, uint32_t mask);
extern void datCalcClearAllBuffs(void *unit);
extern uint32_t datCalcIsDead(void *unit, int32_t delta);
extern uint16_t datCalcMaxHpById(int16_t id);
extern uint16_t datCalcMaxSpById(int16_t id);
extern void datSetUnitHp(int16_t id, int16_t value);
extern void datSetUnitSp(int16_t id, int16_t value);
extern void datSetUnitAiTactic(int16_t id, int16_t tactic);
extern void datSetHeroLevel(int16_t id, uint8_t level);
extern void datSetHeroExp(int16_t id, uint32_t experience);
extern uint8_t datLevelFromExp(int32_t experience);
extern int16_t datGetPartyId(int32_t slot);
extern void datSetPartyId(int32_t slot, int16_t id);
extern int16_t datBuildMemberIdListExcluding5(void *ids);
extern void *datPersonaGetRecord(uint16_t id);
extern void *datPersonaRosterSlot(int16_t slot);
extern int32_t datPersonaRosterCapacity(void);
extern void datPersonaSetRosterMode(int32_t mode);
extern void datPersonaClearRoster(void);
extern int32_t datPersonaInsertRosterRecord(void *record);
extern int32_t datPersonaSetLeadSlot(int32_t slot);
extern void datPersonaInitRecord(void *record, uint16_t id);
extern void datPersonaPreviewLevelUp(void *record, void *delta, int32_t added_experience);
extern void datPersonaApplyLevelUp(void *record, void *delta);
extern uint32_t datPersonaGetExpForLevel(void *record, uint16_t level);
extern void datPersonaAddExp(void *record, int32_t added_experience);
extern int32_t datPersonaAddSkill(void *record, uint16_t skill);
extern int32_t datPersonaReplaceSkill(void *record, int16_t old_skill, uint16_t new_skill);
extern int16_t datGetEquippedItemByUnit(int16_t id, int16_t slot);
extern void datAssignEquippedItem(int16_t id, int16_t slot, int16_t item);
extern int32_t datItemCount(int16_t item);
extern void datItemSetCount(int16_t item, int32_t count);
extern uint32_t datEntryGetKind(int16_t item);
extern int32_t datEntryHasFlag(int16_t item, int16_t actor);
extern int32_t datIdBand(int16_t item);

static uint16_t half(const uint8_t *p) { return (uint16_t)(p[0] | (uint16_t)p[1] << 8); }
static void put_half(uint8_t *p, uint16_t value) { p[0] = (uint8_t)value; p[1] = (uint8_t)(value >> 8); }
static void put_word(uint8_t *p, uint32_t value)
{
    for (unsigned i = 0; i < 4; i++) p[i] = (uint8_t)(value >> (i * 8));
}
static int in_guest(uint32_t base, uint32_t bytes)
{
    return base >= 0x100000 && base <= 0x2000000 && bytes <= 0x2000000 - base;
}
static int record_assertions(const uint8_t *request, const uint8_t *record)
{
    const uint32_t mask = p4_prepare_word(request + 88);
    return (!(mask & 1) || (record[4] == p4_prepare_word(request + 20) && p4_prepare_word(record + 8) == p4_prepare_word(request + 24))) &&
           (!(mask & 2) || !memcmp(record + 12, request + 48, 16)) &&
           (!(mask & 4) || !memcmp(record + 28, request + 64, 5)) &&
           (!(mask & 8) || !memcmp(record + 33, request + 69, 10));
}
static const char *grow(const P4BattlePreparation *view, const uint8_t *request, uint8_t *record, uint8_t *delta)
{
    const uint32_t level = p4_prepare_word(request + 20), experience = p4_prepare_word(request + 24);
    const uint32_t first = p4_prepare_word(request + 80), count = p4_prepare_word(request + 84);
    uint32_t decision_index = 0;
    if (record[4] > level || p4_prepare_word(record + 8) > experience) return "native_growth_cannot_reduce_level_or_exp";
    while (record[4] < level) {
        const uint32_t next_exp = datPersonaGetExpForLevel(record, (uint16_t)(record[4] + 1));
        const uint32_t current_exp = p4_prepare_word(record + 8);
        if (next_exp < current_exp || next_exp > experience || next_exp - current_exp > INT32_MAX)
            return "native_growth_level_exp_inconsistent";
        const int32_t added_exp = (int32_t)(next_exp - current_exp);
        datPersonaPreviewLevelUp(record, delta, added_exp);
        if (delta[0] != 1) return "native_growth_did_not_reach_exact_next_level";
        uint16_t learned[32];
        for (unsigned i = 0; i < 32; i++) learned[i] = half(delta + 2 + i * 2);
        /* Apply the native level/stats/removals exactly once. The actual native
         * learn/replace functions then perform the declared UI choices. */
        memset(delta + 2, 0, 64);
        datPersonaApplyLevelUp(record, delta);
        datPersonaAddExp(record, added_exp);
        for (unsigned i = 0; i < 32; i++) if (learned[i]) {
            if (decision_index == count) return "native_growth_missing_skill_decision";
            const uint8_t *decision = view->decisions + (first + decision_index++) * P4_PREPARE_DECISION_BYTES;
            const uint32_t replacement = p4_prepare_word(decision + 16), removed = p4_prepare_word(decision + 20);
            if (p4_prepare_word(decision + 8) != record[4] || p4_prepare_word(decision + 12) != learned[i])
                return "native_growth_skill_decision_does_not_match_learn_event";
            if (replacement == UINT32_MAX) {
                if (!datPersonaAddSkill(record, learned[i])) return "native_growth_requires_explicit_skill_replacement";
            } else {
                if (half(record + 12 + replacement * 2) != removed) return "native_growth_stale_replacement_slot";
                if (!removed) {
                    unsigned empty = 0;
                    while (empty < 8 && half(record + 12 + empty * 2)) empty++;
                    if (empty != replacement || !datPersonaAddSkill(record, learned[i])) return "native_growth_empty_replacement_not_native_auto_add";
                } else if (datPersonaReplaceSkill(record, (int16_t)removed, learned[i]) != (int32_t)replacement)
                    return "native_growth_native_replacement_failed";
            }
        }
    }
    if (decision_index != count) return "native_growth_extra_skill_decisions";
    if (level < 99 && datPersonaGetExpForLevel(record, (uint16_t)(level + 1)) <= experience)
        return "native_growth_final_exp_exceeds_declared_level";
    const uint32_t remaining = experience - p4_prepare_word(record + 8);
    if (remaining > INT32_MAX) return "native_growth_exp_increment_outside_native_range";
    datPersonaAddExp(record, (int32_t)remaining);
    return record_assertions(request, record) ? NULL : "native_growth_final_record_assertion";
}

const char *p4_battle_prepare_apply(const P4BattlePreparation *view)
{
    const uint8_t *header = view->header;
    const int synthetic = p4_prepare_word(header + 36) == 2;
    uint32_t metadata, metadata_rows, names, name_rows;
    if (!p4_game_table_extent_rows(0x7644c4, 14, &metadata, &metadata_rows) ||
        !p4_game_table_extent_rows(0x764538, 19, &names, &name_rows)) return "preparation_requires_observed_original_persona_and_skill_names";
    const uint32_t item_base = P4_G32(0x7642a0);
    if (item_base < 16 || !in_guest(item_base - 16, 16 + 1536 * 68) || P4_G32(item_base - 16) != 1536)
        return "preparation_requires_original_item_rows_and_native_stock_capacity";
    const uint32_t roster_mode = p4_prepare_word(header + 76), selected = p4_prepare_word(header + 80);
    const int32_t old_capacity = datPersonaRosterCapacity();
    const uint32_t capacity = roster_mode == UINT32_MAX ? (uint32_t)old_capacity : 6 + roster_mode * 2;
    const uint32_t lead = selected == UINT32_MAX ? P4_G16(0x797f88) : selected;
    if (!synthetic && capacity > (uint32_t)old_capacity) return "source_attested_cannot_grant_roster_capacity";
    if (lead >= capacity) return "prepared_selected_roster_slot_outside_capacity";
    const uint32_t scratch = p4_gport_alloc(view->persona_count * 48 + 0x88 + 16, 16);
    if (!scratch) return "preparation_native_scratch_allocation";
    uint8_t *records = P4_G2H(scratch), *delta = records + view->persona_count * 48;
    const uint32_t members = scratch + view->persona_count * 48 + 0x88;
    const int16_t member_count = datBuildMemberIdListExcluding5(P4_G2H(members));
    if (member_count < 1 || member_count > 8) return "native_unlocked_member_list_invalid";
    uint8_t stock[1536];
    memcpy(stock, P4_G2H(0x79757a), sizeof stock);
    struct GearChange { int16_t actor, slot, old_item, new_item; int done; } gear[12];
    unsigned gear_count = 0;
    for (uint32_t i = 0; i < view->actor_count; i++) {
        const uint8_t *actor = view->actors + i * P4_PREPARE_ACTOR_BYTES;
        const int16_t id = (int16_t)p4_prepare_word(actor);
        int unlocked = 0;
        for (int16_t j = 0; j < member_count; j++) if ((int16_t)P4_G16(members + j * 2) == id) unlocked = 1;
        if (!synthetic && !unlocked) return "source_attested_actor_not_unlocked";
        for (int16_t slot = 0; slot < 3; slot++) {
            const uint32_t desired = p4_prepare_word(actor + 4 + slot * 4);
            const int16_t old = datGetEquippedItemByUnit(id, slot);
            if (desired == UINT32_MAX || desired == (uint16_t)old) continue;
            if (!desired || desired >= 0x300 || datIdBand((int16_t)desired) != slot || !datEntryHasFlag((int16_t)desired, id))
                return "prepared_equipment_not_native_actor_eligible";
            if (old < 0 || old >= 1536) return "source_equipment_outside_native_stock";
            gear[gear_count++] = (struct GearChange){id, slot, old, (int16_t)desired, 0};
        }
    }
    /* Choose only stock-feasible native swaps. This does not invent an
     * unequip-to-zero operation to resolve an impossible empty-stock cycle. */
    unsigned gear_order[12], planned = 0;
    while (planned < gear_count) {
        const unsigned before = planned;
        for (unsigned i = 0; i < gear_count; i++) {
            struct GearChange *change = &gear[i];
            if (change->done || (!stock[change->new_item] && !synthetic)) continue;
            if (!stock[change->new_item]) stock[change->new_item] = 1;
            unsigned quantity = stock[change->new_item] - 1;
            stock[change->new_item] = (uint8_t)(quantity > 99 ? 99 : quantity);
            if (change->old_item) {
                const unsigned returned = (stock[change->old_item] + 1u) & 255u;
                stock[change->old_item] = (uint8_t)(returned > 99 ? 99 : returned);
            }
            change->done = 1; gear_order[planned++] = i;
        }
        if (before == planned) return "source_attested_equipment_swaps_have_no_native_stock_feasible_order";
    }
    for (uint32_t i = 0; i < view->item_count; i++) {
        const uint8_t *item = view->items + i * P4_PREPARE_ITEM_BYTES;
        const int16_t id = (int16_t)p4_prepare_word(item);
        if (datEntryGetKind(id) != 3) return "declared_inventory_item_not_native_battle_consumable";
        if (!synthetic && p4_prepare_word(item + 4) > (uint32_t)datItemCount(id)) return "source_attested_cannot_grant_item_stock";
    }
    uint32_t hero_slots = 0, hero_count = 0;
    for (uint32_t i = 0; i < view->persona_count; i++) {
        const uint8_t *request = view->personas + i * P4_PREPARE_PERSONA_BYTES;
        uint8_t *record = records + i * 48;
        const uint32_t owner = p4_prepare_word(request), slot = p4_prepare_word(request + 4);
        const uint32_t mode = p4_prepare_word(request + 12), origin = p4_prepare_word(request + 28);
        if (owner == 1) {
            if (slot >= capacity) return "prepared_hero_persona_outside_roster_capacity";
            hero_slots |= 1u << slot; hero_count++;
        }
        if (!origin) {
            const uint32_t source_slot = p4_prepare_word(request + 16);
            if (owner == 1 && source_slot >= (uint32_t)old_capacity) return "source_persona_slot_outside_original_capacity";
            void *source = owner == 1 ? datPersonaRosterSlot((int16_t)source_slot) : datPersonaGetRecord((uint16_t)owner);
            if (!source) return "source_persona_record_missing";
            memcpy(record, source, 48);
            if (p4_prepare_word(request + 8) != UINT32_MAX && half(record + 2) != p4_prepare_word(request + 8))
                return "source_persona_id_assertion";
        } else {
            const uint32_t id = p4_prepare_word(request + 8);
            if (id >= metadata_rows || !P4_G8(metadata + id * 14 + 3) || P4_G8(metadata + id * 14 + 3) > 99)
                return "prepared_persona_not_populated_native_metadata";
            memset(record, 0, 48);
            datPersonaInitRecord(record, (uint16_t)id);
            put_half(record, 1);
        }
        if (!half(record + 2) || half(record + 2) >= metadata_rows || !record[4] || record[4] > 99)
            return "prepared_persona_record_not_native_valid";
        const uint32_t mask = p4_prepare_word(request + 88);
        if (mask & 16) memcpy(record + 12, request + 32, 16);
        if (mode == 1) {
            const uint32_t level = p4_prepare_word(request + 20), experience = p4_prepare_word(request + 24);
            if (experience < datPersonaGetExpForLevel(record, (uint16_t)level) ||
                (level < 99 && experience >= datPersonaGetExpForLevel(record, (uint16_t)(level + 1))))
                return "synthetic_persona_level_exp_inconsistent";
            record[4] = (uint8_t)level; put_word(record + 8, experience);
            memcpy(record + 12, request + 48, 16); memcpy(record + 28, request + 64, 5);
        } else if (mode == 2) {
            if (!synthetic && (record[4] != p4_prepare_word(request + 20) || p4_prepare_word(record + 8) != p4_prepare_word(request + 24)))
                return "source_attested_cannot_grant_persona_growth";
            const char *reason = grow(view, request, record, delta);
            if (reason) return reason;
        }
        if (!record_assertions(request, record)) return "prepared_persona_record_assertion";
        for (unsigned j = 0; j < 8; j++) {
            const uint16_t skill = half(record + 12 + j * 2);
            if (skill && (skill >= name_rows || !P4_G8(names + skill * 19))) return "prepared_skill_not_in_populated_native_name_catalog";
        }
    }
    if (hero_count) {
        if (hero_slots != (1u << hero_count) - 1) return "declared_hero_roster_must_be_complete_contiguous_slots";
        if (!(hero_slots & (1u << lead))) return "prepared_selected_roster_slot_missing";
    } else if (lead >= (uint32_t)old_capacity || !datPersonaRosterSlot((int16_t)lead)) return "preserved_selected_roster_slot_missing";
    const uint32_t hero_mode = p4_prepare_word(header + 84), hero_level = p4_prepare_word(header + 68), hero_exp = p4_prepare_word(header + 72);
    if (hero_mode == 1 && datLevelFromExp((int32_t)hero_exp) != hero_level) return "synthetic_hero_level_exp_inconsistent";
    if (hero_mode == 2 && (datCalcGetLevel(datUnitGetRecord(1)) != (int32_t)hero_level || P4_G32(0x797400) != hero_exp))
        return "source_hero_level_exp_assertion";
    /* All staged records and stock decisions are validated before publication. */
    if (roster_mode != UINT32_MAX) datPersonaSetRosterMode((int32_t)roster_mode);
    if (hero_count) {
        datPersonaClearRoster();
        for (uint32_t slot = 0; slot < hero_count; slot++) {
            for (uint32_t i = 0; i < view->persona_count; i++) {
                const uint8_t *request = view->personas + i * P4_PREPARE_PERSONA_BYTES;
                if (p4_prepare_word(request) != 1 || p4_prepare_word(request + 4) != slot) continue;
                uint8_t *record = records + i * 48;
                if (!datPersonaInsertRosterRecord(record)) return "native_roster_insert_failed";
                void *actual = datPersonaRosterSlot((int16_t)slot);
                if (!actual) return "native_inserted_roster_slot_missing";
                /* Insert performs real cache/activation handling; preserve mode
                 * retains the actual complete source record, including flags. */
                if (!p4_prepare_word(request + 12)) memcpy(actual, record, 48);
            }
        }
    }
    for (uint32_t i = 0; i < view->persona_count; i++) {
        const uint32_t owner = p4_prepare_word(view->personas + i * P4_PREPARE_PERSONA_BYTES);
        if (owner != 1) memcpy(datPersonaGetRecord((uint16_t)owner), records + i * 48, 48);
    }
    if (!datPersonaSetLeadSlot((int32_t)lead)) return "native_lead_slot_selection_failed";
    if (hero_mode == 1) { datSetHeroExp(1, hero_exp); datSetHeroLevel(1, (uint8_t)hero_level); }
    for (unsigned i = 0; i < gear_count; i++) {
        const struct GearChange *change = &gear[gear_order[i]];
        int32_t available = datItemCount(change->new_item);
        if (!available) {
            if (!synthetic) return "native_source_stock_plan_diverged";
            datItemSetCount(change->new_item, 1); /* Explicit synthetic grant only. */
            available = 1;
        }
        datItemSetCount(change->new_item, available - 1);
        if (change->old_item) datItemSetCount(change->old_item, datItemCount(change->old_item) + 1);
        datAssignEquippedItem(change->actor, change->slot, change->new_item);
    }
    if (p4_prepare_word(header + 112)) {
        for (int16_t id = 0; id < 1536; id++) if (datEntryGetKind(id) == 3 && datItemCount(id)) datItemSetCount(id, 0);
        for (uint32_t i = 0; i < view->item_count; i++) {
            const uint8_t *item = view->items + i * P4_PREPARE_ITEM_BYTES;
            datItemSetCount((int16_t)p4_prepare_word(item), (int32_t)p4_prepare_word(item + 4));
        }
    }
    for (int32_t i = 0; i < 3; i++) datSetPartyId(i, i + 1 < (int32_t)view->actor_count ? (int16_t)p4_prepare_word(view->actors + (i + 1) * P4_PREPARE_ACTOR_BYTES) : 0);
    for (uint32_t i = 0; i < view->actor_count; i++) {
        const uint8_t *actor = view->actors + i * P4_PREPARE_ACTOR_BYTES;
        const int16_t id = (int16_t)p4_prepare_word(actor);
        void *unit = datUnitGetRecord(id);
        if (synthetic && P4_G16(P4_H2G(unit) + 2) != (uint16_t)id) datCalcInitUnitById(id);
        if (p4_prepare_word(actor + 28) != UINT32_MAX) datSetUnitAiTactic(id, (int16_t)p4_prepare_word(actor + 28));
        if (p4_prepare_word(header + 44)) { datCalcClearBadStatus(unit, 0xfff7ffffu); datCalcClearAllBuffs(unit); }
        if (p4_prepare_word(actor + 16)) {
            const uint16_t hp = (uint16_t)p4_prepare_word(actor + 20);
            datSetUnitHp(id, (int16_t)hp); datSetUnitSp(id, (int16_t)p4_prepare_word(actor + 24));
            if (hp) datCalcClearBadStatus(unit, 0x80000); else datCalcSetBadStatus(unit, 0x80000);
        } else if (p4_prepare_word(header + 40) && !datCalcIsDead(unit, 0)) {
            datSetUnitHp(id, (int16_t)datCalcMaxHpById(id)); datSetUnitSp(id, (int16_t)datCalcMaxSpById(id));
        }
    }
    return NULL;
}

static void persona_report(FILE *stream, const uint8_t *record)
{
    fprintf(stream, "{\"id\":%u,\"flags\":%u,\"level\":%u,\"exp\":%u,\"skills\":[", half(record + 2), half(record), record[4], p4_prepare_word(record + 8));
    for (unsigned i = 0; i < 8; i++) fprintf(stream, "%s%u", i ? "," : "", half(record + 12 + i * 2));
    fputs("],\"stats\":[", stream);
    for (unsigned i = 0; i < 5; i++) fprintf(stream, "%s%u", i ? "," : "", record[28 + i]);
    fputs("],\"bonus_a\":[", stream);
    for (unsigned i = 0; i < 5; i++) fprintf(stream, "%s%d", i ? "," : "", (int8_t)record[33 + i]);
    fputs("],\"bonus_b\":[", stream);
    for (unsigned i = 0; i < 5; i++) fprintf(stream, "%s%d", i ? "," : "", (int8_t)record[38 + i]);
    fputs("]}", stream);
}
void p4_battle_prepare_report(FILE *stream, const P4BattlePreparation *view)
{
    char stock_hash[65]; p4_content_digest(P4_G2H(0x79757a), 1536, stock_hash);
    fprintf(stream, "\"native_prepared_state\":{\"hero_level\":%d,\"hero_exp\":%u,\"roster_capacity\":%d,\"selected_slot\":%u,\"stock_sha256\":\"%s\",\"actors\":[",
            datCalcGetLevel(datUnitGetRecord(1)), P4_G32(0x797400), datPersonaRosterCapacity(), P4_G16(0x797f88), stock_hash);
    for (uint32_t i = 0; i < view->actor_count; i++) {
        const int16_t id = (int16_t)p4_prepare_word(view->actors + i * P4_PREPARE_ACTOR_BYTES);
        void *unit = datUnitGetRecord(id);
        fprintf(stream, "%s{\"id\":%d,\"level\":%d,\"hp\":%u,\"sp\":%u,\"max_hp\":%u,\"max_sp\":%u,\"status\":%u,\"equipment\":[%d,%d,%d]}", i ? "," : "", id,
                datCalcGetLevel(unit), datCalcGetHp(unit), datCalcGetSp(unit), datCalcMaxHpById(id), datCalcMaxSpById(id), datCalcGetBadStatus(unit),
                datGetEquippedItemByUnit(id, 0), datGetEquippedItemByUnit(id, 1), datGetEquippedItemByUnit(id, 2));
    }
    fputs("],\"hero_personas\":[", stream);
    int emitted = 0;
    const int32_t capacity = datPersonaRosterCapacity();
    for (int16_t slot = 0; slot < capacity; slot++) {
        const uint8_t *record = datPersonaRosterSlot(slot);
        if (!record) continue;
        fprintf(stream, "%s{\"slot\":%d,\"record\":", emitted++ ? "," : "", slot); persona_report(stream, record); fputc('}', stream);
    }
    fputs("],\"companion_personas\":[", stream); emitted = 0;
    for (uint16_t owner = 2; owner <= 10; owner++) {
        const uint8_t *record = datPersonaGetRecord(owner);
        if (!record || !half(record + 2)) continue;
        fprintf(stream, "%s{\"owner\":%u,\"record\":", emitted++ ? "," : "", owner); persona_report(stream, record); fputc('}', stream);
    }
    fputs("],\"declared_inventory\":[", stream);
    for (uint32_t i = 0; i < view->item_count; i++) {
        const int16_t item = (int16_t)p4_prepare_word(view->items + i * P4_PREPARE_ITEM_BYTES);
        fprintf(stream, "%s{\"id\":%d,\"count\":%d}", i ? "," : "", item, datItemCount(item));
    }
    fputs("]}", stream);
}
