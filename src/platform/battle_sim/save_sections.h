#ifndef P4PC_BATTLE_SAVE_SECTIONS_H
#define P4PC_BATTLE_SAVE_SECTIONS_H
#include <stdint.h>

/* The simulator and lockstep must validate the same native section layout. */
static const struct P4BattleSaveSection {
    uint32_t tag, bytes;
} p4_battle_save_sections[] = {
#define P4LS_SAVE_SECTION(tag, address, bytes) { tag, bytes },
#include "p4pc/lockstep_save_sections.inc"
#undef P4LS_SAVE_SECTION
};
#endif
