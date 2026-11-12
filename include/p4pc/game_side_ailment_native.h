#ifndef P4PC_GAME_SIDE_AILMENT_NATIVE_H
#define P4PC_GAME_SIDE_AILMENT_NATIVE_H
#include <stdint.h>
/* Scalar-only lifecycle boundary of the side-ailment service for the
 * translated battle units (game_side_ailment.h). Inert with no row declared. */
/* btlInit 0x00192B90: the battle, all of its encounter waves. */
void p4_game_side_ailment_native_battle_begin(void);
/* btlActionExitUpdate 0x001B05D0, before the unit is freed: its guest DatUnit. */
void p4_game_side_ailment_native_unit_exit(uint32_t unit);
/* btlRelease 0x00192CD0. */
void p4_game_side_ailment_native_battle_end(void);
#endif
