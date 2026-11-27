#include "../src/platform/golden/golden_vm.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while (0)
static uint8_t code[256], procedures[64], labels[64];
static size_t words;
static void emit(unsigned op, int operand)
{
    uint32_t n = op | (uint32_t)(uint16_t)operand << 16;
    for (unsigned i=0;i<4;++i) code[words*4+i]=(uint8_t)(n>>(i*8));
    ++words;
}
static void imm(uint32_t bits) { emit(bits&65535,(int)(bits>>16)); }
static P4GoldenVm fresh(void)
{
    P4GoldenVm vm={0}; vm.code=code; vm.code_words=64; vm.procedures=procedures;
    vm.procedure_count=2; vm.labels=labels; vm.label_count=2; words=0; return vm;
}
static int64_t top(P4GoldenVm *vm) { int64_t n=0; (void)p4_golden_vm_integer(vm,0,&n); return n; }
int main(void)
{
    P4GoldenVm vm=fresh();
    emit(0,0); imm(0xffffffff); emit(29,-32768);
    CHECK(p4_golden_vm_run(&vm,2)==0 && vm.pc==3 && vm.depth==2 && top(&vm)==-32768);
    vm=fresh(); emit(0,0); imm(0x80000000); emit(18,0);
    CHECK(!p4_golden_vm_run(&vm,2) && top(&vm)==INT64_C(2147483648));
    static const int ops[]={14,15,16,17,20,21,22,23,24,25,26,27};
    static const int expected[]={3,-1,2,0,1,1,0,1,1,0,1,0};
    for (unsigned i=0;i<12;++i) {
        vm=fresh(); emit(29,2); emit(29,1); emit((unsigned)ops[i],0);
        CHECK(!p4_golden_vm_run(&vm,3) && top(&vm)==expected[i]);
    }
    vm=fresh(); emit(29,0); emit(29,7); emit(17,0);
    CHECK(!p4_golden_vm_run(&vm,3) && !top(&vm) && vm.anomalies==1);
    vm=fresh(); p4_golden_vm_return_integer(&vm,INT64_C(4294967296)); emit(4,0); emit(19,0);
    CHECK(!p4_golden_vm_run(&vm,2) && !top(&vm));
    vm=fresh(); p4_golden_vm_return_integer(&vm,INT64_C(4294967296)); emit(29,0); emit(4,0); emit(22,0);
    CHECK(!p4_golden_vm_run(&vm,3) && top(&vm)==1);
    int64_t li[2]={0},gi[2]={0}; float lf[2]={0},gf[2]={0};
    vm=fresh(); vm.local=(P4GoldenVmBank){li,lf,2,2}; vm.global=(P4GoldenVmBank){gi,gf,2,2};
    p4_golden_vm_return_integer(&vm,INT64_C(0x123456789abcdef0));
    emit(4,0); emit(32,0); emit(30,0); emit(5,0); emit(2,0);
    CHECK(!p4_golden_vm_run(&vm,5) && top(&vm)==INT64_C(0x123456789abcdef0) && gi[0]==li[0]);
    vm=fresh(); vm.local=(P4GoldenVmBank){li,lf,2,2};
    emit(1,0); imm(0x7f800000); emit(32,0); emit(30,0);
    CHECK(!p4_golden_vm_run(&vm,3) && top(&vm)==INT64_MAX && vm.anomalies==1);
    vm=fresh(); emit(1,0); imm(0x7fc00000); emit(19,0);
    CHECK(!p4_golden_vm_run(&vm,2) && top(&vm)==0);
    vm=fresh(); vm.global=(P4GoldenVmBank){gi,gf,2,2}; gi[0]=INT64_C(4294967297);
    vm.depth=1; vm.stack[0]=(P4GoldenVmValue){0,2}; float f;
    CHECK(p4_golden_vm_float(&vm,0,&f) && f==1 && top(&vm)==INT64_C(4294967297));
    emit(6,0); CHECK(!p4_golden_vm_run(&vm,1) && gf[0]==4294967296.0f);
    vm=fresh(); procedures[24]=2; emit(11,0); emit(9,0); emit(9,0);
    CHECK(p4_golden_vm_run(&vm,3)==2 && vm.depth==0 && vm.pc==1);
    vm=fresh(); labels[24]=3; p4_golden_vm_return_integer(&vm,INT64_C(4294967296));
    emit(4,0); emit(28,0); emit(29,7); emit(9,0);
    CHECK(p4_golden_vm_run(&vm,3)==2 && vm.depth==0 && vm.pc==3);
    vm=fresh(); for (unsigned i=0;i<48;++i) emit(29,1);
    CHECK(!p4_golden_vm_run(&vm,47) && vm.depth==47);
    CHECK(p4_golden_vm_run(&vm,1)==-1 && vm.stack[47].bits==0);
    P4GoldenBfTables tables={0};
    tables.ai[8479&4095]=(P4GoldenBfCommand){0,1,p4_golden_ai_execute};
    tables.ai[8472&4095]=(P4GoldenBfCommand){0,0,p4_golden_ai_execute};
    P4GoldenAiBattle battle={0}; P4GoldenAi ai={0}; ai.battle=&battle;
    vm=fresh(); vm.tables=&tables; vm.ai=&ai; p4_golden_vm_return_integer(&vm,INT64_C(4294967296));
    emit(29,3); emit(8,8479); emit(4,0);
    CHECK(!p4_golden_vm_run(&vm,3) && battle.assist_choreography==3 && top(&vm)==INT64_C(4294967296));
    vm=fresh(); vm.tables=&tables; vm.ai=&ai; battle.category=2;
    emit(8,8472); emit(4,0); CHECK(!p4_golden_vm_run(&vm,2) && top(&vm)==2);
    puts("golden_vm: synthetic wide storage, arithmetic, control, safety and COMM vectors pass");
    return 0;
}
