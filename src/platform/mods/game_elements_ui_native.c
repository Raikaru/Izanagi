#include "p4pc/game_elements_ui.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_knowledge_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_text_native.h"
#include "p4pc/package_boot.h"
#include "p4pc/platform.h"
#include <stdio.h>
#include <string.h>

/* Exact0x2c payload of the native sprite sample (0046D200/0046D4C0). One
 * guest-backed immediate sample is prepared once; the existing SDK decodes and
 * renders. No per-cell allocation, foreign atlas or renderer. */
typedef struct SdkSpriteSample {
    uint32_t packAddress, recordIndex;
    float x, y;
    uint8_t alphaSubtract, fade, pad12[2];
    int16_t extendX, extendY;
    float angle;
    int16_t pivotX, pivotY;
    uint16_t scaleX, scaleY;
    float depth;
    uint8_t red, green, blue, pad2B;
} SdkSpriteSample;
_Static_assert(sizeof(SdkSpriteSample) == 0x2c, "native SDK sample payload");
extern void sdkSpriteDraw(uint8_t *sample, int32_t set_states);
/* Battle UI canvas helpers of BtlUiSprite (00201410..00201720). */
extern void btlUiSetScale(uint8_t *work, float x, float y);
extern void btlUiDrawSpriteFaded(uint8_t *work, int32_t slot, int32_t record,
    float x, float y, uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha);

/* Borrowed frozen model, staged at UI300 runtime prepare; published only by
 * the coordinator commit after the element owner committed its registry. */
static struct ElementUiStage {
    const P4GameElementUi *ui;
    SdkSpriteSample *sample;
    int prepared;
} staged;
static const P4GameElementUi *active;

_Noreturn void p4_game_elements_ui_native_refuse(int32_t error)
{
    char reason[192];
    snprintf(reason, sizeof reason, "Native element UI refused (%d): %s.", (int)error, p4_game_tables_error(error));
    p4_package_boot_runtime_failed(reason);
}
uint32_t p4_game_elements_ui_native_count(void) { return p4_game_elements_ui_count(active); }
/* The element owner holds every enemy (0..335, initial form) and persona
 * (1..255) alias to the battle-required pairs. A displayed pair outside that
 * set must still be answerable everywhere: Analyze by the original natural
 * route, and the persona panel by its authorized native snapshot word. */
static int display_pair_covered(const P4GameTables *tables, const P4GameActionElement *pair, uint32_t legacy)
{
    int required = p4_game_elements_prepared_required_pair(pair->element_id, pair->channel_id);
    if (required < 0) return required;
    if (required) return P4_TABLE_OK;
    if (!legacy) return P4_TABLE_NOT_READY;
    return p4_game_elements_profile_natural_pair(tables, pair->element_id, pair->channel_id, pair->component) > 0 ?
        P4_TABLE_OK : P4_TABLE_NOT_READY;
}

static int complete_prepare(void *user)
{
    (void)user;
    if (staged.prepared) return P4_TABLE_OK; /* Same immutable snapshot on every loader rebind. */
    const P4GameTables *tables = p4_game_elements_ui_tables(staged.ui);
    /* Element prepares first; it must have prepared exactly this registry. */
    if (!tables || p4_game_elements_prepared_snapshot() != tables) return P4_TABLE_CONFLICT;
    const uint32_t count = p4_game_elements_ui_count(staged.ui);
    for (uint32_t i = 0; i < count; ++i) {
        const P4GameElementUiPreparedColumn *column = p4_game_elements_ui_column(staged.ui, i);
        int result = display_pair_covered(tables, &column->primary, column->primary_legacy);
        if (!result && column->marker.valid) result = display_pair_covered(tables, &column->marker, column->marker_legacy);
        if (result) return result;
    }
    /* Analyze's reveal policy reads actual loaded enemy metadata. Retail rows
     * are already loaded here; prove them before anything is published. An
     * added enemy's row exists only once the enemy owner, which prepared and
     * verified it in this same transaction, commits its relocated bank, and no
     * battle unit can carry an ID beyond that published bank. */
    const size_t aliases = p4_game_tables_affinity_alias_count(tables);
    for (size_t i = 0; i < aliases; ++i) {
        const P4AffinityProfileAlias *alias = p4_game_tables_affinity_alias_at(tables, i);
        uint32_t flags;
        if (alias->entity_kind != P4_ENTITY_ENEMY || alias->entity_id >= P4_ENEMY_RETAIL_COUNT) continue;
        int result = p4_game_elements_enemy_metadata_flags(alias->entity_id, &flags);
        if (result) return result;
    }
    staged.prepared = 1;
    return P4_TABLE_OK;
}
static void complete_commit(void *user)
{
    (void)user;
    active = staged.ui; /* Infallible: everything was prepared. */
}
int p4_game_elements_ui_native_stage(const P4GameElementUi *ui)
{
    const uint32_t count = p4_game_elements_ui_count(ui);
    if (!count) return P4_TABLE_INVALID;
    if (staged.ui) return staged.ui == ui ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    SdkSpriteSample *sample = p4pc_heap_calloc(1, sizeof *sample);
    if (!sample) return P4_TABLE_NOMEM;
    int result = p4_game_tables_battle_complete_register("element-ui", complete_prepare, complete_commit, NULL);
    if (!result) result = p4_game_tables_battle_complete_require("element-ui", "element");
    if (!result) result = p4_game_tables_battle_complete_require("element-ui", "element-knowledge");
    if (result) {
        int rollback = p4_game_tables_battle_complete_unregister("element-ui", complete_prepare, complete_commit, NULL);
        p4pc_heap_free(sample);
        return rollback ? rollback : result;
    }
    staged = (struct ElementUiStage){ui, sample, 0};
    return P4_TABLE_OK;
}
int p4_game_elements_ui_native_detached(const P4GameElementUi *ui)
{
    return !ui || (staged.ui != ui && active != ui);
}
void p4_game_elements_ui_native_session_end(void)
{
    /* Core proved task/queue/graphics quiescence; it clears the shared
     * coordinator itself. This drops only our borrow and sample. */
    active = NULL;
    if (staged.sample) p4pc_heap_free(staged.sample);
    memset(&staged, 0, sizeof staged);
}

static void native_sprite(uint32_t bank, uint32_t record, float x, float y,
    float depth, uint32_t rgba, uint16_t scale_x, uint16_t scale_y)
{
    SdkSpriteSample *sample = staged.sample;
    sample->packAddress = bank;
    sample->recordIndex = record;
    sample->x = x; sample->y = y; sample->depth = depth;
    sample->red = (uint8_t)rgba;
    sample->green = (uint8_t)(rgba >> 8);
    sample->blue = (uint8_t)(rgba >> 16);
    sample->alphaSubtract = (uint8_t)(255u - (rgba >> 24));
    sample->scaleX = scale_x;
    sample->scaleY = scale_y;
    sdkSpriteDraw((uint8_t *)sample, 0); /* Immediate 0046B380; never queued. */
}
/* 00116820 precedence, which differs from Analyze's 00218C60. */
static uint32_t persona_affinity_sprite(uint32_t value)
{
    if (value & P4_AFF_REPEL) return 0x35;
    if (value & P4_AFF_WEAK) return 0x36;
    if (value & P4_AFF_RESIST) return 0x37;
    if (value & P4_AFF_NULL) return 0x38;
    if (value & P4_AFF_DRAIN) return 0x39;
    return 0x3a;
}
static uint32_t analyze_affinity_sprite(uint32_t value)
{
    if (value & P4_AFF_WEAK) return 0x36;
    if (value & P4_AFF_NULL) return 0x38;
    if (value & P4_AFF_RESIST) return 0x37;
    if (value & P4_AFF_DRAIN) return 0x39;
    if (value & P4_AFF_REPEL) return 0x35;
    return 0x3a;
}
static uint32_t ko_marker(uint32_t value)
{
    if (value & P4_AFF_NULL) return P4_KO_NULL;
    if (value & P4_AFF_WEAK) return P4_KO_VULNERABLE;
    if (value & P4_AFF_RESIST) return P4_KO_RESIST;
    return P4_KO_NORMAL;
}
/* Seven columns keep the retail 51/37 spacing; other counts span the same
 * first-to-last header extent (236 px), horizontally fitting native glyphs. */
static float column_x(uint32_t index, uint32_t count)
{
    if (count == 7u) return index ? 51.0f + (float)(index - 1u) * 37.0f : 0.0f;
    return count > 1u ? 236.0f * (float)index / (float)(count - 1u) : 0.0f;
}
static uint16_t column_scale(uint32_t count)
{
    return count > 7u ? (uint16_t)((236u * 4096u) / (37u * (count - 1u))) : 0;
}
static int authored_icon(const P4GameElementUiPreparedColumn *column,
    float x, float y, float depth, uint32_t rgba, float width, float height)
{
    int result = p4_resources_draw(column->icon_token, x, y, depth, rgba,
        width / (float)column->image_width, height / (float)column->image_height);
    return result == P4_RESOURCE_OK ? P4_TABLE_OK : P4_TABLE_RENDER_ERROR;
}

int p4_game_elements_ui_native_skill_icon(void *unit, uint32_t skill, uint32_t *icon)
{
    if (!icon) return P4_TABLE_INVALID;
    if (!active || !p4_game_skill_active(skill)) return P4_ELEMENT_UI_ART_RETAIL;
    /* 00114CB0/0013AC30 draw datCalcSkillElement's skill element, which is the
     * applicability frame: without an actor it is the skill's own element,
     * with the battle row's actual unit a weapon-derived skill uses its weapon. */
    P4GameActionFrame frame;
    p4_game_elements_native_applicability_frame(unit, skill, &frame);
    if (!frame.element.valid) {
        *icon = 0; /* The skill genuinely has no element art. */
        return P4_ELEMENT_UI_ART_HANDLED;
    }
    uint32_t art;
    int result = p4_game_elements_ui_art_find(active, frame.element.element_id, &art);
    if (!result) {
        *icon = art + 1u;
        return P4_ELEMENT_UI_ART_HANDLED;
    }
    if (result != P4_TABLE_NOT_FOUND) return result;
    /* No authored art: a retail leaf keeps the original atlas call. Every
     * visible new leaf has art, so a new leaf here is hidden and has none;
     * it is never narrowed into a legacy atlas index. */
    if (frame.element.element_id < P4_ELEMENT_RETAIL_COUNT) return P4_ELEMENT_UI_ART_RETAIL;
    *icon = 0;
    return P4_ELEMENT_UI_ART_HANDLED;
}
int p4_game_elements_ui_native_draw_icon(uint32_t icon, float x, float y, float depth,
    uint32_t rgba, float width, float height)
{
    if (!active) return P4_TABLE_INVALID;
    if (!icon || !width || !height || !(rgba >> 24)) return P4_TABLE_OK;
    return p4_game_elements_ui_art_draw(active, icon - 1u, x, y, depth, rgba, width, height);
}

int p4_game_elements_ui_native_persona_affinities(uint32_t persona, const uint32_t native_words[7],
    float x, float y, float depth, uint32_t alpha, uint32_t panel_bank, uint32_t affinity_bank)
{
    const uint32_t count = p4_game_elements_ui_count(active);
    if (!count || !staged.sample) return P4_TABLE_PREPARE_REQUIRED;
    alpha &= 255u;
    if (!alpha) return P4_TABLE_OK;
    uint32_t values[P4_ELEMENT_UI_MAX_COLUMNS], markers[P4_ELEMENT_UI_MAX_COLUMNS];
    /* Resolve every pair before any cell draw: no partial row and no static
     * guard/passive synthesis for a display snapshot. */
    for (uint32_t i = 0; i < count; ++i) {
        int result = p4_game_elements_ui_persona_snapshot_use(active, persona, native_words, i, 0, &values[i]);
        if (result) return result;
        if (p4_game_elements_ui_column(active, i)->marker.valid) {
            result = p4_game_elements_ui_persona_snapshot_use(active, persona, native_words, i, 1, &markers[i]);
            if (result) return result;
        }
    }
    const uint32_t opacity = alpha << 24;
    const uint16_t scale_x = column_scale(count);
    for (uint32_t i = 0; i < count; ++i) {
        const P4GameElementUiPreparedColumn *column = p4_game_elements_ui_column(active, i);
        const float point_x = x + column_x(i, count);
        if (column->art_kind == P4_ELEMENT_UI_RETAIL_ART)
            native_sprite(affinity_bank, column->native_affinity_sprite, point_x, y, depth,
                opacity | 0x0081ffffu, scale_x, 0);
        else {
            int result = authored_icon(column, point_x, y, depth, opacity | 0x0081ffffu, 26.0f, 20.0f);
            if (result) return result;
        }
        native_sprite(panel_bank, 0x43, point_x - 2.0f, y + 22.0f, depth, opacity | 0x002ce9ffu, scale_x, 0);
        native_sprite(affinity_bank, persona_affinity_sprite(values[i]), point_x, y + 24.0f, depth,
            opacity | 0x000075deu, scale_x, 0);
        if (column->marker.valid && p4_game_tables_text_draw_geometry(
                p4_game_elements_ui_ko_text(active, ko_marker(markers[i])),
                point_x, y + 46.0f, depth, 0xde750000u | alpha, 1, 0, 0, 0, 64, 64))
            return P4_TABLE_RENDER_ERROR;
    }
    return P4_TABLE_OK;
}

/* Leading scalar fields of BtlUiSprite (0x04..0x23), read without aliasing the
 * native record; resource references stay with the native battle helpers. */
typedef struct ElementUiCanvas {
    int32_t origin_x, origin_y, width, height;
    float factor_x, factor_y, scale_x, scale_y;
} ElementUiCanvas;
_Static_assert(sizeof(ElementUiCanvas) == 0x20, "native battle canvas scalar fields");
static void canvas_point(const ElementUiCanvas *canvas, float x, float y, float *screen_x, float *screen_y)
{
    /* Same projection as 00201410 btlUiDrawSpriteEntry. */
    if (canvas->factor_x == 1.0f && canvas->factor_y == 1.0f) {
        *screen_x = (float)canvas->origin_x + x;
        *screen_y = (float)canvas->origin_y + y;
        return;
    }
    *screen_x = (float)canvas->origin_x + (float)canvas->width * (1.0f - canvas->factor_x) * 0.5f +
        canvas->factor_x * x;
    *screen_y = (float)canvas->origin_y + (float)canvas->height * (1.0f - canvas->factor_y) * 0.5f +
        canvas->factor_y * y;
}
int p4_game_elements_ui_native_analyze_affinities(void *work, void *unit,
    float x, float header_y, float cell_y, float unknown_y)
{
    const uint32_t count = p4_game_elements_ui_count(active);
    if (!work || !unit || !count) return P4_TABLE_INVALID;
    uint32_t known[P4_ELEMENT_UI_MAX_COLUMNS], values[P4_ELEMENT_UI_MAX_COLUMNS];
    uint32_t marker_known[P4_ELEMENT_UI_MAX_COLUMNS], markers[P4_ELEMENT_UI_MAX_COLUMNS];
    uint32_t marker_used = 0;
    int result;
    /* Knowledge first; a hidden affinity is never read for an unknown pair. */
    for (uint32_t i = 0; i < count; ++i) {
        const P4GameElementUiPreparedColumn *column = p4_game_elements_ui_column(active, i);
        result = p4_game_elements_ui_runtime_pair(active, i, 0);
        if (!result) result = p4_game_knowledge_native_analyze(unit, &column->primary, &known[i]);
        if (!result && known[i]) result = p4_game_elements_native_profile_base_affinity(unit,
            column->primary.element_id, column->primary.channel_id, column->primary.component, &values[i]);
        if (result) return result;
        if (!column->marker.valid) continue;
        marker_used = 1;
        result = p4_game_elements_ui_runtime_pair(active, i, 1);
        if (!result) result = p4_game_knowledge_native_analyze(unit, &column->marker, &marker_known[i]);
        if (!result && marker_known[i]) result = p4_game_elements_native_profile_base_affinity(unit,
            column->marker.element_id, column->marker.channel_id, column->marker.component, &markers[i]);
        if (result) return result;
    }
    ElementUiCanvas canvas;
    memcpy(&canvas, (const uint8_t *)work + 4u, sizeof canvas);
    const float saved_x = canvas.scale_x, saved_y = canvas.scale_y;
    float depth;
    memcpy(&depth, (const uint8_t *)work + 0x90u, sizeof depth); /* sample depth at0x6C+0x24 */
    const uint32_t opacity = (255u - ((const uint8_t *)work)[0x7cu]) << 24;
    uint32_t glyph_horizontal = 64, glyph_vertical = 64;
    if (marker_used) {
        const float horizontal = 64.0f * saved_x * canvas.factor_x;
        const float vertical = 64.0f * saved_y * canvas.factor_y;
        if (!(horizontal >= 0.0f && horizontal <= 128.0f && vertical >= 0.0f && vertical <= 128.0f))
            return P4_TABLE_INVALID;
        glyph_horizontal = (uint32_t)horizontal;
        glyph_vertical = (uint32_t)vertical;
    }
    const uint16_t compressed = column_scale(count);
    btlUiSetScale(work, compressed ? saved_x * (float)compressed / 4096.0f : saved_x, saved_y);
    result = P4_TABLE_OK;
    for (uint32_t i = 0; !result && i < count; ++i) {
        const P4GameElementUiPreparedColumn *column = p4_game_elements_ui_column(active, i);
        const float point_x = x + column_x(i, count);
        const int header_known = known[i] || (column->marker.valid && marker_known[i]);
        if (column->art_kind == P4_ELEMENT_UI_RETAIL_ART)
            btlUiDrawSpriteFaded(work, 13, (int32_t)column->native_affinity_sprite, point_x, header_y,
                header_known ? 0x24 : 0x70, header_known ? 0x4c : 0x70, header_known ? 0 : 0x70, 255);
        else {
            float screen_x, screen_y;
            canvas_point(&canvas, point_x, header_y, &screen_x, &screen_y);
            int drawn = p4_resources_draw(column->icon_token, screen_x, screen_y, depth,
                opacity | (header_known ? 0x00004c24u : 0x00707070u),
                canvas.factor_x * saved_x * 26.0f / (float)column->image_width,
                canvas.factor_y * saved_y * 20.0f / (float)column->image_height);
            if (drawn != P4_RESOURCE_OK) { result = P4_TABLE_RENDER_ERROR; break; }
        }
        /* 00218C60 cell for a known pair; 00219790's unknown cell otherwise. */
        btlUiDrawSpriteFaded(work, 14, 0x43, point_x - 2.0f, cell_y,
            known[i] ? 0x24 : 0x70, known[i] ? 0x4c : 0x70, known[i] ? 0 : 0x70, 255);
        if (known[i])
            btlUiDrawSpriteFaded(work, 13, (int32_t)analyze_affinity_sprite(values[i]),
                (point_x - 2.0f) + 2.0f, cell_y + 2.0f, 0x89, 255, 0x1f, 255);
        else btlUiDrawSpriteFaded(work, 10, 0x4a, point_x, unknown_y, 0x89, 255, 0x1f, 255);
        if (column->marker.valid) {
            const uint32_t marker = marker_known[i] ? ko_marker(markers[i]) : P4_ELEMENT_UI_KO_UNKNOWN;
            float text_x, text_y;
            canvas_point(&canvas, point_x, header_y + 43.0f * saved_y, &text_x, &text_y);
            if (p4_game_tables_text_draw_geometry(p4_game_elements_ui_ko_text(active, marker),
                    text_x, text_y, depth, 0x89ff1f00u | (opacity >> 24), 1, 0, 0, 0,
                    glyph_horizontal, glyph_vertical)) result = P4_TABLE_RENDER_ERROR;
        }
    }
    btlUiSetScale(work, saved_x, saved_y);
    return result;
}
