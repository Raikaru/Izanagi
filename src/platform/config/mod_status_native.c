/* Mod status rows (p4.ui.status). The text is validated and copied into
 * port-owned guest storage when a mod sets it; native seams only draw it.
 * Without a row nothing runs: no allocation, no guest write, no queue entry.
 *
 * Capacity: queue 0xb1 is a list of heap nodes (PrimAllocQueueNode), so the
 * bounded resources are the font manager's (frFontInitManager(0x100, 0x100),
 * 0x2713b0): 256 text runs, 1024 glyph nodes and 256 glyph rasters. A row is
 * one frame node and one deferred text run of at most 47 glyphs, so four rows
 * take at most 4 runs, 4 * 47 = 188 glyph nodes and at most one RAW raster
 * per glyph per frame, freed by the queue each frame. */
#include "p4pc/mod_status.h"
#include "p4pc/native_glyphs.h"
#include "p4pc/gmem.h"
#include "p4pc/guest.h"
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

/* Camp's centered draw table 0xb1. On the 640x448 canvas the first two rows
 * are a top band above the menu header; a third and fourth sit above the help
 * line at the bottom. A line wider than the band is narrowed to fit. */
enum { LEFT = 10, TOP = 10, BOTTOM = 300, ROW_HEIGHT = 24, PAD = 8, TEXT_WIDTH = 470 };
#define CAMP_QUEUE (0x00793e80u + 0xb1u * 0x30u)
#define FONT_SCALE 0x007645a0u

typedef struct StatusGuest {
    char text[P4_MOD_STATUS_ROWS][P4_MOD_STATUS_TEXT_MAX];
    int32_t width[P4_MOD_STATUS_ROWS];
} StatusGuest;
typedef struct StatusRow {
    uint32_t owner;
    int placement;
    int32_t scale;
    uint8_t measured;
} StatusRow;
static uint32_t guestAddress;
static StatusRow rows[P4_MOD_STATUS_ROWS];
static int count;
static int campSlotsDrawn; /* Camp rows the Camp root drew this frame */

static StatusGuest *guest(void) { return P4_GPTR(StatusGuest, guestAddress); }

static int valid_line(const char *text)
{
    if (!text) return 0;
    size_t length = strnlen(text, P4_MOD_STATUS_TEXT_MAX);
    return length && length < P4_MOD_STATUS_TEXT_MAX &&
        !p4_native_glyphs_validate((const uint8_t *)text, length + 1, P4_GLYPHS_RAW, 0);
}

int p4_mod_status_set(uint32_t owner, int placement, const char *const *lines, int lineCount)
{
    if ((placement != P4_MOD_STATUS_CAMP && placement != P4_MOD_STATUS_HUD) ||
        lineCount < 0 || lineCount > P4_MOD_STATUS_ROWS || (lineCount && !lines))
        return -1;
    for (int i = 0; i < lineCount; ++i)
        if (!valid_line(lines[i])) return -1;
    int others = 0;
    for (int i = 0; i < count; ++i) others += rows[i].owner != owner;
    if (others + lineCount > P4_MOD_STATUS_ROWS) return -1;
    if (lineCount && !guestAddress && !(guestAddress = p4_gport_alloc(sizeof(StatusGuest), 16))) return -1;
    /* Keep the other owners' rows in load order, then splice this owner's
     * lines in after every earlier-loaded owner. Validation already passed,
     * so the table changes atomically from the caller's point of view. */
    StatusRow kept[P4_MOD_STATUS_ROWS];
    char keptText[P4_MOD_STATUS_ROWS][P4_MOD_STATUS_TEXT_MAX];
    int32_t keptWidth[P4_MOD_STATUS_ROWS];
    int kept_count = 0;
    for (int i = 0; i < count; ++i) {
        if (rows[i].owner == owner) continue;
        kept[kept_count] = rows[i];
        memcpy(keptText[kept_count], guest()->text[i], P4_MOD_STATUS_TEXT_MAX);
        keptWidth[kept_count] = guest()->width[i];
        ++kept_count;
    }
    int insert = 0;
    while (insert < kept_count && kept[insert].owner < owner) ++insert;
    count = 0;
    for (int i = 0; i <= kept_count; ++i) {
        if (i == insert) {
            for (int line = 0; line < lineCount; ++line) {
                rows[count] = (StatusRow){owner, placement, 0, 0};
                memcpy(guest()->text[count], lines[line], strlen(lines[line]) + 1);
                guest()->width[count] = 0;
                ++count;
            }
        }
        if (i == kept_count) break;
        rows[count] = kept[i];
        memcpy(guest()->text[count], keptText[i], P4_MOD_STATUS_TEXT_MAX);
        guest()->width[count] = keptWidth[i];
        ++count;
    }
    return 0;
}

static void draw_row(int row, int slot, void *queue, int32_t base)
{
    StatusGuest *g = guest();
    uint32_t text = guestAddress + (uint32_t)offsetof(StatusGuest, text) +
        (uint32_t)(row * P4_MOD_STATUS_TEXT_MAX);
    int32_t y = slot < 2 ? TOP + slot * ROW_HEIGHT : BOTTOM + (slot - 2) * ROW_HEIGHT;
    if (!rows[row].measured) {
        uint32_t width = guestAddress + (uint32_t)offsetof(StatusGuest, width) + (uint32_t)row * 4u;
        /* Flag 1: measure only (frFontExCreateText sums advances for flags & 0xb). */
        uint32_t font = frFontExCreateText((float)(LEFT + PAD), (float)y, 0.0f, (int)0xffffffffu, 8, 1,
            P4_GPTR(char, text), 1, P4_GPTR(int32_t, width));
        if (font) frFontListRelease(font);
        int32_t unscaled = base > 0 ? base : 128;
        rows[row].scale = g->width[row] > TEXT_WIDTH ? (int32_t)((int64_t)unscaled * TEXT_WIDTH / g->width[row]) : unscaled;
        if (rows[row].scale < 1) rows[row].scale = 1;
        rows[row].measured = 1;
    }
    int32_t shown = g->width[row] > TEXT_WIDTH ? TEXT_WIDTH : g->width[row];
    sflDataDrawNarrowRoundedFrame(LEFT, y, 59000.0f, shown + 2 * PAD, ROW_HEIGHT - 4, 0x191919,
        200, 1, 0, 0, queue, 0.0f, 1.0f, 1.0f);
    if (rows[row].scale != base) P4_G32(FONT_SCALE) = (uint32_t)rows[row].scale;
    frFontExDrawTextDeferredWithCharWidth((float)(LEFT + PAD), (float)(y + 2), 58990.0f,
        (int)0xffffffffu, 8, 1, P4_GPTR(char, text), 0, 0, queue, -1);
    if (rows[row].scale != base) P4_G32(FONT_SCALE) = (uint32_t)base;
}

void p4_mod_status_draw(void)
{
    if (!count) return;
    void *queue = P4_G2H(CAMP_QUEUE);
    int32_t base = (int32_t)P4_G32(FONT_SCALE);
    int slot = 0;
    for (int row = 0; row < count; ++row)
        if (rows[row].placement == P4_MOD_STATUS_CAMP) draw_row(row, slot++, queue, base);
    campSlotsDrawn = slot;
}

int p4_mod_status_count(int placement)
{
    int rowsWithPlacement = 0;
    for (int row = 0; row < count; ++row) rowsWithPlacement += rows[row].placement == placement;
    return rowsWithPlacement;
}

/* HUD rows: the last ordering-table bucket, after every gameplay producer and
 * the screen fades, so the layer stays visible; below any top-band Camp rows
 * the Camp root drew this frame. The Camp frame's sprite is a Camp resource,
 * so HUD rows are plain native text at three quarters of the native size, at
 * most HUD_TEXT_WIDTH wide. */
enum { HUD_TEXT_WIDTH = 300, HUD_ROW_HEIGHT = 18 };
#define HUD_QUEUE (0x00793e80u + 0xd7u * 0x30u)

void p4_mod_status_draw_hud(void)
{
    int slot = campSlotsDrawn;
    campSlotsDrawn = 0;
    if (!count) return;
    StatusGuest *g = guest();
    void *queue = P4_G2H(HUD_QUEUE);
    int32_t base = (int32_t)P4_G32(FONT_SCALE);
    int32_t y = TOP + (slot < 2 ? slot : 2) * ROW_HEIGHT;
    for (int row = 0; row < count; ++row) {
        if (rows[row].placement != P4_MOD_STATUS_HUD) continue;
        uint32_t text = guestAddress + (uint32_t)offsetof(StatusGuest, text) +
            (uint32_t)(row * P4_MOD_STATUS_TEXT_MAX);
        if (!rows[row].measured) {
            uint32_t width = guestAddress + (uint32_t)offsetof(StatusGuest, width) + (uint32_t)row * 4u;
            /* Flag 1: measure only, as for Camp rows. */
            uint32_t font = frFontExCreateText((float)LEFT, (float)y, 0.0f, (int)0xffffffffu, 8, 1,
                P4_GPTR(char, text), 1, P4_GPTR(int32_t, width));
            if (font) frFontListRelease(font);
            int32_t unscaled = base > 0 ? base : 128;
            int32_t scale = unscaled * 3 / 4;
            int32_t scaled = (int32_t)((int64_t)g->width[row] * 3 / 4);
            if (scaled > HUD_TEXT_WIDTH) scale = (int32_t)((int64_t)unscaled * HUD_TEXT_WIDTH / g->width[row]);
            rows[row].scale = scale < 1 ? 1 : scale;
            rows[row].measured = 1;
        }
        P4_G32(FONT_SCALE) = (uint32_t)rows[row].scale;
        frFontExDrawTextDeferredWithCharWidth((float)LEFT, (float)y, 58990.0f,
            (int)0xffffffffu, 8, 1, P4_GPTR(char, text), 0, 0, queue, -1);
        P4_G32(FONT_SCALE) = (uint32_t)base;
        y += HUD_ROW_HEIGHT;
    }
}
