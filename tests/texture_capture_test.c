/* Production post-conversion capture must not change guest bytes or GL output.
 * Only original synthetic artwork is emitted; the player's ELF supplies RW/heap. */
#include "p4gl.h"
#include "p4gl_gl.h"
#include "p4gl_readback.h"
#include "p4pc/content_source.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "p4pc/platform.h"
#include "guest_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void *kwlnProbeHeapBudget(void);
extern int32_t func_003f98a0(void *,void *,int32_t);
extern int32_t func_003fb830(void *,void *,int32_t);
extern int32_t func_003fc5f0(void *,void *,int32_t);
extern int32_t func_003fd230(void *,void *,int32_t);
extern int32_t func_003fd520(void *,void *,int32_t);
extern int32_t func_003f5070(int32_t,uint32_t);
extern int32_t func_0040c0f0(int32_t,uint8_t *,int32_t);
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"texture_capture:%d: %s\n",__LINE__,#x); return 1; } } while (0)

static int draw(P4RwRaster *r, uint8_t rgba[8*8*4])
{
    P4Sky2Vertex v[4];
    memset(v,0,sizeof v);
    for (unsigned i=0;i<4;++i) {
        v[i].x=(i&1)?8.0f:0.0f; v[i].y=(i&2)?8.0f:0.0f;
        v[i].u=(i&1)?1.0f:0.0f; v[i].v=(i&2)?1.0f:0.0f;
        v[i].camZ=v[i].recipZ=1.0f;
        v[i].r=v[i].g=v[i].b=v[i].a=255.0f;
    }
    CHECK(func_003f5070(P4GL_RS_TEXTURERASTER,P4_H2G(r))==1);
    CHECK(func_003f5070(P4GL_RS_TEXTUREFILTER,1)==1);
    p4gl_glClearColor(0,0,0,1);
    p4gl_glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    CHECK(func_0040c0f0(4,(uint8_t *)v,4)==1);
    CHECK(p4gl_readback_rgba(8,8,rgba)==0);
    return 0;
}
static int test_authored(int tracking)
{
    static const uint8_t expected[24]={
        255,0,0,255, 0,255,0,129, 0,0,255,0,
        19,37,53,17, 61,79,97,128, 101,127,149,254
    };
    P4ContentSource source={P4_TEXTURE_OVERRIDE,1,P4_TEX_PAL8,P4_PAL_RGBA8888,
                            "synthetic-pack","RESOURCES/AUTHORED"};
    P4ContentSource found;
    /* Authored rasters must be in the real guest arena. On i386 p4t_galloc
     * uses the host heap, which is outside the asset-buffer authority range. */
    P4RwRaster *r=p4pc_heap_calloc(1,sizeof *r), *other=p4pc_heap_calloc(1,sizeof *other);
    P4RwRaster fresh;
    uint8_t upload[sizeof expected], before[8*8*4], after[8*8*4];
    uint32_t locked;
    const uint8_t *shadow;
    int32_t shadow_stride=1,shadow_rows=1;
    GLint maximum;
    CHECK(r && other);
    CHECK(p4_guest_asset_buffer_contains(r,sizeof *r));
    CHECK(p4_guest_asset_buffer_contains(other,sizeof *other));
    memset(r,0,sizeof *r); r->width=3; r->height=2; r->depth=32; r->cFlags=0x80;
    CHECK(func_003f98a0(NULL,r,0x0504)==1);
    fresh=*r;
    CHECK(p4gl_raster_authored_shadow(r,&shadow_stride,&shadow_rows)==NULL &&
          shadow_stride==0 && shadow_rows==0);
    source.origin=P4_TEXTURE_DISC;
    CHECK(p4gl_raster_set_authored_rgba(r,expected,3,2,&source)==-1);
    CHECK(memcmp(r,&fresh,sizeof fresh)==0);
    source.origin=P4_TEXTURE_OVERRIDE;
    r->depth=24;
    CHECK(p4gl_raster_set_authored_rgba(r,expected,3,2,&source)==-1);
    r->depth=32;
    p4gl_glGetIntegerv(0x0D33u /* GL_MAX_TEXTURE_SIZE */,&maximum);
    CHECK(maximum>0 && maximum<INT32_MAX/4);
    r->width=maximum+1;
    CHECK(p4gl_raster_set_authored_rgba(r,expected,r->width,2,&source)==-1);
    r->width=3;
    CHECK(memcmp(r,&fresh,sizeof fresh)==0);
    memcpy(upload,expected,sizeof upload);
    CHECK(p4gl_raster_set_authored_rgba(r,upload,3,2,&source)==0);
    memset(upload,0x5a,sizeof upload); /* host input is neither retained nor borrowed */
    CHECK(r->width==3 && r->height==2 && r->stride==16 && !(r->cFlags&0x80));
    CHECK(r->cpPixels==0 && r->originalStride==16 && r->originalWidth==3 &&
          r->originalHeight==2 && p4gl_raster_content_key(r)==NULL);
    shadow=p4gl_raster_authored_shadow(r,&shadow_stride,&shadow_rows);
    CHECK(shadow && shadow_stride==16 && shadow_rows==2 && r->cpPixels==0 &&
          p4gl_raster_authored_shadow(r,NULL,NULL)==shadow);
    CHECK(p4gl_raster_set_authored_rgba(r,expected,3,2,&source)==-1);
    CHECK(draw(r,before)==0);
    CHECK(func_003fc5f0(&locked,r,2)==1);
    CHECK(shadow==P4_G2H(locked));
    for (unsigned y=0;y<2;++y) {
        const uint8_t *row=(const uint8_t *)P4_G2H(locked)+y*16;
        CHECK(memcmp(row,expected+y*12,12)==0);
        CHECK(row[12]==0 && row[13]==0 && row[14]==0 && row[15]==0);
    }
    if (tracking) {
        CHECK(p4_content_source_find(P4_G2H(locked),32,&found)==1);
        CHECK(found.origin==P4_TEXTURE_OVERRIDE && found.rank==1 &&
              found.format==P4_TEX_RGBA8888 && found.palette_format==P4_PAL_NONE &&
              strcmp(found.provider,"synthetic-pack")==0 &&
              strcmp(found.key,"RESOURCES/AUTHORED")==0);
    }
    CHECK(func_003fd230(NULL,r,0)==1 && p4gl_raster_content_key(r)==NULL);
    CHECK(draw(r,after)==0 && memcmp(before,after,sizeof after)==0);
    CHECK(func_003fb830(NULL,r,0)==1);
    CHECK(p4gl_raster_authored_shadow(r,&shadow_stride,&shadow_rows)==NULL &&
          shadow_stride==0 && shadow_rows==0);

    memset(other,0,sizeof *other);
    other->width=3; other->height=2; other->depth=32; other->cFlags=0x80;
    CHECK(func_003f98a0(NULL,other,0x0505)==1); /* camera texture is never authored */
    CHECK(p4gl_raster_set_authored_rgba(other,expected,3,2,&source)==-1);
    CHECK(func_003fb830(NULL,other,0)==1);
    memset(other,0,sizeof *other);
    other->width=3; other->height=2; other->depth=32; other->cFlags=0x80;
    CHECK(func_003f98a0(NULL,other,0x0504)==1);
    CHECK(p4gl_raster_set_rgba(other,expected,3,2)==0); /* movie retains its own API */
    CHECK(p4gl_raster_set_authored_rgba(other,expected,3,2,&source)==-1);
    CHECK(func_003fb830(NULL,other,0)==1);
    if (tracking) {
        memset(other,0,sizeof *other);
        other->width=3; other->height=2; other->depth=32; other->cFlags=0x80;
        CHECK(func_003f98a0(NULL,other,0x0504)==1);
        p4gl_raster_content_exclude(other,P4_TEXTURE_PROCEDURAL);
        CHECK(p4gl_raster_set_authored_rgba(other,expected,3,2,&source)==-1);
        CHECK(func_003fb830(NULL,other,0)==1);
    }
    p4pc_heap_free(other);
    p4pc_heap_free(r);
    return 0;
}

static int test_main(int argc, char **argv)
{
    static const uint8_t original[16]={255,0,0,255,0,255,0,128,0,0,255,0,19,37,53,255};
    const char *elf=getenv("P4PC_ELF");
    P4ContentSource source={P4_TEXTURE_SYNTHETIC,0,0,0,"synthetic","SYNTHETIC/CHECKER"};
    P4TexturePixels pixels={2,2,P4_TEX_RGBA8888,0,0,original,NULL};
    P4TextureDumpStats stats;
    P4RwRaster *r,*pal;
    P4RwRaster before;
    uint32_t locked;
    uint8_t disabled[8*8*4], enabled[8*8*4], expected[16], canonicalPalette[1024];
    char key[65], paletteKey[65];
    int sawRed=0;
    if (!elf || !*elf) return 77;
    if (argc!=2) return 2;
    p4_os_headless_env();
    p4pc_load_image(elf); p4pc_apply_patches(); p4pc_kernel_pre_init(); kwlnProbeHeapBudget();
    CHECK(p4_guest_asset_buffer_contains(P4_G2H(0x00100000u),4));
    CHECK(!p4_guest_asset_buffer_contains(P4_G2H(0x00100000u),SIZE_MAX));
    CHECK(!p4_guest_asset_buffer_contains(original,sizeof original));
#ifdef P4_GUEST64
    CHECK(!p4_guest_asset_buffer_contains(P4_G2H(P4_GUEST_STACKS),16));
    CHECK(!p4_guest_asset_buffer_contains(P4_G2H(P4_GUEST_STACKS-4),8));
#endif
    CHECK(p4gl_context_open()==0);
    CHECK(p4gl_readback_target_create(8,8,8,8)==0);
    r=p4t_galloc(sizeof *r,16);
    memset(r,0,sizeof *r); r->width=r->height=2; r->depth=32;
    CHECK(func_003f98a0(NULL,r,0x0504)==1);
    CHECK(func_003fc5f0(&locked,r,1)==1);
    memcpy(P4_G2H(locked),original,sizeof original);
    CHECK(func_003fd230(NULL,r,0)==1);
    CHECK(p4gl_raster_content_key(r)==NULL);
    CHECK(draw(r,disabled)==0);
    for (unsigned i=0;i<64;++i)
        if (disabled[i*4]>200 && disabled[i*4+1]<30 && disabled[i*4+2]<30) sawRed=1;
    CHECK(sawRed); /* identical black/stale frames are not a successful witness */
    before=*r;
    CHECK(test_authored(0)==0); /* authored resources do not depend on dump tracking */

    CHECK(p4_content_sources_enable(1,p4_guest_asset_buffer_contains)==0);
    CHECK(p4pc_heap_observer(p4_content_source_forget)==0);
    CHECK(p4_texture_dump_start(argv[1],1)==0);
    p4gl_raster_content_source(r,&source);
    CHECK(func_003fd230(NULL,r,0)==1);
    CHECK(p4_texture_key(&pixels,key)==0);
    CHECK(p4gl_raster_content_key(r) && strcmp(p4gl_raster_content_key(r),key)==0);
    CHECK(memcmp(&before,r,sizeof before)==0);
    CHECK(draw(r,enabled)==0 && memcmp(disabled,enabled,sizeof enabled)==0);
    CHECK(func_003fc5f0(&locked,r,2)==1);
    CHECK(memcmp(P4_G2H(locked),original,sizeof original)==0);
    CHECK(func_003fd230(NULL,r,0)==1);

    /* PAL8 uses CSM1 storage but identity contains logical palette order. */
    pal=p4t_galloc(sizeof *pal,16);
    memset(pal,0,sizeof *pal); pal->width=pal->height=2; pal->depth=8;
    CHECK(func_003f98a0(NULL,pal,0x2504)==1);
    CHECK(func_003fc5f0(&locked,pal,1)==1);
    ((uint8_t *)P4_G2H(locked))[0]=0; ((uint8_t *)P4_G2H(locked))[1]=8;
    ((uint8_t *)P4_G2H(locked))[2]=16; ((uint8_t *)P4_G2H(locked))[3]=255;
    for (unsigned i=0;i<256;++i) {
        unsigned slot=(i&~0x18u)|((i&8u)<<1)|((i&16u)>>1);
        uint8_t *entry=(uint8_t *)P4_G2H(pal->palette)+slot*4;
        entry[0]=(uint8_t)i; entry[1]=(uint8_t)(255-i); entry[2]=37; entry[3]=128;
        memcpy(canonicalPalette+i*4,entry,4); canonicalPalette[i*4+3]=255;
    }
    for (unsigned i=0;i<4;++i) {
        static const unsigned indices[4]={0,8,16,255};
        memcpy(expected+i*4,canonicalPalette+indices[i]*4,4);
    }
    source.palette_format=P4_PAL_RGBA8888;
    p4gl_raster_content_source(pal,&source);
    CHECK(func_003fd230(NULL,pal,0)==1);
    pixels.format=P4_TEX_PAL8; pixels.rgba=expected; pixels.palette=canonicalPalette;
    pixels.palette_entries=256; pixels.palette_format=P4_PAL_RGBA8888;
    CHECK(p4_texture_key(&pixels,paletteKey)==0);
    CHECK(p4gl_raster_content_key(pal) && strcmp(paletteKey,p4gl_raster_content_key(pal))==0);
    /* An unused palette mutation alone still produces a different public key. */
    ((uint8_t *)P4_G2H(pal->palette))[4]^=1;
    canonicalPalette[4]^=1;
    CHECK(func_003fd520(NULL,pal,0)==1);
    CHECK(p4_texture_key(&pixels,key)==0 && strcmp(key,paletteKey)!=0);
    CHECK(p4gl_raster_content_key(pal) && strcmp(key,p4gl_raster_content_key(pal))==0);
    p4gl_raster_content_exclude(pal,P4_TEXTURE_FRAMEBUFFER);
    CHECK(func_003fd230(NULL,pal,0)==1 && p4gl_raster_content_key(pal)==NULL);
    CHECK(func_003fb830(NULL,pal,0)==1);
    CHECK(func_003fb830(NULL,r,0)==1);

    /* Pointer reuse has no stale source/key/exclusion from its former lifetime. */
    memset(r,0,sizeof *r); r->width=r->height=2; r->depth=32;
    CHECK(func_003f98a0(NULL,r,0x0504)==1);
    CHECK(p4gl_raster_content_key(r)==NULL);
    CHECK(func_003fb830(NULL,r,0)==1);
    CHECK(test_authored(1)==0); /* OVERRIDE remains ineligible for original-only dumps */
    CHECK(p4_texture_dump_finish()==0);
    p4_texture_dump_stats(&stats);
    CHECK(stats.completed==3 && stats.errors==0);
    CHECK(p4pc_heap_observer(NULL)==0);
    CHECK(p4_content_sources_enable(0,NULL)==0);
    p4gl_context_close();
    puts("texture_capture: identical originals, palette swaps, exclusions and owned authored RGBA passed");
    return 0;
}
int main(int argc, char **argv) { return p4t_on_guest_stack(test_main,argc,argv); }
