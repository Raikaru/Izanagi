/* The pinned Steam source and whole-table projection adapter. */
#include "golden_mode_sources.h"
#include "golden_mode_boot.h"
#include "golden_mode_declare_cached.h"
#include "../config/p4cfg.h"
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

static P4GoldenModeSources sources;
P4GoldenModeSources *p4_golden_mode_sources_context(void) { return &sources; }
void p4_golden_mode_sources_rollback(void *context)
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
    if (s->artifacts.fusion.unsupported_twelfth)
        fprintf(stderr, "Golden: omitting %u fusion recipes requiring 12 materials (M5b)\n",
                s->artifacts.fusion.unsupported_twelfth);
    fprintf(stderr, "Golden: 28-arcana fusion charts require M5b and are not published\n");
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
    s->cache.declare = p4_golden_mode_declare_cached;
    s->cache.context = &s->map;
    return 0;
}
static int publish(void *context, P4GameTablesDefinitionBuilder *builder, char *reason, size_t capacity)
{
    P4GoldenModeSources *s = context;
    int result = p4_golden_mode_cache_publish(&s->cache, builder, reason, capacity);
    if (!result) p4_golden_mode_sources_rollback(s);
    return result;
}
int p4_golden_mode_sources_register(void)
{
    if (p4_golden_mode_adapter_register("verified sources and projections", prepare)) return -1;
    return p4_golden_mode_publisher_register(publish, p4_golden_mode_sources_rollback, &sources);
}
