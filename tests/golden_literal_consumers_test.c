/* Actual extracted read sites, isolated service framing, synthetic PS2 image.
 * Steam mode compares Golden results against freshly pinned reader artifacts. */
#include "golden_literal_consumers.h"
#include "../src/platform/golden/golden_mode_image.h"
#include "p4pc/golden_literals.h"
#include "p4pc/game_mode.h"
#include "p4pc/guest.h"
#include "p4pc/gmem.h"
#include "p4pc/package_boot.h"
#include <stddef.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32; typedef int64_t s64; typedef float f32;
typedef struct { f32 x,y,z; } RwV3d;
typedef struct { f32 u,v; } RwTexCoords;
typedef struct { f32 x,y; s32 start,end,sprite; } BtlShuffleSpriteCue;
typedef struct DatPersonaRecord { u8 pad[4]; u8 level; } DatPersonaRecord;
typedef struct {
    u8 beforeD[13]; s8 fD; u8 beforeOutcome[14];
    union { s16 signedValue; u16 value; } outcome;
    s8 outcomeCount, stageKind;
    u8 beforeFlags[0x2c4-0x20]; s8 detailFlags[2][5]; u8 pad2ce[4];
    s8 calendarMonth, calendarDay, ruleIndices[2], detailSide, pad2d7;
    s16 f2D8; s8 outcomeIds[5], outcomeIdCount;
} FclCombineWork;
typedef struct KwlnTask { void *work; } KwlnTask;
#define MEMBER(type, object, structure, field) (*(type *)((u8 *)(object) + offsetof(structure, field)))
static unsigned weather_work_frees;
static void weather_work_free_marker(u8 *work)
{
    assert(p4_golden_literal_weather_work_release(P4_H2G(work)) == -1);
    ++weather_work_frees;
}
static void (*jtbl_008873EC[1])(u8 *) = {weather_work_free_marker};
typedef struct { u32 contentType, elementCount, offset, elementSize; } ScrContentEntry;
typedef struct { char magic[4]; s32 totalEntries; ScrContentEntry *entries; } ScrHeader;
typedef struct { s32 unused; } ScrLblPrcd;
typedef struct { s32 unused; } ScrScriptWork;
typedef struct { u32 indexed[7], offset1C[7], offset38[7]; } TnTable;
_Static_assert(sizeof(TnTable) == 84, "three existing 32-bit queue banks");
static void *tnTableBase;
s32 TnGetOffset1C(u16 slot);
s32 TnGetOffset38(u16 slot);
s32 TnGetIndexedWord(u16 slot);
static void queue_context_reference(const void *table);
typedef RwV3d SceneLightAxis;
static unsigned axis_context_count;
static void axis_context_reference(const void *axis);
static unsigned queue_context_count, queue_flushes, queue_advances;
static void check_queue_getters(void)
{
    static const u8 remap[7] = {0, 1, 4, 6, 2, 3, 5};
    for (unsigned j = 0; j < 7; ++j) {
        u32 handle = 0x00796520u + remap[j] * 48u;
        assert((u32)TnGetOffset1C((u16)j) == handle);
        assert((u32)TnGetOffset38((u16)j) == handle);
        assert((u32)TnGetIndexedWord((u16)j) == handle);
    }
}
static void mdlEffectFlushTreeFrame(void *effect)
{ assert(effect == (void *)(uintptr_t)1); ++queue_flushes; check_queue_getters(); }
static void mdlEffectAdvanceAndDrawChildren(void *effect)
{ assert(effect == (void *)(uintptr_t)1); ++queue_advances; check_queue_getters(); }
#define D_005DC974 (*(u8 (*)[84])P4_G2H(0x005dc974u))
#define D_00713270 "synthetic queue slot assertion"
enum { SCR_CONTENT_TYPE_PROCEDURE, SCR_CONTENT_TYPE_LABEL, SCR_CONTENT_TYPE_INSTR,
       SCR_CONTENT_TYPE_MSG, SCR_CONTENT_TYPE_STRINGS };
#define D_0063E3D0 "synthetic procedure source"
#define D_0063E4D0 "synthetic invalid header"
#define D_0063E4F0 ((char *)P4_G2H(0x0063e4f0u))
#define D_005E45E0 ((s32 *)P4_G2H(0x005e45e0u))
#define D_005EFA20 ((f32 *)P4_G2H(0x005efa20u))
#define D_0063FCA0 ((u8 *)P4_G2H(0x0063fca0u))
#define D_006406F0 ((u8 *)P4_G2H(0x006406f0u))
#define D_006432B0 ((u8 *)P4_G2H(0x006432b0u))
#define D_00643D00 ((u8 *)P4_G2H(0x00643d00u))
#define D_0060A410 ((f32 *)P4_G2H(0x0060a410u))
#define D_0060A840 ((f32 *)P4_G2H(0x0060a840u))
#define D_00626F00 ((f32 *)P4_G2H(0x00626f00u))
#define D_0063CB50 ((s32 *)P4_G2H(0x0063cb50u))
#define D_006417E0 ((u8 *)P4_G2H(0x006417e0u))
#define D_0064EFB0 ((s32 *)P4_G2H(0x0064efb0u))
#define D_00749040 ((s16 *)P4_G2H(0x00749040u))
#define D_00749060 ((s16 *)P4_G2H(0x00749060u))
#define D_00749080 ((s16 *)P4_G2H(0x00749080u))
#define D_007490A0 ((s16 *)P4_G2H(0x007490a0u))
#define D_007490C0 ((s16 *)P4_G2H(0x007490c0u))
#define D_007490E0 ((s16 *)P4_G2H(0x007490e0u))
#define D_007494D0 ((f32 *)P4_G2H(0x007494d0u))
#define D_00749500 ((f32 *)P4_G2H(0x00749500u))
static const s32 ps2_months[12]={31,28,31,30,31,30,31,31,30,31,30,31};
static s8 stage_weights[5]={40,25,20,10,5};
#define iGpffffa8a8 stage_weights[0]
static u32 randoms[2]; static unsigned random_index;
static u32 RpRandom(void) { assert(random_index<2); return randoms[random_index++]; }
static s32 observed_day, requests, cues, fades, stops;
static s32 calHasDayEntry(s32 day) { observed_day=day; return day&1; }
static s64 calendarOverrideValue(s32 day,s32 phase) { return (s64)day*256+phase; }
static s32 sndRequestAreaTrack(void) { ++requests; return 0; }
static void sndPlayAreaCue(void) { ++cues; }
static void sdkSndFadeOutChannel(s32 a,s32 b) { assert(a==3&&!b); ++fades; }
static s32 sdkSndStopChannel(s16 a) { assert(a==30); ++stops; return 0; }
static unsigned rank_value, month_value, day_value; static f32 score_value;
static s32 date_index_value;
static s32 datGetDateIndex(void) { return date_index_value; }
static u16 datPersonaGetArcana(u16 id) { return id & 255u; }
static u16 cmmCommunityRecordValue(u16 community) { (void)community; return (u16)rank_value; }
static u32 datPersonaGetExpForLevel(void *persona,u16 level)
{ (void)persona; return 100u*level*level; }
static s32 shdPersonaCalcExpRemaining(struct DatPersonaRecord *persona,s32 extra)
{ unsigned upper=persona->level+(unsigned)extra+1; if(upper>99)upper=99;
  return (s32)(100u*upper*upper-100u*persona->level*persona->level); }
static s32 fclCalendarMonth(void) { return (s32)month_value; }
static s32 fclCalendarDay(void) { return (s32)day_value; }
static f32 datGetAverageRecordScore(void) { return score_value; }
static s32 fclCombineGetMesWindow(void) { return 0; }
static s32 nmMesWindowAcquire(void *p) { (void)p; return 7; }
static s32 message_value;
static void nmMesWindowShowSlotRecord(s32 slot,s32 message) { assert(slot==7); message_value=message; }
static unsigned draw_count; static BtlShuffleSpriteCue drawn[11];
static u8 drawn_alpha[11];
static f32 btlShuffleCalcWave(f32 frame,f32 first,f32 last,s32 curve)
{ assert(curve==1); return (frame-first)/(last-first); }
static void RpSkyRenderStateSet(s32 state,void *value) { (void)state; (void)value; }
static void sdkSpriteDrawOnceAt(s32 palette,s32 sprite,f32 x,f32 y,u8 r,u8 g,u8 b,u8 a)
{ assert(palette==9&&draw_count<11); (void)r;(void)g;(void)b;
  drawn[draw_count]=(BtlShuffleSpriteCue){x,y,0,0,sprite}; drawn_alpha[draw_count++]=a; }
static const char *assertion_argument;
static void kAssertFail(const char *source, int line)
{ (void)source; (void)line; assert(!"unexpected procedure assertion"); }
static void kAssertFailAt(const char *message, const char *source, int line)
{ (void)source; assert(line == 0x19e); assertion_argument = message; }
static ScrScriptWork procedure_work;
static ScrScriptWork *scrCreateWork(ScrHeader *h, ScrContentEntry *e, u8 *p, u8 *l,
                                   u8 *i, u8 *m, u8 *s, s32 index)
{ (void)h;(void)e;(void)p;(void)l;(void)i;(void)m;(void)s;(void)index;return &procedure_work; }
static void axis_context_reference(const void *axis)
{
    static const u32 unit_y[3] = {0, 0x3f800000u, 0};
    assert(axis == P4_G2H(0x005efa20u) && !memcmp(axis, unit_y, 12));
    ++axis_context_count;
}
#include "golden_literal_consumers_generated.inc"
static void queue_context_reference(const void *table)
{
    assert(table == P4_G2H(0x005dc974u));
    ++queue_context_count;
    mdlEffectFlushWithTable((void *)table, (void *)(uintptr_t)1);
    mdlEffectAdvanceAndFlushWithTable((void *)table, (void *)(uintptr_t)1);
    mdlEffectFlushWithTable(NULL, (void *)(uintptr_t)1);
    mdlEffectAdvanceAndFlushWithTable(NULL, (void *)(uintptr_t)1);
    assert((u32)TnGetTableBase() == 0x005dc974u);
}

static u32 word(const u8 *p)
{ return (u32)p[0]|(u32)p[1]<<8|(u32)p[2]<<16|(u32)p[3]<<24; }
static void put_word(u8 *p,u32 value)
{ for(unsigned i=0;i<4;++i)p[i]=(u8)(value>>(i*8)); }
static f32 real(const u8 *p) { u32 bits=word(p); f32 value; memcpy(&value,&bits,4); return value; }
static void put_real(u8 *p,f32 value) { u32 bits; memcpy(&bits,&value,4); put_word(p,bits); }
static const u8 *expected(const P4GoldenModeIdentityArtifacts *a,const char *name)
{ size_t count; const P4GoldenLiteralSource *s=p4_golden_image_literal_sources(&count);
  for(size_t i=0;i<count;++i)if(!strcmp(s[i].name,name))return a->literal_bytes+s[i].output_offset;
  assert(!"missing fixture literal"); return NULL; }
static void check_queue_normalizer(void)
{
    u8 raw[168], out[84];
    for (unsigned bank = 0; bank < 3; ++bank) for (unsigned j = 0; j < 7; ++j) {
        uint64_t value = UINT64_C(0x100000000) + 0x2000u + j * 96u;
        for (unsigned b = 0; b < 8; ++b) raw[(bank * 7 + j) * 8 + b] = (u8)(value >> (b * 8));
    }
    assert(!p4_golden_image_normalize_queues(raw, tnSlotRemap, UINT64_C(0x100000000), 0x2000, 0x4000, out));
    for (unsigned bank = 0; bank < 3; ++bank) for (unsigned j = 0; j < 7; ++j)
        assert(word(out + (bank * 7 + j) * 4) == 0x4000u + j * 48u);
    assert(word(out + 80) == 0x4120u);
    raw[56] ^= 1; memset(out, 0x35, sizeof out);
    assert(p4_golden_image_normalize_queues(raw, tnSlotRemap, UINT64_C(0x100000000), 0x2000, 0x4000, out));
    for (unsigned i = 0; i < sizeof out; ++i) assert(out[i] == 0x35);
    raw[56] ^= 1;
    u8 bad_remap[8]; memcpy(bad_remap, tnSlotRemap, sizeof bad_remap); bad_remap[0] ^= 1;
    assert(p4_golden_image_normalize_queues(raw, bad_remap, UINT64_C(0x100000000), 0x2000, 0x4000, out));
}
static u16 half(const u8 *p) { return (u16)(p[0] | (unsigned)p[1] << 8); }
static void put_half(u8 *p, u16 value) { p[0]=(u8)value; p[1]=(u8)(value>>8); }
static void synthetic_weather(u8 *data)
{
    for(unsigned i=1;i<127;++i){u8 *p=data+28*i;
        p[0]=(u8)(4+(i-1)/28);p[1]=(u8)(1+(i-1)%28);p[2]=(u8)(i%11);
        put_half(p+4,(u16)(i%8+1));put_half(p+6,(u16)(i%5+2));
        put_word(p+8,1u|100u<<8|0x2101u<<16);
        put_word(p+12,10u|20u<<8|0x1234u<<16);
        put_word(p+16,3u|80u<<8|0x4567u<<16);
    }
    put_half(data+3*28+4,123);put_half(data+3*28+6,456);data[3*28+2]=2;
    memset(data+4*28+8,0,20);put_word(data+4*28+8,10u|100u<<8|0x5678u<<16);
    put_word(data+4*28+12,7u|0xbeefu<<16);
    memset(data+5*28+8,0,20);put_word(data+5*28+8,2u|0x1357u<<16);
    put_word(data+5*28+12,3u|0x2468u<<16);
    memset(data+6*28+8,0,20);put_word(data+6*28+8,1u|100u<<8|0x1111u<<16);
    put_word(data+6*28+12,10u|100u<<8|0x9abcu<<16);
    put_word(data+7*28+12,2u|200u<<8|0xabcdu<<16);
    put_word(data+7*28+16,3u|55u<<8|0x7654u<<16);
    data[8*28+2]=6;put_half(data+8*28+6,0xffff);
    data[127*28]=data[127*28+1]=255;
}
static int weather_find_reference(const u8 *table,unsigned rows,int month,int day)
{
    for(unsigned i=1;i<rows;++i){const u8 *p=table+28*i;
        if((s8)p[0]==-1&&(s8)p[1]==-1)return -1;
        if((s8)p[0]==month&&(s8)p[1]==day)return (int)i;}
    return -1;
}
static int weather_rule_reference(const u8 *row,int count,const u16 ids[7])
{
    int first=(s16)half(row+4),second=(s16)half(row+6),found=0,other=0;
    switch((s8)row[2]){
    case 0:return 1;
    case 1:for(int i=0;i<count;++i)found|=first==(ids[i]&255);return found;
    case 2:for(int i=0;i<count;++i)found|=first==ids[i];return found;
    case 3:return first==(ids[count]&255);
    case 4:return first==ids[count];
    case 5:for(int i=0;i<count;++i){found|=first==(ids[i]&255);other|=second==(ids[i]&255);}return found&&other;
    case 6:for(int i=0;i<count;++i){found|=first==ids[i];other|=second==ids[i];}return found&&other;
    case 7:return count==3;case 8:return count==4;case 9:return count==5;case 10:return count==6;
    default:return 0;
    }
}
static unsigned weather_roll_reference(const u8 *row,u32 random,int golden,u32 selected[5],unsigned *draws)
{
    u32 candidates[5];unsigned n=0,count=0,total=0;
    for(unsigned i=0;i<5;++i){u32 value=word(row+8+4*i),kind=value&255u,weight=(value>>8)&255u;
        if(!kind)continue;
        if(weight==100)selected[n++]=value;else{candidates[count++]=value;total+=weight;}}
    *draws=total!=0;
    if(total){s16 roll,cumulative=0;
        if(golden){volatile f32 masked=(f32)(random&0x7fffffffu);volatile f32 uniform=masked*0x1p-31f;
            volatile f32 scaled=uniform*(f32)total;roll=(s16)(u32)scaled;}
        else roll=(s16)(random%total);
        for(unsigned i=0;i<count;++i){cumulative=(s16)(cumulative+((candidates[i]>>8)&255u));
            if(roll<cumulative){selected[n++]=candidates[i];break;}}}
    return n;
}
static void weather_flags_reference(const u8 *row,int golden,int dated,s16 *count,u8 flags[5])
{
    int best=-1,selected=0;
    if(dated)*count=(s8)row[2]!=0?3:0;
    memset(flags,0,5);
    for(unsigned i=0;i<5;++i){const u8 *p=row+(dated?8:0)+4*i;
        if(!p[0])continue;
        int priority=golden&&dated?p[1]:(s8)p[1];
        if(dated){++*count;if(priority==100){flags[i]=1;continue;}}
        if(best<priority){best=priority;selected=(int)i;}}
    if(best!=-1)flags[selected]=1;
}
static void check_weather_refresh(const u8 *table,unsigned rows,const u8 *months,KwlnTask *task,int month,int day,int golden)
{
    DateSelectionWork *state=task->work;
    month_value=(unsigned)month;day_value=(unsigned)day;
    date_index_value=calDayOfYearFromMonthDay(month,day);
    state->counts[0]=state->counts[1]=77;state->nextSelection=9;
    cmmDateSelectionRefresh((u8 *)task);
    int next_month=month,next_day=day+1;
    if(next_day>(int)word(months+4*(month-1))){next_day=1;next_month=month==12?1:month+1;}
    for(unsigned mode=0;mode<2;++mode){int m=mode?next_month:month,d=mode?next_day:day;
        int slot=weather_find_reference(table,rows,m,d);assert(state->slots[mode]==slot);
        u8 flags[5];s16 count=77;
        int index=calDayOfYearFromMonthDay(m,d);
        const u8 *record=slot==-1?D_00643D00+20*((index&1)?3:4):table+28*slot;
        weather_flags_reference(record,golden,slot!=-1,&count,flags);
        assert(state->counts[mode]==count&&!memcmp(state->selectedFlags[mode],flags,5));}
    assert(state->nextMonth==next_month&&state->nextDay==next_day&&!state->nextSelection);
}
static void check_weather_consumers(const P4GoldenModeIdentityArtifacts *a,int golden)
{
    const u8 *table=golden?expected(a,"image/l1/fusion-weather"):D_0063FCA0;
    const u8 *months=golden?expected(a,"image/l1/005e45e0"):(const u8 *)ps2_months;
    unsigned rows=golden?128:94,live=golden?126:2;
    u8 *first=P4_G2H(0x007ac000u),*second=P4_G2H(0x007ac400u);
    memset(first,0,0x314);memset(second,0,0x314);
    KwlnTask task={first},other={second};FclCombineWork *work=(FclCombineWork *)first;
    weather_work_frees=0;witness_weather_create(first);witness_weather_create(second);
    if(golden)assert(!p4_golden_literal_weather_selected_count(P4_H2G(first))&&!p4_golden_literal_weather_selected_count(P4_H2G(second)));
    assert(cmmFindDateEntry(0,0)==-1);
    static const u32 random_values[]={0,0x18000000u,0x1999999au,0x40000000u,0x7fffffffu};
    for(unsigned i=1;i<=live;++i){const u8 *row=table+28*i;
        assert(cmmFindDateEntry(row[0],row[1])==(s8)i);
        work->ruleIndices[0]=(s8)i;
        for(int count=2;count<=6;++count)for(unsigned variant=0;variant<2;++variant){u16 ids[7];
            for(unsigned j=0;j<7;++j)ids[j]=(u16)(77+j);
            ids[0]=(u16)(half(row+4)+variant*256);ids[1]=(u16)(half(row+6)+variant*256);ids[count]=ids[0];
            assert(fclCombineCheckRules((u8 *)&task,(s8)count,ids)==weather_rule_reference(row,count,ids));}
        for(s16 j=0;j<5;++j){s16 header[3];u32 effect;
            witness_weather_display((s8)i,j,header,&effect);
            assert(header[0]==(s8)row[2]&&header[1]==(s16)half(row+4)&&header[2]==(s16)half(row+6));
            assert(effect==(word(row+8+4*j)&0xffff00ffu));}
        month_value=row[0];day_value=row[1];date_index_value=calDayOfYearFromMonthDay(row[0],row[1]);
        for(unsigned r=0;r<sizeof random_values/sizeof random_values[0];++r){u32 selected[5];unsigned draws;
            unsigned n=weather_roll_reference(row,random_values[r],golden,selected,&draws);
            random_index=0;randoms[0]=random_values[r];fclCombineRollOutcomeList((u8 *)&task);
            assert(random_index==draws&&(unsigned)(u8)work->outcomeIdCount==n);
            if(golden)assert(p4_golden_literal_weather_selected_count(P4_H2G(first))==n);
            for(unsigned j=0;j<n;++j){assert((u8)work->outcomeIds[j]==(selected[j]&255u));
                if(golden)assert(p4_golden_literal_weather_selected_effect(P4_H2G(first),j)==selected[j]);}
            if(golden)for(unsigned j=0;j<n;++j)if((selected[j]&255u)==10u){
                assert((u16)witness_weather_skill(first)==(u16)(selected[j]>>16));break;}}
        check_weather_refresh(table,rows,months,&task,row[0],row[1],golden);
    }
    int missing_month=0,missing_day=0;
    for(int m=1;m<=12&&!missing_month;++m)for(int d=1;d<=(int)word(months+4*(m-1));++d)
        if(weather_find_reference(table,rows,m,d)==-1){missing_month=m;missing_day=d;break;}
    assert(missing_month&&cmmFindDateEntry((u8)missing_month,(u8)missing_day)==-1);
    check_weather_refresh(table,rows,months,&task,missing_month,missing_day,golden);
    work->ruleIndices[0]=-1;u16 ids[7]={0};assert(fclCombineCheckRules((u8 *)&task,2,ids)==1);
    if(golden){((FclCombineWork *)second)->ruleIndices[0]=1;random_index=0;randoms[0]=0;
        fclCombineRollOutcomeList((u8 *)&other);unsigned count=p4_golden_literal_weather_selected_count(P4_H2G(second));
        u32 saved[5];for(unsigned j=0;j<count;++j)saved[j]=p4_golden_literal_weather_selected_effect(P4_H2G(second),j);
        witness_weather_destroy(&task);witness_weather_create(first);
        assert(!p4_golden_literal_weather_selected_count(P4_H2G(first))&&p4_golden_literal_weather_selected_count(P4_H2G(second))==count);
        for(unsigned j=0;j<count;++j)assert(p4_golden_literal_weather_selected_effect(P4_H2G(second),j)==saved[j]);
    }else{witness_weather_destroy(&task);witness_weather_create(first);}
    witness_weather_destroy(&task);witness_weather_destroy(&other);
    assert(weather_work_frees==3&&p4_golden_literal_weather_work_release(P4_H2G(first))==-1&&p4_golden_literal_weather_work_release(P4_H2G(second))==-1);
}

void p4t_literal_synthetic(P4GoldenModeIdentityArtifacts *a)
{
    size_t count; const P4GoldenLiteralSource *s=p4_golden_image_literal_sources(&count);
    memset(a->literal_bytes,0,sizeof a->literal_bytes);
    for(size_t k=0;k<count;++k){ u8 *p=a->literal_bytes+s[k].output_offset; const char *n=s[k].name;
        if(s[k].constant){memcpy(p,s[k].constant,s[k].size);continue;}
        if(!strcmp(n,"image/l1/005dc974")){for(unsigned i=0;i<21;++i)put_word(p+4*i,0x796520u+(i%7)*48u);}
        else if(!strcmp(n,"image/l1/005dc974-remap")){memcpy(p,tnSlotRemap,8);}
        else if(!strcmp(n,"image/l1/005e45e0")){for(unsigned i=0;i<12;++i)put_word(p+4*i,(u32)(i==1?29:ps2_months[i]));}
        else if(!strcmp(n,"image/l1/005efa20")){put_real(p,0);put_real(p+4,1);put_real(p+8,0);}
        else if(!strcmp(n,"image/l1/0060a410")){for(unsigned i=0;i<267;++i)put_real(p+4*i,(f32)i*0.25f+1);}
        else if(!strcmp(n,"image/l1/00609c38")){for(unsigned i=0;i<16;++i)p[i]=(u8)(0x80+i);}
        else if(!strcmp(n,"image/l1/00626f00")){for(unsigned i=0;i<24;++i)put_real(p+4*i,0.125f*(f32)(i%8));}
        else if(!strcmp(n,"image/l1/0063cb50")){for(unsigned i=0;i<192;++i){put_word(p+8*i,0x10000u+i);put_word(p+8*i+4,i%4);}put_word(p+8*191,0x10005);}
        else if(!strcmp(n,"image/l1/0063e4f0")){memcpy(p,"synthetic Golden procedure witness",sizeof "synthetic Golden procedure witness");}
        else if(!strcmp(n,"image/l1/fusion-weather")){synthetic_weather(p);}
        else if(!strcmp(n,"image/l1/006417e0")){for(unsigned i=0;i<132;++i)p[i]=(u8)((i/11+i%11)%5);}
        else if(!strcmp(n,"image/l1/006417e0-map")){for(unsigned i=0;i<5;++i)put_word(p+4*i,1000+17*i);}
        else if(!strcmp(n,"image/l1/0064efb0")){for(unsigned i=0;i<11;++i){put_real(p+20*i,(f32)(10+3*i));put_real(p+20*i+4,(f32)(20+2*i));put_word(p+20*i+8,i);put_word(p+20*i+12,i+5);put_word(p+20*i+16,300+i);}}
        else if(!strcmp(n,"image/l1/007494d0")){for(unsigned i=0;i<11;++i)put_real(p+4*i,1.0f+(f32)i*0.25f);}
        else if(!strcmp(n,"image/l1/00749500")){for(unsigned i=0;i<11;++i)put_real(p+4*i,i==2?2.5f:(f32)i*0.25f);}
        else { unsigned category=(unsigned)((strtoul(n+9,NULL,16)-0x749040u)/32u)+1u;
            assert(s[k].size==20&&category>=1&&category<=6);
            for(unsigned i=0;i<10;++i){unsigned value=1000+100*category+i;p[2*i]=(u8)value;p[2*i+1]=(u8)(value>>8);}}
    }
}
void p4t_literal_seed_ps2(void)
{
    put_real(P4_G2H(0x0076151cu), 0.03125f);
    memcpy(D_005E45E0,ps2_months,sizeof ps2_months);
    D_005EFA20[0]=0;D_005EFA20[1]=1;D_005EFA20[2]=0;
    for (unsigned i = 0; i < 21; ++i) put_word(D_005DC974 + i * 4, 0x796520u + (i % 7) * 48u);
    memset(P4_G2H(0x00796520u), 0xa7, 7u * 48u);
    memset(D_0063FCA0,0,2632);
    for(unsigned i=1;i<=2;++i){u8 *row=D_0063FCA0+28*i;row[0]=4;row[1]=(u8)i;row[2]=(u8)(i==1?2:5);
        put_half(row+4,(u16)(100+i));put_half(row+6,(u16)(110+i));
        put_word(row+8,1u|100u<<8|0x1111u<<16);put_word(row+12,10u|20u<<8|0x2222u<<16);
        put_word(row+16,3u|80u<<8|0x3333u<<16);}
    D_0063FCA0[3*28]=D_0063FCA0[3*28+1]=255;
    memcpy(D_006432B0,D_0063FCA0,2632);
    for(unsigned i=0;i<5;++i){u8 *a=D_006406F0+20*i,*b=D_00643D00+20*i;
        memset(a,0,20);memset(b,0,20);a[0]=b[0]=7;a[1]=b[1]=100;
        put_half(a+2,(u16)(40+i));put_half(b+2,(u16)(40+i));}
    for(unsigned c=0;c<6;++c){s16 *p=P4_G2H(0x749040u+32*c);for(unsigned i=0;i<(c==5?12u:16u);++i)p[i]=(s16)(20+10*c+i);}
    for(unsigned i=0;i<134;++i){D_0063CB50[2*i]=(s32)(0x20000+i);D_0063CB50[2*i+1]=(s32)(i%4);}
    memset(D_0063E4F0, 0, 36); memcpy(D_0063E4F0, "synthetic PS2 procedure witness", sizeof "synthetic PS2 procedure witness");
    for(unsigned i=0;i<267;++i)D_0060A410[i]=-(f32)i-1;
    for(unsigned i=0;i<24;++i)D_00626F00[i]=0.0625f*(f32)(i%8);
    for(unsigned i=0;i<178;++i)D_0060A840[i]=(f32)i*0.125f;
    for(unsigned i=0;i<132;++i)D_006417E0[i]=(u8)(i%5);
    for(unsigned i=0;i<11;++i){D_00749500[i]=i?2.5f:0;D_007494D0[i]=1.75f;
        BtlShuffleSpriteCue *p=(BtlShuffleSpriteCue *)D_0064EFB0+i;
        *p=(BtlShuffleSpriteCue){(f32)i,(f32)(2*i),0,5,(s32)(50+i)};}
}
static int month_offset(const u8 *months,unsigned month)
{ int sum=0; for(unsigned m=4;m!=month;m=m==12?1:m+1)sum+=(int)word(months+4*(m-1)); return sum; }
static int percent(u32 r)
{ volatile f32 a=(f32)(r&0x7fffffffu);volatile f32 b=a*0x1p-31f;volatile f32 c=b*100;return(int)c; }
void p4t_literal_consumers(const P4GoldenModeIdentityArtifacts *a,int golden)
{
    assert((p4_game_mode()==P4_GAME_MODE_GOLDEN)==golden);
    const u8 *epsilon = golden ? expected(a, "image/l1/0076151c") : P4_G2H(0x0076151cu);
    assert(witness_test_menu_epsilon() == real(epsilon));
    assert(witness_path_back_time(0.75f) == 0.75f - real(epsilon));
    for (unsigned weight = 0; weight < 256; ++weight) {
        u8 record[0x38] = {0}, output[48];
        memset(output, 0xa9, sizeof output);
        record[0x2b] = (u8)(255u - weight);
        record[0x13] = (u8)weight;
        put_half(record + 0x14, weight & 1u ? 0x8000u : 0x7fffu);
        put_half(record + 0x16, weight & 1u ? 0xffffu : 0u);
        witness_filter3_extra(output, record);
        assert(word(output) == 255u - weight);
        assert((s32)word(output + 4) == (weight & 1u ? -32768 : 32767));
        assert(real(output + 8) == (float)weight / 100.0f);
        assert((s32)word(output + 40) == (weight & 1u ? -1 : 0));
        const u8 *fixed = golden ? expected(a, "image/l1/0063c660") : D_0063C660 + 12;
        assert(!memcmp(output + 12, fixed, 28));
        for (unsigned i = 44; i < sizeof output; ++i) assert(output[i] == 0xa9);
    }
    const u8 *months=golden?expected(a,"image/l1/005e45e0"):(const u8 *)ps2_months;
    for(unsigned m=1;m<=12;++m){int length=(int)word(months+4*(m-1)),base=month_offset(months,m);
        assert(calGetMonthLength((s32)m)==length);
        for(int d=1;d<=length;++d){int day=base+d-1,got_month,got_day;
            assert(calDayOfYearFromMonthDay((s32)m,d)==day);
            calSplitDayOfYear(day,&got_month,&got_day); assert(got_month==(int)m&&got_day==d);
            assert(calIsDayInRange(day,(s32)m,1,(s32)m,length));
            assert(!calIsDayInRange(base-1,(s32)m,1,(s32)m,length));
            assert(calGetDayEntryForMonthDay((s32)m,d)==(s64)day*256+((day&1)?3:4));
            assert(observed_day==day);
        }}
    for(int day=-733;day<=733;++day){int expected_day=golden?day:day%365;
        assert(calGetWeekdayOfDay(day)==((expected_day%7)+5)%7);
        int month,dom,rem=day%(golden?366:365),m=4;
        while(rem>=(int)word(months+4*(m-1))){rem-=(int)word(months+4*(m-1));m=m==12?1:m+1;}
        calSplitDayOfYear(day,&month,&dom);assert(month==m&&dom==rem+1);}
    const u8 *keys=golden?expected(a,"image/l1/0063cb50"):(const u8 *)D_0063CB50;
    unsigned key_count=golden?192:134;
    for(unsigned i=0;i<=key_count;++i){u32 key=i==key_count?0xfefefefeu:word(keys+8*i);int action=3;
        for(unsigned j=0;j<key_count;++j)if(word(keys+8*j)==key){action=(s32)word(keys+8*j+4);break;}
        requests=cues=fades=stops=0;
        assert(mtMiscDispatchKeyAction((s32)(key>>16),(s32)(key&0xffff))==action);
        assert(requests==(action==1||action==2)&&cues==requests);
        assert(fades==(action==3)&&stops==fades);}
    for(unsigned c=1;c<=6;++c)for(unsigned i=0;i<10;++i){const u8 *table;
        char name[32];snprintf(name,sizeof name,"image/l1/%08x",0x749040u+32*(c-1));
        table=golden?expected(a,name):P4_G2H(0x749040u+32*(c-1));
        s16 value=(s16)(table[2*i]|(unsigned)table[2*i+1]<<8);
        assert(fclCombineStageValue((s32)i,(s8)c)==value&&fclCombinePickItem((s8)c,(s8)i)==value);}
    static const unsigned percentages[]={0,39,40,64,65,84,85,94,95,99};
    for(int level=1;level<=99;++level)for(unsigned r=0;r<sizeof percentages/sizeof percentages[0];++r)
        for(unsigned direction=29;direction<=30;++direction){
            randoms[0]=(u32)(((uint64_t)percentages[r]*0x80000000ull)/100+1024);
            randoms[1]=(u32)(((uint64_t)direction*0x80000000ull)/100+1024);random_index=0;
            int p=golden?percent(randoms[0]):(int)(randoms[0]%100);
            int stage=p<40?0:p<65?1:p<85?2:p<95?3:4;
            int subtract=golden?percent(randoms[1])<=29:randoms[1]%100<30;
            int adjusted=subtract?level-stage:level+stage;if(adjusted<1)adjusted=1;if(adjusted>99)adjusted=99;
            int index=golden?(adjusted-1)/10:adjusted==99?10:adjusted/10;
            FclCombineWork work={0};KwlnTask task={&work};
            int value=fclCombineRollOutcome((u8 *)&task,(s16)level,1);
            assert(random_index==2&&work.outcomeCount==index&&value==D_00749040[index]);}
    FclCombineWork menu={0};
    for(unsigned m=1;m<=12;++m)for(int column=0;column<=10;++column){month_value=m;score_value=(f32)column*0.1f;
        int actual_column=(s8)(10.0f*score_value);witness_month_message((u8 *)&menu);
        const u8 *table=golden?expected(a,"image/l1/006417e0"):D_006417E0;
        unsigned selected=table[(m-1)*11+(unsigned)actual_column];
        int wanted=golden?(int)word(expected(a,"image/l1/006417e0-map")+selected*4):(s8)selected+2;
        assert(message_value==wanted);}
    for(unsigned rank=0;rank<=10;++rank)for(unsigned base=10;base<=98;base+=88)for(int apply=0;apply<=1;++apply){
        rank_value=rank;DatPersonaRecord persona={{0},(u8)base};
        f32 scale=golden?real(expected(a,"image/l1/00749500")+4*rank):D_00749500[rank];
        f32 weather=golden?real(expected(a,"image/l1/007494d0")+4*rank):D_007494D0[rank];
        int wanted=0;
        if(golden&&rank){int target=(int)((f32)base+scale),upper=target<99?target:99;
            wanted=100*upper*upper-100*(int)base*(int)base;
            if(target<99){int fraction=(int)((scale-(f32)(upper-(int)base))*10);if(fraction>0)wanted+=((100*(upper+1)*(upper+1)-100*upper*upper)/10)*fraction;}
            if(wanted<0)wanted=0;
        }else if(!golden){int whole=(u8)scale,fraction=(s16)(10*(scale-(f32)(u8)scale));
            if(whole)wanted=shdPersonaCalcExpRemaining(&persona,whole-1);
            if(fraction){int prior=shdPersonaCalcExpRemaining(&persona,(u8)(whole-1));wanted+=fraction*10*((shdPersonaCalcExpRemaining(&persona,whole)-prior)/100);}}
        if(apply)wanted=(int)((f32)wanted*weather);
        assert(cmmCommunityExpRemaining(1,(u8 *)&persona,(s8)apply)==wanted);
    }
    RwV3d positions[89];RwTexCoords uv[89];u8 colors[89][4];
    witness_sky((u8 *)positions,(u8 *)uv,(u8 *)colors);
    const u8 *vertices=golden?expected(a,"image/l1/0060a410"):(const u8 *)D_0060A410;
    assert(!memcmp(positions,vertices,sizeof positions));
    for(unsigned i=0;i<89;++i){assert(uv[i].u==D_0060A840[2*i]&&uv[i].v==8*D_0060A840[2*i+1]);for(unsigned j=0;j<4;++j)assert(colors[i][j]==255);}
    const u8 *cue_table=golden?expected(a,"image/l1/0064efb0"):(const u8 *)D_0064EFB0;
    for(unsigned frame=0;frame<=30;++frame){u16 counter=(u16)frame;draw_count=0;witness_cues(&counter,9);assert(draw_count==11);
        for(unsigned i=0;i<11;++i){const u8 *p=cue_table+20*i;int first=(s32)word(p+8),last=(s32)word(p+12);
            f32 alpha=(int)frame<first?0:(int)frame<last?((f32)frame-(f32)first)/((f32)last-(f32)first):1;
            assert(drawn[i].sprite==(s32)word(p+16)&&drawn[i].x==30*(1-alpha)+real(p)&&drawn[i].y==real(p+4));
            assert(drawn_alpha[i]==(u8)((i<5?206.0f:255.0f)*alpha));}}
    ScrContentEntry entries[6] = {{0}};
    for(unsigned i=0;i<5;++i) entries[i].contentType=i;
    ScrHeader header={{'F','L','W','0'},5,entries};
    assertion_argument=NULL;
    assert(scrCreateProcedureWork(&header,0)==&procedure_work && !assertion_argument);
    entries[5].contentType=99; header.totalEntries=6;
    assert(!scrCreateProcedureWork(&header,0));
    assert(assertion_argument==D_0063E4F0);
    const u8 *diagnostic=golden?expected(a,"image/l1/0063e4f0"):(const u8 *)D_0063E4F0;
    assert(!memcmp(assertion_argument,diagnostic,36));
    axis_context_count=0;witness_axis_contexts();assert(axis_context_count==4);
    const u8 *coefficients=golden?expected(a,"image/l1/00626f00"):(const u8 *)D_00626F00;
    for(unsigned i=0;i<24;++i)assert(witness_layout_alpha((u16)i)==(u8)(64.0f*real(coefficients+4*i)));
    check_queue_normalizer();
    if (golden) assert(!memcmp(D_005DC974, expected(a, "image/l1/005dc974"), 84));
    queue_context_count = queue_flushes = queue_advances = 0;
    witness_queue_contexts();
    assert(queue_context_count >= 4 && queue_flushes == queue_context_count * 2 && queue_advances == queue_flushes);
    for (unsigned i = 0; i < 7u * 48u; ++i) assert(P4_G8(0x00796520u + i) == 0xa7);
    check_weather_consumers(a,golden);
    printf("golden_literals: actual calendar/key/stage/month/EXP/sky/cue/queue/24-f32 read sites passed in %s mode\n",golden?"Golden":"PS2");
}
