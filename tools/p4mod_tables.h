/* p4mod check's view of [table-source] sections: the game's own metadata rules
 * (src/platform/mods/game_table_manifest.c) and its table XML decoder
 * (game_table_xml.c built with P4_TABLE_XML_CHECK). */
#ifndef P4MOD_TABLES_H
#define P4MOD_TABLES_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct P4ModTableField { const char *key, *value; unsigned line; } P4ModTableField;
typedef struct P4ModTableSource { char owner[64]; const char *file; uint32_t revision; } P4ModTableSource;
/* One mod.ini field of any extension section, for the game's [setting] declarations. */
typedef struct P4ModManifestField { const char *section, *argument, *key, *value; unsigned line; } P4ModManifestField;
/* The game's `when =` gate rules for one boot section (mod_setting_gate.c
 * p4_mod_setting_gate_declared): 0 no gate, 1 a valid gate (*gate its key,
 * *default_on the toggle's declared default), -1 refused with the game's reason. */
int p4mod_section_gate(const P4ModTableField *fields, size_t count, const P4ModManifestField *manifest, size_t all,
    const char **gate, int *default_on, char *reason, size_t capacity);
/* [table-source <argument>] with its fields in file order, as the game's table
 * boot hands them to p4_game_tables_source_metadata. 0, or -1 with its reason. */
int p4mod_table_source_metadata(const char *argument, const P4ModTableField *fields, size_t count,
    P4ModTableSource *out, char *error, size_t capacity);
/* The game's limit for one table-source file (P4_GAME_TABLE_SOURCE_XML_MAX). */
extern const uint32_t p4mod_table_source_max;
typedef void (*P4ModTableVisit)(void *user, const char *record, const char *const *attributes, size_t count,
    unsigned long line);
/* The game's decode of one table-source file with no builder: 0, or -1 with
 * the game's reason "... (lineN)". visit sees each accepted record. */
int p4mod_table_source_check(const P4ModTableSource *source, const uint8_t *bytes, size_t size,
    P4ModTableVisit visit, void *user, char *error, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
