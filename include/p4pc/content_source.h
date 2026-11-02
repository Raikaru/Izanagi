/* Native asset provenance, shared by texture/music consumers. This is not a
 * package resolver: the VFS supplies the already-selected provider metadata. */
#ifndef P4PC_CONTENT_SOURCE_H
#define P4PC_CONTENT_SOURCE_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/texture_dump.h"

/* A named package resource is opened from an issued provider file plan, not
 * from a VFS layer. This sentinel is not a layer number or a precedence rank. */
#define P4_CONTENT_RANK_PACKAGE_PLAN UINT32_MAX

typedef struct P4ContentSource {
    enum P4TextureOrigin origin;
    uint32_t rank;
    uint32_t format, palette_format;
    char provider[64];
    char key[512];
} P4ContentSource;

/* Host-owned raster provenance; never stored in guest structures. */
typedef struct P4ContentRaster {
    P4ContentSource source;
    char key[65];
    int32_t authored_stride, authored_rows;
} P4ContentRaster;
int p4_content_raster_set(P4ContentRaster **raster, const P4ContentSource *source);
void p4_content_raster_free(P4ContentRaster *raster);
/* Validate even with out=NULL; otherwise write the normalized authored copy. */
int p4_content_source_authored(P4ContentSource *out, const P4ContentSource *source);
int p4_content_source_same(const P4ContentSource *a, const P4ContentSource *b);
void p4_content_raster_upload_error(const P4ContentSource *source, unsigned error);

/* Read by native byte/asset seams: zero means no hashing/index/observer work. */
extern int p4_content_sources_enabled;
typedef int (*P4ContentManagedRange)(const void *pointer, size_t size);
/* Automatic copy attribution is restricted to managed_range or an explicit
 * live native scope. NULL means scoped native buffers only, never trust-all. */
int p4_content_sources_enable(int enabled, P4ContentManagedRange managed_range);
/* Raster provenance scratch is render-thread owned. Assert that the caller is
 * on the thread which enabled the source service before borrowing it. */
void p4_content_sources_assert_owner(void);
/* Successful reads/semantic decoder outputs register actual initialized bytes.
 * Ranges are verified against a SHA-256 snapshot before attribution. A full
 * index discards old attribution (unknown), never invents a provider. */
void p4_content_source_mark(const void *bytes, size_t size, const P4ContentSource *source);
int p4_content_source_find(const void *bytes, size_t size, P4ContentSource *source);
/* Native memcpy/clear/allocation seams call these only while enabled. They do
 * not copy or modify the caller's bytes. Forget before releasing/protecting a
 * range, so verification never reads freed memory. */
void p4_content_source_forget(const void *bytes, size_t size);

/* A native malloc buffer may receive VFS bytes only while its owner explicitly
 * guarantees this scope's lifetime. End before free on every path. Otherwise
 * the VFS adapter tracks only managed guest RAM, never arbitrary host buffers. */
typedef struct P4ContentReadScope {
    struct P4ContentReadScope *next;
    const void *bytes;
    size_t size;
} P4ContentReadScope;
void p4_content_host_read_begin(P4ContentReadScope *scope, const void *bytes, size_t size);
void p4_content_host_read_end(P4ContentReadScope *scope);
int p4_content_host_read_contains(const void *bytes, size_t size);
/* Semantic TMX decode boundary (retail 0x00463930), after all guest writes. */
void p4_content_tmx_image(void *image, const void *encoded);
#endif
