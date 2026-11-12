/* p4mod_tables.h over game_table_manifest.c, game_table_xml.c (check build) and the
 * declared-rule half of mod_setting_gate.c (check build). */
#include "p4pc/mod_setting_gate.h"
#include "p4mod_tables.h"
#include "p4pc/game_table_manifest.h"
#include "p4pc/game_table_xml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const uint32_t p4mod_table_source_max = P4_GAME_TABLE_SOURCE_XML_MAX;

int p4mod_section_gate(const P4ModTableField *fields, size_t count, const P4ModManifestField *manifest, size_t all,
    const char **gate, int *default_on, char *reason, size_t capacity)
{
    P4PackageField *section = count ? calloc(count, sizeof *section) : NULL;
    P4PackageField *every = all ? calloc(all, sizeof *every) : NULL;
    if ((count && !section) || (all && !every)) {
        free(section); free(every);
        snprintf(reason, capacity, "out of memory");
        return -1;
    }
    for (size_t i = 0; i < count; ++i) section[i] = (P4PackageField){"", "", fields[i].key, fields[i].value, fields[i].line};
    for (size_t i = 0; i < all; ++i)
        every[i] = (P4PackageField){manifest[i].section, manifest[i].argument, manifest[i].key, manifest[i].value,
                                    manifest[i].line};
    const int result = p4_mod_setting_gate_declared(section, count, every, all, gate, default_on, reason, capacity);
    free(section);
    free(every);
    return result;
}

int p4mod_table_source_metadata(const char *argument, const P4ModTableField *fields, size_t count,
    P4ModTableSource *out, char *error, size_t capacity)
{
    P4PackageField *packaged = count ? calloc(count, sizeof *packaged) : NULL;
    if (count && !packaged) { snprintf(error, capacity, "out of memory"); return -1; }
    for (size_t i = 0; i < count; ++i)
        packaged[i] = (P4PackageField){"table-source", argument, fields[i].key, fields[i].value, fields[i].line};
    P4GameTableSourceMetadata metadata;
    const int result = p4_game_tables_source_metadata(packaged, count, &metadata, error, capacity);
    free(packaged);
    if (result) return -1;
    snprintf(out->owner, sizeof out->owner, "%s", metadata.owner);
    out->file = metadata.file;
    out->revision = metadata.revision;
    return 0;
}
int p4mod_table_source_check(const P4ModTableSource *source, const uint8_t *bytes, size_t size,
    P4ModTableVisit visit, void *user, char *error, size_t capacity)
{
    return p4_game_tables_source_xml_check(source->owner, source->revision, bytes, size, visit, user, error, capacity)
        ? -1 : 0;
}
