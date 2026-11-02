#include "p4pc/game_elements_ui.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_table_use.h"
#include <stdlib.h>
#include <string.h>

/* Reviewed command-free native ASCII: P4KoAffinity order, then unknown. */
static const char ko_labels[5][5] = {"KO:N", "KO:W", "KO:S", "KO:X", "KO:?"};
typedef struct UiNativePair { uint32_t element, component, channel; } UiNativePair;
/* Exact native pair of each legacy selector (00115940 snapshot order). */
static const UiNativePair native_pairs[7] = {
    {0, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE},
    {1, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE},
    {2, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE},
    {3, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE},
    {4, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE},
    {6, P4_ACTION_COMPONENT_INSTANT_KO, P4_CHANNEL_INSTANT_KO},
    {7, P4_ACTION_COMPONENT_INSTANT_KO, P4_CHANNEL_INSTANT_KO}
};
const P4GameElementUiColumn p4_game_elements_ui_retail_columns[7] = {
    {0, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE, 0, 0, 0, 1, 0, 0, 0},
    {1, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE, 0, 0, 0, 1, 1, 0, 0},
    {2, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE, 0, 0, 0, 1, 2, 0, 0},
    {3, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE, 0, 0, 0, 1, 3, 0, 0},
    {4, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE, 0, 0, 0, 1, 4, 0, 0},
    {6, P4_ACTION_COMPONENT_INSTANT_KO, P4_CHANNEL_INSTANT_KO, 0, 0, 0, 1, 5, 0, 0},
    {7, P4_ACTION_COMPONENT_INSTANT_KO, P4_CHANNEL_INSTANT_KO, 0, 0, 0, 1, 6, 0, 0}
};
/* Authored art of one visible descriptor that declares an icon reference. */
typedef struct UiArt { const P4ElementDesc *desc; uint32_t token, width, height; } UiArt;
struct P4GameElementUi {
    P4GameTableAllocator allocator;
    char owner[64];
    uint32_t count, revision, planned, frozen;
    P4GameElementUiColumn columns[P4_ELEMENT_UI_MAX_COLUMNS];
    P4GameElementUiPreparedColumn prepared[P4_ELEMENT_UI_MAX_COLUMNS];
    const P4ElementDesc *descriptors[P4_ELEMENT_UI_MAX_COLUMNS];
    const P4AffinityChannelDesc *channels[P4_ELEMENT_UI_MAX_COLUMNS][2];
    UiArt *arts; /* Ascending leaf id; pinned while frozen. */
    uint32_t art_count, pinned;
    const P4GameTables *tables;
    void *ko_host;
    uint32_t ko_guest;
};

static int authorized_native_pair(uint32_t leaf, uint32_t component, uint32_t channel, uint32_t selector)
{
    return selector < 7u && native_pairs[selector].element == leaf &&
        native_pairs[selector].component == component && native_pairs[selector].channel == channel;
}
/* 00116820/00219790 affinity-bank header sprites; zero for leaves without one. */
static uint32_t native_affinity_header(uint32_t leaf)
{
    if (leaf <= 4u) return 0x2eu + leaf;
    if (leaf == 6u || leaf == 7u) return 0x2du + leaf;
    return 0;
}

P4GameElementUi *p4_game_elements_ui_create(const P4GameTableAllocator *allocator)
{
    if (!allocator || !allocator->allocate || !allocator->release) return NULL;
    P4GameElementUi *ui = calloc(1, sizeof *ui);
    if (ui) ui->allocator = *allocator;
    return ui;
}

int p4_game_elements_ui_layout(P4GameElementUi *ui, const char *owner,
    const P4GameElementUiColumn *columns, uint32_t count, uint32_t revision)
{
    if (!ui || !owner || !columns || !count || count > P4_ELEMENT_UI_MAX_COLUMNS || !revision)
        return P4_TABLE_INVALID;
    if (ui->planned) return P4_TABLE_FROZEN;
    size_t owner_size = 0;
    while (owner_size < sizeof ui->owner && owner[owner_size]) ++owner_size;
    if (!owner_size || owner_size == sizeof ui->owner) return P4_TABLE_INVALID;
    P4GameElementUiColumn canonical[P4_ELEMENT_UI_MAX_COLUMNS];
    for (uint32_t i = 0; i < count; ++i) {
        const P4GameElementUiColumn *column = &columns[i];
        if (column->component > P4_ACTION_COMPONENT_STATUS || column->marker_valid > 1u ||
            column->legacy_primary_valid > 1u || column->legacy_marker_valid > 1u)
            return P4_TABLE_INVALID;
        canonical[i] = *column;
        if (column->marker_valid) {
            if (column->marker_component != P4_ACTION_COMPONENT_INSTANT_KO ||
                column->marker_channel_id == column->channel_id) return P4_TABLE_INVALID;
        } else {
            canonical[i].marker_component = canonical[i].marker_channel_id = 0;
        }
        if (column->legacy_primary_valid) {
            if (!authorized_native_pair(column->element_id, column->component,
                    column->channel_id, column->legacy_primary_selector)) return P4_TABLE_INVALID;
        } else canonical[i].legacy_primary_selector = 0;
        if (column->legacy_marker_valid) {
            if (!column->marker_valid || !authorized_native_pair(column->element_id,
                    column->marker_component, column->marker_channel_id,
                    column->legacy_marker_selector)) return P4_TABLE_INVALID;
        } else canonical[i].legacy_marker_selector = 0;
        for (uint32_t j = 0; j < i; ++j)
            if (canonical[j].element_id == canonical[i].element_id) return P4_TABLE_CONFLICT;
    }
    if (ui->count) return !strcmp(ui->owner, owner) && ui->count == count && ui->revision == revision &&
        !memcmp(ui->columns, canonical, count * sizeof canonical[0]) ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    memcpy(ui->owner, owner, owner_size + 1u);
    memcpy(ui->columns, canonical, count * sizeof canonical[0]);
    ui->count = count;
    ui->revision = revision;
    return P4_TABLE_OK;
}

static int prepare_pair(const P4GameTables *tables, const P4ElementDesc *desc, uint32_t component,
    uint32_t channel_id, P4GameActionElement *element, uint32_t *kind, const P4AffinityChannelDesc **issued)
{
    const P4AffinityChannelDesc *channel = p4_game_tables_find_channel(tables, channel_id);
    if (!channel) return P4_TABLE_NOT_FOUND;
    if ((component == P4_ACTION_COMPONENT_INSTANT_KO) != (channel->kind == P4_CHANNEL_KO))
        return P4_TABLE_INVALID;
    *element = (P4GameActionElement){1, desc->id, channel_id, desc->flags, desc->family, component};
    *kind = channel->kind;
    *issued = channel;
    return P4_TABLE_OK;
}
/* Every declared alias must answer every displayed pair before publication:
 * an exact side row, the canonical natural retail route for an enemy, or an
 * explicitly authorized native snapshot word for a profile-0 persona. */
static int pair_covered(const P4GameTables *tables, const P4AffinityProfileAlias *alias,
    const P4GameActionElement *pair, uint32_t legacy)
{
    if (p4_game_tables_find_affinity(tables, alias->entity_kind, alias->entity_id,
            alias->profile_id, pair->element_id, pair->channel_id)) return 1;
    if (alias->entity_kind == P4_ENTITY_PERSONA)
        return legacy && !alias->native_profile && !alias->profile_id;
    return p4_game_elements_profile_natural_pair(tables, pair->element_id, pair->channel_id, pair->component) > 0;
}
static int prepare_coverage(const P4GameElementUi *ui, const P4GameTables *tables)
{
    const size_t count = p4_game_tables_affinity_alias_count(tables);
    for (size_t a = 0; a < count; ++a) {
        const P4AffinityProfileAlias *alias = p4_game_tables_affinity_alias_at(tables, a);
        for (uint32_t i = 0; i < ui->count; ++i) {
            const P4GameElementUiPreparedColumn *column = &ui->prepared[i];
            if (!pair_covered(tables, alias, &column->primary, ui->columns[i].legacy_primary_valid))
                return P4_TABLE_NOT_FOUND;
            if (column->marker.valid && !pair_covered(tables, alias, &column->marker,
                    ui->columns[i].legacy_marker_valid)) return P4_TABLE_NOT_FOUND;
        }
    }
    return P4_TABLE_OK;
}

int p4_game_elements_ui_prepare(P4GameElementUi *ui, const P4GameTables *tables)
{
    if (!ui || !tables || !p4_game_tables_revision(tables)) return P4_TABLE_INVALID;
    if (ui->planned) return ui->tables == tables ? P4_TABLE_OK : P4_TABLE_FROZEN;
    if (!ui->count) return P4_TABLE_PREPARE_REQUIRED;
    /* Any visible new leaf needs authored art (skill icons), and a visible new
     * affinity leaf needs a column. Retail leaves may be omitted explicitly
     * (Physical Split omits Phys0) and keep their native art without a ref. */
    const size_t elements = p4_game_tables_element_count(tables);
    size_t arts = 0;
    for (size_t e = 0; e < elements; ++e) {
        const P4ElementDesc *desc = p4_game_tables_element_at(tables, e);
        if (!(desc->flags & P4_ELEMENT_VISIBLE)) continue;
        if (desc->icon.owner[0]) ++arts;
        else if (desc->id >= P4_ELEMENT_RETAIL_COUNT) return P4_TABLE_NOT_FOUND;
        if (desc->id < P4_ELEMENT_RETAIL_COUNT || !(desc->flags & P4_ELEMENT_AFFINITY)) continue;
        uint32_t i = 0;
        while (i < ui->count && ui->columns[i].element_id != desc->id) ++i;
        if (i == ui->count) return P4_TABLE_PREPARE_REQUIRED;
    }
    if (arts > UINT32_MAX || arts > SIZE_MAX / sizeof *ui->arts) return P4_TABLE_NOMEM;
    for (uint32_t i = 0; i < ui->count; ++i) {
        const P4GameElementUiColumn *source = &ui->columns[i];
        P4GameElementUiPreparedColumn *column = &ui->prepared[i];
        const P4ElementDesc *desc = p4_game_tables_find_element(tables, source->element_id);
        if (!desc) return P4_TABLE_NOT_FOUND;
        if ((desc->flags & (P4_ELEMENT_VISIBLE | P4_ELEMENT_AFFINITY)) != (P4_ELEMENT_VISIBLE | P4_ELEMENT_AFFINITY))
            return P4_TABLE_INVALID;
        /* Art is either the descriptor's authored reference or the leaf's real
         * native header; never an opaque token reinterpreted as a sprite. */
        if (!desc->icon.owner[0] && !native_affinity_header(desc->id)) return P4_TABLE_NOT_FOUND;
        memset(column, 0, sizeof *column);
        ui->descriptors[i] = desc;
        ui->channels[i][1] = NULL;
        int result = prepare_pair(tables, desc, source->component, source->channel_id,
            &column->primary, &column->primary_kind, &ui->channels[i][0]);
        if (result) return result;
        column->primary_legacy = source->legacy_primary_valid;
        column->marker_legacy = source->legacy_marker_valid;
        if (source->marker_valid) {
            uint32_t kind;
            result = prepare_pair(tables, desc, source->marker_component, source->marker_channel_id,
                &column->marker, &kind, &ui->channels[i][1]);
            if (result) return result;
        }
    }
    int result = prepare_coverage(ui, tables);
    if (result) return result;
    UiArt *art = arts ? calloc(arts, sizeof *art) : NULL;
    if (arts && !art) return P4_TABLE_NOMEM;
    ui->arts = art;
    ui->art_count = (uint32_t)arts;
    for (size_t e = 0; e < elements; ++e) {
        const P4ElementDesc *desc = p4_game_tables_element_at(tables, e);
        if ((desc->flags & P4_ELEMENT_VISIBLE) && desc->icon.owner[0]) (art++)->desc = desc;
    }
    /* Enumeration is (order, id); art lookup is by id. */
    for (uint32_t i = 1; i < ui->art_count; ++i)
        for (uint32_t j = i; j && ui->arts[j - 1].desc->id > ui->arts[j].desc->id; --j) {
            UiArt swap = ui->arts[j - 1];
            ui->arts[j - 1] = ui->arts[j];
            ui->arts[j] = swap;
        }
    ui->tables = tables;
    ui->planned = ui->revision;
    return P4_TABLE_OK;
}

int p4_game_elements_ui_check_images(const P4GameElementUi *ui, const P4ResourceImagesPrepared *images)
{
    if (!ui || !ui->planned) return P4_TABLE_INVALID;
    for (uint32_t i = 0; i < ui->art_count; ++i) {
        const P4ResourceRef *icon = &ui->arts[i].desc->icon;
        if (!images || p4_resources_images_prepared_find(images, icon->owner, icon->key, icon->revision) != 1)
            return P4_TABLE_NOT_FOUND;
    }
    return P4_TABLE_OK;
}

static void release_pins(P4GameElementUi *ui)
{
    while (ui->pinned) p4_resources_release(ui->arts[--ui->pinned].token);
}
static int find_art(const P4GameElementUi *ui, uint32_t leaf, uint32_t *index)
{
    uint32_t low = 0, high = ui->art_count;
    while (low < high) {
        const uint32_t middle = low + (high - low) / 2u;
        if (ui->arts[middle].desc->id < leaf) low = middle + 1u; else high = middle;
    }
    if (low == ui->art_count || ui->arts[low].desc->id != leaf) return P4_TABLE_NOT_FOUND;
    *index = low;
    return P4_TABLE_OK;
}
static int pin_art(P4GameElementUi *ui, const P4Resources *resources)
{
    UiArt *art = &ui->arts[ui->pinned];
    const P4ResourceRef *icon = &art->desc->icon;
    if (!resources || !p4_resources_revision(resources)) return P4_TABLE_NOT_READY;
    const P4ResourceImage *image = p4_resources_find(resources, icon->key);
    if (!image) return P4_TABLE_NOT_FOUND;
    if (strcmp(image->owner, icon->owner) || image->revision != icon->revision) return P4_TABLE_CONFLICT;
    if (!image->token || !image->width || !image->height) return P4_TABLE_NOT_READY;
    if (p4_resources_pin(image->token) != P4_RESOURCE_OK) return P4_TABLE_NOT_READY;
    art->token = image->token;
    art->width = image->width;
    art->height = image->height;
    ++ui->pinned;
    return P4_TABLE_OK;
}
static int freeze_column_art(P4GameElementUi *ui, uint32_t index)
{
    const P4ElementDesc *desc = ui->descriptors[index];
    P4GameElementUiPreparedColumn *column = &ui->prepared[index];
    uint32_t art;
    if (!desc->icon.owner[0]) {
        column->art_kind = P4_ELEMENT_UI_RETAIL_ART;
        column->native_affinity_sprite = native_affinity_header(desc->id);
        return P4_TABLE_OK;
    }
    int result = find_art(ui, desc->id, &art);
    if (result) return result;
    column->art_kind = P4_ELEMENT_UI_AUTHORED_ART;
    column->icon_token = ui->arts[art].token;
    column->image_width = ui->arts[art].width;
    column->image_height = ui->arts[art].height;
    return P4_TABLE_OK;
}
int p4_game_elements_ui_freeze(P4GameElementUi *ui, const P4Resources *resources)
{
    if (!ui || !ui->planned) return P4_TABLE_INVALID;
    if (ui->frozen) return P4_TABLE_FROZEN;
    int result = P4_TABLE_OK;
    uint32_t marker = 0;
    while (!result && ui->pinned < ui->art_count) result = pin_art(ui, resources);
    for (uint32_t i = 0; !result && i < ui->count; ++i) {
        result = freeze_column_art(ui, i);
        marker |= ui->prepared[i].marker.valid;
    }
    if (!result && marker) {
        uint32_t guest = 0;
        void *host = ui->allocator.allocate(ui->allocator.user, sizeof ko_labels, &guest);
        if (!host) result = P4_TABLE_NOMEM;
        else if (!guest || guest > UINT32_MAX - (sizeof ko_labels - 1u)) {
            ui->allocator.release(ui->allocator.user, guest, host, sizeof ko_labels);
            result = P4_TABLE_INVALID;
        } else {
            /* Rendered by the shared MESSAGE glyph recipe; no private decoder. */
            memcpy(host, ko_labels, sizeof ko_labels);
            ui->ko_host = host;
            ui->ko_guest = guest;
        }
    }
    if (result) {
        release_pins(ui);
        return result;
    }
    ui->frozen = ui->revision;
    return P4_TABLE_OK;
}

uint32_t p4_game_elements_ui_revision(const P4GameElementUi *ui) { return ui ? ui->frozen : 0; }
uint32_t p4_game_elements_ui_count(const P4GameElementUi *ui) { return ui && ui->frozen ? ui->count : 0; }
const P4GameElementUiPreparedColumn *p4_game_elements_ui_column(const P4GameElementUi *ui, uint32_t index)
{
    return ui && ui->frozen && index < ui->count ? &ui->prepared[index] : NULL;
}
const P4GameTables *p4_game_elements_ui_tables(const P4GameElementUi *ui)
{
    return ui && ui->frozen ? ui->tables : NULL;
}
int p4_game_elements_ui_art_find(const P4GameElementUi *ui, uint32_t leaf, uint32_t *index)
{
    if (!ui || !ui->frozen || !index) return P4_TABLE_INVALID;
    return find_art(ui, leaf, index);
}
int p4_game_elements_ui_art_draw(const P4GameElementUi *ui, uint32_t index,
    float x, float y, float depth, uint32_t rgba, float width, float height)
{
    if (!ui || !ui->frozen || index >= ui->art_count) return P4_TABLE_INVALID;
    const UiArt *art = &ui->arts[index];
    int result = p4_game_tables_runtime_use_row(ui->tables, P4_TABLE_SOURCE_ELEMENT, art->desc);
    if (result) return result;
    return p4_resources_draw(art->token, x, y, depth, rgba, width / (float)art->width,
        height / (float)art->height) == P4_RESOURCE_OK ? P4_TABLE_OK : P4_TABLE_RENDER_ERROR;
}
int p4_game_elements_ui_runtime_element(const P4GameElementUi *ui, uint32_t index)
{
    if (!ui || !ui->frozen || index >= ui->count) return P4_TABLE_INVALID;
    return p4_game_tables_runtime_use_row(ui->tables, P4_TABLE_SOURCE_ELEMENT, ui->descriptors[index]);
}
int p4_game_elements_ui_runtime_pair(const P4GameElementUi *ui, uint32_t index, uint32_t marker)
{
    if (!ui || !ui->frozen || index >= ui->count || marker > 1u || !ui->channels[index][marker])
        return P4_TABLE_INVALID;
    int result = p4_game_elements_ui_runtime_element(ui, index);
    if (result) return result;
    return p4_game_tables_runtime_use_row(ui->tables, P4_TABLE_SOURCE_CHANNEL, ui->channels[index][marker]);
}
int p4_game_elements_ui_persona_snapshot_use(const P4GameElementUi *ui, uint32_t persona_id,
    const uint32_t native_words[7], uint32_t index, uint32_t marker, uint32_t *value)
{
    if (!ui || !ui->frozen || index >= ui->count || marker > 1u || !value) return P4_TABLE_INVALID;
    const P4GameElementUiPreparedColumn *column = &ui->prepared[index];
    const P4GameActionElement *element = marker ? &column->marker : &column->primary;
    if (element->valid != 1u) return P4_TABLE_INVALID;
    int result = p4_game_elements_ui_runtime_pair(ui, index, marker);
    if (result) return result;
    /* Persona native profile is always 0 (00115940 has no phase selector). */
    const P4AffinityProfileAlias *alias = p4_game_tables_find_affinity_alias(ui->tables,
        P4_ENTITY_PERSONA, persona_id, 0);
    if (alias) {
        result = p4_game_tables_runtime_use_row(ui->tables, P4_TABLE_SOURCE_ALIAS, alias);
        if (result) return result;
        const P4AffinityRow *row = p4_game_tables_find_affinity(ui->tables, P4_ENTITY_PERSONA,
            persona_id, alias->profile_id, element->element_id, element->channel_id);
        if (row) {
            result = p4_game_tables_runtime_use_row(ui->tables, P4_TABLE_SOURCE_AFFINITY, row);
            if (result) return result;
            if (marker || column->primary_kind == P4_CHANNEL_KO) {
                static const uint32_t ko_flags[4] = {0, P4_AFF_WEAK, P4_AFF_RESIST, P4_AFF_NULL};
                *value = ko_flags[row->value] | row->scalar; /* Frozen KO enum/scalar. */
            } else *value = row->value;
            return P4_TABLE_OK;
        }
    }
    const P4GameElementUiColumn *source = &ui->columns[index];
    if (!(marker ? source->legacy_marker_valid : source->legacy_primary_valid) ||
        (alias && alias->profile_id)) return P4_TABLE_NOT_FOUND;
    if (!native_words) return P4_TABLE_NOT_READY;
    /* The exact native pair of this selector was validated at layout. */
    *value = native_words[marker ? source->legacy_marker_selector : source->legacy_primary_selector];
    return P4_TABLE_OK;
}
uint32_t p4_game_elements_ui_ko_text(const P4GameElementUi *ui, uint32_t kind)
{
    return ui && ui->frozen && ui->ko_guest && kind <= P4_ELEMENT_UI_KO_UNKNOWN ?
        ui->ko_guest + kind * (uint32_t)sizeof ko_labels[0] : 0;
}
void p4_game_elements_ui_destroy(P4GameElementUi *ui)
{
    if (!ui) return;
    release_pins(ui);
    if (ui->ko_host) ui->allocator.release(ui->allocator.user, ui->ko_guest, ui->ko_host, sizeof ko_labels);
    free(ui->arts);
    free(ui);
}
