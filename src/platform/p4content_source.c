#include "p4pc/content_source.h"
#include "p4pc/content_digest.h"
#include "p4pc/ps2_libc.h"
#include "p4pc/texture_key.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOURCE_SLOTS 4096u
#define SOURCE_CHUNK (64u*1024u)
/* Sorted, disjoint host buffer intervals, never guest-layout pointers. All
 * mutation is on the game/file-server thread. Most byte operations miss this
 * index; binary rejection must not walk every loaded asset on every memcpy. */
typedef struct SourceRange {
    const uint8_t *bytes;
    size_t size;
    uint64_t age;
    P4ContentSource source;
    char digest[65];
} SourceRange;
static SourceRange *ranges;
static size_t range_count;
static uint64_t next_age;
int p4_content_sources_enabled;
static P4ContentSource pending_source;
static int pending_known;
static P4ContentReadScope *read_scopes;
static P4ContentManagedRange managed_range;
static _Thread_local unsigned source_thread_token;
static const void *source_owner_thread;

void p4_content_sources_assert_owner(void)
{
    assert(p4_content_sources_enabled && source_owner_thread == &source_thread_token);
}

int p4_content_raster_set(P4ContentRaster **raster, const P4ContentSource *source)
{
    if (!*raster) *raster=malloc(sizeof **raster);
    if (!*raster) return -1;
    (*raster)->source=*source;
    (*raster)->key[0]=0;
    (*raster)->authored_stride=(*raster)->authored_rows=0;
    return 0;
}
void p4_content_raster_free(P4ContentRaster *raster) { free(raster); }
int p4_content_source_authored(P4ContentSource *out, const P4ContentSource *source)
{
    if (!source || source->origin!=P4_TEXTURE_OVERRIDE || !source->rank ||
        !source->provider[0] || !source->key[0] ||
        !memchr(source->provider,0,sizeof source->provider) ||
        !memchr(source->key,0,sizeof source->key)) return 0;
    if (out) {
        *out=*source;
        out->format=P4_TEX_RGBA8888;
        out->palette_format=P4_PAL_NONE;
    }
    return 1;
}
int p4_content_source_same(const P4ContentSource *a, const P4ContentSource *b)
{
    return a->origin==b->origin && a->rank==b->rank &&
        !strcmp(a->provider,b->provider) && !strcmp(a->key,b->key);
}
void p4_content_raster_upload_error(const P4ContentSource *source, unsigned error)
{
    fprintf(stderr,"p4gl: authored raster %s/%s upload failed (GL 0x%x)\n",
            source->provider,source->key,error);
}

static int interval(const void *pointer, size_t size, uintptr_t *begin, uintptr_t *end)
{
    *begin=(uintptr_t)pointer;
    if (!pointer || !size || size>UINTPTR_MAX-*begin) return 0;
    *end=*begin+size;
    return 1;
}
void p4_content_host_read_begin(P4ContentReadScope *scope, const void *bytes, size_t size)
{
    uintptr_t start,end;
    if (!p4_content_sources_enabled || !scope || !interval(bytes,size,&start,&end)) return;
    scope->bytes=bytes; scope->size=size; scope->next=read_scopes;
    read_scopes=scope;
}
void p4_content_host_read_end(P4ContentReadScope *scope)
{
    P4ContentReadScope **link=&read_scopes;
    while (*link && *link!=scope) link=&(*link)->next;
    if (*link) {
        *link=scope->next;
        p4_content_source_forget(scope->bytes,scope->size);
    }
}
int p4_content_host_read_contains(const void *bytes, size_t size)
{
    uintptr_t start,end;
    if (!p4_content_sources_enabled || !interval(bytes,size,&start,&end)) return 0;
    for (P4ContentReadScope *scope=read_scopes;scope;scope=scope->next) {
        uintptr_t base=(uintptr_t)scope->bytes;
        if (start>=base && start-base<scope->size && size<=scope->size-(start-base)) return 1;
    }
    return 0;
}
static size_t lower_bound(uintptr_t address)
{
    size_t low=0, high=range_count;
    while (low<high) {
        size_t middle=low+(high-low)/2;
        if ((uintptr_t)ranges[middle].bytes<address) low=middle+1;
        else high=middle;
    }
    return low;
}
static void observe_bytes(void *destination, const void *source, uint32_t size, int after)
{
    if (!after) {
        pending_known=source &&
            ((managed_range && managed_range(destination,size)) || p4_content_host_read_contains(destination,size)) &&
            p4_content_source_find(source,size,&pending_source);
        p4_content_source_forget(destination,size);
    } else if (pending_known) {
        p4_content_source_mark(destination,size,&pending_source);
        pending_known=0;
    }
}
int p4_content_sources_enable(int enabled, P4ContentManagedRange managed)
{
    if (!enabled) {
        if (p4_content_sources_enabled) p4pc_bytes_observer(NULL);
        p4_content_sources_enabled=0; pending_known=0;
        read_scopes=NULL;
        managed_range=NULL;
        source_owner_thread=NULL;
        free(ranges); ranges=NULL; range_count=0; next_age=0;
    } else if (!ranges) {
        ranges=calloc(SOURCE_SLOTS,sizeof *ranges);
        if (!ranges) return -1;
        if (p4pc_bytes_observer(observe_bytes)!=0) {
            free(ranges); ranges=NULL;
            return -1;
        }
        source_owner_thread=&source_thread_token;
        p4_content_sources_enabled=1;
        managed_range=managed;
    } else if (managed_range!=managed) return -1;
    return 0;
}
void p4_content_source_forget(const void *bytes, size_t size)
{
    uintptr_t start,end;
    size_t first,last;
    if (!p4_content_sources_enabled || !interval(bytes,size,&start,&end)) return;
    first=lower_bound(start);
    if (first && start-(uintptr_t)ranges[first-1].bytes<ranges[first-1].size) --first;
    last=first;
    while (last<range_count && (uintptr_t)ranges[last].bytes<end) ++last;
    if (last!=first) {
        memmove(ranges+first,ranges+last,(range_count-last)*sizeof *ranges);
        range_count-=last-first;
    }
}
void p4_content_source_mark(const void *bytes, size_t size, const P4ContentSource *source)
{
    uintptr_t start,end;
    const uint8_t *cursor=bytes;
    P4ContentSource origin;
    if (!p4_content_sources_enabled || !source || !interval(bytes,size,&start,&end)) return;
    origin=*source;
    origin.provider[sizeof origin.provider-1]=0;
    origin.key[sizeof origin.key-1]=0;
    p4_content_source_forget(bytes,size);
    if (origin.origin==P4_TEXTURE_UNKNOWN) return;
    while (size) {
        size_t slot,part=size>SOURCE_CHUNK?SOURCE_CHUNK:size;
        SourceRange *entry;
        if (range_count==SOURCE_SLOTS) {
            size_t oldest=0;
            for (size_t i=1;i<range_count;++i) if (ranges[i].age<ranges[oldest].age) oldest=i;
            memmove(ranges+oldest,ranges+oldest+1,(range_count-oldest-1)*sizeof *ranges);
            --range_count; /* eviction loses attribution, never invents it */
        }
        slot=lower_bound((uintptr_t)cursor);
        memmove(ranges+slot+1,ranges+slot,(range_count-slot)*sizeof *ranges);
        ++range_count;
        entry=&ranges[slot]; entry->bytes=cursor; entry->size=part;
        entry->age=next_age++; entry->source=origin;
        p4_content_digest(cursor,part,entry->digest);
        cursor+=part; size-=part;
    }
}
static int same_source(const P4ContentSource *a, const P4ContentSource *b)
{
    return a->origin==b->origin && a->rank==b->rank && a->format==b->format &&
        a->palette_format==b->palette_format && strcmp(a->provider,b->provider)==0 &&
        strcmp(a->key,b->key)==0;
}
int p4_content_source_find(const void *bytes, size_t size, P4ContentSource *source)
{
    uintptr_t cursor,end;
    int first=1;
    P4ContentSource result;
    if (!p4_content_sources_enabled || !source || !interval(bytes,size,&cursor,&end)) return 0;
    /* Chunk snapshots bound verification to relevant bytes, rather than hashing
     * a whole multi-megabyte resource every time a stream copies one header. */
    while (cursor<end) {
        size_t slot=lower_bound(cursor);
        SourceRange *found;
        char digest[65];
        if (slot==range_count || (uintptr_t)ranges[slot].bytes!=cursor) {
            if (!slot) return 0;
            --slot;
        }
        found=&ranges[slot];
        if (cursor-(uintptr_t)found->bytes>=found->size) return 0;
        p4_content_digest(found->bytes,found->size,digest);
        if (strcmp(found->digest,digest)!=0) {
            memmove(ranges+slot,ranges+slot+1,(range_count-slot-1)*sizeof *ranges);
            --range_count;
            return 0;
        }
        if (first) { result=found->source; first=0; }
        else if (!same_source(&result,&found->source)) return 0;
        cursor=(uintptr_t)found->bytes+found->size;
    }
    *source=result;
    return 1;
}
