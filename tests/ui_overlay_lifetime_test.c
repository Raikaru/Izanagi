/* Queue and resource lifetime fixture. The real walker/renderer is exercised by
 * the game witness; here a queue stub retains the exact pack pointer. */
#include "p4pc/ui_overlay.h"
#include "p4pc/mod_resources.h"
#include "p4pc/native_glyphs.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static P4ResourceImage image = {.token = 1, .raster = 0x12345678, .width = 8, .height = 8,
    .texture_width = 8, .texture_height = 8};
static unsigned pins;
static uint8_t *queued[64], *nodes[64], *node_arena;
static unsigned queued_count, drawn_count;
const P4ResourceImage *p4_resources_query(uint32_t token) { return token == 1 ? &image : NULL; }
int p4_resources_pin(uint32_t token) { assert(token == 1); ++pins; return P4_RESOURCE_OK; }
int p4_resources_release(uint32_t token) { assert(token == 1 && pins); --pins; return P4_RESOURCE_OK; }
int p4_native_glyphs_validate(const uint8_t *b, size_t n, P4NativeGlyphFlavor f, int m)
{ (void)b; (void)n; (void)f; (void)m; return 0; }
void sflDataDrawNarrowRoundedFrame(int32_t x, int32_t y, float z, int32_t w, int32_t h, int32_t rgb,
    int32_t a, int32_t m, int32_t px, int16_t py, void *q, float r, float sx, float sy)
{ (void)x;(void)y;(void)z;(void)w;(void)h;(void)rgb;(void)a;(void)m;(void)px;(void)py;(void)q;(void)r;(void)sx;(void)sy; assert(0); }
int frFontExDrawTextDeferredWithCharWidth(float x,float y,float z,int c,signed char m,int f,const char *t,int fl,int u,void *q,int s)
{ (void)x;(void)y;(void)z;(void)c;(void)m;(void)f;(void)t;(void)fl;(void)u;(void)q;(void)s; assert(0); return 0; }
uint32_t frFontExCreateText(float x,float y,float z,int c,signed char p,int f,const char *t,int fl,int32_t *w)
{ (void)x;(void)y;(void)z;(void)c;(void)p;(void)f;(void)t;(void)fl;(void)w; assert(0); return 0; }
uint32_t frFontListRelease(uint32_t f) { (void)f; assert(0); return 0; }
uint8_t *PrimAllocQueueNode(void)
{
    if (!node_arena) node_arena = p4t_galloc(64 * 0x30, 16);
    assert(node_arena && queued_count < 64);
    uint8_t *node = node_arena + queued_count * 0x30;
    memset(node, 0, 0x30);
    return node;
}
void PrimAppendNode(uint8_t *q, uint8_t *n)
{
    assert(q == P4_G2H(0x00793e80u + 0xd7u * 0x30u) && queued_count < 64);
    nodes[queued_count] = n;
    queued[queued_count++] = P4_G2H(P4_G32(P4_H2G(n) + 16u));
}
uint32_t kwlnGetCamera(void) { return 0; }
int32_t func_003f7490(void *c, void *v, int32_t m) { (void)c;(void)v;(void)m; assert(0); return 0; }
int32_t sdkSpriteDrawNow(float x,float y,float z,int32_t rgb,uint8_t a,int32_t record,void *pack,
    int32_t states,int16_t px,int16_t py,float angle,float sx,float sy)
{
    (void)x;(void)y;(void)z;(void)rgb;(void)a;(void)px;(void)py;(void)angle;(void)sx;(void)sy;
    assert(record == 0 && states == 1 && pack && pins);
    ++drawn_count;
    return 0;
}
static uint32_t word(const uint8_t *p, unsigned off) { uint32_t v; memcpy(&v, p + off, 4); return v; }
static int add_sprite(uint32_t owner, int left)
{
    int s = p4_ui_sprite_create(owner, 1, left, 0, 2, 2); assert(s >= 0);
    P4UiOverlayItem item = {.kind=P4_UI_ITEM_SPRITE, .context=P4_UI_CONTEXT_SCREEN,
        .rgb=0xffffff, .alpha=255, .scale=1, .sprite=s};
    assert(p4_ui_overlay_add(owner, &item) >= 0);
    return s;
}
static void enqueue(void) { p4_ui_overlay_draw(P4_UI_CONTEXT_SCREEN); assert(queued_count); }
static void intact(uint8_t *p, int left)
{
    assert(pins && word(p, 0x104) == image.raster);
    assert(word(p + 0x240, 0x54) == (uint32_t)left);
    assert(word(p + 0x240, 0x5c) == (uint32_t)(left + 2));
}
static void clear_queue(void) { queued_count = 0; p4_ui_overlay_queue_cleared(); }
static void flush_queue(void)
{
    unsigned before = drawn_count;
    for (unsigned i = 0; i < queued_count; ++i) {
        uint32_t addr = P4_H2G(nodes[i]);
        void (*callback)(uint8_t *, int32_t) =
            (void (*)(uint8_t *, int32_t))P4_FN_G2H(P4_G32(addr + 8u));
        callback(nodes[i] + 28, (int32_t)P4_G32(addr + 16u));
    }
    assert(drawn_count == before + queued_count);
}
static void test_flush(void)
{
    add_sprite(7, 1); enqueue(); uint8_t *old = queued[0];
    p4_ui_overlay_release_owner(7); assert(pins == 1); intact(old, 1);
    /* Execute the deferred callback against the original pinned pack. */
    intact(queued[0], 1); flush_queue(); clear_queue(); assert(!pins);
}
static void test_failed_frame(void)
{
    add_sprite(7, 1); enqueue(); uint8_t *old = queued[0];
    p4_ui_overlay_release_owner(7); assert(pins == 1); intact(old, 1);
    /* No draw callback: next frame clears the buckets directly. */
    clear_queue(); assert(!pins && !drawn_count);
}
static void test_reregister(void)
{
    int old_handle = add_sprite(7, 1); enqueue(); uint8_t *old = queued[0];
    p4_ui_overlay_release_owner(7); assert(pins == 1);
    int new_handle = add_sprite(7, 4); assert(new_handle != old_handle && pins == 2);
    intact(old, 1); enqueue(); assert(queued[1] != old); intact(queued[1], 4);
    p4_ui_overlay_release_owner(7); assert(pins == 2);
    flush_queue(); clear_queue(); assert(!pins);
}
static void test_cycles(void)
{
    for (int i = 0; i < 1000; ++i) {
        add_sprite(7, 1); enqueue(); p4_ui_overlay_release_owner(7);
        assert(pins == 1); clear_queue(); assert(!pins && !drawn_count);
    }
    /* A released, never queued sprite needs no clear. */
    add_sprite(7, 0); p4_ui_overlay_release_owner(7); assert(!pins);
    /* The retirement set is the bounded sprite table, not an unbounded list. */
    for (int i = 0; i < P4_UI_SPRITES; ++i) {
        add_sprite((uint32_t)(i + 1), 0); enqueue(); p4_ui_overlay_release_owner((uint32_t)(i + 1));
    }
    assert(pins == P4_UI_SPRITES);
    assert(p4_ui_sprite_create(123, 1, 0, 0, 2, 2) == P4_UI_FULL);
    clear_queue(); assert(!pins);
    add_sprite(7, 0); p4_ui_overlay_release_owner(7); assert(!pins);
}
int main(int argc, char **argv)
{
    if (p4t_guest_map(0x00700000, 0x100000)) return 77;
    assert(argc == 2);
    if (!strcmp(argv[1], "flush")) test_flush();
    else if (!strcmp(argv[1], "failed")) test_failed_frame();
    else if (!strcmp(argv[1], "reregister")) test_reregister();
    else if (!strcmp(argv[1], "cycles")) test_cycles();
    else assert(0);
    printf("ui_overlay_lifetime: %s passed\n", argv[1]);
    return 0;
}
