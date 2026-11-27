#include "p4pc/golden_font.h"
#include "p4pc/game_mode.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static P4GameMode mode=P4_GAME_MODE_PS2;
P4GameMode p4_game_mode(void) { return mode; }
static void put16(uint8_t *p,unsigned v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void put32(uint8_t *p,unsigned v) { put16(p,v); put16(p+2,v>>16); }
static void header(uint8_t *b,unsigned size,unsigned count)
{ memset(b,0,size); b[0]=7; put32(b+4,size); memcpy(b+8,"MSG1",4); put32(b+24,count); put16(b+30,2); }
static void message_vectors(void)
{
    uint8_t b[256]; P4GoldenMessage m={0}; P4GoldenDialog d; P4GoldenText t; P4GoldenToken token;
    header(b,sizeof(b),2); /* Two aliases of one dialog; pointers need not increase. */
    put32(b+36,32); put32(b+44,32); memcpy(b+64,"synthetic",9); put16(b+88,1); put16(b+90,0xffff);
    put32(b+92,68); put32(b+96,10);
    const uint8_t text[]={0xf2,0x23,0,0xf6,'A',10,0x80,0xf6,' ',0}; memcpy(b+100,text,sizeof(text));
    assert(!p4_golden_message_open(b,sizeof(b),&m)); assert(m.dialogs==2);
    assert(!p4_golden_message_dialog(&m,1,&d)); assert(d.count==1 && d.speaker==0xffff);
    assert(!p4_golden_message_page(&d,0,&t));
    assert(!p4_golden_text_next(&t,&token)); assert(token.kind==P4_GOLDEN_CONTROL && token.group==1 && token.index==3 && token.argument_count==1 && token.arguments[1]==0xf6);
    assert(!p4_golden_text_next(&t,&token)); assert(token.kind==P4_GOLDEN_GLYPH && token.scalar=='A' && token.glyph==33);
    assert(!p4_golden_text_next(&t,&token)); assert(token.kind==P4_GOLDEN_NEWLINE);
    assert(!p4_golden_text_next(&t,&token)); assert(token.code==0x80f6 && token.glyph==118);
    assert(!p4_golden_text_next(&t,&token)); assert(token.scalar==' ' && token.glyph==0);
    assert(!p4_golden_text_next(&t,&token)); assert(token.kind==P4_GOLDEN_END);
    assert(p4_golden_message_dialog(&m,2,&d));
    b[28]=1; assert(!strcmp(p4_golden_message_open(b,sizeof(b),&m),"message.relocated")); b[28]=0;
    put32(b+96,1000); assert(p4_golden_message_open(b,sizeof(b),&m)); put32(b+96,10);
    put32(b+92,0); assert(p4_golden_message_open(b,sizeof(b),&m)); put32(b+92,68);
    b[109]='X'; assert(p4_golden_message_open(b,sizeof(b),&m)); b[109]=0;
    for(size_t i=0;i<32;++i) assert(p4_golden_message_open(b,i,&m));
    header(b,sizeof(b),1); put32(b+32,1); put32(b+36,32); put16(b+90,2);
    put32(b+96,76); put32(b+100,78); put32(b+104,4); memcpy(b+108,"A\0B",4);
    assert(!p4_golden_message_open(b,sizeof(b),&m)); assert(!p4_golden_message_dialog(&m,0,&d)); assert(d.kind==1 && d.count==2);
    assert(!p4_golden_message_page(&d,1,&t)); assert(!p4_golden_text_next(&t,&token)); assert(token.scalar=='B');
    const uint8_t bad[][2]={{0xf0,1},{0xf2,1},{0x80,0},{1,0}};
    for(unsigned i=0;i<4;++i) { t=(P4GoldenText){bad[i],2,0}; assert(p4_golden_text_next(&t,&token)); }
    for(unsigned words=1;words<=15;++words) {
        uint8_t control[31]; memset(control,0xf6,sizeof(control)); control[0]=(uint8_t)(0xf0|words); control[1]=0x43; control[words*2]=0;
        t=(P4GoldenText){control,words*2+1,0};
        assert(!p4_golden_text_next(&t,&token)); assert(token.argument_count==words-1 && token.group==2 && token.index==3);
        assert(!p4_golden_text_next(&t,&token)); assert(token.kind==P4_GOLDEN_END);
    }
    header(b,sizeof(b),1); put32(b+36,32); put16(b+88,1); put16(b+90,0);
    put32(b+92,68); put32(b+96,2); memcpy(b+100,"A",2);
    put32(b+40,168); put32(b+44,1); put32(b+200,176); memcpy(b+208,"Speaker",8);
    assert(!p4_golden_message_open(b,sizeof(b),&m)); assert(m.speakers==1);
    assert(!p4_golden_message_speaker(&m,0,&t)); assert(!p4_golden_text_next(&t,&token) && token.scalar=='S');
    put32(b+200,UINT32_MAX); assert(p4_golden_message_open(b,sizeof(b),&m));
    put32(b+200,176); put32(b+36,UINT32_MAX); assert(p4_golden_message_open(b,sizeof(b),&m));
    /* Page fences are offsets, not NULs; out-of-order aliases stay bounded. */
    header(b,sizeof(b),1); put32(b+36,32); put16(b+88,3); put16(b+90,0xffff);
    put32(b+92,79); put32(b+96,76); put32(b+100,79); put32(b+104,5);
    const uint8_t pages[]={'A',0xf1,0x21,'B',0}; memcpy(b+108,pages,sizeof(pages));
    assert(!p4_golden_message_open(b,sizeof(b),&m)); assert(!p4_golden_message_dialog(&m,0,&d));
    assert(!p4_golden_message_page(&d,1,&t) && t.size==3);
    assert(!p4_golden_text_next(&t,&token) && token.scalar=='A');
    assert(!p4_golden_text_next(&t,&token) && token.code==0xf121);
    assert(!p4_golden_text_next(&t,&token) && token.kind==P4_GOLDEN_END);
    assert(!p4_golden_message_page(&d,0,&t) && t.size==2);
    assert(!p4_golden_text_next(&t,&token) && token.scalar=='B');
    header(b,92,1); put32(b+36,32);
    assert(!p4_golden_message_open(b,92,&m)); assert(!p4_golden_message_dialog(&m,0,&d) && !d.count);
}
static size_t font_fixture(uint8_t *b)
{
    memset(b,0,256); put32(b,32); b[10]=1; b[22]=1; put16(b+14,1); put16(b+16,2); put16(b+18,2); put16(b+20,2);
    b[32+4+0]=255; b[32+4+1]=255; b[32+4+2]=255; b[32+4+3]=128;
    put32(b+96,2); b[101]=2; /* one metric, edges [0,2] */
    /* aux length at 102, map at 106; compression starts at 110 */
    uint8_t *h=b+110; put32(h,32); put32(h+4,18); put32(h+8,2); put32(h+16,2); put32(h+24,8); put32(h+28,2);
    uint8_t *tree=h+32; put16(tree+2,1); put16(tree+4,2); tree[6+4]=0x11; tree[12+4]=0;
    put32(tree+18,0); put32(tree+22,2); tree[26]=0; /* 00 -> two 0x11 bytes */
    return 170;
}
static void font_vectors(void)
{
    uint8_t b[256],packed[2],rgba[16]; P4GoldenFont f; size_t size=font_fixture(b); int l,r;
    assert(!p4_golden_font_open(b,size,&f)); assert(!p4_golden_font_glyph(&f,0,packed,2)); assert(packed[0]==0x11 && packed[1]==0x11);
    assert(!p4_golden_font_metrics(&f,0,&l,&r)); assert(l==0 && r==2);
    assert(p4_golden_font_glyph(&f,1,packed,2)); assert(p4_golden_font_glyph(&f,0,packed,1));
    const uint8_t code[]={0x80,0x80,0}; P4GoldenText t={code,sizeof(code),0}; P4GoldenTextPen pen={0,0,{255,255,255,255}};
    memset(rgba,0,sizeof(rgba));
    assert(!strcmp(p4_golden_message_render(NULL,t,NULL,0,0,0,NULL,NULL,NULL),"text.ps2-mode"));
    assert(p4_golden_message_render(&f,t,rgba,2,2,sizeof(rgba),&pen,NULL,NULL)); assert(!rgba[0]);
    mode=P4_GAME_MODE_GOLDEN;
    assert(!p4_golden_message_render(&f,t,rgba,2,2,sizeof(rgba),&pen,NULL,NULL)); assert(rgba[0]==255 && rgba[15]==255 && pen.x==3);
    const uint8_t space[]={' ',0}; t=(P4GoldenText){space,sizeof(space),0}; pen.x=0;
    memset(rgba,0,sizeof(rgba));
    assert(!p4_golden_message_render(&f,t,rgba,2,2,sizeof(rgba),&pen,NULL,NULL)); assert(!rgba[0] && pen.x==3);
    const uint8_t command[]={0xf1,1,0}; t=(P4GoldenText){command,sizeof(command),0};
    assert(!strcmp(p4_golden_message_render(&f,t,rgba,2,2,sizeof(rgba),&pen,NULL,NULL),"text.render-control"));
    put32(b+164,9); assert(!p4_golden_font_open(b,size,&f)); assert(p4_golden_font_glyph(&f,0,packed,2));
    size=font_fixture(b); put16(b+144,99); assert(p4_golden_font_open(b,size,&f));
    size=font_fixture(b); for(size_t i=0;i<size;++i) assert(p4_golden_font_open(b,i,&f));
    size=font_fixture(b); b[168]=2; assert(!p4_golden_font_open(b,size,&f)); assert(!p4_golden_font_glyph(&f,0,packed,2));
    assert(packed[0]==0x11 && packed[1]==0);
}
int main(void) { message_vectors(); font_vectors(); puts("Golden message/font synthetic vectors and PS2 isolation passed"); return 0; }
