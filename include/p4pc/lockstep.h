#ifndef P4PC_LOCKSTEP_H
#define P4PC_LOCKSTEP_H
#include <stddef.h>
#include <stdint.h>
/* Optional pad observer, registered before kwlnBoot; pad-only programs need no harness. */
void p4pc_pad_set_packet_hook(void (*hook)(uint8_t *, size_t));
/* Optional capture-control observer; the bridge excludes repeated key-downs. */
void p4pc_pad_set_key_hook(void (*hook)(int scancode));
void p4pc_lockstep_init(void);
void p4pc_lockstep_frame(void (*capture)(const char *));
void p4pc_lockstep_presented(void);
void p4pc_lockstep_pace(void);
/* Full draw diagnostics; strict scenario replay retains gate draw windows. */
int p4pc_lockstep_draw_active(void);
void p4pc_lockstep_drawline(const char *line);
/* Flushes the recording (input log and per-frame states); 0 on success or when nothing records. */
int p4pc_lockstep_flush(void);
/* In a forked battle-search child (battle_sim/live.c), continues the recording in DIR without a new header: the
 * parent's files hold everything up to the fork, so the segments concatenate to one run's log. 0 on success. */
int p4pc_lockstep_fork_reopen(const char *dir);
#endif
