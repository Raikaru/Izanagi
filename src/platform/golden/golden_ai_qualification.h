#ifndef P4_GOLDEN_AI_QUALIFICATION_H
#define P4_GOLDEN_AI_QUALIFICATION_H
#ifndef P4_GOLDEN_AI_QUALIFICATION
#error Golden AI qualification must never be compiled into a release target
#endif
#include "golden_ai_native.h"
int p4_golden_ai_qualification_product(const P4GoldenDomainMap *map,
    const P4GoldenCacheTable raw[P4_GAI_RAW_COUNT],const P4GoldenAiProduct **out);
int32_t btlCheckBattleEnd(void);
void btlTargetResetCommandRecord(void *record);
int32_t scrFindPrcdIdxByName(void *header,const char *name);
void p4_golden_ai_qualification_seed(uint32_t seed);
void p4_golden_ai_knowledge_qualification_publish(void);
uint64_t p4_golden_ai_qualification_native_calls(void);
int p4_golden_ai_qualification_run(void *work,uint32_t *result,size_t steps,
    uint32_t *pc,uint32_t *command,uint8_t *entered,size_t count,int synthetic_locals);
int p4_golden_ai_qualification_command(void *work,uint32_t function,
    const int64_t *arguments,size_t count,int64_t *result);
#endif
