/* Executes the unchanged retail dtoa/allocator instructions, never a native
 * call hook, against generated binary64 payloads. No retail bytes are stored
 * in this fixture. P4PC_ELF supplies the user's original image at runtime. */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/hostmath.h"
#include "ee_decimal.h"
#include "ps2_fpbit.h"
#include "r5900.h"
#include "p4pc/tables.h"

/* Deliberately empty registrations: load the original image without any
 * native patch target or callback replacing an interpreted library word. */
const p4pc_patch_t p4pc_patch_table[] = {};
const size_t p4pc_patch_count = 0;
const p4pc_alias_t p4pc_alias_table[] = {};
const size_t p4pc_alias_count = 0;
const p4pc_native_t p4pc_native_table[] = {};
const size_t p4pc_native_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {};
const size_t p4pc_hostfn_count = 0;

static unsigned cases;
static int trace_cases;
static unsigned conversion_cases;

static int compare_conversions(uint64_t bits)
{
    static const uint32_t entries[]={0x0044ba00u,0x0044bb60u,0x0044bc68u,0x0044e830u};
    for(unsigned i=0;i<sizeof entries/sizeof entries[0];++i) {
        uint64_t native;
        if(i==0) {
            int64_t input;memcpy(&input,&bits,sizeof input);
            native=func_0044ba00(input);
        } else if(i==1)native=func_0044bb60(bits);
        else if(i==2)native=(uint64_t)func_0044bc68(bits);
        else {
            uint32_t word=func_0044e830(bits);
            int32_t signed_word;memcpy(&signed_word,&word,sizeof word);
            native=(uint64_t)(int64_t)signed_word;
        }
        R5900 c={0};
        c.lo_pc=0x00100000u;c.hi_pc=0x005e4000u;c.max_steps=1000000;
        c.r[4]=bits;c.r[28]=0x007690f0u;c.r[29]=0x01f00000u;
        int rc=r5900_call(&c,entries[i]);
        ++conversion_cases;
        if(rc!=R5900_OK || native!=c.r[2]) {
            fprintf(stderr,"conversion %08x input=%016" PRIx64 " native=%016" PRIx64 " retail=%016" PRIx64 " rc=%d pc=%08x word=%08x\n",
                    entries[i],bits,native,c.r[2],rc,c.bad_pc,c.bad_word);
            return 1;
        }
    }
    return 0;
}

static int compare_dtoa(uint32_t *native_r, uint64_t bits, int mode, int digits)
{
    R5900 c = {0};
    R5900KernelEnv kernel = {0x00010001u,0x01c00000u};
    uint32_t *slots=P4_G2H(0x01c00000u);
    int point,sign;
    char *end;
    if(trace_cases)
        fprintf(stderr,"dtoa case %u bits=%016" PRIx64 " mode=%d digits=%d\n",
                cases,bits,mode,digits);
    slots[0]=slots[1]=slots[2]=0xdeadbeefu;
    c.lo_pc=0x00100000u;c.hi_pc=0x005e4000u;c.max_steps=10000000;
    c.kernel=&kernel;
    c.r[4]=*(const uint32_t *)P4_G2H(0x00710070u);
    c.r[5]=bits;c.r[6]=(uint64_t)(int64_t)mode;c.r[7]=(uint64_t)(int64_t)digits;
    c.r[8]=0x01c00000u;c.r[9]=0x01c00004u;c.r[10]=0x01c00008u;
    c.r[28]=0x007690f0u;c.r[29]=0x01f00000u;
    int rc=r5900_call(&c,0x0043ca70u);
    if(rc!=R5900_OK) {
        fprintf(stderr,"dtoa oracle bits=%016" PRIx64 " mode=%d digits=%d rc=%d pc=%08x ins=%08x address=%08x steps=%lu reason=%s\n",
                bits,mode,digits,rc,c.bad_pc,c.bad_word,c.fault_addr,c.steps,c.fault_why?c.fault_why:"none");
        return 1;
    }
    char *native=p4_decimal_dtoa(native_r,bits,mode,digits,&point,&sign,&end);
    char saved[2048];
    size_t length=(size_t)(end-native);
    if(length>=sizeof saved) { fputs("dtoa output exceeds fixture capacity\n",stderr);return 1; }
    memcpy(saved,native,length+1);
    const char *retail=P4_G2H((uint32_t)c.r[2]);
    ++cases;
    if(strcmp(saved,retail) || point!=(int32_t)slots[0] || sign!=(int32_t)slots[1] ||
       length!=(size_t)(slots[2]-(uint32_t)c.r[2])) {
        fprintf(stderr,"dtoa bits=%016" PRIx64 " mode=%d digits=%d native=[%s] point=%d sign=%d retail=[%s] point=%d sign=%d\n",
                bits,mode,digits,saved,point,sign,retail,(int32_t)slots[0],(int32_t)slots[1]);
        return 1;
    }
    return 0;
}

int main(int argc,char **argv)
{
    const char *elf=argc>1?argv[1]:getenv("P4PC_ELF");
    if(!elf || !*elf) return 77;
    p4pc_load_image(elf);
    if(p4pc_heap_init(P4_G2H(0x01800000u),4u<<20))return 2;
    uint32_t *native_r=P4_G2H(0x017f0000u);
    memset(native_r,0,0x100);
    trace_cases=getenv("P4PC_DECIMAL_TRACE")!=NULL;
    static const uint64_t edge[]={
        0,UINT64_C(0x8000000000000000),1,2,UINT64_C(0xfffffffffffff),
        UINT64_C(0x10000000000000),UINT64_C(0x10000000000001),
        UINT64_C(0x3fdfffffffffffff),UINT64_C(0x3fe0000000000000),UINT64_C(0x3fe0000000000001),
        UINT64_C(0x3ff0000000000000),UINT64_C(0x3ff6000000000000),
        UINT64_C(0x4004000000000000),UINT64_C(0x400c000000000000),
        UINT64_C(0x405edd2f20000000),UINT64_C(0xc05edd2f20000000), /* +/-123.456f */
        UINT64_C(0x40c3881000000000), /* 10000.125f */
        UINT64_C(0x36a0000000000000),UINT64_C(0x380fffffc0000000), /* promoted float subnormals */
        UINT64_C(0x3810000000000000),UINT64_C(0x47efffffe0000000), /* float normal extrema */
        UINT64_C(0x3fdfffffe0000000),UINT64_C(0x3fe0000020000000), /* neighbors of float 0.5 */
        UINT64_C(0x4340000000000000),UINT64_C(0x4480f0cf064dd592),
        UINT64_C(0x7fefffffffffffff),UINT64_C(0x7ff0000000000000),
        UINT64_C(0xfff0000000000000),UINT64_C(0x7ff0000000000001),
        UINT64_C(0x7ff8000000000000),UINT64_C(0xfff8000000000042)
    };
    static const int precision[]={-8,-1,0,1,2,6,14,17,50,340};
    static const uint64_t conversion_edges[]={
        0,1,UINT64_MAX,UINT64_C(0x7fffffff),UINT64_C(0x80000000),UINT64_C(0xffffffff),
        UINT64_C(0x100000000),UINT64_C(0x100000001),UINT64_C(0x1fffffffffffff),
        UINT64_C(0x20000000000000),UINT64_C(0x20000000000001),UINT64_C(0x20000000000003),
        UINT64_C(0xffe0000000000001),UINT64_C(0xffe0000000000000),
        UINT64_C(0xffdfffffffffffff),UINT64_C(0xffdffffffffffffd),
        UINT64_C(0x43dfffffffffffff),UINT64_C(0x43e0000000000000),UINT64_C(0x43e0000000000001),
        UINT64_C(0x43efffffffffffff),UINT64_C(0x43f0000000000000),UINT64_C(0x43f0000000000001),
        UINT64_C(0xc3dfffffffffffff),UINT64_C(0xc3e0000000000000),UINT64_C(0xc3e0000000000001),
        UINT64_C(0xc3efffffffffffff),UINT64_C(0xc3f0000000000000),UINT64_C(0xc3f0000000000001),
        UINT64_C(0x7ffffffffffffffe),UINT64_C(0x7fffffffffffffff),
        UINT64_C(0x8000000000000000),UINT64_C(0x8000000000000001)
    };
    /* printf uses modes 2/3; additionally exercise shortest modes 0/1.
     * Gay's optional debug modes 4..9 can loop forever in the original EE
     * code (DBL_MIN, mode 5, precision -8 reaches the retail step limit). */
    for(int fpu=0;fpu<2;++fpu) {
        if(fpu)p4pc_fpu_ee_mode();
        for(unsigned i=0;i<sizeof conversion_edges/sizeof conversion_edges[0];++i)
            if(compare_conversions(conversion_edges[i]))return 1;
        for(unsigned i=0;i<sizeof edge/sizeof edge[0];++i) {
            if(compare_conversions(edge[i]))return 1;
            for(int mode=0;mode<4;++mode)
                for(unsigned j=0;j<sizeof precision/sizeof precision[0];++j)
                    if(compare_dtoa(native_r,edge[i],mode,precision[j]))return 1;
        }
        uint64_t rng=UINT64_C(0xbadc0ffee1234567);
        for(unsigned i=0;i<256;++i) {
            rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;
            if(compare_conversions(rng))return 1;
            for(int mode=0;mode<4;++mode)
                for(unsigned j=0;j<sizeof precision/sizeof precision[0];++j)
                    if(compare_dtoa(native_r,rng,mode,precision[j]))return 1;
        }
    }
    printf("retail dtoa differential: %u byte-identical cases (normal and EE FPU mode)\n",cases);
    printf("retail 64-bit conversion differential: %u byte-identical cases\n",conversion_cases);
    return 0;
}
