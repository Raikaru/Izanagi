#include "../src/platform/golden/golden_bf.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x); return 1; } } while (0)
static P4GoldenBfNativeCommand common[121], field[51], ai[276], battle[61];
int main(void)
{
    P4GoldenBfNativeGroup originals[]={{common,121},{field,51},{ai,276},{battle,61}};
    for (unsigned group=0;group<4;++group) for (unsigned i=0;i<originals[group].count;++i) {
        P4GoldenBfNativeCommand *entry=(P4GoldenBfNativeCommand *)originals[group].commands+i;
        entry->function=0x100000+4*(i+group*1024); entry->arity=(i+group)%9;
    }
    P4GoldenBfTables tables;
    memset(&tables,0xa5,sizeof tables);
    P4GoldenBfTables before=tables;
    CHECK(p4_golden_bf_tables(P4_GAME_MODE_PS2,&tables,NULL)==0);
    CHECK(!memcmp(&before,&tables,sizeof tables));
    CHECK(p4_golden_bf_tables(P4_GAME_MODE_PS2,NULL,NULL)==0);
    CHECK(p4_golden_bf_tables(P4_GAME_MODE_INVALID,&tables,originals)==-1);
    CHECK(!memcmp(&before,&tables,sizeof tables));
    originals[2].count=275;
    CHECK(p4_golden_bf_tables(P4_GAME_MODE_GOLDEN,&tables,originals)==-1);
    CHECK(!memcmp(&before,&tables,sizeof tables)); originals[2].count=276;
    CHECK(p4_golden_bf_tables(P4_GAME_MODE_GOLDEN,&tables,originals)==1);
    static const uint32_t commands[]={148,8468,8469,8470,8471,8472,8473,8474,8479,8481,8484,8485};
    static const uint32_t arities[]={1,2,1,1,2,0,1,0,1,1,1,1};
    for (unsigned i=0;i<12;++i) {
        const P4GoldenBfCommand *entry=p4_golden_bf_resolve(&tables,commands[i]);
        CHECK(entry && entry->golden==p4_golden_ai_execute && !entry->native_function && entry->arity==arities[i]);
    }
    for (unsigned group=0;group<4;++group) for (unsigned i=0;i<originals[group].count;++i) {
        uint32_t id=group*4096+i;
        if (p4_golden_ai_arity(P4_GAME_MODE_GOLDEN,id)>=0) continue;
        const P4GoldenBfCommand *entry=p4_golden_bf_resolve(&tables,id);
        CHECK(entry && !entry->golden && entry->native_function==originals[group].commands[i].function &&
            entry->arity==originals[group].commands[i].arity);
    }
    CHECK(!p4_golden_bf_resolve(&tables,192));
    CHECK(!p4_golden_bf_resolve(&tables,8192+294));
    CHECK(!p4_golden_bf_resolve(&tables,4096+51));
    CHECK(!p4_golden_bf_resolve(&tables,16384));
    CHECK(!p4_golden_bf_resolve(&tables,149));
    CHECK(!p4_golden_bf_resolve(&tables,8192+291));
    /* All original descriptors are untouched, including null/unfilled slots. */
    for (unsigned group=0;group<4;++group) for (unsigned i=0;i<originals[group].count;++i)
        CHECK(originals[group].commands[i].function==0x100000+4*(i+group*1024) && originals[group].commands[i].arity==(i+group)%9);
    puts("golden_bf_tables: twelve handlers, exact arities, preserved original tables, PS2 no-registration pass");
    return 0;
}
