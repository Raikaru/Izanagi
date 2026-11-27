#ifndef P4PC_BATTLE_ACTION_EXEC_NATIVE_H
#define P4PC_BATTLE_ACTION_EXEC_NATIVE_H
#include "p4pc/battle_action_exec.h"
/* Scalar-only engine adapters; no mirrored host-pointer records. */
void p4_action_exec_native_admit(uint32_t action);
void p4_action_exec_native_command(uint32_t action);
void p4_action_exec_native_end(uint32_t action);
void p4_action_exec_native_checkpoint(uint32_t action);
void p4_action_exec_native_packet_add(uint32_t packet);
typedef struct P4ActionExecUnit {
    uint32_t action, data, uid;
    uint16_t id;
    uint8_t side;
} P4ActionExecUnit;
/* Pure live identity query. Refuses absent/duplicate/stale data addresses. */
int p4_action_exec_native_unit(uint32_t data, P4ActionExecUnit *out);
#endif
