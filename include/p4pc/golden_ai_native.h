#ifndef P4PC_GOLDEN_AI_NATIVE_H
#define P4PC_GOLDEN_AI_NATIVE_H
#include <stdint.h>
/* Scalar-only boundary for translated guest units. No host source/provider
 * descriptors or standard I/O declarations enter the mirrored translation. */
_Noreturn void p4_golden_ai_native_fail(void);
int p4_golden_ai_native_run(void *work,uint32_t *result);
void p4_golden_ai_native_work_release(void *work);
int p4_golden_ai_native_argument_integer(int index,int32_t *value);
int p4_golden_ai_native_argument_float(int index,float *value);
int p4_golden_ai_native_argument_string(int index,const char **value);
int p4_golden_ai_native_return_integer(int32_t value);
int p4_golden_ai_native_return_float(float value);
int p4_golden_ai_native_seed(uint32_t seed);
int p4_golden_ai_native_random(uint32_t *value);
void p4_golden_ai_native_action_init(void *action);
void p4_golden_ai_native_action_release(void *action);
void p4_golden_ai_native_battle_init(void);
void p4_golden_ai_native_battle_release(void);
void p4_golden_ai_native_eligible_start(void *action);
void p4_golden_ai_native_continuous_entry(void);
int p4_golden_ai_native_session_end(void);
int p4_golden_ai_native_knowledge_clear(void);
int p4_golden_ai_native_knowledge_set(uint32_t entry,uint32_t element);
int p4_golden_ai_native_knowledge_test(uint32_t entry,uint32_t element,uint32_t *value);
int p4_golden_ai_native_knowledge_entry(uint32_t entry,uint32_t *value);
uint16_t p4_golden_ai_native_wait_kind(void *action,uint16_t kind);
void p4_golden_ai_native_message_wait(void *action);
void p4_golden_ai_native_bit29(uint32_t flags);
#endif
