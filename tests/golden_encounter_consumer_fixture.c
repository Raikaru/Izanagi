/* Actual spliced Encount_PickEncounter; all world/flag/RNG inputs synthetic. */
#include <stdint.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "p4pc/golden_encounter_native.h"
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32; typedef int64_t s64; typedef float f32;
typedef struct EncEntry { u16 id; u8 rest[8]; } EncEntry;
typedef struct EncWeightedEntry { u16 id,weight; } EncWeightedEntry;
typedef struct EncWeightedTable { u8 normalRates[2],specialRates[2],rareRates[2],pad06[2]; EncWeightedEntry entries[2][30]; } EncWeightedTable;
static P4GameMode mode;
P4GameMode p4_game_mode(void) { return mode; }
static EncEntry selectors[68]; static EncWeightedTable rows[68];
u8 *iGpffffb41c=(u8 *)selectors,*iGpffffb418=(u8 *)rows;
u8 D_005F12C8[4];
static unsigned f,b,allow,day,draws,assertions;
static jmp_buf trap;
static s32 FldMap_IsInBandA(s32 field,s32 room) { (void)field;(void)room; return 1; }
static s32 FldMap_IsInBandB(s32 field,s32 room) { (void)field;(void)room; return 0; }
static s16 datGetDateIndex(void) { return 1; }
static u8 datGetTimeSlot(void) { return 0; }
static s64 calGetDayEntryByPhase(s32 date,u32 phase) { (void)date;(void)phase; return day; }
static u32 datGetFlag(s32 id) { if(id==0x1410)return f; if(id==0x1411)return b; assert(id==0xc0f); return allow; }
static u32 RpRandom(void) { ++draws; return 0; }
static void kAssertFail(void *file,s32 line) { (void)file; assert(line==0x69); ++assertions; longjmp(trap,1); }
static u8 *byteAddressAdd(u32 offset,u8 *base) { return base+offset; }
/*@@CONSUMER@@*/
int main(void)
{
    unsigned cells=0;
    for (unsigned i=0;i<68;++i) { selectors[i].id=(u16)i; for(unsigned d=0;d<2;++d) {
        rows[i].normalRates[d]=(u8)(55+i%56); rows[i].specialRates[d]=7; rows[i].rareRates[d]=3;
    }}
    for (unsigned gm=0;gm<2;++gm) { mode=gm?P4_GAME_MODE_GOLDEN:P4_GAME_MODE_PS2;
        for (unsigned i=0;i<68;++i) for(day=0;day<2;++day) for(f=0;f<2;++f) for(b=0;b<2;++b)
            for(allow=0;allow<2;++allow) for(unsigned level=0;level<3;++level) {
                draws=0;
                assert(Encount_PickEncounter(0x28,0,i,level)==-1 && draws==1);
                ++cells;
            }
    }
    assert(cells==6528 && !assertions);
    /* Eligible-weight zero is safe for each selected slice and column. */
    mode=P4_GAME_MODE_GOLDEN; f=0;b=0;allow=1;
    for (day=0;day<2;++day) for (unsigned c=0;c<3;++c) {
        memset(&rows[0],0,sizeof rows[0]);
        if(c==0) rows[0].normalRates[day]=40;
        if(c==1) rows[0].specialRates[day]=40;
        if(c==2) rows[0].rareRates[day]=40;
        draws=0; assert(Encount_PickEncounter(0,0,0,2)==-1 && draws==1);
        const unsigned first[3]={0,20,25};
        rows[0].entries[day][first[c]].weight=9; /* ID0 is not an eligible formation */
        draws=0; assert(Encount_PickEncounter(0,0,0,2)==-1 && draws==1);
        rows[0].entries[day][first[c]].id=3;
        draws=0; assert(Encount_PickEncounter(0,0,0,2)==((1<<c)<<16 | 3) && draws==2);
    }
    memset(&rows[0],0,sizeof rows[0]); draws=0;
    if (!setjmp(trap)) { Encount_PickEncounter(0,0,0,2); assert(0); }
    assert(assertions==1 && draws==0);
    puts("golden_encounter_consumer: 3264 PS2 + 3264 Golden native cells; empty slices and zero-total assertion passed");
    return 0;
}
