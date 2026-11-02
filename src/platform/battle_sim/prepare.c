#include "p4pc/battle_prepare.h"
#include "p4pc/battle_sim.h"
#include <limits.h>
#include <string.h>

static int fail(const char **reason, const char *message)
{
    if (reason) *reason = message;
    return 0;
}
static int zero(const uint8_t *p, size_t count)
{
    for (size_t i = 0; i < count; i++) if (p[i]) return 0;
    return 1;
}
static uint16_t half(const uint8_t *p)
{
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}
static int skills_valid(const uint8_t *p)
{
    for (unsigned i = 0; i < 8; i++) {
        const uint16_t skill = half(p + i * 2);
        if (skill > P4_PREPARE_MAX_SKILL) return 0;
        if (skill) for (unsigned j = 0; j < i; j++) if (half(p + j * 2) == skill) return 0;
    }
    return 1;
}
static int selected_actor(uint32_t id)
{
    /* The native party selection excludes navigator5 (00353CE0). */
    return id >= 1 && id <= 8 && id != 5;
}

int p4_battle_prepare_decode(const uint8_t *bytes, size_t size, P4BattlePreparation *view, const char **reason)
{
#define WORD(offset) p4_prepare_word(bytes + (offset))
#define REJECT(message) return fail(reason, message)
    if (reason) *reason = NULL;
    if (!view || !bytes || size < P4_PREPARE_HEADER_BYTES || size > P4_PREPARE_MAX_BYTES)
        REJECT("preparation_request_size");
    if (memcmp(bytes, "P4BPRE01", 8) || WORD(8) != P4_PREPARE_HEADER_BYTES || WORD(12) || !zero(bytes + 116, 12))
        REJECT("preparation_header_or_reserved_fields");
    P4BattlePreparation result = {0};
    result.source_bytes = WORD(16); result.actor_count = WORD(20); result.persona_count = WORD(24);
    result.item_count = WORD(28); result.decision_count = WORD(32);
    if (!result.actor_count || result.actor_count > 4 || result.persona_count > 21 ||
        result.item_count > 1536 || result.decision_count > 4096)
        REJECT("preparation_record_count");
    const uint64_t expected = P4_PREPARE_HEADER_BYTES + (uint64_t)result.source_bytes +
        (uint64_t)result.actor_count * P4_PREPARE_ACTOR_BYTES +
        (uint64_t)result.persona_count * P4_PREPARE_PERSONA_BYTES +
        (uint64_t)result.item_count * P4_PREPARE_ITEM_BYTES +
        (uint64_t)result.decision_count * P4_PREPARE_DECISION_BYTES;
    if (expected != size) REJECT("preparation_exact_payload_length");
    if ((WORD(36) != 1 && WORD(36) != 2) || WORD(40) > 1 || WORD(44) > 1 ||
        WORD(84) > 2 || WORD(88) & ~31u || WORD(112) > 1)
        REJECT("preparation_modes");
    if (!WORD(60) || WORD(60) > 10000000 || !WORD(64) || WORD(64) > 10000000)
        REJECT("preparation_update_bounds");
    if ((WORD(76) != UINT32_MAX && WORD(76) > 3) || (WORD(80) != UINT32_MAX && WORD(80) > 11))
        REJECT("preparation_roster_mode_or_selection");
    if (!WORD(84) ? (WORD(68) || WORD(72)) : (!WORD(68) || WORD(68) > 99 || WORD(72) > INT32_MAX))
        REJECT("preparation_hero_level_exp");
    if (WORD(36) == 1 && WORD(84) == 1) REJECT("source_attested_cannot_grant_hero_levels");
    if (!WORD(112) && result.item_count) REJECT("preserved_inventory_has_declared_counts");
    for (unsigned i = 0; i < 5; i++) {
        if (!(WORD(88) & (1u << i)) && WORD(92 + i * 4)) REJECT("unused_context_assertion_value");
    }
    result.header = bytes; result.source = bytes + P4_PREPARE_HEADER_BYTES;
    result.actors = result.source + result.source_bytes;
    result.personas = result.actors + result.actor_count * P4_PREPARE_ACTOR_BYTES;
    result.items = result.personas + result.persona_count * P4_PREPARE_PERSONA_BYTES;
    result.decisions = result.items + result.item_count * P4_PREPARE_ITEM_BYTES;
    if (!p4_battle_save_valid(result.source, result.source_bytes)) REJECT("preparation_source_native_image");
    for (uint32_t i = 0; i < result.actor_count; i++) {
        const uint8_t *actor = result.actors + i * P4_PREPARE_ACTOR_BYTES;
        const uint32_t id = p4_prepare_word(actor);
        if (!selected_actor(id) || (!i && id != 1) || (i && id == 1)) REJECT("preparation_selected_actor_identity");
        for (uint32_t j = 0; j < i; j++)
            if (p4_prepare_word(result.actors + j * P4_PREPARE_ACTOR_BYTES) == id) REJECT("preparation_duplicate_actor");
        for (unsigned j = 4; j <= 12; j += 4) {
            const uint32_t item = p4_prepare_word(actor + j);
            if (item != UINT32_MAX && item > INT16_MAX) REJECT("preparation_equipment_id_range");
        }
        const uint32_t resource_mode = p4_prepare_word(actor + 16), tactic = p4_prepare_word(actor + 28);
        if (resource_mode > 1 || (!resource_mode && !zero(actor + 20, 8)) ||
            (resource_mode && (p4_prepare_word(actor + 20) > 999 || p4_prepare_word(actor + 24) > 999)))
            REJECT("preparation_actor_resource_values");
        if (WORD(40) && resource_mode) REJECT("competing_resource_policies");
        if ((tactic != UINT32_MAX && tactic > 4) || !zero(actor + 32, 32)) REJECT("preparation_actor_tactic_or_reserved");
    }
    uint32_t decision_cursor = 0;
    for (uint32_t i = 0; i < result.persona_count; i++) {
        const uint8_t *persona = result.personas + i * P4_PREPARE_PERSONA_BYTES;
        const uint32_t owner = p4_prepare_word(persona), slot = p4_prepare_word(persona + 4);
        const uint32_t id = p4_prepare_word(persona + 8), mode = p4_prepare_word(persona + 12);
        const uint32_t source_slot = p4_prepare_word(persona + 16), level = p4_prepare_word(persona + 20);
        const uint32_t experience = p4_prepare_word(persona + 24), origin = p4_prepare_word(persona + 28);
        const uint32_t first = p4_prepare_word(persona + 80), count = p4_prepare_word(persona + 84);
        const uint32_t mask = p4_prepare_word(persona + 88);
        if (owner < 1 || owner > 10 || (owner == 1 ? slot > 11 : slot != 0) || mode > 2 ||
            mask & ~31u || persona[79] || !zero(persona + 92, 20)) REJECT("preparation_persona_identity_mode_or_reserved");
        if ((mode || id != UINT32_MAX) && (!id || id > 255)) REJECT("preparation_persona_id_range");
        for (uint32_t j = 0; j < i; j++) {
            const uint8_t *previous = result.personas + j * P4_PREPARE_PERSONA_BYTES;
            if (p4_prepare_word(previous) == owner && p4_prepare_word(previous + 4) == slot)
                REJECT("preparation_duplicate_persona_output");
        }
        if (origin > 1 || (origin ? source_slot != UINT32_MAX : (owner == 1 ? source_slot > 11 : source_slot != 0)))
            REJECT("preparation_persona_source_slot");
        if ((!mode && (origin || mask & 16u)) || (mode == 1 && (origin != 1 || (mask != 7 && mask != 15))) ||
            (WORD(36) == 1 && (mode == 1 || origin || mask & 16u))) REJECT("preparation_persona_mode_capability");
        if (mode || mask & 1u) {
            if (!level || level > 99 || experience > INT32_MAX) REJECT("preparation_persona_level_exp");
        } else if (level || experience) REJECT("unused_persona_level_exp");
        if (mask & 2u) {
            if (!skills_valid(persona + 48)) REJECT("preparation_invalid_final_skills");
        } else if (!zero(persona + 48, 16)) REJECT("unused_persona_final_skills");
        if (mask & 16u) {
            if (mode != 2 || !skills_valid(persona + 32)) REJECT("preparation_starting_skills_mode");
        } else if (!zero(persona + 32, 16)) REJECT("unused_persona_starting_skills");
        if (mask & 4u) {
            for (unsigned j = 64; j < 69; j++) if (!persona[j] || persona[j] > 99) REJECT("preparation_persona_stat_range");
        } else if (!zero(persona + 64, 5)) REJECT("unused_persona_stat_assertions");
        if (mode == 1 || !(mask & 8u)) {
            if (!zero(persona + 69, 10)) REJECT("unused_or_synthetic_bonus_bytes");
        }
        if (mode != 2 && (first || count)) REJECT("nongrowth_persona_has_skill_decisions");
        if ((!count && first) || count > result.decision_count - decision_cursor || (count && first != decision_cursor))
            REJECT("preparation_skill_decision_partition");
        uint32_t last_level = 0;
        for (uint32_t j = 0; j < count; j++) {
            const uint8_t *decision = result.decisions + (first + j) * P4_PREPARE_DECISION_BYTES;
            const uint32_t learned_level = p4_prepare_word(decision + 8), learned = p4_prepare_word(decision + 12);
            const uint32_t replace = p4_prepare_word(decision + 16), removed = p4_prepare_word(decision + 20);
            if (p4_prepare_word(decision) != owner || p4_prepare_word(decision + 4) != slot ||
                !learned_level || learned_level > level || learned_level < last_level || !learned || learned > P4_PREPARE_MAX_SKILL ||
                (replace != UINT32_MAX && replace > 7) || removed > P4_PREPARE_MAX_SKILL || (replace == UINT32_MAX && removed))
                REJECT("preparation_skill_decision_fields");
            last_level = learned_level;
        }
        decision_cursor += count;
    }
    if (decision_cursor != result.decision_count) REJECT("preparation_unowned_skill_decisions");
    if (result.item_count) {
        uint8_t seen[1536 / 8] = {0};
        for (uint32_t i = 0; i < result.item_count; i++) {
            const uint8_t *item = result.items + i * P4_PREPARE_ITEM_BYTES;
            const uint32_t id = p4_prepare_word(item);
            if (id >= 1536 || p4_prepare_word(item + 4) > 99) REJECT("preparation_item_id_or_quantity_range");
            const uint8_t bit = (uint8_t)(1u << (id & 7));
            if (seen[id >> 3] & bit) REJECT("preparation_duplicate_inventory_id");
            seen[id >> 3] |= bit;
        }
    }
    *view = result;
    return 1;
#undef REJECT
#undef WORD
}
