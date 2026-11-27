/* Original synthetic format vectors; no game bytes. */
#include "p4pc/golden_texture.h"
#include "p4pc/golden_atlas.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void w16(uint8_t *p, unsigned v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); }
static void w32(uint8_t *p, uint32_t v) { w16(p,v); w16(p+2,v>>16); }
static void dds(uint8_t *b, uint32_t cc, unsigned w, unsigned h)
{
    memset(b,0,256); memcpy(b,"DDS ",4); w32(b+4,124); w32(b+76,32);
    w32(b+12,h); w32(b+16,w); w32(b+80,4); w32(b+84,cc);
}
static void bc(void)
{
    uint8_t b[256]; P4GoldenTexture t={0};
    dds(b,0x31545844,4,4); w16(b+128,0xf800); w16(b+130,0x07e0);
    w32(b+132,0xe4e4e4e4);
    assert(!p4_golden_texture_decode(b,136,&t));
    assert(t.level_count==1 && t.levels[0].rgba[0]==255 && t.levels[0].rgba[5]==255);
    assert(t.levels[0].rgba[8]==170 && t.levels[0].rgba[9]==85);
    assert(t.levels[0].rgba[12]==85 && t.levels[0].rgba[13]==170);
    p4_golden_texture_free(&t);
    w16(b+128,0); w16(b+130,0xffff); w32(b+132,0xffffffff);
    assert(!p4_golden_texture_decode(b,136,&t)); assert(t.levels[0].rgba[3]==0);
    p4_golden_texture_free(&t);
    dds(b,0x33545844,3,2); memset(b+128,0x88,8); w16(b+136,0); w16(b+138,0xffff);
    w32(b+140,0xffffffff);
    assert(!p4_golden_texture_decode(b,144,&t));
    assert(t.levels[0].rgba[0]==170 && t.levels[0].rgba[3]==136);
    p4_golden_texture_free(&t);
    dds(b,0x35545844,4,4); b[128]=255; b[129]=0; b[130]=2; w16(b+136,0xf800);
    assert(!p4_golden_texture_decode(b,144,&t)); assert(t.levels[0].rgba[3]==218);
    p4_golden_texture_free(&t);
    b[128]=0; b[129]=255; b[130]=6;
    assert(!p4_golden_texture_decode(b,144,&t)); assert(t.levels[0].rgba[3]==0);
    p4_golden_texture_free(&t); b[130]=7;
    assert(!p4_golden_texture_decode(b,144,&t)); assert(t.levels[0].rgba[3]==255);
    p4_golden_texture_free(&t);
    for(size_t n=0;n<144;++n) assert(p4_golden_texture_decode(b,n,&t));
    w32(b+28,2); w16(b+152,0xf800);
    assert(!p4_golden_texture_decode(b,160,&t));
    assert(t.level_count==2 && t.levels[1].width==2); p4_golden_texture_free(&t);
    dds(b,0x30315844,4,4); w32(b+128,72); w32(b+132,3); w32(b+140,1); w16(b+148,0xf800);
    assert(!p4_golden_texture_decode(b,156,&t)); assert(t.srgb); p4_golden_texture_free(&t);
    w32(b+140,2); assert(p4_golden_texture_decode(b,156,&t));
}
static void tmx(void)
{
    uint8_t b[1200]; P4GoldenTexture t={0};
    const unsigned modes[]={0,1,2,10,19,27,20,36,44};
    for(unsigned k=0;k<sizeof modes/sizeof *modes;++k) {
        unsigned mode=modes[k], bits=mode==0?32:mode==1?24:(mode==2||mode==10)?16:(mode==19||mode==27)?8:4;
        size_t palette=bits==8?1024:bits==4?64:0, size=64+palette+2*bits/8;
        memset(b,0,sizeof b); b[0]=2; memcpy(b+8,"TMX0",4); w32(b+4,(unsigned)size);
        w16(b+18,2); w16(b+20,1); b[22]=(uint8_t)mode; b[16]=(uint8_t)(palette!=0);
        if(palette) { b[64]=200; b[67]=128; }
        else if(bits==32) { b[64]=200; b[67]=128; }
        else if(bits==24) b[64]=200;
        else w16(b+64,31);
        assert(!p4_golden_texture_decode(b,size,&t));
        assert(t.levels[0].rgba[0]==(bits==16?248:200)); assert(t.levels[0].rgba[3]==255);
        p4_golden_texture_free(&t);
        assert(p4_golden_texture_decode(b,size-1,&t));
    }
}
static void atlas(void)
{
    uint8_t b[512]={0}; P4GoldenAtlas a={0}; P4GoldenSprite s;
    memcpy(b+8,"SPR0",4); w32(b+4,sizeof b); w16(b+20,1); w16(b+22,1); w32(b+24,32); w32(b+28,40);
    w32(b+36,256); w32(b+44,64);
    w32(b+64+0x54,0); w32(b+64+0x58,0); w32(b+64+0x5c,2); w32(b+64+0x60,1);
    w32(b+64+0x44,(uint32_t)-3); w16(b+64+0x74,4); b[64+0x18]=3;
    b[256]=2; w32(b+260,72); memcpy(b+264,"TMX0",4); w16(b+274,2); w16(b+276,1); b[323]=128; b[327]=128;
    assert(!p4_golden_atlas_open(b,sizeof b,&a));
    assert(!p4_golden_atlas_sprite(&a,0,&s));
    assert(s.texture_index==0 && s.x1==2 && s.width==4 && s.height==1 && s.offset_x==-3 && s.flip==3);
    assert(p4_golden_atlas_sprite(&a,1,&s)); assert(p4_golden_atlas_sprite(&a,-1,&s));
    p4_golden_atlas_free(&a);
    w32(b+64+0x14,1); assert(!p4_golden_atlas_open(b,sizeof b,&a));
    assert(p4_golden_atlas_sprite(&a,0,&s)); p4_golden_atlas_free(&a);
    assert(p4_golden_item_sprite(0)==-1); assert(p4_golden_item_sprite(0x104)==0x34);
    assert(p4_golden_item_sprite(0x00600000)==0x53); assert(p4_golden_item_sprite(0x20000000)==-1);
    assert(p4_golden_arcana_sprite(24,0)==0x6c); assert(p4_golden_arcana_sprite(25,0)==0x251);
    assert(p4_golden_arcana_sprite(25,1)==0x252); assert(p4_golden_arcana_sprite(27,0)==0x253);
}
static void png_tga_raw(void)
{
    static const uint8_t png[]={0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x08,0x06,0x00,0x00,0x00,0x1f,0x15,0xc4,0x89,0x00,0x00,0x00,0x0d,0x49,0x44,0x41,0x54,0x78,0x9c,0x63,0x10,0x54,0x32,0x6e,0x00,0x00,0x01,0x95,0x00,0xe7,0x6b,0x98,0x46,0x38,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82};
    P4GoldenTexture t={0};
    assert(!p4_golden_texture_decode(png,sizeof png,&t));
    assert(t.levels[0].width==1 && !memcmp(t.levels[0].rgba,"\x11\x22\x33\x80",4));
    p4_golden_texture_free(&t);
    uint8_t corrupt[sizeof png]; memcpy(corrupt,png,sizeof png); corrupt[sizeof png-1]^=1;
    assert(p4_golden_texture_decode(corrupt,sizeof corrupt,&t));
    for(unsigned kind=0;kind<4;++kind) for(unsigned origin=0;origin<4;++origin) {
        uint8_t b[32]={0}; b[2]=(uint8_t)(kind==0?2:kind==1?3:kind==2?10:11);
        w16(b+12,2); w16(b+14,1); unsigned gray=kind&1,channels=gray?1:4;
        b[16]=(uint8_t)(channels*8); b[17]=(uint8_t)(origin*16);
        unsigned off=18; if(kind>=2) b[off++]=1; /* raw RLE packet of 2 pixels */
        if(gray) { b[off]=12; b[off+1]=34; }
        else { b[off]=3; b[off+1]=2; b[off+2]=1; b[off+3]=128; b[off+4]=6; b[off+5]=5; b[off+6]=4; b[off+7]=255; }
        assert(!p4_golden_texture_decode(b,off+channels*2,&t));
        assert(t.levels[0].rgba[0]==(gray?((origin&1)?34:12):((origin&1)?4:1)));
        assert(t.levels[0].rgba[3]==(gray?255:((origin&1)?255:128)));
        p4_golden_texture_free(&t);
        assert(p4_golden_texture_decode(b,off+channels*2-1,&t));
    }
    uint8_t b[256]; dds(b,0,1,1); w32(b+80,0x41); w32(b+88,32);
    w32(b+92,0xff0000); w32(b+96,0xff00); w32(b+100,0xff); w32(b+104,0xff000000);
    b[128]=3; b[129]=2; b[130]=1; b[131]=128;
    assert(!p4_golden_texture_decode(b,132,&t)); assert(!memcmp(t.levels[0].rgba,"\x01\x02\x03\x80",4));
    p4_golden_texture_free(&t);
}
static void palette_and_selector_variants(void)
{
    uint8_t b[1200]={0}; P4GoldenTexture t={0};
    b[0]=2; memcpy(b+8,"TMX0",4); w16(b+18,2); w16(b+20,1); b[16]=1; b[22]=19;
    w32(b+4,1090); b[64+16*4]=99; b[64+16*4+3]=128; b[1088]=8; b[1089]=0;
    assert(!p4_golden_texture_decode(b,1090,&t)); assert(t.levels[0].rgba[0]==99); p4_golden_texture_free(&t);
    for(unsigned fmt=2;fmt<=10;fmt+=8) {
        memset(b+64,0,sizeof b-64); b[17]=(uint8_t)fmt; b[22]=20; w32(b+4,97);
        w16(b+66,31); b[96]=0x10;
        assert(!p4_golden_texture_decode(b,97,&t));
        assert(t.levels[0].rgba[3]==0 && t.levels[0].rgba[4]==248 && t.levels[0].rgba[7]==255);
        p4_golden_texture_free(&t);
    }
    const uint32_t flags[]={0x81,0x102,0x204,0x408,0x810,0x1020,0x2040,0x4000,0x8000,
        0x10000,0x20000,0x40000,0x80000,0x100000,0x400000,0x800000,0x1000000,0x2000000,
        0x200000,0x4000000,0x8000000,0x10000000};
    const int expected[]={0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3b,0x3d,0x3f,0x40,0x3e,
        0x50,0x51,0x53,0x4f,0x52,0x54,0x4d,0x3c,0x4c,0x41};
    for(unsigned i=0;i<22;++i) assert(p4_golden_item_sprite(flags[i])==expected[i]);
}
static void hd_atlas(void)
{
    size_t size=256+64+128*128*4;
    uint8_t *b=calloc(1,size); P4GoldenAtlas a={0}; P4GoldenSprite s;
    assert(b);
    memcpy(b+8,"SPR0",4); w16(b+20,1); w16(b+22,1); w32(b+24,32); w32(b+28,40);
    w32(b+36,256); w32(b+44,64);
    w32(b+64+0x54,8); w32(b+64+0x58,16); w32(b+64+0x5c,24); w32(b+64+0x60,40);
    b[64]=3; /* Not the flip field. */
    b[256]=2; w32(b+260,(uint32_t)(size-256)); memcpy(b+264,"TMX0",4);
    w16(b+274,128); w16(b+276,128);
    for(unsigned y=0;y<128;++y) for(unsigned x=0;x<128;++x) {
        size_t p=320+((size_t)y*128+x)*4; b[p]=(uint8_t)x; b[p+1]=(uint8_t)y; b[p+3]=128;
    }
    for(unsigned hd=0;hd<2;++hd) {
        w16(b+258,hd);
        assert(!p4_golden_atlas_open(b,size,&a));
        assert(!p4_golden_atlas_sprite(&a,0,&s));
        unsigned m=hd?2:1;
        assert(s.x0==(int)(8*m) && s.y0==(int)(16*m) && s.x1==(int)(24*m) && s.y1==(int)(40*m));
        assert(s.width==16 && s.height==24 && s.flip==0);
        size_t n=16*m*24*m*4; uint8_t *crop=malloc(n); assert(crop);
        assert(!p4_golden_atlas_crop(&a,0,crop,n)); assert(crop[0]==8*m && crop[1]==16*m);
        a.sprites[0].flip=3;
        assert(!p4_golden_atlas_crop(&a,0,crop,n)); assert(crop[0]==24*m-1 && crop[1]==40*m-1);
        a.sprites[0].x1=65;
        assert((p4_golden_atlas_sprite(&a,0,&s)!=0)==(hd!=0));
        free(crop); p4_golden_atlas_free(&a);
    }
    free(b);
}
static void bits(uint8_t *b,unsigned *position,unsigned value,unsigned count)
{
    for(unsigned i=0;i<count;++i,++*position)
        b[*position/8]|=(uint8_t)(((value>>i)&1u)<<(*position%8));
}
static void bc7_and_indexed_png(void)
{
    uint8_t b[256]; P4GoldenTexture t={0};
    for(unsigned mode=0;mode<8;++mode) {
        dds(b,0x30315844,4,4); w32(b+128,98); w32(b+132,3); w32(b+140,1);
        b[148]=(uint8_t)(1u<<mode);
        assert(!p4_golden_texture_decode(b,164,&t));
        for(unsigned i=0;i<16;++i) {
            assert(!t.levels[0].rgba[i*4] && !t.levels[0].rgba[i*4+1] && !t.levels[0].rgba[i*4+2]);
            assert(t.levels[0].rgba[i*4+3]==(mode<4?255:0));
        }
        p4_golden_texture_free(&t);
        static const unsigned rgb_bits[]={4,6,5,7,5,7,7,5},alpha_bits[]={0,0,0,0,6,8,7,5};
        unsigned endpoints=(mode==0 || mode==2)?6:(mode==1 || mode==3 || mode==7)?4:2, pos=0;
        memset(b+148,0,16); bits(b+148,&pos,1u<<mode,mode+1);
        if(mode==0 || mode==1 || mode==2 || mode==3 || mode==7) bits(b+148,&pos,0,mode==0?4:6);
        if(mode==4 || mode==5) bits(b+148,&pos,0,2);
        if(mode==4) bits(b+148,&pos,0,1);
        for(unsigned i=0;i<3*endpoints;++i) bits(b+148,&pos,(1u<<rgb_bits[mode])-1,rgb_bits[mode]);
        for(unsigned i=0;i<endpoints && alpha_bits[mode];++i) bits(b+148,&pos,(1u<<alpha_bits[mode])-1,alpha_bits[mode]);
        unsigned pbits=mode==1?2:(mode==0 || mode==3 || mode==6 || mode==7)?endpoints:0;
        for(unsigned i=0;i<pbits;++i) bits(b+148,&pos,1,1);
        assert(!p4_golden_texture_decode(b,164,&t));
        for(unsigned i=0;i<64;++i) assert(t.levels[0].rgba[i]==255);
        p4_golden_texture_free(&t);
    }
    dds(b,0x30315844,4,4); w32(b+128,99); w32(b+132,3); w32(b+140,1);
    unsigned pos=0;
    bits(b+148,&pos,64,7); /* Mode 6, two RGBA endpoints. */
    bits(b+148,&pos,0,7); bits(b+148,&pos,127,7);
    for(unsigned i=0;i<4;++i) bits(b+148,&pos,0,7);
    bits(b+148,&pos,127,7); bits(b+148,&pos,127,7);
    bits(b+148,&pos,0,1); bits(b+148,&pos,1,1);
    bits(b+148,&pos,0,3);
    for(unsigned i=1;i<16;++i) bits(b+148,&pos,15,4);
    assert(pos==128);
    assert(!p4_golden_texture_decode(b,164,&t) && t.srgb);
    assert(!memcmp(t.levels[0].rgba,"\0\0\0\xfe\xff\1\1\xff",8));
    p4_golden_texture_free(&t);
    assert(p4_golden_texture_decode(b,163,&t));
    w32(b+28,2); memcpy(b+164,b+148,16);
    assert(!p4_golden_texture_decode(b,180,&t));
    assert(t.level_count==2 && t.levels[1].width==2 && t.levels[1].height==2);
    assert(!memcmp(t.levels[1].rgba,"\0\0\0\xfe\xff\1\1\xff",8)); p4_golden_texture_free(&t);
    w32(b+28,1); w32(b+16,3); w32(b+12,2);
    assert(!p4_golden_texture_decode(b,164,&t));
    assert(t.levels[0].width==3 && t.levels[0].height==2); p4_golden_texture_free(&t);
    static const uint8_t png[]={0x89,0x50,0x4e,0x47,0x0d,0x0a,0x1a,0x0a,0x00,0x00,0x00,0x0d,0x49,0x48,0x44,0x52,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x08,0x03,0x00,0x00,0x00,0xc3,0xfc,0x8f,0xb8,0x00,0x00,0x00,0x06,0x50,0x4c,0x54,0x45,0xff,0x00,0x00,0x00,0xff,0x00,0xd2,0x87,0xef,0x71,0x00,0x00,0x00,0x02,0x74,0x52,0x4e,0x53,0xff,0x80,0x08,0x0f,0xb3,0x6a,0x00,0x00,0x00,0x0b,0x49,0x44,0x41,0x54,0x78,0x9c,0x63,0x60,0x60,0x04,0x00,0x00,0x04,0x00,0x02,0xbf,0x7a,0x3f,0x4a,0x00,0x00,0x00,0x00,0x49,0x45,0x4e,0x44,0xae,0x42,0x60,0x82};
    assert(!p4_golden_texture_decode(png,sizeof png,&t));
    assert(!memcmp(t.levels[0].rgba,"\xff\0\0\xff\0\xff\0\x80",8)); p4_golden_texture_free(&t);
}
int main(void) { bc(); tmx(); atlas(); png_tga_raw(); palette_and_selector_variants(); hd_atlas(); bc7_and_indexed_png(); puts("Golden texture and atlas vectors passed"); return 0; }
