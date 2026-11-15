#include "p4pc/golden_table_cache.h"
#include "p4pc/game_mode.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static P4GameMode mode = P4_GAME_MODE_GOLDEN;
static int miss, invalid_kind = -1, destroyed;
P4GameMode p4_game_mode(void) { return mode; }
const char *p4_game_table_input_label(P4GameTableInputKind kind)
{
    static const char *labels[] = {"init/itemtbl.bin","battle/SKILL.TBL","battle/UNIT.TBL","battle/MSG.TBL",
        "battle/PERSONA.TBL","battle/ENCOUNT.TBL","battle/EFFECT.TBL","battle/MODEL.TBL","battle/AICALC.TBL"};
    return labels[kind];
}
int p4_golden_cache_load(const P4GoldenCachePaths *paths, const P4GoldenCacheIdentity *id,
    P4GoldenCacheTable *tables, size_t count, char key[65], char *reason, size_t cap)
{
    (void)paths; (void)id; (void)reason; (void)cap;
    if (miss) return miss;
    memset(key,'a',64); key[64] = 0;
    for (size_t i = 0; i < count; ++i) {
        if (i) assert(strcmp(tables[i-1].name,tables[i].name) < 0);
        tables[i].data = malloc(1); assert(tables[i].data); tables[i].data[0] = 42; tables[i].size = 1;
    }
    return 0;
}
void p4_golden_cache_free(P4GoldenCacheTable *tables, size_t count)
{
    for (size_t i = 0; i < count; ++i) free(tables[i].data);
}
P4GameTableInput *p4_game_table_input_from_golden_cache(P4GameTableInputKind kind,
    const uint8_t *data, size_t size, const char *key)
{
    assert(size == 1 && data[0] == 42 && strlen(key) == 64);
    return (int)kind == invalid_kind ? NULL : (P4GameTableInput *)malloc(1);
}
void p4_game_table_input_destroy(P4GameTableInput *input)
{
    if (input) { ++destroyed; free(input); }
}
int main(void)
{
    P4GameTablesDefinitionBuilder b = {0};
    b.tables = (void *)(uintptr_t)1; b.sources = (void *)(uintptr_t)1; b.registry_revision = 1;
    mode = P4_GAME_MODE_PS2;
    assert(p4_game_tables_builder_golden_cache(&b,NULL,NULL,NULL,0,NULL,0) == P4_TABLE_INVALID);
    mode = P4_GAME_MODE_GOLDEN; miss = 1;
    assert(p4_game_tables_builder_golden_cache(&b,NULL,NULL,NULL,0,NULL,0) == P4_TABLE_NOT_FOUND);
    miss = -1;
    assert(p4_game_tables_builder_golden_cache(&b,NULL,NULL,NULL,0,NULL,0) == P4_TABLE_INVALID);
    assert(!b.input_count && !b.input_mask);
    miss = 0; invalid_kind = P4_TABLE_INPUT_PERSONA;
    assert(p4_game_tables_builder_golden_cache(&b,NULL,NULL,NULL,0,NULL,0) == P4_TABLE_INVALID);
    assert(!b.input_count && !b.input_mask && destroyed == 4);
    invalid_kind = -1;
    P4GoldenCacheTable invalid[] = {{"z",(uint8_t *)(uintptr_t)1,1},{"a",(uint8_t *)(uintptr_t)1,1}};
    assert(p4_game_tables_builder_golden_cache(&b,NULL,NULL,invalid,2,NULL,0) == P4_TABLE_INVALID);
    assert(!invalid[0].data && !invalid[1].data);
    P4GoldenCacheTable extra[] = {{"extra/exp",NULL,0}};
    assert(p4_game_tables_builder_golden_cache(&b,NULL,NULL,extra,1,NULL,0) == P4_TABLE_OK);
    assert(extra[0].size == 1 && extra[0].data[0] == 42);
    p4_golden_cache_free(extra,1);
    assert(b.input_count == P4_TABLE_INPUT_COUNT && b.input_mask == (1u << P4_TABLE_INPUT_COUNT) - 1);
    assert(p4_game_tables_builder_golden_cache(&b,NULL,NULL,NULL,0,NULL,0) == P4_TABLE_INVALID);
    for (size_t i = 0; i < b.input_count; ++i) p4_game_table_input_destroy(b.inputs[i]);
    return 0;
}
