#ifndef P4PC_GAME_KNOWLEDGE_AI_BODY_INC
#define P4PC_GAME_KNOWLEDGE_AI_BODY_INC
/* Included by the translated AI unit after its native declarations. DatUnit
 * has no host-pointer fields here; action/unit guest-pointer traversal stays
 * in the scoped mirror adapters, where the normal pointer census owns it. */
#include "p4pc/game_knowledge_native.h"
#include "p4pc/game_elements_ai_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/gmem.h"
#include <stdlib.h>

/* These two retail callbacks carry legacy selectors, not tagged semantic IDs.
 * Phys0 means the physical damage family. Light6/Dark7 still mean their KO
 * component; a new damage component does not silently become a legacy KO. */
static int p4_knowledge_ai_legacy_match(const P4GameActionFrame *frame, int32_t selector)
{
    if (selector == -1) return frame->element.valid == 0;
    if (selector < 0 || selector >= (int32_t)P4_ELEMENT_RETAIL_COUNT ||
        frame->element.valid != 1) return 0;
    if (selector == 0)
        return p4_game_action_element_is_physical(&frame->element) &&
            frame->element.component == P4_ACTION_COMPONENT_DAMAGE;
    if (frame->element.element_id != (uint32_t)selector) return 0;
    if (selector == 6 || selector == 7)
        return frame->element.component == P4_ACTION_COMPONENT_INSTANT_KO;
    if (selector >= 8 && selector < 16)
        return frame->element.component == P4_ACTION_COMPONENT_STATUS;
    return frame->element.component == P4_ACTION_COMPONENT_DAMAGE;
}

static int p4_knowledge_ai_effect_match(const P4GameActionFrame *frame, int32_t filter)
{
    if (filter < 0 || filter > 2) return 1;
    if (!p4_game_action_element_is_offensive(&frame->element) ||
        frame->element.component != P4_ACTION_COMPONENT_DAMAGE) return 0;
    if (filter == 1) return !p4_game_action_element_is_physical(&frame->element);
    if (filter == 2) return p4_game_action_element_is_physical(&frame->element);
    return 1;
}

/* Retain 001DB360's unknown-first policy and its native buff/physical gates.
 * The query is retained with this candidate for prediction: an unknown target
 * must never become an ACTUAL affinity read in the later HP-ranking pass. */
static int p4_knowledge_ai_normal(DatUnit *target, uint32_t genus,
    P4GameAiCandidate *candidate)
{
    uint32_t value;
    if (p4_game_elements_ai_native_test(target, &candidate->frame, genus, P4_GAME_AI_TEST_NORMAL,
        &value, &candidate->query)) p4_game_elements_ai_native_fail();
    candidate->query_valid = 1u;
    return (int)value;
}

static void p4_knowledge_ai_available_refresh(uint8_t *action, DatUnit *source)
{
    P4GameAiCandidate available[P4_GAME_AI_CANDIDATE_COUNT];
    const uint16_t *skills = (const uint16_t *)datCalcSkillList(source);
    const uint32_t slots = datCalcGetTableLength(source);
    uint32_t count = 0;
    if (slots > P4_GAME_AI_EQUIPPED_SLOTS) p4_game_elements_ai_native_fail();
    for (uint32_t i = 0; i <= slots; ++i) {
        uint32_t command = i ? skills[i - 1u] : 0u;
        if (i && (!command || !p4_game_skill_active(command))) continue;
        P4GameAiCandidate *candidate = &available[count];
        /* Lookahead is pure, including equipped commands that this VM never
         * queries or selects. Eligibility and actual source use happen at
         * their real consumer, never during spawn/refresh/bank binding. */
        if (p4_game_elements_ai_native_preview_command(action, command, candidate)) p4_game_elements_ai_native_fail();
        candidate->eligibility = P4_GAME_AI_CANDIDATE_EQUIPPED;
        ++count;
    }
    const uint8_t *record;
    if (p4_game_elements_ai_native_source_record(action, &record)) p4_game_elements_ai_native_fail();
    for (uint32_t group_entry = 0; group_entry < P4_GAME_AI_WEIGHT_GROUPS * P4_GAME_AI_WEIGHT_ENTRIES; ++group_entry) {
        const uint8_t *entry = record + 0x2cu + group_entry * 8u;
        if (!entry[0]) continue; /* Authored padding is never a command. */
        uint32_t code = entry[2] | (uint32_t)entry[3] << 8;
        uint32_t command = code == 0x8000u ? 0u : code;
        if (command & 0xf000u) continue; /* Opaque plans execute the original decoder. */
        uint32_t found = 0u;
        for (uint32_t index = 0; index < count; ++index)
            if (available[index].command == command) {
                available[index].eligibility |= P4_GAME_AI_CANDIDATE_TABLE;
                found = 1u;
            }
        if (found) continue;
        if (count == P4_GAME_AI_CANDIDATE_COUNT ||
            p4_game_elements_ai_native_preview_command(action, command, &available[count])) p4_game_elements_ai_native_fail();
        ++count;
    }
    if (p4_game_elements_ai_native_available_set(action, available, count)) p4_game_elements_ai_native_fail();
}

static int32_t p4_knowledge_ai_has_skill(uint8_t *action, DatUnit *source, int32_t selector)
{
    P4GameActionFrame frame;
    const uint16_t *skills;
    uint32_t i, count;
    p4_game_elements_native_primary_frame(source, 0, 0, &frame);
    if (p4_knowledge_ai_legacy_match(&frame, selector)) return 1;
    count = datCalcGetTableLength(source);
    skills = (const uint16_t *)datCalcSkillList(source);
    for (i = 0; i < count; ++i) {
        if (skills[i] == 0 || !p4_game_skill_active(skills[i])) continue;
        p4_game_elements_native_primary_frame(source, skills[i], 0, &frame);
        if (p4_knowledge_ai_legacy_match(&frame, selector) && btlCommandUsable(action, skills[i])) return 1;
    }
    return 0;
}

/* The original 001DD1C0 ranking order, native HP channel1, tie rules and cache
 * clearing remain intact. Only the canonical amount route gains the retained
 * component frame and target-local ACTUAL/neutral prediction policy. */
static const P4GameAiCandidate *p4_knowledge_ai_pick_hp(DatUnit *source, DatUnit *target,
    const P4GameAiCandidate *candidates, uint32_t count, uint32_t cost_class)
{
    const uint32_t target_hp = target->hp & 0xffffu;
    int32_t best_hp = 0xffffff;
    const P4GameAiCandidate *best = NULL;
    uint32_t i;
    for (i = 0; i < count; ++i) {
        const P4GameAiCandidate *candidate = &candidates[i];
        int32_t remaining;
        if ((cost_class == 0 && candidate->cost_kind != 1) ||
            (cost_class == 1 && candidate->cost_kind != 0)) continue;
        if (candidate->query_valid != 1) p4_game_elements_ai_native_fail();
        remaining = target_hp + p4_game_elements_native_amount(candidate->command,
            source, target, 1, 1, 1, 0, 1, &candidate->frame, &candidate->query);
        if (remaining > 0 && best_hp > 0) {
            if (remaining < best_hp) {
                best = candidate;
                best_hp = remaining;
            }
        } else if (remaining <= 0 && (best_hp > 0 || best_hp < remaining)) {
            best = candidate;
            best_hp = remaining;
        }
    }
    datCalcClearField18((int32_t)P4_H2G(source));
    datCalcClearField18((int32_t)P4_H2G(target));
    return best;
}

static int32_t p4_knowledge_ai_collect_body(void *script_work, uint8_t *action, uint8_t *target_action,
    DatUnit *source, DatUnit *target, uint32_t target_genus,
    int32_t cost_preference, int32_t effect_filter, int32_t selector, int use_selector,
    const P4GameActionElement *typed_element, int publish)
{
    /* 0023E130 returns exactly8. The basic command plus all eight slots need9,
     * unlike the retail collector's eight-word local array. Frames never enter
     * guest storage or lose their full-u32 leaf/channel/source identity. */
    P4GameAiCandidate candidates[P4_GAME_AI_EQUIPPED_SLOTS + 1u];
    const P4GameAiCandidate *available;
    uint32_t available_count, count = 0;
    const P4GameAiCandidate *selected;
    if (!script_work) p4_knowledge_ai_available_refresh(action, source);
    if (p4_game_elements_ai_native_available(action, &available, &available_count)) p4_game_elements_ai_native_fail();
    for (uint32_t i = 0; i < available_count; ++i) {
        if (!(available[i].eligibility & P4_GAME_AI_CANDIDATE_EQUIPPED)) continue;
        P4GameAiCandidate *candidate = &candidates[count];
        *candidate = available[i];
        if (candidate->command && !btlCommandUsable(action, candidate->command)) continue;
        if (p4_game_elements_ai_native_candidate_use(action, candidate->command, &candidate->frame)) p4_game_elements_ai_native_fail();
        if (use_selector) {
            if (typed_element) {
                const P4GameActionElement *element = &candidate->frame.element;
                if (element->valid != typed_element->valid || element->element_id != typed_element->element_id ||
                    element->channel_id != typed_element->channel_id || element->flags != typed_element->flags ||
                    element->family != typed_element->family || element->component != typed_element->component) continue;
            } else if (!p4_knowledge_ai_legacy_match(&candidate->frame, selector)) continue;
        } else if (!p4_knowledge_ai_effect_match(&candidate->frame, effect_filter)) {
            continue;
        }
        int allowed = p4_game_elements_ai_vm_candidate_allowed(script_work, action, target_action, &candidate->frame);
        if (allowed < 0) p4_game_elements_ai_native_fail();
        if (!allowed) continue;
        if (!p4_knowledge_ai_normal(target, target_genus, candidate)) continue;
        ++count;
    }
    if (!count) {
        if (publish && p4_game_elements_ai_native_clear_selection(action)) p4_game_elements_ai_native_fail();
        return -1;
    }
    if (cost_preference != 0 && cost_preference != 1) {
        selected = p4_knowledge_ai_pick_hp(source, target, candidates, count, 0xffffu);
    } else {
        selected = p4_knowledge_ai_pick_hp(source, target, candidates, count, (uint32_t)cost_preference);
        if (!selected)
            selected = p4_knowledge_ai_pick_hp(source, target, candidates, count, 1u - (uint32_t)cost_preference);
    }
    if (!selected) {
        if (publish && p4_game_elements_ai_native_clear_selection(action)) p4_game_elements_ai_native_fail();
        return -1;
    }
    if (publish && p4_game_elements_ai_native_select(action, target_action, selected, NULL)) p4_game_elements_ai_native_fail();
    if (publish && script_work) {
        P4GameAiSelection selection;
        if (p4_game_elements_ai_native_selected(action, &selection) ||
            p4_game_elements_ai_vm_pending_selection(script_work, &selection, selected->command)) p4_game_elements_ai_native_fail();
    }
    return (int32_t)selected->command;
}
static int32_t p4_knowledge_ai_collect(void *script_work, uint8_t *action, uint8_t *target_action,
    DatUnit *source, DatUnit *target, uint32_t target_genus,
    int32_t cost_preference, int32_t effect_filter, int32_t selector, int use_selector,
    const P4GameActionElement *typed_element)
{
    return p4_knowledge_ai_collect_body(script_work, action, target_action, source, target,
        target_genus, cost_preference, effect_filter, selector, use_selector, typed_element, 1);
}
#endif
