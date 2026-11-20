/* Shared ordered registry of the native amount stages (game_stage_internal.h). */
#include "game_stage_internal.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int bounded_owner(const char *owner)
{
    if (!owner || !owner[0]) return 0;
    for (size_t i = 0; owner[i]; ++i)
        if (i + 1u >= GAME_STAGE_OWNER_SIZE || owner[i] < 0x21 || owner[i] > 0x7e) return 0;
    return 1;
}
static int before(const GameStageEntry *a, const GameStageEntry *b)
{
    if (a->priority != b->priority) return a->priority < b->priority;
    if (a->kind != b->kind) return a->kind == GAME_STAGE_MULTIPLY;
    if (a->load_order != b->load_order) return a->load_order < b->load_order;
    return a->sequence < b->sequence;
}
int game_stage_register(GameStage *stage, const char *owner, int32_t priority, uint32_t load_order,
    uint32_t kind, GameStageFn fn, void *user, uint32_t *handle)
{
    if (!stage || !handle || !fn || !bounded_owner(owner) ||
        (kind != GAME_STAGE_MULTIPLY && kind != GAME_STAGE_ADD)) return GAME_STAGE_INVALID;
    if (stage->composing) return GAME_STAGE_BUSY;
    if (stage->count == stage->capacity) {
        const size_t grown = stage->capacity ? stage->capacity * 2u : 8u;
        GameStageEntry *next = realloc(stage->entries, grown * sizeof *next);
        if (!next) return GAME_STAGE_NOMEM;
        stage->entries = next;
        stage->capacity = grown;
    }
    if (!stage->next_handle) stage->next_handle = 1u;
    GameStageEntry entry = {.priority = priority, .load_order = load_order, .kind = kind,
        .handle = stage->next_handle++, .sequence = stage->next_sequence++, .fn = fn, .user = user};
    strcpy(entry.owner, owner);
    size_t at = stage->count;
    while (at > 0 && before(&entry, &stage->entries[at - 1u])) --at;
    memmove(&stage->entries[at + 1u], &stage->entries[at], (stage->count - at) * sizeof *stage->entries);
    stage->entries[at] = entry;
    ++stage->count;
    *handle = entry.handle;
    return GAME_STAGE_OK;
}
int game_stage_unregister(GameStage *stage, uint32_t handle)
{
    if (!stage) return GAME_STAGE_INVALID;
    if (stage->composing) return GAME_STAGE_BUSY;
    for (size_t i = 0; i < stage->count; ++i) {
        if (stage->entries[i].handle != handle) continue;
        memmove(&stage->entries[i], &stage->entries[i + 1u], (stage->count - i - 1u) * sizeof *stage->entries);
        if (!--stage->count) {
            free(stage->entries);
            stage->entries = NULL;
            stage->capacity = 0;
        }
        return GAME_STAGE_OK;
    }
    return GAME_STAGE_NOT_FOUND;
}
size_t game_stage_count(const GameStage *stage) { return stage ? stage->count : 0u; }
int game_stage_trace(GameStage *stage)
{
    if (stage->trace == 0) {
        const char *value = stage->trace_variable ? getenv(stage->trace_variable) : NULL;
        stage->trace = value && value[0] && strcmp(value, "0") ? 1 : -1;
    }
    return stage->trace > 0;
}
int game_stage_compose(GameStage *stage, const void *hit, double stock, double *value)
{
    *value = stock;
    if (stage->composing) return 0;
    const int trace = game_stage_trace(stage);
    stage->composing = 1;
    double current = stock;
    for (size_t i = 0; i < stage->count; ++i) {
        GameStageEntry *e = &stage->entries[i];
        double step = 0.0;
        if (!stage->invoke(e->fn, e->user, hit, &step)) continue;
        if (!isfinite(step) || (e->kind == GAME_STAGE_MULTIPLY && step < 0.0)) {
            if (!e->reported) {
                fprintf(stderr, "p4pc: %s: %s returned an invalid %s; ignored for that hit\n",
                    stage->name, e->owner, e->kind == GAME_STAGE_MULTIPLY ? "factor" : "addend");
                e->reported = 1u;
            }
            continue;
        }
        const double prior = current;
        current = e->kind == GAME_STAGE_MULTIPLY ? current * step : current + step;
        /* Wide bounded intermediate: stays finite whatever the step count. */
        if (current > 1e18) current = 1e18;
        else if (current < -1e18) current = -1e18;
        if (trace)
            fprintf(stderr, "p4pc: %s:   %s priority %d load %u %s %.17g: %.17g -> %.17g\n",
                stage->name, e->owner, (int)e->priority, (unsigned)e->load_order,
                e->kind == GAME_STAGE_MULTIPLY ? "multiply" : "add", step, prior, current);
    }
    stage->composing = 0;
    *value = current;
    return 1;
}
