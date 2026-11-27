#include "../src/platform/golden/golden_ai_rng.h"
#include <fenv.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while (0)
int main(void)
{
    static const uint32_t seeds[] = {0,1,0x9a319039};
    static const uint32_t additive[3][4] = {
        {1990395451,1950343301,404384196,1972190027},
        {2078917053,143302914,1027100827,1953210302},
        {1373023817,253234266,2145093567,898975974}};
    static const uint32_t expanded[3][4] = {
        {0,1,0x6c078967,0x714acb41}, {1,0x6c078966,0xdd5254a5,0xb9523b81},
        {0x9a319039,0x315a7a48,0x9c1ac66a,0x3795ef0b}};
    P4GoldenAiRng ai;
    P4GoldenSfmt sfmt;
    for (unsigned s = 0; s < 3; ++s) {
        p4_golden_ai_rng_seed(&ai,seeds[s]);
        for (unsigned i = 0; i < 4; ++i) CHECK(p4_golden_ai_rng_next(&ai) == additive[s][i]);
        p4_golden_sfmt_seed(&sfmt,seeds[s]);
        CHECK(sfmt.index == 624 && !memcmp(sfmt.words,expanded[s],sizeof expanded[s]));
    }
    memset(&ai,0,sizeof ai); ai.front=30; ai.rear=27;
    ai.words[30]=0xffffffff; ai.words[27]=3;
    P4GoldenAiRng before=ai;
    CHECK(p4_golden_ai_rng_next(&ai)==1 && ai.front==0 && ai.rear==28 && ai.words[30]==2);
    for (unsigned i=0;i<30;++i) CHECK(ai.words[i]==before.words[i]);
    for (unsigned a=0;a<2;++a) for (unsigned b=0;b<2;++b) {
        for (unsigned i=0;i<624;++i) sfmt.words[i]=i*12345;
        sfmt.words[0]=a; sfmt.words[3]=b*4; sfmt.index=123;
        P4GoldenSfmt old=sfmt;
        p4_golden_sfmt_certify(&sfmt);
        CHECK(sfmt.index==123 && sfmt.words[0]==(a ^ (1u ^ a ^ b)));
        CHECK(!memcmp(sfmt.words+1,old.words+1,623*sizeof(uint32_t)));
    }
    uint32_t a[4]={1,0,0,0}, zero[4]={0}, out[4];
    p4_golden_sfmt_recur(out,a,zero,zero,zero);
    CHECK(out[0]==0x101 && !out[1] && !out[2] && !out[3]);
    memset(&sfmt,0,sizeof sfmt); sfmt.index=624;
    CHECK(p4_golden_sfmt_next(&sfmt)==0 && sfmt.index==1);
    for (unsigned i=0;i<624;++i) CHECK(sfmt.words[i]==0);
    /* Published SFMT.19937.out.txt, init_gen_rand(1234), first 20 words. */
    static const uint32_t reference[] = {3440181298u,1564997079,1510669302,2930277156u,1452439940,
        3796268453u,423124208,2143818589,3827219408u,2987036003u,
        2674978610u,1536842514,2027035537,2534897563u,1686527725,
        545368292,1489013321,1370534252,4231012796u,3994803019u};
    p4_golden_sfmt_seed(&sfmt,1234);
    for (unsigned i=0;i<sizeof reference/sizeof reference[0];++i) CHECK(p4_golden_sfmt_next(&sfmt)==reference[i]);
    /* Continue across the 624-word refill boundary and into the next block. */
    static const unsigned positions[] = {623,624,625,626,627,998,999};
    static const uint32_t continuation[] = {2570786021u,3899704621u,1633861986,1636979509,
        1438500431,461224431,1168395933};
    unsigned next = 0;
    for (unsigned i = 20; i < 1000; ++i) {
        uint32_t value = p4_golden_sfmt_next(&sfmt);
        if (next < sizeof positions / sizeof positions[0] && i == positions[next]) {
            CHECK(value == continuation[next]);
            ++next;
        }
    }
    CHECK(next == sizeof positions / sizeof positions[0]);
    CHECK(p4_golden_rng2_word(0,5)==0);
    CHECK(p4_golden_rng2_word(UINT32_MAX,5)==6);
    CHECK(p4_golden_rng2_word(UINT32_MAX,-1)==0);
    CHECK(p4_golden_rng2_word(UINT32_MAX,-2)==-1);
    CHECK(p4_golden_rng2_word(UINT32_MAX,INT64_MAX)==INT64_MIN);
    CHECK(p4_golden_rng2_word(UINT32_MAX,INT64_MAX-1)==INT64_MAX);
    sfmt.index=0; sfmt.words[0]=UINT32_MAX; sfmt.anomalies=0;
    CHECK(p4_golden_rng2(&sfmt,INT64_MAX-1)==INT64_MAX && sfmt.anomalies==1);
    sfmt.index=0; sfmt.words[0]=123; sfmt.words[1]=UINT32_MAX;
    CHECK(p4_golden_assist_argument(&sfmt)==3 && sfmt.index==2);
    sfmt.index=0; sfmt.words[1]=0x80000000u;
    CHECK(p4_golden_assist_argument(&sfmt)==1 && sfmt.index==2);
    CHECK(fesetround(FE_DOWNWARD)==0);
    CHECK(p4_golden_rng2_word(UINT32_MAX,5)==5);
    CHECK(fesetround(FE_UPWARD)==0);
    CHECK(p4_golden_rng2_word(UINT32_MAX,5)==6);
    CHECK(fesetround(FE_TOWARDZERO)==0);
    CHECK(p4_golden_rng2_word(UINT32_MAX,5)==5);
    CHECK(fesetround(FE_TONEAREST)==0);
    puts("golden_ai_rng: spec vectors and published SFMT vectors pass");
    return 0;
}
