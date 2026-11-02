/* Scalar-only native element-UI bridge for translated game units. No host
 * registry, resource or layout record enters a translated unit. */
#ifndef P4PC_GAME_ELEMENTS_UI_NATIVE_H
#define P4PC_GAME_ELEMENTS_UI_NATIVE_H
#include <stdint.h>

/* Columns of the committed element-ui snapshot. Zero means no expanded element
 * cohort is active, so callers keep their unchanged retail path. */
uint32_t p4_game_elements_ui_native_count(void);
/* Controlled runtime refusal through the shared boot failure path. */
_Noreturn void p4_game_elements_ui_native_refuse(int32_t error);

/* Skill-art routing over the skill's element (datCalcSkillElement semantics).
 * RETAIL keeps the caller's unchanged legacy call; HANDLED means *icon was
 * written (0 = the skill genuinely has no element art, otherwise a draw handle
 * for p4_game_elements_ui_native_draw_icon); negative is a refusal, never
 * permission to index a legacy atlas. The handle is valid only for the
 * immediate draw in the same frame; never store it. actual_dat_unit is the
 * caller's real unit, or NULL where retail has no actor (the skill's own
 * element, as retail shows it). */
#define P4_ELEMENT_UI_ART_HANDLED 0
#define P4_ELEMENT_UI_ART_RETAIL 1
int p4_game_elements_ui_native_skill_icon(void *actual_dat_unit, uint32_t skill_id, uint32_t *icon);
/* Draws a HANDLED, nonzero handle's authored image with R-low RGBA at the given
 * on-screen size. Zero opacity/size draws nothing (native hidden state). */
int p4_game_elements_ui_native_draw_icon(uint32_t icon, float x, float y, float depth,
    uint32_t rgba, float width, float height);

/* 00116820 persona/compendium affinity row. persona_id is the canonical ID the
 * 00115940 builder copied into the display block; native_words is that
 * block's actual seven-word affinity snapshot, not a DatPersonaRecord. */
int p4_game_elements_ui_native_persona_affinities(uint32_t persona_id,
    const uint32_t native_words[7], float x, float y, float depth, uint32_t alpha,
    uint32_t panel_bank, uint32_t affinity_bank);
/* 00219790 Analyze affinity row on the live battle UI work and the target's
 * actual DatUnit, using the caller's animated row coordinates. */
int p4_game_elements_ui_native_analyze_affinities(void *work, void *actual_dat_unit,
    float x, float header_y, float cell_y, float unknown_y);
#endif
