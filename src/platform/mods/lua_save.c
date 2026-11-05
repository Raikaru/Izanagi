/* Plain-value save snapshots. The native save codec owns transaction ordering;
 * this module never calls Lua or allocates from its commit callbacks. Optional
 * authorize_load/authorize_save run in snapshot mode before any commit, and
 * published runs from the scheduler safe point after a durable save commit. */
#include "lua_runtime.h"
#include "../memcard/mod_save_internal.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_events.h"
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VALUE_MAX_BYTES (1024u * 1024u)
#define VALUE_MAX_NODES 65536u
#define VALUE_MAX_DEPTH 32u
#define VALUE_HEADER 8u
_Static_assert(sizeof(double)==8 && FLT_RADIX==2 && DBL_MANT_DIG==53 && DBL_MAX_EXP==1024,
    "Lua snapshot wire numbers require IEEE binary64");

typedef struct Snapshot {
    struct Snapshot *next;
    size_t size;
    int reference;
    int captured; /* Produced by encode for a save, not decoded from a load. */
    unsigned char bytes[];
} Snapshot;
enum { CB_SNAPSHOT=1, CB_LOAD=2, CB_AUTHORIZE_LOAD=3, CB_AUTHORIZE_SAVE=4, CB_PUBLISHED=5 };
enum { ORIGIN_LOAD=0, ORIGIN_RESET=1 };
typedef struct P4LuaSave {
    P4LuaMod *mod;
    P4ModSaveRegistration *registration;
    Snapshot *current, *retired;
    uint32_t schema;
    size_t maximum;
    int callbacks_ref;
    int authorize_load, authorize_save, published;
    int staged_origin, origin;
} P4LuaSave;
/* One bounded authorization answer: (ok, reason?). Nil/false refuses. */
typedef struct Authorize {
    P4LuaSave *save;
    int callback, reference, ok;
    char reason[512];
} Authorize;
typedef struct Key {
    int type;
    uint64_t bits;
    const char *string;
    size_t length;
} Key;
typedef struct Encode {
    P4LuaSave *save;
    Snapshot *result;
    Key *keys;
    size_t bytes, nodes, keys_count, keys_used, position;
    const void *ancestors[VALUE_MAX_DEPTH];
} Encode;
typedef struct Decode {
    const unsigned char *bytes;
    size_t size, position, nodes;
    int reference;
} Decode;

enum { V_NIL=0, V_FALSE=1, V_TRUE=2, V_NUMBER=3, V_STRING=4, V_TABLE=5 };

static void put32(unsigned char *p,uint32_t n)
{ for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(8*i)); }
static void put64(unsigned char *p,uint64_t n)
{ for(unsigned i=0;i<8;i++)p[i]=(unsigned char)(n>>(8*i)); }
static uint32_t get32(const unsigned char *p)
{ return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static uint64_t get64(const unsigned char *p)
{ return (uint64_t)get32(p)|(uint64_t)get32(p+4)<<32; }
static uint64_t number_bits(double number,int key)
{
    uint64_t bits;
    memcpy(&bits,&number,sizeof bits);
    if(isnan(number))return UINT64_C(0x7ff8000000000000);
    if(key&&number==0.0)return 0;
    return bits;
}
static double bits_number(uint64_t bits)
{
    double number;
    memcpy(&number,&bits,sizeof number);
    return number;
}
static int key_compare(const void *a,const void *b)
{
    const Key *x=a,*y=b;
    if(x->type!=y->type)return x->type<y->type?-1:1;
    if(x->type==V_STRING){
        size_t n=x->length<y->length?x->length:y->length;
        int result=n?memcmp(x->string,y->string,n):0;
        if(result)return result;
        return x->length==y->length?0:x->length<y->length?-1:1;
    }
    return x->bits==y->bits?0:x->bits<y->bits?-1:1;
}
static void need(lua_State *L,Encode *e,size_t amount)
{
    if(amount>e->save->maximum-e->bytes)luaL_error(L,"save snapshot exceeds declared byte limit");
    e->bytes+=amount;
}
static void node(lua_State *L,Encode *e)
{
    if(++e->nodes>VALUE_MAX_NODES)luaL_error(L,"save snapshot exceeds node limit");
}
static Key key_at(lua_State *L,int index)
{
    Key key={0};
    switch(lua_type(L,index)){
    case LUA_TBOOLEAN:key.type=lua_toboolean(L,index)?V_TRUE:V_FALSE;break;
    case LUA_TNUMBER:{
        double n=lua_tonumber(L,index);
        if(isnan(n))luaL_error(L,"save snapshot has a NaN table key");
        key.type=V_NUMBER;key.bits=number_bits(n,1);break;
    }
    case LUA_TSTRING:key.type=V_STRING;key.string=lua_tolstring(L,index,&key.length);break;
    default:luaL_error(L,"save snapshot table keys must be booleans, numbers or strings");
    }
    return key;
}
static void measure(lua_State *L,Encode *e,int index,unsigned depth)
{
    index=lua_absindex(L,index);node(L,e);
    switch(lua_type(L,index)){
    case LUA_TNIL:case LUA_TBOOLEAN:need(L,e,1);return;
    case LUA_TNUMBER:need(L,e,9);return;
    case LUA_TSTRING:{size_t length;lua_tolstring(L,index,&length);if(length>UINT32_MAX)luaL_error(L,"save string is too long");need(L,e,5);need(L,e,length);return;}
    case LUA_TTABLE:break;
    default:luaL_error(L,"save snapshot contains a non-serializable value");
    }
    if(depth>=VALUE_MAX_DEPTH)luaL_error(L,"save snapshot exceeds nesting limit");
    if(lua_getmetatable(L,index)){lua_pop(L,1);luaL_error(L,"save snapshot tables must not have metatables");}
    const void *identity=lua_topointer(L,index);
    for(unsigned i=0;i<depth;i++)if(e->ancestors[i]==identity)luaL_error(L,"save snapshot contains a cycle");
    e->ancestors[depth]=identity;need(L,e,5);
    if(!lua_checkstack(L,3))luaL_error(L,"save snapshot cannot grow interpreter stack");
    lua_pushnil(L);
    while(lua_next(L,index)){
        Key key=key_at(L,-2);node(L,e);++e->keys_count;
        need(L,e,key.type==V_STRING?5:key.type==V_NUMBER?9:1);
        if(key.type==V_STRING){if(key.length>UINT32_MAX)luaL_error(L,"save key is too long");need(L,e,key.length);}
        measure(L,e,-1,depth+1);lua_pop(L,1);
    }
}
static void output_byte(Encode *e,unsigned value){e->result->bytes[e->position++]=(unsigned char)value;}
static void output_key(Encode *e,const Key *key)
{
    output_byte(e,(unsigned)key->type);
    if(key->type==V_NUMBER){put64(e->result->bytes+e->position,key->bits);e->position+=8;}
    else if(key->type==V_STRING){put32(e->result->bytes+e->position,(uint32_t)key->length);e->position+=4;memcpy(e->result->bytes+e->position,key->string,key->length);e->position+=key->length;}
}
static void push_key(lua_State *L,const Key *key)
{
    if(key->type==V_FALSE||key->type==V_TRUE)lua_pushboolean(L,key->type==V_TRUE);
    else if(key->type==V_NUMBER)lua_pushnumber(L,bits_number(key->bits));
    else lua_pushlstring(L,key->string,key->length);
}
static void emit(lua_State *L,Encode *e,int index,unsigned depth)
{
    index=lua_absindex(L,index);
    switch(lua_type(L,index)){
    case LUA_TNIL:output_byte(e,V_NIL);return;
    case LUA_TBOOLEAN:output_byte(e,lua_toboolean(L,index)?V_TRUE:V_FALSE);return;
    case LUA_TNUMBER:output_byte(e,V_NUMBER);put64(e->result->bytes+e->position,number_bits(lua_tonumber(L,index),0));e->position+=8;return;
    case LUA_TSTRING:{size_t n;const char *s=lua_tolstring(L,index,&n);output_byte(e,V_STRING);put32(e->result->bytes+e->position,(uint32_t)n);e->position+=4;memcpy(e->result->bytes+e->position,s,n);e->position+=n;return;}
    default:break; /* measure already validated tables and every descendant. */
    }
    size_t base=e->keys_used,count=0;
    lua_pushnil(L);
    while(lua_next(L,index)){
        e->keys[base+count++]=key_at(L,-2);lua_pop(L,1);
    }
    e->keys_used=base+count;
    if(count>1)qsort(e->keys+base,count,sizeof *e->keys,key_compare);
    output_byte(e,V_TABLE);put32(e->result->bytes+e->position,(uint32_t)count);e->position+=4;
    for(size_t i=0;i<count;i++){
        Key *key=&e->keys[base+i];output_key(e,key);push_key(L,key);lua_rawget(L,index);
        emit(L,e,-1,depth+1);lua_pop(L,1);
    }
    e->keys_used=base;
}
static int snapshot_invoke(lua_State *L)
{
    Encode *e=lua_touserdata(L,1);
    lua_getref(L,e->save->callbacks_ref);
    lua_rawgeti(L,-1,1);lua_remove(L,-2);lua_call(L,0,1);
    if(lua_isnil(L,-1))return 0;
    e->bytes=VALUE_HEADER;measure(L,e,-1,0);
    if(e->keys_count>SIZE_MAX/sizeof *e->keys)luaL_error(L,"save snapshot key budget overflow");
    e->result=malloc(sizeof *e->result+e->bytes);
    if(!e->result)luaL_error(L,"cannot allocate save snapshot");
    *e->result=(Snapshot){.size=e->bytes,.reference=LUA_NOREF,.captured=1};
    if(e->keys_count){e->keys=malloc(e->keys_count*sizeof *e->keys);if(!e->keys)luaL_error(L,"cannot allocate save key index");}
    memcpy(e->result->bytes,"P4LV",4);put32(e->result->bytes+4,1);e->position=VALUE_HEADER;
    emit(L,e,-1,0);
    if(e->position!=e->bytes)luaL_error(L,"save snapshot changed while serializing");
    return 0;
}
static int idle(P4LuaMod *mod)
{
    for(size_t i=0;i<sizeof mod->threads/sizeof mod->threads[0];i++)if(mod->threads[i].frame)return 0;
    return 1;
}
static void dispose(P4LuaSave *save,Snapshot *snapshot)
{
    if(!snapshot)return;
    if(snapshot->reference!=LUA_NOREF&&snapshot->reference!=LUA_REFNIL)lua_unref(save->mod->state,snapshot->reference);
    free(snapshot);
}
static void reclaim(P4LuaSave *save)
{
    while(save->retired){Snapshot *old=save->retired;save->retired=old->next;dispose(save,old);}
}
static void swap(P4LuaSave *save,Snapshot *next)
{
    if(save->current){save->current->next=save->retired;save->retired=save->current;}
    save->current=next;
}
static int encode_snapshot(const P4ModSaveContext *context,P4ModSaveWriter *writer,void *user)
{
    P4LuaSave *save=user;P4LuaMod *mod=save->mod;
    if(!idle(mod)||mod->budget_failed){snprintf(context->error,context->error_capacity,"save snapshot requires a healthy idle interpreter");return -1;}
    reclaim(save);
    int gc=lua_gc(mod->state,LUA_GCISRUNNING,0);if(gc)lua_gc(mod->state,LUA_GCSTOP,0);
    Encode e={.save=save};
    int result=p4_lua_invoke(mod,snapshot_invoke,&e,0,P4_LUA_STARTUP|P4_LUA_SNAPSHOT);
    if(gc)lua_gc(mod->state,LUA_GCRESTART,0);
    free(e.keys);
    if(result){
        if(mod->budget_failed){mod->budget_failed=0;p4_luau_reset_interrupt(mod->vm);}
        snprintf(context->error,context->error_capacity,"snapshot: %s",p4_lua_error(mod));dispose(save,e.result);return -1;
    }
    if(p4_mod_save_capture_stage(writer,e.result)){dispose(save,e.result);return -1;}
    if(!e.result)return 1;
    if(p4_mod_save_require_provider(writer,mod->provider,1)||p4_mod_save_write(writer,e.result->bytes,e.result->size))return -1;
    return 0;
}
static int take(Decode *d,size_t n,const unsigned char **out)
{
    if(n>d->size-d->position)return -1;
    *out=d->bytes+d->position;d->position+=n;return 0;
}
static int authorize_invoke(lua_State *L)
{
    Authorize *a=lua_touserdata(L,1);
    lua_getref(L,a->save->callbacks_ref);
    lua_rawgeti(L,-1,a->callback);lua_remove(L,-2);
    int arguments=0;
    if(a->callback==CB_AUTHORIZE_LOAD){
        if(a->reference!=LUA_NOREF)lua_getref(L,a->reference);else lua_pushnil(L);
        arguments=1;
    }
    lua_call(L,arguments,2);
    a->ok=lua_toboolean(L,-2);
    if(!a->ok){
        size_t length=0;const char *reason=lua_type(L,-1)==LUA_TSTRING?lua_tolstring(L,-1,&length):NULL;
        if(reason&&length)snprintf(a->reason,sizeof a->reason,"%.*s",(int)length,reason);
        else snprintf(a->reason,sizeof a->reason,"refused by mod %s",a->save->mod->info->id);
    }
    return 0;
}
/* Snapshot-mode invocation: no native calls, no guest mutation. A Lua error or
 * an exhausted budget refuses; it never authorizes. 0 authorized, -1 refused. */
static int run_authorize(P4LuaSave *save,int callback,int reference,const P4ModSaveContext *context)
{
    P4LuaMod *mod=save->mod;
    if(!idle(mod)||mod->budget_failed){
        snprintf(context->error,context->error_capacity,"save authorization requires a healthy idle interpreter");return -1;
    }
    Authorize a={.save=save,.callback=callback,.reference=reference};
    int gc=lua_gc(mod->state,LUA_GCISRUNNING,0);if(gc)lua_gc(mod->state,LUA_GCSTOP,0);
    int result=p4_lua_invoke(mod,authorize_invoke,&a,0,P4_LUA_STARTUP|P4_LUA_SNAPSHOT);
    if(gc)lua_gc(mod->state,LUA_GCRESTART,0);
    if(result){
        if(mod->budget_failed){mod->budget_failed=0;p4_luau_reset_interrupt(mod->vm);}
        snprintf(context->error,context->error_capacity,"authorization: %s",p4_lua_error(mod));return -1;
    }
    if(!a.ok){snprintf(context->error,context->error_capacity,"%s",a.reason);return -1;}
    return 0;
}
static int authorize_snapshot(const P4ModSaveContext *context,void *user)
{
    P4LuaSave *save=user;
    return save->authorize_save?run_authorize(save,CB_AUTHORIZE_SAVE,LUA_NOREF,context):0;
}
static int parse_key(Decode *d,Key *key)
{
    const unsigned char *p;
    if(take(d,1,&p))return -1;
    *key=(Key){.type=*p};
    if(key->type==V_FALSE||key->type==V_TRUE)return 0;
    if(key->type==V_NUMBER){
        if(take(d,8,&p))return -1;key->bits=get64(p);
        double number=bits_number(key->bits);
        return isnan(number)||(number==0.0&&key->bits!=0)?-1:0;
    }
    if(key->type==V_STRING){
        if(take(d,4,&p))return -1;key->length=get32(p);
        if(take(d,key->length,&p))return -1;key->string=(const char *)p;return 0;
    }
    return -1;
}
static int validate_value(Decode *d,unsigned depth)
{
    const unsigned char *p;
    if(++d->nodes>VALUE_MAX_NODES||take(d,1,&p))return -1;
    unsigned type=*p;
    if(type<=V_TRUE)return 0;
    if(type==V_NUMBER){
        if(take(d,8,&p))return -1;
        uint64_t bits=get64(p);double number=bits_number(bits);
        return isnan(number)&&bits!=UINT64_C(0x7ff8000000000000)?-1:0;
    }
    if(type==V_STRING){if(take(d,4,&p))return -1;size_t n=get32(p);return take(d,n,&p);}
    if(type!=V_TABLE||depth>=VALUE_MAX_DEPTH||take(d,4,&p))return -1;
    uint32_t count=get32(p);Key previous={0};
    if(count>(VALUE_MAX_NODES-d->nodes)/2)return -1;
    for(uint32_t i=0;i<count;i++){
        Key key;if(++d->nodes>VALUE_MAX_NODES||parse_key(d,&key))return -1;
        if(i&&key_compare(&previous,&key)>=0)return -1;
        previous=key;
        if(d->position>=d->size||d->bytes[d->position]==V_NIL||validate_value(d,depth+1))return -1;
    }
    return 0;
}
static void push_value(lua_State *L,Decode *d,unsigned depth)
{
    const unsigned char *p;
    (void)take(d,1,&p);
    switch(*p){
    case V_NIL:lua_pushnil(L);return;
    case V_FALSE:lua_pushboolean(L,0);return;
    case V_TRUE:lua_pushboolean(L,1);return;
    case V_NUMBER:(void)take(d,8,&p);lua_pushnumber(L,bits_number(get64(p)));return;
    case V_STRING:{(void)take(d,4,&p);size_t n=get32(p);(void)take(d,n,&p);lua_pushlstring(L,(const char *)p,n);return;}
    default:break;
    }
    (void)depth;(void)take(d,4,&p);uint32_t count=get32(p);
    if(!lua_checkstack(L,4))luaL_error(L,"save restore cannot grow interpreter stack");
    lua_createtable(L,0,(int)count);
    for(uint32_t i=0;i<count;i++){
        Key key;(void)parse_key(d,&key);push_key(L,&key);push_value(L,d,depth+1);lua_rawset(L,-3);
    }
}
static int decode_invoke(lua_State *L)
{
    Decode *d=lua_touserdata(L,1);
    push_value(L,d,0);d->reference=lua_ref(L,-1);return 0;
}
static int decode_snapshot(const P4ModSaveContext *context,uint32_t schema,P4ModSaveReader *reader,void **out,void *user)
{
    P4LuaSave *save=user;P4LuaMod *mod=save->mod;
    if(!idle(mod)||mod->budget_failed){
        snprintf(context->error,context->error_capacity,"save restore requires a healthy idle interpreter");return -1;
    }
    /* An empty image is a reset to the New Game defaults, never a player's load. */
    save->staged_origin=context->image.data?ORIGIN_LOAD:ORIGIN_RESET;
    if(!reader){*out=NULL;return 0;}
    uint64_t length=p4_mod_save_remaining(reader);
    if(schema!=save->schema||length<=VALUE_HEADER||length>save->maximum||length>SIZE_MAX-sizeof(Snapshot)){
        snprintf(context->error,context->error_capacity,"unsupported or oversized Lua snapshot schema");return -1;
    }
    Snapshot *snapshot=malloc(sizeof *snapshot+(size_t)length);
    if(!snapshot){snprintf(context->error,context->error_capacity,"cannot allocate Lua snapshot");return -1;}
    *snapshot=(Snapshot){.size=(size_t)length,.reference=LUA_NOREF};*out=snapshot;
    if(p4_mod_save_read(reader,snapshot->bytes,snapshot->size)||memcmp(snapshot->bytes,"P4LV",4)||get32(snapshot->bytes+4)!=1){
        snprintf(context->error,context->error_capacity,"invalid Lua snapshot header");return -1;
    }
    Decode d={.bytes=snapshot->bytes,.size=snapshot->size,.position=VALUE_HEADER,.reference=LUA_NOREF};
    if(snapshot->bytes[VALUE_HEADER]==V_NIL||validate_value(&d,0)||d.position!=d.size){
        snprintf(context->error,context->error_capacity,"invalid bounded Lua snapshot values");return -1;
    }
    d.position=VALUE_HEADER;
    int gc=lua_gc(mod->state,LUA_GCISRUNNING,0);if(gc)lua_gc(mod->state,LUA_GCSTOP,0);
    int status=lua_cpcall(mod->state,decode_invoke,&d);
    if(gc)lua_gc(mod->state,LUA_GCRESTART,0);
    if(status!=LUA_OK){
        const char *message=lua_type(mod->state,-1)==LUA_TSTRING?lua_tostring(mod->state,-1):"interpreter allocation failure";
        snprintf(context->error,context->error_capacity,"cannot stage Lua snapshot: %s",message);
        lua_pop(mod->state,1);return -1;
    }
    snapshot->reference=d.reference;return 0;
}
static int validate_snapshot(const P4ModSaveContext *context,void *staged,void *user)
{
    P4LuaSave *save=user;
    /* New Game/session resets share decode/validate; a refusal there would be a
     * runtime failure, so authorization covers real loads only. */
    if(!save->authorize_load||!context->image.data)return 0;
    Snapshot *snapshot=staged;
    return run_authorize(save,CB_AUTHORIZE_LOAD,snapshot?snapshot->reference:LUA_NOREF,context);
}
static void commit_snapshot(void *staged,void *user)
{
    P4LuaSave *save=user;Snapshot *snapshot=staged;
    swap(save,snapshot);
    if(snapshot&&snapshot->captured){
        if(save->published)p4_mod_safe_point|=P4_MOD_SAFE_PUBLISHED;
    }else save->origin=save->staged_origin;
}
static void discard_snapshot(void *staged,void *user){dispose(user,staged);}
static int notify_invoke(lua_State *L)
{
    P4LuaSave *save=lua_touserdata(L,1);
    lua_getref(L,save->callbacks_ref);
    lua_rawgeti(L,-1,CB_LOAD);lua_remove(L,-2);
    if(save->current&&save->current->reference!=LUA_NOREF)lua_getref(L,save->current->reference);
    else lua_pushnil(L);
    lua_pushstring(L,save->origin==ORIGIN_RESET?"reset":"load");
    lua_call(L,2,0);return 0;
}
static int published_invoke(lua_State *L)
{
    P4LuaSave *save=lua_touserdata(L,1);
    lua_getref(L,save->callbacks_ref);
    lua_rawgeti(L,-1,CB_PUBLISHED);lua_remove(L,-2);
    /* The committed capture is current until a later save/load replaces it. */
    if(save->current&&save->current->captured){
        Decode d={.bytes=save->current->bytes,.size=save->current->size,.position=VALUE_HEADER,.reference=LUA_NOREF};
        push_value(L,&d,0);
    }else lua_pushnil(L);
    lua_call(L,1,0);return 0;
}
static void published_snapshot(void *user)
{
    P4LuaSave *save=user;reclaim(save);
    if(p4_lua_invoke(save->mod,published_invoke,save,0,P4_LUA_SEMANTIC))
        p4_mod_failure(save->mod->owner,p4_lua_error(save->mod));
}
static int published_safe_point(void)
{
    p4_mod_save_published_notify();
    return p4_mod_save_published_pending();
}
static void notify_snapshot(void *user)
{
    P4LuaSave *save=user;reclaim(save);
    /* Default/absent state alone is not semantic package use. Actual guest or
     * native effects are tracked by the ordinary API guards; consumed non-nil
     * state is itself use and must survive a later nil snapshot as history. */
    if(p4_lua_invoke(save->mod,notify_invoke,save,0,save->current?P4_LUA_SEMANTIC:0))
        p4_mod_failure(save->mod->owner,p4_lua_error(save->mod));
}
static int register_save(lua_State *L)
{
    p4_lua_require_mutation(L);
    P4LuaMod *mod=p4_luau_user(L);
    luaL_checktype(L,1,LUA_TTABLE);
    if(mod->save)luaL_error(L,"a mod may register only one save snapshot provider");
    lua_rawgetfield(L,1,"schema");uint32_t schema=(uint32_t)p4_lua_integer(L,-1,1,UINT32_MAX);lua_pop(L,1);
    lua_rawgetfield(L,1,"max_bytes");size_t maximum=(size_t)p4_lua_integer(L,-1,VALUE_HEADER+1,VALUE_MAX_BYTES);lua_pop(L,1);
    static const char *const optional[]={"authorize_load","authorize_save","published"};
    lua_newtable(L);
    lua_rawgetfield(L,1,"snapshot");luaL_checktype(L,-1,LUA_TFUNCTION);lua_rawseti(L,-2,CB_SNAPSHOT);
    lua_rawgetfield(L,1,"load");luaL_checktype(L,-1,LUA_TFUNCTION);lua_rawseti(L,-2,CB_LOAD);
    int present[3]={0};
    for(int i=0;i<3;i++){
        lua_rawgetfield(L,1,optional[i]);
        if(!lua_isnil(L,-1)&&!lua_isfunction(L,-1))luaL_error(L,"p4.save.register %s must be a function or nil",optional[i]);
        present[i]=!lua_isnil(L,-1);
        lua_rawseti(L,-2,CB_AUTHORIZE_LOAD+i);
    }
    int callbacks=lua_ref(L,-1);
    P4LuaSave *save=calloc(1,sizeof *save);
    if(!save){lua_unref(L,callbacks);luaL_error(L,"cannot allocate save snapshot provider");}
    save->mod=mod;save->schema=schema;save->maximum=maximum;save->callbacks_ref=callbacks;
    save->authorize_load=present[0];save->authorize_save=present[1];save->published=present[2];
    P4ModSaveProvider provider={.struct_size=sizeof provider,.abi_version=P4_MOD_SAVE_ABI_VERSION,
        .owner=mod->info->id,.key="lua.value",.schema_version=schema,.flags=P4_MOD_SAVE_REQUIRED,
        .max_bytes=maximum,.user=save,.encode=encode_snapshot,.decode=decode_snapshot,
        .validate=save->authorize_load?validate_snapshot:NULL,
        .commit=commit_snapshot,.discard=discard_snapshot,
        .authorize=save->authorize_save?authorize_snapshot:NULL};
    if(p4_mod_save_register(&provider,&save->registration)){
        lua_unref(L,save->callbacks_ref);free(save);
        luaL_error(L,"cannot register Lua save snapshot provider");
    }
    if(p4_mod_save_set_notify(save->registration,notify_snapshot)||
       (save->published&&p4_mod_save_set_published(save->registration,published_snapshot))){
        p4_mod_save_unregister(save->registration);lua_unref(L,save->callbacks_ref);free(save);
        luaL_error(L,"cannot register Lua save notification");
    }
    if(save->published)p4_mod_events_set_published(published_safe_point);
    mod->save=save;return 0;
}
void p4_lua_save_library(lua_State *L)
{
    static const luaL_Reg methods[]={{"register",register_save},{NULL,NULL}};
    p4_lua_library(L,methods);
}
void p4_lua_save_destroy(P4LuaMod *mod)
{
    P4LuaSave *save=mod->save;
    if(!save)return;
    if(p4_mod_save_unregister(save->registration)){
        fprintf(stderr,"p4pc: mod %s: save provider still has a live transaction\n",mod->info->id);
        fflush(stderr);abort();
    }
    reclaim(save);dispose(save,save->current);
    lua_unref(mod->state,save->callbacks_ref);
    free(save);mod->save=NULL;
}
