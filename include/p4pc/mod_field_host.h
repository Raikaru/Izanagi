#ifndef P4PC_MOD_FIELD_HOST_H
#define P4PC_MOD_FIELD_HOST_H
/* The port's field host: one native UI session that holds the field the way a
 * retail field interaction does (api-research §5.5, §10.1). Today it shows the
 * game's own "forget which skill?" choice for p4.give.skill { choice = true }
 * (give.skill.choice@1, game_party.c).
 *
 * Seam. FldEvent_TriggerUpdate 0x001747D0 (overrides/001747d0.c) polls its
 * eleven retail {test, run} pairs first; only when none fired and no encounter
 * is pending does it ask p4_mod_field_host_enter, and on 1 it enters mode 1 as
 * for any retail trigger (idle animation, camera flag 2, prompt icon off) and
 * calls p4_mod_field_host_run every frame until it returns 0. Mode 1 is what
 * keeps the player controller and every other trigger (Camp, map, NPC talk,
 * examine points, encounters) from reading the pad while the panel does.
 *
 * Session (all native, the battle result's own sequence for a party member,
 * btlResultFriendPsUpdate 0x002239A0 / the hero step 0x002232A0, as the field's
 * FldFieldScriptTriggerRun 0x00170E50 holds the field):
 *   enter: armed, no live field script (0x00764604 == 0), the nm current
 *     window free (0x00882F60 < 0), no shdPersonaStatusDraw panel alive, Camp
 *     and native Config closed, and still a settled field: no sequence change
 *     pending and the field root running (revalidated here, because the offer
 *     was made at the frame boundary before frame_start callbacks and the
 *     frame's tasks ran);
 *   run 1: fclMiscStopAndClose, fclMiscSetDoneFlag, FldFrameObjSetActive(
 *     FldFrame.supporterTask, 1), FldSlotBroadcastState(1); H_Cdvd_AcquireDirectory
 *     of the result screen's battle/result/br_msg.bin (gp-0x5A50, flags 1:
 *     H_Cdvd_BuildVolumePaths gives every \BATTLE\RESULT\ path the BTL volume,
 *     in any scene); request the acquisition sprite using the native sprite
 *     loader; shdPersonaTaskCreate(task, 15, 4, 5, 0) as a child of the
 *     controller, SetOrigin {0, 19}, StartSlideIn, SetPersonaId 174,
 *     SetDisplayState(panel, member, record, 0);
 *   open: until the panel stops transitioning, the PAK is loaded and the sprite
 *     loader reaches state 5; set panel work +0x2c8 as the native state-3
 *     initializer does, then shdPersonaSetPendingSkill (row 8 is the offered
 *     skill) and shdPersonaSkillReplaceCreate with the result message ids;
 *   choose: until datBlockStageIsEight, then destroy the replace task, release
 *     its PAK and finish the panel. Keep the field held and the sprite alive
 *     until the panel task has unlinked, then release the sprite and field.
 * The replace task writes the Persona record itself (datPersonaReplaceSkill);
 * the outcome is read from the record: learned iff the offered skill is in it,
 * forgotten = the slot whose skill changed. The player declines by picking the
 * offered skill (row 8) and answering Yes; the native UI has no cancel button.
 * No RNG is drawn and no story flag is touched. */
#include <stdint.h>

#define P4_FIELD_HOST_SLOT 11 /* after the retail pairs 0..10 of D_005F17D0 */

/* overrides/001747d0.c: 1 takes the slot this frame (task is the controller,
 * a guest address); run returns 1 while the session lasts, 0 when it ended. */
int p4_mod_field_host_enter(uint32_t task);
int p4_mod_field_host_run(uint32_t task);

/* game_party.c and game_services.c, at the services frame boundary or in a
 * callback. offer arms (or re-arms) the next session for member's Persona and
 * skill: 0, or -1 while one runs or its outcome waits. withdraw disarms a
 * session that has not started (a scene request accepted since the offer
 * wins). busy: a session holds the field (the scene services are not ready
 * then). member: the member whose Persona the open panel shows, 0 when none:
 * its skills are the panel's until the session ends, so other skill changes
 * to that Persona wait (game_party.c). */
int p4_mod_field_host_offer(uint32_t member, uint32_t skill);
void p4_mod_field_host_withdraw(void);
int p4_mod_field_host_busy(void);
uint32_t p4_mod_field_host_member(void);
/* 1 once a session has ended, with its outcome (collected: the next call
 * returns 0): learned, the forgotten skill (0 when none) and, when not
 * learned, why ("declined" when the player chose to keep every skill). A
 * running session ends here when its controller is gone (the field closed
 * under it) or with abandon set (no game is loaded any more). Reset the nm
 * window and request task teardown; release the PAK and sprite only after the
 * corresponding tasks have unlinked. */
int p4_mod_field_host_result(int abandon, int *learned, uint16_t *forgot, const char **reason);
#endif
