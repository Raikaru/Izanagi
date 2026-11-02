#include "p4pc/game_tables.h"
#include "p4pc/game_tables_native.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct Context { unsigned calls; int result; const uint8_t *bytes; size_t size; } Context;
static int prepare(void *user, const char *table, const char *source, uint32_t rows, const uint8_t *bytes, size_t size)
{
    Context *context = user;
    assert(!strcmp(table, "item") && !strcmp(source, "DATA/INIT/ITEMTBL.BIN") && rows == 1536);
    assert(bytes == context->bytes && size == context->size);
    ++context->calls;
    assert(p4_game_tables_prepare_register(prepare, user) == P4_TABLE_OK);
    assert(p4_game_tables_prepare_clear() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_prepare(table, source, rows, bytes, size) == P4_TABLE_CONFLICT);
    return context->result;
}
static int other(void *user, const char *table, const char *source, uint32_t rows, const uint8_t *bytes, size_t size)
{
    (void)user; (void)table; (void)source; (void)rows; (void)bytes; (void)size;
    assert(!"a conflicting callback must never be invoked");
    return P4_TABLE_INVALID;
}
typedef struct Family { unsigned id, prepares, commits; int result; } Family;
static unsigned steps[20], step_count, prepared_families, expected_families = 3;
static void family_commit(void *user);
static int family_prepare(void *user)
{
    Family *family = user;
    assert(step_count < expected_families);
    steps[step_count++] = family->id;
    ++family->prepares; ++prepared_families;
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_complete() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_prepare() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_abort() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_complete_require("skill", "element") == P4_TABLE_FROZEN);
    assert(p4_game_tables_battle_complete_unregister("skill", family_prepare, family_commit, user) == P4_TABLE_FROZEN);
    return family->result;
}
static void family_commit(void *user)
{
    Family *family = user;
    assert(prepared_families == expected_families && step_count >= expected_families && step_count < expected_families * 2);
    steps[step_count++] = family->id + expected_families;
    ++family->commits;
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_complete() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_prepare() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_abort() == P4_TABLE_CONFLICT);
}
static void completion_cases(void)
{
    Family families[3] = {{.id = 0}, {.id = 1}, {.id = 2}}, conflict = {0};
    const char *keys[] = {"skill", "persona", "enemy"};
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("skill", family_prepare, family_commit, families) == P4_TABLE_FROZEN);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("unknown", family_prepare, family_commit, families) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_battle_complete_register("skill", NULL, family_commit, families) == P4_TABLE_INVALID);
    /* Registration order is not publication order. */
    for (int i = 2; i >= 0; --i)
        assert(p4_game_tables_battle_complete_register(keys[i], family_prepare, family_commit, &families[i]) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("skill", family_prepare, family_commit, families) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("skill", family_prepare, family_commit, &conflict) == P4_TABLE_CONFLICT);
    for (unsigned load = 0; load < 2; ++load) {
        step_count = prepared_families = 0;
        assert(p4_game_tables_battle_complete() == P4_TABLE_OK && step_count == 6);
        for (unsigned i = 0; i < 6; ++i) assert(steps[i] == i);
        for (unsigned i = 0; i < 3; ++i) assert(families[i].prepares == load + 1 && families[i].commits == load + 1);
    }
    assert(p4_game_tables_battle_complete_register("enemy", family_prepare, family_commit, families + 2) == P4_TABLE_FROZEN);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    memset(families, 0, sizeof families);
    for (unsigned i = 0; i < 3; ++i) {
        families[i].id = i;
        assert(p4_game_tables_battle_complete_register(keys[i], family_prepare, family_commit, families + i) == P4_TABLE_OK);
    }
    step_count = prepared_families = 0;
    families[1].result = P4_TABLE_NOT_FOUND;
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_FOUND && step_count == 2);
    assert(families[0].prepares == 1 && families[1].prepares == 1 && !families[2].prepares);
    for (unsigned i = 0; i < 3; ++i) assert(!families[i].commits);
    families[1].result = 0;
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_FOUND && step_count == 2);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
}
static void completion_dependencies(void)
{
    Family families[4] = {{.id = 0}, {.id = 1}, {.id = 2}, {.id = 3}};
    const char *keys[] = {"skill", "persona", "enemy", "element"};
    assert(p4_game_tables_battle_complete_require(NULL, "element") == P4_TABLE_INVALID);
    assert(p4_game_tables_battle_complete_require("unknown", "element") == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_battle_complete_require("skill", "skill") == P4_TABLE_INVALID);
    assert(p4_game_tables_battle_complete_require("skill", "element") == P4_TABLE_NOT_READY);
    assert(p4_game_tables_battle_complete_register("skill", family_prepare, family_commit, families) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("skill", "element") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("skill", family_prepare, family_commit, families) == P4_TABLE_OK);
    step_count = prepared_families = 0;
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY && !step_count && !prepared_families);
    assert(!families[0].prepares && !families[0].commits);
    assert(p4_game_tables_battle_complete_register("element", family_prepare, family_commit, families + 3) == P4_TABLE_FROZEN);
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY && !step_count);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    expected_families = 4;
    for (int i = 3; i >= 0; --i)
        assert(p4_game_tables_battle_complete_register(keys[i], family_prepare, family_commit, families + i) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("skill", "element") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("skill", "element") == P4_TABLE_OK);
    /* Presence is not a scheduler: element consumes already-frozen skill data
     * before knowledge/UI preparation, before any family publishes. */
    assert(p4_game_tables_battle_complete_require("element", "skill") == P4_TABLE_OK);
    for (unsigned load = 0; load < 2; ++load) {
        step_count = prepared_families = 0;
        assert(p4_game_tables_battle_complete() == P4_TABLE_OK && step_count == 8);
        for (unsigned i = 0; i < 8; ++i) assert(steps[i] == i);
        for (unsigned i = 0; i < 4; ++i) assert(families[i].prepares == load + 1 && families[i].commits == load + 1);
    }
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    memset(families, 0, sizeof families);
    for (unsigned i = 0; i < 4; ++i) {
        families[i].id = i;
        assert(p4_game_tables_battle_complete_register(keys[i], family_prepare, family_commit, families + i) == P4_TABLE_OK);
    }
    families[3].result = P4_TABLE_NOT_FOUND;
    step_count = prepared_families = 0;
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_FOUND && step_count == 4);
    for (unsigned i = 0; i < 4; ++i) assert(families[i].prepares == 1 && !families[i].commits);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    expected_families = 3;
}
static void completion_expanded_consumers(void)
{
    const char *keys[] = {"item", "skill", "persona", "enemy", "table-source", "element", "element-route", "element-ai", "element-knowledge", "element-ui"};
    Family families[10];
    expected_families = 10;
    for (unsigned missing = 4; missing < 10; ++missing) {
        memset(families, 0, sizeof families);
        for (unsigned i = 0; i < 10; ++i) {
            families[i].id = i;
            if (i != missing)
                assert(p4_game_tables_battle_complete_register(keys[i], family_prepare, family_commit, families + i) == P4_TABLE_OK);
        }
        assert(p4_game_tables_battle_complete_require("skill", "element") == P4_TABLE_OK);
        assert(p4_game_tables_battle_complete_require("item", "table-source") == P4_TABLE_OK);
        if (missing != 5) {
            assert(p4_game_tables_battle_complete_require("element", "table-source") == P4_TABLE_OK);
            assert(p4_game_tables_battle_complete_require("element", "element-route") == P4_TABLE_OK);
            assert(p4_game_tables_battle_complete_require("element", "element-ai") == P4_TABLE_OK);
            assert(p4_game_tables_battle_complete_require("element", "element-knowledge") == P4_TABLE_OK);
            assert(p4_game_tables_battle_complete_require("element", "element-ui") == P4_TABLE_OK);
        }
        step_count = prepared_families = 0;
        assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY && !step_count && !prepared_families);
        for (unsigned i = 0; i < 10; ++i) assert(!families[i].prepares && !families[i].commits);
        assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    }
    memset(families, 0, sizeof families);
    for (int i = 9; i >= 0; --i) {
        families[i].id = (unsigned)i;
        assert(p4_game_tables_battle_complete_register(keys[i], family_prepare, family_commit, families + i) == P4_TABLE_OK);
    }
    assert(p4_game_tables_battle_complete_require("item", "table-source") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("element", "table-source") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("element", "element-route") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("element", "element-ai") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("element", "element-knowledge") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("element", "element-ui") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("element-knowledge", "element") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("element-ui", "element") == P4_TABLE_OK);
    for (unsigned load = 0; load < 2; ++load) {
        step_count = prepared_families = 0;
        assert(p4_game_tables_battle_complete() == P4_TABLE_OK && step_count == 20);
        for (unsigned i = 0; i < 20; ++i) assert(steps[i] == i);
    }
    families[9].result = P4_TABLE_INVALID;
    step_count = prepared_families = 0;
    assert(p4_game_tables_battle_complete() == P4_TABLE_INVALID && step_count == 10);
    for (unsigned i = 0; i < 10; ++i) assert(families[i].prepares == 3 && families[i].commits == 2);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    expected_families = 3;
}
static void completion_transaction(void)
{
    Family families[3] = {{.id = 0}, {.id = 1}, {.id = 2}};
    const char *keys[] = {"skill", "persona", "enemy"};
    assert(p4_game_tables_battle_abort() == P4_TABLE_OK);
    for (unsigned i = 0; i < 3; ++i)
        assert(p4_game_tables_battle_complete_register(keys[i], family_prepare, family_commit, families + i) == P4_TABLE_OK);
    step_count = prepared_families = 0;
    assert(p4_game_tables_battle_prepare() == P4_TABLE_OK && step_count == 3);
    for (unsigned i = 0; i < 3; ++i) assert(families[i].prepares == 1 && !families[i].commits);
    assert(p4_game_tables_battle_prepare() == P4_TABLE_CONFLICT && step_count == 3);
    assert(p4_game_tables_battle_complete() == P4_TABLE_CONFLICT && step_count == 3);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_abort() == P4_TABLE_OK && step_count == 3);
    for (unsigned i = 0; i < 3; ++i) assert(!families[i].commits);
    assert(p4_game_tables_battle_complete_register("skill", family_prepare, family_commit, families) == P4_TABLE_FROZEN);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    for (unsigned i = 0; i < 3; ++i)
        assert(p4_game_tables_battle_complete_register(keys[i], family_prepare, family_commit, families + i) == P4_TABLE_OK);
    step_count = prepared_families = 0;
    assert(p4_game_tables_battle_prepare() == P4_TABLE_OK && step_count == 3);
    p4_game_tables_battle_commit();
    assert(step_count == 6);
    for (unsigned i = 0; i < 6; ++i) assert(steps[i] == i);
    for (unsigned i = 0; i < 3; ++i) assert(families[i].prepares == 2 && families[i].commits == 1);
    assert(p4_game_tables_battle_abort() == P4_TABLE_FROZEN); /* Never fake a published rollback. */
    families[1].result = P4_TABLE_INVALID;
    step_count = prepared_families = 0;
    assert(p4_game_tables_battle_prepare() == P4_TABLE_INVALID && step_count == 2);
    assert(p4_game_tables_battle_prepare() == P4_TABLE_INVALID && step_count == 2);
    assert(p4_game_tables_battle_abort() == P4_TABLE_OK && step_count == 2);
    for (unsigned i = 0; i < 3; ++i) assert(families[i].commits == 1); /* Previous active tuple is retained. */
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
}
static void completion_early_rollback(void)
{
    Family removed = {.id = 7}, sibling = {.id = 0}, wrong = {0};
    assert(p4_game_tables_battle_complete_unregister(NULL, family_prepare, family_commit, &removed) == P4_TABLE_INVALID);
    assert(p4_game_tables_battle_complete_unregister("item", NULL, family_commit, &removed) == P4_TABLE_INVALID);
    assert(p4_game_tables_battle_complete_unregister("unknown", family_prepare, family_commit, &removed) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_battle_complete_unregister("item", family_prepare, family_commit, &removed) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("item", family_prepare, family_commit, &removed) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("skill", family_prepare, family_commit, &sibling) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("item", "element") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_unregister("item", family_prepare, family_commit, &wrong) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_battle_complete_unregister("item", family_prepare, family_commit, &removed) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_unregister("item", family_prepare, family_commit, &removed) == P4_TABLE_OK);
    expected_families = 1; step_count = prepared_families = 0;
    assert(p4_game_tables_battle_complete() == P4_TABLE_OK && step_count == 2);
    assert(!removed.prepares && !removed.commits && sibling.prepares == 1 && sibling.commits == 1);
    assert(p4_game_tables_battle_complete_unregister("skill", family_prepare, family_commit, &sibling) == P4_TABLE_FROZEN);
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("item", family_prepare, family_commit, &removed) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_register("skill", family_prepare, family_commit, &sibling) == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_require("skill", "enemy") == P4_TABLE_OK);
    assert(p4_game_tables_battle_complete_unregister("item", family_prepare, family_commit, &removed) == P4_TABLE_OK);
    step_count = prepared_families = 0;
    assert(p4_game_tables_battle_complete() == P4_TABLE_NOT_READY && !step_count);
    assert(sibling.prepares == 1 && sibling.commits == 1); /* Its missing prerequisite was not erased. */
    assert(p4_game_tables_battle_complete_clear() == P4_TABLE_OK);
    expected_families = 3;
}
int main(void)
{
    const uint8_t payload[] = {0, 6, 0, 0};
    Context context = {0, P4_TABLE_NOT_READY, payload, sizeof payload}, another = context;
    assert(p4_game_tables_prepare("item", "DATA/INIT/ITEMTBL.BIN", 1536, payload, sizeof payload) == P4_TABLE_PREPARE_REQUIRED);
    assert(p4_game_tables_prepare_register(NULL, &context) == P4_TABLE_INVALID);
    assert(p4_game_tables_prepare_register(prepare, &context) == P4_TABLE_OK);
    assert(p4_game_tables_prepare_register(prepare, &context) == P4_TABLE_OK && !context.calls);
    assert(p4_game_tables_prepare_register(prepare, &another) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_prepare_register(other, &context) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_prepare("item", "DATA/INIT/ITEMTBL.BIN", 1536, payload, sizeof payload) == P4_TABLE_NOT_READY);
    assert(context.calls == 1);
    context.result = P4_TABLE_OK;
    assert(p4_game_tables_prepare("item", "DATA/INIT/ITEMTBL.BIN", 1536, payload, sizeof payload) == P4_TABLE_OK);
    assert(context.calls == 2 && !another.calls);
    assert(p4_game_tables_prepare_clear() == P4_TABLE_OK);
    assert(p4_game_tables_prepare("item", "DATA/INIT/ITEMTBL.BIN", 1536, payload, sizeof payload) == P4_TABLE_PREPARE_REQUIRED);
    assert(p4_game_tables_prepare_register(prepare, &another) == P4_TABLE_OK);
    assert(p4_game_tables_prepare_clear() == P4_TABLE_OK);
    completion_cases(); completion_dependencies(); completion_expanded_consumers(); completion_transaction(); completion_early_rollback();
    puts("game_tables_prepare: exact source contract, missing-handler/reentrancy refusal and prepare-all/commit-all repeated-load ownership passed");
    return 0;
}
