#ifndef P4PC_GAME_SKILLS_NATIVE_H
#define P4PC_GAME_SKILLS_NATIVE_H
#include <stdint.h>
#include "p4pc/game_table_admission_native.h"
/* Scalar-only bridge for translated game units; no host registry layout. */
#define P4_SKILL_RETAIL_ACTIVE_COUNT 440u
#define P4_SKILL_RETAIL_COUNT 576u
/* Public catalog ceiling: 001DE640 packs AI plan/escape tags at0x1000/0x2000.
 * Keep skill IDs below those tags and preserve signed-halfword/-1 carriers.
 * A larger catalog requires a distinct typed AI skill carrier, not precedence
 * changes that reinterpret existing packed commands. */
#define P4_SKILL_MAX_ID 4095u
#define P4_SKILL_ROW_SIZE 40u
#define P4_SKILL_ELEMENT_SIZE 2u
#define P4_SKILL_INHERITANCE_GROUP_COUNT 19u
/* Signed -1 in the legacy element byte; never a low-byte alias of a new leaf. */
#define P4_SKILL_LEGACY_ELEMENT_INVALID 0xffu
int p4_game_skill_registered(uint32_t id);
int p4_game_skill_active(uint32_t id);
/* Expanded text is a guest-backed native stream; zero means no expanded row.
 * Retail text remains owned by the game's existing banks. */
uint32_t p4_game_skill_name(uint32_t id);
uint32_t p4_game_skill_description(uint32_t id);
/* Retail negative element sentinels stay signed. Native admission refuses any
 * new semantic u32 reference that this legacy result/consumer path would narrow. */
int32_t p4_game_skill_element(uint32_t id, int32_t retail_element);
/* Returns zero only for the untouched retail-bank domain. New IDs must resolve
 * an owned MSG stream; invalid expanded references fail rather than indexing a
 * retail bank. The caller retains the original draw for a zero result. */
int p4_game_skill_draw_description(uint32_t id, float x, float y, float scale,
    uint32_t color, uint32_t style, uint32_t effect, uint32_t flags);
/* The legacy endurance-cache empty value 576 is a valid new skill. Expanded
 * sessions use the first value outside the allocatable skill domain. */
uint16_t p4_game_skill_endurance_sentinel(void);
int p4_game_skills_expanded(void);
int32_t p4_game_skill_inheritance_rate(uint32_t id);
/* This selector indexes the legacy19-column inheritance matrix; it is not an
 * effective element ID. Retail -1 normalization remains in the original caller. */
int32_t p4_game_skill_inheritance_group(uint32_t id, int32_t retail_group);
uint16_t p4_game_skill_pick_mutation(const uint16_t *known_skills, uint32_t excluded_id);
/* Packed-load participation through p4_game_tables_observe_packed only. Scan
 * returns the skill banks among the actual loader's blocks (0 without staging)
 * and their row count; accept records them after the shared authentication. */
int p4_game_skills_packed_pending(void);
uint32_t p4_game_skills_packed_scan(const P4GameTablePackedBlock *blocks, unsigned count, uint32_t *rows);
void p4_game_skills_packed_accept(uint32_t mask);
/* Actual packed-table load start, including reloads; no work without staging. */
void p4_game_skills_load_begin(void);
#endif
