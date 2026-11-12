/* Mod overlays (include/p4pc/ui_overlay.h). State is validated and copied when
 * a mod sets it; the context seams only draw it. Without an item nothing runs:
 * no allocation, no guest write, no queue entry.
 *
 * Sprites: each handle owns one synthetic SdkSpritePack (0x240 bytes, state 5,
 * never linked into the retail pack list) and one 0x80-byte record in
 * port-owned guest memory. rasters[0] is the pinned resource image's real RW
 * raster; the record's texel rectangle selects the sprite, its vertex colours
 * are 0x80 (1.0) and its flags 4, as retail UI records are. sdkSpriteDraw
 * divides the rectangle by the raster's width and height, which equal the GL
 * texture's only for power-of-two images, so only those are accepted.
 *
 * Text: the status rows' calls (frFontExCreateText 00274cd0 to measure,
 * frFontExDrawTextDeferredWithCharWidth to queue, the 128-based glyph scale at
 * 007645a0, sflDataDrawNarrowRoundedFrame for the plate), at most
 * P4_UI_OVERLAY_RUNS runs of 47 glyphs per context: with four status rows and
 * the CONFIG corner that is 9 runs and 382 glyph nodes of the font manager's
 * 256 and 1024 (frFontInitManager(0x100, 0x100), 002713b0). */
#include "p4pc/ui_overlay.h"
#include "p4pc/mod_resources.h"
#include "p4pc/native_glyphs.h"
#include "p4pc/gmem.h"
#include "p4pc/guest.h"
#include <math.h>
#include <stddef.h>
#include <string.h>

extern void sflDataDrawNarrowRoundedFrame(int32_t x, int32_t y, float depth,
    int32_t width, int32_t height, int32_t rgb, int32_t alpha, int32_t mode,
    int32_t pivotX, int16_t pivotY, void *queue, float angle, float scaleX, float scaleY);
extern int frFontExDrawTextDeferredWithCharWidth(float x, float y, float depth,
    int color, signed char mode, int font, const char *text, int flags,
    int unused, void *queue, int spacing);
extern uint32_t frFontExCreateText(float x, float y, float depth, int color,
    signed char palette, int font, const char *text, int flags, int32_t *width);
extern uint32_t frFontListRelease(uint32_t font);
extern int32_t sdkSpriteDrawNow(float x, float y, float depth, int32_t rgb, uint8_t alpha,
    int32_t recordIndex, void *pack, int32_t setStates, int16_t pivotX, int16_t pivotY,
    float angle, float scaleX, float scaleY);

extern uint8_t *PrimAllocQueueNode(void);
extern void PrimAppendNode(uint8_t *queue, uint8_t *node);
extern uint32_t kwlnGetCamera(void); /* the scene camera's guest address */
/* The RenderWare device CAMERACLEAR function (src/render/p4gl_camera.c), which
 * RwCameraClear(camera, colour, mode) runs; mode 2 = rwCAMERACLEARZ. */
extern int32_t func_003f7490(void *camera, void *colour, int32_t mode);
#define QUEUE(index) (0x00793e80u + (index) * 0x30u)
#define FONT_SCALE 0x007645a0u
enum { PACK_SIZE = 0x240, RECORD_SIZE = 0x80, SLOT_SIZE = PACK_SIZE + RECORD_SIZE };
enum { PLATE_PAD = 8, PLATE_HEIGHT = 20, FONT = 1 };
static const uint32_t queues[P4_UI_CONTEXT_COUNT] = {QUEUE(0xb1), QUEUE(0xd7), QUEUE(0xd7)};
/* The status rows' depths: plate 59000, text 58990; sprites sit between. */
static const float plate_depth = 59000.0f, sprite_depth = 58995.0f, text_depth = 58990.0f;

typedef struct Sprite {
    uint32_t owner, token;
    int used, queued, left, top, width, height; /* used: 1 active, 2 retired */
} Sprite;
typedef struct Item {
    uint32_t owner;
    int used, visible, kind, context, align, plate, width, fade, sprite;
    float x, y, scale;
    uint32_t rgb;
    uint8_t alpha;
    int measured;
    int32_t measured_scale; /* glyph scale (128-based) after narrowing */
} Item;
typedef struct TextGuest {
    char text[P4_UI_OVERLAY_ITEMS][P4_UI_OVERLAY_TEXT_MAX];
    int32_t width[P4_UI_OVERLAY_ITEMS];
} TextGuest;

static Sprite sprites[P4_UI_SPRITES];
static Item items[P4_UI_OVERLAY_ITEMS];
static int counts[P4_UI_CONTEXT_COUNT];
static int shown[P4_UI_CONTEXT_COUNT]; /* updates drawn since the context began */
static uint32_t sprite_arena, text_arena;

static uint8_t *pack_of(int handle) { return P4_GPTR(uint8_t, sprite_arena + (uint32_t)handle * SLOT_SIZE); }
static uint32_t pack_address(int handle) { return sprite_arena + (uint32_t)handle * SLOT_SIZE; }
static TextGuest *texts(void) { return P4_GPTR(TextGuest, text_arena); }

static void put32(uint8_t *base, uint32_t offset, uint32_t value) { memcpy(base + offset, &value, 4); }
static void put16(uint8_t *base, uint32_t offset, uint16_t value) { memcpy(base + offset, &value, 2); }

static int power_of_two(uint32_t value) { return value && !(value & (value - 1)); }

static int valid_text(const char *text)
{
    if (!text) return 0;
    size_t length = strnlen(text, P4_UI_OVERLAY_TEXT_MAX);
    return length && length < P4_UI_OVERLAY_TEXT_MAX &&
        !p4_native_glyphs_validate((const uint8_t *)text, length + 1, P4_GLYPHS_RAW, 0);
}

int p4_ui_sprite_create(uint32_t owner, uint32_t token, int left, int top, int width, int height)
{
    const P4ResourceImage *image = p4_resources_query(token);
    if (!image) return P4_UI_NOT_FOUND;
    if (left < 0 || top < 0 || width <= 0 || height <= 0 ||
        (uint32_t)left + (uint32_t)width > image->width || (uint32_t)top + (uint32_t)height > image->height)
        return P4_UI_INVALID;
    if (!power_of_two(image->width) || !power_of_two(image->height) ||
        image->texture_width != image->width || image->texture_height != image->height)
        return P4_UI_NOT_POT;
    int handle = 0;
    while (handle < P4_UI_SPRITES && sprites[handle].used) ++handle;
    if (handle == P4_UI_SPRITES) return P4_UI_FULL;
    if (!sprite_arena && !(sprite_arena = p4_gport_alloc(P4_UI_SPRITES * SLOT_SIZE, 16))) return P4_UI_FULL;
    if (p4_resources_pin(token) != P4_RESOURCE_OK) return P4_UI_NOT_FOUND;
    uint8_t *pack = pack_of(handle), *record = pack + PACK_SIZE;
    memset(pack, 0, SLOT_SIZE);
    put16(pack, 0x000, 5);                                     /* state: loaded */
    memcpy(pack + 0x002, "p4pc-ui", 8);                        /* name (never listed) */
    put32(pack, 0x104, image->raster);                         /* rasters[0] */
    put32(pack, 0x204, pack_address(handle) + PACK_SIZE);      /* recordsAddress */
    put16(pack, 0x216, 1);                                     /* recordCount */
    put16(pack, 0x218 + 0x14, 1);                              /* header.rasterCount */
    put16(pack, 0x218 + 0x16, 1);                              /* header.spriteCount */
    put32(record, 0x14, 0);                                    /* rasterIndex */
    put32(record, 0x18, 4);                                    /* flags, as retail UI records */
    put32(record, 0x54, (uint32_t)left);
    put32(record, 0x58, (uint32_t)top);
    put32(record, 0x5c, (uint32_t)(left + width));
    put32(record, 0x60, (uint32_t)(top + height));
    for (uint32_t i = 0; i < 4; ++i) put32(record, 0x64 + 4 * i, 0x80808080u);
    sprites[handle] = (Sprite){owner, token, 1, 0, left, top, width, height};
    return handle;
}

int p4_ui_sprite_rect(int handle, uint32_t *raster, int rect[4])
{
    if (handle < 0 || handle >= P4_UI_SPRITES || !sprites[handle].used) return P4_UI_NOT_FOUND;
    const P4ResourceImage *image = p4_resources_query(sprites[handle].token);
    if (!image) return P4_UI_NOT_FOUND;
    *raster = image->raster;
    rect[0] = sprites[handle].left;
    rect[1] = sprites[handle].top;
    rect[2] = sprites[handle].width;
    rect[3] = sprites[handle].height;
    return P4_UI_OK;
}

static int item_valid(uint32_t owner, const P4UiOverlayItem *item)
{
    if (!item || item->context < 0 || item->context >= P4_UI_CONTEXT_COUNT ||
        item->align < P4_UI_ALIGN_LEFT || item->align > P4_UI_ALIGN_RIGHT ||
        !isfinite(item->x) || !isfinite(item->y) || fabsf(item->x) > 4096.0f || fabsf(item->y) > 4096.0f ||
        !isfinite(item->scale) || item->scale <= 0.0f || item->scale > 8.0f || item->rgb > 0xFFFFFFu)
        return 0;
    if (item->kind == P4_UI_ITEM_TEXT)
        return valid_text(item->text) && item->width >= 0 && item->width <= 640;
    if (item->kind == P4_UI_ITEM_SPRITE)
        return item->sprite >= 0 && item->sprite < P4_UI_SPRITES && sprites[item->sprite].used == 1 &&
            sprites[item->sprite].owner == owner;
    return 0;
}

int p4_ui_overlay_add(uint32_t owner, const P4UiOverlayItem *item)
{
    if (!item_valid(owner, item)) return P4_UI_INVALID;
    if (item->kind == P4_UI_ITEM_TEXT) {
        int runs = 0;
        for (int i = 0; i < P4_UI_OVERLAY_ITEMS; ++i)
            runs += items[i].used && items[i].kind == P4_UI_ITEM_TEXT && items[i].context == item->context;
        if (runs >= P4_UI_OVERLAY_RUNS) return P4_UI_FULL;
        if (!text_arena && !(text_arena = p4_gport_alloc(sizeof(TextGuest), 16))) return P4_UI_FULL;
    }
    int handle = 0;
    while (handle < P4_UI_OVERLAY_ITEMS && items[handle].used) ++handle;
    if (handle == P4_UI_OVERLAY_ITEMS) return P4_UI_FULL;
    items[handle] = (Item){
        .owner = owner, .used = 1, .visible = 1, .kind = item->kind, .context = item->context,
        .align = item->align, .plate = item->plate, .width = item->width, .fade = item->fade,
        .sprite = item->sprite, .x = item->x, .y = item->y, .scale = item->scale,
        .rgb = item->rgb, .alpha = item->alpha,
    };
    if (item->kind == P4_UI_ITEM_TEXT) {
        memcpy(texts()->text[handle], item->text, strlen(item->text) + 1);
        texts()->width[handle] = 0;
    }
    ++counts[item->context];
    return handle;
}

static Item *owned(uint32_t owner, int handle)
{
    if (handle < 0 || handle >= P4_UI_OVERLAY_ITEMS || !items[handle].used || items[handle].owner != owner)
        return NULL;
    return &items[handle];
}

int p4_ui_overlay_set_text(uint32_t owner, int handle, const char *text)
{
    Item *item = owned(owner, handle);
    if (!item) return P4_UI_NOT_FOUND;
    if (item->kind != P4_UI_ITEM_TEXT || !valid_text(text)) return P4_UI_INVALID;
    if (strcmp(texts()->text[handle], text)) {
        memcpy(texts()->text[handle], text, strlen(text) + 1);
        item->measured = 0;
    }
    return P4_UI_OK;
}

int p4_ui_overlay_set_visible(uint32_t owner, int handle, int visible)
{
    Item *item = owned(owner, handle);
    if (!item) return P4_UI_NOT_FOUND;
    item->visible = !!visible;
    return P4_UI_OK;
}

int p4_ui_overlay_remove(uint32_t owner, int handle)
{
    Item *item = owned(owner, handle);
    if (!item) return P4_UI_NOT_FOUND;
    --counts[item->context];
    *item = (Item){0};
    return P4_UI_OK;
}

static void release_sprite(int i)
{
    p4_resources_release(sprites[i].token);
    sprites[i] = (Sprite){0};
}

void p4_ui_overlay_release_owner(uint32_t owner)
{
    for (int i = 0; i < P4_UI_OVERLAY_ITEMS; ++i)
        if (items[i].used && items[i].owner == owner) p4_ui_overlay_remove(owner, i);
    for (int i = 0; i < P4_UI_SPRITES; ++i) {
        if (sprites[i].used != 1 || sprites[i].owner != owner) continue;
        if (sprites[i].queued) sprites[i].used = 2;
        else release_sprite(i);
    }
}

void p4_ui_overlay_queue_cleared(void)
{
    for (int i = 0; i < P4_UI_SPRITES; ++i) {
        if (sprites[i].used == 2) release_sprite(i);
        else sprites[i].queued = 0;
    }
}

int p4_ui_overlay_count(int context)
{
    return context >= 0 && context < P4_UI_CONTEXT_COUNT ? counts[context] : 0;
}

static void layer_states(uint8_t *node, int32_t argument)
{
    (void)node;
    (void)argument;
    /* frFont and the plate set their own depth test, so clear the depth
     * instead: nothing in the frame draws after the last bucket's layer. */
    const uint32_t camera = kwlnGetCamera();
    if (camera) func_003f7490(P4_G2H(camera), NULL, 2);
}
void p4_ui_overlay_layer_begin(void)
{
    uint8_t *node = PrimAllocQueueNode();
    if (!node) return; /* no node: the layer keeps whatever states precede it */
    const uint32_t address = P4_H2G(node);
    P4_G32(address + 8u) = p4_fn_h2g_sig(layer_states, "vpi");
    P4_G32(address + 12u) = 0;
    P4_G32(address + 16u) = 0;
    P4_G32(address + 20u) = 0;
    P4_G32(address + 24u) = 0; /* the renderer's command discriminator: a callback node */
    PrimAppendNode(P4_G2H(QUEUE(0xd7)), node);
}

void p4_ui_overlay_context_begin(int context)
{
    if (context >= 0 && context < P4_UI_CONTEXT_COUNT) shown[context] = 0;
}

static uint8_t faded(const Item *item, int context)
{
    if (!item->fade || shown[context] >= P4_UI_FADE_UPDATES) return item->alpha;
    return (uint8_t)((uint32_t)item->alpha * (uint32_t)(shown[context] + 1) / (P4_UI_FADE_UPDATES + 1));
}
int p4_ui_overlay_drawable(int context)
{
    if (context < 0 || context >= P4_UI_CONTEXT_COUNT || !counts[context]) return 0;
    for (int i = 0; i < P4_UI_OVERLAY_ITEMS; ++i)
        if (items[i].used && items[i].visible && items[i].context == context && faded(&items[i], context))
            return 1;
    return 0;
}

static float aligned(float x, float width, int align)
{
    return align == P4_UI_ALIGN_CENTER ? x - width / 2.0f : align == P4_UI_ALIGN_RIGHT ? x - width : x;
}

static void draw_text(int handle, Item *item, void *queue, int32_t base, uint8_t alpha)
{
    uint32_t text = text_arena + (uint32_t)offsetof(TextGuest, text) + (uint32_t)handle * P4_UI_OVERLAY_TEXT_MAX;
    int32_t native = base > 0 ? base : 128;
    if (!item->measured) {
        uint32_t width = text_arena + (uint32_t)offsetof(TextGuest, width) + (uint32_t)handle * 4u;
        /* Flag 1: measure only, at the unscaled native size. */
        uint32_t font = frFontExCreateText(item->x, item->y, 0.0f, (int)0xffffffffu, 8, FONT,
            P4_GPTR(char, text), 1, P4_GPTR(int32_t, width));
        if (font) frFontListRelease(font);
        int32_t scale = (int32_t)lroundf((float)native * item->scale);
        int64_t shown_width = (int64_t)texts()->width[handle] * scale / native;
        if (item->width && shown_width > item->width)
            scale = (int32_t)((int64_t)scale * item->width / shown_width);
        item->measured_scale = scale < 1 ? 1 : scale;
        item->measured = 1;
    }
    float width = (float)((int64_t)texts()->width[handle] * item->measured_scale / native);
    float left = aligned(item->x, width, item->align);
    if (item->plate)
        sflDataDrawNarrowRoundedFrame((int32_t)left - PLATE_PAD, (int32_t)item->y - 2, plate_depth,
            (int32_t)width + 2 * PLATE_PAD, PLATE_HEIGHT, 0x191919, 200 * alpha / 255, 1, 0, 0, queue,
            0.0f, 1.0f, 1.0f);
    if (item->measured_scale != base) P4_G32(FONT_SCALE) = (uint32_t)item->measured_scale;
    frFontExDrawTextDeferredWithCharWidth(left, item->y, text_depth, (int)(item->rgb << 8 | alpha), 8, FONT,
        P4_GPTR(char, text), 0, 0, queue, -1);
    if (item->measured_scale != base) P4_G32(FONT_SCALE) = (uint32_t)base;
}

typedef struct SpritePacket {
    float x, y, scale;
    uint32_t rgb, alpha;
} SpritePacket;
_Static_assert(sizeof(SpritePacket) == 20, "sprite packet must fit the queue node");

static void draw_sprite_packet(uint8_t *payload, int32_t pack)
{
    SpritePacket packet;
    memcpy(&packet, payload, sizeof packet);
    sdkSpriteDrawNow(packet.x, packet.y, sprite_depth, (int32_t)packet.rgb, (uint8_t)packet.alpha,
        0, P4_G2H((uint32_t)pack), 1, 0, 0, 0.0f, packet.scale, packet.scale);
}

static void draw_sprite(const Item *item, void *queue, uint8_t alpha)
{
    Sprite *sprite = &sprites[item->sprite];
    uint8_t *node = PrimAllocQueueNode();
    if (!node) return;
    const SpritePacket packet = {
        aligned(item->x, (float)sprite->width * item->scale, item->align),
        item->y, item->scale, item->rgb, alpha
    };
    uint32_t address = P4_H2G(node);
    P4_G32(address + 8u) = p4_fn_h2g_sig(draw_sprite_packet, "vpi");
    P4_G32(address + 12u) = 0;
    P4_G32(address + 16u) = pack_address(item->sprite);
    P4_G32(address + 20u) = 0;
    P4_G32(address + 24u) = 0;
    memcpy(node + 28, &packet, sizeof packet);
    /* Keep the pack pinned until its bucket is actually cleared, including
     * frames where the walker skipped this node. The sample is allocated only
     * inside sdkSpriteDrawNow, so a skipped callback cannot leak a sample. */
    sprite->queued = 1;
    PrimAppendNode(queue, node);
}

void p4_ui_overlay_draw(int context)
{
    if (context < 0 || context >= P4_UI_CONTEXT_COUNT || !counts[context]) return;
    void *queue = P4_G2H(queues[context]);
    int32_t base = (int32_t)P4_G32(FONT_SCALE);
    for (int i = 0; i < P4_UI_OVERLAY_ITEMS; ++i) {
        Item *item = &items[i];
        if (!item->used || !item->visible || item->context != context) continue;
        uint8_t alpha = faded(item, context);
        if (!alpha) continue;
        if (item->kind == P4_UI_ITEM_TEXT) draw_text(i, item, queue, base, alpha);
        else draw_sprite(item, queue, alpha);
    }
    if (shown[context] < P4_UI_FADE_UPDATES) ++shown[context];
}
