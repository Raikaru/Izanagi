#include "p4pc/golden_font.h"
#include "p4pc/game_mode.h"
#include <string.h>
static uint16_t u16(const uint8_t *p) { return (uint16_t)(p[0] | (unsigned)p[1]<<8); }
static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static int span(size_t size, size_t off, size_t length) { return off<=size && length<=size-off; }
const char *p4_golden_font_open(const uint8_t *b, size_t size, P4GoldenFont *out)
{
    if(!b || !out || size<32) return "font.header-span";
    if(u32(b)!=32) return "font.header-variant";
    P4GoldenFont f={0}; f.bytes=b; f.size=size; f.count=u16(b+14); f.width=u16(b+16); f.height=u16(b+18); f.packed_size=u16(b+20);
    if(!f.count || !f.width || !f.height || f.width>64 || f.height>64 || (f.width*f.height)&1 || f.packed_size!=f.width*f.height/2) return "font.cell-variant";
    if(b[10]!=1 || b[22]!=1) return "font.palette-metrics-variant";
    if(size<100) return "font.palette-span";
    f.palette=b+32;
    size_t p=96,n=u32(b+p); p+=4;
    if(!span(size,p,n) || n<(size_t)f.count*2) return "font.metrics-span";
    f.metrics=b+p; p+=n;
    if(!span(size,p,4)) return "font.aux-span";
    n=u32(b+p); p+=4; if(!span(size,p,n)) return "font.aux-span"; p+=n;
    n=(size_t)f.count*4; if(!span(size,p,n)) return "font.map-span"; p+=n;
    if(!span(size,p,32)) return "font.compression-span";
    const uint8_t *h=b+p; size_t header=u32(h),tree=u32(h+4),bits=u32(h+8),positions=u32(h+24);
    if(header!=32 || tree<6 || tree%6 || positions!=(size_t)(f.count+1)*4 || u32(h+16)!=f.packed_size || u32(h+28)!=(uint32_t)f.count*f.packed_size) return "font.compression-variant";
    p+=header;
    if(!span(size,p,tree)) return "font.tree-span";
    f.tree=b+p; f.tree_count=tree/6; p+=tree;
    if(!span(size,p,positions)) return "font.positions-span";
    f.positions=b+p; p+=positions;
    if(!span(size,p,bits) || bits<2) return "font.bits-span";
    f.bits=b+p; f.bit_bytes=bits;
    uint32_t previous=u32(f.positions);
    if(previous) return "font.position-origin";
    for(unsigned i=1;i<=f.count;++i) {
        uint32_t next=u32(f.positions+i*4u);
        if(next<=previous || (uint64_t)next>(uint64_t)bits*8) return "font.positions-order";
        previous=next;
    }
    for(size_t i=0;i<f.tree_count;++i) {
        const uint8_t *node=f.tree+i*6; unsigned a=u16(node+2),c=u16(node+4);
        if(a && (a>=f.tree_count || c>=f.tree_count)) return "font.tree-branch";
    }
    *out=f; return NULL;
}
const char *p4_golden_font_glyph(const P4GoldenFont *f, uint16_t glyph, uint8_t *packed, size_t capacity)
{
    if(!f || !packed || glyph>=f->count || capacity<f->packed_size) return "font.glyph-index-buffer";
    uint32_t start=u32(f->positions+(size_t)glyph*4),end=u32(f->positions+(size_t)(glyph+1)*4);
    size_t node=0,output=0;
    for(uint32_t bit=start;bit<end;++bit) {
        if((size_t)(bit/8)>=f->bit_bytes) return "font.glyph-bit-span";
        const uint8_t *entry=f->tree+node*6;
        unsigned branch=(f->bits[bit/8]>>(bit&7))&1u;
        node=u16(entry+2+branch*2);
        if(node>=f->tree_count) return "font.glyph-branch";
        entry=f->tree+node*6;
        if(!u16(entry+2)) {
            if(output>=f->packed_size) return "font.glyph-overrun";
            packed[output++]=entry[4]; node=0;
        }
    }
    if(node || output!=f->packed_size) return "font.glyph-short";
    return NULL;
}
const char *p4_golden_font_metrics(const P4GoldenFont *f, uint16_t glyph, int *left, int *right)
{
    if(!f || glyph>=f->count || !left || !right) return "font.metric-index";
    int l=(int8_t)f->metrics[(size_t)glyph*2],r=(int8_t)f->metrics[(size_t)glyph*2+1];
    if(l<0 || r<l || r>f->width) return "font.metric-bounds";
    *left=l; *right=r; return NULL;
}
const char *p4_golden_message_render(const P4GoldenFont *f, P4GoldenText text,
    uint8_t *rgba, uint32_t width, uint32_t height, size_t capacity,
    P4GoldenTextPen *pen, P4GoldenControlFn control, void *context)
{
    if(p4_game_mode()!=P4_GAME_MODE_GOLDEN) return "text.ps2-mode";
    if(!f || !rgba || !pen || !width || !height || width>16384 || height>16384 || (uint64_t)width*height*4>capacity) return "text.render-buffer";
    int origin=pen->x; uint8_t packed[P4_GOLDEN_FONT_BITMAP_MAX];
    for(;;) {
        if(pen->x>16384 || pen->x< -16384 || pen->y>16384 || pen->y< -16384) return "text.render-pen";
        P4GoldenToken token; const char *error=p4_golden_text_next(&text,&token); if(error) return error;
        if(token.kind==P4_GOLDEN_END) return NULL;
        if(token.kind==P4_GOLDEN_NEWLINE) { pen->x=origin; pen->y+=f->height; continue; }
        if(token.kind==P4_GOLDEN_CONTROL) {
            if(!control) return "text.render-control";
            error=control(context,&token,pen); if(error) return error; continue;
        }
        int left,right; error=p4_golden_font_metrics(f,token.glyph,&left,&right); if(error) return error;
        if(token.scalar==0x20) { pen->x+=right?right-left+1:f->width; continue; }
        error=p4_golden_font_glyph(f,token.glyph,packed,sizeof(packed)); if(error) return error;
        for(unsigned y=0;y<f->height;++y) for(int x=left;x<right;++x) {
            int64_t dx=(int64_t)pen->x+x-left,dy=(int64_t)pen->y+y;
            if(dx<0 || dy<0 || dx>=width || dy>=height) continue;
            size_t pixel=(size_t)y*f->width+(unsigned)x;
            unsigned index=(packed[pixel/2]>>((pixel&1)*4))&15u;
            const uint8_t *color=f->palette+index*4;
            unsigned opacity=(unsigned)color[3]*2u;
            if(opacity>255) opacity=255; /* FNT retains the PS2 0x80 opaque alpha convention. */
            unsigned alpha=opacity*pen->rgba[3]/255u;
            uint8_t *dest=rgba+((size_t)dy*width+(size_t)dx)*4;
            unsigned alpha_sum=alpha*255u+(unsigned)dest[3]*(255-alpha);
            for(unsigned c=0;c<3;++c) {
                unsigned value=(unsigned)color[c]*pen->rgba[c]/255u;
                unsigned numerator=value*alpha*255u+(unsigned)dest[c]*dest[3]*(255-alpha);
                dest[c]=(uint8_t)(alpha_sum?numerator/alpha_sum:0);
            }
            dest[3]=(uint8_t)((alpha_sum+127)/255u);
        }
        int advance=right?right-left+1:f->width;
        pen->x+=advance;
    }
}
