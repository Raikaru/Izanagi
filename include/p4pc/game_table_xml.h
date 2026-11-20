#ifndef P4PC_GAME_TABLE_XML_H
#define P4PC_GAME_TABLE_XML_H
#include "p4pc/game_table_builder.h"
#include "p4pc/content_digest.h"
#define P4_GAME_TABLE_SOURCE_XML_MAX (8u * 1024u * 1024u)
/* Exact candidate objects and captured source handle, not active globals. All
 * allocation here is host metadata. Caller destroys the ENTIRE unpublished
 * candidate after any failure; partially decoded rows are never published. */
typedef struct P4GameTableXmlTarget {
    P4GameTablesDefinitionBuilder *builder;
    const char *owner;
    uint32_t source, source_revision;
    /* Optional: receives every source byte exactly as decoded. */
    P4ContentDigest *content;
} P4GameTableXmlTarget;
/* p4-tables-xml/schema1: one p4-tables root, flat typed record attributes,
 * strict uint32 values and bounded strings; no executable code/native address,
 * external entity resolution, automatic ID assignment or late payload reread.
 * Unsupported records/attributes refuse, never get silently skipped. */
int p4_game_tables_source_xml(const P4GameTableXmlTarget *target,
    const uint8_t *bytes, size_t size, char *error, size_t error_capacity);
struct P4PackageFile;
/* Reads the already-authorized bounded stream once in small chunks. Owns no
 * file handle; caller closes/revalidates it and discards candidate on failure. */
int p4_game_tables_source_xml_file(const P4GameTableXmlTarget *target,
    struct P4PackageFile *file, char *error, size_t error_capacity);
#ifdef P4_TABLE_XML_CHECK
/* game_table_xml.c built for p4mod check (P4_TABLE_XML_CHECK): the same lexical,
 * root, nesting, size, record-field and attribute checks as above with no
 * builder, so nothing is registered and registration rules (ID claims, key
 * uniqueness, family state, source binding) are not applied. visit sees each
 * accepted record's name and attribute names. 0, or an error naming the line. */
typedef void (*P4GameTableXmlVisit)(void *user, const char *record, const char *const *attributes, size_t count,
    unsigned long line);
int p4_game_tables_source_xml_check(const char *owner, uint32_t revision, const uint8_t *bytes, size_t size,
    P4GameTableXmlVisit visit, void *user, char *error, size_t error_capacity);
#endif
#endif
