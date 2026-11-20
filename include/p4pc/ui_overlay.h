/* Mod overlays (p4.ui.sprite / p4.ui.overlay): authored sprites and positioned
 * native text, drawn only through the game's own ordering-table queues while a
 * named native context draws. With nothing registered nothing is drawn,
 * allocated or queued. Game thread only.
 *
 * Sprites are synthetic one-record SdkSpritePack images in port-owned guest
 * memory whose raster is a pinned P4Resources image. A native queue callback
 * invokes sdkSpriteDrawNow (0025ea20 -> 0046b380) with tint, alpha, scale and
 * depth. Retail packs and records are never
 * touched. Text uses the status rows' native font calls and the same per-frame
 * font budget (docs/modding/lua.md, "Overlays"). */
#ifndef P4PC_UI_OVERLAY_H
#define P4PC_UI_OVERLAY_H
#include <stdint.h>

#define P4_UI_SPRITES 32            /* synthetic sprite records, all mods */
#define P4_UI_OVERLAY_ITEMS 16      /* overlay items, all mods */
#define P4_UI_OVERLAY_TEXT_MAX 48   /* RAW glyph bytes including NUL */
#define P4_UI_OVERLAY_RUNS 4        /* text items per context, all mods */
#define P4_UI_FADE_UPDATES 10       /* native menu fade length (fclSpriteStartAlphaAt 0xA) */

/* SCREEN: every presented frame, never hidden and never faded (status-layer
 * markers that must stay visible over menus, events and battles). */
enum { P4_UI_CONTEXT_CAMP = 0, P4_UI_CONTEXT_HUD = 1, P4_UI_CONTEXT_SCREEN = 2, P4_UI_CONTEXT_COUNT = 3 };
enum { P4_UI_ALIGN_LEFT = 0, P4_UI_ALIGN_CENTER = 1, P4_UI_ALIGN_RIGHT = 2 };
enum { P4_UI_ITEM_TEXT = 0, P4_UI_ITEM_SPRITE = 1 };
enum {
    P4_UI_OK = 0, P4_UI_INVALID = -1, P4_UI_FULL = -2, P4_UI_NOT_FOUND = -3,
    P4_UI_NOT_POT = -4, P4_UI_BUSY = -5
};

typedef struct P4UiOverlayItem {
    int kind;          /* P4_UI_ITEM_* */
    int context;       /* P4_UI_CONTEXT_* */
    float x, y;        /* 640x448 canvas; text x is the aligned edge, y its top */
    int align;         /* P4_UI_ALIGN_* (text and sprites) */
    uint32_t rgb;      /* 0xRRGGBB: text colour, or sprite tint (0xFFFFFF = none) */
    uint8_t alpha;     /* 0..255 */
    float scale;       /* text glyph scale or sprite scale; 1 = native size */
    int plate;         /* text: the status rows' native rounded plate behind it */
    int width;         /* text: narrow a wider line to this canvas width (0 = none) */
    int fade;          /* fade in over P4_UI_FADE_UPDATES when the context appears */
    int sprite;        /* P4_UI_ITEM_SPRITE: handle from p4_ui_sprite_create */
    const char *text;  /* P4_UI_ITEM_TEXT: RAW glyphs, validated and copied */
} P4UiOverlayItem;

/* A sprite for the (left, top, width, height) rectangle of a frozen P4Resources
 * image whose texture is exactly its power-of-two size (an atlas, as retail
 * sprite packs are). Pins the token until owner release and, if queued, the
 * subsequent ordering-table clear. Retired slots count against P4_UI_SPRITES.
 * Returns a handle >= 0 or a P4_UI_* error. */
int p4_ui_sprite_create(uint32_t owner, uint32_t token, int left, int top, int width, int height);
/* The pinned raster (guest RwRaster) and texel rectangle {left, top, width,
 * height} of a live sprite handle, for native consumers that build their own
 * records (native_rows.h); 0 on success. */
int p4_ui_sprite_rect(int handle, uint32_t *raster, int rect[4]);
/* Returns an item handle >= 0 or a P4_UI_* error; nothing changes on error. */
int p4_ui_overlay_add(uint32_t owner, const P4UiOverlayItem *item);
int p4_ui_overlay_set_text(uint32_t owner, int handle, const char *text);
int p4_ui_overlay_set_visible(uint32_t owner, int handle, int visible);
int p4_ui_overlay_remove(uint32_t owner, int handle);
/* Removes every item and sprite of owner. Queued packs and their resource pins
 * remain intact until the main ordering table has been cleared. */
void p4_ui_overlay_release_owner(uint32_t owner);
/* Called only after clearing the entire main ordering table on the game thread. */
void p4_ui_overlay_queue_cleared(void);
int p4_ui_overlay_count(int context);
/* 1 when the context's next draw issues at least one item (visible, nonzero
 * faded alpha), so a layer opens only for frames that draw. */
int p4_ui_overlay_drawable(int context);
/* The native context restarted (the Camp root opened): fades run again. */
void p4_ui_overlay_context_begin(int context);
/* Draw seams: the Camp root's idle state (cmpMainTaskStep, beside the status
 * rows) and the HUD layer (mainDrawFrameUpdate, last ordering bucket). */
void p4_ui_overlay_draw(int context);
/* Opens the HUD layer (status rows, HUD and screen items) in the last ordering
 * bucket: a queued node that clears the depth buffer, so a nearer depth written
 * earlier in the frame (the battle command phase's Z-only TV outline) cannot
 * hide the layer, whose text and plates draw with the depth test on. Nothing
 * draws after the last bucket. */
void p4_ui_overlay_layer_begin(void);
#endif
