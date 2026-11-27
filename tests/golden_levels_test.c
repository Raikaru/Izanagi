/* Invented PE/table data only, not an owner-install receipt. */
#include "p4pc/golden_levels.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"FAIL %d: %s (%s)\n",__LINE__,#x,reason); remove(path); return 1; } } while(0)
static void put32(uint8_t *p,unsigned v) { for(unsigned i=0;i<4;++i) p[i]=(uint8_t)(v>>(i*8)); }
static void put16(uint8_t *p,unsigned v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static int store(const char *path,const uint8_t *p,size_t size) {
    FILE *f=fopen(path,"wb"); if(!f) return -1;
    size_t n=fwrite(p,1,size,f); return fclose(f) || n!=size?-1:0;
}
int main(void)
{
    const char *path="golden_levels_synthetic_fixture.tmp";
    char reason[256]="",exe[65],table[65],actual[65];
    uint8_t pe[10240]={0},out[P4_GOLDEN_LEVEL_BYTES];
    uint8_t *data=pe+512;
    pe[0]='M'; pe[1]='Z'; put32(pe+60,64); memcpy(pe+64,"PE\0\0",4);
    pe[68]=0x64; pe[69]=0x86; pe[70]=1; memcpy(pe+88,".sdata",6);
    put32(pe+96,9216); put32(pe+104,9216); put32(pe+108,512); put32(pe+124,0x40000040);
    for(unsigned row=0;row<99;++row) {
        put16(data+row*44,110); put16(data+row*44+2,100);
        for(unsigned col=2;col<18;++col) put16(data+row*44+col*2,10+row*2+col);
    }
    p4_content_digest(data,P4_GOLDEN_LEVEL_BYTES,table);
    p4_content_digest(pe,sizeof pe,exe); CHECK(!store(path,pe,sizeof pe));
    CHECK(!p4_golden_levels_test_read(path,out,actual,exe,table,reason,sizeof reason));
    CHECK(!strcmp(actual,exe) && !memcmp(out,data,sizeof out));
    uint16_t hp=0,sp=0;
    CHECK(!p4_golden_levels_pair(out,sizeof out,1,0,&hp,&sp) && hp==110 && sp==100);
    CHECK(!p4_golden_levels_pair(out,sizeof out,99,8,&hp,&sp) && hp==222 && sp==223);
    CHECK(!p4_golden_levels_pair(out,sizeof out,99,10,&hp,&sp) && !hp && !sp);
    CHECK(p4_golden_levels_pair(out,sizeof out,0,1,&hp,&sp)==-1);
    CHECK(p4_golden_levels_pair(out,sizeof out,100,1,&hp,&sp)==-1);
    CHECK(p4_golden_levels_pair(out,sizeof out,1,11,&hp,&sp)==-1);
    CHECK(p4_golden_levels_pair(out,sizeof out-1,1,1,&hp,&sp)==-1);
    CHECK(p4_golden_levels_test_read(path,out,actual,exe,"wrong",reason,sizeof reason)==-1);
    CHECK(strstr(reason,"digest"));
    put32(pe+124,0x60000040); p4_content_digest(pe,sizeof pe,exe); CHECK(!store(path,pe,sizeof pe));
    CHECK(p4_golden_levels_test_read(path,out,actual,exe,table,reason,sizeof reason)==-1);
    CHECK(strstr(reason,"non-executable"));
    put32(pe+124,0x40000040); memcpy(data+P4_GOLDEN_LEVEL_BYTES,data,P4_GOLDEN_LEVEL_BYTES);
    p4_content_digest(pe,sizeof pe,exe); CHECK(!store(path,pe,sizeof pe));
    CHECK(p4_golden_levels_test_read(path,out,actual,exe,table,reason,sizeof reason)==-1);
    CHECK(strstr(reason,"ambiguous"));
    memset(data+P4_GOLDEN_LEVEL_BYTES,0,P4_GOLDEN_LEVEL_BYTES);
    put32(pe+104,P4_GOLDEN_LEVEL_BYTES-2); put32(pe+96,9216);
    p4_content_digest(pe,sizeof pe,exe); CHECK(!store(path,pe,sizeof pe));
    CHECK(p4_golden_levels_test_read(path,out,actual,exe,table,reason,sizeof reason)==-1);
    CHECK(!remove(path));
    puts("golden_levels: PE admission, digest, ambiguity, file backing and level pairs pass");
    return 0;
}
