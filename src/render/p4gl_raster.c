/* RenderWare raster handlers for the OpenGL device (stdFunc slots 4, 5, 8, 15, 16, 23, 24,
 * 28). A raster keeps a linear CPU shadow of its pixels. RASTERUNLOCK converts the shadow to
 * RGBA8 and uploads it to a GL texture, so a textured draw reads the texture the core
 * locked. Driver state lives in a port-owned table keyed by raster pointer: the RW 3.7
 * raster struct has no driver-data slot (see docs/renderer_port.md section 4).
 *
 * Handler signature (RwStandardFunc): int32 handler(void *pOut, void *pInOut, int32 nI). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl_gl.h"
#include "p4gl_readback.h"
#include "p4gl.h"
#include "p4pc/gmem.h"
#include "p4pc/platform.h"
#include "p4pc/battle_sim.h"
#include "p4pc/content_source.h"
#include "p4pc/ps2_libc.h"

/* Raster type (cType) and flag bits from rwcore.h:1249 and :1257. */
#define P4_RASTER_TYPE_MASK 0x07u
#define P4_RASTER_TYPE_ZBUFFER 0x01u
#define P4_RASTER_TYPE_CAMERA 0x02u
#define P4_RASTER_TYPE_TEXTURE 0x04u
#define P4_RASTER_TYPE_CAMERATEXTURE 0x05u
#define P4_RASTER_TYPE_NORMAL 0x00u
#define P4_RASTER_DONTALLOCATE 0x80u

/* Format byte (cFormat = rwRASTERFORMAT >> 8, rwcore.h:1302-1320). MIPMAP (0x80) is
 * masked off: only level 0 exists. 0 (DEFAULT) becomes 8888. */
#define P4_FMT_1555 0x01u
#define P4_FMT_565 0x02u
#define P4_FMT_4444 0x03u
#define P4_FMT_LUM8 0x04u
#define P4_FMT_8888 0x05u
#define P4_FMT_888 0x06u
#define P4_FMT_PAL8 0x20u
#define P4_FMT_PAL4 0x40u
#define P4_FMT_BASE_MASK 0x7Fu

#define P4GL_RASTER_SLOTS 4096u
#define P4GL_RASTER_SLOT_MASK (P4GL_RASTER_SLOTS - 1u)

enum { P4_SLOT_EMPTY = 0, P4_SLOT_USED = 1, P4_SLOT_DEAD = 2 };


typedef struct RasterSlot {
    P4RwRaster *key;
    GLuint texture; /* 0 until the first upload */
    uint8_t *pixels; /* linear CPU shadow, stride * height bytes, in guest memory (the game locks it); NULL for non-allocated */
    uint8_t state;
    uint8_t borrowed; /* pixels point into a parent's shadow (SUBRASTER); destroy must not free */
    uint8_t frame;    /* 1 draw raster, 2 display raster: the texture is a copy of the framebuffer */
    int32_t texW, texH; /* size of a frame raster's texture */
    int16_t mipK;       /* TEX1.K as the game set it (RpSkyTextureSetMipmapK): 1/16 units, -2048..2047 */
    uint8_t mipL;       /* TEX1.L (func_003f68a0): 0..3 */
    P4ContentRaster *content;
    uint8_t content_origin;
} RasterSlot;

static RasterSlot sSlots[P4GL_RASTER_SLOTS];

static unsigned slotHash(const void *key)
{
    uint32_t v = P4_H2G(key) >> 4;
    return (v * 2654435761u >> 20) & P4GL_RASTER_SLOT_MASK;
}

static RasterSlot *slotFind(const P4RwRaster *key)
{
    unsigned idx = slotHash(key);
    for (unsigned i = 0; i < P4GL_RASTER_SLOTS; i++, idx = (idx + 1u) & P4GL_RASTER_SLOT_MASK) {
        RasterSlot *s = &sSlots[idx];
        if (s->state == P4_SLOT_EMPTY) {
            return NULL;
        }
        if (s->state == P4_SLOT_USED && s->key == key) {
            return s;
        }
    }
    return NULL;
}

/* Claims a slot for key. Returns NULL when the table is full. */
static RasterSlot *slotClaim(P4RwRaster *key)
{
    unsigned idx = slotHash(key);
    RasterSlot *freeSlot = NULL;
    for (unsigned i = 0; i < P4GL_RASTER_SLOTS; i++, idx = (idx + 1u) & P4GL_RASTER_SLOT_MASK) {
        RasterSlot *s = &sSlots[idx];
        if (s->state == P4_SLOT_USED) {
            if (s->key == key) {
                return s;
            }
            continue;
        }
        if (freeSlot == NULL) {
            freeSlot = s;
        }
        if (s->state == P4_SLOT_EMPTY) {
            break;
        }
    }
    if (freeSlot != NULL) {
        freeSlot->state = P4_SLOT_USED;
        freeSlot->key = key;
        freeSlot->texture = 0;
        freeSlot->pixels = NULL;
        freeSlot->borrowed = 0;
        freeSlot->frame = 0;
        freeSlot->texW = freeSlot->texH = 0;
        freeSlot->content = NULL;
        freeSlot->content_origin = P4_TEXTURE_UNKNOWN;
    }
    return freeSlot;
}

/* A paletted raster carries its palette's colour format beside the index flag (PAL8 | 8888 is
 * 0x2500): the index flag decides the pixel layout, the palette entries are RGBA bytes. */
static uint32_t formatBase(const P4RwRaster *r)
{
    const uint32_t f = (uint32_t)r->cFormat & P4_FMT_BASE_MASK;

    if (f & P4_FMT_PAL4) {
        return (uint32_t)P4_FMT_PAL4 << 8;
    }
    if (f & P4_FMT_PAL8) {
        return (uint32_t)P4_FMT_PAL8 << 8;
    }
    return f << 8;
}

/* Bytes per row for a format base (rwRASTERFORMAT value), or 0 when unsupported. */
static int32_t strideFor(uint32_t base, int32_t width)
{
    switch (base) {
    case 0x0100u: /* 1555 */
    case 0x0200u: /* 565 */
    case 0x0300u: /* 4444 */
        return width * 2;
    case 0x0400u: /* LUM8 */
    case 0x2000u: /* PAL8 */
        return width;
    case 0x0500u: /* 8888 */
        return width * 4;
    case 0x0600u: /* 888 */
        return width * 3;
    case 0x4000u: /* PAL4: two pixels per byte */
        return (width + 1) / 2;
    default:
        return 0;
    }
}

static uint8_t expand5(uint32_t v)
{
    return (uint8_t)((v << 3) | (v >> 2));
}

static uint8_t expand4(uint32_t v)
{
    return (uint8_t)((v << 4) | v);
}

static uint8_t expand6(uint32_t v)
{
    return (uint8_t)((v << 2) | (v >> 4));
}

/* A raster palette is the CLUT as the GS reads it from memory: RGBA bytes, PS2 alpha (0x80 = opaque, so
 * the alpha is doubled to 0..255). An 8-bit texel (csm1 set) reads CLUT entry csm1(index), index bits 3 and 4
 * swapped (CSM1); a 4-bit texel reads its index; the game's TMX loader and the native texture loader both store the palette in that order.
 * Without the swap the title logo's letters read an alpha-0x4d entry instead of an opaque black one
 * (found by reading the logo draw's CLUT out of a PCSX2 GS dump). */
static void paletteEntry(const P4RwRaster *r, uint32_t index, int csm1, uint8_t *o)
{
    if (r->palette == 0) {
        o[0] = o[1] = o[2] = o[3] = 0;
        return;
    }
    if (csm1) {
        index = (index & ~0x18u) | ((index & 8u) << 1) | ((index & 16u) >> 1);
    }
    const uint8_t *p = (const uint8_t *)P4_G2H(r->palette) + index * 4u;
    o[0] = p[0];
    o[1] = p[1];
    o[2] = p[2];
    o[3] = (uint8_t)(p[3] >= 128 ? 255 : (p[3] * 2));
}

static int setRasterContent(RasterSlot *s, const P4ContentSource *source)
{
    if (p4_content_raster_set(&s->content,source)!=0) return -1;
    s->content_origin=(uint8_t)source->origin;
    return 0;
}

void p4gl_raster_content_source(P4RwRaster *r, const P4ContentSource *source)
{
    RasterSlot *s;
    if (!p4_content_sources_enabled || !r || !source) return;
    s=slotFind(r);
    if (!s || s->frame || (r->cType&P4_RASTER_TYPE_MASK)==P4_RASTER_TYPE_CAMERATEXTURE ||
        s->content_origin==P4_TEXTURE_FRAMEBUFFER || s->content_origin==P4_TEXTURE_MOVIE ||
        s->content_origin==P4_TEXTURE_PROCEDURAL) return;
    if (setRasterContent(s,source)!=0 && p4_texture_dump_active()) {
        /* Failure belongs to diagnostic capture, not the game's raster upload. */
        p4_texture_dump_submit(NULL,P4_TEXTURE_DISC,"raster metadata allocation",NULL);
    }
}
void p4gl_raster_content_exclude(void *raster, int origin)
{
    RasterSlot *s;
    if (!p4_content_sources_enabled || !raster) return;
    s=slotFind(raster);
    if (!s) return;
    if (origin==P4_TEXTURE_UNKNOWN && (s->content_origin==P4_TEXTURE_FRAMEBUFFER ||
        s->content_origin==P4_TEXTURE_MOVIE || s->content_origin==P4_TEXTURE_PROCEDURAL)) return;
    if (origin!=P4_TEXTURE_UNKNOWN && s->content_origin!=origin && p4_texture_dump_active())
        p4_texture_dump_submit(NULL,(enum P4TextureOrigin)origin,NULL,NULL);
    p4_content_raster_free(s->content); s->content=NULL;
    s->content_origin=(uint8_t)origin;
}
const char *p4gl_raster_content_key(const P4RwRaster *raster)
{
    RasterSlot *s=raster?slotFind(raster):NULL;
    return s && s->content && s->content->key[0]?s->content->key:NULL;
}
void p4gl_raster_content_reset(void)
{
    for (unsigned i=0;i<P4GL_RASTER_SLOTS;++i) {
        p4_content_raster_free(sSlots[i].content);
        sSlots[i].content=NULL;
        sSlots[i].content_origin=P4_TEXTURE_UNKNOWN;
    }
}
/* The source service owns raster capture on its enabling render thread.
 * Submit copies the used palette bytes before returning to this scratch. */
static uint8_t capturePaletteScratch[256*4];


static void captureRaster(const P4RwRaster *r, RasterSlot *s, const uint8_t *rgba)
{
    P4TexturePixels pixels;
    uint32_t base=formatBase(r);
    uint32_t type=r->cType&P4_RASTER_TYPE_MASK;
    enum P4TextureOrigin origin=(enum P4TextureOrigin)s->content_origin;
    if (s->frame || type==P4_RASTER_TYPE_CAMERA || type==P4_RASTER_TYPE_CAMERATEXTURE ||
        type==P4_RASTER_TYPE_ZBUFFER) origin=P4_TEXTURE_FRAMEBUFFER;
    if (p4_content_sources_enabled) p4_content_sources_assert_owner();
    if (!s->content || origin==P4_TEXTURE_FRAMEBUFFER || origin==P4_TEXTURE_PROCEDURAL ||
        origin==P4_TEXTURE_MOVIE || origin==P4_TEXTURE_OVERRIDE) {
        if (s->content) s->content->key[0]=0;
        p4_texture_dump_submit(NULL,origin,NULL,NULL);
        return;
    }
    memset(&pixels,0,sizeof pixels);
    pixels.width=(uint32_t)r->width; pixels.height=(uint32_t)r->height;
    pixels.format=s->content->source.format;
    if (!pixels.format)
        pixels.format=base==0x4000u?P4_TEX_PAL4:base==0x2000u?P4_TEX_PAL8:base>>8;
    pixels.rgba=rgba;
    if (base==0x4000u || base==0x2000u) {
        pixels.palette_entries=base==0x4000u?16:256;
        pixels.palette_format=s->content->source.palette_format;
        if (!pixels.palette_format) pixels.palette_format=P4_PAL_RGBA8888;
        for (uint32_t i=0;i<pixels.palette_entries;++i)
            paletteEntry(r,i,base==0x2000u,capturePaletteScratch+4*i);
        pixels.palette=capturePaletteScratch;
    }
    s->content->key[0]=0;
    p4_texture_dump_submit(&pixels,origin,s->content->source.key,s->content->key);
}

/* Converts the linear shadow to RGBA8 (width * height * 4 bytes). */
static void convertToRgba8(const P4RwRaster *r, const uint8_t *src, uint8_t *dst)
{
    const uint32_t base = formatBase(r);
    const int32_t w = r->width;
    const int32_t h = r->height;
    const int32_t stride = r->stride;

    for (int32_t y = 0; y < h; y++) {
        const uint8_t *row = src + (size_t)y * (size_t)stride;
        for (int32_t x = 0; x < w; x++) {
            uint8_t *o = dst + ((size_t)y * (size_t)w + (size_t)x) * 4u;
            switch (base) {
            case 0x0100u: { /* 1555: bit 15 alpha, 14-10 R, 9-5 G, 4-0 B */
                uint32_t p = (uint32_t)row[2 * x] | ((uint32_t)row[2 * x + 1] << 8);
                o[0] = expand5((p >> 10) & 31u);
                o[1] = expand5((p >> 5) & 31u);
                o[2] = expand5(p & 31u);
                o[3] = (p & 0x8000u) ? 255 : 0;
                break;
            }
            case 0x0200u: { /* 565 */
                uint32_t p = (uint32_t)row[2 * x] | ((uint32_t)row[2 * x + 1] << 8);
                o[0] = expand5((p >> 11) & 31u);
                o[1] = expand6((p >> 5) & 63u);
                o[2] = expand5(p & 31u);
                o[3] = 255;
                break;
            }
            case 0x0300u: { /* 4444: A 15-12, R 11-8, G 7-4, B 3-0 */
                uint32_t p = (uint32_t)row[2 * x] | ((uint32_t)row[2 * x + 1] << 8);
                o[0] = expand4((p >> 8) & 15u);
                o[1] = expand4((p >> 4) & 15u);
                o[2] = expand4(p & 15u);
                o[3] = expand4((p >> 12) & 15u);
                break;
            }
            case 0x0400u: /* LUM8 */
                o[0] = o[1] = o[2] = row[x];
                o[3] = 255;
                break;
            case 0x0600u: /* 888 */
                o[0] = row[3 * x];
                o[1] = row[3 * x + 1];
                o[2] = row[3 * x + 2];
                o[3] = 255;
                break;
            case 0x2000u: /* PAL8 */
                paletteEntry(r, row[x], 1, o);
                break;
            case 0x4000u: { /* PAL4: low nibble is the even pixel */
                uint8_t byte = row[x / 2];
                uint32_t index = (x & 1) ? (byte >> 4) : (byte & 15u);
                paletteEntry(r, index, 0, o);
                break;
            }
            case 0x0500u: /* 8888: RGBA byte order */
            default:
                o[0] = row[4 * x];
                o[1] = row[4 * x + 1];
                o[2] = row[4 * x + 2];
                o[3] = row[4 * x + 3];
                break;
            }
        }
    }
}

static int isUploadableType(const P4RwRaster *r)
{
    uint32_t type = r->cType & P4_RASTER_TYPE_MASK;
    return type == P4_RASTER_TYPE_NORMAL || type == P4_RASTER_TYPE_TEXTURE ||
           type == P4_RASTER_TYPE_CAMERATEXTURE;
}

static int32_t pow2Ceil(int32_t v)
{
    int32_t p = 1;
    while (p < v) {
        p <<= 1;
    }
    return p;
}

/* Creates or refreshes the GL texture of a slot from w x h RGBA8 pixels. GS textures are power-of-two
 * sized and the game's texture coordinates are relative to that size (the movie quad uses
 * 641/1024 x 449/512 for a 640x448 frame), so the GL texture is the padded size with the image in its
 * top-left corner; the padding is transparent black and is not sampled by those coordinates. */
static void uploadRgba8(RasterSlot *s, int32_t w, int32_t h, const uint8_t *rgba)
{
    if (p4_battle_sim_renderless) {
        if (!s->texture) p4gl_glGenTextures(1, &s->texture);
        return;
    }
    const int32_t pw = pow2Ceil(w);
    const int32_t ph = pow2Ceil(h);

    p4gl_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    if (s->texture == 0) {
        p4gl_glGenTextures(1, &s->texture);
        p4gl_glBindTexture(GL_TEXTURE_2D, s->texture);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        if (pw == w && ph == h) {
            p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
            return;
        }
        uint8_t *blank = calloc((size_t)pw * (size_t)ph, 4u);
        p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, pw, ph, 0, GL_RGBA, GL_UNSIGNED_BYTE, blank);
        free(blank);
    } else {
        p4gl_glBindTexture(GL_TEXTURE_2D, s->texture);
    }
    p4gl_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
}

/* Converts and uploads the whole level 0 shadow. Returns 0 on success. */
static int uploadRaster(P4RwRaster *r, RasterSlot *s)
{
    if (p4_battle_sim_renderless) {
        /* CPU shadows/lock semantics remain real; converting pixels for a
         * nonexistent device would allocate and copy without a consumer. */
        uploadRgba8(s, r->width, r->height, NULL);
        return 0;
    }
    const size_t count = (size_t)r->width * (size_t)r->height * 4u;
    uint8_t *rgba = malloc(count);
    if (rgba == NULL) {
        fprintf(stderr, "p4gl: out of memory converting a %dx%d raster\n", r->width, r->height);
        return 1;
    }
    convertToRgba8(r, s->pixels, rgba);
    if (p4_texture_dump_active()) captureRaster(r,s,rgba);
    uploadRgba8(s, r->width, r->height, rgba);
    free(rgba);
    return 0;
}

/* Replaces level 0 of a raster with width x height RGBA8 pixels (rows packed). For rasters the game
 * creates without a CPU copy (rwRASTERDONTALLOCATE, the movie frame raster), whose pixels the GS
 * received by DMA in retail. Returns 0, or -1 for an unknown raster or a size that is not the raster's. */
int p4gl_raster_set_rgba(P4RwRaster *r, const uint8_t *rgba, int32_t width, int32_t height)
{
    RasterSlot *s = r != NULL ? slotFind(r) : NULL;
    if (s == NULL || rgba == NULL || width != r->width || height != r->height) {
        return -1;
    }
    p4gl_raster_content_exclude(r,P4_TEXTURE_MOVIE);
    uploadRgba8(s, width, height, rgba);
    return 0;
}

/* Explicitly authored resources are not movies or original-disc assets.
 * The public setter accepts only a fresh deferred texture; the private create
 * path may instead adopt the shadow of the raster it just created. Both keep
 * a guest-readable POT-padded shadow owned by RASTERDESTROY. */
static int setAuthoredRgba(P4RwRaster *r, const uint8_t *rgba,
                           int32_t width, int32_t height,
                           const P4ContentSource *source, int fresh_owned)
{
    RasterSlot *s;
    GLint limit=0;
    GLenum error;
    int32_t pw,ph;
    size_t stride,bytes,row_bytes;
    uint8_t *pixels;
    if (!r || !rgba || !p4_guest_asset_buffer_contains(r,sizeof *r) ||
        !p4_content_source_authored(NULL,source)) return -1;
    s=slotFind(r);
    if (!s || s->texture || s->borrowed || s->frame || s->content ||
        s->content_origin!=P4_TEXTURE_UNKNOWN ||
        (fresh_owned ? (!s->pixels || (r->cFlags&P4_RASTER_DONTALLOCATE)) :
                       (s->pixels || !(r->cFlags&P4_RASTER_DONTALLOCATE))) ||
        (r->cType&P4_RASTER_TYPE_MASK)!=P4_RASTER_TYPE_TEXTURE ||
        r->cFormat!=P4_FMT_8888 ||
        r->palette || r->cpPixels || r->depth!=32 || r->nOffsetX || r->nOffsetY ||
        (r->parent && r->parent!=P4_H2G(r)) ||
        width<=0 || height<=0 || width!=r->width || height!=r->height ||
        width>INT32_MAX/4 || !p4gl_glGetIntegerv || !p4gl_glGetError) return -1;
    p4gl_glGetIntegerv(0x0D33u /* GL_MAX_TEXTURE_SIZE */,&limit);
    if (limit<=0 || width>limit || height>limit) return -1;
    pw=pow2Ceil(width); ph=pow2Ceil(height);
    if (pw>limit || ph>limit || pw>INT32_MAX/4) return -1;
    stride=(size_t)pw*4;
    if ((size_t)ph>SIZE_MAX/stride || (size_t)ph>UINT32_MAX/stride) return -1;
    bytes=stride*(size_t)ph;
    row_bytes=(size_t)width*4;
    if (setRasterContent(s,source)!=0) return -1;
    if (!p4_content_source_authored(&s->content->source,source)) goto failed;
    pixels=fresh_owned ? p4pc_heap_realloc(s->pixels,bytes) : p4pc_heap_malloc(bytes);
    if (!pixels) goto failed;
    /* The fresh raster owns its original shadow even if upload later fails. */
    if (fresh_owned) s->pixels=pixels;
    for (int32_t y=0;y<height;++y) {
        uint8_t *row=pixels+(size_t)y*stride;
        func_0043f810(row,rgba+(size_t)y*row_bytes,(uint32_t)row_bytes);
        if (stride!=row_bytes) func_0043f9c8(row+row_bytes,0,(uint32_t)(stride-row_bytes));
    }
    if (ph!=height) func_0043f9c8(pixels+(size_t)height*stride,0,(uint32_t)(bytes-(size_t)height*stride));
    /* Already padded, so the common upload allocates no second blank image. */
    uploadRgba8(s,pw,ph,pixels);
    error=p4gl_glGetError();
    if (!s->texture || error!=GL_NO_ERROR) {
        p4_content_raster_upload_error(source,(unsigned)error);
        if (s->texture) p4gl_glDeleteTextures(1,&s->texture);
        s->texture=0;
        if (!fresh_owned) p4pc_heap_free(pixels);
        goto failed;
    }
    s->pixels=pixels;
    s->mipK=0; s->mipL=0;
    s->content->authored_stride=(int32_t)stride;
    s->content->authored_rows=ph;
    r->stride=(int32_t)stride;
    r->originalStride=r->stride;
    r->originalWidth=width; r->originalHeight=height;
    r->cFlags&=(uint8_t)~P4_RASTER_DONTALLOCATE;
    if (p4_content_sources_enabled) p4_content_source_mark(pixels,bytes,&s->content->source);
    return 0;
failed:
    p4_content_raster_free(s->content); s->content=NULL;
    s->content_origin=P4_TEXTURE_UNKNOWN;
    return -1;
}

int p4gl_raster_set_authored_rgba(P4RwRaster *r, const uint8_t *rgba,
                                 int32_t width, int32_t height,
                                 const P4ContentSource *source)
{
    return setAuthoredRgba(r,rgba,width,height,source,0);
}

extern P4RwRaster *RwRasterCreate(int32_t width, int32_t height, int32_t depth, uint32_t flags);
extern int32_t RwRasterDestroy(void *raster);

P4RwRaster *p4gl_raster_create_authored(const uint8_t *rgba, int32_t width, int32_t height,
                                        const P4ContentSource *source)
{
    if (!rgba || width<=0 || height<=0 || !p4_content_source_authored(NULL,source)) return NULL;
    /* Own the exact raster returned here: no ambient marker can affect nested
     * or concurrent retail creation, and no existing raster can be retagged. */
    P4RwRaster *r=RwRasterCreate(width,height,32,0x0584u);
    if (!r) return NULL;
    if (setAuthoredRgba(r,rgba,width,height,source,1)!=0) {
        RwRasterDestroy(r);
        return NULL;
    }
    return r;
}

const uint8_t *p4gl_raster_authored_shadow(const P4RwRaster *r,
                                         int32_t *stride, int32_t *padded_rows)
{
    RasterSlot *s;
    if (stride) *stride=0;
    if (padded_rows) *padded_rows=0;
    if (!r || !p4_guest_asset_buffer_contains(r,sizeof *r)) return NULL;
    s=slotFind(r);
    if (!s || !s->pixels || s->borrowed || s->frame || !s->content ||
        s->content->authored_stride<=0 || s->content->authored_rows<=0) return NULL;
    if (stride) *stride=s->content->authored_stride;
    if (padded_rows) *padded_rows=s->content->authored_rows;
    return s->pixels;
}

/* The texture of a raster without CPU pixels (the movie frame), made as p4gl_raster_set_rgba makes it, for a render pass that writes the
 * image (p4gl_yuv.c). 0 for an unknown raster. */
uint32_t p4gl_raster_image_texture(P4RwRaster *r)
{
    RasterSlot *s = r != NULL ? slotFind(r) : NULL;

    if (s == NULL || r->width <= 0 || r->height <= 0) {
        return 0;
    }
    p4gl_raster_content_exclude(r,P4_TEXTURE_MOVIE);
    if (s->texture == 0) {
        if (p4_battle_sim_renderless) {
            uploadRgba8(s, r->width, r->height, NULL);
            return s->texture;
        }
        uint8_t *blank = calloc((size_t)r->width * (size_t)r->height, 4u);

        if (blank == NULL) {
            return 0;
        }
        uploadRgba8(s, r->width, r->height, blank);
        free(blank);
    }
    return s->texture;
}

/* RASTERCREATE (stdFunc 4). The caller has set width, height, depth and cFlags;
 * nI carries the rwRASTER type and format bits. */
int32_t func_003f98a0(void *pOut, void *pInOut, int32_t flags)
{
    P4RwRaster *r = pInOut;
    (void)pOut;

    if (r == NULL || r->width <= 0 || r->height <= 0) {
        return 0;
    }
    r->cType = (uint8_t)((uint32_t)flags & P4_RASTER_TYPE_MASK);
    r->cFormat = (uint8_t)(((uint32_t)flags >> 8) & 0xFFu);
    if ((r->cFormat & P4_FMT_BASE_MASK) == 0) {
        r->cFormat = P4_FMT_8888; /* DEFAULT: the device picks 8888 */
    }

    const uint32_t base = formatBase(r);
    const int32_t stride = strideFor(base, r->width);
    if (stride == 0) {
        fprintf(stderr, "p4gl: RASTERCREATE: unsupported format 0x%04x\n", (unsigned)base);
        return 0;
    }
    r->stride = stride;
    r->cpPixels = 0;

    if (base == 0x2000u || base == 0x4000u) {
        uint32_t entries = base == 0x2000u ? 256u : 16u;
        if (r->palette == 0) {
            uint8_t *pal = p4pc_heap_calloc(entries, 4u);

            if (pal == NULL) {
                return 0;
            }
            r->palette = P4_H2G(pal);
        }
    }

    RasterSlot *s = slotClaim(r);
    if (s == NULL) {
        fprintf(stderr, "p4gl: raster table full (%u slots)\n", (unsigned)P4GL_RASTER_SLOTS);
        return 0;
    }

    uint32_t type = r->cType & P4_RASTER_TYPE_MASK;
    if (type == P4_RASTER_TYPE_CAMERA || type == P4_RASTER_TYPE_ZBUFFER ||
        (r->cFlags & P4_RASTER_DONTALLOCATE)) {
        return 1; /* no CPU shadow: the default framebuffer or a deferred allocation */
    }
    s->pixels = p4pc_heap_calloc((size_t)stride, (size_t)r->height);
    if (s->pixels == NULL) {
        s->state = P4_SLOT_DEAD;
        s->key = NULL;
        return 0;
    }
    return 1;
}

static void targetRelease(const P4RwRaster *r);

/* RASTERDESTROY (stdFunc 5). */
int32_t func_003fb830(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *r = pInOut;
    (void)pOut;
    (void)nI;

    if (r == NULL) {
        return 0;
    }
    targetRelease(r);
    RasterSlot *s = slotFind(r);
    if (s != NULL) {
        p4_content_raster_free(s->content); s->content=NULL;
        s->content_origin=P4_TEXTURE_UNKNOWN;
        if (s->texture != 0) {
            p4gl_glDeleteTextures(1, &s->texture);
        }
        if (!s->borrowed) {
            p4pc_heap_free(s->pixels);
        }
        s->borrowed = 0;
        s->state = P4_SLOT_DEAD;
        s->key = NULL;
        s->pixels = NULL;
        s->texture = 0;
    }
    p4pc_heap_free(P4_G2H(r->palette));
    r->palette = 0;
    r->cpPixels = 0;
    return 1;
}

/* RASTERLOCK (stdFunc 15): nI = lockMode + (level << 8). Writes the pixel pointer to pOut. */
int32_t func_003fc5f0(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *r = pInOut;
    uint32_t level = (uint32_t)nI >> 8;

    if (r == NULL || pOut == NULL || level != 0) {
        return 0; /* only level 0 exists */
    }
    RasterSlot *s = slotFind(r);
    if (s == NULL || s->pixels == NULL) {
        return 0;
    }
    r->cpPixels = P4_H2G(s->pixels);
    *(uint32_t *)pOut = P4_H2G(s->pixels);
    return 1;
}

/* RASTERUNLOCK (stdFunc 16): converts and uploads the shadow, then drops the pixel pointer. */
int32_t func_003fd230(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *r = pInOut;
    (void)pOut;
    (void)nI;

    if (r == NULL) {
        return 0;
    }
    RasterSlot *s = slotFind(r);
    r->cpPixels = 0;
    if (s == NULL || s->pixels == NULL || !isUploadableType(r)) {
        return 1;
    }
    return uploadRaster(r, s) == 0 ? 1 : 0;
}

/* RASTERLOCKPALETTE (stdFunc 23): pOut receives the palette pointer. */
int32_t func_003fd460(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *r = pInOut;
    (void)nI;

    if (r == NULL || pOut == NULL || r->palette == 0) {
        return 0;
    }
    *(uint32_t *)pOut = r->palette;
    return 1;
}

/* RASTERUNLOCKPALETTE (stdFunc 24): the palette changed, so re-upload the texture. */
int32_t func_003fd520(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *r = pInOut;
    (void)pOut;
    (void)nI;

    if (r == NULL) {
        return 0;
    }
    RasterSlot *s = slotFind(r);
    if (s == NULL || s->pixels == NULL || !isUploadableType(r)) {
        return 1;
    }
    return uploadRaster(r, s) == 0 ? 1 : 0;
}

/* TEXTURESETRASTER (stdFunc 8): pOut is the texture, pInOut the raster. The GL texture is
 * owned by the raster record, so binding only needs the record to exist. */
int32_t func_003f7a60(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *r = pInOut;
    (void)pOut;
    (void)nI;

    if (r == NULL) {
        return 0;
    }
    return slotFind(r) != NULL ? 1 : 0;
}

/* Render-to-texture. A camera whose frame buffer is a camera-texture raster (the field's shadow camera) renders into
 * that raster: begin binds a framebuffer (colour texture and depth renderbuffer of the raster's size) and returns
 * the size; end copies it, rows flipped (GL's first row is the bottom, the game's texture coordinates start at
 * the top), into the texture the raster samples as. */
/* The render targets, one per camera-texture raster that a camera renders into (the field has the shadow camera's). */
typedef struct RenderTarget {
    const P4RwRaster *key;
    GLuint fbo, color, depth, outFbo;
    int32_t width, height; /* physical storage, not the guest raster dimensions */
} RenderTarget;

#define P4_RENDER_TARGETS 8
static RenderTarget sTargets[P4_RENDER_TARGETS];

static RenderTarget *targetFind(const P4RwRaster *r, int create)
{
    int i;

    for (i = 0; i < P4_RENDER_TARGETS; i++) {
        if (sTargets[i].key == r) {
            return &sTargets[i];
        }
    }
    for (i = 0; create && i < P4_RENDER_TARGETS; i++) {
        if (sTargets[i].key == NULL) {
            sTargets[i].key = r;
            return &sTargets[i];
        }
    }
    return NULL;
}

/* A destroyed camera-texture raster gives its render target back: the table holds eight, and every shadow camera and effect target the game creates
 * and destroys over a long play (a field, a battle, a TV world) took a slot for good, so the ninth raster had no target, nothing rendered into it and its
 * texture stayed empty. The shadow of the 04/15 evening living room then drew as an opaque white quad over the floor. */
static void targetRelease(const P4RwRaster *r)
{
    RenderTarget *t = targetFind(r, 0);

    if (t == NULL) {
        return;
    }
    if (t->outFbo != 0) {
        p4gl_glDeleteFramebuffers(1, &t->outFbo);
    }
    if (t->fbo != 0) {
        p4gl_glDeleteFramebuffers(1, &t->fbo);
    }
    if (t->depth != 0) {
        p4gl_glDeleteRenderbuffers(1, &t->depth);
    }
    if (t->color != 0) {
        p4gl_glDeleteTextures(1, &t->color);
    }
    memset(t, 0, sizeof *t);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_readback_framebuffer());
}

static void restoreFramebuffers(GLint read, GLint draw)
{
    if (read == draw) {
        p4gl_glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)draw);
    } else {
        p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)read);
        p4gl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)draw);
    }
}

/* The render target of a camera-texture raster, made on first use (the framebuffer and the texture it ends up in). NULL when it cannot be made. */
static RenderTarget *targetPrepare(P4RwRaster *r)
{
    RasterSlot *s;
    RenderTarget *t;
    uint32_t mainW = 0, mainH = 0, w, h;
    uint32_t baseW = 0, baseH = 0;
    uint64_t wideW, wideH;
    GLint textureLimit = 0, renderbufferLimit = 0, read = 0, draw = 0;
    GLenum colorStatus, outputStatus, error;

    if (r == NULL || r->width <= 0 || r->height <= 0) return NULL;
    s = slotClaim(r);
    t = targetFind(r, 1);
    if (s == NULL || t == NULL) return NULL;
    p4gl_readback_virtual_size(&baseW, &baseH);
    if (baseW == 0 || baseH == 0) { baseW = 640; baseH = 448; }
    if (baseW == 640 && baseH == 448) {
        baseW = p4gl_aspect_native_width();
    }
    p4gl_readback_pixel_size(&mainW, &mainH);
    if (mainW == 0 || mainH == 0) { mainW = baseW; mainH = baseH; }
    wideW = ((uint64_t)(uint32_t)r->width * mainW + baseW / 2) / baseW;
    wideH = ((uint64_t)(uint32_t)r->height * mainH + baseH / 2) / baseH;
    if (wideW == 0) wideW = 1;
    if (wideH == 0) wideH = 1;
    if (t->fbo != 0 && t->outFbo != 0 && s->texture != 0 &&
        (uint64_t)t->width == wideW && (uint64_t)t->height == wideH &&
        (uint64_t)s->texW == wideW && (uint64_t)s->texH == wideH) return t;

    /* Check before narrowing or changing retained storage. A larger main
     * resolution can put a camera texture beyond the device's limits. */
    p4gl_glGetIntegerv(0x0D33u /* GL_MAX_TEXTURE_SIZE */, &textureLimit);
    p4gl_glGetIntegerv(0x84E8u /* GL_MAX_RENDERBUFFER_SIZE */, &renderbufferLimit);
    if (wideW > (uint64_t)textureLimit || wideH > (uint64_t)textureLimit ||
        wideW > (uint64_t)renderbufferLimit || wideH > (uint64_t)renderbufferLimit) {
        fprintf(stderr, "p4gl_raster: cannot allocate %llux%llu camera target for virtual %dx%d (texture limit %d, renderbuffer limit %d)\n",
                (unsigned long long)wideW, (unsigned long long)wideH, r->width, r->height,
                textureLimit, renderbufferLimit);
        return NULL;
    }
    w = (uint32_t)wideW; h = (uint32_t)wideH;
    p4gl_glGetIntegerv(0x8CAAu /* GL_READ_FRAMEBUFFER_BINDING */, &read);
    p4gl_glGetIntegerv(0x8CA6u /* GL_DRAW_FRAMEBUFFER_BINDING */, &draw);
    if (t->fbo == 0) {
        p4gl_glGenTextures(1, &t->color);
        p4gl_glGenRenderbuffers(1, &t->depth);
        p4gl_glGenFramebuffers(1, &t->fbo);
    }
    p4gl_glBindTexture(GL_TEXTURE_2D, t->color);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    p4gl_glBindRenderbuffer(GL_RENDERBUFFER, t->depth);
    p4gl_glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, (GLsizei)w, (GLsizei)h);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, t->color, 0);
    p4gl_glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, t->depth);
    colorStatus = p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (s->texture == 0) p4gl_glGenTextures(1, &s->texture);
    p4gl_glBindTexture(GL_TEXTURE_2D, s->texture);
    p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    if (t->outFbo == 0) p4gl_glGenFramebuffers(1, &t->outFbo);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, t->outFbo);
    p4gl_glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s->texture, 0);
    outputStatus = p4gl_glCheckFramebufferStatus(GL_FRAMEBUFFER);
    error = p4gl_glGetError();
    if (colorStatus != GL_FRAMEBUFFER_COMPLETE || outputStatus != GL_FRAMEBUFFER_COMPLETE || error != GL_NO_ERROR) {
        fprintf(stderr, "p4gl_raster: %ux%u camera target setup failed (colour FBO 0x%x, output FBO 0x%x, GL error 0x%x)\n",
                w, h, (unsigned)colorStatus, (unsigned)outputStatus, (unsigned)error);
        restoreFramebuffers(read, draw);
        return NULL;
    }
    t->width = s->texW = (int32_t)w;
    t->height = s->texH = (int32_t)h;
    return t;
}

int p4gl_raster_target_begin(P4RwRaster *r, int *w, int *h)
{
    RenderTarget *t = targetPrepare(r);

    if (t == NULL) {
        return 0;
    }
    p4gl_raster_content_exclude(r,P4_TEXTURE_FRAMEBUFFER);
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    p4gl_glViewport(0, 0, t->width, t->height);
    p4gl_target_override(r->width, r->height, t->width, t->height);
    *w = r->width;
    *h = r->height;
    return 1;
}

/* RwCameraClear of a camera-texture camera (the field's shadow camera: the game clears it with mode 3 before the update that draws into it): clears the raster's
 * own render target, not the framebuffer that happens to be bound. Without this the target keeps what it held, which a driver that hands out zeroed texture memory
 * (Mesa) hides until the shadow moves and one that recycles it (an Adreno 650) shows as the last texture that lived there. */
void p4gl_raster_target_clear(P4RwRaster *r, uint32_t bits, const float rgba[4])
{
    RenderTarget *t;
    GLint read = 0, draw = 0;
    p4gl_glGetIntegerv(0x8CAAu /* GL_READ_FRAMEBUFFER_BINDING */, &read);
    p4gl_glGetIntegerv(0x8CA6u /* GL_DRAW_FRAMEBUFFER_BINDING */, &draw);
    t = targetPrepare(r);
    if (t == NULL) return;
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, t->fbo);
    if ((bits & GL_COLOR_BUFFER_BIT) != 0u) {
        p4gl_glClearColor(rgba[0], rgba[1], rgba[2], rgba[3]);
    }
    p4gl_glClear(bits);
    restoreFramebuffers(read, draw);
}

void p4gl_raster_target_end(P4RwRaster *r)
{
    RenderTarget *t = targetFind(r, 0);

    if (t != NULL && t->fbo != 0 && t->outFbo != 0) {
        p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, t->fbo);
        p4gl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, t->outFbo);
        p4gl_glBlitFramebuffer(0, 0, t->width, t->height, 0, t->height, t->width, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    p4gl_glBindFramebuffer(GL_FRAMEBUFFER, p4gl_readback_framebuffer());
}

static void frameCopy(RasterSlot *s);

/* GL name for the raster, 0 when it has no upload yet. The Im2D draw path binds it. */
uint32_t p4gl_raster_texture(const P4RwRaster *raster)
{
    RasterSlot *s = raster != NULL ? slotFind(raster) : NULL;

    if (s != NULL && s->frame == 1) {
        frameCopy(s); /* the frame so far */
    }
    return s != NULL ? s->texture : 0;
}

int p4gl_raster_is_frame(const P4RwRaster *raster)
{
    const RasterSlot *s = raster != NULL ? slotFind(raster) : NULL;

    return s != NULL && s->frame != 0;
}

/* Frame rasters. The retail sky driver keeps two rasters whose GS blocks swap on a flip; effects
 * read the display raster (the last presented frame) or the draw raster (the frame so far) as a
 * texture. Here both are RwRasters with no pixels whose texture is refreshed from the framebuffer:
 * the draw raster whenever it is bound, the display raster at present. The copy is a blit with the
 * rows flipped, because GL's framebuffer starts at the bottom and the game's texture
 * coordinates start at the top. Derived from sky2hook's frame-buffer feedback handling
 * (skyDrawRaster/skyDisplayRaster in its address list), tge-was-taken, reused with permission; see
 * third_party/README.md. */
/* The retail globals, referenced by address (nothing in a compiled unit names them): */
#define P4_SKY_DISPLAY_RASTER (*(volatile int32_t *)P4_G2H(0x007649F0u)) /* skyDisplayRaster */
#define P4_SKY_DRAW_RASTER (*(volatile int32_t *)P4_G2H(0x007649F4u))    /* skyDrawRaster */

#define P4_FRAME_W 640
#define P4_FRAME_H 448

static P4RwRaster *sFrameRaster[2];
static GLuint sCopyFbo;

static void frameCopy(RasterSlot *s)
{
    int w;
    int h;
    GLint readFramebuffer = 0, drawFramebuffer = 0;

    if (!p4gl_gl_draw_enabled() || !p4gl_target_pixel_size(&w, &h)) {
        return; /* P4PC_TURBO=2 dropped this frame's draws: the blit would copy the pixels of the last drawn frame again */
    }
    if (p4_content_sources_enabled)
        p4gl_raster_content_exclude(s->key,P4_TEXTURE_FRAMEBUFFER);
    p4gl_glGetIntegerv(0x8CAAu /* GL_READ_FRAMEBUFFER_BINDING */, &readFramebuffer);
    p4gl_glGetIntegerv(0x8CA6u /* GL_DRAW_FRAMEBUFFER_BINDING */, &drawFramebuffer);
    if (s->texture == 0) {
        p4gl_glGenTextures(1, &s->texture);
        p4gl_glBindTexture(GL_TEXTURE_2D, s->texture);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        p4gl_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    if (s->texW != w || s->texH != h) {
        p4gl_glBindTexture(GL_TEXTURE_2D, s->texture);
        p4gl_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        s->texW = w;
        s->texH = h;
    }
    if (sCopyFbo == 0) {
        p4gl_glGenFramebuffers(1, &sCopyFbo);
    }
    p4gl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, sCopyFbo);
    p4gl_glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                s->texture, 0);
    p4gl_glBlitFramebuffer(0, 0, w, h, 0, h, w, 0, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    p4gl_glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)readFramebuffer);
    p4gl_glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)drawFramebuffer);
}

void p4gl_frame_rasters_open(void)
{
    int k;

    for (k = 0; k < 2; k++) {
        P4RwRaster *r;
        RasterSlot *s;

        if (sFrameRaster[k] != NULL) {
            continue;
        }
        r = (P4RwRaster *)P4_G2H(p4_gport_alloc(sizeof *r, 16)); /* the game holds this raster: guest memory */
        if (r == NULL) {
            return;
        }
        r->parent = P4_H2G(r);
        r->width = r->originalWidth = P4_FRAME_W;
        r->height = r->originalHeight = P4_FRAME_H;
        r->depth = 32;
        r->stride = r->originalStride = P4_FRAME_W * 4;
        r->cType = P4_RASTER_TYPE_CAMERA;
        r->cFormat = P4_FMT_8888;
        s = slotClaim(r);
        if (s == NULL) {
            return; /* the guest storage is never freed */
        }
        s->frame = (uint8_t)(k + 1);
        sFrameRaster[k] = r;
    }
    P4_SKY_DRAW_RASTER = P4_H2G(sFrameRaster[0]);
    P4_SKY_DISPLAY_RASTER = P4_H2G(sFrameRaster[1]);
}

/* RpSkyGetDisplayBufferRaster (retail 0x00401b80, basky.c excluded): the presented-frame raster. */
int32_t func_00401b80(void)
{
    return P4_SKY_DISPLAY_RASTER;
}

void p4gl_frame_rasters_present(void)
{
    RasterSlot *s = sFrameRaster[1] != NULL ? slotFind(sFrameRaster[1]) : NULL;

    if (s != NULL) {
        frameCopy(s);
    }
}

/* RASTERGETMIPLEVELS (stdFunc 28): writes the level count through pOut. The device keeps
 * only level 0, so the count is 1. */
int32_t func_003fd630(void *pOut, void *pInOut, int32_t nI)
{
    (void)pInOut;
    (void)nI;

    if (pOut == NULL) {
        return 0;
    }
    *(int32_t *)pOut = 1;
    return 1;
}

/* Bytes from a row start to pixel x in a format base, or -1 when the base has no byte layout
 * (palettes are rejected by SUBRASTER before this is reached). */
static int64_t rowBytesTo(uint32_t base, int32_t x)
{
    switch (base) {
    case 0x0100u: /* 1555 */
    case 0x0200u: /* 565 */
    case 0x0300u: /* 4444 */
        return (int64_t)x * 2;
    case 0x0400u: /* LUM8 */
        return x;
    case 0x0500u: /* 8888 */
        return (int64_t)x * 4;
    case 0x0600u: /* 888 */
        return (int64_t)x * 3;
    default:
        return -1;
    }
}

/* SUBRASTER (stdFunc 12): pOut is the child, pInOut the parent. The child is a rectangle of
 * the parent's shadow: its pixels point into the parent's rows at (nOffsetX, nOffsetY) and use
 * the parent's stride, so an unlock uploads exactly the parent's pixels in that rectangle.
 * Offsets are clamped as retail does (decomp/src/rw/basky.c, 003fb3f0). The GS page rounding
 * and TEX0/TEX1 register words are not reproduced: there is no VRAM. The child borrows the
 * parent's shadow, so destroy the child before the parent. A camera parent has no shadow, so
 * its child keeps a shadow of its own. Returns 1 on success, 0 for a wrong type or a rectangle
 * that does not fit. */
int32_t func_003fb3f0(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *child = pOut;
    P4RwRaster *parent = pInOut;
    (void)nI;

    if (child == NULL || parent == NULL) {
        return 0;
    }
    if ((parent->cType & P4_RASTER_TYPE_MASK) != P4_RASTER_TYPE_CAMERA ||
        (child->cType & P4_RASTER_TYPE_MASK) != P4_RASTER_TYPE_CAMERATEXTURE) {
        return 0;
    }
    const uint32_t base = formatBase(parent);
    if (base == 0x2000u || base == 0x4000u) {
        return 0; /* paletted: the child would need its own palette copy, not supported */
    }

    /* Retail clamp: an overhang smaller than the offset moves the rectangle back by the
     * overhang. */
    int32_t x = child->nOffsetX;
    int32_t y = child->nOffsetY;
    const int32_t excessX = x + child->width - parent->width;
    if (excessX > 0 && excessX < x) {
        x -= excessX;
    }
    const int32_t excessY = y + child->height - parent->height;
    if (excessY > 0 && excessY < y) {
        y -= excessY;
    }
    if (x < 0 || y < 0 || x + child->width > parent->width || y + child->height > parent->height) {
        return 0;
    }
    const int64_t xBytes = rowBytesTo(base, x);
    if (xBytes < 0) {
        return 0;
    }

    RasterSlot *ps = slotFind(parent);
    RasterSlot *cs = slotFind(child);
    if (cs == NULL) {
        cs = slotClaim(child);
    }
    if (cs == NULL) {
        return 0;
    }
    p4gl_raster_content_exclude(parent,P4_TEXTURE_FRAMEBUFFER);
    p4gl_raster_content_exclude(child,P4_TEXTURE_FRAMEBUFFER);

    child->parent = P4_H2G(parent);
    child->nOffsetX = (int16_t)x;
    child->nOffsetY = (int16_t)y;
    child->cFormat = parent->cFormat;
    child->depth = parent->depth;
    child->stride = parent->stride;

    if (ps != NULL && ps->pixels != NULL) {
        if (cs->pixels != NULL && !cs->borrowed) {
            p4pc_heap_free(cs->pixels);
        }
        cs->pixels = ps->pixels + (size_t)y * (size_t)parent->stride + (size_t)xBytes;
        cs->borrowed = 1;
    } else if (cs->pixels == NULL || cs->borrowed) {
        cs->pixels = p4pc_heap_calloc((size_t)child->stride, (size_t)child->height);
        cs->borrowed = 0;
        if (cs->pixels == NULL) {
            cs->state = P4_SLOT_DEAD;
            cs->key = NULL;
            return 0;
        }
    }
    child->cpPixels = P4_H2G(cs->pixels);
    return 1;
}

/* RpSkyTextureSetMipmapK (retail 0x003f6800): the texture's raster (texture +0) gets TEX1.K = trunc(16 * k) clamped to
 * -2048..2047 in the sky raster record's halfword at +0x14 (low 12 bits, the top four kept). The port keeps no GS
 * texture state: the value is stored in the raster's slot, where the sampler can read it. Returns the texture, or 0 for
 * a texture without a raster. */
void *RpSkyTextureSetMipmapK(void *texture, float k)
{
    P4RwRaster *raster;
    uint32_t rasterAddr;
    RasterSlot *slot;
    int32_t v;

    if (texture == NULL) {
        return NULL;
    }
    memcpy(&rasterAddr, texture, sizeof rasterAddr);
    raster = (P4RwRaster *)P4_G2H(rasterAddr);
    if (raster == NULL) {
        return NULL;
    }
    v = (int32_t)(16.0f * k);
    v = v < -2048 ? -2048 : v > 2047 ? 2047 : v;
    slot = slotFind(raster);
    if (slot != NULL) {
        slot->mipK = (int16_t)v;
    }
    return texture;
}

/* Retail 0x003f68a0: TEX1.L (bits 12-13 of the same halfword) = min(l, 3). */
void *func_003f68a0(void *texture, uint32_t l)
{
    P4RwRaster *raster;
    uint32_t rasterAddr;
    RasterSlot *slot;

    if (texture == NULL) {
        return NULL;
    }
    memcpy(&rasterAddr, texture, sizeof rasterAddr);
    raster = (P4RwRaster *)P4_G2H(rasterAddr);
    if (raster == NULL) {
        return NULL;
    }
    slot = slotFind(raster);
    if (slot != NULL) {
        slot->mipL = (uint8_t)(l < 4u ? l : 3u);
    }
    return texture;
}

void p4gl_raster_context_lost(void)
{
    for (unsigned i = 0; i < P4GL_RASTER_SLOTS; i++) {
        RasterSlot *s = &sSlots[i];
        const int had = s->texture != 0;

        if (s->state != P4_SLOT_USED) {
            continue;
        }
        s->texture = 0;
        s->texW = s->texH = 0;
        /* a raster with a CPU shadow is the same upload as at unlock; frame copies, render targets and the movie raster are refilled by their next use */
        if (had && s->frame == 0 && s->pixels != NULL && isUploadableType(s->key)) {
            uploadRaster(s->key, s);
        }
    }
    for (int i = 0; i < P4_RENDER_TARGETS; i++) {
        sTargets[i].fbo = sTargets[i].color = sTargets[i].depth = sTargets[i].outFbo = 0;
    }
    sCopyFbo = 0;
}
