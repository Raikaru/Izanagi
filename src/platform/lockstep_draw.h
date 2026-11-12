#ifndef P4PC_LOCKSTEP_DRAW_H
#define P4PC_LOCKSTEP_DRAW_H
#include <stddef.h>

/* Host-owned retained frame storage, never a guest-memory layout. */
typedef struct {
    char *data;
    size_t length, capacity;
} P4LockstepDrawBuffer;

/* NULL on success; failures leave the retained buffer unchanged. */
const char *p4pc_lockstep_draw_append(P4LockstepDrawBuffer *buffer, const char *line);
#endif
