/* RenderWare native texture handlers for the OpenGL device (stdFunc slots 25, 26, 27): the
 * size, read and write of a texture's "PS2" chunk. The chunk layout, the version window and
 * the error raises are the retail ones (decomp/src/rw/basky.c, 003fdd10, 003fdf20, 003fddc0,
 * and the disassembly they were checked against). The stream helpers are retail code and are
 * called as they are; the raster reader (003fd940, readTEXFromStream) is the port's, see the
 * block above func_003fd940; the raster writer (003fd690) is retail code and is not ported.
 * GS-only fields are not touched: see docs/renderer_port.md section 7.
 *
 * Handler signature (RwStandardFunc): int32 handler(void *pOut, void *pInOut, int32 nI).
 * GETSIZE: pOut is the int32 result, pInOut the texture.
 * READ: pOut is the stream, pInOut receives the new texture.
 * WRITE: pOut is the stream, pInOut is the texture. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4gl.h"
#include "p4gl_nativetex.h"
#include "p4pc/gmem.h"
#include "p4pc/content_source.h"

/* Mirror of struct RwTexture (decomp/include/rw/inc/rwcore.h:2400), 32-bit layout: guest memory, so the raster
 * and dictionary pointers are guest addresses. */
typedef struct P4RwTexture {
    uint32_t raster;           /* @0x00 */
    uint32_t dict;             /* @0x04 */
    uint32_t lInDictionary[2]; /* @0x08 */
    char name[32];             /* @0x10 */
    char mask[32];             /* @0x30 */
    uint32_t filterAddressing; /* @0x50 */
    uint32_t refCount;         /* @0x54 */
} P4RwTexture;

_Static_assert(sizeof(P4RwTexture) == 0x58, "RwTexture size");
_Static_assert(offsetof(P4RwTexture, dict) == 0x04, "RwTexture.dict offset");
_Static_assert(offsetof(P4RwTexture, lInDictionary) == 0x08, "RwTexture.lInDictionary offset");
_Static_assert(offsetof(P4RwTexture, name) == 0x10, "RwTexture.name offset");
_Static_assert(offsetof(P4RwTexture, mask) == 0x30, "RwTexture.mask offset");
_Static_assert(offsetof(P4RwTexture, filterAddressing) == 0x50, "RwTexture.filterAddressing offset");
_Static_assert(offsetof(P4RwTexture, refCount) == 0x54, "RwTexture.refCount offset");

/* The two RwRaster fields the native chunk size reads (struct RwRaster, rwcore.h:1365). */
typedef struct P4RwRasterDims {
    uint8_t pad[0x28];
    int32_t originalWidth;  /* @0x28 */
    int32_t originalHeight; /* @0x2C */
} P4RwRasterDims;

_Static_assert(offsetof(P4RwRasterDims, originalWidth) == 0x28, "RwRaster.originalWidth offset");
_Static_assert(offsetof(P4RwRasterDims, originalHeight) == 0x2C, "RwRaster.originalHeight offset");

/* Retail data: the base the raster pointer is offset by (basky.c uses it the same way). */
#define iGpffffb938 P4_GPDATA(int32_t, 0xb938)

/* Retail helpers. The argument lists are the ones the calls in the three handlers use. */
extern void *func_003deff0(void *stream, int32_t type, int32_t size, int32_t version, int32_t flags);   /* RwStream*: the stream, or NULL */
extern int32_t func_003df050(void *stream, int32_t type, int32_t *size, int32_t *version);
extern int32_t func_003e2910(void *stream, void *buf, int32_t size);
extern void *func_003e2ab0(void *stream, const void *buf, int32_t size);
extern void *func_003e4f60(void *dst, void *stream);
extern void *func_003e4b20(const void *str, void *stream);
extern int32_t func_003e4ad0(const char *str);
extern int32_t func_003fd690(void *stream, void *raster, int32_t size);
extern void *func_003ef2e0(void *raster);
extern void func_003ef470(void *texture, const void *name);
extern void func_003ef510(void *texture, const void *mask);
extern void func_003ec330(void *raster);
extern uint32_t func_003df590(uint32_t code);
extern void func_003df4d0(void *err);

/* Chunk version written by WRITE, and the window READ accepts: [0x5000, 0x37003). */
#define P4_NATIVE_VERSION_WRITE 0x37002u
#define P4_NATIVE_VERSION_MIN 0x5000u
#define P4_NATIVE_VERSION_END 0x37003u
/* Chunk type and the flag word both chunk headers carry. */
#define P4_NATIVE_CHUNK_TYPE 1
#define P4_NATIVE_CHUNK_FLAGS 55

static P4RwRasterDims *rasterDims(const P4RwTexture *tex)
{
    return (P4RwRasterDims *)P4_G2H(tex->raster + (uint32_t)iGpffffb938);
}

static int versionAccepted(int32_t version)
{
    return (uint32_t)version >= P4_NATIVE_VERSION_MIN && (uint32_t)version < P4_NATIVE_VERSION_END;
}

/* Raises the stream error the retail read path raises for a bad chunk (3fe170 / 3fe1a8). */
static void raiseStreamError(void)
{
    int32_t err[2];
    err[0] = 1;
    err[1] = (int32_t)func_003df590(0x80000004u);
    func_003df4d0(err);
}

/* Raster calls (named decomp C, baraster.c). */
extern void *RwRasterCreate(int32_t width, int32_t height, int32_t depth, uint32_t flags);
extern uint8_t *RwRasterLock(void *raster, uint8_t level, int32_t lockMode);
extern void *RwRasterUnlock(void *raster);
extern uint8_t *RwRasterLockPalette(void *raster, int32_t lockMode);
extern void *RwRasterUnlockPalette(void *raster);
#define P4_RASTERLOCKWRITE 0x01

static void *rasterFail(void *raster, int32_t errorCode)
{
    fprintf(stderr, "p4gl_native: native texture read failed (error %d)\n", (int)errorCode);
    if (raster != NULL) {
        func_003ec330(raster);
    }
    if (errorCode != 0) {
        raiseStreamError();
    }
    return NULL;
}


/* Retail 003fd940 (readTEXFromStream): the raster of a native texture. Two type-1 chunks follow in the
 * stream: a 64-byte header and the data. Header words: [0] width, [1] height, [2] depth, [3] raster
 * flags (low 16: the RwRasterCreate flags, for example 0x2504 = PAL8 | 8888 | texture) and the data
 * kind (high 16: 0 or 1 = raster pixels, 2 = the GS upload packets of a paletted texture, decoded by
 * p4gl_nativetex_decode); [4..15] the GS registers and sizes that the retail code copies into the sky
 * raster extension, which the port does not keep. Kind 0 and 1: the data chunk is the raster's pixel
 * bytes; 32-bit pixels carry the PS2 alpha (0x80 opaque), doubled here so the shadow holds what the GL
 * upload samples. */
void *func_003fd940(void *stream)
{
    int32_t size = 0;
    int32_t version = 0;
    uint32_t hdr[16];

    if (func_003df050(stream, P4_NATIVE_CHUNK_TYPE, &size, &version) == 0) {
        fprintf(stderr, "p4gl_native: no native texture chunk header\n");
        return NULL;
    }
    if ((uint32_t)version < 0x35000u || (uint32_t)version > 0x37002u) {
        return rasterFail(NULL, 1);
    }
    memset(hdr, 0, sizeof hdr);
    if (size < 16 || size > (int32_t)sizeof hdr || func_003e2910(stream, hdr, size) != size) {
        fprintf(stderr, "p4gl_native: native texture header of %d bytes\n", (int)size);
        return NULL;
    }
    const int32_t w = (int32_t)hdr[0];
    const int32_t h = (int32_t)hdr[1];
    const int32_t depth = (int32_t)hdr[2];
    const uint32_t flags = hdr[3] & 0xFFFFu;
    const uint32_t kind = hdr[3] >> 16;
    void *raster = RwRasterCreate(w, h, depth, flags);
    if (raster == NULL) {
        fprintf(stderr, "p4gl_native: RwRasterCreate(%d, %d, %d, %#x) failed\n", (int)w, (int)h, (int)depth, (unsigned)flags);
        return NULL;
    }

    if (func_003df050(stream, P4_NATIVE_CHUNK_TYPE, &size, &version) == 0) {
        return rasterFail(raster, 0);
    }
    if ((uint32_t)version < 0x35000u || (uint32_t)version > 0x37002u) {
        return rasterFail(raster, 1);
    }
    P4ContentReadScope inputScope;
    uint8_t *data = malloc((size_t)(size > 0 ? size : 1));
    if (p4_content_sources_enabled)
        p4_content_host_read_begin(&inputScope,data,(size_t)(size>0?size:1));
    if (data == NULL || func_003e2910(stream, data, size) != size) {
        if (p4_content_sources_enabled) p4_content_host_read_end(&inputScope);
        free(data);
        return rasterFail(raster, 0);
    }
    P4ContentSource origin;
    int hasOrigin=p4_content_sources_enabled && size>0 &&
        p4_content_source_find(data,(size_t)size,&origin);
    if (hasOrigin) {
        origin.format=0; /* canonical format is the decoded raster format */
        origin.palette_format=(flags&0x6000u)?P4_PAL_RGBA8888:P4_PAL_NONE;
    }

    uint8_t *pixels = RwRasterLock(raster, 0, P4_RASTERLOCKWRITE);
    if (pixels == NULL) {
        if (p4_content_sources_enabled) p4_content_host_read_end(&inputScope);
        free(data);
        return rasterFail(raster, 0);
    }
    const int32_t stride = ((const P4RwRaster *)raster)->stride;
    int ok = 1;

    if (kind < 2) {
        const int32_t room = stride * h;
        const int32_t n = size < room ? size : room;
        memcpy(pixels, data, (size_t)n);
        if ((flags & 0x0F00u) == 0x0500u && (flags & 0x6000u) == 0) {
            for (int32_t i = 3; i < n; i += 4) {
                pixels[i] = pixels[i] >= 0x80u ? 0xFFu : (uint8_t)(pixels[i] * 2u);
            }
        }
    } else if (depth == 32) {
        /* PSMCT32: no palette, the pixels in raster order; PS2 alpha (0x80 opaque) doubled as for kind 0 and 1. */
        const int32_t n = stride * h;

        ok = n >= w * h * 4 && p4gl_nativetex_decode_ct32(data, (uint32_t)size, w, h, pixels);
        if (ok && (flags & 0x0F00u) == 0x0500u) {
            for (int32_t i = 3; i < w * h * 4; i += 4) {
                pixels[i] = pixels[i] >= 0x80u ? 0xFFu : (uint8_t)(pixels[i] * 2u);
            }
        }
    } else {
        const uint32_t entries = depth == 8 ? 256u : 16u;
        uint8_t *idx = malloc((size_t)w * (size_t)h);
        uint8_t palette[256 * 4];

        ok = idx != NULL && p4gl_nativetex_decode(data, (uint32_t)size, w, h, depth, idx, palette);
        if (ok) {
            if (depth == 8) {
                memcpy(pixels, idx, (size_t)w * (size_t)h);
            } else {
                for (int32_t y = 0; y < h; y++) {
                    for (int32_t x = 0; x < w; x += 2) {
                        pixels[(size_t)y * (size_t)stride + (size_t)(x / 2)] =
                            (uint8_t)(idx[(size_t)y * (size_t)w + (size_t)x] |
                                      (x + 1 < w ? idx[(size_t)y * (size_t)w + (size_t)x + 1] << 4 : 0));
                    }
                }
            }
            uint8_t *pal = RwRasterLockPalette(raster, P4_RASTERLOCKWRITE);
            if (pal == NULL) {
                ok = 0;
            } else {
                /* A raster palette is in GS CLUT memory order (what the game's TMX loader stores too): the
                 * 8-bit CSM1 read swaps index bits 3 and 4, so entry j holds logical entry csm1(j). */
                for (uint32_t j = 0; j < entries; j++) {
                    const uint32_t src = depth == 8 ? ((j & ~0x18u) | ((j & 8u) << 1) | ((j & 16u) >> 1)) : j;

                    memcpy(pal + 4u * j, palette + 4u * src, 4u);
                }
                if (hasOrigin) p4gl_raster_content_source(raster,&origin);
                RwRasterUnlockPalette(raster);
            }
        }
        free(idx);
    }
    if (ok && hasOrigin && (kind<2 || depth==32))
        p4gl_raster_content_source(raster,&origin);
    RwRasterUnlock(raster);
    if (p4_content_sources_enabled) p4_content_host_read_end(&inputScope);
    free(data);
    if (!ok) {
        fprintf(stderr, "p4gl_native: decode of a %dx%d depth %d kind %u texture failed\n", (int)w, (int)h, (int)depth, (unsigned)kind);
    }
    return ok ? raster : rasterFail(raster, 0);
}

extern void *func_003df360(void *stream, void *buf, int32_t size);
extern void *func_003df240(void *stream, const void *buf, int32_t size);   /* RwStreamWrite: the stream, or 0 */

/* Retail 003f69f0 (skyGetSizeTexCB, decomp src/rw/basky.c): the size of the extension chunk of a texture, 4 bytes
 * when the texture has a raster (its first word) and none otherwise. The texture plugin's size callback. */
int32_t func_003f69f0(uint32_t *texture)
{
    return texture != NULL && *texture != 0 ? 4 : 0;
}

/* Retail 003f6910 (skyReadTexCB) and 003f6990 (skyWriteTexCB): the stream callbacks of the texture
 * plugin 0x110, registered by _rwDeviceRegisterPlugin (src/platform/rw_device_register.c). The
 * extension chunk is one word whose low half the retail code stores in the sky raster extension
 * (+0x14, the sky texture filter bits that skyGetSizeTexCB's 4 bytes size covers). The port's
 * rasters have no sky extension and nothing reads those bits, so the read consumes the word and
 * the write emits the retail default word 0x0FC0 (what the native chunk header carries). Both
 * return the stream, or NULL for a texture without a raster or a chunk that is not 4 bytes. */
void *func_003f6910(void *stream, int32_t length, void *object)
{
    uint32_t word = 0;

    if (length != 4 || object == NULL || *(uint32_t *)object == 0) {
        return NULL;
    }
    if (func_003df360(stream, &word, length) == 0) {
        return NULL;
    }
    return stream;
}

void *func_003f6990(void *stream, int32_t length, void *object)
{
    uint32_t word = 0x0FC0u;

    (void)length;
    if (object == NULL || *(uint32_t *)object == 0) {
        return NULL;
    }
    if (func_003df240(stream, &word, 4) == NULL) {
        return NULL;
    }
    return stream;
}

/* Native texture size (stdFunc 25): the byte count WRITE produces for the texture. */
int32_t func_003fdd10(void *pOut, void *pInOut, int32_t nI)
{
    int32_t *out = pOut;
    const P4RwTexture *tex = pInOut;
    (void)nI;

    *out = 0;
    *out += 0x14;
    *out += func_003e4ad0(tex->name) + 0xC;
    *out += func_003e4ad0(tex->mask) + 0xC;
    const P4RwRasterDims *temp = rasterDims(tex);
    *out += temp->originalWidth + temp->originalHeight + 0xC;
    *out += 0x4C;
    *out += 0xC;
    return 1;
}

/* Native texture read (stdFunc 26). Returns 1 with the texture in *result, or 0. */
int32_t func_003fdf20(void *pOut, void *pInOut, int32_t nI)
{
    void *stream = pOut;
    uint32_t *result = pInOut; /* guest slot: receives the texture's guest address */
    int32_t size = 0;
    int32_t version = 0;
    (void)nI;

    *result = 0;
    if (func_003df050(stream, P4_NATIVE_CHUNK_TYPE, &size, &version) == 0) {
        return 0;
    }
    if (!versionAccepted(version)) {
        raiseStreamError();
        return 0;
    }

    /* The 8-byte struct: "PS2\0", then the filter/address word. */
    uint8_t hdr[8];
    memset(hdr, 0, sizeof hdr);
    if (func_003e2910(stream, hdr, size) != size) {
        return 0;
    }
    if (hdr[0] != 'P' || hdr[1] != 'S' || hdr[2] != '2' || hdr[3] != 0) {
        return 0;
    }

    char name[32];
    char mask[32];
    if (func_003e4f60(name, stream) == 0) {
        return 0;
    }
    if (func_003e4f60(mask, stream) == 0) {
        return 0;
    }

    if (func_003df050(stream, P4_NATIVE_CHUNK_TYPE, &size, &version) == 0) {
        return 0;
    }
    if (!versionAccepted(version)) {
        raiseStreamError();
        return 0;
    }

    void *raster = func_003fd940(stream);
    if (raster == NULL) {
        return 0;
    }
    void *texture = func_003ef2e0(raster);
    *result = P4_H2G(texture);
    if (texture == NULL) {
        func_003ec330(raster);
        return 0;
    }
    func_003ef470(texture, name);
    func_003ef510(texture, mask);

    /* Filter in the low byte; U and V addressing in bits 8-11 and 12-15, V = U when 0. */
    uint32_t word = 0;
    memcpy(&word, hdr + 4, sizeof word);
    const uint32_t filter = hdr[4];
    const uint32_t u = (word >> 8) & 0xFu;
    uint32_t v = (word >> 12) & 0xFu;
    if (v == 0) {
        v = u;
    }
    P4RwTexture *t = texture;
    t->filterAddressing = (t->filterAddressing & 0xFFFFFF00u) | filter;
    t->filterAddressing = (t->filterAddressing & 0xFFFFF0FFu) | ((u << 8) & 0xF00u);
    t->filterAddressing = (t->filterAddressing & 0xFFFF0FFFu) | ((v << 12) & 0xF000u);
    return 1;
}

/* Native texture write (stdFunc 27). Returns 1 on success, 0 on a stream failure. */
int32_t func_003fddc0(void *pOut, void *pInOut, int32_t nI)
{
    void *stream = pOut;
    P4RwTexture *tex = pInOut;
    (void)nI;

    void *raster = P4_G2H(tex->raster);
    if (func_003deff0(stream, P4_NATIVE_CHUNK_TYPE, 8, (int32_t)P4_NATIVE_VERSION_WRITE,
                      P4_NATIVE_CHUNK_FLAGS) == 0) {
        return 0;
    }

    static const char kId[4] = {'P', 'S', '2', '\0'};
    uint8_t hdr[8];
    memcpy(hdr, kId, sizeof kId);
    const uint32_t fa = tex->filterAddressing;
    const uint32_t filter = fa & 0xFFu;
    const uint32_t u = (fa >> 8) & 0xFu;
    const uint32_t v = (fa >> 12) & 0xFu;
    const uint32_t word = filter | (u << 8) | (v << 12);
    memcpy(hdr + 4, &word, sizeof word);
    if (func_003e2ab0(stream, hdr, 8) == 0) {
        return 0;
    }

    if (func_003e4b20(tex->name, stream) == 0) {
        return 0;
    }
    if (func_003e4b20(tex->mask, stream) == 0) {
        return 0;
    }

    /* The raster chunk: 76 bytes of struct plus the pixel payload size the GETSIZE sum uses. */
    const P4RwRasterDims *temp = rasterDims(tex);
    const int32_t size = 76 + temp->originalWidth + temp->originalHeight + 0xC;
    if (func_003deff0(stream, P4_NATIVE_CHUNK_TYPE, size, (int32_t)P4_NATIVE_VERSION_WRITE,
                      P4_NATIVE_CHUNK_FLAGS) == 0) {
        return 0;
    }
    return func_003fd690(stream, raster, size) != 0;
}
