/* Raw Golden AI dependencies retained from the exact verified M5 cache cohort. */
#include "golden_ai_product.h"
#include "golden_mode_boot.h"
#include "golden_mode_sources.h"
#ifdef P4_GOLDEN_AI_QUALIFICATION
#include "golden_ai_qualification.h"
#endif
#include "golden_mode_declare_cached.h"
#include "golden_mode_declare_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static P4GoldenAiProduct *owned;
static const char *const names[]={"golden-ai/item","golden-ai/levels","golden-ai/persona",
    "golden-ai/programs","golden-ai/skill","golden-ai/unit"};
static void dispose(P4GoldenAiProduct *p)
{
    if(!p) return;
    p4_golden_cache_free(p->raw,P4_GAI_RAW_COUNT); free(p);
}
static int fail(char *reason,size_t capacity,const char *text)
{
    if(reason && capacity) snprintf(reason,capacity,"%s",text);
    return -1;
}
static int parse(P4GoldenAiProduct *p)
{
    if(p4_golden_tbl_spans(p->raw[P4_GAI_PERSONA].data,p->raw[P4_GAI_PERSONA].size,p->persona,12)!=6 ||
       p4_golden_tbl_spans(p->raw[P4_GAI_PROGRAMS].data,p->raw[P4_GAI_PROGRAMS].size,p->programs,12)!=11 ||
       p4_golden_tbl_spans(p->raw[P4_GAI_SKILL].data,p->raw[P4_GAI_SKILL].size,p->skill,12)!=2 ||
       p4_golden_tbl_spans(p->raw[P4_GAI_UNIT].data,p->raw[P4_GAI_UNIT].size,p->unit,12)!=3 ||
       p->skill[0].size!=624*2 || p->skill[1].size!=472*44 ||
       p->unit[0].size!=368*60 || p->unit[1].size!=368*32 || p->unit[2].size!=256*32 ||
       p->raw[P4_GAI_ITEM].size!=16+2560*68+16+2560*24 ||
       p->raw[P4_GAI_LEVELS].size!=P4_GOLDEN_LEVEL_BYTES) return -1;
    for(unsigned i=0;i<2;++i)
        if(p4_golden_vm_program(&p->scripts[i],p->programs[9+i].bytes,p->programs[9+i].size)) return -1;
    return 0;
}
static int admit(void *context,char *reason,size_t capacity)
{
    const P4GoldenModeSources *source=context;
    if(!source || !source->projection.skill_metadata.data ||
        !source->projection.skill_metadata.size)
        return fail(reason,capacity,"R4 production lift requires M7a SKILL1 projection");
    if(source->outputs.table_count>P4_GOLDEN_OUTPUT_NATIVE_COUNT+P4_GOLDEN_OUTPUT_EXTRA_COUNT)
        return fail(reason,capacity,"invalid Golden AI output cohort");
    P4GoldenAiProduct view;
    /* Admission precedes cache publication, which fills the extra descriptors. */
    for(unsigned i=0;i<P4_GAI_RAW_COUNT;++i) {
        const P4GoldenCacheTable *raw=NULL;
        for(size_t j=0;j<source->outputs.table_count;++j)
            if(source->outputs.tables[j].name &&
                !strcmp(source->outputs.tables[j].name,names[i]))
                raw=source->outputs.tables+j;
        if(!raw || !raw->data || !raw->size)
            return fail(reason,capacity,"missing Golden AI raw prerequisite");
        view.raw[i]=*raw;
    }
    return parse(&view)?fail(reason,capacity,"malformed Golden AI raw prerequisite"):0;
}
int p4_golden_ai_adapter_register(P4GameMode mode)
{
    if(mode==P4_GAME_MODE_PS2) return 0;
    if(mode!=P4_GAME_MODE_GOLDEN) return -1;
    return p4_golden_mode_adapter_register("AICALC9/10 compatibility",admit);
}
int p4_golden_ai_cache_declare(void *context,P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCacheTable *extra,size_t count,char *reason,size_t capacity)
{
    if(!context || !builder || !extra || owned)
        return fail(reason,capacity,"invalid Golden AI cache ownership");
    P4GoldenAiProduct *p=calloc(1,sizeof *p);
    if(!p) return fail(reason,capacity,"cannot allocate Golden AI cache product");
    p->builder=builder; p->map=*(const P4GoldenDomainMap *)context;
    for(unsigned i=0;i<P4_GAI_RAW_COUNT;++i) {
        const P4GoldenCacheTable *source=NULL;
        for(size_t j=0;j<count;++j)
            if(extra[j].name && !strcmp(extra[j].name,names[i])) { source=extra+j; break; }
        if(!source || !source->data || !source->size) goto invalid;
        p->raw[i]=(P4GoldenCacheTable){names[i],malloc(source->size),source->size};
        if(!p->raw[i].data) goto invalid;
        memcpy(p->raw[i].data,source->data,source->size);
    }
    if(parse(p)) goto invalid;
    if(p4_golden_mode_declare_cached(context,builder,extra,count,reason,capacity) ||
       p4_golden_mode_cache_origin(P4_GAME_MODE_GOLDEN,builder,&p->cache_origin,reason,capacity)) {
        dispose(p); return -1;
    }
    owned=p; return 0;
invalid:
    dispose(p); return fail(reason,capacity,"missing or malformed verified Golden AI dependencies");
}
const P4GoldenAiProduct *p4_golden_ai_product(const P4GameTablesDefinitionBuilder *builder)
{
    return owned && owned->builder==builder ? owned : NULL;
}
int p4_golden_ai_product_release(const P4GameTablesDefinitionBuilder *builder)
{
    if(!owned) return 0;
    if(owned->builder!=builder || owned->consumers) return -1;
    if(p4_golden_ai_boot_forget(owned)) return -1;
    dispose(owned); owned=NULL; return 0;
}
int p4_golden_ai_product_acquire(const P4GoldenAiProduct *product)
{
    if(product!=owned || !owned || owned->consumers) return -1;
    ++owned->consumers; return 0;
}
void p4_golden_ai_product_detach(const P4GoldenAiProduct *product)
{
    if(product!=owned || !owned || owned->consumers!=1) abort();
    --owned->consumers;
}
#ifdef P4_GOLDEN_AI_QUALIFICATION
int p4_golden_ai_qualification_product(const P4GoldenDomainMap *map,
    const P4GoldenCacheTable raw[P4_GAI_RAW_COUNT],const P4GoldenAiProduct **out)
{
    if(!map || !raw || !out || owned) return -1;
    P4GoldenAiProduct *p=calloc(1,sizeof *p);
    if(!p) return -1;
    p->map=*map;
    for(unsigned i=0;i<P4_GAI_RAW_COUNT;++i) {
        if(!raw[i].data || !raw[i].size) { dispose(p); return -1; }
        p->raw[i]=(P4GoldenCacheTable){names[i],malloc(raw[i].size),raw[i].size};
        if(!p->raw[i].data) { dispose(p); return -1; }
        memcpy(p->raw[i].data,raw[i].data,raw[i].size);
    }
    if(parse(p)) { dispose(p); return -1; }
    owned=p; *out=p; return 0;
}
#endif
