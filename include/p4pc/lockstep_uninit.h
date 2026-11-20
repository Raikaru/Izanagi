/* Lockstep-only sync of words retail reads before it writes them (src/platform/lockstep_uninit.c,
 * docs/lockstep.md "Uninitialised words").
 *
 * The thunder effect's bolt list creators (ThunderGeometryCreate 0x00495160,
 * ThunderGeometryCreatePadded 0x00496340, ThunderPairedGeometryCreate 0x00497750) take a heap
 * block and write only each bolt's geometry, age and (simple bolts) base angle; the bolt updates
 * then test the never-written colour (the padded bolts' other fields too). Its value is whatever
 * the block's previous occupant left, which the port's heap does not reproduce. In an online
 * lockstep replay (P4PC_LOCKSTEP_DISC_ONLINE) each list the port creates after the initial anchor
 * is retail's list of the same index: same creator, update and copies, and the same written words;
 * the unwritten words are copied from retail before anything reads them. P4PC_LOCKSTEP_UNINIT_LOG
 * records every list the port creates, before any copy, in any run. Normal play is unchanged. */
#ifndef P4PC_LOCKSTEP_UNINIT_H
#define P4PC_LOCKSTEP_UNINIT_H

#include <stdint.h>

#define P4LU_SIMPLE 0x00495160u
#define P4LU_PADDED 0x00496340u
#define P4LU_PAIRED 0x00497750u

/* Opens DIRECTORY/port-uninit.tsv for an online replay; returns 1 when one is configured. */
int p4pc_lockstep_uninit_open(const char *directory);
/* At the online replay's anchor: from the next update on, every list is synced. */
void p4pc_lockstep_uninit_activate(uint32_t anchor_frame);
/* A creator's new bolt list (its return value) and its copies; called by every creator. It does
 * nothing unless P4PC_LOCKSTEP_UNINIT_LOG is set (read once) or the online replay is active. */
void p4pc_lockstep_thunder_list(uint32_t creator, void *list, uint32_t copies);

/* The list block's size in words, its pointer words (never compared or copied) and the words its
 * creator writes (compared), for one creator; 0 for an unknown creator. */
static inline uint32_t p4lu_words(uint32_t creator, uint32_t copies) {
    switch (creator) {
    case P4LU_SIMPLE: return 1u + copies * 4u;
    case P4LU_PADDED: return 4u + copies * 12u;
    case P4LU_PAIRED: return 1u + copies * 3u;
    default: return 0;
    }
}
/* 0 an unwritten word (copied), 1 a pointer (skipped), 2 a word the creator writes (compared). */
static inline int p4lu_word_class(uint32_t creator, uint32_t word) {
    switch (creator) {
    case P4LU_SIMPLE: /* entries; geometry, age, colour, base angle */
        if (word == 0) return 1;
        return (word - 1u) % 4u == 0 ? 1 : (word - 1u) % 4u == 2 ? 0 : 2;
    case P4LU_PADDED: /* entries, radius, radius step, allocation; geometry, 4 unwritten, age, 6 unwritten */
        if (word < 4) return word == 0 || word == 3 ? 1 : 0;
        return (word - 4u) % 12u == 0 ? 1 : (word - 4u) % 12u == 5 ? 2 : 0;
    case P4LU_PAIRED: /* entries; geometry, age, colour */
        if (word == 0) return 1;
        return (word - 1u) % 3u == 0 ? 1 : (word - 1u) % 3u == 1 ? 2 : 0;
    default: return 1;
    }
}

#endif
