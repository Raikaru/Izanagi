#ifndef P4PC_BATTLE_LIVE_H
#define P4PC_BATTLE_LIVE_H
#include <stddef.h>
#include <stdint.h>

typedef struct P4BattleLiveEntry {
    uint32_t flags, mode, encounter, occurrence; /* mode 3: occurrence is absolute flip ceiling */
    uint32_t route_limit, fight_limit, action_limit, completion;
    uint32_t endpoint_mask, field, room, date, step;
} P4BattleLiveEntry;

/* Fixed 64-byte P4BLIVE1 header, all scalar fields little-endian. */
int p4_battle_live_decode(const uint8_t *bytes, size_t length, P4BattleLiveEntry *out);
int p4_battle_live_start(const char *entry, const char *result, const char *trace);
void p4_battle_live_before_update(void);
void p4_battle_live_before_present(void);
void p4_battle_live_after_present(void);
void p4_battle_live_command_ready(uint32_t action);
void p4_battle_live_after_update(int completed);
/* 0 when inactive or an observation prefix was recorded; nonzero on error. */
int p4_battle_live_lockstep_stop(void);
int p4_battle_live_lockstep_eof(void);
int p4_battle_live_lockstep_boundary(void);
extern int p4_battle_live_active;
#endif
