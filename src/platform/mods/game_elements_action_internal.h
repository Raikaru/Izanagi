#ifndef P4PC_GAME_ELEMENTS_ACTION_INTERNAL_H
#define P4PC_GAME_ELEMENTS_ACTION_INTERNAL_H
#include "p4pc/game_elements.h"
/* Only actual native semantic consumption uses these entrypoints. The single
 * shared computation authorizes exact Skill/Item definitions BEFORE reading
 * them; CPU validation and pure callers use the public non-touch variants. */
int p4_game_elements_resolve_runtime_primary_frame(const P4GameTables *tables,
    const struct P4GameSkills *skills, const struct P4GameItems *items, void *unit,
    uint32_t skill_id, uint32_t native_options, P4GameActionFrame *frame);
int p4_game_elements_resolve_runtime_applicability_frame(const P4GameTables *tables,
    const struct P4GameSkills *skills, const struct P4GameItems *items, void *unit,
    uint32_t skill_id, P4GameActionFrame *frame);
int p4_game_elements_resolve_runtime_action_frame(const P4GameTables *tables,
    const struct P4GameSkills *skills, const struct P4GameItems *items, void *unit,
    uint32_t skill_id, uint32_t component, uint32_t status_mask,
    uint32_t native_options, P4GameActionFrame *frame);
int p4_game_elements_resolve_runtime_weapon(const P4GameTables *tables,
    const struct P4GameItems *items, uint32_t item_id, uint32_t component,
    P4GameActionElement *element);
#endif
