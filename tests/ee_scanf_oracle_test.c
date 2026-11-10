/* Original-ELF decimal input/scanner oracle. No substituted retail calls. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/hostmath.h"
#include "p4pc/ps2_libc.h"
#include "p4pc/tables.h"
#include "ee_decimal.h"
#include "r5900.h"
const p4pc_patch_t p4pc_patch_table[]={};const size_t p4pc_patch_count=0;
const p4pc_alias_t p4pc_alias_table[]={};const size_t p4pc_alias_count=0;
const p4pc_native_t p4pc_native_table[]={};const size_t p4pc_native_count=0;
const p4pc_hostfn_t p4pc_hostfn_table[]={};const size_t p4pc_hostfn_count=0;
extern int func_00442100(const char *,const char *,...);
extern int func_00446108(uint32_t *,uint32_t *,const char *,const void *);
static int trace;
static unsigned cases;
static uint32_t retail_r;
static R5900 setup(R5900KernelEnv *env)
{
    R5900 c={0};c.lo_pc=0x100000;c.hi_pc=0x5e4000;c.max_steps=10000000;
    c.kernel=env;c.r[28]=0x7690f0;c.r[29]=0x1f00000;return c;
}
static int call(R5900 *c,uint32_t address)
{
    int rc=r5900_call(c,address);
    if(rc!=R5900_OK){fprintf(stderr,"oracle address=%08x rc=%d pc=%08x ins=%08x steps=%lu reason=%s\n",address,rc,c->bad_pc,c->bad_word,c->steps,c->fault_why?c->fault_why:"none");return 1;}
    return 0;
}
static int decimal(uint32_t *native_r,const char *text)
{
    if(trace)fprintf(stderr,"case%u strtod [%s]\n",cases,text);
    char *input=P4_G2H(0x1c01000);strcpy(input,text);
    uint32_t *tail=P4_G2H(0x1c00000),*rr=P4_G2H(retail_r);
    *tail=0xdeadbeef;rr[0]=native_r[0]=73;
    R5900KernelEnv env={0x10001,0x1c00000};R5900 c=setup(&env);
    c.r[4]=retail_r;c.r[5]=0x1c01000;c.r[6]=0x1c00000;
    if(call(&c,0x443068))return 1;
    if(trace)fprintf(stderr," retail=%016"PRIx64"\n",c.r[2]);
    const char *end;uint64_t bits=p4_decimal_strtod(native_r,input,&end);++cases;
    if(bits!=c.r[2]||(uint32_t)(end-input)!=*tail-0x1c01000||native_r[0]!=rr[0]){
        fprintf(stderr,"strtod [%s] native=%016"PRIx64"/%td/%u retail=%016"PRIx64"/%u/%u\n",text,bits,end-input,native_r[0],c.r[2],*tail-0x1c01000,rr[0]);return 1;
    }
    uint32_t end_words[3]={0xa5a5a5a5,0xdeadbeef,0xa5a5a5a5};
    native_r[0]=73;
    bits=func_00443068(native_r,input,&end_words[1]);++cases;
    if(bits!=c.r[2]||end_words[1]!=*tail||end_words[0]!=0xa5a5a5a5||
       end_words[2]!=0xa5a5a5a5||native_r[0]!=rr[0]){
        fprintf(stderr,"strtod bound entry/end-slot [%s] mismatch\n",text);return 1;
    }
    return 0;
}
static int scanner(uint32_t *native_r,const char *text,const char *format)
{
    if(trace)fprintf(stderr,"case%u scanf [%s] [%s]\n",cases,text,format);
    char *input=P4_G2H(0x1c01000),*fmt=P4_G2H(0x1c02000);
    strcpy(input,text);strcpy(fmt,format);
    unsigned char *retail=P4_G2H(0x1c03000),*native=P4_G2H(0x3401000);
    memset(retail,0xa5,128);memset(native,0xa5,128);
    uint32_t *rr=P4_G2H(retail_r);rr[0]=native_r[0]=73;
    R5900KernelEnv env={0x10001,0x1c00000};R5900 c=setup(&env);
    c.r[4]=0x1c01000;c.r[5]=0x1c02000;
    for(int i=0;i<6;++i)c.r[6+i]=0x1c03000+16*i;
    if(call(&c,0x442100))return 1;
    *(uint32_t *)P4_G2H(0x710070)=P4_H2G(native_r);
    int result=func_00442100(input,fmt,native,native+16,native+32,native+48,native+64,native+80);
    *(uint32_t *)P4_G2H(0x710070)=retail_r;
    ++cases;
    if(result!=(int32_t)c.r[2]||memcmp(native,retail,128)||native_r[0]!=rr[0]){
        fprintf(stderr,"scanf [%s] [%s] native=%d retail=%d errno=%u/%u\n",text,format,result,(int32_t)c.r[2],native_r[0],rr[0]);
        for(int i=0;i<96;++i)if(native[i]!=retail[i])fprintf(stderr," byte%d=%02x/%02x",i,native[i],retail[i]);
        fputc('\n',stderr);return 1;
    }
    memset(native,0xa5,128);memset(retail,0xa5,128);
    rr[0]=native_r[0]=73;c=setup(&env);
    c.r[4]=retail_r;c.r[5]=0x1c01000;c.r[6]=0x1c02000;
    for(int i=0;i<5;++i)c.r[7+i]=0x1c03000+16*i;
    if(call(&c,0x442188))return 1;
    /* Explicit reentrancy must not write the global retail errno block. */
    int reentrant=func_00442188(native_r,input,fmt,native,native+16,native+32,native+48,native+64);
    if(reentrant!=(int32_t)c.r[2]||memcmp(native,retail,128)||native_r[0]!=rr[0]){
        fprintf(stderr,"reentrant scanf [%s] [%s] mismatch\n",text,format);return 1;
    }
    ++cases;
    /* Same destinations through the packed guest va-list and FILE entry. */
    uint32_t file[24]={0};uint64_t packed[6];memset(native,0xa5,128);
    for(int i=0;i<6;++i)packed[i]=P4_H2G(native+16*i);
    file[0]=P4_H2G(input);file[1]=(uint32_t)strlen(input);native_r[0]=73;
    int packed_result=func_00446108(native_r,file,fmt,packed);
    if(packed_result!=result||memcmp(native,retail,128)||native_r[0]!=rr[0])return 1;
    ++cases;
    return 0;
}
/* The tokenizer's bytes, return pointer and four-byte save state are part of
 * the CRT contract too. Run both original entry points without any call hooks. */
static int tokenizer(uint32_t *native_r)
{
    static const char *inputs[] = {"", ",,,", "a", "a,b,c", ",a,,b,",
                                  "a b:c,d", "\200,\377,\200", "   x\t y  "};
    static const char *delimiters[] = {",", ":,", " \t", ""};
    char *input = P4_G2H(0x1c01000), *delimiter = P4_G2H(0x1c02000);
    uint32_t *retail_save = P4_G2H(0x1c00000);
    for (unsigned n = 0; n < sizeof inputs / sizeof inputs[0]; ++n) {
        for (unsigned changing = 0; changing < 2; ++changing) {
            for (unsigned wrapper = 0; wrapper < 2; ++wrapper) {
                uint32_t native_save[3] = {0x12345678, 0, 0x87654321};
                uint32_t before_left = native_r[0x58 / 4], before_right = native_r[0x60 / 4];
                uint32_t *save = wrapper ? native_r + 0x5c / 4 : native_save + 1;
                *save = 0;
                memset(input, 0xa5, 128);
                strcpy(input, inputs[n]);
                for (unsigned step = 0; step < 8; ++step) {
                    unsigned char before[128], expected[128];
                    uint32_t saved = *save;
                    uint32_t *retail_slot = wrapper
                        ? (uint32_t *)P4_G2H(retail_r + 0x5c) : retail_save + 1;
                    memcpy(before, input, sizeof before);
                    strcpy(delimiter, delimiters[changing ? step % 4 : 0]);
                    *retail_slot = saved;
                    *(uint32_t *)P4_G2H(0x710070) = retail_r;
                    R5900 c = setup(NULL);
                    c.r[4] = step ? 0 : 0x1c01000;
                    c.r[5] = 0x1c02000;
                    c.r[6] = 0x1c00004;
                    if (call(&c, wrapper ? 0x443f18 : 0x443f38)) return 1;
                    uint32_t expected_save = *retail_slot;
                    memcpy(expected, input, sizeof expected);
                    memcpy(input, before, sizeof before);
                    *(uint32_t *)P4_G2H(0x710070) = P4_H2G(native_r);
                    char *actual = wrapper ? func_00443f18(step ? NULL : input, delimiter)
                                           : func_00443f38(step ? NULL : input, delimiter, save);
                    ++cases;
                    if (P4_H2G(actual) != (uint32_t)c.r[2] || *save != expected_save ||
                        memcmp(input, expected, sizeof expected) ||
                        native_save[0] != 0x12345678 || native_save[2] != 0x87654321 ||
                        native_r[0x58 / 4] != before_left || native_r[0x60 / 4] != before_right) {
                        fprintf(stderr, "tokenizer entry=%08x input=%u changing=%u step=%u return=%08x/%08x save=%08x/%08x\n",
                                wrapper ? 0x443f18 : 0x443f38, n, changing, step,
                                P4_H2G(actual), (uint32_t)c.r[2], *save, expected_save);
                        return 1;
                    }
                    *(uint32_t *)P4_G2H(0x710070) = retail_r;
                }
            }
        }
    }
    return 0;
}

int main(int argc,char **argv)
{
#ifndef P4_GUEST64
    /* Native and interpreted heaps are disjoint; the native arena needs the
     * 64-MiB guest block supplied by the Linux/Windows host64 test twins. */
    return 77;
#endif
    const char *elf=argc>1?argv[1]:getenv("P4PC_ELF");if(!elf||!*elf)return 77;
    p4pc_load_image(elf);retail_r=*(uint32_t *)P4_G2H(0x710070);
    if(p4pc_heap_init(P4_G2H(0x1800000),4u<<20))return 2;
    uint32_t *r=P4_G2H(0x3400000);memset(r,0,0x100);
    trace=getenv("P4PC_DECIMAL_TRACE")!=NULL;
    static const char *edge[]={""," ","+","-",".","-.","0","-0","0000","1","-1","  +1.25tail","\t\r\n-1e+2!","1e","1e+","1e-","0x1p2","0x1.ffffp1023","inf","nan","1e309","-1e99999999999999999999","1e-9999999","1e-308","1e-307","1.2e-307","2.2250738585072014e-308","2.2250738585072011e-308","4.9406564584124654e-324","1.7976931348623157e308","1.7976931348623159e308","9007199254740993","1.00000000000000011102230246251565404236316680908203125","1.00000000000000011102230246251565404236316680908203126","0.4999999999999999999999999999999999999999999999"};
    static const char *scantext[]={""," ","+","-","x","0x","0xg","-0x1","+0x1","00x1","ffffffffffffffff","10000000000000000","-ffffffffffffffff","1 2 3","0xffffffffffffffff 0x100000000 7","  +a  -b  c!","1z2z3","1e+","1.2e-307"};
    static const char *formats[]={"%lx%n","%lx %lx %lx%n","%llx%n","%*lx %lx%n","%1lx%n","%2lx%n","%3lx%n","%i%n","%ld%n","%o%n","%u%n","%p%n","%c%n","%3c%n","%s%n","%[a-f]%n","%lf%n","%f%n"};
    for(int mode=0;mode<2;++mode){
        if(mode)p4pc_fpu_ee_mode();
        for(unsigned i=0;i<sizeof edge/sizeof edge[0];++i)if(decimal(r,edge[i]))return 1;
        for(unsigned i=0;i<sizeof scantext/sizeof scantext[0];++i)
            for(unsigned j=0;j<sizeof formats/sizeof formats[0];++j)if(scanner(r,scantext[i],formats[j]))return 1;
        char longscan[1024];
        memset(longscan,'a',900);longscan[900]=0;
        if(scanner(r,longscan,"%lx%n"))return 1;
        memset(longscan,'0',900);strcpy(longscan+900,"1.25e-3!");
        if(scanner(r,longscan,"%lf%n"))return 1;
        uint64_t rng=0xcafebabedeadbeefULL;
        for(unsigned i=0;i<2048;++i){
            char input[2048];rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;
            unsigned digits=1+(unsigned)(rng%900),p=0;
            if(rng&1)input[p++]='-';if(rng&2)input[p++]=' ';
            for(unsigned j=0;j<digits;++j){rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;input[p++]=(char)('0'+rng%10);if(j==digits/2)input[p++]='.';}
            snprintf(input+p,sizeof input-p,"e%d!",(int)(rng%1300)-650);
            if(decimal(r,input))return 1;
            snprintf(input,sizeof input,"%"PRIx64" %"PRIx64" %"PRIx64"!",rng,rng^0xabcdef,rng>>4);
            if(scanner(r,input,"%lx %lx %lx%n"))return 1;
        }
        if (tokenizer(r)) return 1;
    }
    printf("retail strtod/scanner/tokenizer differential: %u byte-identical cases, normal+EE mode\n",cases);return 0;
}
