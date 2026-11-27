#include "p4pc/golden_table_cache.h"
#include "p4pc/game_mode.h"
#include "../mods/game_table_inputs_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compare_tables(const void *a, const void *b)
{
    return strcmp(((const P4GoldenCacheTable *)a)->name, ((const P4GoldenCacheTable *)b)->name);
}
int p4_game_tables_builder_golden_cache(P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCachePaths *paths, const P4GoldenCacheIdentity *identity,
    P4GoldenCacheTable *extra, size_t extra_count, char *reason, size_t capacity)
{
    P4GameTableInput *prepared[P4_TABLE_INPUT_COUNT] = {0};
    P4GoldenCacheTable *tables = NULL;
    char key[65]; int result = P4_TABLE_INVALID;
    if ((extra_count && !extra) || extra_count > 4087u) return P4_TABLE_INVALID;
    for (size_t i = 0; i < extra_count; ++i) { extra[i].data = NULL; extra[i].size = 0; }
    for (size_t i = 0; i < extra_count; ++i)
        if (!extra[i].name || (i && strcmp(extra[i-1].name,extra[i].name) >= 0)) return P4_TABLE_INVALID;
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN || !builder || !builder->tables || !builder->sources ||
        !builder->registry_revision || builder->sealing || builder->sealed || builder->input_count ||
        (builder->input_mask & ~((1u << P4_TABLE_INPUT_COUNT) - 1u))) {
        if (reason && capacity) snprintf(reason,capacity,"Golden cache requires an empty Golden-mode builder");
        return P4_TABLE_INVALID;
    }
    size_t count = P4_TABLE_INPUT_COUNT + extra_count;
    tables = calloc(count,sizeof *tables);
    if (!tables) return P4_TABLE_NOMEM;
    for (size_t i = 0; i < P4_TABLE_INPUT_COUNT; ++i) tables[i].name = p4_game_table_input_label((P4GameTableInputKind)i);
    for (size_t i = 0; i < extra_count; ++i) tables[P4_TABLE_INPUT_COUNT+i].name = extra[i].name;
    qsort(tables,count,sizeof *tables,compare_tables);
    int loaded = p4_golden_cache_load(paths,identity,tables,count,key,reason,capacity);
    if (loaded) { result = loaded > 0 ? P4_TABLE_NOT_FOUND : P4_TABLE_INVALID; goto done; }
    for (size_t i = 0; i < P4_TABLE_INPUT_COUNT; ++i) {
        const char *name = p4_game_table_input_label((P4GameTableInputKind)i);
        size_t at = 0; while (at < count && strcmp(name,tables[at].name)) ++at;
        if (at == count || !(prepared[i] = p4_game_table_input_from_golden_cache(
            (P4GameTableInputKind)i,tables[at].data,tables[at].size,key))) {
            if (reason && capacity) snprintf(reason,capacity,"invalid cached table %s",name);
            goto done;
        }
    }
    for (size_t i = 0; i < P4_TABLE_INPUT_COUNT; ++i) {
        builder->inputs[builder->input_count++] = prepared[i];
        prepared[i] = NULL;
        builder->input_mask |= 1u << i;
    }
    for (size_t i = 0; i < extra_count; ++i) {
        size_t at = 0; while (at < count && strcmp(extra[i].name,tables[at].name)) ++at;
        extra[i].data = tables[at].data; extra[i].size = tables[at].size;
        tables[at].data = NULL; tables[at].size = 0;
    }
    result = P4_TABLE_OK;
done:
    for (size_t i = 0; i < P4_TABLE_INPUT_COUNT; ++i) p4_game_table_input_destroy(prepared[i]);
    p4_golden_cache_free(tables,count);
    free(tables);
    return result;
}
