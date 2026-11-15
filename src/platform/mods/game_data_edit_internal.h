#ifndef P4PC_GAME_DATA_EDIT_INTERNAL_H
#define P4PC_GAME_DATA_EDIT_INTERNAL_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_data_edit.h"
/* Where data/edits.xml's decoded records go (game_data_edit_xml.c). Every
 * function returns 0, or -1 (resolve, set and learn with a readable reason).
 * resolve finds a row by name or key, index a row by catalog ID, id is a
 * row's catalog ID. The game's sink resolves against the catalog and writes
 * the running layer; p4mod check's (tools/p4mod_data_edits.c) only decodes. */
typedef struct P4DataEditsXmlSink {
    int (*resolve)(P4DataKind kind, const char *query, size_t *index, char *error, size_t capacity);
    int (*index)(P4DataKind kind, uint32_t id, size_t *index);
    uint32_t (*id)(P4DataKind kind, size_t index);
    int (*set)(P4DataKind kind, size_t index, const char *path, const P4DataEditValue *value,
        char *error, size_t capacity);
    int (*learn)(size_t persona, P4DataLearnOp op, uint32_t skill, uint32_t value, char *error, size_t capacity);
} P4DataEditsXmlSink;
/* Decodes one data/edits.xml and hands every record to sink, in file order,
 * stopping at the first refusal. 0 done, -1 with a reason naming the line. */
int p4_data_edits_xml_decode(const uint8_t *bytes, size_t size, const P4DataEditsXmlSink *sink,
    char *error, size_t capacity);
#ifdef P4_DATA_EDIT_CHECK
/* game_data_edit.c built for p4mod check (P4_DATA_EDIT_CHECK): p4_data_edit_set
 * and p4_data_edit_learn apply every rule that needs no disc tables and write
 * nothing. This counts the rules since the last call that needed the tables
 * and so were left to the game, copies the first one's description, resets. */
unsigned p4_data_edit_check_unchecked(char *first, size_t capacity);
#endif

/* A persisted session edit as it was made (game_data_session.c saves them by
 * key): a field write, or a learnset change (learn = op + 1). */
typedef struct P4DataSessionRecord {
    char owner[64];
    P4DataKind kind;
    uint32_t id;
    unsigned learn;
    char path[96];
    int text;
    double number;
    char string[64];
    uint32_t skill, argument; /* learnsets: the skill, then a level or the replacement skill */
} P4DataSessionRecord;
size_t p4_data_session_record_count(void);
const P4DataSessionRecord *p4_data_session_record_at(size_t index);
/* Forget the persisted records (their writes stay until a revert). */
void p4_data_session_records_clear(void);
#endif
