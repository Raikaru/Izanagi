#ifndef P4PC_GAME_ELEMENTS_AI_SCRIPT_BODY_INC
#define P4PC_GAME_ELEMENTS_AI_SCRIPT_BODY_INC
#include "p4pc/game_elements_ai_native.h"
#include <stdlib.h>
#include "p4pc/guest.h"
/* The context accessor returns a guest-address word; preserve that retail ABI. */
extern int32_t scrGetContext(void);
#define P4_AI_SCRIPT_WORK() P4_G2H((uint32_t)scrGetContext())
/* These macros do not traverse guest layouts. Every adapter passes the actual
 * calling work and source explicitly, before legacy masks/casts can discard a
 * tagged argument. Inactive functions continue through their original body. */
#define P4_AI_DECODE(SOURCE, WORK, CODE) \
    (p4_game_elements_ai_native_enabled() ? \
        p4_game_elements_ai_decode_script(P4_AI_SCRIPT_WORK(), (SOURCE), (WORK), (CODE), UINT32_MAX) : \
        btlAICommandSetActionCode((SOURCE), (WORK), (CODE)))
#define P4_AI_DECODE_ARG(SOURCE, WORK, CODE, PARAMETER) \
    (p4_game_elements_ai_native_enabled() ? \
        p4_game_elements_ai_decode_script(P4_AI_SCRIPT_WORK(), (SOURCE), (WORK), (CODE), (PARAMETER)) : \
        btlAICommandSetActionCode((SOURCE), (WORK), (CODE)))
#define P4_AI_CONDITION(SOURCE, SELECTOR, POLICY) do { \
    if (p4_game_elements_ai_native_enabled()) { \
        uint32_t p4_value; \
        if (p4_game_elements_ai_native_condition_query((SOURCE), (SELECTOR), (POLICY), &p4_value)) p4_game_elements_ai_native_fail(); \
        return (int32_t)p4_value; \
    } \
} while (0)
#define P4_AI_QUERY_SELF(POLICY) do { \
    if (p4_game_elements_ai_native_enabled()) { \
        void *p4_source = (void *)scrGetAiState(); \
        int32_t p4_value = p4_game_elements_ai_query_script(P4_AI_SCRIPT_WORK(), \
            p4_source, p4_source, 0u, (int32_t)scrGetParamInt(0), (POLICY), 0u); \
        scrSetReturnInt(p4_value); \
        return 1; \
    } \
} while (0)
#define P4_AI_QUERY_ID(POLICY) do { \
    if (p4_game_elements_ai_native_enabled()) { \
        uint32_t p4_id = (uint32_t)scrGetParamInt(0); \
        int32_t p4_selector = (int32_t)scrGetParamInt(1); \
        void *p4_source = (void *)scrGetAiState(); \
        void *p4_target = (void *)btlActionFindById(p4_id & 0x0fffffffu); \
        int32_t p4_value = p4_game_elements_ai_query_script(P4_AI_SCRIPT_WORK(), \
            p4_source, p4_target, 1u, p4_selector, (POLICY), 1u); \
        scrSetReturnInt(p4_value); \
        return 1; \
    } \
} while (0)
#define P4_AI_QUERY_SIDE(OPPOSITE, STATUS, FLAGS, POLICY) do { \
    if (p4_game_elements_ai_native_enabled()) { \
        void *p4_source = (void *)scrGetAiState(); \
        uint32_t p4_sides; \
        if (p4_game_elements_ai_native_side_mask(p4_source, (OPPOSITE), &p4_sides)) p4_game_elements_ai_native_fail(); \
        int32_t p4_value = p4_game_elements_ai_find_script(P4_AI_SCRIPT_WORK(), \
            p4_source, 0u, (int32_t)scrGetParamInt(0), p4_sides, (STATUS), (FLAGS), (POLICY)); \
        scrSetReturnInt(p4_value); \
        return 1; \
    } \
} while (0)
#define P4_AI_SELECT(SOURCE, VALUE, STATUS, FLAGS, REJECTED, ORIGINAL, POLICY) \
    (p4_game_elements_ai_native_enabled() ? \
        p4_game_elements_ai_select_script(P4_AI_SCRIPT_WORK(), (void *)(SOURCE), 0u, \
            (int32_t)(VALUE), (int32_t)(STATUS), (int32_t)(FLAGS), (int32_t)(REJECTED), (POLICY)) : \
        btlAICommandSelectCandidates((SOURCE), (VALUE), (STATUS), (FLAGS), (REJECTED), (ORIGINAL)))
static inline uint32_t p4_ai_score_normal(void *target_unit, uint32_t genus,
    const P4GameActionFrame *frame, P4GameAffinityQuery *query)
{
    uint32_t value;
    if (p4_game_elements_ai_native_test(target_unit, frame, genus, P4_GAME_AI_TEST_NORMAL, &value, query)) p4_game_elements_ai_native_fail();
    return value;
}
static inline int p4_ai_score_same_frame(const P4GameActionFrame *a, const P4GameActionFrame *b)
{
    return a->element.valid == b->element.valid &&
        a->element.element_id == b->element.element_id &&
        a->element.channel_id == b->element.channel_id &&
        a->element.flags == b->element.flags &&
        a->element.family == b->element.family &&
        a->element.component == b->element.component &&
        a->skill_id == b->skill_id && a->source_kind == b->source_kind &&
        a->source_id == b->source_id && a->native_options == b->native_options &&
        a->owned_binding == b->owned_binding;
}
static inline int p4_ai_score_allowed(void *source, void *target, const P4GameActionFrame *frame)
{
    int allowed = p4_game_elements_ai_vm_candidate_allowed(P4_AI_SCRIPT_WORK(), source, target, frame);
    if (allowed < 0) p4_game_elements_ai_native_fail();
    return allowed;
}
#endif
