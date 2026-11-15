#ifndef P4PC_BATTLE_SEMANTIC_H
#define P4PC_BATTLE_SEMANTIC_H
/* Semantic battle events (capabilities events.turn_start, events.skill_used,
 * events.damage, events.miss, events.ailment, events.knockdown, events.ko,
 * events.revive, events.one_more, events.reward, events.level_up and
 * events.skill_learned, each @1), derived from the native battle observer's
 * copied commit packets (include/p4pc/battle_events.h) and the native battle
 * notifications (include/p4pc/battle_notify.h). One observer, registered only
 * while some kind has a subscriber; records queue in commit order and are
 * drained at the next frame safe point, so subscribers run outside the
 * observer's no-mutation window. The packet decoding follows
 * local://api-research.md section 6 (btlTargetResolveSelectedSkill rows). */
#include <stddef.h>
#include <stdint.h>
#include "p4pc/battle_events.h"

enum P4SemanticKind {
    P4_SEMANTIC_TURN_START = 0,
    P4_SEMANTIC_SKILL_USED,
    P4_SEMANTIC_DAMAGE,
    P4_SEMANTIC_MISS,
    P4_SEMANTIC_AILMENT,
    P4_SEMANTIC_KNOCKDOWN,
    P4_SEMANTIC_KO,
    P4_SEMANTIC_REVIVE,
    P4_SEMANTIC_ONE_MORE,
    P4_SEMANTIC_REWARD,
    P4_SEMANTIC_LEVEL_UP,
    P4_SEMANTIC_SKILL_LEARNED,
    P4_SEMANTIC_KIND_COUNT
};
/* one_more: what earned it (the action's hits: a weakness, else a critical, else a Down). */
enum P4SemanticCause { P4_SEMANTIC_CAUSE_DOWN = 0, P4_SEMANTIC_CAUSE_WEAKNESS, P4_SEMANTIC_CAUSE_CRITICAL };
enum P4SemanticResult {
    P4_SEMANTIC_HIT = 0, P4_SEMANTIC_WEAK, P4_SEMANTIC_CRITICAL, P4_SEMANTIC_NULL, P4_SEMANTIC_DRAIN
};
#define P4_SEMANTIC_AILMENT_BITS 0x7FFFFu /* DatUnit.bad bits 0-18; 19 is KO, 20 Down */
#define P4_SEMANTIC_KO_BIT 0x80000u
#define P4_SEMANTIC_DOWN_BIT 0x100000u

typedef struct P4SemanticUnit {
    uint32_t uid;
    uint16_t id;       /* playable character ID or enemy ID */
    uint8_t side;      /* 0 party, 1 enemy */
    uint8_t present;
} P4SemanticUnit;

typedef struct P4SemanticEvent {
    uint32_t sequence;          /* commit order, from 1 */
    uint8_t kind;               /* P4SemanticKind */
    uint8_t result;             /* damage: P4SemanticResult */
    uint8_t flag;               /* turn_start: follow-up; damage: reflected; ailment: 1 applied, 0 removed */
    uint8_t endured;            /* damage: the target endured at 1 HP */
    P4SemanticUnit source;      /* turn/skill: the actor; hits: the attacker */
    P4SemanticUnit target;      /* hits and status changes: the unit hit */
    uint16_t skill, item, action_kind;
    uint16_t hp_before, hp_after;
    int32_t amount;             /* damage: HP lost (negative when drained); reward: the yen given */
    uint32_t status;            /* ailment: the one DatUnit.bad bit */
    /* reward: the protagonist's EXP, the items given and the party members' EXP. */
    int32_t exp;
    uint8_t item_count, party_count;
    uint16_t items[3], item_counts[3];
    uint16_t party[4];
    int32_t party_exp[4];
    /* level_up (target: the member; persona 0 for the protagonist's own level) and skill_learned. */
    uint16_t persona;
    uint8_t level, from;
} P4SemanticEvent;

/* -1 for a name that is not a semantic battle event. */
int p4_semantic_kind(const char *name);
const char *p4_semantic_name(int kind);
/* Subscriber count of a kind changed by delta; the observer is registered while
 * any count is nonzero. 0, or -1 when the observer cannot be registered. */
int p4_semantic_interest(int kind, int delta);
/* Translation of one committed packet into queued records (tests feed these
 * directly; the observer forwards the native packets). */
void p4_semantic_admission(const P4BattleAdmission *event);
void p4_semantic_command(const P4BattleCommand *event);
void p4_semantic_hit(const P4BattleHit *event);
/* The notifications (battle_notify.h; guest addresses), forwarded while some kind wants them. */
void p4_semantic_one_more(uint32_t action);
void p4_semantic_reward(uint32_t awards, uint32_t reward, int32_t money, uint32_t hero_exp_before);
void p4_semantic_level_up(uint32_t member, uint32_t persona, uint32_t delta);
/* Copies queued records in commit order and clears them; *dropped counts
 * records lost to a full queue since the last drain. */
size_t p4_semantic_drain(P4SemanticEvent *out, size_t capacity, uint32_t *dropped);
#endif
