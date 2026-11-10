#ifndef P4PC_GAME_ENCOUNTERS_H
#define P4PC_GAME_ENCOUNTERS_H
/* ENCOUNT.TBL blocks by packed-bank span, with their SLUS_217.82 005f66c0
 * pointer slots and retail row counts:
 * 0 formations (0x00764504, iGpffffb414): 24-byte rows by formation ID, five
 *   u16 enemy IDs at +8 (0 is none); 912.
 * 1 selectors (0x0076450C, iGpffffb41c): 10-byte rows by field step index, a
 *   u16 weighted group at +0 and a u16 record group at +6; 160.
 * 2 a 200-byte block nothing reads (0x00764510).
 * 3 weighted groups (0x00764508, iGpffffb418): 248-byte rows, an 8-byte rate
 *   header, then two lists (normal day, alternate day) of 30 (u16 formation,
 *   u16 weight) entries; 56.
 * 4 record groups (0x00764514, iGpffffb424): 348-byte rows of 29 weighted
 *   12-byte field-object (treasure) records, u16 weight then u16 item ID (0 is
 *   none), and at +7 a kind bit (bit 0) that Encount_FindRecord's mode 0 or 1
 *   selects; 75.
 * Every reader indexes these rows by an ID taken from another row, field data
 * or a script; none uses a count (Encount_* 0x00161630..0x00161C80, the
 * battle and field formation readers, datUnitCreateEncounterGroup
 * 0x00231630). */
#define P4_ENCOUNT_FORMATION_SPAN 0u
#define P4_ENCOUNT_FORMATION_SIZE 24u
#define P4_ENCOUNT_FORMATION_ENEMIES 8u
#define P4_ENCOUNT_FORMATION_ENEMY_COUNT 5u
#define P4_ENCOUNT_SELECTOR_SPAN 1u
#define P4_ENCOUNT_SELECTOR_SIZE 10u
#define P4_ENCOUNT_SELECTOR_WEIGHTED 0u
#define P4_ENCOUNT_SELECTOR_RECORDS 6u
#define P4_ENCOUNT_WEIGHTED_SPAN 3u
#define P4_ENCOUNT_WEIGHTED_SIZE 248u
#define P4_ENCOUNT_WEIGHTED_HEADER 8u
#define P4_ENCOUNT_WEIGHTED_ENTRIES 60u
#define P4_ENCOUNT_RECORD_SPAN 4u
#define P4_ENCOUNT_RECORD_SIZE 348u
#define P4_ENCOUNT_RECORD_COUNT 29u
#define P4_ENCOUNT_RECORD_STRIDE 12u
#define P4_ENCOUNT_RECORD_ITEM 2u
#define P4_ENCOUNT_RECORD_FLAGS 7u
/* Port bound on a relocated block's rows; every index carrier is a u16. */
#define P4_ENCOUNT_ROW_MAX 4096u
#endif
