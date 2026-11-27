/* Original bounded TIM2/BMP decoders; committed model-formats-textures.md only.
 * U06 provisional defaults: row-major, low nibble first, CSM1 indexed8 CLUT,
 * straight full-range RGBA. Unmeasured framing/encoding variants are refused. */
#include "p4pc/golden_model.h"
#include <stdlib.h>
#include <string.h>
static uint32_t u16(const uint8_t *p) { return p[0] | (uint32_t)p[1]<<8; }
static uint32_t u32(const uint8_t *p) { return u16(p) | u16(p+2)<<16; }
static int zero(const uint8_t *p, size_t n) { for(size_t i=0;i<n;++i) if(p[i]) return 0; return 1; }
static int allocate(P4GoldenTexture *t, uint32_t w, uint32_t h)
{
    if(!w || !h || w>16384 || h>16384 || (uint64_t)w*h*4>P4_GOLDEN_TEXTURE_BUDGET) return -1;
    t->storage=malloc((size_t)w*h*4); if(!t->storage) return -1;
    t->level_count=1; t->levels[0]=(P4GoldenTextureLevel){w,h,t->storage}; return 0;
}
static int tim2(const uint8_t *b, size_t size, P4GoldenTexture *t)
{
    if(size<64 || b[4]!=3 || b[5] || u16(b+6)!=1 || !zero(b+8,8)) return -1;
    const uint8_t *p=b+16;
    uint32_t total=u32(p), clut=u32(p+4), image=u32(p+8), colors=u16(p+14);
    uint32_t w=u16(p+20), h=u16(p+22), type=p[19];
    if(u16(p+12)!=48 || p[16] || p[17]!=1 || !zero(p+24,24) || total!=size-16 ||
       total<48 || image>total-48 || clut!=total-48-image ||
       w<8 || h<8 || w>512 || h>512 || (w&(w-1)) || (h&(h-1))) return -1;
    uint64_t pixels=(uint64_t)w*h;
    if(type==2 || type==3) {
        if(p[18] || colors || clut || image!=pixels*(type==2?3u:4u)) return -1;
    } else if(type==4 || type==5) {
        if(p[18]!=3 || colors!=(type==4?16u:256u) || clut!=colors*4 || image!=(type==4?pixels/2:pixels)) return -1;
    } else return -1;
    if(allocate(t,w,h)) return -1;
    const uint8_t *src=p+48, *palette=src+image;
    uint8_t *dst=t->storage;
    for(size_t i=0;i<(size_t)pixels;++i) {
        if(type==2) { memcpy(dst+4*i,src+3*i,3); dst[4*i+3]=255; }
        else if(type==3) memcpy(dst+4*i,src+4*i,4);
        else {
            uint32_t index=type==4?(src[i/2]>>((i&1)*4))&15:src[i];
            if(type==5) index=(index&~24u)|((index&8u)<<1)|((index&16u)>>1);
            memcpy(dst+4*i,palette+4*index,4);
        }
    }
    return 0;
}
static int bmp(const uint8_t *b, size_t size, P4GoldenTexture *t)
{
    if(size<54 || u32(b+2)!=size || u32(b+6) || u32(b+10)!=54 || u32(b+14)!=40 ||
       u16(b+26)!=1 || u16(b+28)!=24 || u32(b+30) || u32(b+46) || u32(b+50)) return -1;
    uint32_t w=u32(b+18), h=u32(b+22);
    /* Support the measured positive, uncompressed dimensions; signed overflow is refused. */
    if(!w || !h || w>16384 || h>16384) return -1;
    size_t row=((size_t)w*3+3)&~(size_t)3;
    if((uint64_t)row*h!=size-54 || (u32(b+34) && u32(b+34)!=size-54)) return -1;
    if(allocate(t,w,h)) return -1;
    uint8_t *dst=t->storage;
    for(uint32_t y=0;y<h;++y) for(uint32_t x=0;x<w;++x) {
        const uint8_t *src=b+54+(size_t)(h-1-y)*row+x*3;
        uint8_t *d=dst+((size_t)y*w+x)*4;
        d[0]=src[2]; d[1]=src[1]; d[2]=src[0]; d[3]=255;
    }
    return 0;
}
int p4_golden_model_image(const uint8_t *b, size_t size, P4GoldenTexture *out)
{
    P4GoldenTexture t={0};
    if(!b || !out || out->storage || size>P4_GOLDEN_TEXTURE_BUDGET) return -1;
    int result=size>=4 && !memcmp(b,"TIM2",4)?tim2(b,size,&t):size>=2 && !memcmp(b,"BM",2)?bmp(b,size,&t):-1;
    if(!result) *out=t;
    return result;
}
