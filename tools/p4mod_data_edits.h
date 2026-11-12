/* p4mod check's view of data/edits.xml: the game's own decoder
 * (src/platform/mods/game_data_edit_xml.c) over the game's own field rules
 * (game_data_edit.c built with P4_DATA_EDIT_CHECK), with nothing written. */
#ifndef P4MOD_DATA_EDITS_H
#define P4MOD_DATA_EDITS_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct P4ModDataEdits {
    /* What the records use, for the capabilities they need: item renames
     * (data.item_names), item shop fields (data.shop), dungeon rows (data.dungeons). */
    int item_names, shop, dungeons;
    /* Writes the layer's cross-field rules (game_data_edit.c invariants) depend on: skill
     * hits.min/hits.max (min <= max) and enemy drop chances (at most 100% in all). The game
     * checks them on the final rows, after this mod's data.luau has also run, against the
     * disc's values for every field the layer did not write: never here. */
    int skill_hits, enemy_drops;
    /* Rules that need the disc's tables (row names, keys and IDs, a row's
     * current bytes) and were left to the game: how many, and the first. */
    unsigned unchecked;
    char first_unchecked[192];
} P4ModDataEdits;
/* Decodes bytes as the game does and applies every field rule that needs no
 * disc: XML, the <p4-edits schema="1"> root, rows and children, attributes,
 * slots, numbers, ranges, names of elements, affinities, ailments and levels.
 * 0, or -1 with "line N: reason". out is filled either way. */
int p4mod_data_edits_xml_check(const uint8_t *bytes, size_t size, P4ModDataEdits *out, char *error, size_t capacity);
/* The game's size limits for data/edits.xml and data.luau (p4pc/game_data_edit.h). */
extern const uint32_t p4mod_data_edits_xml_max, p4mod_data_luau_max;
#ifdef __cplusplus
}
#endif
#endif
