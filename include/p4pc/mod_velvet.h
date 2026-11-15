/* One mod command in the Velvet Room (p4.velvet.command, docs/modding/lua.md
 * "Velvet Room command"). While the Velvet root menu (fclCombineMenuRoot
 * 0x002E90D0, state f0 0 / f1 0x18) waits for input, the command's hint is drawn
 * under the root list and Square opens the command's registered native page,
 * hosted by the Config task inside the Velvet task (p4_settings_native_host).
 * Square is read by no Velvet root state. An operation (weapon_fusion) is asked
 * at stage OFFER first; a denial hides the hint and the command. Game thread. */
#ifndef P4PC_MOD_VELVET_H
#define P4PC_MOD_VELVET_H

#include <stddef.h>
#include <stdint.h>

typedef struct P4ModVelvetCommand P4ModVelvetCommand;
/* At most one command at a time; a second registration is refused (NULL).
 * page is a registered p4.ui page key; label is RAW font text, 1..47 bytes;
 * operation is P4_MOD_AUTH_WEAPON_FUSION (asked at stage OFFER) or -1. */
P4ModVelvetCommand *p4_mod_velvet_add(size_t owner, const char *page, const char *label, int operation);
int p4_mod_velvet_enable(P4ModVelvetCommand *command, int enabled);
int p4_mod_velvet_remove(P4ModVelvetCommand *command);
void p4_mod_velvet_remove_owner(size_t owner);
/* 1 while the command's page is open over the Velvet root. */
int p4_mod_velvet_open(void);

#endif
