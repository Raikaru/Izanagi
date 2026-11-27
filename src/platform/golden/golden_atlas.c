/* SPR0 layout and selectors: committed clean-room item-icon-flags.md,
 * 7e355dda7. No game assets or RE implementation code. */
#include "p4pc/golden_atlas.h"
#include <stdlib.h>
#include <string.h>
static uint32_t u16(const uint8_t *b) { return (uint32_t)b[0]|(uint32_t)b[1]<<8; }
static uint32_t u32(const uint8_t *b) { return u16(b)|u16(b+2)<<16; }
static int range(size_t size, size_t offset, size_t bytes) { return offset<=size && bytes<=size-offset; }
void p4_golden_atlas_free(P4GoldenAtlas *a)
{
    if(!a) return;
    for(size_t i=0;i<a->texture_count;++i) p4_golden_texture_free(&a->textures[i]);
    free(a->textures); free(a->sprites); memset(a,0,sizeof *a);
}
int p4_golden_atlas_open(const uint8_t *b, size_t size, P4GoldenAtlas *out)
{
    P4GoldenAtlas a={0}; size_t budget=0;
    if(!b || !out || out->textures || out->sprites || size<32 || size>P4_GOLDEN_TEXTURE_BUDGET ||
       memcmp(b+8,"SPR0",4) || !u16(b+20) || u16(b+20)>32767 || u16(b+22)>32767) return -1;
    size_t nt=u16(b+20), ns=u16(b+22), to=u32(b+24), so=u32(b+28);
    if(to<32 || so<32 || !range(size,to,nt*8) || !range(size,so,ns*8)) return -1;
    a.textures=calloc(nt,sizeof *a.textures); a.sprites=ns?calloc(ns,sizeof *a.sprites):NULL;
    if(!a.textures || (ns && !a.sprites)) goto invalid;
    a.texture_count=nt; a.sprite_count=ns;
    for(size_t i=0;i<nt;++i) {
        size_t off=u32(b+to+i*8+4), end=size;
        if(off<32 || !range(size,off,12)) goto invalid;
        if(!memcmp(b+off+8,"TMX0",4)) {
            size_t length=u32(b+off+4);
            if(length<64 || !range(size,off,length)) goto invalid;
            end=off+length;
        } else {
            /* Other embedded formats lack TMX's declared container length. */
            for(size_t j=0;j<nt;++j) { size_t next=u32(b+to+j*8+4); if(next>off && next<end) end=next; }
            for(size_t j=0;j<ns;++j) { size_t next=u32(b+so+j*8+4); if(next>off && next<end) end=next; }
        }
        if(p4_golden_texture_decode(b+off,end-off,&a.textures[i])) goto invalid;
        for(unsigned l=0;l<a.textures[i].level_count;++l) {
            const P4GoldenTextureLevel *t=&a.textures[i].levels[l]; size_t bytes=(size_t)t->width*t->height*4;
            if(bytes>P4_GOLDEN_TEXTURE_BUDGET-budget) goto invalid;
            budget+=bytes;
        }
    }
    for(size_t i=0;i<ns;++i) {
        size_t off=u32(b+so+i*8+4); if(off<32 || !range(size,off,128)) goto invalid;
        const uint8_t *entry=b+off; int32_t texture=(int32_t)u32(entry+0x14);
        P4GoldenSprite *s=&a.sprites[i];
        s->texture_index=(uint32_t)texture;
        s->x0=(int32_t)u32(entry+0x54); s->y0=(int32_t)u32(entry+0x58);
        s->x1=(int32_t)u32(entry+0x5c); s->y1=(int32_t)u32(entry+0x60);
        s->offset_x=(int32_t)u32(entry+0x44); s->offset_y=(int32_t)u32(entry+0x48);
        /* Blank/unused records may exist. Validate the selected rectangle in
         * sprite(), not every unrelated sprite in the resource. */
        s->width=(int16_t)u16(entry+0x74);
        s->height=(int16_t)u16(entry+0x76);
        s->flip=(uint8_t)(u32(entry+0x18)&3);
    }
    *out=a; return 0;
invalid:
    p4_golden_atlas_free(&a); return -1;
}
int p4_golden_atlas_sprite(const P4GoldenAtlas *a, int index, P4GoldenSprite *out)
{
    if(!a || !out || !a->sprites || !a->textures || index<0 || (size_t)index>=a->sprite_count) return -1;
    P4GoldenSprite s=a->sprites[index];
    if(s.texture_index>=a->texture_count) return -1;
    const P4GoldenTexture *texture=&a->textures[s.texture_index];
    const P4GoldenTextureLevel *t=&texture->levels[0];
    uint32_t scale=texture->hd_coordinates?2u:1u;
    if(s.x0<0 || s.y0<0 || s.x1<=s.x0 || s.y1<=s.y0 ||
       (uint32_t)s.x1>t->width/scale || (uint32_t)s.y1>t->height/scale) return -1;
    if(!s.width) s.width=s.x1-s.x0;
    if(!s.height) s.height=s.y1-s.y0;
    s.x0*=(int32_t)scale; s.y0*=(int32_t)scale;
    s.x1*=(int32_t)scale; s.y1*=(int32_t)scale;
    *out=s; return 0;
}
int p4_golden_item_sprite(uint32_t flags)
{
    static const uint32_t masks[]={0x81,0x102,0x204,0x408,0x810,0x1020,0x2040,0x4000,0x8000,
        0x10000,0x20000,0x40000,0x80000,0x100000,0x400000,0x800000,0x1000000,0x2000000,
        0x200000,0x4000000,0x8000000,0x10000000};
    static const uint8_t sprites[]={0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3b,0x3d,0x3f,
        0x40,0x3e,0x50,0x51,0x53,0x4f,0x52,0x54,0x4d,0x3c,0x4c,0x41};
    for(unsigned i=0;i<sizeof sprites;++i) if(flags&masks[i]) return sprites[i];
    return -1;
}
int p4_golden_arcana_sprite(unsigned arcana, int special_state)
{
    if(arcana<1 || arcana>31) return -1;
    if(arcana==25 && special_state) arcana=26;
    return (int)(arcana<=24?0x54+arcana:0x238+arcana);
}
int p4_golden_atlas_crop(const P4GoldenAtlas *a, int index, uint8_t *rgba, size_t size)
{
    P4GoldenSprite s;
    if(!rgba || p4_golden_atlas_sprite(a,index,&s)) return -1;
    const P4GoldenTextureLevel *t=&a->textures[s.texture_index].levels[0];
    size_t w=(size_t)(s.x1-s.x0),h=(size_t)(s.y1-s.y0);
    if(size!=w*h*4) return -1;
    for(size_t y=0;y<h;++y) for(size_t x=0;x<w;++x) {
        size_t sx=(size_t)s.x0+((s.flip&1)?w-1-x:x),sy=(size_t)s.y0+((s.flip&2)?h-1-y:y);
        memcpy(rgba+(y*w+x)*4,t->rgba+(sy*t->width+sx)*4,4);
    }
    return 0;
}
