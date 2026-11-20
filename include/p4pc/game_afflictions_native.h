#ifndef P4PC_GAME_AFFLICTIONS_NATIVE_H
#define P4PC_GAME_AFFLICTIONS_NATIVE_H
#include <stdint.h>
/* Afflictions this battle (docs/modding/tables.md "Affliction count"): how
 * many ailments landed on each unit since the battle began, for the status
 * chance and wear-off stages' prior-affliction rules. datCalcSetBadStatus
 * 0x00232680 reports each landing; the battle constructor's TASK_BEGIN
 * (00192E90) clears every count. Units are the live host views of guest
 * DatUnit records. Counting runs only while a chance or wear-off modifier is
 * registered. Main-thread battle code only. */
/* Rage, fear, silence, confusion, poison, exhaustion and enervation; Dizzy
 * (0x01) and KO are not afflictions. */
#define P4_AFFLICTION_MASK 0xFEu
int p4_game_afflictions_native_active(void);
/* One landing of statuses (bits outside the mask are ignored). */
void p4_game_afflictions_native_landed(const void *unit, uint32_t statuses);
uint32_t p4_game_afflictions_native_count(const void *unit);
#endif
