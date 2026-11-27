/* Original BC1/2/3 implementation from Khronos S3TC 1.6; BC7 uses reviewed
 * bcdec 0.985 (MIT). License/format record: docs/golden_mode/textures.md. */
#include "p4pc/golden_texture.h"
#include "p4pc/tmx.h"
#include "../../render/p4content_image.h"
#define LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_NO_COMPILE_ALLOCATORS
#include "../../../third_party/lodepng/lodepng.h"
#define BCDECDEF static inline
#define BCDEC_IMPLEMENTATION
#include "../../../third_party/bcdec/bcdec.h"
#include <stdlib.h>
#include <string.h>
/* Shared budgeted allocator supplied by resource_png.c, as in p4model_cook. */
void lodepng_free(void *pointer);

static uint32_t u16(const uint8_t *b) { return (uint32_t)b[0]|(uint32_t)b[1]<<8; }
static uint32_t u32(const uint8_t *b) { return u16(b)|u16(b+2)<<16; }
static int dimensions(uint32_t w, uint32_t h)
{ return w && h && w<=16384 && h<=16384 && (uint64_t)w*h*4<=P4_GOLDEN_TEXTURE_BUDGET; }
static void rgb565(uint32_t v, uint8_t *p)
{
    unsigned r=v>>11, g=(v>>5)&63, b=v&31;
    p[0]=(uint8_t)((r<<3)|(r>>2)); p[1]=(uint8_t)((g<<2)|(g>>4)); p[2]=(uint8_t)((b<<3)|(b>>2)); p[3]=255;
}
static void block(const uint8_t *src, unsigned format, uint8_t pixels[64])
{
    const uint8_t *rgb=src+(format==1?0:8);
    uint8_t c[4][4], alpha[8];
    uint32_t c0=u16(rgb), c1=u16(rgb+2), indices=u32(rgb+4);
    rgb565(c0,c[0]); rgb565(c1,c[1]);
    for(unsigned k=0;k<3;++k) {
        if(c0>c1 || format!=1) {
            c[2][k]=(uint8_t)((2u*c[0][k]+c[1][k])/3u);
            c[3][k]=(uint8_t)((c[0][k]+2u*c[1][k])/3u);
        } else { c[2][k]=(uint8_t)((c[0][k]+c[1][k])/2u); c[3][k]=0; }
    }
    c[2][3]=255; c[3][3]=(c0<=c1 && format==1)?0:255;
    uint64_t ai=0;
    if(format==3) {
        alpha[0]=src[0]; alpha[1]=src[1];
        for(unsigned i=2;i<8;++i) {
            if(src[0]>src[1]) alpha[i]=(uint8_t)(((8-i)*src[0]+(i-1)*src[1])/7);
            else if(i<6) alpha[i]=(uint8_t)(((6-i)*src[0]+(i-1)*src[1])/5);
            else alpha[i]=(uint8_t)(i==6?0:255);
        }
        for(unsigned i=0;i<6;++i) ai|=(uint64_t)src[2+i]<<(i*8);
    }
    for(unsigned i=0;i<16;++i) {
        memcpy(pixels+i*4,c[(indices>>(2*i))&3],4);
        if(format==2) pixels[i*4+3]=(uint8_t)(((src[i/2]>>((i&1)*4))&15)*17);
        else if(format==3) pixels[i*4+3]=alpha[(ai>>(3*i))&7];
    }
}
static int memory_read(void *user, uint64_t offset, void *out, size_t size)
{ memcpy(out,(const uint8_t *)user+(size_t)offset,size); return 0; }
static int dds(const uint8_t *b, size_t size, P4GoldenTexture *t)
{
    P4ContentImageInput input={(void *)b,size,memory_read}; P4ContentImageInfo info;
    if(p4_content_image_probe(&input,&info,NULL,0) || info.container!=P4_IMAGE_DDS ||
       info.format>P4_IMAGE_BC7 || info.level_count>P4_GOLDEN_TEXTURE_MIPS) return -1;
    size_t bytes=0;
    for(unsigned i=0;i<info.level_count;++i) {
        const P4ContentImageLevel *l=&info.levels[i];
        if(!dimensions(l->width,l->height)) return -1;
        size_t n=(size_t)l->width*l->height*4;
        if(n>P4_GOLDEN_TEXTURE_BUDGET-bytes) return -1;
        bytes+=n;
    }
    uint8_t *storage=malloc(bytes); if(!storage) return -1;
    t->storage=storage; t->level_count=info.level_count; t->srgb=(info.flags&P4_IMAGE_SRGB_HINT)!=0;
    size_t offset=0;
    for(unsigned level=0;level<info.level_count;++level) {
        const P4ContentImageLevel *l=&info.levels[level];
        uint8_t *dest=storage+offset; const uint8_t *src=b+(size_t)l->offset;
        t->levels[level]=(P4GoldenTextureLevel){l->width,l->height,dest};
        offset+=(size_t)l->width*l->height*4;
        if(info.format==P4_IMAGE_RGBA8) {
            for(unsigned y=0;y<l->height;++y) memcpy(dest+(size_t)y*l->width*4,src+(size_t)y*l->row_bytes,(size_t)l->width*4);
        } else {
            unsigned step=info.format==P4_IMAGE_BC1?8:16;
            for(unsigned y=0;y<l->height;y+=4) for(unsigned x=0;x<l->width;x+=4) {
                uint8_t rgba[64];
                const uint8_t *compressed=src+(size_t)(y/4)*l->row_bytes+(x/4)*step;
                if(info.format==P4_IMAGE_BC7) bcdec_bc7(compressed,rgba,16);
                else block(compressed,(unsigned)info.format,rgba);
                for(unsigned j=0;j<4 && y+j<l->height;++j)
                    for(unsigned i=0;i<4 && x+i<l->width;++i)
                        memcpy(dest+((size_t)(y+j)*l->width+x+i)*4,rgba+(j*4+i)*4,4);
            }
        }
        if(p4_content_image_transform_rgba(dest,(size_t)l->width*l->height*4,l->width,l->height,(size_t)l->width*4,&info)) {
            p4_golden_texture_free(t); return -1;
        }
    }
    return 0;
}
static int tmx(const uint8_t *b, size_t size, P4GoldenTexture *t)
{
    if(size<64 || u32(b+4)>size || u32(b+4)<64 || b[23] || b[16]>1 || !dimensions(u16(b+18),u16(b+20))) return -1;
    P4TmxImage image={0};
    if(p4tmx_decode(b,u32(b+4),&image)) return -1;
    t->storage=image.rgba; t->level_count=1; t->levels[0]=(P4GoldenTextureLevel){image.width,image.height,image.rgba};
    t->hd_coordinates=(uint8_t)(u16(b+2)==1);
    return 0;
}
static int png(const uint8_t *b, size_t size, P4GoldenTexture *t)
{
    P4ContentImageInput input={(void *)b,size,memory_read}; P4ContentImageInfo info;
    if(p4_content_image_probe(&input,&info,NULL,0) || !dimensions(info.width,info.height)) return -1;
    uint8_t *rgba=NULL; unsigned w=0,h=0;
    if(lodepng_decode32(&rgba,&w,&h,b,size)) { lodepng_free(rgba); return -1; }
    t->png_storage=1; t->storage=rgba; t->level_count=1; t->srgb=(info.flags&P4_IMAGE_SRGB_HINT)!=0;
    t->levels[0]=(P4GoldenTextureLevel){w,h,rgba}; return 0;
}
static int tga(const uint8_t *b, size_t size, P4GoldenTexture *t)
{
    if(size<18 || b[1] || (b[2]!=2 && b[2]!=3 && b[2]!=10 && b[2]!=11) || (b[17]&0xc0)) return -1;
    unsigned w=u16(b+12),h=u16(b+14),channels=b[16]/8;
    int gray=b[2]==3 || b[2]==11, rle=b[2]>=10;
    if(!dimensions(w,h) || (gray?channels!=1:(channels!=3 && channels!=4)) || b[16]!=channels*8) return -1;
    size_t cursor=18u+b[0], n=(size_t)w*h, p=0;
    uint8_t *rgba=malloc(n*4); if(!rgba) return -1;
    while(p<n) {
        unsigned count=1,repeat=0;
        if(rle) { if(cursor>=size) goto invalid; unsigned header=b[cursor++]; count=(header&127)+1; repeat=header&128; }
        if(count>n-p || cursor>size || (size_t)(repeat?1:count)*channels>size-cursor) goto invalid;
        const uint8_t *src=b+cursor;
        for(unsigned i=0;i<count;++i,++p) {
            size_t x=p%w,y=p/w;
            if(b[17]&16) x=w-1-x;
            if(!(b[17]&32)) y=h-1-y;
            uint8_t *dst=rgba+(y*w+x)*4;
            dst[0]=src[gray?0:2]; dst[1]=src[gray?0:1]; dst[2]=src[0]; dst[3]=channels==4?src[3]:255;
            if(!repeat) src+=channels;
        }
        cursor+=(size_t)(repeat?1:count)*channels;
    }
    t->storage=rgba; t->level_count=1; t->levels[0]=(P4GoldenTextureLevel){w,h,rgba}; return 0;
invalid:
    free(rgba); return -1;
}
int p4_golden_texture_decode(const uint8_t *b, size_t size, P4GoldenTexture *out)
{
    P4GoldenTexture t={0};
    if(!b || !out || out->storage || size>P4_GOLDEN_TEXTURE_BUDGET) return -1;
    int result;
    if(size>=4 && !memcmp(b,"DDS ",4)) result=dds(b,size,&t);
    else if(size>=12 && !memcmp(b+8,"TMX0",4)) result=tmx(b,size,&t);
    else if(size>=8 && !memcmp(b,"\x89PNG\r\n\x1a\n",8)) result=png(b,size,&t);
    else result=tga(b,size,&t);
    if(!result) *out=t;
    return result;
}
void p4_golden_texture_free(P4GoldenTexture *t)
{
    if(!t) return;
    if(t->png_storage) lodepng_free(t->storage); else free(t->storage);
    memset(t,0,sizeof *t);
}
