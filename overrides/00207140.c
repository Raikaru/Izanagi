#include "p4pc/game_items_native.h"
#include "p4pc/game_skills_native.h"
/* Saved cursors use the shared action target-block union and four typed cursor records. */
void btlSelectionRefreshList(u16 *flags, u8 *work)
{
    u8 *action;
    u8 *unit;
    u8 *persona;
    u16 skillSlot;
    u16 skillCount;
    u16 skill;

    if (datGetFlag(0x3C) != 0) {
        action = (u8 *)((BtlAction *)(((BtlPanelWork *)work)->action))->targetBlock.recovery.savedCursors;
        if (action != NULL) {
            btlSelectionUpdateWindow((u8 *)flags, ((BtlSelectionCursorRecord *)action)->slots[0].selected, *(s16 *)action);
        }
    }
    *flags |= 4;
    *flags |= 2;
    skillCount = 0;
    unit = (u8 *)((BtlAction *)((BtlPanelWork *)work)->action)->unit;
    if (unit[0xA2] == 0) {
        persona = (u8 *)datPersonaGetRecord(((BtlUnit *)unit)->charId);
        for (skillSlot = 0; skillSlot < 8; skillSlot++) {
            skill = datPersonaGetSkillAt((DatPersonaRecord *)persona, (s16)skillSlot);
            if (skill != 0 && p4_game_skill_active(skill) && (((DatSkillEntry *)iGpffffb3b8)[skill].flags01 & 2)) {
                ((BtlPanelWork *)work)->skills[skillCount] = skill;
                skillCount++;
            }
        }
    }
    ((BtlPanelWork *)work)->skillCount = skillCount;
    ((BtlPanelWork *)work)->itemCount = p4_game_tables_items_native_battle_refresh(work);
}
