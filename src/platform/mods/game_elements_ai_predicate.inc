/* One predicate body for production and isolated boundary witnesses. Native
 * publication/actor lifetime validation belongs to the calling bridge.
 * Verified against001DA230..001DA2F0,001DB360 and001DB9F0: BASE flags,
 * wrapper normal flags, unknown-first return, then buff/physical gates. */
static int p4_ai_predicate_affinity(void *unit, const P4GameActionFrame *frame,
    uint32_t wrapper, uint32_t literal_profile, uint32_t *word)
{
    if (!literal_profile) {
        *word = wrapper ? p4_game_elements_native_element_affinity(unit, frame) :
            p4_game_elements_native_affinity(unit, frame);
        return P4_TABLE_OK;
    }
    if (!frame->element.valid) { *word = 0u; return P4_TABLE_OK; }
    return wrapper ? p4_game_elements_native_profile_affinity(unit, frame->element.element_id,
        frame->element.channel_id, frame->element.component, word) :
        p4_game_elements_native_profile_base_affinity(unit, frame->element.element_id,
            frame->element.channel_id, frame->element.component, word);
}
static int p4_ai_test_frame(void *unit, const P4GameActionFrame *frame,
    uint32_t genus, uint32_t policy, uint32_t literal_profile,
    uint32_t *value, P4GameAffinityQuery *prediction)
{
    if (!unit || !frame || !value || policy < P4_GAME_AI_TEST_REPEL ||
        policy > P4_GAME_AI_TEST_RESIST) return P4_TABLE_INVALID;
    if (policy <= P4_GAME_AI_TEST_WEAK || policy == P4_GAME_AI_TEST_RESIST) {
        static const uint32_t masks[5] = {P4_AFF_REPEL, P4_AFF_DRAIN, P4_AFF_NULL, P4_AFF_WEAK, P4_AFF_RESIST};
        uint32_t affinity;
        int result = p4_ai_predicate_affinity(unit, frame, 0u, literal_profile, &affinity);
        if (result) return result;
        *value = !!(affinity & masks[policy == P4_GAME_AI_TEST_RESIST ? 4u : policy - 1u]);
        if (prediction) *prediction = (P4GameAffinityQuery){P4_AFFINITY_QUERY_ACTUAL, 0u};
        return P4_TABLE_OK;
    }
    P4GameAffinityQuery query = {P4_AFFINITY_QUERY_ACTUAL, 0u};
    if (policy != P4_GAME_AI_TEST_NORMAL_STATUS) {
        int result = p4_game_knowledge_native_ai_query(unit, frame, &query);
        if (result) return result;
        if (prediction) *prediction = query;
        if (query.mode == P4_AFFINITY_QUERY_PROVIDED) {
            *value = policy != P4_GAME_AI_TEST_NOT_NORMAL;
            return P4_TABLE_OK;
        }
    } else if (prediction) *prediction = query;
    const P4GameActionElement *element = &frame->element;
    if (policy == P4_GAME_AI_TEST_NORMAL && p4_game_action_element_has_affinity(element)) {
        if (p4_game_action_element_is_physical(element)) {
            if (datCalcGetBuffStatus(unit, 0x10) > 0) { *value = 0u; return P4_TABLE_OK; }
        } else if (element->family != 5u && datCalcGetBuffStatus(unit, 0x11) > 0) {
            *value = 0u; return P4_TABLE_OK;
        }
    }
    if ((policy == P4_GAME_AI_TEST_NORMAL || policy == P4_GAME_AI_TEST_NORMAL_NO_GATE) &&
        p4_game_action_element_is_physical(element)) {
        if (genus == 1u) {
            if (P4_G16(P4_H2G(unit)) & 0x100u) { *value = 0u; return P4_TABLE_OK; }
        } else {
            const uint16_t *skills = (const uint16_t *)datCalcSkillList(unit);
            uint32_t count = datCalcGetTableLength(unit);
            for (uint32_t i = 0; i < count; ++i) {
                if (skills[i] == 0x1f6u || skills[i] == 0x1f7u || skills[i] == 0x1f8u) {
                    *value = 0u; return P4_TABLE_OK;
                }
            }
        }
    }
    uint32_t affinity;
    int result = p4_ai_predicate_affinity(unit, frame, 1u, literal_profile, &affinity);
    if (result) return result;
    uint32_t blocked = !!(affinity & (P4_AFF_REPEL | P4_AFF_DRAIN | P4_AFF_NULL));
    *value = policy == P4_GAME_AI_TEST_NOT_NORMAL ? blocked : !blocked;
    return P4_TABLE_OK;
}

/* The side-ANY root's ordering is NOT normal's unknown-first ordering.
 * Option8 may bypass later death/status tests, but never a preceding barrier. */
static int p4_ai_side_frame_gate(void *unit, const P4GameActionFrame *frame,
    uint32_t genus, uint32_t options, uint32_t *test, uint32_t *unknown)
{
    *test = *unknown = 0u;
    if ((options & 2u) && p4_game_action_element_has_affinity(&frame->element)) {
        if (p4_game_action_element_is_physical(&frame->element)) {
            if (datCalcGetBuffStatus(unit, 0x10) > 0) return P4_TABLE_OK;
        } else if (frame->element.family != 5u && datCalcGetBuffStatus(unit, 0x11) > 0) {
            return P4_TABLE_OK;
        }
    }
    if ((options & 12u) && genus == 1u) {
        P4GameAffinityQuery query;
        int result = p4_game_knowledge_native_ai_query(unit, frame, &query);
        if (result) return result;
        if (query.mode == P4_AFFINITY_QUERY_PROVIDED) {
            *unknown = !(options & 4u);
            return P4_TABLE_OK;
        }
    }
    *test = 1u;
    return P4_TABLE_OK;
}
