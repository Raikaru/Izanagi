/* Publish only cache-verified Golden inputs and declarations. */
#include "golden_mode_cache_publish.h"
#include "p4pc/game_data_catalog.h"
#include <string.h>
#include <stdio.h>

static int fail(char *reason, size_t capacity, const char *text)
{
    if (reason && capacity) snprintf(reason, capacity, "%s", text);
    return -1;
}
int p4_golden_mode_cache_publish(const P4GoldenModeCachePlan *plan, P4GameTablesDefinitionBuilder *builder,
                                  char *reason, size_t capacity)
{
    char key[65];
    if (!plan || !builder || !plan->paths || !plan->identity || !plan->tables || !plan->table_count ||
        plan->table_count > 4096u || (plan->extra_count && !plan->extra) || !plan->declare)
        return fail(reason, capacity, "invalid Golden cache publication plan");
    int result = p4_game_tables_builder_golden_cache(builder, plan->paths, plan->identity,
                                                      plan->extra, plan->extra_count, reason, capacity);
    if (result == P4_TABLE_NOT_FOUND) {
        if (p4_golden_cache_store(plan->paths, plan->identity, plan->tables, plan->table_count,
                                  key, reason, capacity)) return -1;
        result = p4_game_tables_builder_golden_cache(builder, plan->paths, plan->identity,
                                                      plan->extra, plan->extra_count, reason, capacity);
    }
    if (result) return -1;
    const P4GameTableInput *msg = NULL;
    const P4GoldenCacheTable *items = NULL;
    for (size_t i = 0; i < builder->input_count; ++i)
        if (p4_game_table_input_kind(builder->inputs[i]) == P4_TABLE_INPUT_MSG) msg = builder->inputs[i];
    for (size_t i = 0; i < plan->extra_count; ++i)
        if (!strcmp(plan->extra[i].name, "decl/item-names")) items = &plan->extra[i];
    if (p4_data_catalog_capture_golden(msg ? p4_game_table_input_bytes(msg) : NULL,
            msg ? p4_game_table_input_size(msg) : 0, items ? items->data : NULL, items ? items->size : 0)) {
        p4_golden_cache_free(plan->extra, plan->extra_count);
        p4_data_catalog_golden_rollback();
        return fail(reason, capacity, "verified Golden catalog name capture failed");
    }
    result = plan->declare(plan->context, builder, plan->extra, plan->extra_count, reason, capacity);
    if (!result && plan->capture_image)
        result = plan->capture_image(builder, plan->extra, plan->extra_count, reason, capacity);
    if (!result && plan->capture_battle)
        result = plan->capture_battle(plan->context, builder, plan->extra, plan->extra_count, reason, capacity);
    if (!result && plan->capture_items)
        result = plan->capture_items(plan->context, builder, plan->extra, plan->extra_count, reason, capacity);
    p4_golden_cache_free(plan->extra, plan->extra_count);
    if (result) p4_data_catalog_golden_rollback();
    return result ? -1 : 0;
}
