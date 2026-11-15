/* Playable characters' combat progression for mods. Two services share this
 * header: party records (p4.party.record/set, docs/modding/lua.md "Party
 * records") and party actions (p4.party.persona/level, p4.give.skill/exp,
 * docs/modding/systems/actions.md). Game thread only. */
#ifndef P4PC_MOD_PARTY_H
#define P4PC_MOD_PARTY_H

#include <stddef.h>
#include <stdint.h>

/* Party records. Party ids 2..10 name the party slots' Persona records (the
 * character's level, EXP, five stats and eight skill slots); Yu (1) is not a
 * party record. Reads copy the record; a write is validated whole, queued and
 * applied at the frame safe point outside battle, and then keeps the
 * character's current HP/SP within the new maxima. */
typedef struct P4ModPartyRecord {
    uint16_t persona;   /* the record's Persona ID; a write never changes it */
    uint8_t level;      /* 1..99 */
    uint32_t exp;       /* within the level's EXP band of this Persona's curve */
    uint8_t stats[5];   /* St Ma En Ag Lu, each 1..99 */
    uint16_t skills[8]; /* registered skill IDs first, unique, then zeros */
} P4ModPartyRecord;

/* 0, or -1 when id is not 2..10 or the slot has no Persona. */
int p4_mod_party_read(int id, P4ModPartyRecord *out);
/* Validates the whole record now and queues it for the next frame safe point
 * outside battle (p4_mod_safe_point). 0, or -1 with a reason; a refusal queues
 * nothing and the record stays byte-identical. The owner's second write to an
 * id replaces its pending one and is ordered as asked for then. */
int p4_mod_party_queue(size_t owner, int id, const P4ModPartyRecord *record, char *reason, size_t capacity);
/* The safe point's step: applies each queued write, revalidated against the
 * record as it is then (a misfit is the owner's mod failure, never a partial
 * write). A write waits, other ids still applying, during a battle, while the
 * skill-choice panel shows that Persona (p4_mod_field_host_member), and while
 * a party action to that Persona asked for before it is queued. Nonzero while
 * a write waits. */
int p4_mod_party_apply_pending(void);
/* An owner's VM teardown drops its queued writes. */
void p4_mod_party_clear_owner(size_t owner);
/* Call order across p4.party.set and the queued party actions below: every
 * queued change to a Persona record takes the next ticket. write_ticket is the
 * pending write's for id (2..10), 0 when none, and copies the pending record
 * into record (may be NULL); set_actions registers the party actions'
 * p4_mod_party_first_action, the ticket a write must not pass. */
uint64_t p4_mod_party_ticket(void);
uint64_t p4_mod_party_write_ticket(uint32_t id, P4ModPartyRecord *record);
void p4_mod_party_set_actions(uint64_t (*first)(uint32_t id));

/* Party actions (capabilities party.persona@1, give.skill@1, give.exp@1) over
 * the game's own Persona records and level code (api-research §10):
 * datPersonaGetRecord 0x0010A900 (member 1 = the protagonist's current
 * Persona, 2..8 a party member's Persona; a party member's level is their
 * Persona's), datPersonaFindSkillSlot/AddSkill/ReplaceSkill 0x0010CE10/
 * 0x0010CC20/0x0010CD70, the result screen's silent EXP path
 * (datPersonaPreviewLevelUp 0x0010BE60, datPersonaAddExp 0x0010C980,
 * datPersonaCanLevelUp 0x0010C6F0, datPersonaApplyLevelUp 0x0010C5A0) and
 * the protagonist's own level (datSetHeroExp 0x00105990, datLevelFromExp
 * 0x001059E0, datCalcSetLevel 0x00231EF0). Like inventory (mod_inventory.h)
 * a change applies at once with a game loaded outside battle and is
 * validated, queued and applied at the first safe point outside a battle or
 * a scene change otherwise. Refusals change nothing and give a static reason.
 *
 * give.skill.choice@1 adds the game's own "forget which skill?" choice for a
 * Persona whose 8 slots are full: the battle result's Persona panel and
 * skill-replace task (shdPersonaSkillReplaceCreate 0x0011FBC0), hosted in the
 * field by include/p4pc/mod_field_host.h (api-research §10.1). */
#define P4_MOD_PARTY_MEMBERS 8u     /* 1 protagonist .. 8 Teddie (p4_data_character_name) */
#define P4_MOD_EXP_MAX 9999999u
#define P4_MOD_PERSONA_SKILLS 8u

typedef struct P4ModPersona {
    uint16_t id;                    /* Persona ID (catalog kind persona) */
    uint8_t level;
    uint32_t exp;
    uint16_t skills[P4_MOD_PERSONA_SKILLS]; /* 0 when empty */
} P4ModPersona;

/* The member's Persona as it is now: 0, or -1 with reason (no game, no such
 * member, no Persona yet). */
int p4_mod_party_persona(uint32_t member, P4ModPersona *out, const char **reason);
/* The protagonist's own level and EXP. */
int p4_mod_party_hero(uint8_t *level, uint32_t *exp, const char **reason);
/* The protagonist's stored level (capability party.hero_level@1): the level
 * byte (+0x06) of his unit record (datUnitGetRecord 0x00105510 with id 1),
 * which the game's own level read returns for him (datCalcGetLevelById
 * 0x00104C70, datCalcGetLevel 0x00231E20). EXP set without a level-up (EXP
 * awaiting the result screen) does not change it, unlike p4_mod_party_hero's
 * level. 0, or -1 with reason: no game loaded, or no level yet (0, which the
 * game's read treats as fatal). */
int p4_mod_party_hero_level(uint8_t *level, const char **reason);
/* Teaches skill to the member's Persona: the first empty slot, or replace's
 * slot when replace is nonzero. 0 applied, 1 queued, -1 refused (unknown
 * member/skill, no Persona, already known, replace not known, all 8 slots
 * full without replace, not joined yet). name labels log lines (the mod id; it
 * must outlive a queued change). Changes to one Persona apply in call order:
 * while the skill-choice panel shows that Persona ("queued until the skill
 * choice open for that Persona closes"), while a p4.party.set to it is pending
 * ("queued behind a pending p4.party.set for that Persona", applied at the
 * services step after the write) or behind an earlier queued change to it
 * ("queued behind an earlier change to that Persona") the change is queued and
 * applied, checked again, in order (one that no longer holds then is logged and
 * skipped). Behind a pending write or queued changes the refusals are checked
 * against the skills they leave (the write's list, then the queued skill
 * changes after it), not the record as it is; level-ups of queued EXP, battle
 * results and a panel's answer are not foreseen. */
int p4_mod_skill_give(uint32_t member, uint32_t skill, uint32_t replace, const char *name, const char **reason);
/* Adds EXP with the game's level-ups: to the member's Persona, or with hero
 * set (member 1 only) to the protagonist's own level. Skills a level-up would
 * teach that do not fit (8 already) are not learned; each is logged. Results
 * and order as p4_mod_skill_give (the protagonist's own level only after his
 * earlier queued EXP). */
int p4_mod_exp_give(uint32_t member, int hero, uint32_t amount, const char *name, const char **reason);
/* The ticket (p4_mod_party_ticket) of the first queued change to member's
 * Persona record (not the protagonist's own level), 0 when none: a
 * p4.party.set asked for after it waits for it (p4_mod_party_set_actions). */
uint64_t p4_mod_party_first_action(uint32_t member);
/* Applies changes queued during a battle, between scenes, behind an open
 * panel or behind a pending p4.party.set and settles skill choices (below);
 * nonzero while some remain, and for one more boundary after a done callback
 * ran (it may have queued work for a service whose pump already ran). */
int p4_mod_party_pump(void);

#define P4_MOD_SKILL_CHOICES 16u
typedef struct P4ModSkillChoice {
    uint32_t member, skill; /* as requested */
    int learned;            /* the Persona knows skill now */
    uint16_t forgot;        /* the skill it forgot to make room, 0 when none */
    const char *reason;     /* static, when not learned: "declined" (the player kept every skill) or why not */
} P4ModSkillChoice;
typedef void (*P4ModSkillChoiceDone)(const P4ModSkillChoice *outcome, void *user);
/* p4_mod_skill_give without replace, except that a Persona whose 8 slots are
 * full asks the player which skill to forget, or whether to keep them all,
 * through the game's own panel, the next time the field host can hold a
 * ready field or dungeon (no battle, event, script, menu or scene change).
 * Like an event it ends a camera lease first (and waits for its exclusive
 * input to drain); a scene request accepted before the panel opens goes
 * first and the choice waits for the field after it.
 * 0: learned now (an empty slot, in the field or an event, nothing queued
 * before); 1: queued with reason; -1: refused (unknown skill or member, no
 * game, not joined, no Persona, already known, P4_MOD_SKILL_CHOICES waiting).
 * A queued choice is checked again when it comes up: it is learned silently
 * if a slot freed meanwhile and ends unlearned if the skill is known by then.
 * It comes up (and a choice is learned now) only once no p4.party.set and no
 * queued party action to that Persona is pending: they apply first.
 * done (may be NULL) runs exactly once with the outcome, in request order, at
 * a later services frame boundary (p4_mod_party_pump), unless
 * p4_mod_skill_choice_release(owner) dropped the choice first. With no game
 * loaded any more a choice ends unlearned. name labels log lines and must
 * outlive the choice. */
int p4_mod_skill_choose(uint32_t member, uint32_t skill, size_t owner, const char *name,
                        P4ModSkillChoiceDone done, void *user, const char **reason);
/* Drops owner's choices and outcomes without calling done. One the player
 * already sees stays on screen until answered; its outcome is discarded. */
void p4_mod_skill_choice_release(size_t owner);
/* 1 while the next choice holds the ready field for its panel (offered to
 * the field host, or waiting for an ended camera lease's input to drain):
 * p4_mod_camera_acquire refuses then. */
int p4_mod_skill_choice_claimed(void);
#endif
