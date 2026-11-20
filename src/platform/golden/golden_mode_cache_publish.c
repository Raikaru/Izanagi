/* Publish only cache-verified Golden inputs and declarations. */
#include "golden_mode_cache_publish.h"
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
    result = plan->declare(plan->context, builder, plan->extra, plan->extra_count, reason, capacity);
    p4_golden_cache_free(plan->extra, plan->extra_count);
    return result ? -1 : 0;
}
