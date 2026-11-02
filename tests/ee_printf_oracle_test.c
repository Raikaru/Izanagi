/* Byte-for-byte printf-family differential against unchanged retail sprintf.
 * All source-inventoried game formats plus synthetic conversion/flag cases.
 * Values are marshalled through the existing p4pf integer-class mask, never
 * through a host formatter. The retail side has no native call hook. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/hostmath.h"
#include "p4pc/tables.h"
#include "ee_stdio.h"
#include "r5900.h"

const p4pc_patch_t p4pc_patch_table[] = {};
const size_t p4pc_patch_count = 0;
const p4pc_alias_t p4pc_alias_table[] = {};
const size_t p4pc_alias_count = 0;
const p4pc_native_t p4pc_native_table[] = {};
const size_t p4pc_native_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {};
const size_t p4pc_hostfn_count = 0;

#include "data/ee_printf_formats.inc"

#ifdef P4_GUEST64
extern int p4pf_sprintf(char *, const char *, ...);
static unsigned comparisons;

static int arguments(const char *format, unsigned sample, uint64_t floating, uint64_t args[16])
{
    static const uint64_t integer[]={0,1,UINT64_MAX,UINT64_C(0x80000000),
        UINT64_C(0x7fffffff),UINT64_C(0xffffffff),UINT64_C(0x8000000000000000),
        UINT64_C(0x7fffffffffffffff),UINT64_C(0xfedcba9876543210)};
    unsigned count=0;
    while(*format) {
        if(*format++!='%')continue;
        while(*format && strchr("-+ #0",*format))++format;
        if(*format=='*') { if(count==16)return -1;args[count++]=(uint64_t)(int64_t)(sample&1?-17:20);++format; }
        else while(*format>='0' && *format<='9')++format;
        if(*format=='.') {
            ++format;
            if(*format=='*') { if(count==16)return -1;args[count++]=sample%18;++format; }
            else while(*format>='0' && *format<='9')++format;
        }
        while(*format && strchr("hlLq",*format))++format;
        char conversion=*format;
        if(!conversion)break;
        ++format;
        if(!strchr("diDOouxXUcspeEfgGn",conversion))continue;
        if(count==16)return -1;
        uint64_t value=integer[(sample+count)%(sizeof integer/sizeof integer[0])];
        switch(conversion) {
        case 's':value=sample?0x01c04000u:0;break;
        case 'p':value=sample?0x01c04000u:0;break;
        case 'c':value=sample?'A'+sample%26:0;break;
        case 'n':value=0x01c06000u+8*count;break;
        case 'e':case 'E':case 'f':case 'g':case 'G':value=floating;break;
        }
        args[count++]=value;
    }
    while(count<16)args[count++]=0;
    return 0;
}

static int native_format(char *out, const char *fmt, const uint64_t a[16])
{
    return p4pf_sprintf(out,fmt,UINT32_MAX,
        a[0],a[1],a[2],a[3],a[4],a[5],a[6],a[7],
        a[8],a[9],a[10],a[11],a[12],a[13],a[14],a[15]);
}

static int compare_format(uint32_t address, unsigned sample, uint64_t floating,
                           uint32_t retail_r, uint32_t native_r)
{
    const char *fmt=P4_G2H(address);
    uint64_t a[16];
    if(arguments(fmt,sample,floating,a)) {
        fprintf(stderr,"format exceeds fixture argument capacity: %s\n",fmt);return 1;
    }
    char native[8192];
    memset(native,0xa5,sizeof native);
    memset(P4_G2H(0x01c00000u),0xa5,sizeof native);
    unsigned char after[128];
    unsigned char *destinations=P4_G2H(0x01c06000u);
    memset(destinations,0xa5,sizeof after);
    R5900 c={0};
    R5900KernelEnv kernel={0x00010001u,0x01c00000u};
    *(uint32_t *)P4_G2H(0x00710070u)=retail_r;
    c.lo_pc=0x00100000u;c.hi_pc=0x005e4000u;c.max_steps=10000000;c.kernel=&kernel;
    c.r[4]=0x01c00000u;c.r[5]=address;
    for(unsigned i=0;i<6;++i)c.r[6+i]=a[i];
    memcpy(P4_G2H(0x01f00000u),a+6,10*sizeof(uint64_t));
    c.r[28]=0x007690f0u;c.r[29]=0x01f00000u;
    int rc=r5900_call(&c,0x00442088u);
    if(rc!=R5900_OK) {
        fprintf(stderr,"sprintf oracle fmt=%08x sample=%u float=%016" PRIx64 " rc=%d pc=%08x ins=%08x address=%08x steps=%lu\n",
                address,sample,floating,rc,c.bad_pc,c.bad_word,c.fault_addr,c.steps);return 1;
    }
    int retail_length=(int32_t)c.r[2];
    if(retail_length<0 || retail_length>=(int)sizeof native) {
        fprintf(stderr,"retail format length outside fixture: %d\n",retail_length);return 1;
    }
    memcpy(after,destinations,sizeof after);
    memset(destinations,0xa5,sizeof after);
    *(uint32_t *)P4_G2H(0x00710070u)=native_r;
    int length=native_format(native,fmt,a);
    *(uint32_t *)P4_G2H(0x00710070u)=retail_r;
    const char *retail=P4_G2H(0x01c00000u);
    ++comparisons;
    if(length!=retail_length || memcmp(native,retail,sizeof native) ||
       memcmp(after,destinations,sizeof after)) {
        fprintf(stderr,"sprintf fmt=%08x [%s] sample=%u float=%016" PRIx64 " native(%d)=[%s] retail(%d)=[%s]\n",
                address,fmt,sample,floating,length,native,retail_length,retail);
        return 1;
    }
    return 0;
}
#endif

int main(int argc, char **argv)
{
#ifndef P4_GUEST64
    (void)argc;(void)argv;
    return 77; /* Generated mixed arguments use the shipping 64-bit mask ABI. */
#else
    const char *elf=argc>1?argv[1]:getenv("P4PC_ELF");
    if(!elf || !*elf)return 77;
    p4pc_load_image(elf);
    if(p4pc_heap_init(P4_G2H(0x01800000u),4u<<20))return 2;
    uint32_t retail_r=*(const uint32_t *)P4_G2H(0x00710070u);
    uint32_t native_r=0x03400000u;
    memset(P4_G2H(native_r),0,0x100);
    strcpy(P4_G2H(0x01c04000u),"guest-\200-text");
    static const uint64_t values[]={
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
    static const char *synthetic[]={
        "%f","%4.2f","_%4.2f","%g","%.0g","%.17g","%e","%.0e","%.17E",
        "%#g","%+020.8f","% -20.6e","%#020.0f","%.*g","%*.*f",
        "%s %4.2f %d %4.2f","%d %i %u %o %x %X",
        "%+d|% d|%05d|%-6.4d|%#08x","%hhd %hd %ld %lld %qd %D",
        "%p|%.0p|%#o|%.0d|%c|%.3s%n","%hn %ln %lln","%#X|%#.0o|%#08x|%%"
    };
    for(int fpu=0;fpu<2;++fpu) {
        if(fpu)p4pc_fpu_ee_mode();
        for(unsigned f=0;f<sizeof game_formats/sizeof game_formats[0];++f) {
            uint32_t address=game_formats[f].address;
            if(!address) { address=0x01c08000u;strcpy(P4_G2H(address),game_formats[f].source_literal); }
            for(unsigned i=0;i<sizeof values/sizeof values[0];++i)
                if(compare_format(address,i,values[i],retail_r,native_r))return 1;
        }
        for(unsigned f=0;f<sizeof synthetic/sizeof synthetic[0];++f) {
            strcpy(P4_G2H(0x01c08000u),synthetic[f]);
            for(unsigned i=0;i<sizeof values/sizeof values[0];++i)
                if(compare_format(0x01c08000u,i,values[i],retail_r,native_r))return 1;
            uint64_t rng=UINT64_C(0xbadc0ffee1234567);
            for(unsigned i=0;i<256;++i) {
                rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;
                if(compare_format(0x01c08000u,i,rng,retail_r,native_r))return 1;
            }
        }
    }
    printf("retail formatter differential: %u byte-identical cases, %u inventoried game formats, normal and EE FPU modes\n",
            comparisons,(unsigned)(sizeof game_formats/sizeof game_formats[0]));
    return 0;
#endif
}
