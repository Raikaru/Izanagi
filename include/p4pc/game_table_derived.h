#ifndef P4PC_GAME_TABLE_DERIVED_H
#define P4PC_GAME_TABLE_DERIVED_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_table_inputs.h"
#include "p4pc/packages.h"

/* Derived table sources. A producer is native code, registered by core code
 * before package preparation, that supplies retail-layout replacements for
 * the selected table inputs from an external install it locates and
 * fingerprints itself. A package selects at most one producer with a
 * [table-derived KEY] section (producer, revision, inputs and the optional
 * section gates); nothing runs for packages that do not select one.
 * `inputs` lists the inputs the producer reads or replaces, by lowercase name
 * (item skill unit msg persona encount effect model aicalc, for example
 * `inputs = skill persona unit item msg`); they are prepared before it runs.
 *
 * The producer runs once per table preparation, on the preparing thread,
 * before any native publication. It never sees guest memory. Everything it
 * hands to the sink is copied; its buffers may be freed when the call
 * returns. The composed input replaces only the rows/spans it names in the
 * input that would otherwise be selected (developer, package replacement or
 * disc), and its identity folds in the producer id, revision and every
 * fingerprint the producer reports, so saves and catalogs bind to them.
 *
 * Families are (input kind, packed-bank span index) exactly as sealed by
 * p4_game_table_input_seal: SKILL 0..1, UNIT 0..2, MSG 0..4, PERSONA 0..5,
 * ENCOUNT 0..4, EFFECT 0, and ITEM 0 (the raw ITEMTBL span; row N is the
 * 68-byte record after its 16-byte header). One package owns a family: a
 * second producer naming the same family refuses both. */

#define P4_TABLE_DERIVED_ID_MAX 64u
#define P4_TABLE_DERIVED_DIGEST_BYTES 32u

/* Same-size row replacement. ids are strictly ascending row indices inside
 * the span and must already exist there; row_size must equal the span's
 * native stride or the whole production is refused. */
typedef struct P4TableDerivedRows {
    P4GameTableInputKind input;
    uint32_t span;
    uint32_t row_size;
    size_t count;
    const uint32_t *ids;
    const uint8_t *bytes; /* count * row_size bytes, exact PS2 layout */
} P4TableDerivedRows;

/* Whole-span replacement. bytes is the span payload without the packed-bank
 * length word. A span keeps its length, except a relocatable block:
 * - PERSONA 4, the skill-replacement pairs (P4_PERSONA_SKILL_PAIR_*), takes 1
 *   to P4_PERSONA_SKILL_PAIR_MAX pairs whose last pair is (0, 0), whose other
 *   pairs have no zero half and distinct original skills, and whose skills are
 *   retail ones or ones a package declares;
 * - ENCOUNT 0, 1, 3 and 4 (formations, selectors, weighted groups and record
 *   groups, PS2 layout: game_encounters.h) take whole rows, no fewer than the
 *   selected block has and at most P4_ENCOUNT_ROW_MAX. Every selector must
 *   name an existing weighted and record group, every weighted entry an
 *   existing formation, and formation enemies and record items retail IDs or
 *   ones a package declares. Every step the selected selectors have keeps
 *   exactly the treasure kinds (record flag bit 0, with item and weight) its
 *   selected record group yields: saved dungeon boards re-pick treasures by
 *   kind, and a kind without weight divides by zero.
 * The loader keeps loading the selected original block into the shared battle
 * arena; after the battle tables load, the table service points the block's
 * slot at a heap copy of the new rows and publishes its extent. Other length
 * changes refuse with their family. */
typedef struct P4TableDerivedSpan {
    P4GameTableInputKind input;
    uint32_t span;
    size_t size;
    const uint8_t *bytes;
} P4TableDerivedSpan;

typedef struct P4TableDerivedSink P4TableDerivedSink;
/* 0 accepted, -1 refused (the sink records the reason; stop producing). */
int p4_table_derived_sink_rows(P4TableDerivedSink *sink, const P4TableDerivedRows *rows);
int p4_table_derived_sink_span(P4TableDerivedSink *sink, const P4TableDerivedSpan *span);
/* At least one is required: a digest of the external source the rows came
 * from (for example its executable and each archive read), with a short
 * bounded label shown in diagnostics. */
int p4_table_derived_sink_fingerprint(P4TableDerivedSink *sink, const char *label,
    const uint8_t digest[P4_TABLE_DERIVED_DIGEST_BYTES]);
/* Keeps a copy of one raw input the producer loaded (1 byte to 4 MiB, a short
 * printable name unique per producer) for p4.tables.source; charged to package
 * memory, folded into the catalog identity as name and SHA-256, and freed with
 * the session. */
#define P4_TABLE_DERIVED_SOURCE_MAX (4u << 20)
int p4_table_derived_sink_source(P4TableDerivedSink *sink, const char *name, const uint8_t *bytes, size_t size);
/* Read-only view of a recorded source: only once the table catalog the
 * producer composed is active, for a selected producer id. 0 found, -1 not
 * recorded, not selected, before exposure or after a refusal. */
int p4_table_derived_source(const char *producer_id, const char *name, const uint8_t **bytes, size_t *size);

typedef struct P4TableDerivedRequest {
    const P4PackageProvider *provider; /* the selecting package; read its admitted
                                          restart settings with p4_mod_setting_boot */
    const char *section_key;           /* KEY of its [table-derived KEY] */
} P4TableDerivedRequest;

/* The span of a declared input exactly as it would be selected without any
 * derived replacement (developer, package replacement or disc), borrowed until
 * produce returns: the bytes after the packed-bank length word, or the whole
 * ITEMTBL for ITEM 0. 0 found, -1 an undeclared input or a span it lacks. */
int p4_table_derived_base(const P4TableDerivedRequest *request, P4GameTableInputKind input,
    uint32_t span, const uint8_t **bytes, size_t *size);

/* 0 produced, 1 source absent (the package is refused with the producer's
 * reason; nothing is composed), -1 failed with a bounded reason in error. */
typedef int (*P4TableDerivedProduce)(void *user, const P4TableDerivedRequest *request,
    P4TableDerivedSink *sink, char *error, size_t capacity);

typedef struct P4TableDerivedProducer {
    const char *id;     /* bounded lowercase owner:key, e.g. "p4pc:golden" */
    uint32_t revision;  /* nonzero; bump when the produced layout changes */
    P4TableDerivedProduce produce;
    void *user;
} P4TableDerivedProducer;

/* Core boot only, before package preparation. The producer struct and its id
 * are borrowed for the process lifetime. 0 registered, -1 invalid or a
 * duplicate id. */
int p4_game_tables_derived_register(const P4TableDerivedProducer *producer);

#endif
