/* The actual shared scalar parser only: no Core/VFS/provider/boot/payload APIs. */
#include "p4pc/game_table_manifest.h"
#include "p4pc/game_tables.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void reject(const P4PackageField *fields, size_t count, P4GameTableSourceMetadata *out)
{
    const P4GameTableSourceMetadata before = *out;
    char error[240] = {0};
    assert(p4_game_tables_source_metadata(fields, count, out, error, sizeof error) == P4_TABLE_INVALID);
    assert(*error && !memcmp(&before, out, sizeof before));
}
int main(void)
{
    P4PackageField fields[] = {
        {.section = "table-source", .argument = "compat.owner:combat", .key = "file", .value = "data/tables/combat.xml"},
        {.section = "table-source", .argument = "compat.owner:combat", .key = "format", .value = "p4-tables-xml"},
        {.section = "table-source", .argument = "compat.owner:combat", .key = "schema", .value = "1"},
        {.section = "table-source", .argument = "compat.owner:combat", .key = "revision", .value = "7"},
    };
    P4GameTableSourceMetadata out = {0};
    char error[240] = "previous error";
    assert(!p4_game_tables_source_metadata(fields, 4, &out, error, sizeof error) && !*error);
    assert(out.key == fields[0].argument && out.file == fields[0].value && !strcmp(out.owner, "compat.owner"));
    assert(out.schema == 1 && out.revision == 7); /* Independent of registry revision. */
    fields[3].value = "4294967295";
    assert(!p4_game_tables_source_metadata(fields, 4, &out, NULL, 0) && out.revision == UINT32_MAX);
    fields[3].value = "4294967296"; reject(fields, 4, &out);
    fields[3].value = "0"; reject(fields, 4, &out);
    fields[3].value = "+7"; reject(fields, 4, &out);
    fields[3].value = " 7"; reject(fields, 4, &out);
    fields[3].value = "7";
    fields[2].value = "2"; reject(fields, 4, &out);
    fields[2].value = "0"; reject(fields, 4, &out);
    fields[2].value = "1";
    fields[1].value = "other-format"; reject(fields, 4, &out);
    fields[1].value = "p4-tables-xml";
    fields[0].value = ""; reject(fields, 4, &out);
    fields[0].value = "data/tables/combat.xml";
    fields[3].key = "file"; reject(fields, 4, &out); /* Duplicate also omits mandatory revision. */
    fields[3].key = "unknown"; reject(fields, 4, &out);
    fields[3].key = NULL; reject(fields, 4, &out);
    fields[3].key = "revision";
    reject(fields, 3, &out); reject(fields, 0, &out); reject(NULL, 4, &out);
    const char *invalid_keys[] = {NULL, "plain", ":key", "owner:", "owner:two:colons", "owner:bad/key", "owner:bad key"};
    for (size_t i = 0; i < sizeof invalid_keys / sizeof invalid_keys[0]; ++i) {
        fields[0].argument = invalid_keys[i]; reject(fields, 4, &out);
    }
    char bounded_owner[66]; memset(bounded_owner, 'a', 63); memcpy(bounded_owner + 63, ":k", 3);
    fields[0].argument = bounded_owner;
    assert(!p4_game_tables_source_metadata(fields, 4, &out, error, sizeof error) && strlen(out.owner) == 63);
    char too_long[P4_TABLE_KEY_SIZE + 1]; memset(too_long, 'a', sizeof too_long - 1);
    too_long[1] = ':'; too_long[sizeof too_long - 1] = 0;
    fields[0].argument = too_long; reject(fields, 4, &out);
    fields[0].argument = "Upper.Owner:Qualified-Key"; /* Existing metadata grammar, not a row/runtime capability. */
    assert(!p4_game_tables_source_metadata(fields, 4, &out, error, sizeof error));
    assert(!strcmp(out.owner, "Upper.Owner"));
    assert(p4_game_tables_source_metadata(fields, 4, NULL, error, sizeof error) == P4_TABLE_INVALID);
    puts("game_table_manifest: shared no-I/O metadata grammar, independent u32 revision and atomic refusal passed");
    return 0;
}
