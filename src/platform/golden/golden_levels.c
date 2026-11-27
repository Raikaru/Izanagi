/* Clean-room 99af5c91 level-table-source.md; first payload row is level 1. */
#include "p4pc/golden_levels.h"
#include "p4pc/content_digest.h"
#include "golden_pe_data.h"
#include <stdio.h>
#include <string.h>
static uint16_t half(const uint8_t *p) { return (uint16_t)(p[0]|(uint16_t)p[1]<<8); }
static int fail(char *reason,size_t capacity,const char *text)
{
    if(reason && capacity) snprintf(reason,capacity,"%s",text);
    return -1;
}
static int shape(const uint8_t *p)
{
    if(half(p)!=110 || half(p+2)!=100) return 0;
    for(unsigned row=0;row<99;++row) {
        const uint8_t *r=p+row*44;
        if(half(r)!=110 || half(r+2)!=100 || half(r+36) || half(r+38) || half(r+40) || half(r+42)) return 0;
        for(unsigned column=2;column<18;++column) {
            unsigned value=half(r+column*2);
            if(!value || value>999 || (row && value<half(r-44+column*2))) return 0;
        }
    }
    for(unsigned column=2;column<18;++column)
        if(half(p+98*44+column*2)<=half(p+column*2)) return 0;
    return 1;
}
typedef struct LevelRead { uint8_t *out; const char *expected; unsigned found; } LevelRead;
static int visit(const uint8_t *data,size_t size,void *context,char *reason,size_t capacity)
{
    LevelRead *read=context;
    for(size_t offset=0;offset+P4_GOLDEN_LEVEL_BYTES<=size;offset+=2) {
        if(!shape(data+offset)) continue;
        if(++read->found!=1) return fail(reason,capacity,"level source has ambiguous structural candidates");
        char digest[65]; p4_content_digest(data+offset,P4_GOLDEN_LEVEL_BYTES,digest);
        if(strcmp(digest,read->expected)) return fail(reason,capacity,"unsupported level table digest");
        memcpy(read->out,data+offset,P4_GOLDEN_LEVEL_BYTES);
    }
    return 0;
}
static int read_expected(const char *path,uint8_t *out,char exe_digest[65],const char *exe,
    const char *table,char *reason,size_t capacity)
{
    static const char *const names[]={".sdata",".xpdata",".rdata"};
    if(!out) return fail(reason,capacity,"missing level table output");
    LevelRead read={out,table,0};
    if(p4_golden_pe_data(path,exe,names,3,1,visit,&read,exe_digest,reason,capacity)) return -1;
    if(read.found!=1) return fail(reason,capacity,"level source has no unique structural candidate");
    if(reason && capacity) reason[0]=0;
    return 0;
}
int p4_golden_levels_read(const char *path,uint8_t out[P4_GOLDEN_LEVEL_BYTES],
    char exe_digest[65],char *reason,size_t capacity)
{
    return read_expected(path,out,exe_digest,
        "0f327c7669e6cac72aaf355181b1e51edd12fee4d71709fc1802dbf0da70546e",
        "88795bd395dc2bc08bed4ac39607d1359de8a8091d0f9234367108e838eff677",reason,capacity);
}
int p4_golden_levels_pair(const uint8_t *table,size_t size,int level,int id,uint16_t *hp,uint16_t *sp)
{
    if(!table || size!=P4_GOLDEN_LEVEL_BYTES || level<1 || level>99 || id<0 || id>10 || !hp || !sp) return -1;
    const uint8_t *pair=table+(size_t)(level-1)*44+(size_t)id*4;
    *hp=half(pair); *sp=half(pair+2); return 0;
}
#ifdef P4_GOLDEN_LEVELS_TEST
int p4_golden_levels_test_read(const char *path,uint8_t *out,char exe_digest[65],const char *exe,
    const char *table,char *reason,size_t capacity)
{
    return read_expected(path,out,exe_digest,exe,table,reason,capacity);
}
#endif
