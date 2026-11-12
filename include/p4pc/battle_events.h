#ifndef P4PC_BATTLE_EVENTS_H
#define P4PC_BATTLE_EVENTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Copied host packets, never guest pointers. Callbacks run synchronously on
 * the game thread after the native commit; they must not mutate game state,
 * retain packet pointers, or alter subscriptions during delivery. Luau's
 * eventual bridge is one C subscriber, not another native publisher. */
typedef struct P4BattleActor {
    uint64_t action_uid;
    uint32_t unit_uid;
    uint16_t native_id;
    uint8_t side;
    uint8_t present;
} P4BattleActor;

typedef enum P4BattleAdmissionKind {
    P4_BATTLE_QUEUE_ORDINARY = 1,
    P4_BATTLE_QUEUE_FOLLOWUP = 2
} P4BattleAdmissionKind;

typedef struct P4BattleAdmission {
    P4BattleActor actor;
    uint32_t native_action_count;
    uint16_t next_state;
    uint8_t queue_kind;
    uint8_t reserved;
} P4BattleAdmission;

typedef enum P4BattleCostKind {
    P4_BATTLE_COST_HP = 1,
    P4_BATTLE_COST_SP = 2,
    P4_BATTLE_COST_ITEM = 3
} P4BattleCostKind;

typedef struct P4BattleCost {
    P4BattleActor actor;
    uint32_t requested_cost;
    uint16_t skill_id;
    uint16_t item_id;
    uint16_t hp_before, hp_after;
    uint16_t sp_before, sp_after;
    uint16_t item_count_before, item_count_after;
    uint8_t kind;
    uint8_t reserved[3];
} P4BattleCost;

typedef struct P4BattleHit {
    P4BattleActor source, target;
    int32_t requested_hp_delta, requested_sp_delta;
    int32_t requested_money_delta;
    uint32_t added_status, removed_status;
    uint32_t buff_changes;
    uint32_t status_before, status_after;
    uint16_t skill_id, action_kind;
    uint16_t hp_before, hp_after;
    uint16_t sp_before, sp_after;
    uint16_t target_flags, result_flags;
    uint16_t effect;
    uint8_t resources_applied;
    uint8_t statuses_applied;
} P4BattleHit;

/* Selected native command has left READY for its execution/presentation
 * owner. This is an execution attempt, not proof of a landed hit. Persona
 * and Analyze are explicitly free; admissions and commands are not rounds. */
typedef struct P4BattleCommand {
    P4BattleActor actor;
    uint16_t action_kind, skill_id, item_id, target_count, native_state;
    uint8_t free_action;
    uint8_t reserved;
} P4BattleCommand;

/* Natural request/constructor boundaries. Payload addresses are guest u32
 * values, never host pointers. BEGIN precedes the original fade/getter;
 * IDENTIFIED carries that getter's real u16 result. FIELD_BEGIN occurs only
 * on the first field request, before writes/allocation. COLLISION_READY is
 * the later 00172e00 ordinary field case10, immediately before constructor:
 * it is not the original collision or its advantage-RNG boundary.
 * TASK_BEGIN precedes btlInit; TASK_CREATED follows the real constructor. */
typedef enum P4BattleRequestStage {
    P4_BATTLE_SCRIPT_REQUEST_BEGIN = 1,
    P4_BATTLE_SCRIPT_REQUEST_IDENTIFIED = 2,
    P4_BATTLE_FIELD_REQUEST_BEGIN = 3,
    P4_BATTLE_TASK_BEGIN = 4,
    P4_BATTLE_TASK_CREATED = 5,
    P4_BATTLE_SCRIPT_GROUP_CREATED = 6,
    P4_BATTLE_FIELD_GROUP_CREATED = 7,
    P4_BATTLE_COLLISION_READY = 8
} P4BattleRequestStage;

typedef struct P4BattleRequest {
    uint32_t stage;
    uint32_t encounter_id; /* IDENTIFIED/GROUP/COLLISION u16; FIELD_BEGIN raw bits */
    uint32_t guest_object;  /* field task, script group, or battle setup */
    uint32_t value;         /* field flags/group, collision setup or TASK_CREATED return */
} P4BattleRequest;


enum {
    P4_BATTLE_OBSERVE_ADMISSION = 1u,
    P4_BATTLE_OBSERVE_COST = 2u,
    P4_BATTLE_OBSERVE_HIT = 4u,
    P4_BATTLE_OBSERVE_COMMAND = 8u,
    P4_BATTLE_OBSERVE_REQUEST = 16u
};

/* No Lua/mod/package switch participates in interest. Native bodies check
 * the relevant bit BEFORE taking snapshots or constructing a packet. */
extern uint32_t p4_battle_observer_interest;

typedef struct P4BattleObserver {
    void *context;
    void (*admission)(const P4BattleAdmission *, void *);
    void (*cost)(const P4BattleCost *, void *);
    void (*hit)(const P4BattleHit *, void *);
    void (*command)(const P4BattleCommand *, void *);
    void (*request)(const P4BattleRequest *, void *);
} P4BattleObserver;

/* Eight native brokers, not eight script packages. Return 0 on invalid,
 * full, busy or exhausted registration; tokens cannot alias stale handles.
 * Registration/removal is game-thread-only and outside event delivery. */
uint32_t p4_battle_observer_add(const P4BattleObserver *observer);
int p4_battle_observer_remove(uint32_t token);
void p4_battle_publish_admission(const P4BattleAdmission *event);
void p4_battle_publish_cost(const P4BattleCost *event);
void p4_battle_publish_hit(const P4BattleHit *event);
void p4_battle_publish_command(const P4BattleCommand *event);
void p4_battle_publish_request(const P4BattleRequest *event);

#ifdef __cplusplus
}
#endif

#endif
