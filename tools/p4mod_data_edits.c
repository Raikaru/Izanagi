/* p4mod check's data/edits.xml sink (p4mod_data_edits.h): the decoder's
 * refusals and the field rules are the game's own code; rows resolve to the
 * check build's scratch row, so name lookups are counted as left to the game. */
#include "p4mod_data_edits.h"
#include "game_data_edit_internal.h"
#include <stdio.h>
#include <string.h>

static P4ModDataEdits *seen;

static void left_to_game(const char *what)
{
    if (!seen->unchecked++) snprintf(seen->first_unchecked, sizeof seen->first_unchecked, "%s", what);
}
static void use(P4DataKind kind, const char *path)
{
    if (kind == P4_DATA_FLOOR || kind == P4_DATA_FORMATION || kind == P4_DATA_ENCOUNTERS || kind == P4_DATA_TREASURE)
        seen->dungeons = 1;
    if (kind == P4_DATA_ITEM && path && !strcmp(path, "name")) seen->item_names = 1;
    if (kind == P4_DATA_ITEM && path && !strncmp(path, "shop.", 5)) seen->shop = 1;
    if (kind == P4_DATA_SKILL && path && !strncmp(path, "hits.", 5)) seen->skill_hits = 1;
    if (kind == P4_DATA_ENEMY && path && !strncmp(path, "drops.", 6) && strstr(path, ".chance")) seen->enemy_drops = 1;
}
static int row_by_name(P4DataKind kind, const char *query, size_t *index, char *error, size_t capacity)
{
    (void)query; (void)error; (void)capacity;
    use(kind, NULL);
    left_to_game("a row named by name or key exists");
    *index = 0;
    return 0;
}
static int row_by_id(P4DataKind kind, uint32_t id, size_t *index)
{
    (void)id;
    use(kind, NULL);
    left_to_game("a row named by ID exists");
    *index = 0;
    return 0;
}
static uint32_t row_id(P4DataKind kind, size_t index)
{
    (void)kind;
    return (uint32_t)index;
}
static int field(P4DataKind kind, size_t index, const char *path, const P4DataEditValue *value, char *error, size_t capacity)
{
    use(kind, path);
    return p4_data_edit_set(kind, index, path, value, error, capacity);
}
static const P4DataEditsXmlSink check_sink = {row_by_name, row_by_id, row_id, field, p4_data_edit_learn};

const uint32_t p4mod_data_edits_xml_max = P4_DATA_EDIT_XML_MAX, p4mod_data_luau_max = P4_DATA_EDIT_LUAU_MAX;

int p4mod_data_edits_xml_check(const uint8_t *bytes, size_t size, P4ModDataEdits *out, char *error, size_t capacity)
{
    memset(out, 0, sizeof *out);
    seen = out;
    p4_data_edit_check_unchecked(NULL, 0);
    const int result = p4_data_edits_xml_decode(bytes, size, &check_sink, error, capacity);
    char first[sizeof out->first_unchecked];
    const unsigned rules = p4_data_edit_check_unchecked(first, sizeof first);
    if (rules && !out->unchecked) snprintf(out->first_unchecked, sizeof out->first_unchecked, "%s", first);
    out->unchecked += rules;
    seen = NULL;
    return result;
}
