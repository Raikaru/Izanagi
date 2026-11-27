#include "golden_ai_native.h"
#ifdef P4_GOLDEN_AI_QUALIFICATION
#include "golden_ai_qualification.h"
#include <stdio.h>
#endif
#ifdef P4_GOLDEN_AI_QUALIFICATION
static uint64_t qualification_native_calls;
uint64_t p4_golden_ai_qualification_native_calls(void) { return qualification_native_calls; }
static P4GoldenVm *qualification_vm;
static uint8_t *qualification_entered;
static size_t qualification_count;
static int qualification_synthetic;
static void qualification_call(P4GoldenVm *vm,size_t procedure)
{
    if(vm==qualification_vm && procedure<qualification_count)
        qualification_entered[procedure]=1;
}
#endif
#include "p4pc/gmem.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct NativeVm {
    P4GoldenVm vm;
    uint32_t work,function;
    int32_t timer_pc;
    int bound,running;
} NativeVm;
static NativeVm slots[3];
static int64_t locals[3][800];
static float float_locals[3][8];
static P4GoldenBfTables tables;
static NativeVm *running;
static int tables_ready;
static void require(int condition) { if(!condition) abort(); }
/* Actual PS2 command entry identities, not BF argument guesses.
 * SetSkillAction (001df040) takes an ITEM id and must not be skill-remapped. */
static uint32_t native_argument(uint32_t function,int index,uint32_t value)
{
    int skill=(index==0 && (function==0x001defa0 || function==0x001e5f40 ||
                           function==0x001e64c0)) ||
              (index==1 && function==0x001e6a50);
    if(skill && value<624) {
        uint32_t port;
        require(!p4_golden_domain_to_port(&p4_golden_ai_native_product()->map,
            P4_GOLDEN_DOMAIN_SKILL,value,&port,NULL));
        return port;
    }
    return value;
}
static int64_t native_result(uint32_t function,int32_t value)
{
    switch(function) {
    /* Tagged action-ID writers in the original btlEffect command family.
     * Their unsigned bit31 expression widens to a positive Golden carrier. */
    case 0x001e6d20: case 0x001e6dc0: case 0x001e6ea0: case 0x001e6fa0:
    case 0x001e7080: case 0x001e7160: case 0x001e7240: case 0x001e7320:
    case 0x001e7400: case 0x001e7510: case 0x001e7640: case 0x001e77f0:
    case 0x001e7950: case 0x001e7ab0: case 0x001e7d30: case 0x001e7e30:
    case 0x001e7fb0: case 0x001e8090: case 0x001e8190: case 0x001e8290:
    case 0x001e8390: case 0x001e8490: case 0x001e85c0: case 0x001e86f0:
    case 0x001e8820: case 0x001e8950: case 0x001e8a80: case 0x001e8dd0:
    case 0x001e8eb0: case 0x001e8fe0: case 0x001e9110: case 0x001e9240:
    case 0x001e94e0: case 0x001e9600: case 0x001e9770: case 0x001e9f20:
    case 0x001ea050: case 0x001ea180:
        return value==-1?-1:(int64_t)(uint32_t)value;
    case 0x001e8bb0: case 0x001e8cd0: case 0x001e9950:
        if(value>=0 && value<624) {
            uint32_t golden;
            require(!p4_golden_domain_from_port(&p4_golden_ai_native_product()->map,
                P4_GOLDEN_DOMAIN_SKILL,(uint32_t)value,&golden));
            return golden;
        }
        break;
    }
    return value;
}
static NativeVm *for_work(uint32_t work)
{
    uint32_t pool=P4_G32(0x00764610),first=pool+0x18;
    if(!pool || work<first || (work-first)%0x168 || (work-first)/0x168>=3) return NULL;
    return &slots[(work-first)/0x168];
}
static P4GoldenVm *current(void)
{
    if(!p4_golden_ai_native_product()) return NULL;
    uint32_t work=P4_G32(0x00764600);
    NativeVm *slot=for_work(work);
    return slot && slot->bound && slot->running && running==slot &&
        slot->work==work?&slot->vm:NULL;
}
int p4_golden_ai_native_argument_integer(int index,int32_t *value)
{
    P4GoldenVm *vm=current(); if(!vm) return 0;
    if(!value) abort();
    int64_t wide;
    if(!p4_golden_vm_integer(vm,index,&wide)) abort();
    uint32_t low=native_argument(running->function,index,(uint32_t)wide);
    memcpy(value,&low,4); return 1;
}
int p4_golden_ai_native_argument_float(int index,float *value)
{
    P4GoldenVm *vm=current(); if(!vm) return 0;
    require(p4_golden_vm_float(vm,index,value)); return 1;
}
int p4_golden_ai_native_argument_string(int index,const char **value)
{
    P4GoldenVm *vm=current(); if(!vm) return 0;
    require(value!=NULL);
    const char *text=p4_golden_vm_string(vm,index);
    const P4GoldenAiProduct *product=p4_golden_ai_native_product();
    unsigned script=vm->strings==product->scripts[0].strings?0:1;
    const P4GoldenVmProgram *program=product->scripts+script;
    require(text && vm->strings==program->strings);
    uintptr_t offset=(uintptr_t)text-(uintptr_t)program->bytes;
    uint32_t header=P4_GPDATA(uint32_t,script?0xb42c:0xb428);
    require(offset<program->size && header && header<P4_GUEST_SIZE &&
        program->size<=P4_GUEST_SIZE-header);
    /* Native consumers may store this pointer in guest fields. Return the
     * verified guest copy, never a host-cache pointer that H2G would truncate. */
    *value=P4_G2H(header+offset); return 1;
}
int p4_golden_ai_native_return_integer(int32_t value)
{
    P4GoldenVm *vm=current(); if(!vm) return 0;
    p4_golden_vm_return_integer(vm,native_result(running->function,value)); return 1;
}
int p4_golden_ai_native_return_float(float value)
{
    P4GoldenVm *vm=current(); if(!vm) return 0;
    p4_golden_vm_return_float(vm,value); return 1;
}
static int native(P4GoldenVm *vm,uint32_t function,uint32_t command)
{
    (void)command;
    require(running && &running->vm==vm);
    uint32_t work=running->work;
    if(running->timer_pc!=vm->pc) P4_G32(work+0x138)=0;
#ifdef P4_GOLDEN_AI_QUALIFICATION
    ++qualification_native_calls;
#endif
    P4_G32(work+0x18)=(uint32_t)vm->pc;
    P4_G32(work+0x1c)=(uint32_t)vm->depth;
    P4_G32(0x00764600)=work;
    uint32_t (*call)(void)=(uint32_t (*)(void))P4_FN_G2H(function);
    require(call!=NULL);
    uint32_t previous=running->function; running->function=function;
    int success=call()!=0;
    running->function=previous;
    vm->pc=(int32_t)P4_G32(work+0x18);
    return success;
}
static int golden(P4GameMode mode,P4GoldenAi *unused,uint32_t command,
    const int64_t *arguments,P4GoldenAiResult *result)
{
    (void)unused; require(running!=NULL);
    P4GoldenAi *ai;
    uint32_t actor=P4_G32(running->work+0x154);
    require(!p4_golden_ai_native_refresh(actor,command,&ai));
    int handled=p4_golden_ai_execute(mode,ai,command,arguments,result);
    require(handled);
    p4_golden_ai_native_commit(actor,command,result->value);
    /* Original native continuations read the PS2 result slot. It mirrors only
     * low32/type; the separate VM result above remains the wide authority. */
    P4_G8(running->work+0x4f)=(uint8_t)result->type;
    P4_G32(running->work+0x10c)=(uint32_t)result->value;
    return handled;
}
int p4_golden_vm_native_prepare(void)
{
    static const uint32_t counts[]={121,51,276,61};
    P4GoldenBfNativeCommand commands[509]; P4GoldenBfNativeGroup original[4];
    size_t first=0;
    for(unsigned group=0;group<4;++group) {
        uint32_t base=P4_G32(0x00748530+group*8);
        if(!base || P4_G32(0x00748534+group*8)!=counts[group]) return -1;
        original[group]=(P4GoldenBfNativeGroup){commands+first,counts[group]};
        for(unsigned i=0;i<counts[group];++i)
            commands[first+i]=(P4GoldenBfNativeCommand){P4_G32(base+i*8),P4_G32(base+i*8+4)};
        first+=counts[group];
    }
    if(p4_golden_bf_tables(P4_GAME_MODE_GOLDEN,&tables,original)!=1) return -1;
    P4GoldenBfCommand *groups[]={tables.common,tables.field,tables.ai,tables.battle};
    const unsigned sizes[]={192,51,294,61};
    for(unsigned group=0;group<4;++group)
        for(unsigned i=0;i<sizes[group];++i) if(groups[group][i].golden) groups[group][i].golden=golden;
#ifdef P4_GOLDEN_AI_QUALIFICATION
    const P4GoldenAiProduct *product=p4_golden_ai_native_product();
    if(!product) return -1;
    for(unsigned script=0;script<2;++script) {
        const P4GoldenVmProgram *program=product->scripts+script;
        size_t commands_seen=0,unresolved=0;
        for(size_t pc=0;pc<program->code_words;++pc) {
            const uint8_t *p=program->code+pc*4;
            unsigned op=(unsigned)p[0]|(unsigned)p[1]<<8;
            if(op==0 || op==1) ++pc; /* The next word is literal data. */
            else if(op==8) {
                uint32_t command=(uint32_t)((unsigned)p[2]|(unsigned)p[3]<<8);
                const P4GoldenBfCommand *entry=p4_golden_bf_resolve(&tables,command);
                ++commands_seen;
                if(!entry || (!entry->golden && !entry->native_function)) {
                    ++unresolved;
                    printf("qualification unresolved block=%u pc=%zu command=%u\n",script+9,pc,command);
                }
            }
        }
        printf("qualification closure AICALC%u commands=%zu unresolved=%zu\n",script+9,commands_seen,unresolved);
        if(unresolved) return -1;
    }
#endif
    tables_ready=1; return 0;
}
static int run(void *pointer,uint32_t *result,size_t steps)
{
    const P4GoldenAiProduct *product=p4_golden_ai_native_product();
    if(!product) return 0;
    uint32_t work=P4_H2G(pointer),header=P4_G32(work+0x110);
    unsigned script;
    if(header==P4_GPDATA(uint32_t,0xb428)) script=0;
    else if(header==P4_GPDATA(uint32_t,0xb42c)) script=1;
    else return 0;
    NativeVm *slot=for_work(work);
    if(!tables_ready || !slot || slot->running || !result) return -1;
    if(!slot->bound) {
        uint32_t pool=P4_G32(0x00764614),first=pool+0x18,local=P4_G32(work+0x140);
        if(!pool || local<first || (local-first)%0x348 || (local-first)/0x348>=3) return -1;
        unsigned local_index=(local-first)/0x348;
        if(p4_golden_vm_start(&slot->vm,&product->scripts[script],0,
            (P4GoldenVmBank){locals[local_index],float_locals[local_index],800,8},
            (P4GoldenVmBank){0},&tables,NULL)) return -1;
        slot->vm.pc=(int32_t)P4_G32(work+0x18); slot->vm.native=native;
#ifdef P4_GOLDEN_AI_QUALIFICATION
        slot->vm.call=qualification_call;
        if(qualification_synthetic && &slot->vm==qualification_vm) {
            for(size_t i=0;i<product->scripts[script].local_integers;++i)
                slot->vm.local.integers[i]=1;
            for(size_t i=0;i<product->scripts[script].local_floats;++i)
                slot->vm.local.floats[i]=1.0f;
        }
#endif
        slot->work=work; slot->bound=1; slot->timer_pc=-1;
    }
    NativeVm *previous=running; running=slot; slot->running=1;
    int status=p4_golden_vm_run(&slot->vm,steps);
    slot->running=0; running=previous;
    P4_G32(work+0x18)=(uint32_t)slot->vm.pc;
    /* A native control consumer sees only its original s32 mirror; wide values
     * stay in the separate engine and are never routed through this carrier. */
    P4_G32(work+0x1c)=(uint32_t)slot->vm.depth;
    if(status==1) {
        ++P4_G32(work+0x138); ++P4_G32(work+0x134);
        slot->timer_pc=slot->vm.pc; *result=1; return 1;
    }
    if(status!=2) return -1;
    P4_G32(work+0x138)=0; slot->timer_pc=-1; *result=2; return 1;
}
void p4_golden_ai_native_work_release(void *pointer)
{
    if(!p4_golden_ai_native_product()) return;
    uint32_t payload=P4_H2G(pointer),pool=P4_G32(0x00764614),first=pool+0x18;
    /* Native scrPoolFree clears a successful local-pool payload. Mirror that
     * actual writer for the separate wide bank; startup itself does not clear. */
    if(pool && payload>=first && (payload-first)%0x348==0 && (payload-first)/0x348<3) {
        unsigned index=(payload-first)/0x348;
        for(unsigned i=0;i<3;++i)
            require(!slots[i].running || slots[i].vm.local.integers!=locals[index]);
        memset(locals[index],0,sizeof locals[index]);
        memset(float_locals[index],0,sizeof float_locals[index]);
        return;
    }
    NativeVm *slot=for_work(payload);
    if(!slot || !slot->bound) return;
    require(!slot->running); memset(slot,0,sizeof *slot);
}
int p4_golden_vm_native_quiescent(void)
{
    if(running) return 0;
    for(unsigned i=0;i<3;++i) if(slots[i].bound || slots[i].running) return 0;
    return 1;
}
void p4_golden_vm_native_reset(void)
{
    require(p4_golden_vm_native_quiescent());
    memset(slots,0,sizeof slots); memset(locals,0,sizeof locals);
    memset(float_locals,0,sizeof float_locals); memset(&tables,0,sizeof tables); tables_ready=0;
}
int p4_golden_ai_native_run(void *pointer,uint32_t *result)
{
    return run(pointer,result,SIZE_MAX);
}
#ifdef P4_GOLDEN_AI_QUALIFICATION
int p4_golden_ai_qualification_run(void *work,uint32_t *result,size_t steps,
    uint32_t *pc,uint32_t *command,uint8_t *entered,size_t count,int synthetic_locals)
{
    NativeVm *slot=for_work(P4_H2G(work));
    const P4GoldenAiProduct *product=p4_golden_ai_native_product();
    uint32_t header=P4_G32(P4_H2G(work)+0x110);
    unsigned script=header==P4_GPDATA(uint32_t,0xb428)?0:1;
    if(!slot || !product || !entered || !pc || !command || qualification_vm ||
       header!=P4_GPDATA(uint32_t,script?0xb42c:0xb428) ||
       count!=product->scripts[script].procedure_count) return -1;
    qualification_vm=&slot->vm; qualification_entered=entered;
    qualification_count=count; qualification_synthetic=synthetic_locals;
    int status=run(work,result,steps);
    qualification_vm=NULL; qualification_entered=NULL;
    qualification_count=0; qualification_synthetic=0;
    if(!slot || !slot->bound) return -1;
    *pc=(uint32_t)slot->vm.pc;
    *command=UINT32_MAX;
    if(slot->vm.pc>=0 && (size_t)slot->vm.pc<slot->vm.code_words) {
        const uint8_t *p=slot->vm.code+(size_t)slot->vm.pc*4;
        if(p[0]==8 && !p[1]) *command=(uint32_t)p[2]|(uint32_t)p[3]<<8;
    }
    return status;
}
#endif
#ifdef P4_GOLDEN_AI_QUALIFICATION
int p4_golden_ai_qualification_command(void *pointer,uint32_t function,
    const int64_t *arguments,size_t count,int64_t *result)
{
    NativeVm *slot=for_work(P4_H2G(pointer));
    if(!tables_ready || !slot || slot->bound || !result || count>47 ||
        (count && !arguments)) return -1;
    const P4GoldenBfCommand *groups[]={tables.common,tables.field,tables.ai,tables.battle};
    const unsigned sizes[]={192,51,294,61};
    uint32_t command=UINT32_MAX;
    for(unsigned group=0;group<4;++group)
        for(unsigned i=0;i<sizes[group];++i)
            if(groups[group][i].native_function==function && groups[group][i].arity==count)
                command=group*4096+i;
    if(command==UINT32_MAX) return -1;
    const P4GoldenAiProduct *product=p4_golden_ai_native_product();
    uint32_t header=P4_G32(P4_H2G(pointer)+0x110);
    unsigned script=header==P4_GPDATA(uint32_t,0xb428)?0:1;
    if(!product || header!=P4_GPDATA(uint32_t,script?0xb42c:0xb428)) return -1;
    const P4GoldenVmProgram *program=product->scripts+script;
    /* Invented ABI vectors, separate from the actual native invocation below. */
    if(native_argument(0x001defa0,0,440)!=576 ||
       native_argument(0x001df040,0,440)!=440 ||
       native_result(0x001e8bb0,576)!=440 ||
       native_result(0x001e8cd0,440)!=472 ||
       native_result(0x001e6fa0,-1)!=-1) return -1;
    uint32_t bits=UINT32_C(0x80000001); int32_t encoded;
    memcpy(&encoded,&bits,4);
    if(native_result(0x001e6fa0,encoded)!=INT64_C(2147483649)) return -1;
    memset(slot,0,sizeof *slot);
    slot->bound=slot->running=1; slot->work=P4_H2G(pointer); slot->timer_pc=-1;
    slot->vm.depth=(int32_t)count;
    slot->vm.strings=program->strings;
    for(size_t i=0;i<count;++i)
        slot->vm.stack[count-1-i]=(P4GoldenVmValue){(uint64_t)arguments[i],0};
    NativeVm *previous=running; running=slot;
    int success=native(&slot->vm,function,command);
    int vectors=1;
    if(program->string_size) {
        slot->vm.depth=1;
        slot->vm.stack[0]=(P4GoldenVmValue){(uintptr_t)program->strings,5};
        const char *text=NULL;
        vectors=p4_golden_ai_native_argument_string(0,&text)==1 &&
            text==P4_G2H(header+(program->strings-program->bytes));
    }
    memcpy(result,&slot->vm.stack[47].bits,8);
    running=previous; memset(slot,0,sizeof *slot);
    return vectors?(success?0:1):-1;
}
#endif
