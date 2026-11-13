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
/* Draws a HANDLED, nonzero handle's authored image with R-low RGBA at its own
 * pixel size times the given scale, as the native glyph it replaces would
 * draw (1.0 = 1:1). Zero opacity/scale draws nothing (native hidden state). */
int p4_game_elements_ui_native_draw_icon(uint32_t icon, float x, float y, float depth,
    uint32_t rgba, float scale_x, float scale_y);
/* A leaf's authored header art as a draw handle for
 * p4_game_elements_ui_native_draw_icon (1.0 = its own size), for header rows
 * outside the prepared columns (the Analyze ailment page). OK writes *icon;
 * NOT_FOUND when no snapshot is active or the leaf declares no header art. */
int p4_game_elements_ui_native_header_icon(uint32_t leaf, uint32_t *icon);
/* A declared [element-prompt] image (P4_ELEMENT_UI_PROMPT_* of
 * game_elements_ui.h) as a draw handle for p4_game_elements_ui_native_draw_icon;
 * NOT_FOUND when no snapshot is active or no package declares it. */
int p4_game_elements_ui_native_prompt(uint32_t prompt, uint32_t *icon);

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

/* Analyze ailment page (PortSeamlessUI): Square in btlActionAnalyzeUpdate
 * (001A24B0, sub-state 3) flips 00219790's row between the affinities (page 0,
 * retail and expanded columns unchanged) and one column per status ailment the
 * target has a pair for and the active packages give header art (page 1).
 * reset() returns to page 0 on entry and on every target switch. flip()
 * returns 1 when it flipped (the caller plays the cursor cue), 0 when the
 * target has no ailment page. page() is 1 only while page 1 is selected and
 * still has columns for that target. */
void p4_game_elements_ui_native_analyze_reset(void);
int p4_game_elements_ui_native_analyze_flip(void *actual_dat_unit);
int p4_game_elements_ui_native_analyze_page(void *actual_dat_unit);
int p4_game_elements_ui_native_analyze_ailments(void *work, void *actual_dat_unit,
    float x, float header_y, float cell_y, float unknown_y);
/* The declared page-flip prompt for the current page, only while the target
 * has an ailment page; OK without a declaration (no prompt). */
int p4_game_elements_ui_native_analyze_prompt(void *work, void *actual_dat_unit, float x, float header_y);
#endif
