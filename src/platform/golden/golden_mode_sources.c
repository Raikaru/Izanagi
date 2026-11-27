/* The pinned Steam source and whole-table projection adapter. */
#include "golden_mode_sources.h"
#include "golden_mode_boot.h"
#include "golden_mode_declare_cached.h"
#include "golden_mode_image_bind.h"
#include "p4pc/golden_names_native.h"
#include "golden_battle_rows.h"
#include "p4pc/golden_ai_boot.h"
#include "golden_items_native.h"
#include "p4pc/golden_items.h"
#include "p4pc/golden_encounter_rates.h"
#include "golden_icons.h"
#include "../config/p4cfg.h"
#include "p4pc/frame_timing.h"
#include "p4pc/golden_battle_rows_native.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static P4GoldenModeSources sources;
P4GoldenModeSources *p4_golden_mode_sources_context(void) { return &sources; }
static void release_source(void *context)
{
    P4GoldenModeSources *s = context;
    if (!s) return;
    if (s->release_outputs) s->release_outputs(s);
    p4_golden_mode_outputs_free(&s->outputs);
    SDL_free(s->preference_directory);
    p4_golden_mode_projection_free(&s->projection);
    p4_golden_extra_free(&s->extra);
    p4_golden_free(&s->source);
    memset(s, 0, sizeof *s);
}
void p4_golden_mode_sources_rollback(void *context)
{
    if (!context) return;
    p4_golden_battle_rows_reset();
    p4pc_frame_timing_install(NULL);
    p4_golden_frame_step_reset();
    p4_golden_items_reset();
    release_source(context);
}
static int prepare(void *context, char *reason, size_t capacity)
{
    static const P4GoldenDomainShape shape = {624, 472, 368, 2560, 256};
    P4GoldenModeSources *s = context;
    if (!s || s->source.directory[0] || s->projection.skill_elements.data) {
        if (reason && capacity) snprintf(reason, capacity, "invalid Golden source context");
        return -1;
    }
    if (p4_golden_locate(p4cfg_get("golden.install"), s->directory, sizeof s->directory)) {
        if (reason && capacity) snprintf(reason, capacity, "supported Steam Golden installation not found");
        return -1;
    }
    if (p4_golden_domain_map_build(&s->map, &shape, reason, capacity) ||
        p4_golden_load(&s->source, s->directory, reason, capacity) ||
        p4_golden_extra_load(&s->extra, s->directory, reason, capacity) ||
        p4_golden_mode_identity_load(&s->identity, &s->artifacts, s->directory,
                                     &s->source, &s->extra, &s->map, reason, capacity)) return -1;
    if (p4_golden_mode_project(&s->map, &s->source, &s->projection, reason, capacity)) return -1;
    if (p4_golden_mode_outputs_prepare(&s->outputs, &s->map, &s->source, &s->extra,
                                       &s->projection, &s->artifacts, reason, capacity)) return -1;
    s->preference_directory = SDL_GetPrefPath("p4pc", "Persona4");
    const char *program = SDL_GetBasePath();
    if (!s->preference_directory || !program) {
        if (reason && capacity) snprintf(reason, capacity, "cannot locate user cache or program directory");
        return -1;
    }
    s->paths = (P4GoldenCachePaths){s->preference_directory, s->directory, program, NULL};
    s->cache.paths = &s->paths;
    s->cache.identity = &s->identity.identity;
    s->cache.tables = s->outputs.tables;
    s->cache.table_count = s->outputs.table_count;
    s->cache.extra = s->outputs.extra;
    s->cache.extra_count = s->outputs.extra_count;
    s->cache.declare = p4_golden_ai_cache_declare;
    s->cache.context = &s->map;
    s->cache.capture_image = p4_golden_mode_image_capture;
    s->cache.capture_battle = p4_golden_battle_rows_capture;
    s->cache.capture_items = p4_golden_items_capture;
    return 0;
}
static int publish(void *context, P4GameTablesDefinitionBuilder *builder, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    int result = p4_golden_mode_cache_publish(&s->cache, builder, reason, capacity);
    if (!result) release_source(s);
    return result;
}
static int prepare_image(void *context, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    if (s) for (size_t i = 0; i < s->outputs.table_count; ++i) {
        const P4GoldenCacheTable *t = s->outputs.tables + i;
        if (!strcmp(t->name, "image-index.json") && t->data && t->size) return 0;
    }
    if (reason && capacity) snprintf(reason, capacity, "Golden image object registry is unavailable");
    return -1;
}
static int prepare_literals(void *context, char *reason, size_t capacity)
{
    (void)context;
    return p4_golden_image_literal_prepare(reason, capacity);
}
static int prepare_names(void *context, char *reason, size_t capacity)
{
    const P4GoldenModeSources *s = context;
    if (s && s->projection.skill_names.data && s->projection.persona_names.data &&
        s->projection.skill_names.size == s->map.shape.skills * P4_GOLDEN_SKILL_NAME_SIZE &&
        s->projection.persona_names.size == s->map.shape.personas * P4_GOLDEN_PERSONA_NAME_SIZE) return 0;
    if (reason && capacity) snprintf(reason, capacity, "Golden widened name banks are unavailable");
    return -1;
}
static int passive_elements(void *context, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    const P4GoldenTable *skill = s ? &s->source.tables[P4_GOLDEN_SKILL] : NULL;
    if (!skill || s->map.shape.active_skills != 472u || s->map.shape.skills != 624u ||
        skill->span_count != 2 || skill->spans[0].size != 624u * 2u ||
        !skill->spans[0].bytes || !s->projection.skill_elements.data) goto invalid;
    for (uint32_t id = 472; id < 624; ++id) {
        uint32_t port;
        if (p4_golden_domain_to_port(&s->map, P4_GOLDEN_DOMAIN_SKILL, id, &port, NULL) ||
            skill->spans[0].bytes[id * 2u] != 0xFF ||
            s->projection.skill_elements.size < (size_t)(port + 1u) * 2u ||
            s->projection.skill_elements.data[port * 2u] != 0xFF ||
            s->projection.skill_elements.data[port * 2u + 1u] != skill->spans[0].bytes[id * 2u + 1u])
            goto invalid;
    }
    return 0;
invalid:
    if (reason && capacity) snprintf(reason, capacity, "Golden passive SKILL0 signed NONE cohort mismatch");
    return -1;
}
static int model_rows(void *context, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    if (s) {
        const P4GoldenExtraTable *model = &s->extra.tables[P4_GOLDEN_EXTRA_MODEL];
        if (model->span_count == 3 && model->spans[0].bytes &&
            model->spans[0].size == 11u * 382u) return 0;
    }
    if (reason && capacity) snprintf(reason, capacity, "Golden MODEL0 raw backing is unavailable");
    return -1;
}
static int effect_rows(void *context, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    if (s && s->map.shape.active_skills == 472) {
        const P4GoldenExtraTable *effect = &s->extra.tables[P4_GOLDEN_EXTRA_EFFECT];
        if (effect->span_count == 1 && effect->spans[0].bytes &&
            effect->spans[0].size == 472u * 12u) {
            for (size_t i = 0; i < s->outputs.extra_count; ++i)
                if (!strcmp(s->outputs.extra[i].name, "rows/effect0") &&
                    s->outputs.extra[i].data &&
                    s->outputs.extra[i].size == effect->spans[0].size &&
                    !memcmp(s->outputs.extra[i].data, effect->spans[0].bytes,
                            effect->spans[0].size)) return 0;
        }
    }
    if (reason && capacity) snprintf(reason, capacity, "Golden EFFECT0 full-row publication mismatch");
    return -1;
}
static int effect_timing(void *context, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    if (s && s->cache.capture_battle == p4_golden_battle_rows_capture &&
        s->extra.tables[P4_GOLDEN_EXTRA_EFFECT].span_count == 1 &&
        s->extra.tables[P4_GOLDEN_EXTRA_EFFECT].spans[0].bytes &&
        s->extra.tables[P4_GOLDEN_EXTRA_EFFECT].spans[0].size == 472u * 12u) return 0;
    if (reason && capacity) snprintf(reason, capacity, "Golden signed EFFECT0 timing requires the retained native battle-row lifetime");
    return -1;
}
static int widened_flags(void *context, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    if (!s || s->map.shape.active_skills != 472) goto invalid;
    const P4GoldenSpan *raw = &s->projection.skill_golden;
    if (!raw->bytes || raw->size != 472u * 44u ||
        raw->bytes != s->source.tables[P4_GOLDEN_SKILL].spans[1].bytes ||
        !s->projection.skill_metadata.data) goto invalid;
    for (uint32_t id = 0; id < 472; ++id) {
        uint32_t port;
        if (p4_golden_domain_to_port(&s->map, P4_GOLDEN_DOMAIN_SKILL, id, &port, NULL) ||
            (size_t)port * 40u + 40u > s->projection.skill_metadata.size) goto invalid;
        const uint8_t *row = raw->bytes + id * 44u;
        const uint8_t *native = s->projection.skill_metadata.data + port * 40u;
        if (row[0] != native[0] || memcmp(row + 4, native + 1, 3) ||
            memcmp(row + 8, native + 4, 36)) goto invalid;
    }
    for (size_t i = 0; i < s->outputs.extra_count; ++i)
        if (!strcmp(s->outputs.extra[i].name, "rows/skill1") &&
            s->outputs.extra[i].size == raw->size &&
            !memcmp(s->outputs.extra[i].data, raw->bytes, raw->size)) return 0;
invalid:
    if (reason && capacity) snprintf(reason, capacity, "Golden SKILL1 native projection or full flags/padding sidecar mismatch");
    return -1;
}
static int item_routes(void *context, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    P4GoldenItemSort sort;
    if (!s || s->map.shape.items != 2560 ||
        s->projection.item_metadata.size != 2560u * 68u ||
        p4_golden_item_sort_open(&sort, s->extra.tables[P4_GOLDEN_EXTRA_CMP].bytes,
            s->extra.tables[P4_GOLDEN_EXTRA_CMP].size)) goto invalid;
    for (unsigned i = 0; i < 2560; ++i) {
        const uint8_t *row = s->projection.item_metadata.data + i * 68u;
        uint32_t kind, skill, golden = 0;
        memcpy(&kind, row, sizeof kind);
        memcpy(&skill, row + 0x24, sizeof skill);
        if (p4_golden_item_category(kind) < 0 ||
            (skill && p4_golden_domain_from_port(&s->map,
                P4_GOLDEN_DOMAIN_SKILL, skill, &golden))) goto invalid;
    }
    unsigned gc, pc;
    if (p4_golden_encounter_matrix(s->source.tables[P4_GOLDEN_ENCOUNT].spans[3].bytes,
            s->source.tables[P4_GOLDEN_ENCOUNT].spans[3].size,
            s->projection.encounter[3].data, s->projection.encounter[3].size, &gc, &pc))
        goto invalid;
    return 0;
invalid:
    if (reason && capacity) snprintf(reason, capacity, "Golden item routes/sorts or P10 matrix failed");
    return -1;
}
int p4_golden_mode_sources_register(void)
{
    if (p4_golden_mode_adapter_register("verified sources and projections", prepare)) return -1;
    if (p4_golden_mode_adapter_register("skill and persona name widths", prepare_names)) return -1;
    if (p4_golden_mode_adapter_register("M6 image-table cache binder (chart, gate, recipe, EXP, inheritance)",
                                        prepare_image)) return -1;
    if (p4_golden_mode_adapter_register("image supplier ledger", prepare_literals)) return -1;
    if (p4_golden_mode_adapter_register("passive skill semantic elements", passive_elements)) return -1;
    if (p4_golden_mode_adapter_register("MODEL0 Golden rows (11x382)", model_rows)) return -1;
    if (p4_golden_mode_adapter_register("widened skill flags", widened_flags)) return -1;
    if (p4_golden_mode_adapter_register("EFFECT0 Golden rows (472x12)", effect_rows)) return -1;
    if (p4_golden_mode_adapter_register("R2 EFFECT0 packet timing", effect_timing)) return -1;
    if (p4_golden_ai_adapter_register(P4_GAME_MODE_GOLDEN)) return -1;
    if (p4_golden_mode_adapter_register("expanded item categories and actions", item_routes)) return -1;
    if (p4_golden_icons_register(P4_GAME_MODE_GOLDEN)) return -1;
    if (p4_golden_mode_publisher_register(publish, p4_golden_mode_sources_rollback, &sources)) return -1;
    p4_golden_frame_step_reset();
    p4pc_frame_timing_install(p4_golden_frame_step_publish);
    return 0;
}
