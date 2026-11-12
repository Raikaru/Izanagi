#ifndef P4PC_MOD_BATTLE_MENU_H
#define P4PC_MOD_BATTLE_MENU_H
#include <stdint.h>
/* The battle command panel as the game itself drives it (p4.battle).
 *
 * Native observers on the panel's own input handlers record which panel is
 * waiting for the acting unit's input this frame; requests then act through
 * the game's own flow: a chosen command makes the next input read of that panel
 * return confirm (so availability checks, sounds and the next panel are the
 * game's), a pointed row moves a list cursor, and an aimed unit becomes the
 * default of the next target selection of that action. Nothing touches the pad
 * and nothing executes without the game's own next step. Game thread only. */
enum {
    P4_BATTLE_PANEL_NONE = 0,
    P4_BATTLE_PANEL_COMMANDS,
    P4_BATTLE_PANEL_SKILLS,
    P4_BATTLE_PANEL_ITEMS,
    P4_BATTLE_PANEL_TACTICS,
    P4_BATTLE_PANEL_PERSONA,
};
/* The command circle's rows, in the game's order. */
enum {
    P4_BATTLE_ANALYZE = 0, P4_BATTLE_TACTICS, P4_BATTLE_GUARD, P4_BATTLE_ATTACK,
    P4_BATTLE_SKILL, P4_BATTLE_PERSONA, P4_BATTLE_ITEM, P4_BATTLE_ESCAPE, P4_BATTLE_COMMANDS
};
typedef struct P4BattleMenu {
    int panel;
    uint32_t selection, work, action, unit; /* guest addresses */
    int cursor, first, count, page;
} P4BattleMenu;
/* A wait is one panel of one action waiting for the controlled actor's input
 * (the player's own command, never party AI, Rush, enemies or the battle
 * simulator). The first owner whose request is accepted drives that wait; the
 * others are refused until it ends. */
/* Creates the panel observers, disabled, while no hook dispatch is active
 * (mod VM initialization); later calls do nothing. */
void p4_battle_menu_prepare(void);
/* 0 and the waiting panel, or -1 when none waits (enables the observers on
 * first use, from any callback). */
int p4_battle_menu(P4BattleMenu *menu);
/* Requests return 0, -1 when refused, or -2 when another owner drives this
 * wait (its owner then in *driver). */
/* Commands panel: the next input read of the panel moves to command and confirms. */
int p4_battle_menu_choose(const void *owner, int command, const void **driver);
/* A list panel: moves the cursor (and the first visible row) to row. */
int p4_battle_menu_point(const void *owner, int row, const void **driver);
/* unit becomes the default of the waiting action's next target selection
 * when the game offers it as a candidate. */
int p4_battle_menu_aim(const void *owner, uint32_t unit, const void **driver);
/* Called once when a confirm the player made (or a chosen command's confirm)
 * ends a wait the owner influenced: that is when its use is recorded. */
void p4_battle_menu_on_use(void (*use)(const void *owner));
/* Drops the owner's requests and ownership (VM teardown). */
void p4_battle_menu_release(const void *owner);
#endif
