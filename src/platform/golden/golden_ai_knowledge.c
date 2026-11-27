#include "golden_ai_knowledge.h"
#include <string.h>
static int bit_index(uint32_t entry,uint32_t element,uint32_t *bit)
{
    *bit=(entry&65535u)*8u+(element&65535u);
    return *bit<P4_GOLDEN_KNOWLEDGE_WORDS*32u?0:-1;
}
void p4_golden_ai_knowledge_clear(P4GoldenAiKnowledge *state) { memset(state->words,0,sizeof state->words); }
int p4_golden_ai_knowledge_set(P4GoldenAiKnowledge *state,uint32_t entry,uint32_t element)
{
    uint32_t bit; if(!state || bit_index(entry,element,&bit)) return -1;
    state->words[bit/32]|=1u<<(bit&31); return 0;
}
int p4_golden_ai_knowledge_test(const P4GoldenAiKnowledge *state,uint32_t entry,uint32_t element,int *known)
{
    uint32_t bit; if(!state || !known || bit_index(entry,element,&bit)) return -1;
    *known=!!(state->words[bit/32]&(1u<<(bit&31))); return 0;
}
int p4_golden_ai_knowledge_clear_entry(P4GoldenAiKnowledge *state,uint32_t entry)
{
    uint32_t bit; if(!state || bit_index(entry,7,&bit)) return -1;
    bit-=7; state->words[bit/32]&=~(255u<<(bit&31)); return 0;
}
int p4_golden_ai_knowledge_reveal(P4GoldenAiKnowledge *state,uint32_t entry)
{
    uint32_t bit; if(!state || bit_index(entry,7,&bit)) return -1;
    bit-=7; state->words[bit/32]|=0xdfu<<(bit&31); return 0;
}
void p4_golden_ai_knowledge_backup(P4GoldenAiKnowledge *state)
{
    memcpy(state->backup,state->words,sizeof state->words); state->backup_valid=1;
}
int p4_golden_ai_knowledge_restore(P4GoldenAiKnowledge *state)
{
    if(!state || !state->backup_valid) return -1;
    memcpy(state->words,state->backup,sizeof state->words); return 0;
}
