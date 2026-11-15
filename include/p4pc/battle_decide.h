#ifndef P4PC_BATTLE_DECIDE_H
#define P4PC_BATTLE_DECIDE_H
/* Battle decision stage (capability battle.decide@1, p4.battle.decide). At the
 * AI-to-READY transition of btlActionAiUpdate (0x001A2B50, overrides/001a2b50.c)
 * every enabled decider sees a copied snapshot of the deciding enemy's native
 * choice and the battle's units, and may keep it, choose a skill (any target
 * scope) with one legal target, or pass the turn. Legality is the game's own
 * target collector (btlSkillClassifyTargets/btlEffectCollectTargetUnits): pure
 * reads, no RNG. The override tests p4_battle_decide_interest first, so with no
 * decider the retail body runs unchanged. */
#include <stddef.h>
#include <stdint.h>

#define P4_BATTLE_DECIDE_UNITS 12u

typedef struct P4BattleDecideUnit {
    uint32_t action;          /* guest BtlAction of the unit */
    uint32_t uid;             /* unit uid (unit +0xA8) */
    uint16_t id;              /* character ID (party) or enemy ID */
    uint8_t side;             /* 0 party, 1 enemy */
    uint8_t alive;
    uint16_t hp, hp_max, sp, sp_max;
    uint32_t status;          /* DatUnit ailment word */
    uint8_t charged;          /* Power/Mind Charge state (DatUnit +0x14 & 0xC0000) */
    uint8_t reserved[3];
} P4BattleDecideUnit;

enum { P4_BATTLE_PICK_OTHER = 0, P4_BATTLE_PICK_SKILL = 1, P4_BATTLE_PICK_PASS = 2 };

typedef struct P4BattleDecideState {
    uint32_t count;           /* units[0..count) in battle-list order */
    uint32_t actor;           /* index of the deciding unit */
    P4BattleDecideUnit units[P4_BATTLE_DECIDE_UNITS];
    uint16_t native_kind;     /* the command record's kind (+0x6C) */
    uint16_t skill;           /* its skill (+0x6E) when pick == SKILL */
    uint8_t pick;             /* P4_BATTLE_PICK_* */
    uint8_t target_count;
    uint8_t targets[12];      /* unit indices */
    uint8_t one_more;         /* BtlAction +0x28: a One More turn */
    uint8_t extra;            /* BtlAction +0x29: a native extra action */
    uint16_t round;           /* BtlBattleState +0x20: protagonist base turns */
} P4BattleDecideState;

typedef struct P4BattleDecision {
    uint8_t pick;             /* P4_BATTLE_PICK_OTHER keeps the current choice */
    uint8_t has_target;
    uint16_t skill;
    uint32_t target;          /* unit index when has_target */
} P4BattleDecision;

/* Legal targets of skill for the deciding unit: indices into state->units, and
 * whether the skill hits all of them. 0, or -1 for a skill outside the table. */
int p4_battle_decide_candidates(const P4BattleDecideState *state, uint16_t skill,
                                uint8_t *indices, uint32_t *count, int *all);

/* 0 = decision filled (OTHER keeps); -1 = callback failure (mod recovery). */
typedef int (*P4BattleDecideFn)(const P4BattleDecideState *state, P4BattleDecision *decision, void *user);
/* After the stage applied this decider's decision (save provenance). */
typedef void (*P4BattleDecideAppliedFn)(void *user);
typedef struct P4BattleDecider P4BattleDecider;
/* Game thread, outside a decision. Lower priority runs first, then owner order. */
P4BattleDecider *p4_battle_decider_add(size_t owner, int32_t priority, P4BattleDecideFn callback,
                                       P4BattleDecideAppliedFn applied, void *user);
int p4_battle_decider_enable(P4BattleDecider *decider, int enabled);
int p4_battle_decider_remove(P4BattleDecider *decider);
void p4_battle_deciders_remove_owner(size_t owner);

extern uint32_t p4_battle_decide_interest;
/* overrides/001a2b50.c, once the AI released its command record. */
void p4_battle_decide_ready(uint32_t action);
#endif
