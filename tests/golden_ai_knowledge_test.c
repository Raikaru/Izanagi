/* Invented lifecycle vectors, not a gameplay receipt. */
#include "../src/platform/golden/golden_ai_knowledge.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main(void)
{
    P4GoldenAiKnowledge state={0};
    int known=-1;
    CHECK(p4_golden_ai_knowledge_restore(&state)==-1);
    CHECK(!p4_golden_ai_knowledge_set(&state,4,3));
    CHECK(state.words[1]==8);
    CHECK(!p4_golden_ai_knowledge_test(&state,4,3,&known) && known);
    CHECK(!p4_golden_ai_knowledge_reveal(&state,5));
    CHECK((state.words[1]>>8)==0xdf);
    CHECK(!p4_golden_ai_knowledge_test(&state,5,5,&known) && !known);
    p4_golden_ai_knowledge_backup(&state);
    CHECK(!p4_golden_ai_knowledge_clear_entry(&state,5));
    CHECK(state.words[1]==8);
    CHECK(!p4_golden_ai_knowledge_restore(&state));
    CHECK(state.words[1]==0xdf08);
    CHECK(!p4_golden_ai_knowledge_set(&state,371,7));
    CHECK(state.words[92]==0x80000000u);
    CHECK(p4_golden_ai_knowledge_set(&state,372,0)==-1);
    CHECK(p4_golden_ai_knowledge_clear_entry(&state,372)==-1);
    CHECK(p4_golden_ai_knowledge_reveal(&state,372)==-1);
    CHECK(!p4_golden_ai_knowledge_set(&state,65540,65539));
    CHECK(state.words[1]==0xdf08);
    p4_golden_ai_knowledge_clear(&state);
    for(unsigned i=0;i<P4_GOLDEN_KNOWLEDGE_WORDS;++i) CHECK(!state.words[i]);
    CHECK(!p4_golden_ai_knowledge_restore(&state));
    CHECK(state.words[1]==0xdf08 && !state.words[92]);
    puts("golden_ai_knowledge: observation, seven-bit reveal, clear, bounds and backup/restore pass");
    return 0;
}
