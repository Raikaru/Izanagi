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
#endif
