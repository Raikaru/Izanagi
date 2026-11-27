#include "golden_ai_native.h"
#include "golden_ai_knowledge.h"
#include "p4pc/mod_save.h"
#include "p4pc/game_table_sources.h"
#include <stdlib.h>
#include <string.h>
static P4GoldenAiKnowledge state;
static P4ModSaveRegistration *registration;
static const P4GameTablesBootProduct *owner;
static char cache_key[64];
static int published;
static void require(int condition) { if(!condition) abort(); }
static int ready(void)
{
    return owner && owner->golden_ai==p4_golden_ai_native_product() &&
        p4_game_table_sources_is_frozen(owner->sources) &&
        !p4_game_table_sources_validate_view(owner->sources,owner->tables)?0:P4_TABLE_NOT_READY;
}
static int authorize(const P4ModSaveContext *context,void *user)
{
    (void)context; (void)user; return ready();
}
static int encode(const P4ModSaveContext *context,P4ModSaveWriter *writer,void *user)
{
    (void)context; (void)user;
    if(ready()) return P4_TABLE_NOT_READY;
    int result=p4_mod_save_write_u32(writer,UINT32_C(0x314e4b47));
    if(!result) result=p4_mod_save_write(writer,cache_key,sizeof cache_key);
    for(unsigned i=0;i<P4_GOLDEN_KNOWLEDGE_WORDS && !result;++i)
        result=p4_mod_save_write_u32(writer,state.words[i]);
    return result;
}
static int decode(const P4ModSaveContext *context,uint32_t schema,P4ModSaveReader *reader,void **staged,void *user)
{
    (void)user;
    if(!reader) return schema==0 && (!context || !context->image.data)?0:P4_TABLE_INVALID;
    if(schema!=1 || ready() || p4_mod_save_remaining(reader)!=4+64+372) return P4_TABLE_INVALID;
    uint32_t magic; char key[64];
    int result=p4_mod_save_read_u32(reader,&magic);
    if(!result) result=p4_mod_save_read(reader,key,sizeof key);
    if(result || magic!=UINT32_C(0x314e4b47) || memcmp(key,cache_key,sizeof key)) return P4_TABLE_INVALID;
    uint32_t *words=malloc(sizeof state.words);
    if(!words) return P4_TABLE_NOMEM;
    *staged=words;
    for(unsigned i=0;i<P4_GOLDEN_KNOWLEDGE_WORDS && !result;++i)
        result=p4_mod_save_read_u32(reader,words+i);
    return result;
}
static int validate(const P4ModSaveContext *context,void *staged,void *user)
{
    (void)staged; (void)user;
    return context && context->image.data?ready():0;
}
static void commit(void *staged,void *user)
{
    (void)user;
    if(staged) { memcpy(state.words,staged,sizeof state.words); free(staged); }
    else p4_golden_ai_knowledge_clear(&state);
}
static void discard(void *staged,void *user) { (void)user; free(staged); }
int p4_golden_ai_knowledge_register(const P4GameTablesBootProduct *root)
{
    if(registration) return owner==root?0:-1;
    if(!root || !root->golden_ai || !root->sources || !p4_game_table_sources_is_sealed(root->sources)) return -1;
    size_t cursor=0; const P4TableSourceOrigin *origin; unsigned found=0;
    while((origin=p4_game_table_sources_native_mode_next(root->sources,&cursor))!=NULL) {
        if(!strncmp(origin->source_key,"golden-cache:",13) && strlen(origin->source_key)==13+64) {
            memcpy(cache_key,origin->source_key+13,64); ++found;
        }
    }
    if(found!=1) return -1;
    P4ModSaveProvider provider={.struct_size=sizeof provider,.abi_version=P4_MOD_SAVE_ABI_VERSION,
        .owner="p4pc",.key="golden-ai-knowledge",.schema_version=1,.flags=P4_MOD_SAVE_REQUIRED,
        .max_bytes=4+64+372,.encode=encode,.decode=decode,.validate=validate,.commit=commit,
        .discard=discard,.authorize=authorize};
    if(p4_mod_save_register(&provider,&registration)) return -1;
    memset(&state,0,sizeof state); owner=root; published=1; return 0;
}
int p4_golden_ai_knowledge_unregister(void)
{
    if(!registration && !published) return 0;
    if(registration && p4_mod_save_unregister(registration)) return -1;
    registration=NULL; owner=NULL; published=0; memset(&state,0,sizeof state); return 0;
}
const uint32_t *p4_golden_ai_native_knowledge_words(void)
{
    return published?state.words:NULL;
}
int p4_golden_ai_native_knowledge_clear(void)
{
    if(!published) return 0;
    p4_golden_ai_knowledge_clear(&state); return 1;
}
int p4_golden_ai_native_knowledge_set(uint32_t entry,uint32_t element)
{
    if(!published) return 0;
    require(!p4_golden_ai_knowledge_set(&state,entry,element)); return 1;
}
int p4_golden_ai_native_knowledge_test(uint32_t entry,uint32_t element,uint32_t *value)
{
    if(!published) return 0;
    if(!value) abort();
    int known;
    if(p4_golden_ai_knowledge_test(&state,entry,element,&known)) abort();
    uint32_t bit=(entry&65535u)*8+(element&65535u);
    *value=known?1u<<(bit&31):0; return 1;
}
int p4_golden_ai_native_knowledge_entry(uint32_t entry,uint32_t *value)
{
    if(!published) return 0;
    uint32_t bit=(entry&65535u)*8;
    require(value && bit/32<P4_GOLDEN_KNOWLEDGE_WORDS);
    *value=state.words[bit/32]&(255u<<(bit&31)); return 1;
}
#ifdef P4_GOLDEN_AI_QUALIFICATION
void p4_golden_ai_knowledge_qualification_publish(void)
{
    require(!published && !registration); memset(&state,0,sizeof state); published=1;
}
#endif
