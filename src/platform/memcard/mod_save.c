/* Native save blocks: owned staging, canonical LE wire data and exact package
 * requirements. No filesystem, renderer, Lua or guest-state publication here. */
#include "mod_save_internal.h"
#include "mod_save_hash.h"
#include "save_image.h"
#include "p4pc/packages.h"
#include "p4pc/mod_fidelity.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PROVIDERS P4_MOD_SAVE_MAX_BLOCKS
#define MAX_REQUIREMENTS P4_MOD_SAVE_MAX_REQUIREMENTS
#define MAX_HISTORY P4_MOD_SAVE_MAX_HISTORY
#define HEADER_SIZE P4_MOD_SAVE_HEADER_BYTES
#define TRAILER_SIZE 32u
#define BLOCK_HEADER_SIZE 32u
#define DEP_HEADER_SIZE 20u
#define HISTORY_HEADER_SIZE 16u
#define MAX_WARNING_BYTES (8u * 1024u * 1024u)
static const unsigned char magic[8]={'P','4','M','O','D','S',0,0};

typedef struct Bytes {
    unsigned char *data;
    size_t size,capacity;
} Bytes;
typedef struct Dependency {
    uint32_t block,kind,required;
    char id[64],version[64];
} Dependency;
typedef struct Dependencies {
    Dependency *data;
    size_t count,capacity;
} Dependencies;
typedef struct SavedBlock {
    char owner[64],key[96];
    uint32_t schema,flags;
    const unsigned char *data;
    size_t size;
} SavedBlock;
typedef struct SavedState {
    unsigned char *blob;
    SavedBlock *blocks;
    size_t block_count;
    int witness;
    Dependencies requirements,history;
} SavedState;
struct P4ModSaveRegistration {
    P4ModSaveProvider provider;
    char owner[64],key[96];
    void (*notify)(void *);
    int notify_pending;
    /* Durable-publication notification: queued at capture commit only after the
     * native transaction committed, dispatched later at a game-thread safe point. */
    void (*published)(void *);
    int published_pending;
    struct P4ModSaveRegistration *next;
};
struct P4ModSaveCaptureStage {
    P4ModSaveRegistration *registration;
    void *value;
};
typedef struct CaptureBuilder {
    Bytes bytes;
    uint32_t block_count;
    Dependencies requirements,history;
    P4ModSaveCaptureStage *stages;
    size_t stage_count,stage_capacity;
    P4ModSaveContext context;
} CaptureBuilder;
struct P4ModSaveWriter {
    CaptureBuilder *capture;
    P4ModSaveRegistration *registration;
    size_t header,payload;
    uint32_t block;
    int started,failed,staged;
};
struct P4ModSaveReader {
    const unsigned char *data;
    size_t size,position;
    int failed;
};
typedef struct LoadStage {
    P4ModSaveRegistration *registration;
    void *value;
    int decoded;
} LoadStage;
struct P4ModSaveLoad {
    P4ModSaveContext context;
    SavedState state;
    LoadStage *stages;
    size_t stage_count;
    uint64_t retail_size;
    unsigned char retail_hash[32];
    int phase,warning_failed;
    Bytes warning; /* Full list; native UI pages it instead of truncating IDs. */
};
static P4ModSaveRegistration *registrations;
static size_t registration_count;
static int running;
static P4ModSaveLoad *pending_load;
static P4ModSaveCapture *pending_capture;
static int notifying;
static SavedState active_state;
static Dependencies touches;
static int witness_run;

enum { LOAD_DECODE=1,LOAD_VALIDATE,LOAD_READY };

static int fail(char *error,size_t cap,const char *format,...)
{
    if(cap){va_list args;va_start(args,format);vsnprintf(error,cap,format,args);va_end(args);}
    return -1;
}
static int context_fail(const P4ModSaveContext *context,const char *message)
{
    return fail(context->error,context->error_capacity,"%s",message);
}
static void clear_error(P4ModSaveContext *context)
{
    if(context->error_capacity)context->error[0]=0;
}
static int callback_failure(P4ModSaveContext *context,P4ModSaveRegistration *r,const char *phase,int result)
{
    char detail[512]={0};
    if(context->error_capacity){context->error[context->error_capacity-1]=0;snprintf(detail,sizeof detail,"%s",context->error);}
    return fail(context->error,context->error_capacity,"Save block %s:%s %s refused (%d)%s%s",
        r->owner,r->key,phase,result,detail[0]?": ":"",detail);
}
static int identifier(const char *s,size_t capacity)
{
    if(!s||!s[0])return 0;
    size_t i=0;
    for(;i<capacity&&s[i];i++){
        unsigned char c=(unsigned char)s[i];
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'||c=='_'||c=='-'||c==':'))return 0;
    }
    return i<capacity;
}
static int version_valid(const char *s)
{
    if(!s||!s[0])return 0;
    size_t i=0;
    for(;i<64&&s[i];i++)if((unsigned char)s[i]<32||(unsigned char)s[i]==127)return 0;
    return i<64;
}
static int key_compare(const char *ao,const char *ak,const char *bo,const char *bk)
{
    int n=strcmp(ao,bo);return n?n:strcmp(ak,bk);
}
static P4ModSaveRegistration *find_registration(const char *owner,const char *key)
{
    for(P4ModSaveRegistration *r=registrations;r;r=r->next){
        int order=key_compare(r->owner,r->key,owner,key);
        if(!order)return r;
        if(order>0)break;
    }
    return NULL;
}
static int reserve(void **data,size_t *capacity,size_t need,size_t stride,size_t maximum)
{
    if(need>maximum||need>SIZE_MAX/stride)return -1;
    if(need<=*capacity)return 0;
    size_t n=*capacity?*capacity:8;
    while(n<need){if(n>maximum/2){n=maximum;break;}n*=2;}
    void *next=realloc(*data,n*stride);
    if(!next)return -1;
    *data=next;*capacity=n;return 0;
}
static int append(Bytes *b,const void *data,size_t size)
{
    if(size>P4_MOD_SAVE_MAX_BYTES-b->size||(!data&&size))return -1;
    if(reserve((void **)&b->data,&b->capacity,b->size+size,1,P4_MOD_SAVE_MAX_BYTES))return -1;
    if(size)memcpy(b->data+b->size,data,size);
    b->size+=size;return 0;
}
static int zeros(Bytes *b,size_t size)
{
    if(size>P4_MOD_SAVE_MAX_BYTES-b->size||reserve((void **)&b->data,&b->capacity,b->size+size,1,P4_MOD_SAVE_MAX_BYTES))return -1;
    memset(b->data+b->size,0,size);b->size+=size;return 0;
}
static int append_warning(P4ModSaveLoad *load,const char *text)
{
    size_t length=0;
    if(text)while(length<=4096&&text[length])++length;
    if(!length||length>4096)return -1;
    Bytes *warning=&load->warning;
    size_t base=warning->size?warning->size-1:0;
    size_t extra=(base?1:0)+length+1;
    if(extra>MAX_WARNING_BYTES-base||
       reserve((void **)&warning->data,&warning->capacity,base+extra,1,MAX_WARNING_BYTES))return -1;
    if(base)warning->data[base++]='\n';
    memcpy(warning->data+base,text,length+1);warning->size=base+length+1;
    return 0;
}
int p4_mod_save_warn(const P4ModSaveContext *context,const char *message)
{
    if(!context||!context->load)return -1;
    /* Public contexts expose a const view of this framework-owned allocation. */
    P4ModSaveLoad *load=(P4ModSaveLoad *)context->load;
    if(!running||context!=&load->context||(load->phase!=LOAD_DECODE&&load->phase!=LOAD_VALIDATE))return -1;
    if(!context->image.data||!context->image.size||append_warning(load,message)){
        load->warning_failed=1;
        return context_fail(context,"Invalid/over-limit native save warning or warning storage unavailable");
    }
    return 0;
}
static void put16(unsigned char *p,uint16_t n){p[0]=(unsigned char)n;p[1]=(unsigned char)(n>>8);}
static void put32(unsigned char *p,uint32_t n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i));}
static void put64(unsigned char *p,uint64_t n){for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(8*i));}
static uint16_t get16(const unsigned char *p){return (uint16_t)((uint16_t)p[0]|(uint16_t)p[1]<<8);}
static uint32_t get32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t get64(const unsigned char *p){return (uint64_t)get32(p)|(uint64_t)get32(p+4)<<32;}
static int dependency_compare(const void *a,const void *b)
{
    const Dependency *x=a,*y=b;
    if(x->block!=y->block)return x->block>y->block?1:-1;
    if(x->kind!=y->kind)return x->kind>y->kind?1:-1;
    int n=strcmp(x->id,y->id);return n?n:strcmp(x->version,y->version);
}
static int dependency_add(Dependencies *list,const Dependency *value,int history)
{
    for(size_t i=0;i<list->count;i++){
        Dependency *old=&list->data[i];
        if(old->kind!=value->kind||strcmp(old->id,value->id)||(!history&&old->block!=value->block))continue;
        if(strcmp(old->version,value->version)){
            if(history)continue;
            return -1;
        }
        old->required|=value->required;return 0;
    }
    if(reserve((void **)&list->data,&list->capacity,list->count+1,sizeof *list->data,history?MAX_HISTORY:MAX_REQUIREMENTS))return -1;
    list->data[list->count++]=*value;return 0;
}
static void dependencies_free(Dependencies *list){free(list->data);memset(list,0,sizeof *list);}
static void state_free(SavedState *state)
{
    free(state->blob);free(state->blocks);dependencies_free(&state->requirements);dependencies_free(&state->history);memset(state,0,sizeof *state);
}
static int package_lookup(const char *id,const char *version,Dependency *out)
{
    if(!identifier(id,sizeof out->id)||(version&&!version_valid(version)))return -1;
    const P4PackageInfo *match=NULL;
    for(size_t i=0;i<p4_packages_count();i++){
        if(!p4_package_provider_selected(p4_packages_provider(i)))continue;
        const P4PackageInfo *info=p4_packages_info(i);
        if(strcmp(info->id,id))continue;
        if(match)return -1;
        match=info;
    }
    if(!match||(version&&strcmp(version,match->version)))return -1;
    memset(out,0,sizeof *out);out->kind=(uint32_t)match->kind;
    memcpy(out->id,match->id,strlen(match->id)+1);memcpy(out->version,match->version,strlen(match->version)+1);
    return 0;
}
static int package_provider(const P4PackageProvider *provider,Dependency *out)
{
    if(!p4_package_provider_selected(provider))return -1;
    const P4PackageInfo *info=p4_package_provider_info(provider);
    memset(out,0,sizeof *out);out->kind=(uint32_t)info->kind;
    memcpy(out->id,info->id,strlen(info->id)+1);memcpy(out->version,info->version,strlen(info->version)+1);
    return 0;
}
static int package_matches(const Dependency *d)
{
    for(size_t i=0;i<p4_packages_count();i++){
        if(!p4_package_provider_selected(p4_packages_provider(i)))continue;
        const P4PackageInfo *info=p4_packages_info(i);
        if((uint32_t)info->kind==d->kind&&!strcmp(info->id,d->id)&&!strcmp(info->version,d->version))return 1;
    }
    return 0;
}
int p4_mod_save_register(const P4ModSaveProvider *p,P4ModSaveRegistration **out)
{
    if(!out||*out||running||pending_load||pending_capture||notifying||!p||p->struct_size<sizeof *p||p->abi_version!=P4_MOD_SAVE_ABI_VERSION||
       !identifier(p->owner,64)||!identifier(p->key,96)||!p->schema_version||(p->flags&~P4_MOD_SAVE_REQUIRED)||
       p->max_bytes>P4_MOD_SAVE_MAX_BYTES||!p->encode||!p->decode||!p->commit||!p->discard||
       registration_count>=MAX_PROVIDERS)return -1;
    if(find_registration(p->owner,p->key))return -1;
    /* A newly installed provider cannot overwrite an opaque already-loaded block
     * without decoding it. Initial loading/registration is restart-required. */
    for(size_t i=0;i<active_state.block_count;i++)
        if(!key_compare(active_state.blocks[i].owner,active_state.blocks[i].key,p->owner,p->key))return -1;
    P4ModSaveRegistration *r=calloc(1,sizeof *r);
    if(!r)return -1;
    r->provider=*p;strcpy(r->owner,p->owner);strcpy(r->key,p->key);r->provider.owner=r->owner;r->provider.key=r->key;
    P4ModSaveRegistration **link=&registrations;
    while(*link&&key_compare((*link)->owner,(*link)->key,r->owner,r->key)<0)link=&(*link)->next;
    r->next=*link;*link=r;registration_count++;*out=r;return 0;
}
int p4_mod_save_unregister(P4ModSaveRegistration *r)
{
    if(!r||running||pending_load||pending_capture||notifying||r->notify_pending||r->published_pending)return -1;
    P4ModSaveRegistration **link=&registrations;
    while(*link&&*link!=r)link=&(*link)->next;
    if(!*link)return -1;
    *link=r->next;registration_count--;free(r);return 0;
}
int p4_mod_save_set_notify(P4ModSaveRegistration *r,void (*notify)(void *))
{
    if(!r||!notify||running||pending_load||pending_capture||notifying||
       find_registration(r->owner,r->key)!=r||r->notify)return -1;
    r->notify=notify;return 0;
}
int p4_mod_save_set_published(P4ModSaveRegistration *r,void (*published)(void *))
{
    if(!r||!published||running||pending_load||pending_capture||notifying||
       find_registration(r->owner,r->key)!=r||r->published)return -1;
    r->published=published;return 0;
}
int p4_mod_save_has_providers(void){return registration_count!=0;}
int p4_mod_save_needs_codec(void)
{
    return registration_count||active_state.block_count||active_state.history.count||
        touches.count||active_state.witness||witness_run||p4_mod_witness_enabled();
}
void p4_mod_save_witness_source(void){witness_run=1;}
int p4_mod_save_touch(const P4PackageProvider *provider)
{
    Dependency d;
    if(running||package_provider(provider,&d))return -1;
    return dependency_add(&touches,&d,1);
}
static int begin_block(P4ModSaveWriter *writer)
{
    if(writer->started)return 0;
    CaptureBuilder *c=writer->capture;
    P4ModSaveProvider *p=&writer->registration->provider;
    if(c->block_count>=MAX_PROVIDERS)return -1;
    if(!c->bytes.size&&zeros(&c->bytes,HEADER_SIZE))return -1;
    writer->header=c->bytes.size;
    unsigned char header[BLOCK_HEADER_SIZE]={0};
    size_t owner=strlen(p->owner),key=strlen(p->key);
    put16(header,(uint16_t)owner);put16(header+2,(uint16_t)key);put32(header+4,p->schema_version);put32(header+8,p->flags);
    if(append(&c->bytes,header,sizeof header)||append(&c->bytes,p->owner,owner)||append(&c->bytes,p->key,key))return -1;
    writer->payload=c->bytes.size;writer->block=c->block_count++;writer->started=1;return 0;
}
int p4_mod_save_write(P4ModSaveWriter *writer,const void *data,size_t size)
{
    if(!writer||writer->failed||!running)return -1;
    if(begin_block(writer)||size>writer->registration->provider.max_bytes-(writer->capture->bytes.size-writer->payload)||append(&writer->capture->bytes,data,size)){
        writer->failed=1;return context_fail(&writer->capture->context,"Save block payload exceeds its declared bound or cannot be allocated");
    }
    return 0;
}
int p4_mod_save_capture_stage(P4ModSaveWriter *writer,void *staged)
{
    if(!writer||writer->failed||!running)return -1;
    CaptureBuilder *capture=writer->capture;
    if(writer->staged||capture->stage_count>=MAX_PROVIDERS){
        writer->failed=1;return context_fail(&capture->context,"Invalid native capture participant");
    }
    if(capture->stage_count==capture->stage_capacity){
        size_t capacity=capture->stage_capacity?capture->stage_capacity*2:8;
        if(capacity>MAX_PROVIDERS)capacity=MAX_PROVIDERS;
        P4ModSaveCaptureStage *stages=realloc(capture->stages,capacity*sizeof *stages);
        if(!stages){writer->failed=1;return context_fail(&capture->context,"Cannot allocate native capture participants");}
        capture->stages=stages;capture->stage_capacity=capacity;
    }
    capture->stages[capture->stage_count++]=(P4ModSaveCaptureStage){writer->registration,staged};
    writer->staged=1;return 0;
}
static int require_dependency(P4ModSaveWriter *writer,Dependency *d,int required)
{
    if(begin_block(writer)){writer->failed=1;return context_fail(&writer->capture->context,"Cannot allocate save requirement block");}
    d->block=writer->block;d->required=required?1u:0u;
    if(dependency_add(&writer->capture->requirements,d,0)){writer->failed=1;return context_fail(&writer->capture->context,"Conflicting or excessive save package requirements");}
    d->block=0;d->required=0;
    if(dependency_add(&writer->capture->history,d,1)){writer->failed=1;return context_fail(&writer->capture->context,"Save provenance limit exhausted");}
    return 0;
}
int p4_mod_save_require_mod(P4ModSaveWriter *writer,const char *id,const char *version,int required)
{
    if(!writer||writer->failed||!running)return -1;
    Dependency d;
    if(package_lookup(id,version,&d)){
        writer->failed=1;return fail(writer->capture->context.error,writer->capture->context.error_capacity,
            "Save block requires an unavailable/ambiguous package or wrong version: %s",id?id:"(null)");
    }
    return require_dependency(writer,&d,required);
}
int p4_mod_save_require_provider(P4ModSaveWriter *writer,const P4PackageProvider *provider,int required)
{
    if(!writer||writer->failed||!running)return -1;
    Dependency d;
    if(package_provider(provider,&d)){writer->failed=1;return context_fail(&writer->capture->context,"Save dependency is not an actual current package provider");}
    return require_dependency(writer,&d,required);
}
int p4_mod_save_read(P4ModSaveReader *reader,void *data,size_t size)
{
    if(!reader||reader->failed||size>reader->size-reader->position||(!data&&size)){if(reader)reader->failed=1;return -1;}
    if(size)memcpy(data,reader->data+reader->position,size);
    reader->position+=size;return 0;
}
uint64_t p4_mod_save_remaining(const P4ModSaveReader *reader){return reader&&!reader->failed?(uint64_t)(reader->size-reader->position):0;}
int p4_mod_save_write_u32(P4ModSaveWriter *w,uint32_t n){unsigned char b[4];put32(b,n);return p4_mod_save_write(w,b,sizeof b);}
int p4_mod_save_write_u64(P4ModSaveWriter *w,uint64_t n){unsigned char b[8];put64(b,n);return p4_mod_save_write(w,b,sizeof b);}
int p4_mod_save_read_u32(P4ModSaveReader *r,uint32_t *n){unsigned char b[4];if(!n){if(r)r->failed=1;return -1;}if(p4_mod_save_read(r,b,sizeof b))return -1;*n=get32(b);return 0;}
int p4_mod_save_read_u64(P4ModSaveReader *r,uint64_t *n){unsigned char b[8];if(!n){if(r)r->failed=1;return -1;}if(p4_mod_save_read(r,b,sizeof b))return -1;*n=get64(b);return 0;}
static int append_saved_block(CaptureBuilder *c,const SavedBlock *block,uint32_t old_index)
{
    if(block->flags&P4_MOD_SAVE_REQUIRED)return context_fail(&c->context,"A required loaded save provider was removed; save is refused");
    P4ModSaveRegistration temporary={0};
    temporary.provider.owner=block->owner;temporary.provider.key=block->key;temporary.provider.schema_version=block->schema;temporary.provider.flags=block->flags;temporary.provider.max_bytes=block->size;
    P4ModSaveWriter writer={.capture=c,.registration=&temporary};
    if(p4_mod_save_write(&writer,block->data,block->size))return -1;
    put64(c->bytes.data+writer.header+16,block->size);
    for(size_t i=0;i<active_state.requirements.count;i++){
        Dependency d=active_state.requirements.data[i];
        if(d.block!=old_index)continue;
        d.block=writer.block;
        if(dependency_add(&c->requirements,&d,0))return context_fail(&c->context,"Cannot retain opaque save requirements");
    }
    return 0;
}
static int append_dependencies(Bytes *bytes,Dependencies *deps,int history)
{
    if(deps->count>1)qsort(deps->data,deps->count,sizeof *deps->data,dependency_compare);
    for(size_t i=0;i<deps->count;i++){
        const Dependency *d=&deps->data[i];
        unsigned char header[DEP_HEADER_SIZE]={0};
        size_t id=strlen(d->id),version=strlen(d->version),prefix=history?HISTORY_HEADER_SIZE:DEP_HEADER_SIZE;
        if(history){put32(header,d->kind);put16(header+4,(uint16_t)id);put16(header+6,(uint16_t)version);}
        else{put32(header,d->block);put32(header+4,d->kind);put16(header+8,(uint16_t)id);put16(header+10,(uint16_t)version);put32(header+12,d->required);}
        if(append(bytes,header,prefix)||append(bytes,d->id,id)||append(bytes,d->version,version))return -1;
    }
    return 0;
}
int p4_mod_save_capture_image(const P4ModSaveImage *image,P4ModSaveCapture **out,char *error,size_t cap)
{
    if(!out||*out||running||pending_load||pending_capture||notifying||!image||!image->data)return fail(error,cap,"Invalid or busy mod-save capture");
    if(!p4_mod_save_needs_codec())return 0;
    if(p4save_image_validate(image->data,image->size,error,cap))return -1;
    running=1;
    CaptureBuilder c={.context={.image=*image,.error=error,.error_capacity=cap}};
    int result=-1;
    for(size_t i=0;i<active_state.history.count;i++)if(dependency_add(&c.history,&active_state.history.data[i],1))goto allocation;
    for(size_t i=0;i<touches.count;i++)if(dependency_add(&c.history,&touches.data[i],1))goto allocation;
    P4ModSaveRegistration *r=registrations;
    size_t old=0;
    for(P4ModSaveRegistration *a=registrations;a;a=a->next){
        if(!a->provider.authorize)continue;
        clear_error(&c.context);
        int authorized=a->provider.authorize(&c.context,a->provider.user);
        if(authorized){callback_failure(&c.context,a,"authorize",authorized);goto done;}
    }
    while(r||old<active_state.block_count){
        SavedBlock *block=old<active_state.block_count?&active_state.blocks[old]:NULL;
        int order=!r?1:!block?-1:key_compare(r->owner,r->key,block->owner,block->key);
        if(order>0){if(append_saved_block(&c,block,(uint32_t)old))goto done;old++;continue;}
        P4ModSaveWriter writer={.capture=&c,.registration=r};
        clear_error(&c.context);
        int encoded=r->provider.encode(&c.context,&writer,r->provider.user);
        if(encoded==1&&!writer.started&&!writer.failed){if(!order)old++;r=r->next;continue;}
        if(encoded||writer.failed){callback_failure(&c.context,r,"encode",encoded?encoded:-1);goto done;}
        if(begin_block(&writer))goto allocation;
        put64(c.bytes.data+writer.header+16,c.bytes.size-writer.payload);
        if(!order)old++;
        r=r->next;
    }
    int witness=p4_mod_witness_enabled()||active_state.witness||witness_run;
    if(!c.block_count&&!c.history.count&&!witness){
        if(!c.stage_count){result=0;goto done;}
        P4ModSaveCapture *capture=calloc(1,sizeof *capture);
        if(!capture)goto allocation;
        capture->stages=c.stages;capture->stage_count=c.stage_count;
        c.stages=NULL;c.stage_count=0;pending_capture=capture;
        *out=capture;result=0;goto done;
    }
    if(!c.bytes.size&&zeros(&c.bytes,HEADER_SIZE))goto allocation;
    if(append_dependencies(&c.bytes,&c.requirements,0)||append_dependencies(&c.bytes,&c.history,1)||zeros(&c.bytes,TRAILER_SIZE))goto allocation;
    unsigned char *h=c.bytes.data;
    memcpy(h,magic,sizeof magic);put32(h+8,1);put32(h+12,HEADER_SIZE);put64(h+16,image->size);p4_save_hash(image->data,image->size,h+24);
    put32(h+56,c.block_count);put32(h+60,(uint32_t)c.requirements.count);put32(h+64,(uint32_t)c.history.count);put64(h+72,c.bytes.size-HEADER_SIZE-TRAILER_SIZE);
    put32(h+68,witness?P4_MOD_SAVE_WITNESS:0);
    p4_save_hash(c.bytes.data,c.bytes.size-TRAILER_SIZE,c.bytes.data+c.bytes.size-TRAILER_SIZE);
    P4ModSaveCapture *capture=calloc(1,sizeof *capture);
    if(!capture)goto allocation;
    capture->retail_size=image->size;memcpy(capture->retail_hash,h+24,32);capture->sidecar=c.bytes.data;capture->sidecar_size=c.bytes.size;
    capture->witness=(get32(h+68)&P4_MOD_SAVE_WITNESS)!=0;
    p4_save_hash(capture->sidecar,capture->sidecar_size,capture->sidecar_hash);
    c.bytes.data=NULL;capture->stages=c.stages;capture->stage_count=c.stage_count;
    c.stages=NULL;c.stage_count=0;
    if(capture->stage_count)pending_capture=capture;
    *out=capture;result=0;goto done;
allocation:
    fail(error,cap,"Save sidecar/provenance budget exhausted");
done:
    for(size_t i=c.stage_count;i;i--){
        P4ModSaveCaptureStage *stage=&c.stages[i-1];
        stage->registration->provider.discard(stage->value,stage->registration->provider.user);
    }
    free(c.stages);free(c.bytes.data);dependencies_free(&c.requirements);dependencies_free(&c.history);running=0;return result;
}
void p4_mod_save_capture_free(P4ModSaveCapture *capture)
{
    if(!capture)return;
    if(capture->stage_count){
        if(running||pending_capture!=capture){fprintf(stderr,"p4pc: invalid native save-capture discard\n");abort();}
        running=1;
        for(size_t i=capture->stage_count;i;i--){
            P4ModSaveCaptureStage *stage=&capture->stages[i-1];
            stage->registration->provider.discard(stage->value,stage->registration->provider.user);
        }
        pending_capture=NULL;running=0;
    }
    free(capture->stages);free(capture->sidecar);free(capture);
}
void p4_mod_save_capture_commit(P4ModSaveCapture *capture)
{
    if(!capture)return;
    if(capture->stage_count){
        if(running||pending_capture!=capture){fprintf(stderr,"p4pc: invalid native save-capture commit\n");abort();}
        running=1;
        for(size_t i=0;i<capture->stage_count;i++){
            P4ModSaveCaptureStage *stage=&capture->stages[i];
            /* Only an emitted block is a publication of this provider's state. */
            if(stage->value&&stage->registration->published)stage->registration->published_pending=1;
            stage->registration->provider.commit(stage->value,stage->registration->provider.user);
        }
        pending_capture=NULL;capture->stage_count=0;running=0;
    }
    p4_mod_save_capture_free(capture);
}
int p4_mod_save_published_pending(void)
{
    for(P4ModSaveRegistration *r=registrations;r;r=r->next)if(r->published_pending)return 1;
    return 0;
}
void p4_mod_save_published_notify(void)
{
    if(running||pending_capture||pending_load||notifying)return; /* Retried at the next safe point. */
    notifying=1;
    for(P4ModSaveRegistration *r=registrations;r;r=r->next){
        if(r->published_pending){r->published_pending=0;r->published(r->provider.user);}
    }
    notifying=0;
}
static int parse_string(P4ModSaveReader *reader,char *out,size_t capacity,size_t length,int version)
{
    if(!length||length>=capacity||p4_mod_save_read(reader,out,length))return -1;
    out[length]=0;
    if(memchr(out,0,length))return -1;
    return version?!version_valid(out):!identifier(out,capacity);
}
static int parse_dependencies(P4ModSaveReader *reader,Dependencies *out,uint32_t count,uint32_t blocks,int history)
{
    if(count>(history?MAX_HISTORY:MAX_REQUIREMENTS))return -1;
    if(count&&reserve((void **)&out->data,&out->capacity,count,sizeof *out->data,history?MAX_HISTORY:MAX_REQUIREMENTS))return -1;
    for(uint32_t i=0;i<count;i++){
        unsigned char h[DEP_HEADER_SIZE];Dependency d={0};
        if(p4_mod_save_read(reader,h,history?HISTORY_HEADER_SIZE:DEP_HEADER_SIZE))return -1;
        uint16_t id,version;
        if(history){d.kind=get32(h);id=get16(h+4);version=get16(h+6);if(get64(h+8))return -1;}
        else{d.block=get32(h);d.kind=get32(h+4);id=get16(h+8);version=get16(h+10);d.required=get32(h+12);if(d.block>=blocks||d.required>1||get32(h+16))return -1;}
        if((d.kind!=P4_PACKAGE_MOD&&d.kind!=P4_PACKAGE_ENGINE_PACK)||parse_string(reader,d.id,sizeof d.id,id,0)||parse_string(reader,d.version,sizeof d.version,version,1))return -1;
        if(i&&dependency_compare(&out->data[i-1],&d)>=0)return -1;
        if(!history&&i){const Dependency *previous=&out->data[i-1];if(previous->block==d.block&&previous->kind==d.kind&&!strcmp(previous->id,d.id))return -1;}
        out->data[out->count++]=d;
    }
    return 0;
}
static int parse_sidecar(P4ModSaveLoad *load,unsigned char *data,size_t size,char *error,size_t cap)
{
    load->state.blob=data;
    if(p4_save_sidecar_verify(load->retail_size,load->retail_hash,data,size,error,cap))return -1;
    load->state.witness=(get32(data+68)&P4_MOD_SAVE_WITNESS)!=0;
    uint32_t count=get32(data+56);
    if(count){load->state.blocks=calloc(count,sizeof *load->state.blocks);if(!load->state.blocks)return fail(error,cap,"Cannot allocate save block index");}
    P4ModSaveReader reader={.data=data+HEADER_SIZE,.size=size-HEADER_SIZE-TRAILER_SIZE};
    for(uint32_t i=0;i<count;i++){
        unsigned char h[BLOCK_HEADER_SIZE];
        if(p4_mod_save_read(&reader,h,sizeof h))return fail(error,cap,"Truncated save block header");
        SavedBlock *block=&load->state.blocks[i];
        block->schema=get32(h+4);block->flags=get32(h+8);uint64_t length=get64(h+16);
        if(!block->schema||(block->flags&~P4_MOD_SAVE_REQUIRED)||get32(h+12)||get64(h+24)||
           parse_string(&reader,block->owner,sizeof block->owner,get16(h),0)||parse_string(&reader,block->key,sizeof block->key,get16(h+2),0)||length>p4_mod_save_remaining(&reader)||length>SIZE_MAX)
            return fail(error,cap,"Invalid save block identity/schema/length");
        if(i&&key_compare(load->state.blocks[i-1].owner,load->state.blocks[i-1].key,block->owner,block->key)>=0)return fail(error,cap,"Duplicate or noncanonical save block key");
        block->data=reader.data+reader.position;block->size=(size_t)length;reader.position+=block->size;load->state.block_count++;
        P4ModSaveRegistration *r=find_registration(block->owner,block->key);
        if(!r&&(block->flags&P4_MOD_SAVE_REQUIRED))return fail(error,cap,"Missing required save provider %s:%s",block->owner,block->key);
        if(r&&block->size>r->provider.max_bytes)return fail(error,cap,"Save block %s:%s exceeds provider bound",block->owner,block->key);
        if(!r){
            char message[384];
            snprintf(message,sizeof message,"Optional extension %s:%s schema%u is unavailable. Its bytes will be preserved, not applied.",
                block->owner,block->key,block->schema);
            if(append_warning(load,message))return fail(error,cap,"Cannot retain complete optional-extension warning");
        }
    }
    if(parse_dependencies(&reader,&load->state.requirements,get32(data+60),count,0)||parse_dependencies(&reader,&load->state.history,get32(data+64),count,1)||p4_mod_save_remaining(&reader))
        return fail(error,cap,"Invalid save requirement/provenance stream");
    for(size_t i=0;i<load->state.requirements.count;i++){
        const Dependency *d=&load->state.requirements.data[i];
        if(d->required&&!package_matches(d))return fail(error,cap,"Save requires package kind%u %s version%s",d->kind,d->id,d->version);
    }
    for(size_t i=0;i<load->state.history.count;i++){
        const Dependency *dependency=&load->state.history.data[i];
        if(package_matches(dependency))continue;
        char message[256];
        snprintf(message,sizeof message,"%s %s version%s is unavailable/different. This save records its historical use.",
            dependency->kind==P4_PACKAGE_ENGINE_PACK?"Engine pack":"Mod",dependency->id,dependency->version);
        if(append_warning(load,message))return fail(error,cap,"Cannot retain complete missing-package warning");
    }
    return 0;
}
static void discard_load(P4ModSaveLoad *load,int committed)
{
    if(!load)return;
    if(!committed)for(size_t i=load->stage_count;i;i--){LoadStage *s=&load->stages[i-1];if(s->decoded)s->registration->provider.discard(s->value,s->registration->provider.user);}
    state_free(&load->state);free(load->warning.data);free(load->stages);free(load);
}
int p4_mod_save_stage_image(const P4ModSaveImage *image,unsigned char *sidecar,size_t size,P4ModSaveLoad **out,char *error,size_t cap)
{
    if(!out||*out||running||pending_load||pending_capture||notifying||!image||(!sidecar&&size)||(!image->data&&image->size)){
        free(sidecar);return fail(error,cap,"Invalid or busy mod-save load staging");
    }
    if(!sidecar&&!p4_mod_save_needs_codec())return 0;
    if(image->data&&p4save_image_validate(image->data,image->size,error,cap)){free(sidecar);return -1;}
    if(sidecar&&!image->data){free(sidecar);return fail(error,cap,"A sidecar needs its complete retail image");}
    P4ModSaveLoad *load=calloc(1,sizeof *load);
    if(!load){free(sidecar);return fail(error,cap,"Cannot allocate save load transaction");}
    load->context=(P4ModSaveContext){.image=*image,.load=load,.error=error,.error_capacity=cap};
    load->retail_size=image->size;
    if(image->data)p4_save_hash(image->data,image->size,load->retail_hash);
    load->phase=LOAD_DECODE;running=1;
    if(sidecar&&parse_sidecar(load,sidecar,size,error,cap))goto bad;
    if(witness_run||p4_mod_witness_enabled())load->state.witness=1;
    if(registration_count){load->stages=calloc(registration_count,sizeof *load->stages);if(!load->stages){fail(error,cap,"Cannot allocate native save staging");goto bad;}}
    size_t block_index=0;
    for(P4ModSaveRegistration *r=registrations;r;r=r->next){
        while(block_index<load->state.block_count&&key_compare(load->state.blocks[block_index].owner,load->state.blocks[block_index].key,r->owner,r->key)<0)block_index++;
        SavedBlock *block=block_index<load->state.block_count&&!key_compare(load->state.blocks[block_index].owner,load->state.blocks[block_index].key,r->owner,r->key)?&load->state.blocks[block_index]:NULL;
        P4ModSaveReader reader={.data=block?block->data:NULL,.size=block?block->size:0};
        LoadStage *stage=&load->stages[load->stage_count++];stage->registration=r;stage->decoded=1;
        clear_error(&load->context);
        int result=r->provider.decode(&load->context,block?block->schema:0,block?&reader:NULL,&stage->value,r->provider.user);
        if(result||reader.failed||reader.position!=reader.size||load->warning_failed){callback_failure(&load->context,r,"decode",result?result:-1);goto bad;}
    }
    load->phase=LOAD_VALIDATE;
    for(size_t i=0;i<load->stage_count;i++){
        LoadStage *stage=&load->stages[i];P4ModSaveProvider *p=&stage->registration->provider;
        if(p->validate){clear_error(&load->context);int result=p->validate(&load->context,stage->value,p->user);if(result||load->warning_failed){callback_failure(&load->context,stage->registration,"validate",result?result:-1);goto bad;}}
    }
    /* The encoded image is borrowed only through decode/validate. Restore later
     * proves the exact image with the retained digest, without a second copy. */
    load->context.image=(P4ModSaveImage){0};load->context.error=NULL;load->context.error_capacity=0;
    load->phase=LOAD_READY;running=0;pending_load=load;*out=load;return 0;
bad:
    discard_load(load,0);running=0;return -1;
}
int p4_mod_save_staged(const P4ModSaveContext *context,const char *owner,const char *key,const void **out)
{
    if(!out)return -1;*out=NULL;
    if(!context||!context->load||context->load->phase!=LOAD_VALIDATE||!running||!owner||!key)return -1;
    const P4ModSaveLoad *load=context->load;
    for(size_t i=0;i<load->stage_count;i++){
        const LoadStage *stage=&load->stages[i];
        if(!key_compare(stage->registration->owner,stage->registration->key,owner,key)){*out=stage->value;return 0;}
    }
    return 1;
}
const char *p4_mod_save_load_warning(const P4ModSaveLoad *load){return load&&load->warning.size?(const char *)load->warning.data:NULL;}
void p4_mod_save_load_abort(P4ModSaveLoad *load)
{
    if(!load)return;
    if(running||pending_load!=load){fprintf(stderr,"p4pc: invalid native save-load abort\n");abort();}
    pending_load=NULL;running=1;discard_load(load,0);running=0;
}
void p4_mod_save_load_commit(P4ModSaveLoad *load)
{
    if(!load)return;
    if(running||pending_load!=load||load->phase!=LOAD_READY){fprintf(stderr,"p4pc: invalid native save-load commit\n");abort();}
    running=1;
    for(size_t i=0;i<load->stage_count;i++){
        LoadStage *stage=&load->stages[i];
        stage->registration->provider.commit(stage->value,stage->registration->provider.user);
        if(stage->registration->notify)stage->registration->notify_pending=1;
        stage->decoded=0;
    }
    state_free(&active_state);active_state=load->state;memset(&load->state,0,sizeof load->state);
    if(active_state.witness)witness_run=1;
    pending_load=NULL;discard_load(load,1);running=0;
}
void p4_mod_save_load_notify(void)
{
    if(running||pending_load||notifying){fprintf(stderr,"p4pc: invalid native save notification\n");abort();}
    notifying=1;
    for(P4ModSaveRegistration *r=registrations;r;r=r->next){
        if(r->notify_pending){r->notify_pending=0;r->notify(r->provider.user);}
    }
    notifying=0;
}
int p4_mod_save_load_matches(const P4ModSaveLoad *load,int32_t version,const void *payload,size_t size)
{
    if(!load||load!=pending_load||load->phase!=LOAD_READY||!payload||size>SIZE_MAX-4||load->retail_size!=size+4)return 0;
    unsigned char prefix[4],digest[32];put32(prefix,(uint32_t)version);
    P4SaveHash hash;p4_save_hash_init(&hash);p4_save_hash_update(&hash,prefix,sizeof prefix);p4_save_hash_update(&hash,payload,size);p4_save_hash_final(&hash,digest);
    return !memcmp(digest,load->retail_hash,sizeof digest);
}
int p4_mod_save_reset_session(char *error,size_t cap)
{
    if(pending_load)p4_mod_save_load_abort(pending_load);
    P4ModSaveImage empty={0};P4ModSaveLoad *load=NULL;
    if(p4_mod_save_stage_image(&empty,NULL,0,&load,error,cap))return -1;
    p4_mod_save_load_commit(load);p4_mod_save_load_notify();return 0;
}
int p4_mod_save_load_witness(const P4ModSaveLoad *load)
{
    return load&&load==pending_load&&load->state.witness;
}
