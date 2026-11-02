/* Element-UI model fixture: layout validation, CPU plan and alias coverage, the
 * descriptor art set, CPU image checks, real resource token pins and the marker
 * stream. Native draws, the boot provider and captures are separate gates. */
#include "p4pc/game_elements_ui.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Scripted dependencies outside this unit. The natural-route predicate is the
 * element owner's; this stand-in answers the original retail pairs only. */
int p4_game_elements_profile_natural_pair(const P4GameTables *tables, uint32_t leaf,
    uint32_t channel, uint32_t component)
{
    (void)tables;
    if (leaf < 6u) return channel == P4_CHANNEL_DAMAGE && component == P4_ACTION_COMPONENT_DAMAGE;
    if (leaf == 6u || leaf == 7u) return channel == P4_CHANNEL_INSTANT_KO && component == P4_ACTION_COMPONENT_INSTANT_KO;
    return 0;
}
struct P4ResourceImagesPrepared { const P4ResourceRef *refs; size_t count; };
int p4_resources_images_prepared_find(const P4ResourceImagesPrepared *prepared,
    const char *owner, const char *key, uint32_t revision)
{
    if (!prepared || !owner || !key) return -1;
    for (size_t i = 0; i < prepared->count; ++i)
        if (!strcmp(prepared->refs[i].key, key))
            return prepared->refs[i].revision == revision && !strcmp(prepared->refs[i].owner, owner);
    return 0;
}
static unsigned rasters;
int p4_resources_raster_create(const struct P4ContentSource *src, const uint8_t *rgba,
    uint32_t width, uint32_t height, uint32_t *raster, uint32_t *pixels, uint32_t *stride, uint32_t *rows)
{
    (void)src;
    uint8_t *shadow = p4t_galloc(width * height * 4u, 16); assert(shadow);
    memcpy(shadow, rgba, width * height * 4u);
    *stride = width * 4u; *rows = height;
    *pixels = P4_H2G(shadow); *raster = P4_H2G(p4t_galloc(64, 16));
    ++rasters;
    return P4_RESOURCE_OK;
}
void p4_resources_raster_destroy(uint32_t raster) { assert(raster); --rasters; }
int p4_resources_draw(uint32_t token, float x, float y, float depth, uint32_t rgba, float sx, float sy)
{
    (void)token; (void)x; (void)y; (void)depth; (void)rgba; (void)sx; (void)sy;
    assert(!"element UI model fixture never draws");
    return P4_RESOURCE_RENDER_ERROR;
}
static void *guest_allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user;
    void *host = p4t_galloc(size, 16);
    if (host) *guest = P4_H2G(host);
    return host;
}
static void guest_release(void *user, uint32_t guest, void *host, uint32_t size)
{
    (void)user; (void)guest; (void)host; (void)size; /* Fixture arena. */
}
static const P4GameTableAllocator allocator = {NULL, guest_allocate, guest_release};

enum { SLASH = 19u };
static const P4ResourceRef slash_icon = {"phys.split", "phys.split:slash", 1};
typedef struct Scenario {
    int slash_icon, slash_visible_without_icon, enemy_slash_row, persona_slash_row, light_damage_rows;
} Scenario;
static void row(P4GameTables *tables, uint32_t kind, uint32_t id, uint32_t profile,
    uint32_t leaf, uint32_t channel, uint32_t value)
{
    P4AffinityRow affinity = {kind, id, profile, leaf, channel, value, 100};
    assert(p4_game_tables_affinity(tables, "phys.split", &affinity) == P4_TABLE_OK);
}
static void alias(P4GameTables *tables, uint32_t kind, uint32_t id, uint32_t native_profile,
    uint32_t profile, const char *key)
{
    P4AffinityProfileAlias value = {kind, id, native_profile, profile, id, 1, 1, ""};
    strcpy(value.key, key);
    assert(p4_game_tables_affinity_alias(tables, "phys.split", &value) == P4_TABLE_OK);
}
static P4GameTables *tables_for(const Scenario *scenario)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    P4ElementDesc slash = {.id = SLASH, .flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE |
        P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, .family = 0, .order = 19, .revision = 1,
        .key = "phys.split:slash", .display_name = "Slash", .short_name = "Slash"};
    if (scenario->slash_icon) slash.icon = slash_icon;
    assert(p4_game_tables_element(tables, "phys.split", &slash) == P4_TABLE_OK);
    if (scenario->slash_visible_without_icon) {
        P4ElementDesc pierce = {.id = 20, .flags = P4_ELEMENT_OFFENSIVE | P4_ELEMENT_VISIBLE, .order = 20,
            .revision = 1, .key = "phys.split:mark", .display_name = "Mark", .short_name = "Mark"};
        assert(p4_game_tables_element(tables, "phys.split", &pierce) == P4_TABLE_OK);
    }
    alias(tables, P4_ENTITY_ENEMY, 1, 1, 10, "phys.split:enemy-one");
    /* A persona's semantic profile ID is free (the E2E package uses 0x1000+p);
     * its legacy columns still read the native snapshot at native profile 0. */
    alias(tables, P4_ENTITY_PERSONA, 30, 0, 0x101eu, "phys.split:persona-thirty");
    if (scenario->enemy_slash_row) row(tables, P4_ENTITY_ENEMY, 1, 10, SLASH, P4_CHANNEL_DAMAGE, P4_AFF_WEAK | 125u);
    if (scenario->persona_slash_row) row(tables, P4_ENTITY_PERSONA, 30, 0x101eu, SLASH, P4_CHANNEL_DAMAGE, 100);
    if (scenario->light_damage_rows) {
        row(tables, P4_ENTITY_ENEMY, 1, 10, 6, P4_CHANNEL_DAMAGE, P4_AFF_RESIST | 50u);
        row(tables, P4_ENTITY_PERSONA, 30, 0x101eu, 6, P4_CHANNEL_DAMAGE, 100);
    }
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    return tables;
}
static P4GameElementUiColumn column(uint32_t leaf, uint32_t component, uint32_t channel)
{
    return (P4GameElementUiColumn){leaf, component, channel, 0, 0, 0, 0, 0, 0, 0};
}
/* Physical Split: three physical leaves (one here) then the six magic leaves,
 * Light as damage with its independent KO marker; Phys0 omitted. */
static uint32_t split_layout(P4GameElementUiColumn *columns)
{
    uint32_t count = 0;
    columns[count++] = column(SLASH, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE);
    for (uint32_t i = 1; i < 5; ++i) columns[count++] = p4_game_elements_ui_retail_columns[i];
    P4GameElementUiColumn light = column(6, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE);
    light.marker_valid = 1;
    light.marker_component = P4_ACTION_COMPONENT_INSTANT_KO;
    light.marker_channel_id = P4_CHANNEL_INSTANT_KO;
    light.legacy_marker_valid = 1;
    light.legacy_marker_selector = 5;
    columns[count++] = light;
    columns[count++] = p4_game_elements_ui_retail_columns[6];
    return count;
}

static void layout_validation(void)
{
    P4GameElementUi *ui = p4_game_elements_ui_create(&allocator); assert(ui);
    P4GameElementUiColumn columns[10];
    for (int i = 0; i < 10; ++i) columns[i] = p4_game_elements_ui_retail_columns[i % 7];
    assert(p4_game_elements_ui_layout(ui, "test", columns, 0, 1) == P4_TABLE_INVALID);
    assert(p4_game_elements_ui_layout(ui, "test", columns, 10, 1) == P4_TABLE_INVALID);
    assert(p4_game_elements_ui_layout(ui, "test", columns, 7, 0) == P4_TABLE_INVALID);
    P4GameElementUiColumn bad = column(1, 3, P4_CHANNEL_DAMAGE);
    assert(p4_game_elements_ui_layout(ui, "test", &bad, 1, 1) == P4_TABLE_INVALID);
    bad = column(6, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE);
    bad.marker_valid = 1; bad.marker_component = P4_ACTION_COMPONENT_DAMAGE; bad.marker_channel_id = P4_CHANNEL_INSTANT_KO;
    assert(p4_game_elements_ui_layout(ui, "test", &bad, 1, 1) == P4_TABLE_INVALID); /* Marker is instant KO. */
    bad.marker_component = P4_ACTION_COMPONENT_INSTANT_KO; bad.marker_channel_id = P4_CHANNEL_DAMAGE;
    assert(p4_game_elements_ui_layout(ui, "test", &bad, 1, 1) == P4_TABLE_INVALID); /* Distinct channel. */
    bad = column(2, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE);
    bad.legacy_primary_valid = 1; bad.legacy_primary_selector = 1; /* Fire's word on Ice. */
    assert(p4_game_elements_ui_layout(ui, "test", &bad, 1, 1) == P4_TABLE_INVALID);
    bad = column(6, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE);
    bad.legacy_primary_valid = 1; bad.legacy_primary_selector = 5; /* KO word never feeds Light damage. */
    assert(p4_game_elements_ui_layout(ui, "test", &bad, 1, 1) == P4_TABLE_INVALID);
    bad = column(6, P4_ACTION_COMPONENT_INSTANT_KO, P4_CHANNEL_INSTANT_KO);
    bad.legacy_marker_valid = 1; bad.legacy_marker_selector = 5; /* No marker declared. */
    assert(p4_game_elements_ui_layout(ui, "test", &bad, 1, 1) == P4_TABLE_INVALID);
    columns[1] = columns[0];
    assert(p4_game_elements_ui_layout(ui, "test", columns, 2, 1) == P4_TABLE_CONFLICT);
    assert(p4_game_elements_ui_layout(ui, "test", p4_game_elements_ui_retail_columns, 7, 1) == P4_TABLE_OK);
    assert(p4_game_elements_ui_layout(ui, "test", p4_game_elements_ui_retail_columns, 7, 1) == P4_TABLE_OK);
    assert(p4_game_elements_ui_layout(ui, "other", p4_game_elements_ui_retail_columns, 7, 1) == P4_TABLE_CONFLICT);
    assert(p4_game_elements_ui_layout(ui, "test", p4_game_elements_ui_retail_columns, 6, 1) == P4_TABLE_CONFLICT);
    assert(!p4_game_elements_ui_count(ui) && !p4_game_elements_ui_column(ui, 0));
    p4_game_elements_ui_destroy(ui);
}
static int plan(const Scenario *scenario, const P4GameElementUiColumn *columns, uint32_t count,
    P4GameTables **tables, P4GameElementUi **ui)
{
    *tables = tables_for(scenario);
    *ui = p4_game_elements_ui_create(&allocator); assert(*ui);
    assert(p4_game_elements_ui_layout(*ui, "phys.split", columns, count, 3) == P4_TABLE_OK);
    return p4_game_elements_ui_prepare(*ui, *tables);
}
static void discard(P4GameTables *tables, P4GameElementUi *ui)
{
    p4_game_elements_ui_destroy(ui);
    p4_game_tables_destroy(tables);
}
static void plan_and_coverage(void)
{
    P4GameElementUiColumn columns[P4_ELEMENT_UI_MAX_COLUMNS];
    const uint32_t count = split_layout(columns);
    const Scenario complete = {1, 0, 1, 1, 1};
    P4GameTables *tables;
    P4GameElementUi *ui;
    /* A visible new affinity leaf needs a column even under the retail row. */
    assert(plan(&complete, p4_game_elements_ui_retail_columns, 7, &tables, &ui) == P4_TABLE_PREPARE_REQUIRED);
    discard(tables, ui);
    Scenario scenario = complete; scenario.slash_icon = 0;
    assert(plan(&scenario, columns, count, &tables, &ui) == P4_TABLE_NOT_FOUND); /* No art. */
    discard(tables, ui);
    scenario = complete; scenario.slash_visible_without_icon = 1;
    assert(plan(&scenario, columns, count, &tables, &ui) == P4_TABLE_NOT_FOUND); /* Visible, no art. */
    discard(tables, ui);
    scenario = complete; scenario.enemy_slash_row = 0;
    assert(plan(&scenario, columns, count, &tables, &ui) == P4_TABLE_NOT_FOUND); /* Enemy has no answer. */
    discard(tables, ui);
    scenario = complete; scenario.persona_slash_row = 0;
    assert(plan(&scenario, columns, count, &tables, &ui) == P4_TABLE_NOT_FOUND); /* No persona snapshot word. */
    discard(tables, ui);
    scenario = complete; scenario.light_damage_rows = 0;
    assert(plan(&scenario, columns, count, &tables, &ui) == P4_TABLE_NOT_FOUND); /* Light damage is not KO. */
    discard(tables, ui);
    assert(plan(&complete, columns, count, &tables, &ui) == P4_TABLE_OK);
    assert(p4_game_elements_ui_prepare(ui, tables) == P4_TABLE_OK);
    assert(p4_game_elements_ui_layout(ui, "phys.split", columns, count, 3) == P4_TABLE_FROZEN);
    assert(!p4_game_elements_ui_count(ui)); /* Planning publishes no prepared column. */
    P4GameTables *other = tables_for(&complete);
    assert(p4_game_elements_ui_prepare(ui, other) == P4_TABLE_FROZEN);
    p4_game_tables_destroy(other);
    const P4ResourceRef stale = {"phys.split", "phys.split:slash", 2};
    P4ResourceImagesPrepared images = {&stale, 1};
    assert(p4_game_elements_ui_check_images(ui, NULL) == P4_TABLE_NOT_FOUND);
    assert(p4_game_elements_ui_check_images(ui, &images) == P4_TABLE_NOT_FOUND);
    images.refs = &slash_icon;
    assert(p4_game_elements_ui_check_images(ui, &images) == P4_TABLE_OK);
    discard(tables, ui);
}
static const P4ContentSource source = {.origin = P4_TEXTURE_OVERRIDE, .rank = 3, .provider = "phys.split", .key = "icons/slash.png"};
static P4Resources *resources_with(uint32_t revision, uint32_t *token)
{
    static uint8_t rgba[4 * 4 * 4];
    P4Resources *resources = p4_resources_create(4096); assert(resources);
    assert(p4_resources_image(resources, "phys.split", "phys.split:slash", &source, rgba, sizeof rgba,
        4, 4, revision, token) == P4_RESOURCE_OK);
    assert(p4_resources_freeze(resources, 1) == P4_RESOURCE_OK);
    return resources;
}
static void freeze_and_pins(void)
{
    P4GameElementUiColumn columns[P4_ELEMENT_UI_MAX_COLUMNS];
    const uint32_t count = split_layout(columns);
    const Scenario complete = {1, 0, 1, 1, 1};
    P4GameTables *tables;
    P4GameElementUi *ui;
    uint32_t token;
    /* Wrong authored revision refuses and leaves nothing pinned. */
    assert(plan(&complete, columns, count, &tables, &ui) == P4_TABLE_OK);
    P4Resources *resources = resources_with(2, &token);
    assert(p4_game_elements_ui_freeze(ui, resources) == P4_TABLE_CONFLICT);
    assert(!p4_game_elements_ui_count(ui));
    assert(p4_resources_retire_unpinned(resources) == P4_RESOURCE_OK);
    assert(p4_game_elements_ui_freeze(ui, NULL) == P4_TABLE_NOT_READY);
    discard(tables, ui);

    assert(plan(&complete, columns, count, &tables, &ui) == P4_TABLE_OK);
    resources = resources_with(1, &token);
    assert(p4_game_elements_ui_freeze(ui, resources) == P4_TABLE_OK);
    assert(p4_game_elements_ui_freeze(ui, resources) == P4_TABLE_FROZEN);
    assert(p4_game_elements_ui_count(ui) == count && p4_game_elements_ui_revision(ui) == 3);
    assert(p4_game_elements_ui_tables(ui) == tables);
    const P4GameElementUiPreparedColumn *slash = p4_game_elements_ui_column(ui, 0);
    assert(slash->art_kind == P4_ELEMENT_UI_AUTHORED_ART && slash->icon_token == token);
    assert(slash->image_width == 4 && slash->image_height == 4);
    assert(slash->primary.valid && slash->primary.element_id == SLASH && !slash->marker.valid);
    const P4GameElementUiPreparedColumn *fire = p4_game_elements_ui_column(ui, 1);
    assert(fire->art_kind == P4_ELEMENT_UI_RETAIL_ART && fire->native_affinity_sprite == 0x2fu);
    const P4GameElementUiPreparedColumn *light = p4_game_elements_ui_column(ui, 5);
    assert(light->primary.channel_id == P4_CHANNEL_DAMAGE && light->primary_kind == P4_CHANNEL_PERCENT);
    assert(light->marker.valid && light->marker.channel_id == P4_CHANNEL_INSTANT_KO);
    assert(light->native_affinity_sprite == 0x33u);
    assert(p4_game_elements_ui_column(ui, 6)->native_affinity_sprite == 0x34u);
    uint32_t art = 99;
    assert(p4_game_elements_ui_art_find(ui, SLASH, &art) == P4_TABLE_OK && art == 0);
    art = 99;
    assert(p4_game_elements_ui_art_find(ui, 1, &art) == P4_TABLE_NOT_FOUND && art == 99);
    const uint32_t first = p4_game_elements_ui_ko_text(ui, P4_KO_NORMAL);
    assert(first && p4_game_elements_ui_ko_text(ui, P4_ELEMENT_UI_KO_UNKNOWN) == first + 20u);
    assert(!p4_game_elements_ui_ko_text(ui, P4_ELEMENT_UI_KO_UNKNOWN + 1u));
    assert(!strcmp((const char *)P4_G2H(first + 15u), "KO:X"));
    /* Pins live until destroy, so the generation cannot retire under the UI. */
    assert(p4_resources_retire_unpinned(resources) == P4_RESOURCE_CONFLICT);
    discard(tables, ui);
    assert(p4_resources_retire_unpinned(resources) == P4_RESOURCE_OK && !rasters);

    /* The retail row needs no resource service and no marker stream. */
    const Scenario retail = {1, 0, 1, 1, 0};
    P4GameElementUiColumn retail_columns[8];
    retail_columns[0] = column(SLASH, P4_ACTION_COMPONENT_DAMAGE, P4_CHANNEL_DAMAGE);
    memcpy(retail_columns + 1, p4_game_elements_ui_retail_columns, sizeof p4_game_elements_ui_retail_columns);
    assert(plan(&retail, retail_columns, 8, &tables, &ui) == P4_TABLE_OK);
    resources = resources_with(1, &token);
    assert(p4_game_elements_ui_freeze(ui, resources) == P4_TABLE_OK);
    assert(p4_game_elements_ui_column(ui, 1)->native_affinity_sprite == 0x2eu);
    assert(!p4_game_elements_ui_ko_text(ui, P4_KO_NORMAL));
    discard(tables, ui);
    assert(p4_resources_retire_unpinned(resources) == P4_RESOURCE_OK && !rasters);
}

int main(void)
{
    if (p4t_guest_init()) return 77;
    layout_validation();
    plan_and_coverage();
    freeze_and_pins();
    puts("game_elements_ui: layout, CPU coverage, art set, exact image pins and marker stream passed; no draw/boot/capture claim");
    return 0;
}
