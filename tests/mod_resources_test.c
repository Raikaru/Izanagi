/* Token/lifetime unit fixture; real RW/GL behavior has a separate native gate. */
#include "p4pc/mod_resources.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t created, destroyed;
static int backend_error;
static const P4ContentSource source_data = {
    .origin = P4_TEXTURE_OVERRIDE, .rank = 3, .provider = "replacement.mod", .key = "icons/card.png"
};
static const P4ContentSource *source = &source_data;
int p4_resources_raster_create(const struct P4ContentSource *src,
    const uint8_t *rgba, uint32_t width, uint32_t height,
    uint32_t *raster, uint32_t *pixels, uint32_t *stride, uint32_t *rows)
{
    assert(src == source && width == 2 && height == 3);
    if (backend_error) return backend_error;
    uint8_t *copy = p4t_galloc(32, 16); assert(copy);
    memset(copy, 0, 32); memcpy(copy, rgba, 24);
    *stride = 8; *rows = 4;
    *pixels = P4_H2G(copy); *raster = P4_H2G(p4t_galloc(64, 16)); ++created;
    return P4_RESOURCE_OK;
}
void p4_resources_raster_destroy(uint32_t raster)
{
    assert(raster); ++destroyed;
}
int main(void)
{
    if (p4t_guest_init()) return 77;
    uint8_t rgba[24]; for (size_t i = 0; i < sizeof rgba; ++i) rgba[i] = (uint8_t)(i * 7);
    P4Resources *r = p4_resources_create(128); assert(r);
    uint32_t first = 123, second = 456;
    backend_error = P4_RESOURCE_RENDER_ERROR;
    assert(p4_resources_image(r, "cards", "cards:first", source, rgba, sizeof rgba, 2, 3, 1, &first) == P4_RESOURCE_RENDER_ERROR);
    assert(first == 123 && !p4_resources_count(r) && !created);
    backend_error = P4_RESOURCE_NOT_READY;
    assert(p4_resources_image(r, "cards", "cards:first", source, rgba, sizeof rgba, 2, 3, 1, &first) == P4_RESOURCE_NOT_READY);
    assert(first == 123 && !p4_resources_count(r) && !created);
    backend_error = 0;
    assert(p4_resources_image(r, "cards", "cards:first", source, rgba, sizeof rgba, 2, 3, 1, &first) == P4_RESOURCE_OK);
    assert(p4_resources_image(r, "cards", "cards:first", source, rgba, sizeof rgba, 2, 3, 1, &second) == P4_RESOURCE_CONFLICT && second == 456);
    assert(p4_resources_image(r, "cards", "other:second", source, rgba, sizeof rgba, 2, 3, 1, &second) == P4_RESOURCE_INVALID);
    assert(p4_resources_image(r, "cards", "cards:second", source, rgba, 23, 2, 3, 1, &second) == P4_RESOURCE_INVALID);
    assert(p4_resources_image(r, "cards", "cards:second", source, rgba, sizeof rgba, 2, 3, 1, &second) == P4_RESOURCE_OK);
    uint32_t unchanged = 99;
    assert(p4_resources_image(r, "cards", "cards:third", source, rgba, sizeof rgba, 2, 3, 1, &unchanged) == P4_RESOURCE_NOMEM && unchanged == 99);
    assert(!p4_resources_query(first) && p4_resources_pin(first) == P4_RESOURCE_NOT_FOUND);
    assert(!p4_resources_at(r, 0) && !p4_resources_find(r, "cards:first"));
    assert(p4_resources_freeze(r, 7) == P4_RESOURCE_OK);
    assert(p4_resources_revision(r) == 7 && p4_resources_count(r) == 2);
    assert(p4_resources_freeze(r, 8) == P4_RESOURCE_FROZEN);
    const P4ResourceImage *image = p4_resources_query(first); assert(image);
    assert(image == p4_resources_at(r, 0) && image == p4_resources_find(r, "cards:first"));
    assert(image->width == 2 && image->height == 3 && image->stride == 8 && image->texture_width == 2 && image->texture_height == 4);
    assert(!memcmp(P4_G2H(image->pixels), rgba, sizeof rgba));
    const uint8_t *shadow = P4_G2H(image->pixels);
    for (size_t i = sizeof rgba; i < 32; ++i) assert(shadow[i] == 0);
    assert(!strcmp(image->owner, "cards") && !strcmp(image->source.provider, "replacement.mod"));
    assert(!strcmp(image->source.key, "icons/card.png") && image->source.rank == 3);
    assert((image->flags & P4_RESOURCE_PROVIDER_OVERRIDE) && p4_resources_conflicts(r) == 2);
    assert(p4_resources_pin(first) == P4_RESOURCE_OK && p4_resources_pin(first) == P4_RESOURCE_OK);
    assert(p4_resources_pin(second) == P4_RESOURCE_OK);
    assert(p4_resources_retire_unpinned(NULL) == P4_RESOURCE_INVALID);
    assert(p4_resources_retire_unpinned(r) == P4_RESOURCE_CONFLICT);
    assert(p4_resources_revision(r) == 7 && p4_resources_query(first) && !destroyed);
    assert(p4_resources_release(second) == P4_RESOURCE_OK);
    assert(p4_resources_retire_unpinned(r) == P4_RESOURCE_CONFLICT);
    assert(p4_resources_pin(second) == P4_RESOURCE_OK);
    p4_resources_retire(r);
    assert(!destroyed && p4_resources_query(first) && p4_resources_query(second));
    assert(p4_resources_pin(first) == P4_RESOURCE_RETIRED);
    assert(p4_resources_release(first) == P4_RESOURCE_OK && !destroyed);
    assert(p4_resources_release(second) == P4_RESOURCE_OK && !destroyed && !p4_resources_query(second));
    assert(p4_resources_release(first) == P4_RESOURCE_OK && destroyed == 2);
    assert(!p4_resources_query(first) && !p4_resources_query(second));
    assert(p4_resources_release(first) == P4_RESOURCE_NOT_FOUND);
    r = p4_resources_create(64); assert(r);
    uint32_t next = 0;
    assert(p4_resources_image(r, "cards", "cards:first", source, rgba, sizeof rgba, 2, 3, 2, &next) == P4_RESOURCE_OK);
    assert(next != first && !p4_resources_query(first));
    p4_resources_retire(r); /* Failed activation/staging also releases real ownership. */
    assert(created == 3 && destroyed == 3);
    r = p4_resources_create(0); assert(r);
    assert(p4_resources_image(r, "cards", "cards:first", source, rgba, sizeof rgba, 2, 3, 1, &next) == P4_RESOURCE_NOMEM);
    assert(p4_resources_freeze(r, 1) == P4_RESOURCE_OK);
    p4_resources_retire(r);
    assert(created == 3 && destroyed == 3);
    r = p4_resources_create(64); assert(r);
    assert(p4_resources_image(r, "cards", "cards:first", source, rgba, sizeof rgba, 2, 3, 1, &next) == P4_RESOURCE_OK);
    assert(p4_resources_freeze(r, 1) == P4_RESOURCE_OK);
    assert(p4_resources_pin(next) == P4_RESOURCE_OK);
    assert(p4_resources_retire_unpinned(r) == P4_RESOURCE_CONFLICT);
    assert(p4_resources_release(next) == P4_RESOURCE_OK);
    assert(p4_resources_retire_unpinned(r) == P4_RESOURCE_OK);
    assert(!p4_resources_query(next) && created == 4 && destroyed == 4);
    puts("mod_resources: quota, atomic failure, immutable tokens, pins and generation retirement passed");
    return 0;
}
