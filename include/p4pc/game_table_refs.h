#ifndef P4PC_GAME_TABLE_REFS_H
#define P4PC_GAME_TABLE_REFS_H
#include "p4pc/game_table_sources.h"
#include "p4pc/mod_save.h"
#define P4_GAME_TABLE_REFS_WIRE_VERSION 1u
typedef struct P4GameTableRefWriter P4GameTableRefWriter;
typedef struct P4GameTableRefReader P4GameTableRefReader;
/* Strict AUTHORED references only. Explicit unowned retail/builtin projection
 * formats stay with their component; a missing ledger entry NEVER infers native.
 * Exact native-materialized views and their frozen immutable source context
 * outlive the transaction. No semantic-use/history tagging occurs here.
 *
 * Begin writes/reads the version word. Origins are defined once, in first-use
 * order; subsequent references use only this block's canonical dictionary ID,
 * never a persisted source-builder handle. The component emits reference records
 * in its canonical order and pairs every successful begin with end on ALL paths.
 * End releases temporary metadata only, never the borrowed reader/writer/views.
 * Required missing/different entry or origin fails the whole transaction; this
 * is not optional historical provenance and does not create load warnings. */
int p4_game_table_refs_writer_begin(const P4GameTableSources *sources,
    const P4ModSaveContext *transaction, P4ModSaveWriter *writer, P4GameTableRefWriter **out);
void p4_game_table_refs_writer_end(P4GameTableRefWriter *);
int p4_game_table_refs_write(P4GameTableRefWriter *, const void *exact_view,
    uint32_t domain, const uint32_t ids[5]);
int p4_game_table_refs_reader_begin(const P4GameTableSources *sources,
    const P4ModSaveContext *transaction, P4ModSaveReader *reader, P4GameTableRefReader **out);
void p4_game_table_refs_reader_end(P4GameTableRefReader *);
/* Output is the exact CURRENT immutable ledger identity after full validation,
 * not a saved pointer or a reconstructed provider. Unchanged on failure. */
int p4_game_table_refs_read(P4GameTableRefReader *, const void *exact_view,
    uint32_t expected_domain, const P4TableSourceEntry **out);
#endif
