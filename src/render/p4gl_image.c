/* RenderWare image <-> raster standard functions of the sky2 driver (decomp/src/rw/skyconv.c, stdFunc slots
 * 2, 3, 6, 7, 9 and 11), and the two RpSky clipper selectors (basky.c). The retail code converts between
 * RwImage pixels and the GS-format pixel memory of a raster; the port's rasters hold a linear shadow
 * (src/render/p4gl_raster.c), so the conversions are between RwImage rows and that shadow, reached through
 * RwRasterLock. Alpha follows the PS2 convention only in palettes: a raster palette entry holds 0..0x80
 * (the raster upload doubles it), an RwImage palette entry 0..0xFF; 8888 pixels hold 0..0xFF in the
 * shadow (the native texture reader expands them when it reads).
 *
 * Handler signature (RwStandardFunc): int32 handler(void *pOut, void *pInOut, int32 nI). */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4pc/gmem.h"
#include "p4pc/content_source.h"

/* RwImage (rwcore.h:2006), 32-bit layout: guest memory, so the pixel and palette pointers are guest addresses. */
typedef struct P4RwImage {
    int32_t flags;
    int32_t width, height, depth, stride;
    uint32_t cpPixels;
    uint32_t palette; /* RGBA bytes */
} P4RwImage;
_Static_assert(sizeof(P4RwImage) == 28, "RwImage layout");
_Static_assert(offsetof(P4RwImage, width) == 4, "RwImage.width offset");
_Static_assert(offsetof(P4RwImage, stride) == 16, "RwImage.stride offset");
_Static_assert(offsetof(P4RwImage, cpPixels) == 20, "RwImage.cpPixels offset");
_Static_assert(offsetof(P4RwImage, palette) == 24, "RwImage.palette offset");
/* Only the source service's owning render thread may use this copy. */
static P4ContentSource tmxOriginScratch;


void p4_content_tmx_image(void *image, const void *encoded)
{
    const P4RwImage *im=image;
    const uint8_t *bytes=encoded;
    P4ContentSource *origin=&tmxOriginScratch;
    uint32_t length;
    if (!p4_content_sources_enabled || !im || !bytes) return;
    p4_content_sources_assert_owner();
    length=(uint32_t)bytes[4]|(uint32_t)bytes[5]<<8|(uint32_t)bytes[6]<<16|(uint32_t)bytes[7]<<24;
    if (length<=0x40 || !im->cpPixels || im->height<=0 || im->stride<=0 ||
        (size_t)im->height>SIZE_MAX/(size_t)im->stride) return;
    /* Palette/texel payload establishes pixel provenance. The matched decoder
     * already consumed the header; it may have staged that metadata on a host
     * stack in the identity build, whose lifetime we deliberately do not retain. */
    if (!p4_content_source_find(bytes+0x40,length-0x40,origin)) return;
    origin->format=0;
    origin->palette_format=im->depth==4 || im->depth==8?
        (bytes[0x11]==2 || bytes[0x11]==10?P4_PAL_RGB555:P4_PAL_RGBA8888):P4_PAL_NONE;
    p4_content_source_mark(P4_G2H(im->cpPixels),(size_t)im->height*(size_t)im->stride,origin);
    if ((im->depth==4 || im->depth==8) && im->palette)
        p4_content_source_mark(P4_G2H(im->palette),(size_t)(1u<<im->depth)*4,origin);
}

/* The raster handlers of this driver (stdFunc 15, 16, 23, 24; src/render/p4gl_raster.c): the conversions go
 * to the device layer directly, as the retail ones go to the GS raster memory. */
extern int32_t func_003fc5f0(void *pOut, void *pInOut, int32_t nI);
extern int32_t func_003fd230(void *pOut, void *pInOut, int32_t nI);
extern int32_t func_003fd460(void *pOut, void *pInOut, int32_t nI);
extern int32_t func_003fd520(void *pOut, void *pInOut, int32_t nI);

static uint8_t *RwRasterLock(void *raster, uint8_t level, int32_t lockMode)
{
    uint32_t px = 0; /* the handler stores a guest address in the caller's slot */

    return func_003fc5f0(&px, raster, lockMode + ((int32_t)level << 8)) ? (uint8_t *)P4_G2H(px) : NULL;
}

static void RwRasterUnlock(void *raster)
{
    func_003fd230(NULL, raster, 0);
}

/* An 8-bit raster palette is the CLUT as the GS stores it, entries in CSM1 order (index bits 3 and 4 swapped, an involution), while an RwImage
 * palette is in logical order: the image/raster palette copies move entry i to and from slot csm1(i) (RenderWare's sky raster does the same). A 4-bit
 * palette has no swap. Without it a smooth 8-bit ramp (the dialogue portraits) reads every 8-entry run out of order and shows as horizontal bands. */
static uint32_t paletteSlot(uint32_t i, uint32_t entries)
{
    return entries == 256u ? ((i & ~0x18u) | ((i & 8u) << 1) | ((i & 16u) >> 1)) : i;
}

static uint8_t *RwRasterLockPalette(void *raster, int32_t lockMode)
{
    uint32_t pal = 0;

    return func_003fd460(&pal, raster, lockMode) ? (uint8_t *)P4_G2H(pal) : NULL;
}

static void RwRasterUnlockPalette(void *raster)
{
    func_003fd520(NULL, raster, 0);
}

/* Retail globals: the raster context (0x007649F0 + 0x80 = gp-18048), and the clipper selectors. */
#define iGpffffb980 P4_GPDATA(uint32_t, 0xb980)
#define iGpffffb994 P4_GPDATA(int32_t, 0xb994)
#define iGpffffac10 P4_GPDATA(int32_t, 0xac10)
#define D_0088CAC0 P4_GDATA(int32_t, 0x0088CAC0u)
#define D_0088CAC4 P4_GDATA(int32_t, 0x0088CAC4u)
#define D_0088CAC8 P4_GDATA(int32_t, 0x0088CAC8u)
#define D_0088CACC P4_GDATA(int32_t, 0x0088CACCu)

#define P4_LOCK_READ 0x02
#define P4_LOCK_WRITE 0x01

#define P4_RASTER_TYPE_MASK 0x07u
#define P4_RASTER_TYPE_ZBUFFER 1
#define P4_RASTER_TYPE_CAMERA 2
#define P4_RASTER_TYPE_NORMAL 0
#define P4_RASTER_TYPE_TEXTURE 4
#define P4_RASTER_TYPE_CAMERATEXTURE 5

#define P4_FMT_PAL8 0x20u
#define P4_FMT_PAL4 0x40u

static uint32_t rasterBase(const P4RwRaster *r)
{
    const uint32_t f = (uint32_t)r->cFormat & 0x7Fu;

    if (f & P4_FMT_PAL4) {
        return 0x4000u;
    }
    if (f & P4_FMT_PAL8) {
        return 0x2000u;
    }
    return f << 8;
}

/* Smallest n with 2^n >= x (retail 0x003f5040). */
static int32_t ceilLog2(int32_t x)
{
    int32_t n = 0;

    while (n < 30 && (1 << n) < x) {
        n++;
    }
    return n;
}

/* stdFunc 11 SETRASTERCONTEXT (retail 0x00407bd0): remembers the raster and copies its offsets and size
 * into the retail context words. Returns 1. */
int32_t func_00407bd0(void *pOut, void *pInOut, int32_t nI)
{
    const P4RwRaster *r = pInOut;

    (void)pOut;
    (void)nI;
    iGpffffb980 = P4_H2G(pInOut);
    D_0088CAC0 = r->nOffsetX;
    D_0088CAC4 = r->nOffsetY;
    D_0088CAC8 = r->width;
    D_0088CACC = r->height;
    return 1;
}

/* stdFunc 2 RGBTOPIXEL (retail 0x00407c10): pOut receives the pixel for the RwRGBA at pInOut in the raster
 * format nI (0x600 888, 0x500 8888, 0 or 0x100 1555); any other format returns 0. The 8888 alpha is
 * scaled to 0..0x80. */
int32_t func_00407c10(void *pOut, void *pInOut, int32_t nI)
{
    const uint8_t *c = pInOut;
    uint32_t *pixel = pOut;
    const uint32_t format = (uint32_t)nI & 0xF00u;

    if (format == 0x600u) {
        *pixel = c[0] | 0x80000000u | ((uint32_t)c[1] << 8) | ((uint32_t)c[2] << 16);
    } else if (format == 0x500u) {
        *pixel = c[0] | ((uint32_t)c[1] << 8) | ((uint32_t)c[2] << 16) | (((uint32_t)c[3] * 0x808081u) & 0xFF000000u);
    } else if (format == 0 || format == 0x100u) {
        *pixel = ((uint32_t)c[0] >> 3) | (((uint32_t)c[3] << 8) & 0x8000u) | (((uint32_t)c[2] << 7) & 0x7C00u) |
                 (((uint32_t)c[1] << 2) & 0x3E0u);
    } else {
        return 0;
    }
    return 1;
}

/* stdFunc 3 PIXELTORGB (retail 0x00407d10): the inverse. pOut is the RwRGBA, pInOut the pixel. */
int32_t func_00407d10(void *pOut, void *pInOut, int32_t nI)
{
    uint8_t *c = pOut;
    uint32_t p;
    const uint32_t format = (uint32_t)nI & 0xF00u;

    memcpy(&p, pInOut, sizeof p);
    if (format == 0x600u) {
        c[0] = (uint8_t)p;
        c[1] = (uint8_t)(p >> 8);
        c[2] = (uint8_t)(p >> 16);
        c[3] = 0xFF;
    } else if (format == 0x500u) {
        const uint32_t x = p & 0xFF000000u;
        const uint32_t hi = (uint32_t)(((uint64_t)0xFE0001FFu * x) >> 32);

        c[0] = (uint8_t)p;
        c[1] = (uint8_t)(p >> 8);
        c[2] = (uint8_t)(p >> 16);
        c[3] = (uint8_t)((((x - hi) >> 1) + hi) >> 23);
    } else if (format == 0 || format == 0x100u) {
        c[0] = (uint8_t)((p << 3) & 0xF8u);
        c[1] = (uint8_t)((p >> 2) & 0xF8u);
        c[2] = (uint8_t)((p >> 7) & 0xF8u);
        c[3] = (p & 0x8000u) ? 0xFF : 0;
    } else {
        return 0;
    }
    return 1;
}

/* stdFunc 9 IMAGEFINDRASTERFORMAT (retail 0x00407df0): pOut is a scratch RwRaster that receives the raster the
 * image should become for the raster type nI (type in the low 3 bits, format and mipmap flags above):
 * the image's size, depth left 0, and the format. Normal rasters keep the requested format (1555 when none);
 * z-buffers have none; camera rasters follow the video depth (the port runs 32 bit: 8888); textures are
 * 8888, PAL8 for 8-bit images and PAL4 for 4-bit ones (the retail choice between 8888 and 1555 follows the
 * GS memory budget, which the port does not have), carry the mipmap flag bits 0x9000, and their size rounds up
 * to a power of two, at most 1024. Returns 1, or 0 for a raster type that has no format. */
int32_t _rwSkyImageFindRasterFormat(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *r = pOut;
    const P4RwImage *image = pInOut;
    const uint32_t flags = (uint32_t)nI;

    r->width = image->width;
    r->height = image->height;
    r->depth = 0;
    r->cType = (uint8_t)(flags & P4_RASTER_TYPE_MASK);
    r->cFlags = (uint8_t)(flags & ~P4_RASTER_TYPE_MASK);

    switch (r->cType) {
    case P4_RASTER_TYPE_ZBUFFER:
        r->cFormat = 0;
        return 1;
    case P4_RASTER_TYPE_CAMERA:
    case P4_RASTER_TYPE_CAMERATEXTURE:
        r->cFormat = 5;
        return 1;
    case P4_RASTER_TYPE_NORMAL:
        r->cFormat = (uint8_t)((flags & 0xF00u) >> 8);
        if (r->cFormat == 0) {
            r->cFormat = 1;
        }
        return 1;
    case P4_RASTER_TYPE_TEXTURE: {
        uint32_t format = 0x500u;

        if (image->depth == 4) {
            format |= 0x4000u;
        } else if (image->depth == 8) {
            format |= 0x2000u;
        }
        r->cFormat = (uint8_t)((format >> 8) | ((flags & 0x9000u) >> 8));
        r->width = 1 << ceilLog2(image->width);
        if (r->width > 1024) {
            r->width = 1024;
        }
        r->height = 1 << ceilLog2(image->height);
        if (r->height > 1024) {
            r->height = 1024;
        }
        return 1;
    }
    default:
        return 0;
    }
}

/* One image pixel as RGBA bytes (alpha 0..0xFF), or 0 when the image cannot supply it. */
static int imageRgba(const P4RwImage *im, int32_t x, int32_t y, uint8_t out[4])
{
    const uint8_t *row = (const uint8_t *)P4_G2H(im->cpPixels) + (size_t)y * (size_t)im->stride;
    const uint8_t *ipal = (const uint8_t *)P4_G2H(im->palette);

    if (im->depth == 32) {
        memcpy(out, row + 4 * (size_t)x, 4);
        return 1;
    }
    if (ipal == NULL) {
        return 0;
    }
    if (im->depth == 8) {
        memcpy(out, ipal + 4u * row[x], 4);
        return 1;
    }
    if (im->depth == 4) {
        /* a 4-bit RwImage holds one index per byte (the stride of an 8-bit image, RwImageSetStride): tmxUnpackNibbles writes them so */
        memcpy(out, ipal + 4u * (row[x] & 15u), 4);
        return 1;
    }
    return 0;
}
/* Source ranges may move while raster locks and uploads run, so these copies
 * stay valid through the synchronous render-thread operation. */
static P4ContentSource imageOriginScratch;
static P4ContentSource imagePaletteOriginScratch;


static int imageOrigin(const P4RwImage *im, P4ContentSource *origin)
{
    if (im->height<=0 || im->stride<=0 || (size_t)im->height>SIZE_MAX/(size_t)im->stride ||
        !p4_content_source_find(P4_G2H(im->cpPixels),(size_t)im->height*(size_t)im->stride,origin))
        return 0;
    if (im->depth==4 || im->depth==8) {
        if (!im->palette || !p4_content_source_find(P4_G2H(im->palette),(size_t)(1u<<im->depth)*4,&imagePaletteOriginScratch) ||
            !p4_content_source_same(&imagePaletteOriginScratch,origin))
            return 0;
    }
    origin->format=0; /* identity format is the final decoded raster format */
    return 1;
}

/* stdFunc 7 RASTERSETIMAGE (retail 0x0040a500): pOut is the raster, pInOut the image (the same size: the
 * texture reader resamples first). A paletted raster takes the image's indices and palette; the other
 * formats take the image's colours converted to the raster's format. Returns 1, or 0 when the image does
 * not fit the raster or its pixels are missing. */
int32_t _rwSkyRasterSetImage(void *pOut, void *pInOut, int32_t nI)
{
    P4RwRaster *r = pOut;
    const P4RwImage *im = pInOut;
    uint8_t *px;
    const uint32_t base = r != NULL ? rasterBase(r) : 0;
    int32_t w;
    int32_t h;

    (void)nI;
    if (r == NULL || im == NULL || im->cpPixels == 0) {
        return 0;
    }
    P4ContentSource *origin=&imageOriginScratch;
    if (p4_content_sources_enabled) p4_content_sources_assert_owner();
    int hasOrigin=p4_content_sources_enabled && imageOrigin(im,origin);
    if (p4_content_sources_enabled) p4gl_raster_content_exclude(r,P4_TEXTURE_UNKNOWN);
    w = im->width < r->width ? im->width : r->width;
    h = im->height < r->height ? im->height : r->height;
    px = RwRasterLock(r, 0, P4_LOCK_WRITE);
    if (px == NULL) {
        return 0;
    }

    if (base == 0x2000u || base == 0x4000u) {
        const uint32_t entries = base == 0x2000u ? 256u : 16u;
        uint8_t *pal;

        if (im->palette == 0 || im->depth != (base == 0x2000u ? 8 : 4)) {
            RwRasterUnlock(r);
            return 0;
        }
        for (int32_t y = 0; y < h; y++) {
            const uint8_t *src = (const uint8_t *)P4_G2H(im->cpPixels) + (size_t)y * (size_t)im->stride;
            uint8_t *dst = px + (size_t)y * (size_t)r->stride;

            if (base == 0x2000u) {
                memcpy(dst, src, (size_t)w);
            } else {   /* the image has one index per byte, the PAL4 raster two per byte, the even pixel in the low nibble */
                for (int32_t x = 0; x < w; x += 2) {
                    dst[x / 2] = (uint8_t)((src[x] & 15u) | (x + 1 < w ? (src[x + 1] & 15u) << 4 : 0u));
                }
            }
        }
        RwRasterUnlock(r);
        pal = RwRasterLockPalette(r, P4_LOCK_WRITE);
        if (pal == NULL) {
            return 0;
        }
        for (uint32_t i = 0; i < entries; i++) {
            const uint8_t *e = (const uint8_t *)P4_G2H(im->palette) + 4u * i;

            uint8_t *d = pal + 4u * paletteSlot(i, entries);

            d[0] = e[0];
            d[1] = e[1];
            d[2] = e[2];
            d[3] = (uint8_t)((e[3] + 1u) >> 1);
        }
        if (hasOrigin) p4gl_raster_content_source(r,origin);
        RwRasterUnlockPalette(r);
        return 1;
    }

    for (int32_t y = 0; y < h; y++) {
        uint8_t *row = px + (size_t)y * (size_t)r->stride;

        for (int32_t x = 0; x < w; x++) {
            uint8_t c[4];

            if (!imageRgba(im, x, y, c)) {
                RwRasterUnlock(r);
                return 0;
            }
            switch (base) {
            case 0x0100u: { /* 1555 */
                const uint32_t p = ((uint32_t)c[2] >> 3) | (((uint32_t)c[1] >> 3) << 5) | (((uint32_t)c[0] >> 3) << 10) |
                                   (c[3] >= 128 ? 0x8000u : 0u);
                row[2 * x] = (uint8_t)p;
                row[2 * x + 1] = (uint8_t)(p >> 8);
                break;
            }
            case 0x0200u: { /* 565 */
                const uint32_t p = ((uint32_t)c[2] >> 3) | (((uint32_t)c[1] >> 2) << 5) | (((uint32_t)c[0] >> 3) << 11);
                row[2 * x] = (uint8_t)p;
                row[2 * x + 1] = (uint8_t)(p >> 8);
                break;
            }
            case 0x0300u: { /* 4444 */
                const uint32_t p = ((uint32_t)c[2] >> 4) | (((uint32_t)c[1] >> 4) << 4) | (((uint32_t)c[0] >> 4) << 8) |
                                   (((uint32_t)c[3] >> 4) << 12);
                row[2 * x] = (uint8_t)p;
                row[2 * x + 1] = (uint8_t)(p >> 8);
                break;
            }
            case 0x0400u: /* LUM8 */
                row[x] = (uint8_t)(((uint32_t)c[0] + c[1] + c[2]) / 3u);
                break;
            case 0x0600u: /* 888 */
                row[3 * x] = c[0];
                row[3 * x + 1] = c[1];
                row[3 * x + 2] = c[2];
                break;
            case 0x0500u: /* 8888 */
            default:
                memcpy(row + 4 * (size_t)x, c, 4);
                break;
            }
        }
    }
    if (hasOrigin) p4gl_raster_content_source(r,origin);
    RwRasterUnlock(r);
    return 1;
}

/* stdFunc 6 IMAGEGETRASTER (retail 0x00407ff0): pOut is the image, pInOut the raster of the same size; the
 * raster's pixels become the image's. A 32-bit image takes RGBA (alpha 0..0xFF); an 8 or 4 bit image takes the
 * raster's indices and palette when the raster has the same depth, with the palette alpha doubled back. Other
 * combinations return 0. */
int32_t func_00407ff0(void *pOut, void *pInOut, int32_t nI)
{
    P4RwImage *im = pOut;
    P4RwRaster *r = pInOut;
    const uint32_t base = r != NULL ? rasterBase(r) : 0;
    uint8_t *px;
    int32_t w;
    int32_t h;

    (void)nI;
    if (im == NULL || r == NULL || im->cpPixels == 0) {
        return 0;
    }
    w = im->width < r->width ? im->width : r->width;
    h = im->height < r->height ? im->height : r->height;
    px = RwRasterLock(r, 0, P4_LOCK_READ);
    if (px == NULL) {
        return 0;
    }
    if (base == 0x2000u || base == 0x4000u) {
        const uint32_t entries = base == 0x2000u ? 256u : 16u;
        uint8_t *pal;

        if (im->palette == 0 || im->depth != (base == 0x2000u ? 8 : 4)) {
            RwRasterUnlock(r);
            return 0;
        }
        for (int32_t y = 0; y < h; y++) {
            uint8_t *dst = (uint8_t *)P4_G2H(im->cpPixels) + (size_t)y * (size_t)im->stride;
            const uint8_t *src = px + (size_t)y * (size_t)r->stride;

            if (base == 0x2000u) {
                memcpy(dst, src, (size_t)w);
            } else {
                for (int32_t x = 0; x < w; x++) {
                    dst[x] = (x & 1) ? (uint8_t)(src[x / 2] >> 4) : (uint8_t)(src[x / 2] & 15u);
                }
            }
        }
        RwRasterUnlock(r);
        pal = RwRasterLockPalette(r, P4_LOCK_READ);
        if (pal == NULL) {
            return 0;
        }
        uint8_t *ipal = (uint8_t *)P4_G2H(im->palette);

        for (uint32_t i = 0; i < entries; i++) {
            const uint8_t *s = pal + 4u * paletteSlot(i, entries);

            ipal[4 * i] = s[0];
            ipal[4 * i + 1] = s[1];
            ipal[4 * i + 2] = s[2];
            ipal[4 * i + 3] = (uint8_t)(s[3] >= 128 ? 255 : s[3] * 2);
        }
        RwRasterUnlockPalette(r);
        return 1;
    }
    if (im->depth != 32 || base != 0x0500u) {
        RwRasterUnlock(r);
        return 0;
    }
    for (int32_t y = 0; y < h; y++) {
        memcpy((uint8_t *)P4_G2H(im->cpPixels) + (size_t)y * (size_t)im->stride, px + (size_t)y * (size_t)r->stride, 4u * (size_t)w);
    }
    RwRasterUnlock(r);
    return 1;
}

/* RpSkySelectTrueTSClipper / RpSkySelectTrueTLClipper (retail 0x003f67e0 / 0x003f67f0): record the choice in
 * the retail globals, return TRUE. The GL path clips in hardware, so nothing reads them. */
int32_t RpSkySelectTrueTSClipper(int32_t trueClipper)
{
    iGpffffb994 = trueClipper;
    return 1;
}

int32_t RpSkySelectTrueTLClipper(int32_t trueClipper)
{
    iGpffffac10 = trueClipper;
    return 1;
}
