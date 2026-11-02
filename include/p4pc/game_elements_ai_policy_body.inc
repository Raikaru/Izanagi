#ifndef P4PC_GAME_ELEMENTS_AI_POLICY_BODY_INC
#define P4PC_GAME_ELEMENTS_AI_POLICY_BODY_INC
#include "p4pc/game_elements_ai_native.h"
#include <stdlib.h>
/* Included only after the translated unit's generic callback type and native
 * declarations. This identity map is shared by side-ANY and target selectors;
 * base-affinity predicates must never become wrapper/normal predicates. */
extern s32 btlAICommandNomalCheckNoGate(u8 *, s32);
extern s32 btlAICommandNomalStatusCheck(u8 *, u32);
extern s32 btlCond_MYNOTNOMAL(u8 *, s16);
static inline u32 p4_btl_predicate_policy(code predicate)
{
    if (predicate == (code)btlCond_MYTAISEI) return P4_GAME_AI_TEST_RESIST;
    if (predicate == (code)btlCond_MYHANSYA) return P4_GAME_AI_TEST_REPEL;
    if (predicate == (code)btlCond_MYKYUSYU) return P4_GAME_AI_TEST_DRAIN;
    if (predicate == (code)btlCond_MYMUKOU) return P4_GAME_AI_TEST_NULL;
    if (predicate == (code)btlCond_MYWEAK) return P4_GAME_AI_TEST_WEAK;
    if (predicate == (code)btlCond_MYNOMAL) return P4_GAME_AI_TEST_NORMAL;
    if (predicate == (code)btlAICommandNomalCheckNoGate) return P4_GAME_AI_TEST_NORMAL_NO_GATE;
    if (predicate == (code)btlAICommandNomalStatusCheck) return P4_GAME_AI_TEST_NORMAL_STATUS;
    if (predicate == (code)btlCond_MYNOTNOMAL) return P4_GAME_AI_TEST_NOT_NORMAL;
    return 0;
}
static inline code p4_btl_predicate_from_policy(u32 policy)
{
    switch (policy) {
    case P4_GAME_AI_TEST_RESIST: return (code)btlCond_MYTAISEI;
    case P4_GAME_AI_TEST_REPEL: return (code)btlCond_MYHANSYA;
    case P4_GAME_AI_TEST_DRAIN: return (code)btlCond_MYKYUSYU;
    case P4_GAME_AI_TEST_NULL: return (code)btlCond_MYMUKOU;
    case P4_GAME_AI_TEST_WEAK: return (code)btlCond_MYWEAK;
    case P4_GAME_AI_TEST_NORMAL: return (code)btlCond_MYNOMAL;
    case P4_GAME_AI_TEST_NORMAL_NO_GATE: return (code)btlAICommandNomalCheckNoGate;
    case P4_GAME_AI_TEST_NORMAL_STATUS: return (code)btlAICommandNomalStatusCheck;
    case P4_GAME_AI_TEST_NOT_NORMAL: return (code)btlCond_MYNOTNOMAL;
    default: p4_game_elements_ai_native_fail();
    }
}
#endif
