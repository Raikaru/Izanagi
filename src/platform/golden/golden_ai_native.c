/* Native-owned Golden views. Refresh from the live roster/equipment/action
 * owners for every command; commit only the command's specified write set. */
#include "golden_ai_native.h"
#include "golden_ai_rng_native.h"
#include "golden_ai_knowledge.h"
#include "p4pc/gmem.h"
#include <stdlib.h>
#include <string.h>

typedef struct LiveActor {
    struct LiveActor *owned_next;
    uint32_t guest;
    P4GoldenAiEntity entity;
    P4GoldenAiUnit unit;
    P4GoldenAiItem equipment[3];
    uint16_t skills[8], affinity[16];
} LiveActor;
static const P4GoldenAiProduct *active, *staged;
static LiveActor *actors;
static uint32_t battle_guest;
static P4GoldenAiBattle battle;
static P4GoldenAi ai;
static uint16_t half(const uint8_t *p) { return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
static uint32_t word(const uint8_t *p) { return (uint32_t)half(p)|(uint32_t)half(p+2)<<16; }
static void require(int condition) { if(!condition) abort(); }
_Noreturn void p4_golden_ai_native_fail(void)
{
    p4_package_boot_runtime_failed("Malformed Golden AI work");
}
static LiveActor *at(uint32_t guest)
{
    for(LiveActor *a=actors;a;a=a->owned_next) if(a->guest==guest) return a;
    return NULL;
}
const P4GoldenAiProduct *p4_golden_ai_native_product(void) { return active; }
int p4_golden_ai_native_stage(const P4GoldenAiProduct *product)
{
#ifndef P4_GOLDEN_AI_QUALIFICATION
    /* A real PS2 session cannot acquire a Golden native owner. The explicitly
     * dev-only hybrid executable is refused by normal boot instead. */
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN) return -1;
#endif
    if(staged) return staged==product?0:-1;
    if(active || actors || battle_guest || p4_golden_ai_product_acquire(product)) return -1;
    staged=product; return 0;
}
void p4_golden_ai_native_activate(const P4GoldenAiProduct *product)
{
    if(active==product) return;
    require(staged==product && !active);
    active=staged; staged=NULL;
}
void p4_golden_ai_native_action_init(void *action)
{
    if(!active) return;
    uint32_t guest=P4_H2G(action);
    require(guest && !at(guest));
    LiveActor *a=calloc(1,sizeof *a); require(a!=NULL);
    a->guest=guest; a->entity.unit=&a->unit;
    a->unit.skills=a->skills; a->unit.affinity=a->affinity;
    for(unsigned i=0;i<3;++i) a->unit.equipment[i]=a->equipment+i;
    a->owned_next=actors; actors=a;
}
void p4_golden_ai_native_action_release(void *action)
{
    if(!active) return;
    uint32_t guest=P4_H2G(action); LiveActor **link=&actors;
    while(*link && (*link)->guest!=guest) link=&(*link)->owned_next;
    require(*link!=NULL);
    LiveActor *removed=*link; *link=removed->owned_next;
    for(LiveActor *a=actors;a;a=a->owned_next) {
        if(a->entity.selected==&removed->entity) a->entity.selected=NULL;
        for(unsigned i=0;i<12;++i)
            if(a->entity.candidates[i]==&removed->entity) a->entity.candidates[i]=NULL;
    }
    free(removed);
}
void p4_golden_ai_native_battle_init(void)
{
    if(!active) return;
    require(!battle_guest && !actors);
    battle_guest=P4_GPDATA(uint32_t,0xb3ac); require(battle_guest!=0);
    memset(&battle,0,sizeof battle);
}
void p4_golden_ai_native_battle_release(void)
{
    if(!active) return;
    require(!actors);
    battle_guest=0; memset(&battle,0,sizeof battle); memset(&ai,0,sizeof ai);
}
void p4_golden_ai_native_eligible_start(void *action)
{
    if(!active) return;
    require(battle_guest!=0);
    uint32_t guest=P4_H2G(action);
    P4GoldenAi *view;
    require(!p4_golden_ai_native_refresh(guest,8474,&view));
    if(!P4_G8(guest+0x28) && !P4_G8(guest+0x29) &&
        p4_golden_ai_recovery_mask(view->actor->unit)) return;
    p4_golden_ai_battle_eligible_start(&battle);
}
void p4_golden_ai_native_continuous_entry(void)
{
    if(active) { require(battle_guest!=0); p4_golden_ai_battle_continuous_entry(&battle); }
}
static int unit_view(LiveActor *a,int maximums)
{
    uint32_t visual=P4_G32(a->guest+0x30), calc=visual?P4_G32(visual+0xa64):0;
    if(!calc) return -1;
    P4GoldenAiUnit *u=&a->unit;
    u->flags=P4_G16(calc); u->id=P4_G16(calc+2); u->affinity_id=P4_G16(calc+4);
    u->hp=P4_G16(calc+8); u->sp=P4_G16(calc+10); u->status=P4_G32(calc+12);
    u->side=P4_G8(visual+0xa2); u->character_id=P4_G16(visual+0xa4);
    u->target_flags=(uint8_t)P4_G32(visual+0x9c);
    u->secondary[0]=P4_G8(calc+0x22); u->secondary[1]=P4_G8(calc+0x23);
    const uint8_t *skills, *affinity;
    if(u->flags&4) {
        if(u->id>=368 || u->affinity_id>=368) return -1;
        const uint8_t *row=active->unit[0].bytes+(size_t)u->id*60;
        skills=row+14; affinity=active->unit[1].bytes+(size_t)u->affinity_id*32;
        u->base_hp=half(row+4); u->base_sp=half(row+6);
        u->enemy_flags=row[0]; u->enemy_element=row[0x38];
        for(unsigned i=0;i<8;++i) a->skills[i]=half(skills+i*2);
        memset(a->equipment,0,sizeof a->equipment);
    } else {
        uint8_t *persona=datPersonaGetRecord(u->id);
        if(!persona || half(persona+2)>=256) return -1;
        u->affinity_id=half(persona+2);
        affinity=active->unit[2].bytes+(size_t)u->affinity_id*32;
        skills=persona+12;
        for(unsigned i=0;i<8;++i) {
            uint32_t golden;
            if(p4_golden_domain_from_port(&active->map,P4_GOLDEN_DOMAIN_SKILL,half(skills+i*2),&golden)) return -1;
            a->skills[i]=(uint16_t)golden;
        }
        for(unsigned i=0;i<3;++i) {
            int16_t id=datGetEquippedItemByUnit((int16_t)u->id,(int16_t)i);
            u->equipment_ids[i]=id;
            if(id<0 && i==2) { memset(a->equipment+i,0,sizeof a->equipment[i]); continue; }
            if((uint16_t)id>=2560) return -1;
            const uint8_t *item=active->raw[P4_GAI_ITEM].data+16+(size_t)(uint16_t)id*68;
            a->equipment[i]=(P4GoldenAiItem){word(item+0x24),half(item+0x28),half(item+0x2c)};
        }
        if(maximums) {
            unsigned level=u->id==1?P4_G8(calc+6):persona[4];
            if(p4_golden_levels_pair(active->raw[P4_GAI_LEVELS].data,
                active->raw[P4_GAI_LEVELS].size,(int)level,u->id,&u->base_hp,&u->base_sp)) return -1;
        }
    }
    for(unsigned i=0;i<16;++i) a->affinity[i]=half(affinity+i*2);
    a->entity.id=P4_G32(a->guest+8); a->entity.flags=P4_G16(a->guest+0x1a);
    a->entity.message_parameter=P4_G16(a->guest+0xec);
    a->entity.action_kind=P4_G16(a->guest+0x6c);
    return 0;
}
int p4_golden_ai_native_refresh(uint32_t actor,uint32_t command,P4GoldenAi **out)
{
    if(!active || !battle_guest || !out) return -1;
    LiveActor *current=at(actor); if(!current) return -1;
    uint32_t next=P4_G32(battle_guest+0x174); LiveActor *previous=NULL;
    size_t visited=0,total=0;
    for(LiveActor *a=actors;a;a=a->owned_next) ++total;
    ai.head=NULL;
    while(next) {
        LiveActor *a=at(next);
        if(!a || ++visited>total || unit_view(a,command==8471 || command==8473)) return -1;
        if(previous) previous->entity.next=&a->entity; else ai.head=&a->entity;
        previous=a; next=P4_G32(next+0x450);
    }
    if(previous) previous->entity.next=NULL;
    if(p4_golden_ai_native_streams(&ai.additive,&ai.sfmt)) return -1;
    ai.actor=&current->entity; ai.battle=&battle;
    ai.skill_rows=active->skill[1].bytes; ai.skill_elements=active->skill[0].bytes;
    ai.knowledge=p4_golden_ai_native_knowledge_words();
    ai.knowledge_words=P4_GOLDEN_KNOWLEDGE_WORDS;
    if(!ai.knowledge) return -1;
    battle.flags=(P4_G32(battle_guest+0x0c)&~UINT32_C(0x20000000))|
        (battle.flags&UINT32_C(0x20000000));
    battle.category=P4_G16(battle_guest+0x1a);
    *out=&ai; return 0;
}
void p4_golden_ai_native_commit(uint32_t actor,uint32_t command,int64_t result)
{
    LiveActor *a=at(actor); require(a!=NULL);
    if(command==8469 || command==8481) {
        P4_G16(actor+0xec)=a->entity.message_parameter;
        btlAICommandSetActionCode(P4_G2H(actor),P4_G2H(actor+0x38),command==8469?0x8007:0x8002);
    } else if(command==8485) {
        /* Candidate work is replaced even when filtering finds no survivor. */
        P4_G16(actor+0xd0)=a->entity.candidate_count;
        for(unsigned i=0;i<12;++i) {
            uint32_t guest=0;
            if(a->entity.candidates[i]) {
                for(LiveActor *v=actors;v;v=v->owned_next)
                    if(&v->entity==a->entity.candidates[i]) { guest=v->guest; break; }
                require(guest!=0);
            }
            P4_G32(actor+0x98+i*4)=guest;
        }
        /* Failure is -1, not zero. A stale selection flag is not a new write. */
        if(result!=-1) {
            LiveActor *selected=NULL;
            for(LiveActor *v=actors;v;v=v->owned_next)
                if(&v->entity==a->entity.selected) { selected=v; break; }
            require(selected!=NULL);
            P4_G32(actor+0x38)=selected->guest; P4_G16(actor+0x6a)=1;
        }
    }
}
int p4_golden_ai_native_session_end(void)
{
    if(!active && !staged) return 0;
    if(actors || battle_guest || !p4_golden_vm_native_quiescent()) return -1;
    if(p4_golden_ai_knowledge_unregister()) return -1;
    if(active) p4_golden_ai_product_detach(active);
    else if(staged) p4_golden_ai_product_detach(staged);
    active=staged=NULL; p4_golden_vm_native_reset(); return 0;
}
uint16_t p4_golden_ai_native_wait_kind(void *action,uint16_t kind)
{
    (void)action;
    return active && kind==8?7:kind;
}
void p4_golden_ai_native_message_wait(void *action)
{
    if(!active) return;
    LiveActor *actor=at(P4_H2G(action)); require(actor!=NULL);
    actor->entity.message_parameter=P4_G16(actor->guest+0xec);
    /* Snapshot consumption is owned here. The M11 follow-up packet mechanic
     * is absent, so no packet is fabricated or queued from this latch yet. */
    (void)p4_golden_ai_message_wait_latch(&actor->entity);
}
void p4_golden_ai_native_bit29(uint32_t flags)
{
    if(active) battle.flags=(battle.flags&~UINT32_C(0x20000000))|(flags&UINT32_C(0x20000000));
}
