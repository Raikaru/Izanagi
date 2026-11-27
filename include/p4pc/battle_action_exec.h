#ifndef P4PC_BATTLE_ACTION_EXEC_H
#define P4PC_BATTLE_ACTION_EXEC_H
#include <stdint.h>

/* Native synchronous execution contract; battle.execution@1 is its Luau view.
 * See docs/platform/battle_action_exec.md for evidence and limits.
 * All addresses are guest words. Tokens never alias BtlAction.uid. Install
 * before battle creation; callbacks copy facts and must not reenter the engine
 * or change the observer. Missing snapshots and UNPROVEN always fail closed. */
enum { P4_EXEC_ADMITTED = 1, P4_EXEC_COMMITTED, P4_EXEC_BASE_BOUNDARY, P4_EXEC_TERMINAL,
       P4_EXEC_UNCERTAIN, P4_EXEC_BATTLE_BEGIN, P4_EXEC_BATTLE_END, P4_EXEC_LOST };
enum {
    P4_EXEC_RUNNING, P4_EXEC_RESOLVED, P4_EXEC_HITLESS, P4_EXEC_DIRECT_END,
    P4_EXEC_REMOVED, P4_EXEC_FLUSHED, P4_EXEC_BATTLE_RELEASE,
    P4_EXEC_REPLACED, P4_EXEC_UNPROVEN
};
enum {
    P4_EXEC_UNKNOWN_WORK = 1u, P4_EXEC_CAPACITY = 2u,
    P4_EXEC_ORPHAN_RESUME = 4u, P4_EXEC_CONFLICT = 8u
};
#define P4_EXEC_RECORDS 64u
#define P4_EXEC_PACKETS 2048u

typedef struct P4ActionExec {
    uint64_t token, action_uid;
    uint32_t action, unit_uid, unit_data;
    uint32_t pending_packets, result_packets, uncertainty;
    uint16_t native_id, command, skill;
    uint8_t side, base_turn, committed, direct_end;
    uint8_t end_seen, end_processed, sealed, base_boundary, outcome;
} P4ActionExec;
typedef void (*P4ActionExecObserver)(unsigned event, const P4ActionExec *execution, void *context);
extern unsigned p4_action_exec_enabled;
extern unsigned p4_action_exec_battle_active;
extern uint64_t p4_action_exec_battle_epoch;
/* Refuses replacement/removal while work is live or from inside a callback.
 * Installing a sink does not invent identities for already admitted actions. */
int p4_action_exec_observe(P4ActionExecObserver observer, void *context);
int p4_action_exec_snapshot(uint64_t token, P4ActionExec *out);
uint64_t p4_action_exec_current(uint32_t action);
/* Inputs to the same reducer used by native adapters and synthetic C tests. */
uint64_t p4_action_exec_admit(uint32_t action, uint64_t action_uid, uint32_t unit_uid,
                            uint16_t native_id, uint8_t side, uint16_t flags,
                            uint8_t one_more, uint8_t extra, uint32_t unit_data);
void p4_action_exec_resume(uint32_t action);
void p4_action_exec_command(uint32_t action, uint16_t command, uint16_t skill);
void p4_action_exec_end_seen(uint32_t action, int direct_end);
void p4_action_exec_end_processed(uint32_t action, int restart);
void p4_action_exec_standby(uint32_t action);
void p4_action_exec_base_boundary(uint32_t action);
void p4_action_exec_remove(uint32_t action);
void p4_action_exec_flush(void);
void p4_action_exec_release(void);
void p4_action_exec_battle_begin(void);
/* Save/restore scopes bracket ALL action init/update and packet init/update/
 * destroy callbacks, including delayed initializers and nested callbacks. */
uint64_t p4_action_exec_enter_action(uint32_t action);
uint64_t p4_action_exec_enter_packet(uint32_t packet);
void p4_action_exec_leave(uint64_t previous);
void p4_action_exec_packet_add(uint32_t packet, uint64_t action_uid, uint32_t type);
void p4_action_exec_packet_remove(uint32_t packet, int completed);
#endif
