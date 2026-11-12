#include "p4pc/settings_native.h"
#include "p4pc/settings_pages.h"
#include "p4pc/settings.h"
#include "p4pc/native_glyphs.h"
#include "p4pc/mod_settings.h"
#include "p4pc/gmem.h"
#include "p4cfg.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Canonical definitions, not the integer/pointer guesses at some m2c callsites. */
extern unsigned char *resourceFindByName(char *name);
extern unsigned char *sdkSpriteNewNode(uint32_t pack, uint32_t image);
extern void sdkSpriteFreeNode(void *node);
extern void sdkSpriteDrawAt(void *sprite, float x, float y,
    uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);
extern void radarAnimateValue(unsigned char *motion, int32_t frame, int32_t mode);
extern void sflDataDrawNarrowRoundedFrame(int32_t x, int32_t y, float depth,
    int32_t width, int32_t height, int32_t rgb, int32_t alpha, int32_t mode,
    int32_t pivotX, int16_t pivotY, void *queue, float angle, float scaleX, float scaleY);
extern int frFontExDrawTextDeferredWithCharWidth(float x, float y, float depth,
    int color, signed char mode, int font, const char *text, int flags,
    int unused, void *queue, int spacing);
extern uint32_t frFontExCreateText(float x, float y, float depth, int color,
    signed char palette, int font, const char *text, int flags, int32_t *width);
extern void frFontExFreeText(int32_t unused, uint32_t font);
extern uint32_t frFontListRelease(uint32_t font);
extern int32_t nLineSelectSet(int32_t set);
extern void nLineSetActive(int32_t active);
enum { NATIVE_INFO_TEXT_WIDTH = 360 };

/* Same 48-byte record consumed by radarAnimateValue/001437b0. System's scrollbar
 * uses startY +/-10, targetY 0 and frames 0..3 (00361970, 0035f5b0). */
typedef struct NativeMotion {
    float start[2], target[2], position[2];
    uint8_t opacity[4];
    uint16_t width[3], height[3];
    int32_t startFrame, endFrame;
} NativeMotion;
/* frFontComputeGlyphMetrics reads these live inputs; CreateTextList uses slot 1.
 * The slot's header/metrics/object columns are at 881630 + 32, not host pointers. */
typedef struct NativeFontKey {
    uint32_t scale, advance, height, binding, header, metrics, count;
} NativeFontKey;
typedef struct NativeTextCache {
    char text[P4_SETTINGS_PAGE_HELP_MAX];
    NativeFontKey font;
    int32_t scale, maximumWidth;
    uint8_t measured;
} NativeTextCache;
typedef struct NativeStorage {
    P4SettingsNativeState state;
    uint16_t pcTimers[P4_SETTINGS_MODEL_BUILTINS + P4_SETTINGS_EXTENSION_MAX];
    int32_t mapping[2][P4_SETTINGS_MODEL_BUILTINS + P4_SETTINGS_EXTENSION_MAX];
    NativeMotion scroll;
    int32_t scrollFrame, previousScroll;
    int32_t textWidth;
    char labels[P4_SETTINGS_NATIVE_VISIBLE][P4_SETTINGS_PAGE_LABEL_MAX];
    char values[P4_SETTINGS_NATIVE_VISIBLE][P4_SETTINGS_PAGE_VALUE_MAX];
    char help[P4_SETTINGS_NATIVE_VISIBLE][P4_SETTINGS_PAGE_HELP_MAX + 16];
    char keys[P4_SETTINGS_NATIVE_VISIBLE][P4_SETTINGS_PAGE_KEY_MAX];
    char targets[P4_SETTINGS_NATIVE_VISIBLE][P4_SETTINGS_PAGE_KEY_MAX];
    uint16_t visibleTimers[P4_SETTINGS_NATIVE_VISIBLE];
    char title[P4_SETTINGS_PAGE_LABEL_MAX];
    char detail[P4_SETTINGS_PAGE_DETAIL_MAX];
    uint16_t detailLines[P4_SETTINGS_PAGE_DETAIL_MAX];
    int32_t detailCount;
    char helpBody[P4_SETTINGS_PAGE_HELP_MAX + 16];
    uint16_t helpLines[P4_SETTINGS_PAGE_HELP_MAX + 16];
    int32_t helpCount, helpRestart;
    NativeFontKey helpFont, detailFont;
    char helpLine[P4_SETTINGS_PAGE_HELP_MAX];
    NativeTextCache textCache[2][P4_SETTINGS_NATIVE_VISIBLE], titleCache, helpCache;
} NativeStorage;
_Static_assert(sizeof(NativeMotion) == 48, "retail motion ABI");
_Static_assert(sizeof(P4SettingsNativeRow) == 36, "native row guest ABI");

static uint32_t storageAddress, owner, routeParent, routeWork, qualificationWork;
static int request, routeStage;
static uint32_t inputFrame, requestFrame, touchFrame;
static uint32_t titleReadyFrame;
static int titleReadySeen;
static int retailOnly = -1;

/* Registry and callbacks are host-only. The guest sidecar contains snapshots,
 * never borrowed descriptor pointers. Virtual catalogs cost seven rows, not an
 * allocation or mapping entry per item. */
typedef struct PageSlot { P4SettingsPage page; int registered; } PageSlot;
typedef struct PageFrame {
    int view, cursor, scroll, count, queried, opened, model;
    uint64_t revision;
    char key[P4_SETTINGS_PAGE_KEY_MAX];
} PageFrame;
static PageSlot pages[P4_SETTINGS_PAGE_MAX];
static PageFrame history[P4_SETTINGS_PAGE_DEPTH_MAX];
static int historyCount, callbackDepth, requestedView = -1, requestedRouted;
/* One registered flow hosted by a game parent's own cmpConfigCreate child. */
static uint32_t hostWork;
static int hostView, hosted;
static int pageFeedback = -1;
static int pageRefresh(NativeStorage *s, int selected);
static void pageDescribe(NativeStorage *s, int slot, int logical);
static void closeFrames(int first);
static int pageError(const char *message);
static int pageSlot(const char *key);
static int validText(const char *text, size_t capacity);
static int copyText(char *to, size_t capacity, const char *from);
static int fontLines(char *text, size_t capacity);

static NativeStorage *storage(void) { return P4_GPTR(NativeStorage, storageAddress); }
static void fail(const char *what)
{
    fprintf(stderr, "p4pc native Config: %s\n", what);
    exit(1);
}
static void fontKey(NativeFontKey *key)
{
    key->scale = P4_G32(0x007645a0u);
    key->advance = P4_G32(0x007645a8u);
    key->height = P4_G8(0x00763809u);
    key->binding = P4_G32(0x0088166cu);
    key->header = P4_G32(0x00881654u);
    key->metrics = P4_G32(0x00881660u);
    key->count = P4_G32(0x00881658u);
}
static void drawMeasured(const char *text, NativeTextCache *cache, int maximumWidth,
    int palette, int flags, float x, float y, uint32_t color)
{
    if (!text[0]) return;
    NativeStorage *s = storage();
    NativeFontKey key;
    fontKey(&key);
    int32_t previousScale = (int32_t)key.scale;
    uint32_t font = 0;
    if (!cache->measured || cache->maximumWidth != maximumWidth ||
        memcmp(&cache->font, &key, sizeof key) || strcmp(cache->text, text)) {
        font = frFontExCreateText(x, y, 0.0f, (int)color, (signed char)palette, 1,
            text, flags, &s->textWidth);
        if (!font) fail("required retail settings font is not loaded");
        cache->scale = previousScale;
        if (s->textWidth > maximumWidth) {
            frFontListRelease(font);
            font = 0;
            /* Native <=0 means unscaled metrics (128), not MSG's 123 fallback. */
            int32_t base = previousScale > 0 ? previousScale : 128;
            int32_t fitted = (int32_t)((int64_t)base * maximumWidth / s->textWidth);
            cache->scale = fitted > 0 ? fitted : 1;
        }
        memcpy(cache->text, text, strlen(text) + 1);
        cache->font = key;
        cache->maximumWidth = maximumWidth;
        cache->measured = 1;
    }
    if (!font) {
        if (cache->scale != previousScale) P4_G32(0x007645a0u) = (uint32_t)cache->scale;
        font = frFontExCreateText(x, y, 0.0f, (int)color, (signed char)palette, 1, text, flags, NULL);
        if (cache->scale != previousScale) P4_G32(0x007645a0u) = (uint32_t)previousScale;
        if (!font) fail("required retail bounded settings font is not loaded");
    }
    frFontExFreeText(0, font);
}
static void drawBoundedText(float x, float y, uint32_t color, int row, int label)
{
    NativeStorage *s = storage();
    int info = label && s->state.rows[row].kind == P4_SETTINGS_NATIVE_INFO;
    const char *text = label ? P4_GPTR(char, s->state.rows[row].label) : s->values[row];
    drawMeasured(text, &s->textCache[label][row], info ? NATIVE_INFO_TEXT_WIDTH : label ? 226 : 96,
        8, info ? 1 : label ? 2 : 8, info ? x - 226.0f : x, y, color);
}
static int textWidth(char *text, size_t end, int palette)
{
    if (!end) return 0;
    char saved = text[end];
    text[end] = '\0';
    int32_t width = p4_native_glyphs_measure_raw((const uint8_t *)text, 1, (uint8_t)palette);
    text[end] = saved;
    if (width < 0) fail("required retail page font is not loaded");
    return width / 16;
}
/* Longest measured prefix, with whitespace breaks and game-font byte pairs.
 * Binary search avoids laying out every growing prefix of a long paragraph. */
static int wrapText(char *text, uint16_t *lines, int width, int palette, size_t lineCapacity)
{
    size_t start = 0;
    int count = 0;
    while (text[start]) {
        uint16_t ends[P4_SETTINGS_PAGE_HELP_MAX];
        size_t end = start;
        int n = 0, lo = 0;
        lines[count++] = (uint16_t)start;
        while (text[end] && text[end] != '\n' && end - start < lineCapacity - 1) {
            size_t step = (unsigned char)text[end] >= 0x80 ? 2u : 1u;
            if (end - start + step >= lineCapacity) break;
            end += step;
            ends[n++] = (uint16_t)(end - start);
        }
        if (n) {
            int hi = n;
            while (lo < hi) {
                int middle = lo + (hi - lo + 1) / 2;
                if (textWidth(text + start, ends[middle - 1], palette) <= width) lo = middle;
                else hi = middle - 1;
            }
            if (!lo) lo = 1; /* one complete glyph; never discard a byte */
            end = start + ends[lo - 1];
            if (lo < n || (text[end] && text[end] != '\n')) {
                size_t space = start;
                for (size_t p = start; p < end;) {
                    size_t step = (unsigned char)text[p] >= 0x80 ? 2u : 1u;
                    if (text[p] == ' ' ||
                        ((unsigned char)text[p] == 0x83 && (unsigned char)text[p + 1] == 0xf6))
                        space = p + step;
                    p += step;
                }
                if (space > start) end = space;
            }
        }
        if (text[end] == '\n') ++end;
        start = end;
    }
    lines[count] = (uint16_t)start;
    return count;
}
static void copyLine(char *out, const char *text, const uint16_t *lines, int line)
{
    size_t start = lines[line], end = lines[line + 1];
    while (end > start) {
        if (text[end - 1] == '\n' || text[end - 1] == ' ') --end;
        else if (end - start >= 2 && (unsigned char)text[end - 2] == 0x83 &&
                 (unsigned char)text[end - 1] == 0xf6) end -= 2;
        else break;
    }
    memcpy(out, text + start, end - start);
    out[end - start] = '\0';
}
void p4_settings_native_draw_value(float x, float y, uint32_t color, int row)
{
    drawBoundedText(x, y, color, row, 0);
}
void p4_settings_native_draw_label(float x, float y, uint32_t color, int row)
{
    drawBoundedText(x, y, color, row, 1);
}
void p4_settings_native_draw_info_bar(void *sprite, float x, float y, uint32_t color)
{
    /* The stock selected asset is a 7-pixel quad plus a 240-pixel rightBorder.
     * sdkSpriteDraw/0046b380 scales the quad, not that asset-record strip.
     * Widen 247 to 380, then restore the shared ISO record after drawing. */
    const uint32_t *sample = (const uint32_t *)sprite;
    uint32_t pack = sample[0];
    uint32_t record = P4_G32(pack + 0x204u) + sample[1] * 0x80u;
    uint32_t border = record + 0x40u;
    uint32_t saved = P4_G32(border);
    P4_G32(border) = saved + 133u;
    sdkSpriteDrawAt(sprite, x, y,
        (uint8_t)(color >> 24), (uint8_t)(color >> 16),
        (uint8_t)(color >> 8), (uint8_t)color);
    P4_G32(border) = saved;
}
void p4_settings_native_draw_title(float x, float y, uint32_t color)
{
    NativeStorage *s = storage();
    drawMeasured(s->title, &s->titleCache, 440, 8, 1, x, y, color);
}
void p4_settings_native_draw_help(float x, float y, uint32_t color, int row)
{
    NativeStorage *s = storage();
    const char *help = s->help[row];
    const char *notice = p4_settings_model_notice();
    if (s->state.view >= 2 && notice) help = notice;
    int restart = s->state.rows[row].restart;
    NativeFontKey key;
    fontKey(&key);
    if (strcmp(help, s->helpBody) || s->helpRestart != restart ||
        memcmp(&s->helpFont, &key, sizeof key)) {
        s->helpRestart = restart;
        s->helpFont = key;
        if (copyText(s->helpBody, sizeof s->helpBody, help) < 0 ||
            !fontLines(s->helpBody, sizeof s->helpBody))
            fail("invalid native glyphs in settings help");
        s->helpCount = wrapText(s->helpBody, s->helpLines, restart ? 450 : 530, 6,
            sizeof s->helpLine - 64);
        s->state.helpPage = 0;
    }
    s->state.helpPages = s->helpCount;
    if (!s->helpCount) {
        if (restart)
            drawMeasured("Applies on restart.", &s->helpCache, 592, 6, 2, x, y, color);
        return;
    }
    if (s->state.helpPage >= s->helpCount) s->state.helpPage = 0;
    copyLine(s->helpLine, s->helpBody, s->helpLines, s->state.helpPage);
    if (restart) strcat(s->helpLine, " (restart)");
    if (s->helpCount > 1) {
        size_t end = strlen(s->helpLine);
        snprintf(s->helpLine + end, sizeof s->helpLine - end, "  (%d/%d)",
            s->state.helpPage + 1, s->helpCount);
    }
    drawMeasured(s->helpLine, &s->helpCache, 592, 6, 2, x, y, color);
}
void p4_settings_native_help_step(int direction)
{
    if (!owner) return;
    P4SettingsNativeState *s = &storage()->state;
    if (s->helpPages > 1) {
        s->helpPage = (s->helpPage + (direction < 0 ? -1 : 1) + s->helpPages) % s->helpPages;
        pageFeedback = 0;
    }
}
static int rowCount(NativeStorage *s, int view)
{
    int n = 0;
    if (view == P4_SETTINGS_NATIVE_ROOT) {
        for (int i = 0; i < 6; ++i) s->mapping[view][n++] = -4 - i;
        for (int i = 0; i < 9; ++i)
            if (p4_settings_entry_available(i)) s->mapping[view][n++] = i;
        s->mapping[view][n++] = -2; /* Controls */
        for (int i = P4_SETTINGS_MODEL_BUILTINS; i < p4_settings_entry_count(); ++i)
            if (p4_settings_entry_available(i)) s->mapping[view][n++] = i;
        s->mapping[view][n++] = -3; /* retail CONFIRM is always last */
    } else {
        for (int i = P4_SETTINGS_MODEL_BINDINGS_FIRST; i <= P4_SETTINGS_MODEL_RESET_BINDINGS; ++i)
            if (p4_settings_entry_available(i)) s->mapping[view][n++] = i;
    }
    return n;
}
static void uppercase(char *text)
{
    for (; *text; ++text) {
        if ((unsigned char)*text >= 0x80 && text[1]) ++text;
        else if (*text >= 'a' && *text <= 'z') *text -= 'a' - 'A';
    }
}

void p4_settings_native_request(void)
{
    if (!owner) {
        request = 1;
        requestFrame = inputFrame;
    }
}
int p4_settings_native_field_request(uint32_t task, int allowed)
{
    /* A fresh retail trigger probe supersedes an abandoned probe of this task.
     * An event/battle never calls this entry: frame_end discards that request. */
    if (routeParent == task) routeParent = 0;
    if (!request) return 0;
    request = 0;
    if (!allowed) { requestedView = -1; return 0; }
    requestedRouted = requestedView >= 0;
    routeParent = task;
    return 1;
}
void p4_settings_native_camp_created(uint32_t parent, uint32_t work)
{
    routeWork = routeParent && routeParent == parent ? work : 0;
    routeParent = 0;
    routeStage = 0;
}
int p4_settings_native_camp_route(uint32_t work)
{
    /* This entry runs only at the native Camp title's interactive phase. */
    if (request && !routeWork) {
        request = 0;
        routeWork = work;
        routeStage = 0;
    }
    if (!routeWork || routeWork != work || routeStage != 0) return 0;
    routeStage = 1;
    return 1;
}
int p4_settings_native_system_route(uint32_t work, int allowed)
{
    /* Already inside System: use its ordinary Config selection, not a new Camp. */
    if (request && !routeWork) {
        request = 0;
        if (!allowed) return 0;
        routeWork = work - 0x9000u;
        routeStage = 1;
    }
    if (!routeWork || work != routeWork + 0x9000u || routeStage != 1) return 0;
    if (!allowed) {
        routeWork = 0;
        return 0;
    }
    routeStage = 2;
    return 1;
}
int p4_settings_native_title_request(void)
{
    titleReadyFrame = inputFrame;
    titleReadySeen = 1;
    int pending = request;
    request = 0;
    if (pending && requestedView >= 0) requestedRouted = 1;
    return pending;
}
int p4_settings_native_title_ready(void)
{
    return titleReadySeen && (uint32_t)(inputFrame - titleReadyFrame) <= 1u;
}
int p4_settings_native_active(void) { return owner != 0; }

void p4_settings_native_begin(uint32_t work, int mode)
{
    static const uint32_t images[] = {0x36, 0x34, 0x35};
    if (retailOnly < 0) {
        const char *value = getenv("P4PC_SETTINGS_RETAIL_ONLY");
        retailOnly = value && !strcmp(value, "1");
    }
    if (retailOnly) {
        if (hostWork == work) fail("a hosted Config flow cannot run in retail-only mode");
        qualificationWork = work; /* host-only observation, no guest writes */
        return;
    }
    if (p4_mod_settings_init()) fail("native MODS hub registration failed");
    if (owner) fail("two Config tasks attempted to own the settings sidecar");
    if (!storageAddress) {
        storageAddress = p4_gport_alloc(sizeof(NativeStorage), 16);
        if (!storageAddress) fail("port-owned guest storage exhausted");
    }
    NativeStorage *s = storage();
    memset(s, 0, sizeof *s);
    s->state.touchRow = -1;
    s->state.jump = -1;
    s->state.count = rowCount(s, P4_SETTINGS_NATIVE_ROOT);
    for (unsigned i = 0; i < sizeof s->pcTimers / sizeof s->pcTimers[0]; ++i)
        s->pcTimers[i] = 5;
    s->scroll.endFrame = s->scrollFrame = 3;
    hosted = hostWork != 0 && hostWork == work;
    if (mode == 0 && routeWork && routeStage == 2 && !hosted)
        s->state.flags = P4_SETTINGS_NATIVE_SHORTCUT;
    routeWork = 0;
    routeStage = 0;
    /* D_005e5830 is init/c_equip_01.spr. These are exactly System's three
     * scrollbar nodes (D_0064e240[37..39]); never decode or extract an asset. */
    unsigned char *pack = resourceFindByName(P4_GPTR(char, 0x005e5830u));
    if (!pack) fail("required init/c_equip_01.spr is not loaded");
    for (unsigned i = 0; i < 3; ++i) {
        unsigned char *node = sdkSpriteNewNode(P4_H2G(pack), images[i]);
        if (!node) fail("required retail scrollbar sprite could not be allocated");
        s->state.scrollbar[i] = P4_H2G(node);
    }
    for (unsigned i = 0; i < 2; ++i) {
        /* The game's actual 16x16 stat arrow, rotated by its own sprite SDK. */
        unsigned char *node = sdkSpriteNewNode(P4_H2G(pack), 0x28);
        if (!node) fail("required retail choice arrow could not be allocated");
        s->state.choiceArrows[i] = P4_H2G(node);
    }
    owner = work;
    p4_settings_cursor_begin();
    memset(history, 0, sizeof history);
    historyCount = 1;
    history[0].view = P4_SETTINGS_NATIVE_ROOT;
    pageFeedback = -1;
    p4_settings_model_reset();
    if (hosted) {
        /* The hidden root frame is never presented; leaving the page closes. */
        hostWork = 0;
        s->state.flags = P4_SETTINGS_NATIVE_HOSTED;
        p4_settings_native_view(work, hostView, 0);
        if (historyCount != 2) s->state.navigation = P4_SETTINGS_NATIVE_CLOSE;
    } else if (requestedView >= 2) {
        int view = requestedView;
        requestedView = -1;
        p4_settings_native_view(work, view, 0);
    }
    requestedRouted = 0;
}
int p4_settings_native_host(uint32_t task, const char *key)
{
    int slot = pageSlot(key);
    if (slot < 0) return pageError("Page is not registered.");
    if (retailOnly < 0) {
        const char *value = getenv("P4PC_SETTINGS_RETAIL_ONLY");
        retailOnly = value && !strcmp(value, "1");
    }
    if (retailOnly || qualificationWork) return pageError("Pages are disabled in retail-only mode.");
    if (!task || owner || hostWork || requestedView >= 0 || request || routeParent || routeWork)
        return pageError("Config is busy and cannot be hosted.");
    uint32_t work = P4_G32(task + 0x38u);
    if (!work) return pageError("Hosted Config task has no work block.");
    /* Mode 0 waits on Camp's nLine forever outside Camp; mode 1 drives its own. */
    if (P4_G32(work + 0x1Cu) != 1) return pageError("Hosted Config requires title mode 1.");
    hostWork = work;
    hostView = slot + 2;
    return 0;
}
void p4_settings_native_end(uint32_t work)
{
    if (qualificationWork == work) qualificationWork = 0;
    if (hostWork == work) hostWork = 0; /* destroyed before its first update */
    if (owner != work) return;
    int wasHosted = hosted;
    hosted = 0;
    closeFrames(0);
    requestedView = -1;
    NativeStorage *s = storage();
    for (unsigned i = 0; i < 3; ++i) {
        if (s->state.scrollbar[i]) sdkSpriteFreeNode(P4_G2H(s->state.scrollbar[i]));
        s->state.scrollbar[i] = 0;
    }
    for (unsigned i = 0; i < 2; ++i) {
        if (s->state.choiceArrows[i]) sdkSpriteFreeNode(P4_G2H(s->state.choiceArrows[i]));
        s->state.choiceArrows[i] = 0;
    }
    p4_settings_cursor_end();
    if (wasHosted) {
        /* Title mode set nLine active at creation; a hosted parent runs outside
         * Camp/title with it off, as cmpMainTaskDestroy leaves it. */
        nLineSelectSet(0);
        nLineSetActive(0);
    }
    owner = 0;
    p4_settings_model_reset();
}
uint32_t p4_settings_native_retail_work(void) { return qualificationWork; }

static void describeRow(NativeStorage *s, int slot, int logical)
{
    P4SettingsNativeRow *row = &s->state.rows[slot];
    memset(row, 0, sizeof *row);
    row->index = -1;
    if (logical < 0 || logical >= s->state.count) {
        row->kind = P4_SETTINGS_NATIVE_EMPTY;
        return;
    }
    int model = s->mapping[s->state.view][logical];
    if (s->state.view == P4_SETTINGS_NATIVE_CONTROL_LIST) {
        row->index = model;
        row->kind = model == P4_SETTINGS_MODEL_RESET_BINDINGS
            ? P4_SETTINGS_NATIVE_ACTION : P4_SETTINGS_NATIVE_BINDING;
    } else if (logical < 6) {
        row->kind = P4_SETTINGS_NATIVE_RETAIL;
        row->index = logical;
        row->label = P4_G32(0x0064d230u + (uint32_t)logical * 4u);
        row->help = P4_G32(0x0064d380u + (uint32_t)logical * 4u);
        row->boolean = (int32_t)P4_G32(owner + 0x48u + (uint32_t)logical * 4u);
        row->timer = owner + 0x3au + (uint32_t)logical * 2u;
        return;
    } else if (model == -2) {
        row->kind = P4_SETTINGS_NATIVE_CONTROLS;
        row->label = p4_gport_str("CONTROLS");
        row->help = p4_gport_str("Change control assignments.");
        return;
    } else if (model == -3) {
        row->kind = P4_SETTINGS_NATIVE_CONFIRM;
        row->label = P4_G32(0x0064d230u + 6u * 4u);
        row->help = P4_G32(0x0064d380u + 6u * 4u);
        return;
    } else {
        row->index = model;
        unsigned flags = p4_settings_entry_flags(model);
        row->kind = flags & P4_SETTINGS_ACTION ? P4_SETTINGS_NATIVE_ACTION
            : flags & P4_SETTINGS_TOGGLE ? P4_SETTINGS_NATIVE_TOGGLE : P4_SETTINGS_NATIVE_VALUE;
    }
    row->label = P4_H2G(s->labels[slot]);
    row->value = P4_H2G(s->values[slot]);
    row->help = P4_H2G(s->help[slot]);
    int restart = p4_settings_entry(row->index, s->labels[slot], sizeof s->labels[slot],
        s->values[slot], sizeof s->values[slot]);
    if (restart < 0) fail("registered settings row index is invalid");
    uppercase(s->labels[slot]);
    if (row->kind == P4_SETTINGS_NATIVE_TOGGLE) {
        row->timer = P4_H2G(&s->pcTimers[row->index]);
        row->boolean = !strcmp(s->values[slot], "On") || !strcmp(s->values[slot], "ON");
    }
    uppercase(s->values[slot]);
    row->restart = restart;
    if (row->kind == P4_SETTINGS_NATIVE_VALUE && s->values[slot][0] == '\0')
        row->kind = P4_SETTINGS_NATIVE_ACTION;
    const char *help = p4_settings_model_description(row->index);
    if (copyText(s->help[slot], sizeof s->help[slot], help) < 0)
        fail("registered settings help exceeds its bound");
    if (row->kind == P4_SETTINGS_NATIVE_BINDING && !p4_settings_model_capturing()) {
        size_t length = strlen(s->values[slot]);
        if (length > 12 && length <= 30)
            snprintf(s->help[slot], sizeof s->help[slot], "Binding: %s", s->values[slot]);
    }
}
uint32_t p4_settings_native_state(uint32_t work, int refresh)
{
    if (!owner || owner != work) return 0;
    uint32_t address = storageAddress + (uint32_t)offsetof(NativeStorage, state);
    NativeStorage *s = storage();
    if (!refresh || s->state.navigation) return address;
    int selected = (int32_t)P4_G32(work + 0x30u);
    if (s->state.view >= 2) {
        if (pageRefresh(s, selected) < 0) return address;
    } else {
        int logical = s->state.scroll + selected;
        int previousCount = s->state.count;
        int model = logical >= 0 && logical < previousCount
            ? s->mapping[s->state.view][logical] : INT32_MIN;
        s->state.count = rowCount(s, s->state.view);
        if (s->state.count != previousCount) {
            for (int i = 0; i < s->state.count; ++i) {
                if (s->mapping[s->state.view][i] == model) {
                    logical = i;
                    break;
                }
            }
            if (logical < 0) logical = 0;
            if (logical >= s->state.count) logical = s->state.count - 1;
            s->state.scroll = logical - selected;
            if (s->state.scroll < 0) s->state.scroll = 0;
            int maximum = s->state.count > 7 ? s->state.count - 7 : 0;
            if (s->state.scroll > maximum) s->state.scroll = maximum;
            s->state.touchRow = logical - s->state.scroll;
            touchFrame = inputFrame;
            s->state.activate = s->state.step = 0;
            if (p4_settings_model_capturing()) p4_settings_model_cancel_capture();
        }
    }
    int maximum = s->state.count > 7 ? s->state.count - 7 : 0;
    if (s->state.scroll > maximum) s->state.scroll = maximum;
    if (s->state.scroll != s->previousScroll) {
        s->scroll.start[1] = s->state.scroll > s->previousScroll ? -10.0f : 10.0f;
        s->scrollFrame = 0;
        s->previousScroll = s->state.scroll;
    }
    if (s->scrollFrame < 3) {
        radarAnimateValue((unsigned char *)&s->scroll, ++s->scrollFrame, 0);
        s->state.scrollOffset = s->scroll.position[1];
    }
    for (int i = 0; i < P4_SETTINGS_NATIVE_VISIBLE; ++i) {
        if (s->state.view >= 2) pageDescribe(s, i, s->state.scroll + i);
        else describeRow(s, i, s->state.scroll + i);
        if (s->state.navigation) break;
    }
    return address;
}
void p4_settings_native_view(uint32_t work, int view, int cursor)
{
    if (owner != work || !historyCount) return;
    NativeStorage *s = storage();
    s->state.navigation = P4_SETTINGS_NATIVE_SWITCH;
    if (view >= 0) {
        if (historyCount == P4_SETTINGS_PAGE_DEPTH_MAX ||
            (view >= 2 && (view - 2 >= P4_SETTINGS_PAGE_MAX || !pages[view - 2].registered))) {
            pageError("Page is unavailable or stack is full.");
            view = P4_SETTINGS_NATIVE_RESTORE;
        } else {
            PageFrame *parent = &history[historyCount - 1];
            parent->cursor = s->state.scroll + cursor;
            parent->scroll = s->state.scroll;
            if (parent->view < 2)
                parent->model = s->mapping[parent->view][parent->cursor];
            if (parent->view >= 2 && cursor >= 0 && cursor < 7)
                copyText(parent->key, sizeof parent->key, s->keys[cursor]);
            PageFrame *next = &history[historyCount++];
            memset(next, 0, sizeof *next);
            next->view = view;
            if (view >= 2) {
                P4SettingsPage *page = &pages[view - 2].page;
                int result = 0;
                if (page->opened) {
                    ++callbackDepth;
                    result = page->opened(page->userdata);
                    --callbackDepth;
                }
                if (result < 0) {
                    --historyCount;
                    pageFeedback = 8;
                    view = P4_SETTINGS_NATIVE_RESTORE;
                } else next->opened = 1;
            }
        }
    } else if (view == P4_SETTINGS_NATIVE_BACK && historyCount > 1) {
        closeFrames(historyCount - 1);
    }
    if (hosted)
        s->state.flags = P4_SETTINGS_NATIVE_HOSTED | (historyCount == 2 ? P4_SETTINGS_NATIVE_HOSTED_TOP : 0);
    PageFrame *current = &history[historyCount - 1];
    s->state.view = current->view;
    s->state.scroll = current->scroll;
    s->state.cursor = current->cursor - current->scroll;
    s->state.count = s->state.view < 2 ? rowCount(s, s->state.view) : 1;
    if (current->view < 2) {
        for (int i = 0; i < s->state.count; ++i)
            if (s->mapping[current->view][i] == current->model) current->cursor = i;
        int maximum = s->state.count > 7 ? s->state.count - 7 : 0;
        if (s->state.scroll > maximum) s->state.scroll = maximum;
        if (current->cursor < s->state.scroll) s->state.scroll = current->cursor;
        if (current->cursor >= s->state.scroll + 7) s->state.scroll = current->cursor - 6;
        s->state.cursor = current->cursor - s->state.scroll;
    }
    s->state.touchRow = -1;
    s->state.activate = s->state.back = s->state.step = 0;
    s->state.helpPage = s->state.helpPages = 0;
    s->state.jump = -1;
    s->helpCount = 0;
    memset(s->keys, 0, sizeof s->keys);
    memset(s->state.rows, 0, sizeof s->state.rows);
    for (int i = 0; i < 7; ++i) s->state.rows[i].kind = P4_SETTINGS_NATIVE_EMPTY;
    s->helpBody[0] = '\0';
    s->helpCount = 0;
    s->helpRestart = -1;
    s->previousScroll = s->state.scroll;
    s->scrollFrame = 3;
    s->state.scrollOffset = 0;
    s->state.navigation = 0;
    if (current->view >= 2) {
        copyText(s->title, sizeof s->title, pages[current->view - 2].page.title);
        uppercase(s->title);
        s->state.title = P4_H2G(s->title);
        current->queried = 0;
        if (pageRefresh(s, s->state.cursor) < 0) return;
        if (s->state.touchRow >= 0) s->state.cursor = s->state.touchRow;
    } else s->state.title = 0;
}

void p4_settings_native_back(void)
{
    if (owner) {
        storage()->state.back = 1;
        touchFrame = inputFrame;
    }
}
void p4_settings_native_touch_scroll(int direction)
{
    if (direction < -7) direction = -7;
    if (direction > 7) direction = 7;
    if (owner) {
        storage()->state.step = direction;
        touchFrame = inputFrame;
    }
}
void p4_settings_native_jump(int last)
{
    if (owner) {
        P4SettingsNativeState *s = &storage()->state;
        s->jump = last ? s->count - 1 : 0;
        touchFrame = inputFrame;
    }
}
void p4_settings_native_touch(float x, float y)
{
    if (!owner) {
        if (x >= 544 && x <= 640 && y >= 0 && y < 48) p4_settings_native_request();
        return;
    }
    P4SettingsNativeState *s = &storage()->state;
    touchFrame = inputFrame;
    if (y >= 388) {
        if (x >= 544) s->back = 1;
        else if (x >= 480) {
            s->touchRow = (int32_t)P4_G32(owner + 0x30u);
            s->activate = 1;
            s->direction = 0;
        }
    } else if (x >= 618 && x <= 640 && y >= 88 && y <= 116) {
        s->step = -1;
    } else if (x >= 618 && x <= 640 && y >= 316 && y <= 344) {
        s->step = 1;
    } else if (x >= 618 && x <= 640 && y > 116 && y < 316 && s->count > 7) {
        float fraction = (y - 117.0f) / 198.0f;
        if (fraction < 0.0f) fraction = 0.0f;
        if (fraction > 1.0f) fraction = 1.0f;
        s->jump = (int)(fraction * (float)(s->count - 1));
    } else if (y >= 344 && y < 388) {
        p4_settings_native_help_step(1);
    } else if (x >= 219 && x < 618 && y >= 119 && y < 343) {
        int row = (int)((y - 119) / 32);
        if (s->scroll + row >= s->count) return;
        s->touchRow = row;
        s->activate = row == (int32_t)P4_G32(owner + 0x30u);
        s->direction = x < 498 && x >= 464 ? -1 : x >= 568 ? 1 : 0;
    }
}
void p4_settings_native_hover(float x, float y)
{
    if (!owner || x < 219 || x >= 618 || y < 119 || y >= 343) return;
    P4SettingsNativeState *s = &storage()->state;
    int row = (int)((y - 119) / 32);
    if (s->scroll + row >= s->count || row == (int32_t)P4_G32(owner + 0x30u)) return;
    s->touchRow = row;
    s->activate = 0;
    s->direction = 0;
    touchFrame = inputFrame;
}
int p4_settings_native_cursor(int *view)
{
    if (!owner) return -1;
    const P4SettingsNativeState *s = &storage()->state;
    *view = s->view;
    return s->scroll + (int32_t)P4_G32(owner + 0x30u);
}
int p4_settings_native_find_row(int modelIndex)
{
    if (!owner) return -1;
    NativeStorage *s = storage();
    if (s->state.view >= 2) return -1;
    int count = rowCount(s, s->state.view);
    for (int i = 0; i < count; ++i)
        if (s->mapping[s->state.view][i] == modelIndex) return i;
    return -1;
}
int p4_settings_native_row_position(int modelIndex, float *x, float *y)
{
    int logical = p4_settings_native_find_row(modelIndex);
    if (logical < 0) return 0;
    int slot = logical - storage()->state.scroll;
    if (slot < 0 || slot >= P4_SETTINGS_NATIVE_VISIBLE) return 0;
    *x = 535.0f;
    *y = 134.0f + 32.0f * (float)slot;
    return 1;
}
void p4_settings_native_value_changed(int modelIndex)
{
    if (owner && modelIndex >= 0 && modelIndex < P4_SETTINGS_MODEL_BUILTINS + P4_SETTINGS_EXTENSION_MAX)
        storage()->pcTimers[modelIndex] = 0;
}
void p4_settings_native_frame_end(void)
{
    /* libpad pumps SDL after this frame's task updates. Keep its inbox through
     * the next task-update opportunity only; an event/battle refusal never
     * becomes a delayed request when the game later permits Camp. */
    if (requestFrame != inputFrame) {
        request = 0;
        if (!requestedRouted) requestedView = -1;
    }
    if (owner && touchFrame != inputFrame) {
        P4SettingsNativeState *s = &storage()->state;
        s->touchRow = -1;
        s->activate = s->back = s->step = 0;
        s->jump = -1;
    }
    ++inputFrame;
    p4_settings_model_frame_end();
}
void p4_settings_native_corner(void)
{
    if (!p4_settings_touch_active() || owner || qualificationWork || routeWork) return;
    /* Camp's centered table 0xb1, not the field banner's left-anchored 0x4a.
     * The same 640x448 touch coordinates stay correct in widescreen. */
    void *queue = P4_G2H(0x00793e80u + 0xb1u * 0x30u);
    sflDataDrawNarrowRoundedFrame(548, 10, 59000.0f, 80, 30, 0x191919,
        255, 1, 40, 15, queue, 0.0f, 1.0f, 1.0f);
    frFontExDrawTextDeferredWithCharWidth(588.0f, 13.0f, 58990.0f,
        (int)0xfaff20ffu, 8, 1, P4_GPTR(char, p4_gport_str("CONFIG")), 8, 0, queue, -1);
}

static size_t textLength(const char *text, size_t capacity)
{
    if (text)
        for (size_t i = 0; i < capacity; ++i)
            if (!text[i]) return i;
    return capacity;
}
static int validText(const char *text, size_t capacity)
{
    return textLength(text, capacity) < capacity;
}
static int copyText(char *to, size_t capacity, const char *from)
{
    if (!from) from = "";
    size_t length = textLength(from, capacity);
    if (length == capacity) return pageError("Page text exceeds its limit.");
    if (to != from) memcpy(to, from, length + 1);
    return 0;
}
static int fontText(const char *text, size_t capacity)
{
    size_t length = textLength(text, capacity);
    return length < capacity && p4_native_glyphs_validate((const uint8_t *)text,
        length + 1, P4_GLYPHS_RAW, 0) == 0;
}
/* LF belongs to the page layer, not the raw font grammar. Only owned snapshots
 * are split temporarily; the provider's borrowed bytes are never modified. */
static int fontLines(char *text, size_t capacity)
{
    size_t line = 0;
    for (size_t end = 0; end < capacity; ++end) {
        char separator = text[end];
        if (separator == '\n' || separator == '\0') {
            text[end] = '\0';
            int valid = p4_native_glyphs_validate((const uint8_t *)text + line,
                end - line + 1, P4_GLYPHS_RAW, 0) == 0;
            text[end] = separator;
            if (!valid) return 0;
            if (!separator) return 1;
            line = end + 1;
        }
    }
    return 0;
}
static int pageError(const char *message)
{
    p4_settings_message(message);
    pageFeedback = 8;
    return -1;
}
static int pageSlot(const char *key)
{
    if (!validText(key, P4_SETTINGS_PAGE_KEY_MAX) || !key[0]) return -1;
    for (int i = 0; i < P4_SETTINGS_PAGE_MAX; ++i)
        if (pages[i].registered && !strcmp(pages[i].page.key, key)) return i;
    return -1;
}
int p4_settings_page_register(const P4SettingsPage *page)
{
    if (!page || !validText(page->key, P4_SETTINGS_PAGE_KEY_MAX) || !page->key[0] ||
        !fontText(page->title, P4_SETTINGS_PAGE_LABEL_MAX) || !page->title[0] ||
        (page->detail ? (page->count || page->row || page->find || page->adjust) :
            (!page->count || !page->row || !page->find)))
        return pageError("Invalid page descriptor.");
    if (pageSlot(page->key) >= 0) return pageError("Page key is already registered.");
    for (int i = 0; i < P4_SETTINGS_PAGE_MAX; ++i) {
        if (!pages[i].registered) {
            pages[i].page = *page;
            pages[i].registered = 1;
            return 0;
        }
    }
    return pageError("Page registry is full.");
}
static void closeFrames(int first)
{
    while (historyCount > first) {
        PageFrame *frame = &history[--historyCount];
        if (frame->view >= 2 && frame->opened) {
            P4SettingsPage *page = &pages[frame->view - 2].page;
            if (page->closed) {
                ++callbackDepth;
                page->closed(page->userdata);
                --callbackDepth;
            }
        }
        memset(frame, 0, sizeof *frame);
    }
}
int p4_settings_page_unregister(const char *key)
{
    if (callbackDepth) return pageError("Cannot unregister in a callback.");
    int slot = pageSlot(key);
    if (slot < 0) return pageError("Page is not registered.");
    int view = slot + 2;
    if (requestedView == view) {
        requestedView = -1;
        requestedRouted = 0;
        request = 0;
    }
    if (owner) {
        P4SettingsNativeState *s = &storage()->state;
        if (s->navigation == P4_SETTINGS_NATIVE_SWITCH && s->nextView == view)
            s->nextView = P4_SETTINGS_NATIVE_RESTORE;
        for (int i = 1; i < historyCount; ++i) {
            if (history[i].view != view) continue;
            closeFrames(i); /* descendants close before their owning page */
            if (hosted && i == 1) {
                s->navigation = P4_SETTINGS_NATIVE_CLOSE; /* never reveal the hidden root */
            } else if (s->navigation != P4_SETTINGS_NATIVE_CLOSE) {
                s->nextView = P4_SETTINGS_NATIVE_RESTORE;
                s->navigation = P4_SETTINGS_NATIVE_SWITCH;
            }
            break;
        }
    }
    memset(&pages[slot], 0, sizeof pages[slot]);
    return 0;
}
int p4_settings_page_open(const char *key)
{
    int slot = pageSlot(key);
    if (slot < 0) return pageError("Page is not registered.");
    if (qualificationWork || (retailOnly == 1))
        return pageError("Pages are disabled in retail-only mode.");
    if (!owner) {
        if (requestedView >= 0 || request || routeParent || routeWork || hostWork)
            return pageError("A menu request is already pending.");
        requestedView = slot + 2;
        requestedRouted = 0;
        p4_settings_native_request();
        return 0;
    }
    P4SettingsNativeState *s = &storage()->state;
    if (p4_settings_model_capturing()) return pageError("Finish binding capture first.");
    if (s->navigation || historyCount == P4_SETTINGS_PAGE_DEPTH_MAX)
        return pageError("Page navigation is busy or full.");
    s->nextView = slot + 2;
    s->navigation = P4_SETTINGS_NATIVE_SWITCH;
    return 0;
}
int p4_settings_page_back(void)
{
    if (!owner || historyCount <= 1) return pageError("No parent page.");
    P4SettingsNativeState *s = &storage()->state;
    if (p4_settings_model_capturing()) return pageError("Finish binding capture first.");
    if (s->navigation) return pageError("Page navigation is busy.");
    if (hosted && historyCount == 2) s->navigation = P4_SETTINGS_NATIVE_CLOSE;
    else {
        s->nextView = P4_SETTINGS_NATIVE_BACK;
        s->navigation = P4_SETTINGS_NATIVE_SWITCH;
    }
    return 0;
}
int p4_settings_page_close(void)
{
    if (!owner) return pageError("Config is not open.");
    P4SettingsNativeState *s = &storage()->state;
    if (p4_settings_model_capturing()) return pageError("Finish binding capture first.");
    if (s->navigation) return pageError("Page navigation is busy.");
    s->navigation = P4_SETTINGS_NATIVE_CLOSE;
    return 0;
}
const char *p4_settings_page_active(void)
{
    if (!owner || !historyCount) return NULL;
    int view = history[historyCount - 1].view;
    return view >= 2 && pages[view - 2].registered ? pages[view - 2].page.key : NULL;
}

static int pageQueryFailed(void)
{
    pageFeedback = 8;
    if (owner && !storage()->state.navigation && historyCount > 1)
        p4_settings_page_back();
    return -1;
}
static int pageRefresh(NativeStorage *s, int selected)
{
    PageFrame *frame = &history[historyCount - 1];
    P4SettingsPage *page = &pages[frame->view - 2].page;
    uint64_t revision = 0;
    int logical = s->state.scroll + selected;
    if (frame->queried && selected >= 0 && selected < 7 &&
        s->state.rows[selected].index == logical && s->keys[selected][0])
        copyText(frame->key, sizeof frame->key, s->keys[selected]);
    int count;
    if (page->detail) {
        ++callbackDepth;
        const char *body = page->detail(page->userdata, &revision);
        --callbackDepth;
        if (!body) return pageQueryFailed();
        NativeFontKey key;
        fontKey(&key);
        if (!frame->queried || revision != frame->revision ||
            memcmp(&s->detailFont, &key, sizeof key)) {
            if (copyText(s->detail, sizeof s->detail, body) < 0 ||
                !fontLines(s->detail, sizeof s->detail)) {
                pageError("Invalid or oversized detail text.");
                return pageQueryFailed();
            }
            s->detailFont = key;
            s->detailCount = wrapText(s->detail, s->detailLines, NATIVE_INFO_TEXT_WIDTH, 8,
                P4_SETTINGS_PAGE_LABEL_MAX);
        }
        count = s->detailCount;
    } else {
        ++callbackDepth;
        count = page->count(page->userdata, &revision);
        --callbackDepth;
        if (count < 0) return pageQueryFailed();
        if (count > P4_SETTINGS_PAGE_ROWS_MAX) {
            pageError("Page row count exceeds its limit.");
            return pageQueryFailed();
        }
    }
    if (!frame->queried || frame->revision != revision || frame->count != count) {
        logical = frame->queried ? logical : frame->cursor;
        if (!page->detail && frame->key[0]) {
            ++callbackDepth;
            int found = page->find(page->userdata, frame->key);
            --callbackDepth;
            if (found >= 0 && found < count) logical = found;
            else frame->key[0] = '\0';
        }
        int maximum = count > 7 ? count - 7 : 0;
        if (logical >= count) logical = count ? count - 1 : 0;
        if (logical < 0) logical = 0;
        int scroll = logical - selected;
        if (scroll < 0) scroll = 0;
        if (scroll > maximum) scroll = maximum;
        s->state.scroll = scroll;
        s->state.touchRow = logical - scroll;
        touchFrame = inputFrame;
        s->state.activate = s->state.step = 0;
        frame->revision = revision;
        frame->count = count;
        frame->queried = 1;
    }
    frame->cursor = logical;
    frame->scroll = s->state.scroll;
    s->state.count = count ? count : 1;
    return 0;
}
static void pageDescribe(NativeStorage *s, int slot, int logical)
{
    P4SettingsNativeRow *row = &s->state.rows[slot];
    PageFrame *frame = &history[historyCount - 1];
    P4SettingsPage *page = &pages[frame->view - 2].page;
    memset(row, 0, sizeof *row);
    row->index = logical;
    row->kind = P4_SETTINGS_NATIVE_EMPTY;
    row->label = P4_H2G(s->labels[slot]);
    row->value = P4_H2G(s->values[slot]);
    row->help = P4_H2G(s->help[slot]);
    s->labels[slot][0] = s->values[slot][0] = s->help[slot][0] = '\0';
    if (logical >= s->state.count) return;
    if (!frame->count) {
        if (!logical) {
            row->kind = P4_SETTINGS_NATIVE_INFO;
            copyText(s->labels[slot], sizeof s->labels[slot], page->detail ? "No text." : "No entries.");
            copyText(s->help[slot], sizeof s->help[slot], "Back: return to the previous page.");
        }
        s->keys[slot][0] = '\0';
        return;
    }
    if (page->detail) {
        row->kind = P4_SETTINGS_NATIVE_INFO;
        copyLine(s->labels[slot], s->detail, s->detailLines, logical);
        copyText(s->help[slot], sizeof s->help[slot], "Up/Down: scroll. L1/R1: page.");
        s->keys[slot][0] = '\0';
        return;
    }
    P4SettingsPageRow item = {0};
    ++callbackDepth;
    int result = page->row(page->userdata, logical, &item);
    --callbackDepth;
    if (result < 0) { pageQueryFailed(); return; }
    if (!validText(item.key, P4_SETTINGS_PAGE_KEY_MAX) || !item.key[0] ||
        !fontText(item.label, sizeof s->labels[slot]) ||
        (item.value && !fontText(item.value, sizeof s->values[slot])) ||
        (item.help && !validText(item.help, P4_SETTINGS_PAGE_HELP_MAX)) ||
        (item.target && !validText(item.target, P4_SETTINGS_PAGE_KEY_MAX)) ||
        item.kind < P4_SETTINGS_PAGE_INFO || item.kind > P4_SETTINGS_PAGE_LINK ||
        (item.kind == P4_SETTINGS_PAGE_LINK &&
            (!validText(item.target, P4_SETTINGS_PAGE_KEY_MAX) || !item.target[0]))) {
        pageError("Invalid or oversized page row.");
        pageQueryFailed();
        return;
    }
    if (strcmp(s->keys[slot], item.key)) s->visibleTimers[slot] = 5;
    copyText(s->keys[slot], sizeof s->keys[slot], item.key);
    copyText(s->labels[slot], sizeof s->labels[slot], item.label);
    copyText(s->values[slot], sizeof s->values[slot], item.value);
    copyText(s->help[slot], sizeof s->help[slot], item.help);
    copyText(s->targets[slot], sizeof s->targets[slot], item.target);
    if (!fontLines(s->help[slot], sizeof s->help[slot])) {
        pageError("Invalid native glyphs in page help.");
        pageQueryFailed();
        return;
    }
    row->flags = (item.flags & P4_SETTINGS_PAGE_DISABLED) ? P4_SETTINGS_NATIVE_ROW_DISABLED : 0;
    row->restart = (item.flags & P4_SETTINGS_PAGE_RESTART) != 0;
    row->kind = item.kind == P4_SETTINGS_PAGE_INFO ? P4_SETTINGS_NATIVE_INFO :
        item.kind == P4_SETTINGS_PAGE_TOGGLE ? P4_SETTINGS_NATIVE_TOGGLE :
        item.kind == P4_SETTINGS_PAGE_VALUE ? P4_SETTINGS_NATIVE_VALUE : P4_SETTINGS_NATIVE_CHOICE;
    row->boolean = item.checked != 0;
    row->timer = P4_H2G(&s->visibleTimers[slot]);
    if (item.flags & P4_SETTINGS_PAGE_CURRENT)
        copyText(s->values[slot], sizeof s->values[slot], "CURRENT");
    else if (row->kind == P4_SETTINGS_NATIVE_CHOICE && !s->values[slot][0])
        copyText(s->values[slot], sizeof s->values[slot], "OK");
    if (row->kind != P4_SETTINGS_NATIVE_INFO) uppercase(s->labels[slot]);
    uppercase(s->values[slot]);
    if (logical == frame->cursor) copyText(frame->key, sizeof frame->key, s->keys[slot]);
}
void p4_settings_native_activate(int slot, int direction)
{
    if (!owner || !historyCount || slot < 0 || slot >= 7) return;
    NativeStorage *s = storage();
    PageFrame *frame = &history[historyCount - 1];
    if (frame->view < 2 || s->state.navigation || !s->keys[slot][0]) return;
    P4SettingsPage *page = &pages[frame->view - 2].page;
    if (page->detail) return;
    char key[P4_SETTINGS_PAGE_KEY_MAX], target[P4_SETTINGS_PAGE_KEY_MAX];
    copyText(key, sizeof key, s->keys[slot]);
    uint64_t revision;
    ++callbackDepth;
    int count = page->count(page->userdata, &revision);
    int index = count >= 0 ? page->find(page->userdata, key) : -1;
    P4SettingsPageRow item = {0};
    int result = index >= 0 && index < count ? page->row(page->userdata, index, &item) : -1;
    --callbackDepth;
    if (result < 0 || !validText(item.key, sizeof key) || strcmp(item.key, key)) {
        pageError("The selected item is no longer available.");
        frame->queried = 0;
        return;
    }
    if (item.flags & P4_SETTINGS_PAGE_DISABLED) {
        pageError("This item is unavailable.");
        return;
    }
    if (item.kind == P4_SETTINGS_PAGE_INFO) return;
    if (item.kind == P4_SETTINGS_PAGE_LINK) {
        if (!direction && !copyText(target, sizeof target, item.target) &&
            p4_settings_page_open(target) == 0) pageFeedback = 1;
        return;
    }
    if (!page->adjust) { pageError("This item has no action."); return; }
    if (item.kind == P4_SETTINGS_PAGE_ACTION && direction) return;
    ++callbackDepth;
    result = page->adjust(page->userdata, key, direction);
    --callbackDepth;
    pageFeedback = result < 0 ? 8 : 1;
    if (!result) s->visibleTimers[slot] = 0;
}
int p4_settings_native_feedback(void)
{
    int feedback = p4_settings_model_feedback();
    if (pageFeedback >= 0) feedback = pageFeedback;
    pageFeedback = -1;
    return feedback;
}
