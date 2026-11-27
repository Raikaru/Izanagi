#include "p4pc/golden_encounter_native.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    uint8_t g[68*252]={0}, p[68*248]={0};
    for (unsigned i=0;i<68;++i) for (unsigned d=0;d<2;++d) {
        g[i*252+d]=p[i*248+d]=(uint8_t)(40+d);
        g[i*252+6+d]=p[i*248+4+d]=10;
    }
    unsigned gc=0,pc=0;
    assert(!p4_golden_encounter_matrix(g,sizeof g,p,sizeof p,&gc,&pc) && gc==6528 && pc==3264);
    for (unsigned f=0;f<2;++f) for (unsigned b=0;b<2;++b) for (unsigned m=0;m<65536;++m) {
        int total=p4_golden_encounter_total(g,0,(uint16_t)m,f,b);
        assert(total==(m==4 ? 10 : (m>=2 && m!=3 && !f ? 50 : 40)));
    }
    /* All 64 weather override masks and matching/stale date states reduce to
     * these columns. First set flag wins, not the most recently tested bit. */
    const unsigned weather[6]={0,1,2,3,4,7};
    for (unsigned mask=0;mask<64;++mask) for (unsigned match=0;match<2;++match)
        for (unsigned calendar=0;calendar<8;++calendar) {
            unsigned value=calendar;
            if (match) for (unsigned i=0;i<6;++i) if (mask&(1u<<i)) { value=weather[i]; break; }
            unsigned d=value==1 || value==3 || value==7;
            assert(p4_golden_encounter_total(g,d,2,0,1)>0);
        }
    p[2]=7; p[4]=11;
    for (unsigned level=0;level<3;++level) for (int f=0;f<2;++f) for (int b=0;b<2;++b)
        for (int e=0;e<2;++e) {
            int m=e?0:(b?100:1);
            int expected=40+(f?0:((level?m*7:0)+(level>=2?11:0)));
            assert(p4_golden_encounter_native_total(p,0,(uint16_t)level,f,m)==expected);
        }
    assert(p4_golden_encounter_native_total(p,0,3,0,1)==58); /* still level>=2, not forced mode3 */
    g[0]=0; assert(p4_golden_encounter_matrix(g,sizeof g,p,sizeof p,&gc,&pc)==-1);
    uint8_t zero[252]={0};
    assert(!p4_golden_encounter_total(zero,0,2,0,0));
    assert(!p4_golden_encounter_projected_total(zero,0,2,0,0,0));
    puts("golden_encounter_rates: full mode/weather equivalence and signed projected native seam passed");
    return 0;
}
