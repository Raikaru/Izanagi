#include "p4pc/game_tables.h"
#include "p4pc/game_tables_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static P4GameTablePrepareFn callback;
static void *callback_user;
static int preparing;

int p4_game_tables_prepare_register(P4GameTablePrepareFn prepare, void *user)
{
    if (!prepare) return P4_TABLE_INVALID;
    if (callback) return callback == prepare && callback_user == user ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    if (preparing) return P4_TABLE_CONFLICT;
    callback = prepare; callback_user = user;
    return P4_TABLE_OK;
}
int p4_game_tables_prepare_unregister(P4GameTablePrepareFn prepare, void *user)
{
    if (!prepare) return P4_TABLE_INVALID;
    if (preparing) return P4_TABLE_CONFLICT;
    if (!callback) return P4_TABLE_OK;
    if (callback != prepare || callback_user != user) return P4_TABLE_CONFLICT;
    callback = NULL; callback_user = NULL;
    return P4_TABLE_OK;
}
int p4_game_tables_prepare(const char *table_key, const char *source_key,
    uint32_t original_rows, const uint8_t *retail_bytes, size_t size)
{
    if (!callback) return P4_TABLE_PREPARE_REQUIRED;
    if (preparing) return P4_TABLE_CONFLICT;
    preparing = 1;
    int result = callback(callback_user, table_key, source_key, original_rows, retail_bytes, size);
    preparing = 0;
    return result;
}
int p4_game_tables_prepare_clear(void)
{
    if (preparing) return P4_TABLE_CONFLICT;
    callback = NULL; callback_user = NULL;
    return P4_TABLE_OK;
}

typedef struct CompleteParticipant {
    P4GameTableCompletePrepareFn prepare;
    P4GameTableCompleteCommitFn commit;
    void *user;
    unsigned required;
} CompleteParticipant;
static const char *const participant_keys[] = {
    "item", "skill", "persona", "enemy", "table-source", "element", "element-route",
    "element-ai", "element-knowledge", "element-ui"
};
enum { COMPLETE_COUNT = sizeof participant_keys / sizeof participant_keys[0] };
static CompleteParticipant participants[COMPLETE_COUNT];
static int completion_locked, completion_running, completion_error;
static int completion_prepared, completion_committed;
static unsigned participant_index(const char *key)
{
    unsigned index = 0;
    while (index < COMPLETE_COUNT && strcmp(key, participant_keys[index])) ++index;
    return index;
}

int p4_game_tables_battle_complete_register(const char *table_key,
    P4GameTableCompletePrepareFn prepare, P4GameTableCompleteCommitFn commit, void *user)
{
    if (!table_key || !prepare || !commit) return P4_TABLE_INVALID;
    if (completion_locked) return P4_TABLE_FROZEN;
    const unsigned index = participant_index(table_key);
    if (index == COMPLETE_COUNT) return P4_TABLE_NOT_FOUND;
    CompleteParticipant *slot = &participants[index];
    if (slot->prepare)
        return slot->prepare == prepare && slot->commit == commit && slot->user == user ?
            P4_TABLE_OK : P4_TABLE_CONFLICT;
    *slot = (CompleteParticipant){prepare, commit, user, 0};
    return P4_TABLE_OK;
}
int p4_game_tables_battle_complete_unregister(const char *table_key,
    P4GameTableCompletePrepareFn prepare, P4GameTableCompleteCommitFn commit, void *user)
{
    if (!table_key || !prepare || !commit) return P4_TABLE_INVALID;
    if (completion_locked) return P4_TABLE_FROZEN;
    const unsigned index = participant_index(table_key);
    if (index == COMPLETE_COUNT) return P4_TABLE_NOT_FOUND;
    CompleteParticipant *slot = &participants[index];
    if (!slot->prepare) return P4_TABLE_OK;
    if (slot->prepare != prepare || slot->commit != commit || slot->user != user) return P4_TABLE_CONFLICT;
    *slot = (CompleteParticipant){0};
    return P4_TABLE_OK;
}
int p4_game_tables_battle_complete_require(const char *owner_key, const char *required_key)
{
    if (!owner_key || !required_key) return P4_TABLE_INVALID;
    if (completion_locked) return P4_TABLE_FROZEN;
    const unsigned owner = participant_index(owner_key), required = participant_index(required_key);
    if (owner == COMPLETE_COUNT || required == COMPLETE_COUNT) return P4_TABLE_NOT_FOUND;
    if (owner == required) return P4_TABLE_INVALID;
    if (!participants[owner].prepare) return P4_TABLE_NOT_READY;
    participants[owner].required |= 1u << required;
    return P4_TABLE_OK;
}
int p4_game_tables_battle_prepare(void)
{
    if (completion_running || completion_prepared) return P4_TABLE_CONFLICT;
    if (completion_error) return completion_error;
    completion_locked = completion_running = 1;
    for (unsigned i = 0; i < COMPLETE_COUNT; ++i) {
        if (!participants[i].required) continue;
        for (unsigned needed = 0; needed < COMPLETE_COUNT; ++needed)
            if ((participants[i].required & (1u << needed)) && !participants[needed].prepare) {
                completion_error = P4_TABLE_NOT_READY;
                completion_running = 0;
                return completion_error;
            }
    }
    for (unsigned i = 0; i < COMPLETE_COUNT; ++i) {
        if (!participants[i].prepare) continue;
        int result = participants[i].prepare(participants[i].user);
        if (result) {
            completion_error = result;
            completion_running = 0;
            return result;
        }
    }
    completion_prepared = 1;
    completion_running = 0;

    return P4_TABLE_OK;
}
void p4_game_tables_battle_commit(void)
{
    if (completion_running || !completion_prepared || completion_error)
        p4_game_tables_battle_refuse(P4_TABLE_NOT_READY);
    completion_running = 1;
    for (unsigned i = 0; i < COMPLETE_COUNT; ++i)
        if (participants[i].commit) participants[i].commit(participants[i].user);
    completion_prepared = completion_running = 0;
    completion_committed = 1;
}
int p4_game_tables_battle_abort(void)
{
    if (completion_running) return P4_TABLE_CONFLICT;
    if (!completion_prepared && !completion_error) return completion_committed ? P4_TABLE_FROZEN : P4_TABLE_OK;
    /* No commit has run for this transaction. Do not pretend to undo frozen
     * allocations or prior active state: real owner teardown retains that duty. */
    completion_prepared = completion_error = 0;
    return P4_TABLE_OK;
}
int p4_game_tables_battle_complete(void)
{
    int result = p4_game_tables_battle_prepare();
    if (result) return result;
    p4_game_tables_battle_commit();
    return P4_TABLE_OK;
}
int p4_game_tables_battle_complete_clear(void)
{
    if (completion_running || completion_prepared) return P4_TABLE_CONFLICT;
    memset(participants, 0, sizeof participants);
    completion_locked = completion_error = completion_committed = 0;
    return P4_TABLE_OK;
}
_Noreturn void p4_game_tables_battle_refuse(int result)
{
    fprintf(stderr, "p4pc: battle table publication refused (%d)\n", result);
    abort();
}
