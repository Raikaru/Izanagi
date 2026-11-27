#include "../src/platform/golden/golden_vm.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while (0)
static uint8_t rows[472*44], elements[624*2];
static uint16_t skills[8], affinity[16];
static P4GoldenAiItem equipment[3];
static P4GoldenAiUnit units[3];
static P4GoldenAiEntity entities[3];
static P4GoldenAiBattle battle;
static P4GoldenAiRng rng;
static P4GoldenSfmt sfmt;
static uint32_t knowledge[32];
static P4GoldenAi ai;
static P4GoldenAiResult result;
static int64_t args[2];
static void reset(void)
{
    memset(rows,0,sizeof rows); memset(elements,0,sizeof elements);
    memset(skills,0,sizeof skills); memset(affinity,0,sizeof affinity);
    memset(equipment,0,sizeof equipment); memset(units,0,sizeof units);
    memset(entities,0,sizeof entities); memset(&battle,0,sizeof battle);
    memset(knowledge,0,sizeof knowledge); memset(&sfmt,0,sizeof sfmt);
    memset(&rng,0,sizeof rng); rng.front=3;
    for (unsigned i=0;i<3;++i) {
        units[i].hp=100; units[i].base_hp=100; units[i].base_sp=100;
        units[i].sp=25; units[i].skills=skills; units[i].affinity=affinity;
        units[i].target_flags=8; units[i].character_id=4;
        for (unsigned j=0;j<3;++j) units[i].equipment[j]=equipment+j;
        entities[i].id=42+i; entities[i].flags=9; entities[i].unit=units+i;
        entities[i].next=i==2 ? NULL : entities+i+1;
    }
    ai=(P4GoldenAi){entities,entities,&battle,rows,elements,knowledge,32,&rng,&sfmt};
    result=(P4GoldenAiResult){99,4}; args[0]=args[1]=0;
}
static void queue(unsigned a,unsigned b,unsigned c)
{
    memset(&rng,0,sizeof rng); rng.front=3;
    rng.words[3]=2*a; rng.words[4]=2*b; rng.words[5]=2*c;
}
static int call(unsigned command)
{
    /* Exercise every command vector through a real synthetic COMM, including
     * topmost-first arguments and persistent wide/void native results. */
    P4GoldenBfTables tables={0};
    P4GoldenBfCommand *groups[]={tables.common,tables.field,tables.ai,tables.battle};
    int arity=p4_golden_ai_arity(P4_GAME_MODE_GOLDEN,command);
    if (arity<0) return 0;
    groups[command>>12][command&4095]=(P4GoldenBfCommand){0,(uint32_t)arity,p4_golden_ai_execute};
    const uint8_t code[]={8,0,(uint8_t)command,(uint8_t)(command>>8)};
    P4GoldenVm vm={0};
    vm.code=code; vm.code_words=1; vm.tables=&tables; vm.ai=&ai;
    vm.depth=arity;
    for (int i=0;i<arity;++i) vm.stack[arity-1-i]=(P4GoldenVmValue){(uint64_t)args[i],0};
    vm.stack[47]=(P4GoldenVmValue){(uint64_t)result.value,result.type};
    if (p4_golden_vm_run(&vm,1)!=0 || vm.pc!=1 || vm.depth!=0) return 0;
    memcpy(&result.value,&vm.stack[47].bits,8); result.type=vm.stack[47].tag;
    return 1;
}
int main(void)
{
    static const unsigned commands[]={8468,8469,8470,8471,8472,8473,8474,8479,8481,8484,8485,148};
    static const int arities[]={2,1,1,2,0,1,0,1,1,1,1,1};
    for (unsigned i=0;i<12;++i) {
        CHECK(p4_golden_ai_arity(P4_GAME_MODE_GOLDEN,commands[i])==arities[i]);
        CHECK(p4_golden_ai_arity(P4_GAME_MODE_PS2,commands[i])==-1);
        CHECK(!p4_golden_ai_execute(P4_GAME_MODE_PS2,NULL,commands[i],NULL,NULL));
    }
    CHECK(!p4_golden_ai_execute(P4_GAME_MODE_GOLDEN,NULL,123,NULL,NULL));
    reset(); args[0]=0xf000002a; args[1]=0xab000008; units[0].status=0x28;
    CHECK(call(8468) && result.value==1 && result.type==0 && rng.front==3);
    entities[1].id=42; units[0].status=0; units[1].status=8; entities[0].flags=0; units[0].hp=0;
    CHECK(call(8468) && result.value==0);
    reset(); args[0]=0x12345; entities[0].message_flags=0xa5;
    CHECK(call(8469) && entities[0].message_parameter==0x2345 && entities[0].action_kind==8);
    CHECK(result.value==99 && result.type==4 && entities[0].message_flags==0xa5);
    CHECK(call(8481) && entities[0].action_kind==7 && entities[0].message_flags==0xa7);
    CHECK(result.value==99 && result.type==4 && rng.front==3);
    reset(); entities[1].next=NULL; units[0].side=units[1].side=1; args[0]=1; queue(3,0,0);
    CHECK(call(8470) && result.value==0x8000002b && rng.front==4 && !entities[0].selected);
    entities[0].next=NULL; queue(123,0,0); args[0]=7;
    CHECK(call(8470) && result.value==0x8000002a && rng.front==4);
    args[0]=1; knowledge[1]=2; queue(0,0,0);
    CHECK(call(8470) && result.value==-1 && rng.front==3);
    knowledge[1]=0; args[0]=5;
    CHECK(call(8470) && result.value==-1 && rng.front==3);
    args[0]=8; CHECK(call(8470) && result.value==-1);
    args[0]=1; units[0].hp=0; CHECK(call(8470) && result.value==-1);
    units[0].hp=100; units[0].status=0x80000; CHECK(call(8470) && result.value==-1);
    reset(); p4_golden_ai_battle_categories(&battle,0x20000000,1,1);
    CHECK(call(8472) && result.value==-1);
    p4_golden_ai_battle_categories(&battle,0x20000000,0,2); CHECK(call(8472) && result.value==2);
    battle.category=0xffff; CHECK(call(8472) && result.value==3);
    CHECK(call(8474) && result.value==1);
    battle.flags=0x1fffffff; CHECK(call(8472) && result.value==0);
    CHECK(call(8474) && result.value==0);
    result=(P4GoldenAiResult){99,4}; args[0]=0x12345678;
    CHECK(call(8479) && battle.assist_choreography==0x5678 && result.value==99 && result.type==4);
    reset(); entities[1].next=NULL; args[0]=25;
    CHECK(call(8473) && result.value==1);
    units[1].sp=26; CHECK(call(8473) && result.value==0);
    units[1].side=1; CHECK(call(8473) && result.value==1);
    units[1].side=0; entities[1].flags=0; CHECK(call(8473) && result.value==1);
    entities[1].flags=9; units[1].hp=0; CHECK(call(8473) && result.value==1);
    ai.head=NULL; args[0]=0; CHECK(call(8473) && result.value==1);
    ai.head=entities; args[0]=0x1000000; CHECK(call(8473) && result.value==0);
    entities[0].next=NULL; units[0].flags=4; units[0].sp=units[0].base_sp=65535; args[0]=65537;
    CHECK(call(8473) && result.value==1);
    reset(); units[0].base_hp=units[0].base_sp=101;
    const uint16_t sp_skills[]={0x213,0x214,0x215,0x24e,0x22c,0x22d,0,0};
    memcpy(skills,sp_skills,sizeof skills); equipment[0].effect=equipment[1].effect=0x1f;
    CHECK(p4_golden_ai_maximum(units,1)==282);
    const uint16_t hp_skills[]={0x210,0x211,0x212,0x24d,0x22c,0x22d,0,0};
    memcpy(skills,hp_skills,sizeof skills); equipment[0].effect=equipment[1].effect=0x1a;
    CHECK(p4_golden_ai_maximum(units,0)==282);
    memset(skills,0,sizeof skills); skills[0]=0x22d; units[0].base_sp=900;
    CHECK(p4_golden_ai_maximum(units,1)==999); units[0].flags=4;
    CHECK(p4_golden_ai_maximum(units,1)==1800); units[0].base_sp=65535;
    CHECK(p4_golden_ai_maximum(units,1)==131070);
    reset(); rows[44+6]=1; rows[44+8]=10; rows[44+10]=2;
    CHECK(p4_golden_ai_cost(&ai,units,1)==12); skills[0]=0x22b;
    CHECK(p4_golden_ai_cost(&ai,units,1)==6);
    rows[44+8]=rows[44+10]=0; CHECK(p4_golden_ai_cost(&ai,units,1)==1);
    rows[44]=0x10; rows[44+6]=2; rows[44+8]=10; rows[44+10]=2;
    equipment[0].effect=equipment[1].effect=0x8c; skills[0]=0x22a;
    CHECK(p4_golden_ai_cost(&ai,units,1)==18);
    rows[44+8]=rows[44+10]=0; CHECK(p4_golden_ai_cost(&ai,units,1)==0);
    rows[44+1]=2; units[0].status=8; CHECK(p4_golden_ai_trigger(&ai,units,1)==0);
    reset(); rows[44+6]=1; rows[44+8]=20; units[0].hp=20;
    CHECK(p4_golden_ai_trigger(&ai,units,1)==1); rows[44]=1;
    CHECK(p4_golden_ai_trigger(&ai,units,1)==0);
    rows[44+6]=2; units[0].sp=19; CHECK(p4_golden_ai_trigger(&ai,units,1)==2);
    reset(); elements[0]=2; elements[20]=elements[40]=1;
    skills[0]=10; skills[1]=20; rows[20*44+12]=9; args[0]=1; args[1]=1;
    queue(8,9,50); CHECK(call(8471) && result.value==10 && rng.front==5);
    args[1]=0; queue(8,9,50); CHECK(call(8471) && result.value==20 && rng.front==5);
    args[1]=2; queue(8,9,50); CHECK(call(8471) && result.value==20 && rng.front==6);
    queue(8,9,51); CHECK(call(8471) && result.value==10 && rng.front==6);
    skills[0]=0; args[1]=1; queue(9,99,0); CHECK(call(8471) && result.value==20 && rng.front==4);
    args[1]=2; queue(9,99,0); CHECK(call(8471) && result.value==20 && rng.front==5);
    skills[1]=0; queue(0,0,0); CHECK(call(8471) && result.value==-1 && rng.front==3);
    elements[0]=1; units[0].status=0x80008; args[1]=1;
    CHECK(call(8471) && result.value==0 && rng.front==4);
    reset(); elements[0]=2; elements[20]=1; skills[0]=skills[1]=10; args[0]=1; args[1]=1;
    queue(1,0,0); CHECK(call(8471) && result.value==10 && rng.front==4);
    reset(); ai.actor=entities+2; entities[1].next=NULL; units[2].side=1;
    units[0].status=units[1].status=1; affinity[0]=0x2000;
    CHECK(call(8484) && result.value==2 && !entities[2].candidates_initialized);
    CHECK(call(8485) && result.value==-1 && entities[2].candidate_count==2 && entities[2].candidates_initialized);
    units[0].status=0; affinity[0]=0; entities[2].selection=7;
    CHECK(call(8485) && result.value==0x8000002a && entities[2].selection==1 && entities[2].selected==entities && rng.front==3);
    units[1].status=0; queue(3,0,0);
    CHECK(call(8485) && result.value==0x8000002b && entities[2].selected==entities+1 && rng.front==4);
    units[2].side=2; entities[2].selection=7;
    CHECK(call(8484) && result.value==0);
    CHECK(call(8485) && result.value==-1 && entities[2].candidate_count==0 && entities[2].selection==7 && entities[2].selected==entities+1);
    reset(); CHECK(!p4_golden_ai_affinity_bit29(NULL,-1));
    for (int i=16;i<19;++i) CHECK(!p4_golden_ai_affinity_bit29(NULL,(int16_t)i));
    affinity[0]=0x2000; CHECK(p4_golden_ai_affinity_bit29(units,0));
    affinity[1]=0x2800; units[0].secondary[0]=1;
    CHECK(!p4_golden_ai_affinity_bit29(units,1));
    const unsigned nibbles[]={0,8,15};
    for (unsigned i=0;i<3;++i) { units[0].secondary[0]=(uint8_t)nibbles[i]; CHECK(p4_golden_ai_affinity_bit29(units,1)); }
    units[0].secondary[0]=1; units[0].flags=0x200; CHECK(p4_golden_ai_affinity_bit29(units,1));
    units[0].flags=0; skills[0]=0x1df; CHECK(p4_golden_ai_affinity_bit29(units,1));
    skills[0]=0; equipment[0].effect=0x98; CHECK(p4_golden_ai_affinity_bit29(units,1)); equipment[0].effect=0;
    skills[0]=0x1dc; CHECK(p4_golden_ai_affinity_bit29(units,1));
    affinity[1]=0x2900; CHECK(!p4_golden_ai_affinity_bit29(units,1));
    skills[0]=0x1fb; affinity[1]=0x2800; CHECK(p4_golden_ai_affinity_bit29(units,1));
    affinity[1]=0x2a00; CHECK(!p4_golden_ai_affinity_bit29(units,1));
    skills[0]=0x1de; affinity[1]=0x2800; CHECK(p4_golden_ai_affinity_bit29(units,1));
    affinity[1]=0x2c00; CHECK(!p4_golden_ai_affinity_bit29(units,1));
    reset(); entities[0].next=NULL; units[0].side=1; args[0]=1;
    CHECK(call(8470) && rng.front==4 && sfmt.index==0);
    sfmt.words[0]=UINT32_MAX; args[0]=5; CHECK(call(148) && result.value==6 && sfmt.index==1 && rng.front==4);
    args[0]=1; CHECK(call(8470) && rng.front==5 && sfmt.index==1);
    sfmt.words[1]=UINT32_MAX; args[0]=INT32_MAX;
    CHECK(call(148) && result.value==INT64_C(2147483648) && result.type==0);
    sfmt.words[2]=UINT32_MAX; args[0]=INT64_C(4294967295);
    CHECK(call(148) && result.value==INT64_C(4294967296));
    sfmt.words[3]=UINT32_MAX; args[0]=-2;
    CHECK(call(148) && result.value==-1);
    reset(); p4_golden_ai_battle_categories(&battle,0,0,1);
    CHECK(call(8472) && result.value==1);
    p4_golden_ai_battle_eligible_start(&battle);
    CHECK(battle.category_guard==1 && call(8472) && result.value==-1);
    p4_golden_ai_battle_continuous_entry(&battle);
    CHECK(battle.category_guard==0 && call(8472) && result.value==1);
    battle.category_guard=INT32_MAX;
    p4_golden_ai_battle_eligible_start(&battle);
    CHECK(battle.category_guard==INT32_MIN);
    p4_golden_ai_battle_assist(&battle,1); CHECK(p4_golden_ai_assist_selector(&battle)==0x21);
    args[0]=0x10003; CHECK(call(8479) && p4_golden_ai_assist_selector(&battle)==0x22);
    args[0]=2; CHECK(call(8479) && p4_golden_ai_assist_selector(&battle)==0x20);
    entities[0].message_parameter=99; entities[0].message_flags=0xa7;
    CHECK(p4_golden_ai_message_wait_latch(entities)==1 && entities[0].message_flags==0xa5);
    entities[0].message_parameter=0; entities[0].message_flags=0xa7;
    CHECK(p4_golden_ai_message_wait_latch(entities)==0 && entities[0].message_flags==0xa7);
    puts("golden_ai: command, dependency, write-set, draw-count and PS2 no-op vectors pass");
    return 0;
}
