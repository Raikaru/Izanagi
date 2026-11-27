#ifndef P4_GOLDEN_AI_KNOWLEDGE_H
#define P4_GOLDEN_AI_KNOWLEDGE_H
#include <stdint.h>
#define P4_GOLDEN_KNOWLEDGE_WORDS 93u
typedef struct P4GoldenAiKnowledge {
    uint32_t words[P4_GOLDEN_KNOWLEDGE_WORDS];
    uint32_t backup[P4_GOLDEN_KNOWLEDGE_WORDS];
    int backup_valid;
} P4GoldenAiKnowledge;
void p4_golden_ai_knowledge_clear(P4GoldenAiKnowledge *state);
int p4_golden_ai_knowledge_set(P4GoldenAiKnowledge *state,uint32_t entry,uint32_t element);
int p4_golden_ai_knowledge_test(const P4GoldenAiKnowledge *state,uint32_t entry,uint32_t element,int *known);
int p4_golden_ai_knowledge_clear_entry(P4GoldenAiKnowledge *state,uint32_t entry);
int p4_golden_ai_knowledge_reveal(P4GoldenAiKnowledge *state,uint32_t entry);
void p4_golden_ai_knowledge_backup(P4GoldenAiKnowledge *state);
int p4_golden_ai_knowledge_restore(P4GoldenAiKnowledge *state);
#endif
