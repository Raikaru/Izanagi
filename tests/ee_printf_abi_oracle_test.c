/* Actual compiled, AST-rewritten Sint32/Uint32 call expressions versus retail
 * sprintf instructions. This is not the pre-widened formatter-core corpus. */
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/hostmath.h"
#include "p4pc/tables.h"
#include "r5900.h"

const p4pc_patch_t p4pc_patch_table[] = {};
const size_t p4pc_patch_count = 0;
const p4pc_alias_t p4pc_alias_table[] = {};
const size_t p4pc_alias_count = 0;
const p4pc_native_t p4pc_native_table[] = {};
const size_t p4pc_native_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {};
const size_t p4pc_hostfn_count = 0;

extern int p4_printf_abi_words(char *, const char *, int32_t, uint32_t);
extern int p4_printf_abi_mixed(char *, const char *, int32_t, uint32_t,
                               char *, uint64_t, double, int32_t, int32_t, int32_t);
extern int p4_printf_abi_alias(char *, const char *, int32_t, uint32_t, char *, double);

enum { OUT=0x01c00000u, FMT=0x01c01000u, TEXT=0x01c02000u,
       NATIVE_REENT=0x017f0000u, STACK=0x01f00000u, CAP=1024 };
static unsigned comparisons;
static uint32_t retail_reent;

static int compare(unsigned kind, int32_t s, uint32_t u, uint64_t soft,
                     uint64_t real_bits, int width, int precision, int character)
{
    static const char *formats[]={
        "%ld|%lu|%lld|%llu|%d|%u|%hd|%hu",
        "[%*.*ld]|%s|%s|%.9g|%.9g|%p|%p|%c|%d",
        "%ld|%lu|%s|%.17g"
    };
    const char *fmt=P4_G2H(FMT);
    strcpy(P4_G2H(FMT),formats[kind]);
    uint64_t a[11]={0};
    unsigned count;
    if(kind==0) {
        uint64_t args[]={ (uint64_t)(int64_t)s,u,(uint64_t)(int64_t)s,u,
                          (uint64_t)(int64_t)s,u,
                          (uint64_t)(int64_t)(short)s,(unsigned short)u };
        memcpy(a,args,sizeof args);count=8;
    } else if(kind==1) {
        uint64_t args[]={ (uint64_t)(int64_t)width,(uint64_t)(int64_t)precision,
                          (uint64_t)(int64_t)s,TEXT,TEXT,soft,real_bits,
                          TEXT,TEXT,(uint64_t)(int64_t)character,(uint64_t)(int64_t)s };
        memcpy(a,args,sizeof args);count=11;
    } else {
        a[0]=(uint64_t)(int64_t)s;a[1]=u;a[2]=TEXT;a[3]=real_bits;count=4;
    }
    memset(P4_G2H(OUT),0xa5,CAP);
    R5900 c={0};
    R5900KernelEnv kernel={0x00010001u,0x01e00000u};
    *(uint32_t *)P4_G2H(0x00710070u)=retail_reent;
    c.lo_pc=0x00100000u;c.hi_pc=0x005e4000u;c.max_steps=10000000;c.kernel=&kernel;
    c.r[4]=OUT;c.r[5]=FMT;
    for(unsigned i=0;i<count && i<6;++i)c.r[6+i]=a[i];
    if(count>6)memcpy(P4_G2H(STACK),a+6,(count-6)*sizeof(uint64_t));
    c.r[28]=0x007690f0u;c.r[29]=STACK;
    int rc=r5900_call(&c,0x00442088u);
    if(rc!=R5900_OK) {
        fprintf(stderr,"rewritten printf oracle rc=%d pc=%08x ins=%08x address=%08x\n",
                rc,c.bad_pc,c.bad_word,c.fault_addr);return 1;
    }
    char native[CAP];
    memset(native,0xa5,sizeof native);
    double real;
    memcpy(&real,&real_bits,sizeof real);
    *(uint32_t *)P4_G2H(0x00710070u)=NATIVE_REENT;
    int length;
    if(kind==0)length=p4_printf_abi_words(native,fmt,s,u);
    else if(kind==1)length=p4_printf_abi_mixed(native,fmt,s,TEXT,P4_G2H(TEXT),
                                              soft,real,width,precision,character);
    else length=p4_printf_abi_alias(native,fmt,s,u,P4_G2H(TEXT),real);
    *(uint32_t *)P4_G2H(0x00710070u)=retail_reent;
    ++comparisons;
    if(length!=(int32_t)c.r[2] || memcmp(native,P4_G2H(OUT),CAP)) {
        fprintf(stderr,"rewritten printf kind=%u s=%" PRId32 " u=%" PRIu32
                " soft=%016" PRIx64 " real=%016" PRIx64
                " native(%d)=[%s] retail(%d)=[%s]\n",kind,s,u,soft,real_bits,
                length,native,(int32_t)c.r[2],(char *)P4_G2H(OUT));return 1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *elf=argc>1?argv[1]:getenv("P4PC_ELF");
    if(!elf || !*elf)return 77;
    p4pc_load_image(elf);
    /* Identical production heap environment to ee_printf_oracle_test. Keep
     * both reent records and all witness scratch inside mapped EE RAM on i386. */
    if(p4pc_heap_init(P4_G2H(0x01800000u),4u<<20))return 2;
    retail_reent=*(const uint32_t *)P4_G2H(0x00710070u);
    memset(P4_G2H(NATIVE_REENT),0,0x100);
    strcpy(P4_G2H(TEXT),"native-and-guest-\200-text");
    static const int32_t signed_values[]={INT32_MIN,INT32_MIN+1,-7,-1,0,1,0x40000000,INT32_MAX};
    static const uint32_t unsigned_values[]={0,1,7,32768,0x7fffffff,0x80000000u,0xfffffffeu,UINT32_MAX};
    static const uint64_t floats[]={
        0,UINT64_C(0x8000000000000000),1,UINT64_C(0xfffffffffffff),
        UINT64_C(0x10000000000000),UINT64_C(0x10000000000001),
        UINT64_C(0x3fdfffffffffffff),UINT64_C(0x3fe0000000000000),UINT64_C(0x3fe0000000000001),
        UINT64_C(0x3ff0000000000000),UINT64_C(0xbff0000000000000),UINT64_C(0x3ff6000000000000),
        UINT64_C(0x4004000000000000),UINT64_C(0x400c000000000000),
        UINT64_C(0x405edd2f20000000),UINT64_C(0xc05edd2f20000000),UINT64_C(0x40c3881000000000),
        UINT64_C(0x36a0000000000000),UINT64_C(0x380fffffc0000000),
        UINT64_C(0x3810000000000000),UINT64_C(0x47efffffe0000000),
        UINT64_C(0x4340000000000000),UINT64_C(0x4480f0cf064dd592),
        UINT64_C(0x7fefffffffffffff),UINT64_C(0xffefffffffffffff),
        UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),
        UINT64_C(0x7ff0000000000001),UINT64_C(0x7ff8000000000000),UINT64_C(0xfff8000000000042)
    };
    static const int widths[]={-17,0,1,20},precisions[]={-1,0,1,6};
    for(unsigned fpu=0;fpu<2;++fpu) {
        if(fpu)p4pc_fpu_ee_mode();
        for(unsigned i=0;i<8;++i)for(unsigned j=0;j<8;++j) {
            if(compare(0,signed_values[i],unsigned_values[j],0,0,0,0,0))return 1;
            for(unsigned f=0;f<sizeof floats/sizeof floats[0];++f)
                if(compare(2,signed_values[i],unsigned_values[j],0,floats[f],0,0,0))return 1;
        }
        for(unsigned i=0;i<8;++i)for(unsigned f=0;f<sizeof floats/sizeof floats[0];++f)
            for(unsigned w=0;w<4;++w)
                if(compare(1,signed_values[i],0,floats[f],floats[(f+3)%(sizeof floats/sizeof floats[0])],
                           widths[w],precisions[w],(int32_t)i-3))return 1;
    }
    printf("retail rewritten printf ABI differential: %u byte-identical cases, %u-bit native pointers, normal and EE FPU modes\n",
           comparisons,(unsigned)(8*sizeof(void *)));
    return 0;
}
