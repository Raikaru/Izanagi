#ifndef P4_GOLDEN_AI_NATIVE_H
#define P4_GOLDEN_AI_NATIVE_H
#include "golden_ai_product.h"
#include "p4pc/golden_ai_native.h"
/* Native record owners. Kept here rather than ad-hoc command-local prototypes. */
uint8_t *datPersonaGetRecord(uint16_t id);
int16_t datGetEquippedItemByUnit(int16_t id,int16_t slot);
void btlAICommandSetActionCode(void *actor,void *work,uint16_t code);
/* Private runtime owner: no allocation-zero state is admitted as live backing. */
int p4_golden_ai_native_stage(const P4GoldenAiProduct *product);
void p4_golden_ai_native_activate(const P4GoldenAiProduct *product);
const P4GoldenAiProduct *p4_golden_ai_native_product(void);
int p4_golden_ai_native_refresh(uint32_t actor,uint32_t command,P4GoldenAi **out);
void p4_golden_ai_native_commit(uint32_t actor,uint32_t command,int64_t result);
int p4_golden_vm_native_prepare(void);
int p4_golden_vm_native_quiescent(void);
void p4_golden_vm_native_reset(void);
int p4_golden_ai_knowledge_register(const P4GameTablesBootProduct *root);
int p4_golden_ai_knowledge_unregister(void);
const uint32_t *p4_golden_ai_native_knowledge_words(void);
#endif
