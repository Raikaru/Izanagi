/* One cache-to-image transaction, owned by the actual sealed table builder. */
#include "golden_mode_image_bind.h"
#include "../mods/game_table_golden_attest.h"
#include "p4pc/content_digest.h"
#include "p4pc/golden_fusion_runtime.h"
#include "p4pc/guest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/golden_literals.h"
#include "p4pc/package_boot.h"

typedef struct ImageSession {
    P4GameTablesDefinitionBuilder *owner;
    P4GoldenCacheTable objects[P4_GOLDEN_IMAGE_COUNT];
    char index[P4_GOLDEN_IMAGE_INDEX_CAP], origin[P4_TABLE_KEY_SIZE];
    size_t index_size;
    uint8_t *storage, *backup;
    unsigned attempted, bound;
} ImageSession;
static ImageSession session;
typedef struct WeatherWork {
    struct WeatherWork *next;
    uint32_t work, selected[5];
    unsigned count;
} WeatherWork;
static WeatherWork *weather_works;
static int fail(char *reason, size_t capacity, const char *message)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", message);
    return -1;
}
static const P4GoldenCacheTable *find(const P4GoldenCacheTable *extra, size_t count, const char *name)
{
    const P4GoldenCacheTable *found = NULL;
    for (size_t i = 0; i < count; ++i) if (extra[i].name && !strcmp(extra[i].name, name)) {
        if (found) return NULL;
        found = extra + i;
    }
    return found;
}
static int authentic(const ImageSession *s)
{
    char index[P4_GOLDEN_IMAGE_INDEX_CAP]; size_t size;
    return !p4_golden_image_index(s->objects, P4_GOLDEN_IMAGE_COUNT, index, sizeof index, &size) &&
        size == s->index_size && !memcmp(index, s->index, size);
}
int p4_golden_mode_image_capture(P4GameTablesDefinitionBuilder *b,
    const P4GoldenCacheTable *extra, size_t count, char *reason, size_t capacity)
{
    ImageSession built = {0};
    size_t n, bytes = 0, backup_size = 0;
    const P4GoldenImageObject *registry = p4_golden_image_registry(&n);
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN || !b || !b->sources || b->sealing || b->sealed ||
        session.owner || !extra || !p4_game_table_golden_inputs_attest(b->inputs, b->input_count, 1))
        return fail(reason, capacity, "image capture requires one unpublished verified Golden cache owner");
    const P4GoldenCacheTable *index = find(extra, count, "image-index.json");
    if (!index || !index->data || !index->size || index->size >= sizeof built.index)
        return fail(reason, capacity, "missing Golden image-index.json");
    built.index_size = index->size;
    memcpy(built.index, index->data, index->size);
    for (size_t i = 0; i < n; ++i) {
        const P4GoldenCacheTable *object = find(extra, count, registry[i].name);
        if (!object || !object->data || object->size != registry[i].size ||
            registry[i].size > SIZE_MAX - bytes || registry[i].retail_size > registry[i].size ||
            (registry[i].address && (registry[i].address >= P4_GUEST_SIZE ||
             registry[i].retail_size > P4_GUEST_SIZE - registry[i].address)))
            return fail(reason, capacity, "missing or invalid Golden image object/destination");
        for (size_t j = 0; j < i; ++j)
            if (registry[i].address && registry[j].address &&
                registry[i].address < registry[j].address + registry[j].retail_size &&
                registry[j].address < registry[i].address + registry[i].retail_size)
                return fail(reason, capacity, "overlapping Golden image destinations");
        built.objects[i] = *object;
        bytes += object->size;
        backup_size += registry[i].retail_size;
    }
    if (!authentic(&built)) return fail(reason, capacity, "Golden image manifest/hash mismatch");
    const P4GoldenCacheTable *rates = find(extra, count, "decl/skill-rates");
    const P4GoldenCacheTable *image_rates = find(extra, count, "image/inheritance");
    if (rates && (!rates->data || rates->size != image_rates->size ||
                  memcmp(rates->data, image_rates->data, rates->size)))
        return fail(reason, capacity, "Golden image and declared inheritance rates differ");
    built.storage = malloc(bytes);
    built.backup = malloc(backup_size);
    if (!built.storage || !built.backup) {
        free(built.storage); free(built.backup);
        return fail(reason, capacity, "cannot retain Golden image transaction");
    }
    size_t offset = 0;
    for (size_t i = 0; i < n; ++i) {
        memcpy(built.storage + offset, built.objects[i].data, built.objects[i].size);
        built.objects[i].data = built.storage + offset;
        offset += built.objects[i].size;
    }
    /* A bounded existing native-mode identity commits BOTH the cache key and
     * canonical image manifest. No new save block or non-Golden ledger row. */
    char identity[128], digest[65], manifest_digest[65];
    p4_content_digest(built.index, built.index_size, manifest_digest);
    memcpy(identity, b->inputs[0]->golden_origin + sizeof "golden-cache/" - 1u, 64);
    memcpy(identity + 64, manifest_digest, 64);
    p4_content_digest(identity, sizeof identity, digest);
    snprintf(built.origin, sizeof built.origin, "golden-image:%s", digest);
    uint32_t source;
    if (p4_game_table_sources_native(b->sources, built.origin, 1, 1, &source)) {
        free(built.storage); free(built.backup);
        return fail(reason, capacity, "cannot issue Golden image source ledger origin");
    }
    built.owner = b;
    session = built;
    return 0;
}
static void restore(void)
{
    size_t n, offset = 0;
    const P4GoldenImageObject *registry = p4_golden_image_registry(&n);
    if (session.bound) for (size_t i = 0; i < n; ++i) if (registry[i].address) {
        memcpy(P4_G2H(registry[i].address), session.backup + offset, registry[i].retail_size);
        offset += registry[i].retail_size;
    }
    p4_golden_fusion_reset();
    p4_golden_recipes_clear();
    session.bound = 0;
}
static unsigned u16(const uint8_t *p) { return p[0] | (unsigned)p[1] << 8; }
int p4_golden_mode_image_bind(P4GameMode mode, P4GameTablesDefinitionBuilder *b, char *reason, size_t capacity)
{
    if (mode == P4_GAME_MODE_PS2) return 0;
    if (mode != P4_GAME_MODE_GOLDEN || session.owner != b || !b || !b->sealed ||
        !p4_game_table_sources_is_sealed(b->sources) || !b->native_uses || session.attempted ||
        !p4_game_table_golden_inputs_attest(b->inputs, b->input_count, 1) || !authentic(&session))
        return fail(reason, capacity, "Golden image bind requires its sealed unused cache transaction");
    size_t cursor = 0;
    const P4TableSourceOrigin *origin;
    while ((origin = p4_game_table_sources_native_mode_next(b->sources, &cursor)))
        if (!strcmp(origin->source_key, session.origin) && origin->schema == 1 && origin->revision == 1) break;
    if (!origin) return fail(reason, capacity, "Golden image source was not sealed by its owner");
    session.attempted = 1;
    const P4GoldenCacheTable *charts = find(session.objects, P4_GOLDEN_IMAGE_COUNT, "image/charts");
    const P4GoldenCacheTable *gates = find(session.objects, P4_GOLDEN_IMAGE_COUNT, "image/gates");
    const P4GoldenCacheTable *wire = find(session.objects, P4_GOLDEN_IMAGE_COUNT, "image/recipes");
    P4GoldenRecipe recipes[P4_GOLDEN_RECIPE_MAX] = {0};
    for (unsigned i = 0; i < P4_GOLDEN_RECIPE_MAX; ++i) {
        const uint8_t *p = wire->data + i * 27u;
        recipes[i].result = (uint16_t)u16(p);
        recipes[i].material_count = p[26];
        for (unsigned j = 0; j < 12; ++j) recipes[i].materials[j] = (uint16_t)u16(p + 2u + j * 2u);
    }
    if (p4_golden_fusion_install(mode, charts->data, charts->size, gates->data, gates->size) ||
        p4_golden_recipes_install(recipes, P4_GOLDEN_RECIPE_MAX)) {
        restore();
        return fail(reason, capacity, "Golden widened image installers refused their sealed objects");
    }
    size_t n, offset = 0;
    const P4GoldenImageObject *registry = p4_golden_image_registry(&n);
    for (size_t i = 0; i < n; ++i) if (registry[i].address) {
        void *guest = P4_G2H(registry[i].address);
        memcpy(session.backup + offset, guest, registry[i].retail_size);
        memcpy(guest, session.objects[i].data, registry[i].retail_size);
        offset += registry[i].retail_size;
    }
    session.bound = 1;
    /* Readback authenticates the real loaded destinations, not a planned copy.
     * The image is now the consumer of the issued immutable native origin. */
    for (size_t i = 0; i < n; ++i) if (registry[i].address &&
        memcmp(P4_G2H(registry[i].address), session.objects[i].data, registry[i].retail_size)) {
        restore(); return fail(reason, capacity, "Golden image destination readback differs");
    }
    if (p4_game_table_native_origin_use(b->native_uses, origin)) {
        restore(); return fail(reason, capacity, "Golden image lost its issued native-use owner");
    }
    if (reason && capacity) reason[0] = 0;
    return 0;
}
void p4_golden_mode_image_release(P4GameTablesDefinitionBuilder *b)
{
    if (!session.owner || session.owner != b) return;
    restore();
    free(session.storage); free(session.backup);
    memset(&session, 0, sizeof session);
}
int p4_golden_mode_image_ready(const P4GameTablesDefinitionBuilder *b)
{ return b && session.owner == b && session.bound; }

static const uint8_t *literal_data(const char *name, size_t size)
{
    const P4GoldenCacheTable *object = session.bound ?
        find(session.objects, P4_GOLDEN_IMAGE_COUNT, name) : NULL;
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN || !object || object->size != size)
        p4_package_boot_runtime_failed("Golden literal read requires its sealed bound image");
    return object->data;
}
static uint32_t literal_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
void p4_golden_literal_filter3_extra(uint8_t out[44], const uint8_t record[44])
{
    const uint8_t *fixed = literal_data("image/l1/0063c660", 28);
    uint32_t mode = record[0x2b];
    int32_t first = (int16_t)u16(record + 0x14);
    int32_t last = (int16_t)u16(record + 0x16);
    volatile float converted = (float)record[0x13];
    float weight = converted / 100.0f;
    uint32_t words[3];
    words[0] = mode;
    words[1] = (uint32_t)first;
    memcpy(words + 2, &weight, sizeof weight);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned byte = 0; byte < 4; ++byte)
            out[4u * i + byte] = (uint8_t)(words[i] >> (8u * byte));
    memcpy(out + 12, fixed, 28);
    for (unsigned byte = 0; byte < 4; ++byte)
        out[40u + byte] = (uint8_t)((uint32_t)last >> (8u * byte));
}
int32_t p4_golden_literal_key_action(uint32_t key)
{
    const uint8_t *data = literal_data("image/l1/0063cb50", 192u * 8u);
    for (unsigned i = 0; i < 192; ++i)
        if (literal_u32(data + i * 8u) == key) return (int32_t)literal_u32(data + i * 8u + 4u);
    return 3;
}
uint32_t p4_golden_literal_message(unsigned month, int column)
{
    if (month < 1 || month > 12 || column < 0 || column > 10)
        p4_package_boot_runtime_failed("Golden monthly selector domain");
    const uint8_t *selectors = literal_data("image/l1/006417e0", 132);
    const uint8_t *messages = literal_data("image/l1/006417e0-map", 20);
    unsigned selector = selectors[(month - 1u) * 11u + (unsigned)column];
    if (selector >= 5) p4_package_boot_runtime_failed("Golden monthly message selector domain");
    return literal_u32(messages + selector * 4u);
}
static float literal_float(const char *name, unsigned rank)
{
    if (rank > 10) p4_package_boot_runtime_failed("Golden community rank domain");
    uint32_t bits = literal_u32(literal_data(name, 44) + rank * 4u);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}
float p4_golden_literal_scale(unsigned rank) { return literal_float("image/l1/00749500", rank); }
float p4_golden_literal_weather(unsigned rank) { return literal_float("image/l1/007494d0", rank); }
static int literal_percent(uint32_t random)
{
    volatile float converted = (float)(random & 0x7fffffffu);
    volatile float normalized = converted * 0x1p-31f;
    volatile float percent = normalized * 100.0f;
    return (int)percent;
}
int p4_golden_literal_roll_index(int level, uint32_t stage_random, uint32_t direction_random)
{
    int percent = literal_percent(stage_random);
    int stage = percent < 40 ? 0 : percent < 65 ? 1 : percent < 85 ? 2 : percent < 95 ? 3 : 4;
    int adjusted = literal_percent(direction_random) <= 29 ? level - stage : level + stage;
    if (adjusted < 1) adjusted = 1;
    if (adjusted > 99) adjusted = 99;
    return (adjusted - 1) / 10;
}

static const uint8_t *literal_date_row(int index)
{
    if ((unsigned)index >= 127u)
        p4_package_boot_runtime_failed("Golden dated weather index domain");
    return literal_data("image/l1/fusion-weather", 128u * 28u) + (unsigned)index * 28u;
}
int p4_golden_literal_date_find(int month, int day)
{
    const uint8_t *data = literal_data("image/l1/fusion-weather", 128u * 28u);
    for (unsigned i = 1; i < 128; ++i) {
        const uint8_t *row = data + i * 28u;
        if ((int8_t)row[0] == -1 && (int8_t)row[1] == -1) return -1;
        if ((int8_t)row[0] == month && (int8_t)row[1] == day) return (int)i;
    }
    return -1;
}
int32_t p4_golden_literal_date_constraint(int index, unsigned field)
{
    if (field > 2u)
        p4_package_boot_runtime_failed("Golden dated weather constraint field");
    const uint8_t *row = literal_date_row(index);
    if (!field) return (int8_t)row[2];
    row += field == 1 ? 4 : 6;
    return (int16_t)((uint16_t)row[0] | (uint16_t)row[1] << 8);
}
uint32_t p4_golden_literal_date_effect(int index, unsigned slot)
{
    if (slot >= 5u)
        p4_package_boot_runtime_failed("Golden dated weather effect slot");
    return literal_u32(literal_date_row(index) + 8u + slot * 4u);
}

static WeatherWork *literal_weather_work(uint32_t work)
{
    for (WeatherWork *state = weather_works; state; state = state->next)
        if (state->work == work) return state;
    p4_package_boot_runtime_failed("unregistered Golden fusion weather work");
}
void p4_golden_literal_weather_work_register(uint32_t work)
{
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN || !work)
        p4_package_boot_runtime_failed("invalid Golden fusion weather work");
    for (WeatherWork *state = weather_works; state; state = state->next)
        if (state->work == work)
            p4_package_boot_runtime_failed("duplicate Golden fusion weather work");
    WeatherWork *state = malloc(sizeof *state);
    if (!state) p4_package_boot_runtime_failed("cannot retain Golden fusion weather effects");
    state->work = work;
    state->count = 0;
    state->next = weather_works;
    weather_works = state;
}
int p4_golden_literal_weather_work_release(uint32_t work)
{
    for (WeatherWork **link = &weather_works; *link; link = &(*link)->next) {
        WeatherWork *state = *link;
        if (state->work != work) continue;
        *link = state->next;
        free(state);
        return 0;
    }
    return -1;
}
unsigned p4_golden_literal_weather_selected_count(uint32_t work)
{
    return literal_weather_work(work)->count;
}
uint32_t p4_golden_literal_weather_selected_effect(uint32_t work, unsigned slot)
{
    WeatherWork *state = literal_weather_work(work);
    if (slot >= state->count)
        p4_package_boot_runtime_failed("Golden fusion weather selected-effect domain");
    return state->selected[slot];
}

void p4_golden_literal_weather_roll(uint32_t work, int index, uint32_t (*random)(void))
{
    WeatherWork *state = literal_weather_work(work);
    const uint8_t *row = literal_date_row(index);
    uint32_t candidates[5];
    unsigned count = 0, total = 0;
    state->count = 0;
    for (unsigned i = 0; i < 5; ++i) {
        uint32_t effect = literal_u32(row + 8u + i * 4u);
        unsigned kind = effect & 255u, weight = (effect >> 8) & 255u;
        if (!kind) continue;
        if (weight == 100u) state->selected[state->count++] = effect;
        else {
            candidates[count++] = effect;
            total += weight;
        }
    }
    /* Zero weights are candidates, but cannot select an effect or consume RNG. */
    if (total) {
        volatile float masked = (float)(random() & 0x7fffffffu);
        volatile float uniform = masked * 0x1p-31f;
        volatile float scaled = uniform * (float)total;
        int16_t roll = (int16_t)(uint32_t)scaled, cumulative = 0;
        for (unsigned i = 0; i < count; ++i) {
            cumulative = (int16_t)(cumulative + ((candidates[i] >> 8) & 255u));
            if (roll < cumulative) {
                state->selected[state->count++] = candidates[i];
                break;
            }
        }
    }
    uint8_t *legacy = P4_G2H(work);
    for (unsigned i = 0; i < state->count; ++i)
        legacy[0x2dau + i] = (uint8_t)state->selected[i];
    legacy[0x2df] = (uint8_t)state->count;
}
