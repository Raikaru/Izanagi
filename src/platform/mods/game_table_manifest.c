#include "p4pc/game_table_manifest.h"
#include "p4pc/game_tables.h"
#include <stdio.h>
#include <string.h>

static int source_owner(const char *key, char owner[64])
{
    if (!key) return 0;
    const char *colon = strchr(key, ':');
    if (!colon || colon == key || !colon[1] || (size_t)(colon - key) >= 64u || strlen(key) >= P4_TABLE_KEY_SIZE) return 0;
    for (const unsigned char *p = (const unsigned char *)key; *p; ++p) {
        unsigned char c = *p;
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') &&
            c != '.' && c != '-' && c != '_' && (const char *)p != colon) return 0;
    }
    memcpy(owner, key, (size_t)(colon - key)); owner[colon - key] = 0;
    return 1;
}
static int positive_decimal(const char *text, uint32_t *out)
{
    if (!text || !*text) return 0;
    uint32_t value = 0;
    do {
        unsigned char c = (unsigned char)*text++;
        if (c < '0' || c > '9' || value > (UINT32_MAX - (c - '0')) / 10u) return 0;
        value = value * 10u + c - '0';
    } while (*text);
    if (!value) return 0;
    *out = value; return 1;
}
static int refuse(char *error, size_t capacity, const char *reason)
{
    if (error && capacity) snprintf(error, capacity, "%s", reason);
    return P4_TABLE_INVALID;
}
int p4_game_tables_source_metadata(const P4PackageField *fields, size_t count,
    P4GameTableSourceMetadata *out, char *error, size_t capacity)
{
    if (error && capacity) *error = 0;
    if (!fields || !count || !out) return refuse(error, capacity, "Missing table-source metadata");
    P4GameTableSourceMetadata candidate = {.key = fields[0].argument};
    const char *format = NULL, *schema = NULL, *revision = NULL;
    if (!source_owner(candidate.key, candidate.owner))
        return refuse(error, capacity, "table-source requires a bounded qualified owner:key");
    for (size_t i = 0; i < count; ++i) {
        const char *name = fields[i].key;
        if (!name) return refuse(error, capacity, "Missing table-source declaration field");
        if (!strcmp(name, "file")) candidate.file = fields[i].value;
        else if (!strcmp(name, "format")) format = fields[i].value;
        else if (!strcmp(name, "schema")) schema = fields[i].value;
        else if (!strcmp(name, "revision")) revision = fields[i].value;
        else return refuse(error, capacity, "Unknown table-source declaration field");
    }
    if (count != 4 || !candidate.file || !*candidate.file || !format || strcmp(format, "p4-tables-xml") ||
        !positive_decimal(schema, &candidate.schema) || candidate.schema != 1 || !positive_decimal(revision, &candidate.revision))
        return refuse(error, capacity, "table-source requires explicit file, supported p4-tables-xml/schema1 and nonzero authored revision");
    *out = candidate;
    return P4_TABLE_OK;
}
