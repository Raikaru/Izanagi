#include "p4pc/mod_resources.h"
#include "mod_resources_native.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct ImageSlot { P4ResourceImage image; uint32_t pins; } ImageSlot;
struct P4Resources {
    ImageSlot *images;
    size_t count, capacity;
    uint32_t generation, budget, used, revision, pins, conflicts;
    int retired;
};
/* Never recycle a generation: even a stale unpinned token cannot alias a new
 * image. Exhaustion is explicit instead of wrapping a 16-bit identity. */
static P4Resources *generations[65536];
static uint32_t next_generation = 1;

static int owner_valid(const char *s, size_t capacity)
{
    if (!s || !s[0] || s[0] == '.') return 0;
    for (size_t i = 0; i < capacity; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (!c) return i && s[i - 1] != '.';
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '.' || c == '-' || c == '_')) return 0;
    }
    return 0;
}
static int key_valid(const char *owner, const char *key)
{
    if (!key) return 0;
    size_t n = strlen(owner), length = 0;
    while (length < sizeof ((P4ResourceImage *)0)->key && key[length]) ++length;
    return length < sizeof ((P4ResourceImage *)0)->key && length > n + 1 &&
        !strncmp(owner, key, n) && key[n] == ':' && owner_valid(key + n + 1, length - n);
}
static uint32_t texture_size(uint32_t size)
{
    uint32_t result = 1;
    while (result < size) {
        if (result > UINT32_MAX / 2) return 0;
        result *= 2;
    }
    return result;
}
static ImageSlot *slot(uint32_t token, P4Resources **generation)
{
    P4Resources *r = generations[token >> 16];
    uint32_t index = token & 65535u;
    if (!r || !r->revision || !index || index > r->count) return NULL;
    if (generation) *generation = r;
    return &r->images[index - 1];
}
static void dispose(P4Resources *r)
{
    generations[r->generation] = NULL;
    for (size_t i = 0; i < r->count; ++i)
        p4_resources_raster_destroy(r->images[i].image.raster);
    free(r->images); free(r);
}
P4Resources *p4_resources_create(uint32_t budget)
{
    if (next_generation >= 65536u) return NULL;
    P4Resources *r = calloc(1, sizeof *r);
    if (!r) return NULL;
    r->generation = next_generation++; r->budget = budget;
    generations[r->generation] = r;
    return r;
}
int p4_resources_image(P4Resources *r, const char *owner, const char *key,
    const struct P4ContentSource *source, const uint8_t *rgba, size_t size,
    uint32_t width, uint32_t height, uint32_t revision, uint32_t *token)
{
    if (!r || !owner_valid(owner, 64) || !key_valid(owner, key) || !source || !rgba || !token || !revision ||
        !width || !height || width > INT32_MAX || height > INT32_MAX) return P4_RESOURCE_INVALID;
    if (source->origin != P4_TEXTURE_OVERRIDE || !source->rank || !memchr(source->provider, 0, sizeof source->provider) ||
        !memchr(source->key, 0, sizeof source->key) || !source->provider[0] || !source->key[0]) return P4_RESOURCE_INVALID;
    if (r->retired) return P4_RESOURCE_RETIRED;
    if (r->revision) return P4_RESOURCE_FROZEN;
    uint64_t bytes = (uint64_t)width * height * 4u;
    if (bytes != size || bytes > UINT32_MAX) return P4_RESOURCE_INVALID;
    uint32_t texture_width = texture_size(width), texture_height = texture_size(height);
    if (!texture_width || !texture_height) return P4_RESOURCE_INVALID;
    uint64_t charge = (uint64_t)texture_width * texture_height * 8u;
    if (charge > r->budget - r->used || r->count == 65535u) return P4_RESOURCE_NOMEM;
    for (size_t i = 0; i < r->count; ++i)
        if (!strcmp(r->images[i].image.key, key)) return P4_RESOURCE_CONFLICT;
    if (r->count == r->capacity) {
        size_t capacity = r->capacity ? r->capacity * 2 : 16;
        ImageSlot *images = realloc(r->images, capacity * sizeof *images);
        if (!images) return P4_RESOURCE_NOMEM;
        r->images = images; r->capacity = capacity;
    }
    uint32_t raster = 0, pixels = 0, stride = 0, padded_rows = 0;
    int result = p4_resources_raster_create(source, rgba, width, height, &raster, &pixels, &stride, &padded_rows);
    if (result) return result;
    /* Charge and expose the immutable real allocation, never mutable RW fields. */
    charge = (uint64_t)stride * padded_rows * 2u;
    if (charge > r->budget - r->used) {
        p4_resources_raster_destroy(raster);
        return P4_RESOURCE_NOMEM;
    }
    ImageSlot *image = &r->images[r->count]; memset(image, 0, sizeof *image);
    uint32_t assigned = r->generation << 16 | (uint32_t)(r->count + 1);
    image->image.token = assigned; image->image.raster = raster; image->image.pixels = pixels;
    image->image.stride = stride;
    image->image.width = width; image->image.height = height;
    image->image.texture_width = stride / 4u; image->image.texture_height = padded_rows;
    image->image.revision = revision; image->image.source = *source;
    image->image.source.format = P4_TEX_RGBA8888; image->image.source.palette_format = P4_PAL_NONE;
    if (strcmp(owner, source->provider)) { image->image.flags |= P4_RESOURCE_PROVIDER_OVERRIDE; ++r->conflicts; }
    strcpy(image->image.key, key); strcpy(image->image.owner, owner);
    ++r->count; r->used += (uint32_t)charge; *token = assigned;
    return P4_RESOURCE_OK;
}
int p4_resources_freeze(P4Resources *r, uint32_t revision)
{
    if (!r || !revision) return P4_RESOURCE_INVALID;
    if (r->retired) return P4_RESOURCE_RETIRED;
    if (r->revision) return P4_RESOURCE_FROZEN;
    r->revision = revision; return P4_RESOURCE_OK;
}
uint32_t p4_resources_revision(const P4Resources *r) { return r && !r->retired ? r->revision : 0; }
size_t p4_resources_count(const P4Resources *r) { return r && !r->retired ? r->count : 0; }
const P4ResourceImage *p4_resources_at(const P4Resources *r, size_t index)
{ return r && r->revision && !r->retired && index < r->count ? &r->images[index].image : NULL; }
const P4ResourceImage *p4_resources_find(const P4Resources *r, const char *key)
{
    if (r && r->revision && !r->retired && key)
        for (size_t i = 0; i < r->count; ++i) if (!strcmp(r->images[i].image.key, key)) return &r->images[i].image;
    return NULL;
}
size_t p4_resources_conflicts(const P4Resources *r) { return r && !r->retired ? r->conflicts : 0; }
const P4ResourceImage *p4_resources_query(uint32_t token)
{
    P4Resources *r;
    ImageSlot *image = slot(token, &r);
    return image && (!r->retired || image->pins) ? &image->image : NULL;
}
int p4_resources_pin(uint32_t token)
{
    P4Resources *r;
    ImageSlot *image = slot(token, &r);
    if (!image) return P4_RESOURCE_NOT_FOUND;
    if (r->retired) return P4_RESOURCE_RETIRED;
    if (r->pins == UINT32_MAX || image->pins == UINT32_MAX) return P4_RESOURCE_NOMEM;
    ++r->pins; ++image->pins; return P4_RESOURCE_OK;
}
int p4_resources_release(uint32_t token)
{
    P4Resources *r;
    ImageSlot *image = slot(token, &r);
    if (!image || !image->pins) return P4_RESOURCE_NOT_FOUND;
    --image->pins; --r->pins;
    if (r->retired && !r->pins) dispose(r);
    return P4_RESOURCE_OK;
}
void p4_resources_retire(P4Resources *r)
{
    if (!r) return;
    r->retired = 1;
    if (!r->pins) dispose(r);
}
int p4_resources_retire_unpinned(P4Resources *r)
{
    if (!r) return P4_RESOURCE_INVALID;
    if (r->retired) return P4_RESOURCE_RETIRED;
    if (r->pins) return P4_RESOURCE_CONFLICT;
    dispose(r);
    return P4_RESOURCE_OK;
}
