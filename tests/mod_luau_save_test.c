/* Actual Luau VM, native save codec and card-generation transport; synthetic image. */
#include "lua_runtime.h"
#include "native_bindings.h"
#include "p4pc/game_passive_grant.h"
#include "guest_test.h"
#include "p4pc/image.h"
#include "p4pc/mods.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_failure.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "p4pc/tables.h"
#include "../src/platform/memcard/mod_save_internal.h"
#include "../src/platform/memcard/mod_save_hash.h"
#include "../src/platform/memcard/mod_save_transport.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t mod_test_unsigned(uint32_t);
int16_t mod_test_narrow(int8_t, uint16_t);
float mod_test_mixed(int,float,unsigned char *,double);
uint32_t *mod_test_pointer(uint32_t *);
int mod_test_recursive(int);
void mod_test_void(uint32_t *);
void mod_test_set_action_code(unsigned char *, unsigned char *, uint16_t);
const p4pc_native_t p4pc_native_table[] = {
    {0x100100,(void *)mod_test_unsigned,"mod_test_unsigned",NULL},
    {0x100120,(void *)mod_test_narrow,"mod_test_narrow",NULL},
    {0x100140,(void *)mod_test_mixed,"mod_test_mixed",NULL},
    {0x100160,(void *)mod_test_pointer,"mod_test_pointer",NULL},
    {0x100180,(void *)mod_test_recursive,"mod_test_recursive",NULL},
    {0x1001a0,(void *)mod_test_void,"mod_test_void",NULL},
    {0x1de640,(void *)mod_test_set_action_code,"mod_test_set_action_code",NULL}
};
const size_t p4pc_native_count=sizeof p4pc_native_table/sizeof p4pc_native_table[0];
const p4pc_patch_t p4pc_patch_table[]={{0,NULL,NULL}};
const size_t p4pc_patch_count=0;
const p4pc_hostfn_t p4pc_hostfn_table[]={{NULL,NULL}};
const size_t p4pc_hostfn_count=0;
const p4pc_image_guest_range_t *p4pc_image_guest_ranges(size_t *count){*count=0;return NULL;}
int p4pc_heap_guest_span(uint32_t *begin,uint32_t *end){(void)begin;(void)end;return -1;}
int p4pc_heap_guest_host_overlap(uint32_t address,size_t size){(void)address;(void)size;return 0;}
int32_t func_004216d0(void);
#define CHECK(x) do{if(!(x)){fprintf(stderr,"Lua save witness line %d: %s\n",__LINE__,#x);exit(1);}}while(0)
static const char slot[]="BASLUS-21782Save0";
static unsigned char image[4100],icon[964];
static char card[P4_SAVE_PATH_CAP],error[1024];
static unsigned warnings;
static uint32_t native_active;
static unsigned native_commits,native_discards;
static int native_encode_fail,native_validate_fail;
static int native_encode(const P4ModSaveContext *context,P4ModSaveWriter *writer,void *user)
{
    (void)context;(void)user;
    if(native_encode_fail)return -1;
    return native_active?p4_mod_save_write_u32(writer,native_active):1;
}
static int native_decode(const P4ModSaveContext *context,uint32_t schema,P4ModSaveReader *reader,void **out,void *user)
{
    (void)context;(void)user;
    uint32_t *value=calloc(1,sizeof *value);if(!value)return -1;
    *out=value;
    return reader?(schema==1?p4_mod_save_read_u32(reader,value):-1):0;
}
static int native_validate(const P4ModSaveContext *context,void *staged,void *user)
{
    (void)context;(void)staged;(void)user;return native_validate_fail?-1:0;
}
static void native_commit(void *staged,void *user)
{
    (void)user;native_active=*(uint32_t *)staged;native_commits++;free(staged);
}
static void native_discard(void *staged,void *user)
{
    (void)user;native_discards++;free(staged);
}
static int confirm(const char *message){CHECK(message&&strstr(message,"test.luasave"));warnings++;return 1;}
static void expected_failure(size_t owner,const char *reason,void *user)
{
    (void)owner;(void)user;
    _Exit(reason&&strstr(reason,"intentional load callback failure")?42:43);
}
/* The real p4.passive binding (lua_passive.c) over the real grant table. */
static void passive_initialize(lua_State *L){p4_lua_passive_initialize(L);}
static void passive_before_destroy(P4LuaMod *mod){p4_lua_passive_close(mod);}
static const P4LuaNativeBindings passive_bindings={passive_initialize,passive_before_destroy};
static void run(P4LuaMod *mod,const char *source)
{
    int result=p4_lua_compile(mod,source,strlen(source),"@save-fixture");
    if(!result)result=p4_lua_execute(mod);
    if(result)fprintf(stderr,"Lua save fixture: %s\n%s\n",p4_lua_error(mod),source);
    CHECK(!result);
}
static void path(char out[P4_SAVE_PATH_CAP],const char *root,const char *file)
{
    int n=snprintf(out,P4_SAVE_PATH_CAP,"%s/%s",root,file);
    CHECK(n>0&&(size_t)n<P4_SAVE_PATH_CAP);
}
static void put(const char *file,const void *bytes,size_t size)
{
    FILE *f=fopen(file,"wb");CHECK(f);CHECK(fwrite(bytes,1,size,f)==size);CHECK(!fclose(f));
}
static void put_authenticated(const char *sidecar,const char *marker,unsigned char *bytes,size_t size)
{
    unsigned char digest[32],commit[P4_MOD_SAVE_MARKER_BYTES];
    CHECK(size>=P4_MOD_SAVE_HEADER_BYTES+32);
    p4_save_hash(bytes,size-32,bytes+size-32);
    p4_save_hash(bytes,size,digest);
    p4_save_marker_make(sizeof image,bytes+24,size,digest,commit);
    put(sidecar,bytes,size);put(marker,commit,sizeof commit);
}
static unsigned char *read_all(const char *file,size_t *size)
{
    FILE *f=fopen(file,"rb");CHECK(f);CHECK(!fseek(f,0,SEEK_END));long n=ftell(f);CHECK(n>=0);
    CHECK(!fseek(f,0,SEEK_SET));unsigned char *data=malloc(n?(size_t)n:1);CHECK(data);
    CHECK(fread(data,1,(size_t)n,f)==(size_t)n);CHECK(!fclose(f));*size=(size_t)n;return data;
}
static void route(const char *name,char out[P4_SAVE_PATH_CAP])
{
    CHECK(p4_mod_save_transport_route(card,slot,name,out,P4_SAVE_PATH_CAP)==1);
}
static int begin_save(void)
{
    error[0]=0;return p4_mod_save_transport_begin(card,0,image,sizeof image,error,sizeof error);
}
static void finish_save(void)
{
    char dir[P4_SAVE_PATH_CAP],file[P4_SAVE_PATH_CAP];
    route(NULL,dir);p4_os_mkdir(dir);
    static const unsigned char ico[]={1,2,3,4};
    const char *names[]={slot,"icon.ico","icon.sys"};
    const void *bytes[]={image,ico,icon};
    const size_t sizes[]={sizeof image,sizeof ico,sizeof icon};
    for(size_t i=0;i<3;i++){
        route(names[i],file);put(file,bytes[i],sizes[i]);
        error[0]=0;int result=p4_mod_save_transport_written(card,slot,names[i],0,(uint32_t)sizes[i],error,sizeof error);
        if(result)fprintf(stderr,"save publish: %s\n",error);CHECK(!result);
    }
}
static int stage_load(void)
{
    P4SaveTransaction transaction={0};error[0]=0;
    CHECK(!p4_save_transaction_begin(&transaction,card,0,1,0,error,sizeof error));
    int result=p4_mod_save_transport_loaded(&transaction,image,sizeof image,error,sizeof error);
    p4_save_transaction_abort(&transaction);return result;
}
static void commit_load(void)
{
    error[0]=0;CHECK(!p4_mod_save_transport_restore(0,image+4,sizeof image-4,error,sizeof error));
    p4_mod_save_transport_restore_commit();
}
static int witness(int argc,char **argv)
{
    int failure_case=argc==3&&!strcmp(argv[1],"--callback-failure");
    if(argc!=2&&!failure_case)return 2;
    const char *root=argv[failure_case?2:1];
#ifdef P4_GUEST64
    for(size_t i=0;i<p4pc_native_count;i++)p4_fn_register(p4pc_native_table[i].addr,p4pc_native_table[i].fn);
#endif
    CHECK(func_004216d0()==1);
    CHECK(!p4_lua_bind_native(&passive_bindings));
    char mods[P4_SAVE_PATH_CAP],directory[P4_SAVE_PATH_CAP],file[P4_SAVE_PATH_CAP],sidecar[P4_SAVE_PATH_CAP];
    p4_os_mkdir(root);path(card,root,"card");p4_os_mkdir(card);
    path(mods,root,"mods");p4_os_mkdir(mods);path(directory,mods,"test.luasave");p4_os_mkdir(directory);
    path(file,directory,"mod.ini");
    static const char manifest[]="[mod]\nid=test.luasave\nname=Save fixture\nversion=1.0\napi=1.1\npriority=0\norder=0\n";
    put(file,manifest,sizeof manifest-1);path(file,directory,"main.luau");put(file,"-- fixture\n",11);
    CHECK(!p4_mods_stage(mods,"test.luasave")&&!p4_package_registry_publish());
    const P4PackageProvider *provider=p4_packages_provider(0);CHECK(provider);
    P4LuaMod *mod=p4_lua_create(provider);CHECK(mod);
    if(failure_case){
        CHECK(!p4_mod_failure_handler(expected_failure,NULL));
        run(mod,"p4.save.register{schema=1,max_bytes=32,snapshot=function() return nil end,load=function(v) error('intentional load callback failure') end}");
        CHECK(!p4_mod_save_transport_reset(error,sizeof error));
        p4_mod_save_transport_reset_commit();
        return 3; /* A returned post-commit callback must not be treated as success. */
    }
    run(mod,"value=nil; loaded='not loaded'; loads=0; spin=false; mutate=false; operation=nil; scratch=p4.mem.alloc(8); p4.mem.write('u32',scratch,123)\n"
        "p4.save.register{schema=7,max_bytes=512,snapshot=function() if operation then operation() end; if spin then while true do end end; "
        "if mutate then p4.mem.write('u32',scratch,456) end; return value end,"
        "load=function(v) loaded=v; loads+=1 end}");
    image[57]=0x20;image[60]=1;memset(image+65,0xff,4);
    path(directory,card,slot);path(sidecar,directory,P4_MOD_SAVE_SIDECAR);
    CHECK(!begin_save());finish_save();CHECK(!p4_os_is_file_nofollow(sidecar));
    CHECK(!stage_load());run(mod,"assert(loads==0)");commit_load();run(mod,"assert(loads==1 and loaded==nil)");
    CHECK(!begin_save());finish_save();CHECK(!p4_os_is_file_nofollow(sidecar));
    run(mod,"value={ [false]='f', [true]='t', [2.5]='number', ['a\\0b']='binary', nested={1,2,{x=-3.5}}, nan=0/0, inf=1/0 }");
    CHECK(!begin_save());finish_save();CHECK(p4_os_is_file_nofollow(sidecar));
    size_t baseline_size;unsigned char *baseline=read_all(sidecar,&baseline_size);
    run(mod,"value={inf=1/0,nan=0/0,nested={1,2,{x=-3.5}},['a\\0b']='binary',[2.5]='number',[true]='t',[false]='f'}");
    CHECK(!begin_save());finish_save();size_t repeated_size;unsigned char *repeated=read_all(sidecar,&repeated_size);
    CHECK(repeated_size==baseline_size&&!memcmp(repeated,baseline,baseline_size));free(repeated);
    run(mod,"loaded='unchanged'; value=42");CHECK(!stage_load());run(mod,"assert(loaded=='unchanged' and loads==1)");
    p4_mod_save_transport_load_abort();run(mod,"assert(loaded=='unchanged' and loads==1)");
    CHECK(!stage_load());commit_load();
    run(mod,"assert(loads==2 and loaded[false]=='f' and loaded[true]=='t' and loaded[2.5]=='number' and loaded['a\\0b']=='binary' and loaded.nested[3].x==-3.5 and loaded.nan~=loaded.nan and loaded.inf==1/0)");
    CHECK(!begin_save());p4_mod_save_transport_abort();
    CHECK(!begin_save());route(slot,file);put(file,image,sizeof image);
    CHECK(!p4_mod_save_transport_written(card,slot,slot,0,sizeof image,error,sizeof error));
    CHECK(p4_mod_save_transport_written(card,slot,"icon.sys",0,sizeof icon,error,sizeof error)!=0);
    CHECK(!p4_mod_save_transport_saving());CHECK(!stage_load());commit_load();
    run(mod,"assert(loads==3 and loaded.nested[3].x==-3.5)");
    static const char *const invalid[]={
        "value=function() end", "value=coroutine.create(function() end)", "value=buffer.create(1)", "value=scratch",
        "value={}; value.self=value", "value=setmetatable({}, {})", "value={[{}]=1}",
        "value=string.rep('x',600)", "value={}; local t=value; for i=1,40 do t.next={}; t=t.next end",
        "mutate=true", "spin=true",
        "operation=function() p4.mem.alloc(8) end",
        "operation=function() p4.events.on('frame_start',function(e) end) end",
        "operation=function() p4.call(0x100100,'II',1) end",
        "operation=function() p4.input.action('confirm') end",
        "operation=function() p4.log('forbidden') end",
        "operation=function() coroutine.yield() end"};
    for(size_t i=0;i<sizeof invalid/sizeof invalid[0];i++){
        run(mod,invalid[i]);CHECK(begin_save()!=0&&error[0]);
        run(mod,"mutate=false; spin=false; operation=nil; assert(p4.mem.read('u32',scratch)==123)");
        size_t actual_size;unsigned char *actual=read_all(sidecar,&actual_size);
        CHECK(actual_size==baseline_size&&!memcmp(actual,baseline,baseline_size));free(actual);
    }
    /* Authentication failure must not notify or partially activate a value. */
    CHECK(baseline_size);baseline[baseline_size-1]^=1;put(sidecar,baseline,baseline_size);
    CHECK(stage_load()!=0&&error[0]);run(mod,"assert(loads==3)");
    baseline[baseline_size-1]^=1;put(sidecar,baseline,baseline_size);
    /* Hashes are integrity, not trust: recompute both hashes around malformed
     * plain values and prove the bounded decoder still rejects before commit. */
    char marker[P4_SAVE_PATH_CAP];path(marker,directory,P4_MOD_SAVE_COMMIT);
    size_t block=P4_MOD_SAVE_HEADER_BYTES;
    size_t payload=block+32+(size_t)baseline[block]+((size_t)baseline[block+1]<<8)
        +(size_t)baseline[block+2]+((size_t)baseline[block+3]<<8);
    CHECK(payload+15<baseline_size&&!memcmp(baseline+payload,"P4LV",4));
    CHECK(baseline[payload+8]==5&&baseline[payload+13]==1);
    unsigned char *malformed=malloc(baseline_size);CHECK(malformed);
    for(unsigned mode=0;mode<3;mode++){
        memcpy(malformed,baseline,baseline_size);
        if(mode==0)malformed[payload+8]=255;       /* Unknown value tag. */
        if(mode==1)malformed[payload+14]=0;        /* Nil map value. */
        if(mode==2)malformed[payload+13]=2;        /* Duplicate/unsorted key. */
        put_authenticated(sidecar,marker,malformed,baseline_size);
        CHECK(stage_load()!=0&&strstr(error,"invalid bounded Lua snapshot"));
        run(mod,"assert(loads==3)");
    }
    free(malformed);put_authenticated(sidecar,marker,baseline,baseline_size);
    /* A present Lua block binds the actual package and requires its provider. */
    p4_lua_destroy(mod);CHECK(stage_load()!=0&&error[0]);
    CHECK(!p4_mod_save_reset_session(error,sizeof error));
    mod=p4_lua_create(provider);CHECK(mod);
    run(mod,"value=nil; loads=0; p4.save.register{schema=8,max_bytes=1048576,snapshot=function() return value end,load=function(v) loads+=1 end}");
    CHECK(stage_load()!=0&&error[0]);run(mod,"assert(loads==0)");
    run(mod,"value=table.create(33000,1)");
    CHECK(begin_save()!=0&&strstr(error,"node limit"));
    p4_lua_destroy(mod);CHECK(!p4_mod_save_reset_session(error,sizeof error));
    mod=p4_lua_create(provider);CHECK(mod);
    run(mod,"value=nil; loaded=nil; loads=0; p4.save.register{schema=7,max_bytes=512,snapshot=function() return value end,load=function(v) loaded=v; loads+=1 end}");
    CHECK(!stage_load());commit_load();run(mod,"assert(loads==1 and loaded.nested[3].x==-3.5)");
    /* The Lua provider and a real second native provider share one generation:
     * an encode or late validation failure cannot publish either participant. */
    P4ModSaveProvider native={.struct_size=sizeof native,.abi_version=P4_MOD_SAVE_ABI_VERSION,
        .owner="z.fixture",.key="native",.schema_version=1,.max_bytes=4,
        .encode=native_encode,.decode=native_decode,.validate=native_validate,
        .commit=native_commit,.discard=native_discard};
    P4ModSaveRegistration *registration=NULL;CHECK(!p4_mod_save_register(&native,&registration));
    native_active=17;run(mod,"value={answer=55}; loaded='unchanged'");
    CHECK(!begin_save());finish_save();
    size_t combined_size;unsigned char *combined=read_all(sidecar,&combined_size);
    native_active=99;native_encode_fail=1;run(mod,"value=77");
    CHECK(begin_save()!=0&&native_active==99);native_encode_fail=0;
    size_t same_size;unsigned char *same=read_all(sidecar,&same_size);
    CHECK(same_size==combined_size&&!memcmp(same,combined,combined_size));free(same);free(combined);
    native_validate_fail=1;CHECK(stage_load()!=0&&native_active==99&&!native_commits&&native_discards);
    run(mod,"assert(loads==1 and loaded=='unchanged')");
    native_validate_fail=0;CHECK(!stage_load());CHECK(native_active==99&&!native_commits);
    run(mod,"assert(loads==1 and loaded=='unchanged')");commit_load();
    CHECK(native_active==17&&native_commits==1);run(mod,"assert(loads==2 and loaded.answer==55)");
    CHECK(!p4_mod_save_transport_reset(error,sizeof error));CHECK(native_active==17);
    run(mod,"assert(loads==2)");p4_mod_save_transport_reset_commit();
    CHECK(native_active==0&&native_commits==2);run(mod,"assert(loads==3 and loaded==nil)");
    run(mod,"value=nil");CHECK(!begin_save());finish_save();
    /* Current semantic use remains as historical provenance even without a block. */
    CHECK(p4_os_is_file_nofollow(sidecar));
    CHECK(!p4_mod_save_unregister(registration));
    /* p4.passive grants outlive loads and New Game. A mod drops its
     * save-bound grants in its load callback, which runs for a real load
     * ('load') and a session reset ('reset'), and grants again from the loaded
     * value; another owner's grants are never touched. */
    p4_lua_destroy(mod);CHECK(!p4_mod_save_reset_session(error,sizeof error));
    CHECK(!p4_game_passive_count());
    CHECK(p4_game_passive_grant("test.other",0xFFFFu,P4_PASSIVE_PARTY,3,0x1F6)==1);
    mod=p4_lua_create(provider);CHECK(mod);
    run(mod,"trait=true; origins={}\n"
        "p4.save.register{schema=10,max_bytes=32,snapshot=function() return trait end,"
        "load=function(v,o) table.insert(origins,o); p4.passive.clear(); if v==true then p4.passive.grant({party=2},0x1F6) end end}\n"
        "assert(p4.passive.grant({party=2},0x1F6) and p4.passive.grant({persona=0x2a},0x1F7))");
    CHECK(!begin_save());finish_save();
    run(mod,"trait=false; assert(p4.passive.grant({enemy=40},0x1FB))");
    CHECK(p4_game_passive_count()==4);
    CHECK(!stage_load());commit_load();
    run(mod,"assert(origins[1]=='load' and p4.passive.granted({party=2},0x1F6))\n"
        "assert(not p4.passive.granted({persona=0x2a},0x1F7) and not p4.passive.granted({enemy=40},0x1FB))\n"
        "assert(p4.passive.granted({party=3},0x1F6))");
    CHECK(p4_game_passive_count()==2&&p4_game_passive_held(P4_PASSIVE_PARTY,3,0x1F6));
    CHECK(!p4_mod_save_transport_reset(error,sizeof error));p4_mod_save_transport_reset_commit();
    run(mod,"assert(#origins==2 and origins[2]=='reset' and not p4.passive.granted({party=2},0x1F6))");
    CHECK(p4_game_passive_count()==1&&p4_game_passive_held(P4_PASSIVE_PARTY,3,0x1F6));
    run(mod,"assert(p4.passive.grant({party=2},0x1F6))");
    p4_lua_destroy(mod);CHECK(!p4_mod_save_reset_session(error,sizeof error));
    CHECK(p4_game_passive_count()==1&&p4_game_passive_revoke_owner(0xFFFFu)==1);
    mod=p4_lua_create(provider);CHECK(mod);
    /* Optional authorizers run before any slot byte or commit; a Lua error or a
     * nil answer refuses. Publication waits for the scheduler frame boundary. */
    p4_lua_destroy(mod);CHECK(!p4_mod_save_reset_session(error,sizeof error));
    mod=p4_lua_create(provider);CHECK(mod);
    run(mod,"value={run=1}; allow_save=false; reason='NO SAVES LEFT'; allow_load=true; explode=false; published=nil; loaded=nil; origin=nil; authorized=nil\n"
        "p4.save.register{schema=9,max_bytes=128,snapshot=function() return value end,"
        "load=function(v,o) loaded=v; origin=o end,"
        "authorize_save=function() if explode then error('boom') end return allow_save,reason end,"
        "authorize_load=function(v) authorized=v; return allow_load,'RUN IS LOCKED' end,"
        "published=function(v) published=v end}");
    size_t kept_size;unsigned char *kept=read_all(sidecar,&kept_size);
    CHECK(begin_save()!=0&&strstr(error,"NO SAVES LEFT"));
    run(mod,"allow_save=true; explode=true");CHECK(begin_save()!=0&&strstr(error,"boom"));
    run(mod,"explode=false; allow_save=nil");CHECK(begin_save()!=0&&strstr(error,"NO SAVES LEFT"));
    run(mod,"reason=nil");CHECK(begin_save()!=0&&strstr(error,"refused by mod test.luasave"));
    size_t unchanged_size;unsigned char *unchanged=read_all(sidecar,&unchanged_size);
    CHECK(unchanged_size==kept_size&&!memcmp(unchanged,kept,kept_size));free(unchanged);free(kept);
    run(mod,"allow_save=true");CHECK(!begin_save());
    CHECK(!(p4_mod_safe_point&P4_MOD_SAFE_PUBLISHED));finish_save();
    CHECK(p4_mod_safe_point&P4_MOD_SAFE_PUBLISHED);run(mod,"assert(published==nil)");
    p4_mod_event_frame(1);CHECK(!(p4_mod_safe_point&P4_MOD_SAFE_PUBLISHED));
    run(mod,"assert(published.run==1)");
    run(mod,"allow_load=false");CHECK(stage_load()!=0&&strstr(error,"RUN IS LOCKED"));
    run(mod,"assert(authorized.run==1 and loaded==nil)");
    run(mod,"allow_load=true; value={run=2}");CHECK(!stage_load());commit_load();
    run(mod,"assert(loaded.run==1 and origin=='load')");
    run(mod,"allow_load=false; authorized=nil");
    CHECK(!p4_mod_save_transport_reset(error,sizeof error));p4_mod_save_transport_reset_commit();
    run(mod,"assert(loaded==nil and origin=='reset' and authorized==nil)");
    /* An omitted block publishes nothing; restore the earlier no-block generation. */
    run(mod,"value=nil; published=nil");CHECK(!begin_save());finish_save();
    CHECK(!(p4_mod_safe_point&P4_MOD_SAFE_PUBLISHED));p4_mod_event_frame(2);run(mod,"assert(published==nil)");
    p4_lua_destroy(mod);p4_package_registry_retire();p4_mod_save_transport_ui(confirm,NULL);
    CHECK(!stage_load()&&warnings==1);p4_mod_save_transport_load_abort();
    free(baseline);
    puts("Lua save transport: synthetic VM, staged generation and load callbacks passed");return 0;
}
int main(int argc,char **argv)
{
    p4_os_init();p4_os_setenv("P4PC_VIRTUAL_CLOCK","1",1);p4_os_setenv("P4PC_TURBO","0",1);
    return p4t_on_guest_stack(witness,argc,argv);
}
