#ifndef P4PC_LOCKSTEP_BATTLE_BOUNDARY_H
#define P4PC_LOCKSTEP_BATTLE_BOUNDARY_H

#include <stdint.h>

/* SLUS_217.82: FldEvent_FieldToBattle calls btlTaskCreate at 0x173d40;
 * the delay slot is a nop and the constructor's return address is 0x173d48.
 * This decoder observes only the ordinary collision case, never mutates RAM,
 * and treats a broken chain at that callsite as an error, not a missed match. */
typedef int (*P4BattleBoundaryRead)(void *user, uint32_t address, uint32_t *value);
typedef struct {
    uint32_t work, setup, actor, group, encounter;
} P4BattleBoundary;
typedef struct {
    uint32_t task, work, encounter, flags;
} P4BattleFieldRequest;

static int p4battle_address(uint32_t address, uint32_t size)
{
    return address >= 0x00100000u && address <= 0x02000000u - size;
}

/* Entry to FldEvent_BeginEncounter: zero latch, before any field mutation.
 * 0: an existing request is being polled; 1: new request; -1: bad chain. */
static int p4battle_field_request(P4BattleBoundaryRead read, void *user,
                                  uint32_t task, uint32_t encounter, uint32_t flags,
                                  P4BattleFieldRequest *out)
{
    uint32_t work, latch;
    if (!read || !out || !p4battle_address(task, 0x3cu) || !read(user, task + 0x38u, &work) ||
        !p4battle_address(work, 0x110u) || !read(user, work + 0x10cu, &latch)) return -1;
    if (latch) return 0;
    *out = (P4BattleFieldRequest){task, work, encounter & 0xffffu, flags};
    return 1;
}

/* 0: another call/case; 1: ordinary collision; -1: malformed witness. */
static int p4battle_collision(P4BattleBoundaryRead read, void *user,
                              uint32_t ra, uint32_t s2, uint32_t setup,
                              P4BattleBoundary *out)
{
    uint32_t latch, actor, group, actor_group, identity;
    if (ra != 0x00173d48u) return 0;
    if (!read || !out || !p4battle_address(s2, 0x110u) || setup != s2 + 0x50u ||
        !read(user, s2 + 0x10cu, &latch)) return -1;
    if (latch) return 0;
    if (!read(user, s2 + 0x44u, &actor) || !p4battle_address(actor, 0x4cu) ||
        !read(user, setup + 0x14u, &group) || !p4battle_address(group, 12u) ||
        !read(user, actor + 0x48u, &actor_group) || group != actor_group ||
        !read(user, group + 8u, &identity)) return -1;
    *out = (P4BattleBoundary){s2, setup, actor, group, identity & 0xffffu};
    return 1;
}
#endif
