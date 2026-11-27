/* Test-only full-game-linked entry. No native command, allocator or result is mocked.
 * A synthetic guest arena supplies battle/task/HUD inputs before game startup;
 * not a gameplay receipt. Content comes from the owned disc and verified Golden install. */
#include "../src/platform/golden/golden_ai_qualification.h"
#include "../src/platform/golden/golden_mode_identity.h"
#include "../src/platform/golden/golden_mode_boot.h"
#include "../src/platform/golden/golden_mode_sources.h"
#include "../src/platform/files/iso9660.h"
#include "p4pc/golden_extra_source.h"
#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/crt.h"
#include "p4pc/platform.h"
#include "p4pc/content_digest.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Same matched checked-heap bootstrap used by ee_heap_retail_test.c. */
extern int32_t (**chkmemInitRenderWareMemoryFunctions(void))(uint32_t);
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"qualification FAIL %d: %s (%s)\n",__LINE__,#x,reason); return 1; } } while(0)
static uint32_t arena=0x01000000;
static size_t native_enemy_rows;
static uint32_t allocate(size_t size)
{
    uint32_t base=arena;
    if(size>0x02000000u-arena) abort();
    arena+=(uint32_t)((size+15)&~(size_t)15);
    memset(P4_G2H(base),0,size); return base;
}
static uint32_t copy(const void *data,size_t size)
{
    uint32_t base=allocate(size); memcpy(P4_G2H(base),data,size); return base;
}
static void bind(uint32_t slot,const void *data,size_t size) { P4_G32(slot)=copy(data,size); }
/* D5: these are the pinned real PS2 banks, not projected/truncated Golden rows. */
typedef struct NativeBank {
    const char *key,*digest;
    size_t size;
    unsigned container,count;
    uint32_t slots[12];
} NativeBank;
/* Native slot order follows game_table_admission.c. Golden BF9/10 are installed
 * separately; the original eight calculation constants remain real PS2 banks. */
static const NativeBank native_banks[]={
    {"INIT/ITEMTBL.BIN","2896f1c4ff31d2ad4c3f357e3ed6fc48ae6a84d904cd9df07c58652139206699",104464,0,0,{0x7642a0}},
    {"BATTLE/SKILL.TBL","cbf031b309f826b5e94344c407eb3281f5a5a410c59268e230dd7f907778356c",18784,1,2,{0x7644a4,0x7644a8}},
    {"BATTLE/UNIT.TBL","88864c47c3c3cf9a7b15bf633a6b62a62fa678069d599d0cf56a673b6325d41c",39152,1,3,{0x7644b4,0x7644b8,0x7644c8}},
    {"BATTLE/PERSONA.TBL","15269fcf6b301afd441d759c335255a7c06f15f301d1af798d229aeccdf961b7",41232,1,6,{0x7644c4,0x7644cc,0x7644d4,0x7644d8,0x7644dc,0x7644e0}},
    {"BATTLE/AICALC.TBL","f46b675b1800acf8f988e5e1cbd3ef306e08cf444f50db47457ca29afdd3d22a",279328,1,11,{0x7644e4,0x7644e8,0x7644ec,0x7644f0,0x7644f4,0x7644f8,0x7644fc,0x764500,0x7644c0}},
    {"BATTLE/EFFECT.TBL","4422aa3d4bbb6fa0797045273674a649adbea6bb70ba20f508d1268ed6577854",1776,1,1,{0x7644ac}},
    {"BATTLE/MODEL.TBL","3f03a072f24c043779a53438d57c83ec10dcadaf629fce241ba356ecc67f6a51",104176,1,3,{0x7644b0,0x7644bc,0x7644d0}},
    {"BATTLE/ENCOUNT.TBL","da1cd20cb00f38222decd4a89f63d0798283cedf050673e1ba16f9e1d7682eb3",63744,1,5,{0x764504,0x76450c,0x764510,0x764508,0x764514}},
    {"BATTLE/MSG.TBL","895098cf6bc05fb5c4a7bce60cefb272e0a9c94a68cf8800de65447666150839",38240,1,5,{0x76453c,0x764538,0x764534,0x764530,0x76452c}}
};
enum { NATIVE_BANKS=sizeof native_banks/sizeof native_banks[0] };
typedef struct DiscIndex {
    long long containers[2],offset[NATIVE_BANKS],size[NATIVE_BANKS];
    unsigned roots,seen[NATIVE_BANKS],container;
} DiscIndex;
static void disc_root(void *context,const char *path,long long offset,long long size)
{
    DiscIndex *index=context; (void)size;
    if(!strcmp(path,"DATA.CVM")) { index->containers[0]=offset; index->roots|=1; }
    if(!strcmp(path,"BTL.CVM")) { index->containers[1]=offset; index->roots|=2; }
}
static void disc_bank(void *context,const char *path,long long offset,long long size)
{
    DiscIndex *index=context;
    for(unsigned i=0;i<NATIVE_BANKS;++i)
        if(native_banks[i].container==index->container && !strcmp(path,native_banks[i].key)) {
            index->offset[i]=offset; index->size[i]=size; ++index->seen[i];
        }
}
static int load_native_banks(const char *path,char *reason,size_t capacity)
{
    FILE *file=fopen(path,"rb");
    if(!file) { snprintf(reason,capacity,"cannot open owned PS2 disc"); return -1; }
    DiscIndex index={0}; int result=-1; uint8_t *data=NULL;
    if(iso_walk(file,0,disc_root,&index) || index.roots!=3) goto done;
    for(unsigned i=0;i<2;++i) {
        char magic[4];
        if(iso_read_at(file,index.containers[i],magic,4) || memcmp(magic,"CVMH",4)) goto done;
        index.container=i;
        if(iso_walk(file,index.containers[i]+0x1800,disc_bank,&index)) goto done;
    }
    for(unsigned i=0;i<NATIVE_BANKS;++i) {
        const NativeBank *bank=native_banks+i;
        if(index.seen[i]!=1 || index.size[i]!=(long long)bank->size) goto done;
        data=malloc(bank->size);
        if(!data || iso_read_at(file,index.offset[i],data,bank->size)) goto done;
        char digest[65]; p4_content_digest(data,bank->size,digest);
        if(strcmp(digest,bank->digest)) goto done;
        if(!bank->count) bind(bank->slots[0],data+16,bank->size-16);
        else {
            P4GoldenSpan spans[12];
            if(p4_golden_tbl_spans(data,bank->size,spans,12)!=(int)bank->count) goto done;
            if(!strcmp(bank->key,"BATTLE/UNIT.TBL")) native_enemy_rows=spans[0].size/60;
            for(unsigned j=0;j<bank->count;++j)
                if(bank->slots[j]) bind(bank->slots[j],spans[j].bytes,spans[j].size);
        }
        free(data); data=NULL;
    }
    result=0;
done:
    free(data); fclose(file);
    if(result) snprintf(reason,capacity,"owned PS2 native bank framing/identity failed");
    return result;
}
enum { PARTY=4, ENEMIES=6, ACTORS=PARTY+ENEMIES };
static uint32_t actors[ACTORS],units[ACTORS],battle,work,local;
static uint16_t initial_hp[ACTORS],initial_sp[ACTORS];
static void actors_init(void)
{
    P4_G16(0x00797f88)=0;
    P4_G16(0x00797f8c)=1; /* datPersonaRosterSlot admits only active records. */
    battle=allocate(0xe88); P4_GPDATA(uint32_t,0xb3ac)=battle;
    p4_golden_ai_native_battle_init();
    /* Synthetic encounter input: slot0 and verified native ENCOUNT0 rule1. */
    uint32_t encounter=allocate(10);
    P4_G16(encounter+8)=1;
    P4_G32(battle+0xc68)=encounter;
    /* Native KwlnTask/BtlHudWork layout inputs; getters and UI writers stay real.
     * No task scheduler, renderer or task-creation receipt is claimed here. */
    uint32_t ui=allocate(0x54),hud=allocate(0x8cc);
    P4_G32(ui+0x1c)=2;
    P4_G32(ui+0x38)=hud;
    P4_G32(battle+0xdd4)=ui;
    uint32_t action_id=P4_GPDATA(uint32_t,0xa0b8);
    printf("qualification roster=%u first_native_action_id=%u\n",ACTORS,action_id);
    for(unsigned id=1;id<=10;++id) {
        uint8_t *persona=datPersonaGetRecord((uint16_t)id);
        memset(persona,0,48); persona[0]=1; persona[2]=1; persona[4]=10; persona[12]=1;
    }
    for(unsigned i=0;i<ACTORS;++i) {
        uint32_t a=actors[i]=allocate(0x458),v=allocate(0xb00),c=allocate(0x30);
        units[i]=v; P4_G32(a)=i+1; P4_G32(a+4)=0; P4_G32(v+0xa8)=i+1;
        if(action_id>=UINT32_C(0x0fffffff)) action_id=1;
        P4_G32(a+8)=action_id++; P4_G16(a+0x1a)=9; P4_G32(a+0x30)=v;
        P4_G32(a+0x450)=i?actors[i-1]:0;
        if(i) P4_G32(actors[i-1]+0x44c)=a;
        if(i && i!=PARTY) {
            P4_G32(units[i-1]+0xa6c)=v; P4_G32(v+0xa68)=units[i-1];
        }
        P4_G32(v+0xa64)=c; P4_G32(v+0x9c)=8;
        P4_G8(v+0xa2)=i<PARTY?0:1; P4_G16(v+0xa4)=i<PARTY?i+1:i-PARTY+1;
        P4_G16(c)=i<PARTY?0:4; P4_G16(c+2)=i<PARTY?i+1:i-PARTY+1;
        P4_G16(c+4)=i<PARTY?1:i-PARTY+1; P4_G8(c+6)=10;
        const P4GoldenAiProduct *product=p4_golden_ai_native_product();
        uint16_t hp,sp;
        if(i<PARTY) {
            if(p4_golden_levels_pair(product->raw[P4_GAI_LEVELS].data,
                product->raw[P4_GAI_LEVELS].size,10,(int)i+1,&hp,&sp)) abort();
        } else {
            const uint8_t *row=P4_G2H(P4_G32(0x7644b4)+(uint32_t)(i-PARTY+1)*60);
            hp=(uint16_t)(row[4]|(uint16_t)row[5]<<8);
            sp=(uint16_t)(row[6]|(uint16_t)row[7]<<8);
        }
        initial_hp[i]=P4_G16(c+8)=hp; initial_sp[i]=P4_G16(c+10)=sp;
        p4_golden_ai_native_action_init(P4_G2H(a));
    }
    P4_G32(battle+0x174)=actors[ACTORS-1]; P4_G32(battle+0x170)=actors[0];
    P4_G32(battle+0x178)=units[0]; P4_G32(battle+0x17c)=units[PARTY-1];
    P4_G32(battle+0x180)=units[PARTY]; P4_G32(battle+0x184)=units[ACTORS-1];
    P4_GPDATA(uint32_t,0xa0b8)=action_id;
    uint32_t pool=allocate(0x18+3*0x168),locals=allocate(0x18+3*0x348);
    P4_G32(0x00764610)=pool; P4_G32(0x00764614)=locals;
    work=pool+0x18; local=locals+0x18;
}
static int lifecycle(void)
{
    P4GoldenAi *view;
    if(p4_golden_ai_native_refresh(actors[0],8472,&view)) return -1;
    if(btlCheckBattleEnd()!=0 || P4_G16(battle+0x1c)!=0) return -1;
    for(unsigned i=PARTY;i<ACTORS;++i) {
        uint32_t calc=P4_G32(P4_G32(actors[i]+0x30)+0xa64);
        P4_G32(calc+0xc)=0x80000; P4_G16(calc+8)=0;
    }
    if(btlCheckBattleEnd()!=1 || P4_G16(battle+0x1c)!=1) return -1;
    for(unsigned i=PARTY;i<ACTORS;++i) {
        uint32_t calc=P4_G32(P4_G32(actors[i]+0x30)+0xa64);
        P4_G32(calc+0xc)=0; P4_G16(calc+8)=initial_hp[i];
    }
    uint32_t leader=P4_G32(P4_G32(actors[0]+0x30)+0xa64);
    P4_G32(leader+0xc)=0x80000; P4_G16(leader+8)=0;
    if(btlCheckBattleEnd()!=1 || P4_G16(battle+0x1c)!=2) return -1;
    P4_G32(leader+0xc)=0; P4_G16(leader+8)=initial_hp[0];
    for(unsigned i=0;i<PARTY;++i) P4_G16(actors[i]+0x1a)=0;
    if(btlCheckBattleEnd()!=1 || P4_G16(battle+0x1c)!=3) return -1;
    for(unsigned i=0;i<PARTY;++i) P4_G16(actors[i]+0x1a)=9;
    if(btlCheckBattleEnd()!=0 || P4_G16(battle+0x1c)!=0) return -1;
    if(view->battle->category_guard) return -1;
    p4_golden_ai_native_eligible_start(P4_G2H(actors[0]));
    p4_golden_ai_native_eligible_start(P4_G2H(actors[PARTY]));
    if(view->battle->category_guard!=2) return -1;
    p4_golden_ai_native_continuous_entry();
    if(view->battle->category_guard) return -1;
    P4_G32(battle+0xc)|=UINT32_C(0x20000000);
    p4_golden_ai_native_bit29(P4_G32(battle+0xc));
    if(p4_golden_ai_native_refresh(actors[0],8474,&view) ||
        !(view->battle->flags&UINT32_C(0x20000000))) return -1;
    P4_G32(battle+0xc)&=~UINT32_C(0x20000000);
    p4_golden_ai_native_bit29(P4_G32(battle+0xc));
    uint8_t packets[32]; memcpy(packets,P4_G2H(battle+0x198),sizeof packets);
    int64_t message=42;
    P4GoldenAiResult result={0};
    if(!p4_golden_ai_execute(P4_GAME_MODE_GOLDEN,view,8481,&message,&result)) return -1;
    p4_golden_ai_native_commit(actors[0],8481,result.value);
    if(!(view->actor->message_flags&2) || P4_G16(actors[0]+0xec)!=42) return -1;
    p4_golden_ai_native_message_wait(P4_G2H(actors[0]));
    if(view->actor->message_flags&2) return -1;
    message=0;
    if(!p4_golden_ai_execute(P4_GAME_MODE_GOLDEN,view,8481,&message,&result)) return -1;
    p4_golden_ai_native_commit(actors[0],8481,result.value);
    p4_golden_ai_native_message_wait(P4_G2H(actors[0]));
    if(!(view->actor->message_flags&2)) return -1;
    if(memcmp(packets,P4_G2H(battle+0x198),sizeof packets)) return -1;
    message=3;
    if(!p4_golden_ai_execute(P4_GAME_MODE_GOLDEN,view,8479,&message,&result) ||
        view->battle->assist_choreography!=3 ||
        memcmp(packets,P4_G2H(battle+0x198),sizeof packets)) return -1;
    uint32_t known=0;
    if(!p4_golden_ai_native_knowledge_clear() ||
        !p4_golden_ai_native_knowledge_set(4,3) ||
        !p4_golden_ai_native_knowledge_test(4,3,&known) || known!=8) return -1;
    puts("qualification lifecycle=eligible-all-actors/continuous/bit29/clear187AC/knowledge; R4-dep:M11-assist and R4-dep:M11-message-follow-up emit no packet");
    return 0;
}
static void reset_work(const P4GoldenAiProduct *p,unsigned script,size_t procedure)
{
    for(unsigned i=0;i<ACTORS;++i) {
        uint32_t calc=P4_G32(units[i]+0xa64);
        P4_G16(calc+8)=initial_hp[i]; P4_G16(calc+10)=initial_sp[i];
        P4_G32(calc+0xc)=0; P4_G16(actors[i]+0x1a)=9;
        btlTargetResetCommandRecord(P4_G2H(actors[i]+0x38));
    }
    p4_golden_ai_native_work_release(P4_G2H(local));
    p4_golden_ai_native_work_release(P4_G2H(work));
    memset(P4_G2H(work),0,0x168); memset(P4_G2H(local),0,0x348);
    /* Native script-work constructor default with no message manager attached. */
    P4_G32(work+0x130)=UINT32_MAX;
    const uint8_t *r=p->scripts[script].procedures+procedure*32+24;
    uint32_t pc=(uint32_t)r[0]|(uint32_t)r[1]<<8|(uint32_t)r[2]<<16|(uint32_t)r[3]<<24;
    P4_G32(work+0x18)=pc; P4_G32(work+0x110)=P4_GPDATA(uint32_t,script?0xb42c:0xb428);
    unsigned acting=script?PARTY:0; /* 001DED30: BF9 party, BF10 enemy. */
    P4_G32(work+0x140)=local; P4_G32(work+0x154)=actors[acting];
    P4_G32(battle+0x170)=actors[acting];
    for(unsigned i=0;i<ACTORS;++i)
        P4_G32(battle+0x29c+i*4)=actors[(acting+i)%ACTORS];
    (void)btlCheckBattleEnd();
    P4_G32(0x00764600)=work;
    p4_golden_ai_qualification_seed(1);
    p4_golden_ai_native_continuous_entry();
    p4_golden_ai_native_eligible_start(P4_G2H(actors[acting]));
}
static int party_context(const P4GoldenAiProduct *product,unsigned character)
{
    unsigned other=1;
    for(unsigned i=0;i<PARTY;++i) {
        unsigned id=character;
        if(i) { if(other==character) ++other; id=other++; }
        uint16_t hp,sp;
        if(p4_golden_levels_pair(product->raw[P4_GAI_LEVELS].data,
            product->raw[P4_GAI_LEVELS].size,10,(int)id,&hp,&sp) || !hp) return -1;
        uint32_t calc=P4_G32(units[i]+0xa64);
        P4_G16(units[i]+0xa4)=(uint16_t)id; P4_G16(calc+2)=(uint16_t)id;
        P4_G8(calc+0x10)=0;
        initial_hp[i]=hp; initial_sp[i]=sp;
    }
    return 0;
}
static void enemy_context(unsigned id)
{
    uint32_t calc=P4_G32(units[PARTY]+0xa64),row=P4_G32(0x7644b4)+id*60;
    P4_G16(units[PARTY]+0xa4)=(uint16_t)id;
    P4_G16(calc+2)=(uint16_t)id; P4_G16(calc+4)=(uint16_t)id;
    P4_G8(calc+0x10)=0;
    initial_hp[PARTY]=P4_G16(row+4); initial_sp[PARTY]=P4_G16(row+6);
}
static int production_admission(const P4GoldenDomainMap *map,const P4GoldenSource *source,
    const P4GoldenExtraSource *extra,const P4GoldenModeIdentityArtifacts *artifacts,
    const P4GoldenAiProduct *product,const P4GoldenModeAdapters *adapters,
    char *reason,size_t capacity)
{
    /* The R4 converter consumes only these two prepared-source fields. */
    P4GoldenModeSources context;
    context.projection=(P4GoldenModeProjection){0};
    context.outputs=(P4GoldenModeOutputs){0};
    int result=-1;
    if(p4_golden_mode_project(map,source,&context.projection,reason,capacity) ||
       p4_golden_mode_outputs_prepare(&context.outputs,map,source,extra,&context.projection,
           artifacts,reason,capacity) ||
       adapters->items[0].convert(&context,reason,capacity)) goto done;
    const P4GoldenCacheTable *ai=NULL;
    for(size_t i=0;i<context.outputs.table_count;++i)
        if(!strcmp(context.outputs.tables[i].name,"battle/AICALC.TBL")) ai=context.outputs.tables+i;
    P4GoldenSpan spans[12];
    if(!ai || p4_golden_tbl_spans(ai->data,ai->size,spans,12)!=11) goto done;
    for(unsigned i=9;i<11;++i)
        if(spans[i].size!=product->programs[i].size ||
            memcmp(spans[i].bytes,product->programs[i].bytes,spans[i].size)) goto done;
    puts("qualification production_R4_conversion=1 M7a_SKILL1_projection=present BF9_BF10_unchanged=1");
    result=0;
done:
    p4_golden_mode_outputs_free(&context.outputs);
    p4_golden_mode_projection_free(&context.projection);
    return result;
}
static int qualify(const P4GoldenAiProduct *product,unsigned script,size_t procedure,
    int synthetic,uint8_t *entered)
{
    size_t count=product->scripts[script].procedure_count;
    memset(entered,0,count);
    reset_work(product,script,procedure);
    printf("qualification begin source=%s block=%u procedure=%zu",
        synthetic?"synthetic-locals":"real-caller",script+9,procedure);
    if(synthetic) printf(" integer_locals=%u integer_value=1 float_locals=%u float_value=1 entity_id=1 skill_id=1 index=1",
        product->scripts[script].local_integers,product->scripts[script].local_floats);
    putchar('\n'); fflush(stdout);
    uint32_t result=0,pc=0,command=0;
    int status; unsigned resumes=0;
    do {
        status=p4_golden_ai_qualification_run(P4_G2H(work),&result,100000,&pc,&command,
            entered,count,synthetic);
    } while(status==1 && result==1 && ++resumes<10000);
    if(status==1 && result==2) return 0;
    printf("qualification failure source=%s block=%u procedure=%zu status=%d result=%u pc=%u command=%u\n",
        synthetic?"synthetic-locals":"real-caller",script+9,procedure,status,result,pc,command);
    return 1;
}
int main(int argc,char **argv)
{
    char reason[256]="",directory[1024],key[65];
    if(argc!=4 && argc!=6) { fprintf(stderr,"usage: qualification ELF PS2_ISO STEAM_INSTALL [BLOCK PROCEDURE]\n"); return 2; }
    puts("hybrid qualification (PS2 native tables + Golden raw banks); not gameplay receipt; real native commands");
    fflush(stdout);
    CHECK(p4_game_mode()==P4_GAME_MODE_PS2);
    CHECK(!p4_golden_ai_adapter_register(P4_GAME_MODE_PS2));
    CHECK(!p4_golden_ai_adapter_register(P4_GAME_MODE_GOLDEN));
    const P4GoldenModeAdapters *adapters=p4_golden_mode_registered();
    CHECK(adapters->count==1 && !strcmp(adapters->items[0].name,"AICALC9/10 compatibility"));
    CHECK(adapters->items[0].convert(NULL,reason,sizeof reason)==-1);
    CHECK(strstr(reason,"R4 production lift requires M7a SKILL1 projection"));
    CHECK(p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN,NULL,adapters,reason,sizeof reason)==-1);
    CHECK(strstr(reason,"M6 image-table cache binder required"));
    reason[0]=0;
    puts("qualification production guards: M7a SKILL1 and M6 binder refusals preserved");
    CHECK(!p4_golden_locate(argv[3],directory,sizeof directory));
    P4GoldenSource source={0}; P4GoldenExtraSource extra={0};
    P4GoldenDomainMap map; const P4GoldenDomainShape shape={624,472,368,2560,256};
    P4GoldenModeIdentity identity={0}; P4GoldenModeIdentityArtifacts artifacts={0};
    CHECK(!p4_golden_domain_map_build(&map,&shape,reason,sizeof reason));
    CHECK(!p4_golden_load(&source,directory,reason,sizeof reason));
    CHECK(!p4_golden_extra_load(&extra,directory,reason,sizeof reason));
    CHECK(!p4_golden_mode_identity_load(&identity,&artifacts,directory,&source,&extra,&map,reason,sizeof reason));
    const unsigned source_ids[]={P4_GOLDEN_ITEM,0,P4_GOLDEN_PERSONA,0,P4_GOLDEN_SKILL,P4_GOLDEN_UNIT};
    const char *names[]={"golden-ai/item","golden-ai/levels","golden-ai/persona","golden-ai/programs","golden-ai/skill","golden-ai/unit"};
    P4GoldenCacheTable raw[P4_GAI_RAW_COUNT]={0},cached[P4_GAI_RAW_COUNT]={0};
    for(unsigned i=0;i<P4_GAI_RAW_COUNT;++i) {
        raw[i]=(P4GoldenCacheTable){names[i],source.tables[source_ids[i]].bytes,source.tables[source_ids[i]].size};
        cached[i].name=names[i];
    }
    raw[P4_GAI_LEVELS]=(P4GoldenCacheTable){names[P4_GAI_LEVELS],artifacts.levels,sizeof artifacts.levels};
    raw[P4_GAI_PROGRAMS]=(P4GoldenCacheTable){names[P4_GAI_PROGRAMS],extra.tables[P4_GOLDEN_EXTRA_AICALC].bytes,extra.tables[P4_GOLDEN_EXTRA_AICALC].size};
    char *pref=SDL_GetPrefPath("p4pc","Persona4-M7bNativeQualification"); CHECK(pref);
    P4GoldenCachePaths paths={pref,directory,SDL_GetBasePath(),NULL};
    CHECK(!p4_golden_cache_store(&paths,&identity.identity,raw,P4_GAI_RAW_COUNT,key,reason,sizeof reason));
    CHECK(!p4_golden_cache_load(&paths,&identity.identity,cached,P4_GAI_RAW_COUNT,key,reason,sizeof reason));
    const P4GoldenAiProduct *product;
    CHECK(!p4_golden_ai_qualification_product(&map,cached,&product));
    CHECK(!production_admission(&map,&source,&extra,&artifacts,product,adapters,reason,sizeof reason));
    p4pc_trap_configure(); (void)p4pc_load_image(argv[1]); (void)p4pc_apply_patches();
    /* Keep the real EE/chkmem heap below the synthetic arena; no allocator mock. */
    CHECK(!p4pc_heap_init(P4_G2H(0x00938a00),arena-0x00938a00));
    memcpy(P4_G2H(0x008873e8),chkmemInitRenderWareMemoryFunctions(),16);
    CHECK(!load_native_banks(argv[2],reason,sizeof reason));
    for(unsigned i=0;i<2;++i) {
        P4_GPDATA(uint32_t,i?0xb42c:0xb428)=copy(product->programs[i+9].bytes,product->programs[i+9].size);
        CHECK(product->programs[i+9].size==extra.tables[P4_GOLDEN_EXTRA_AICALC].spans[i+9].size &&
            !memcmp(product->programs[i+9].bytes,extra.tables[P4_GOLDEN_EXTRA_AICALC].spans[i+9].bytes,product->programs[i+9].size));
    }
    CHECK(!p4_golden_ai_native_stage(product)); p4_golden_ai_native_activate(product);
    p4_golden_ai_knowledge_qualification_publish(); p4_golden_ai_qualification_seed(1);
    CHECK(!p4_golden_vm_native_prepare()); actors_init();
    CHECK(!lifecycle());
    reset_work(product,0,0);
    int64_t argument=440,abi_result;
    CHECK(!p4_golden_ai_qualification_command(P4_G2H(work),0x001defa0,&argument,1,&abi_result));
    CHECK(P4_G16(actors[0]+0x6c)==2 && P4_G16(actors[0]+0x6e)==576);
    CHECK(!p4_golden_ai_qualification_command(P4_G2H(work),0x001df040,&argument,1,&abi_result));
    CHECK(P4_G16(actors[0]+0x6c)==3 && P4_G16(actors[0]+0x70)==440);
    CHECK(!p4_golden_ai_qualification_command(P4_G2H(work),0x001e7e30,NULL,0,&abi_result));
    uint32_t queued=P4_G32(battle+0x2a0);
    CHECK(abi_result==(INT64_C(0x80000000)|P4_G32(queued+8)));
    CHECK(P4_G8(work+0x4f)==0 && P4_G32(work+0x10c)==(uint32_t)abi_result);
    puts("qualification native ABI: real skill/item setters and encoded-entity getter pass; skill-result vectors pass");
    unsigned failed=0;
    for(unsigned script=0;script<2;++script) {
        if(argc==6 && (unsigned)strtoul(argv[4],NULL,0)!=script+9) continue;
        size_t count=product->scripts[script].procedure_count;
        uint8_t *coverage=calloc(count,3); CHECK(coverage);
        uint8_t *real=coverage,*synthetic=coverage+count,*entered=coverage+count*2;
        unsigned attempts_failed=0,entries=0;
        if(argc!=6 && !script) {
            for(unsigned character=1;character<=10;++character) {
                if(party_context(product,character)) continue;
                for(unsigned tactic=0;tactic<=UINT8_MAX;++tactic) {
                    char name[32];
                    int length=snprintf(name,sizeof name,P4_G2H(0x00609e20),character,tactic);
                    CHECK(length>=0 && (size_t)length<sizeof name);
                    int32_t procedure=scrFindPrcdIdxByName(P4_G2H(P4_GPDATA(uint32_t,0xb428)),name);
                    if(procedure<0 || (size_t)procedure>=count) continue;
                    P4_G8(P4_G32(units[0]+0xa64)+0x10)=(uint8_t)tactic;
                    ++entries;
                    if(qualify(product,script,(size_t)procedure,0,entered)) { ++attempts_failed; continue; }
                    real[procedure]=1;
                    for(size_t i=0;i<count;++i) if(entered[i]) real[i]=1;
                }
            }
        } else if(argc!=6) {
            const P4GoldenSpan *rows=product->programs+8;
            for(size_t id=1;id<native_enemy_rows && id<rows->size/164;++id) {
                unsigned procedure=(unsigned)rows->bytes[id*164+2]|(unsigned)rows->bytes[id*164+3]<<8;
                if(!procedure || procedure>=count || !P4_G16(P4_G32(0x7644b4)+(uint32_t)id*60+4)) continue;
                enemy_context((unsigned)id); ++entries;
                if(qualify(product,script,procedure,0,entered)) { ++attempts_failed; continue; }
                real[procedure]=1;
                for(size_t i=0;i<count;++i) if(entered[i]) real[i]=1;
            }
        }
        CHECK(!party_context(product,1)); enemy_context(1);
        for(size_t procedure=0;procedure<count;++procedure) {
            if(real[procedure] || (argc==6 && strtoul(argv[5],NULL,0)!=procedure)) continue;
            if(qualify(product,script,procedure,1,entered)) { ++attempts_failed; continue; }
            synthetic[procedure]=1;
            for(size_t i=0;i<count;++i) if(entered[i] && !real[i]) {
                real[i]=2;
                printf("qualification coverage source=real-caller parent_source=synthetic-locals block=%u procedure=%zu parent=%zu\n",
                    script+9,i,procedure);
            }
        }
        unsigned caller_count=0,from_synthetic=0,synthetic_count=0,remaining=0,total=0;
        for(size_t i=0;i<count;++i) {
            if(argc==6 && strtoul(argv[5],NULL,0)!=i) continue;
            ++total;
            if(real[i]) { ++caller_count; from_synthetic+=real[i]==2; }
            else if(synthetic[i]) ++synthetic_count;
            else { ++remaining; printf("qualification uncovered block=%u procedure=%zu\n",script+9,i); }
        }
        printf("qualification AICALC%u real_caller=%u caller_from_synthetic=%u synthetic_locals=%u failed=%u total=%u native_entries=%u failed_attempts=%u\n",
            script+9,caller_count,from_synthetic,synthetic_count,remaining,total,entries,attempts_failed);
        failed+=remaining+attempts_failed+(total==0);
        free(coverage);
    }
    p4_golden_ai_native_work_release(P4_G2H(work));
    for(unsigned i=0;i<ACTORS;++i) p4_golden_ai_native_action_release(P4_G2H(actors[i]));
    printf("qualification real_native_calls=%llu cache_verified=1 opaque_programs_unchanged=1\n",
        (unsigned long long)p4_golden_ai_qualification_native_calls());
    p4_golden_ai_native_battle_release(); CHECK(!p4_golden_ai_native_session_end());
    CHECK(!p4_golden_ai_product_release(NULL));
    p4_golden_cache_free(cached,P4_GAI_RAW_COUNT); SDL_free(pref);
    p4_golden_extra_free(&extra); p4_golden_free(&source);
    return failed?1:0;
}
