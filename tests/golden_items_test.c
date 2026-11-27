/* Synthetic RE-3 vectors; no payload, upstream UI/duplicate rule or source pin. */
#include "p4pc/golden_items.h"
#include "../src/platform/golden/golden_items_native.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
static uint32_t words[2560], counts[2560], emitted[2560], used;
static P4GameMode mode=P4_GAME_MODE_PS2;
static P4GameTableCompletePrepareFn prepared;
static P4GameTableCompleteCommitFn committed;
static unsigned registrations, unregistrations, quantity_writes;
static P4ItemRow native_rows[2560];
static jmp_buf return_refusal;
static unsigned return_refusals;
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{
    assert(!strcmp(reason,"R7a-dep:M11-return-field-flow"));
    ++return_refusals; longjmp(return_refusal,1);
}
P4GameMode p4_game_mode(void) { return mode; }
int p4_game_tables_battle_complete_register(const char *key,P4GameTableCompletePrepareFn p,
    P4GameTableCompleteCommitFn c,void *user)
{
    assert(!strcmp(key,"golden-items") && !user && !prepared && !committed);
    prepared=p; committed=c; ++registrations; return 0;
}
int p4_game_tables_battle_complete_unregister(const char *key,P4GameTableCompletePrepareFn p,
    P4GameTableCompleteCommitFn c,void *user)
{
    assert(!strcmp(key,"golden-items") && !user && prepared==p && committed==c);
    prepared=NULL; committed=NULL; ++unregistrations; return 0;
}
uint32_t datEntryGetKind(int16_t id) { assert(id>=0 && id<2560); return native_rows[id].kind; }
uint32_t datEntryGetFlags(int16_t id) { assert(id>=0 && id<2560); return words[id]; }
int32_t datItemCount(int16_t id) { assert(id>=0 && id<2560); return (int32_t)counts[id]; }
void datItemSetCount(int16_t id,int32_t n) { assert(id>=0 && id<2560 && n>=0 && n<=99); counts[id]=(uint32_t)n; ++quantity_writes; }
const uint8_t *p4_game_tables_items_native_metadata(uint32_t id)
{ assert(id<2560); return (const uint8_t *)&native_rows[id]; }
const uint8_t *p4_golden_skill_raw(uint32_t id)
{ static const uint8_t row[44]={[4]=1}; (void)id; return row; }
_Noreturn void p4_game_tables_items_native_refuse(int result)
{ fprintf(stderr,"unexpected native item refusal %d\n",result); abort(); }
static uint32_t flags(void *p,uint32_t id) { (void)p; return words[id]; }
static uint32_t quantity(void *p,uint32_t id) { (void)p; return counts[id]; }
static int emit(void *p,uint32_t id,uint32_t n)
{ (void)p; assert(n || id==UINT32_MAX); emitted[used++]=id; return 0; }
static void put32(uint8_t *p,uint32_t v)
{ p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
static size_t member(uint8_t *pak,size_t at,const char *name,const uint8_t *data,size_t size)
{
    memcpy(pak+at,name,strlen(name)); put32(pak+at+32,(uint32_t)size);
    memcpy(pak+at+36,data,size); return at+36+size;
}
static void stem_grammar(void)
{
    uint8_t pak[1024]={0}, camp[320]={0}, equip[128]={0}, range[32]={0};
    const uint32_t masks[]={0x10000,0x800000,0x400000,0x200000,0x4000000,0x8000000,
        0x1000000,0x80000,0x10000000,0x20000,0x2000000,0x40000};
    const uint32_t gear[]={0x3fff,0x4000,0x8000,0x100000};
    put32(camp,12*24); put32(camp+4,12); put32(equip,4*24); put32(equip+4,4);
    for (unsigned i=0;i<12;++i) { put32(camp+16+i*24,masks[i]); memcpy(camp+20+i*24,"cmpTypeID_TEST",14); }
    for (unsigned i=0;i<4;++i) { put32(equip+16+i*24,gear[i]); memcpy(equip+20+i*24,"cmpTypeID_TEST",14); }
    put32(range,4); put32(range+4,1); range[18]=2; /* inclusive [0,2] */
    put32(pak,3);
    size_t at=member(pak,4,"cmpItemSortTable.ctd",camp,sizeof camp);
    at=member(pak,at,"cmpEquipSortTable.ctd",equip,sizeof equip);
    at=member(pak,at,"cmpTypeID_TEST.ctd",range,sizeof range);
    P4GoldenItemSort sort;
    assert(!p4_golden_item_sort_open(&sort,pak,at));
    for (unsigned id=0;id<3;++id) { words[id]=0x10000; counts[id]=1; }
    used=0; assert(p4_golden_item_list(&sort,-1,flags,quantity,emit,NULL)==3);
    assert(emitted[0]==0 && emitted[1]==1 && emitted[2]==2);
    memset(words,0,sizeof words); memset(counts,0,sizeof counts); used=0;
    pak[60]='C'; assert(p4_golden_item_sort_open(&sort,pak,at)==-1); pak[60]='c';
    assert(!p4_golden_item_sort_open(&sort,pak,at));
    P4GameTablesDefinitionBuilder builder={0};
    P4GoldenCacheTable cache={.name="items/cmp-tables",.data=pak,.size=at};
    assert(p4_golden_items_capture(NULL,&builder,&cache,1,NULL,0)==-1 && !registrations);
    mode=P4_GAME_MODE_GOLDEN;
    assert(!p4_golden_items_capture(NULL,&builder,&cache,1,NULL,0) && registrations==1);
    p4_golden_items_reset();
    assert(unregistrations==1 && !prepared && !committed);
    assert(!p4_golden_items_capture(NULL,&builder,&cache,1,NULL,0));
    memset(pak,0,sizeof pak); /* native owner must retain its own verified bytes */
    assert(prepared && !prepared(NULL)); committed(NULL);
    for (unsigned id=0;id<2560;++id) native_rows[id].kind=id/256;
    for (unsigned id=0;id<3;++id) { words[id]=0x10000; counts[id]=1; }
    P4GameItemInventoryRow inventory[3];
    assert(p4_golden_item_native_inventory(inventory,3)==3 && inventory[0].id==0 && inventory[2].id==2);
    assert(p4_golden_item_native_category(0x900)==0 && p4_golden_item_native_aux_index(0x801)==0x101);
    assert(p4_golden_item_native_aux_index(UINT32_MAX)==-1);
    P4GoldenDomainMap map;
    const P4GoldenDomainShape shape={624,472,368,2560,256};
    uint32_t skill;
    assert(!p4_golden_domain_map_build(&map,&shape,NULL,0) &&
        !p4_golden_domain_to_port(&map,P4_GOLDEN_DOMAIN_SKILL,601,&skill,NULL));
    native_rows[0x600].granted_skill=skill; counts[0x600]=2;
    uint16_t persona[8]={1,2,3,4,5,6,7,0}, full[8]={1,2,3,4,5,6,7,8};
    assert(p4_golden_item_native_route(0x600)==P4_GOLDEN_ITEM_TEACH);
    assert(p4_golden_item_native_teach(0x600,persona,0,1,0)==-1 && !quantity_writes);
    assert(p4_golden_item_native_teach(0x600,persona,1,1,0)==7 && quantity_writes==1);
    assert(persona[7]==skill && counts[0x600]==1);
    assert(p4_golden_item_native_teach(0x600,full,1,0,4)==-1 && quantity_writes==1 && full[3]==4);
    assert(p4_golden_item_native_teach(0x600,full,1,1,4)==3 && quantity_writes==2 && counts[0x600]==0);
    assert(full[3]==skill);
    if (!setjmp(return_refusal)) p4_golden_item_native_return_pending();
    assert(return_refusals==1);
    p4_golden_items_reset(); prepared=NULL; committed=NULL; mode=P4_GAME_MODE_PS2;
    memset(words,0,sizeof words); memset(counts,0,sizeof counts);
}
int main(void)
{
    stem_grammar();
    unsigned seen[2560]={0}, total=0;
    const int categories[]={0,1,2,4,5,6,7,3,4,0};
    for (unsigned kind=0;kind<10;++kind) assert(p4_golden_item_category(kind)==categories[kind]);
    assert(p4_golden_item_category(10)==-1 && p4_golden_item_category(32)==-1);
    for (unsigned c=0;c<8;++c) {
        unsigned n=p4_golden_item_category_count(c);
        for (unsigned i=0;i<n;++i) {
            int id=p4_golden_item_category_id(c,i);
            assert(id>=0 && id<2560 && !seen[id]++); ++total;
        }
        assert(p4_golden_item_category_id(c,n)==-1);
    }
    assert(total==2560 && !p4_golden_item_category_count(8));
    assert(p4_golden_item_category_id(0,255)==0xff && p4_golden_item_category_id(0,256)==0x900 &&
           p4_golden_item_category_id(0,511)==0x9ff);
    assert(p4_golden_item_category_id(4,255)==0x3ff && p4_golden_item_category_id(4,256)==0x800 &&
           p4_golden_item_category_id(4,511)==0x8ff);
    assert(p4_golden_item_aux_index(8,0x801)==0x101 && p4_golden_item_aux_index(9,0x901)==0x101);
    const int shop[]={0,1,2,4,6,3};
    for (unsigned i=0;i<6;++i) assert(p4_golden_item_shop_category(i)==shop[i]);
    assert(p4_golden_item_shop_category(6)==-1);
    const uint32_t masks[]={0x10000,0x800000,0x400000,0x200000,0x4000000,0x8000000,
        0x1000000,0x80000,0x10000000,0x20000,0x2000000,0x40000};
    P4GoldenItemSort sort={0};
    for (unsigned i=0;i<12;++i) { sort.camp[i].flags=masks[i]; assert(p4_golden_item_camp_group(masks[i])==(int)i); }
    static const uint8_t ranges[]={12,0,12,0,9,0,10,0};
    sort.camp[0].ranges=(P4GoldenSpan){ranges,sizeof ranges};
    words[12]=words[9]=words[10]=0x10000; counts[12]=counts[9]=counts[10]=1;
    assert(p4_golden_item_list(&sort,-1,flags,quantity,emit,NULL)==3 && used==3);
    assert(emitted[0]==12 && emitted[1]==9 && emitted[2]==10);
    words[9]=0x810000; used=0;
    assert(p4_golden_item_list(&sort,-1,flags,quantity,emit,NULL)==2);
    assert(p4_golden_item_camp_group(words[9])==-1);
    memset(counts,0,sizeof counts);
    words[1]=words[0x900]=2; words[5]=3;
    counts[1]=counts[5]=counts[0x900]=1; used=0;
    assert(p4_golden_item_list(&sort,0,flags,quantity,emit,NULL)==3);
    assert(emitted[0]==1 && emitted[1]==0x900 && emitted[2]==5);
    counts[0x100]=counts[0x200]=counts[0x500]=counts[0x501]=counts[0x700]=1;
    used=0; assert(p4_golden_item_list(&sort,1,flags,quantity,emit,NULL)==1 && emitted[0]==0x100);
    used=0; assert(p4_golden_item_list(&sort,2,flags,quantity,emit,NULL)==1 && emitted[0]==0x200);
    used=0; assert(p4_golden_item_list(&sort,4,flags,quantity,emit,NULL)==3);
    assert(emitted[0]==UINT32_MAX && emitted[1]==0x500 && emitted[2]==0x501);
    used=0; assert(p4_golden_item_list(&sort,5,flags,quantity,emit,NULL)==1 && emitted[0]==0x700);
    assert(p4_golden_item_list(&sort,6,flags,quantity,emit,NULL)==-1);
    const int help[]={0x1c,0x1e,0x2e,0x1d,0x2d,0x2c,0x1f,7,8,10};
    for (unsigned bit=19;bit<=28;++bit) assert(p4_golden_item_help(1u<<bit)==help[bit-19]);
    for (unsigned bit=29;bit<32;++bit) assert(p4_golden_item_help(1u<<bit)==-1);
    for (unsigned i=0;i<12;++i) assert(p4_golden_item_sell_group(i)==(i<7 || i==8));
    uint8_t skill[44]={0}; skill[4]=1;
    assert(p4_golden_item_route(9,7,skill)==P4_GOLDEN_ITEM_EQUIPMENT);
    assert(p4_golden_item_route(7,7,skill)==P4_GOLDEN_ITEM_EQUIPMENT);
    assert(p4_golden_item_route(8,7,skill)==P4_GOLDEN_ITEM_FIELD);
    assert(p4_golden_item_route(3,246,skill)==P4_GOLDEN_ITEM_RETURN);
    assert(p4_golden_item_route(6,601,NULL)==P4_GOLDEN_ITEM_TEACH);
    assert(p4_golden_item_route(6,0,NULL)==P4_GOLDEN_ITEM_UNAVAILABLE);
    assert(p4_golden_item_route(4,7,skill)==P4_GOLDEN_ITEM_CONTEXT);
    skill[0]=2; assert(p4_golden_item_route(8,7,skill)==P4_GOLDEN_ITEM_UNAVAILABLE);
    skill[0]=0; skill[4]=0; assert(p4_golden_item_route(8,7,skill)==P4_GOLDEN_ITEM_UNAVAILABLE);
    uint8_t count=3;
    assert(!p4_golden_item_consume(&count,0,0) && count==3);
    assert(p4_golden_item_consume(&count,4,0) && count==2);
    assert(p4_golden_item_consume(&count,1,1) && count==2);
    P4GoldenDomainMap map;
    const P4GoldenDomainShape shape={624,472,368,2560,256};
    assert(!p4_golden_domain_map_build(&map,&shape,NULL,0));
    uint32_t port;
    assert(!p4_golden_domain_to_port(&map,P4_GOLDEN_DOMAIN_SKILL,601,&port,NULL));
    uint16_t personas[2][8]={{1,2,3,4,5,6,7,0},{1,2,3,4,5,6,7,8}}, before[2][8];
    memcpy(before,personas,sizeof before); count=3;
    assert(p4_golden_item_teach(&map,6,port,personas[0],&count,0,1,0)==-1 && count==3);
    assert(!memcmp(before,personas,sizeof before));
    assert(p4_golden_item_teach(&map,6,port,personas[0],&count,1,1,4)==7 && count==2);
    assert(personas[0][7]==port && personas[0][3]==4 && !memcmp(before[1],personas[1],sizeof before[1]));
    assert(p4_golden_item_teach(&map,6,port,personas[1],&count,1,0,4)==-1 && count==2);
    assert(!memcmp(before[1],personas[1],sizeof before[1]));
    assert(p4_golden_item_teach(&map,6,port,personas[1],&count,1,1,4)==3 && count==1);
    assert(personas[1][3]==port);
    assert(p4_golden_item_teach(&map,6,port,personas[1],&count,1,1,400)==-1 && count==1);
    assert(p4_golden_item_teach(&map,6,0,personas[1],&count,1,1,4)==-2);
    assert(p4_golden_item_teach(&map,6,624,personas[1],&count,1,1,4)==-2);
    assert(p4_golden_item_teach(&map,6,port,NULL,&count,1,1,4)==-2);
    uint16_t replacement[8]={4,7,4,8,9,10,11,12};
    assert(p4_golden_item_teach(&map,6,port,replacement,&count,1,1,4)==0 && count==0);
    assert(replacement[0]==port && replacement[2]==4); /* first matching old slot only */
    assert(p4_golden_item_teach(&map,6,port,replacement,&count,1,1,7)==-1 && !count && replacement[1]==7);
    puts("golden_items: 2560 category IDs; 10 expanded tags; six routes; teach append/replace/cancel passed");
    return 0;
}
