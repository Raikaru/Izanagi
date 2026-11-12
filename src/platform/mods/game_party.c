/* Party actions: include/p4pc/mod_party.h. */
#include "p4pc/mod_party.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/gmem.h"
#include "p4pc/guest.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_field_host.h"
#include "p4pc/mod_inventory.h"
#include "p4pc/mod_services.h"
#include <stdio.h>
#include <string.h>

extern void *datPersonaGetRecord(uint16_t partyId);
extern uint16_t datPersonaCountValidMembers(void);
extern int32_t datPersonaFindSkillSlot(void *persona, uint16_t skillId);
extern int32_t datPersonaAddSkill(void *persona, uint16_t skillId);
extern int32_t datPersonaReplaceSkill(void *persona, int16_t oldSkill, uint16_t newSkill);
extern void datPersonaPreviewLevelUp(void *persona, void *delta, int32_t addedExp);
extern void datPersonaAddExp(void *persona, int32_t addedExp);
extern int32_t datPersonaCanLevelUp(void *persona);
extern void datPersonaApplyLevelUp(void *persona, void *delta);
extern uint32_t datPersonaGetExpById(int16_t unitId);
extern void datSetHeroExp(int16_t unitId, uint32_t experience);
extern uint8_t datLevelFromExp(int32_t exp);
extern void *datUnitGetRecord(int16_t unitId);
extern void datCalcSetLevel(void *unit, uint8_t level);
extern int32_t datGetFlag(int32_t flagId);
extern void datSetFlag(int32_t flagId, int32_t enabled);

/* DatPersonaRecord (0x30 bytes): id +2, level +4, exp +8, skills[8] +0x0C. */
#define RECORD_ID 0x02u
#define RECORD_LEVEL 0x04u
#define RECORD_EXP 0x08u
#define RECORD_SKILLS 0x0Cu
/* DatUnit (0x30 bytes): the stored level at +6. */
#define UNIT_LEVEL 0x06u
/* The level-up delta (datPersonaPreviewLevelUp): added[32] u16 at +2. */
#define DELTA_SIZE 0x88u
#define DELTA_ADDED 0x02u
#define DELTA_ADDED_COUNT 32u
#define LEVEL_MAX 99u

/* Party-joined flags by member (research §4.3): Yosuke 0x30 .. Naoto 0x36. */
static const int16_t joined_flag[P4_MOD_PARTY_MEMBERS + 1] = {0, 0, 0x30, 0x31, 0x32, 0x34, 0x33, 0x36, 0x35};
/* Navigator skills the result screen marks as learned (btlResultFriendPsLvUp). */
static const struct { uint16_t skill; int16_t flag; } navigator_flags[] = {{0x112, 0x1012}, {0x113, 0x1013}};

/* The member's Persona record (host pointer), or NULL with reason. */
static uint8_t *record_of(uint32_t member, const char **reason)
{
    if (member < 1 || member > P4_MOD_PARTY_MEMBERS) { *reason = "no such party member (1-8)"; return NULL; }
    if (p4_mod_inventory_ready(reason) < 0) return NULL;
    if (member == 1 ? !datPersonaCountValidMembers() : !datGetFlag(joined_flag[member])) {
        *reason = member == 1 ? "the protagonist has no Persona yet" : "that member has not joined the party yet";
        return NULL;
    }
    uint8_t *record = datPersonaGetRecord((uint16_t)member);
    if (!record || !*(uint16_t *)(record + RECORD_ID)) { *reason = "that member has no Persona yet"; return NULL; }
    return record;
}

int p4_mod_party_persona(uint32_t member, P4ModPersona *out, const char **reason)
{
    const uint8_t *record = record_of(member, reason);
    if (!record) return -1;
    out->id = *(const uint16_t *)(record + RECORD_ID);
    out->level = record[RECORD_LEVEL];
    memcpy(&out->exp, record + RECORD_EXP, sizeof out->exp);
    memcpy(out->skills, record + RECORD_SKILLS, sizeof out->skills);
    return 0;
}

int p4_mod_party_hero(uint8_t *level, uint32_t *exp, const char **reason)
{
    if (p4_mod_inventory_ready(reason) < 0) return -1;
    *exp = datPersonaGetExpById(1);
    *level = datLevelFromExp((int32_t)*exp);
    return 0;
}

int p4_mod_party_hero_level(uint8_t *level, const char **reason)
{
    if (p4_mod_inventory_ready(reason) < 0) return -1;
    const uint8_t *unit = datUnitGetRecord(1);
    if (!unit || !unit[UNIT_LEVEL]) { *reason = "the protagonist has no level yet"; return -1; }
    *level = unit[UNIT_LEVEL];
    return 0;
}

#define PENDING 32u
static struct Pending {
    uint8_t exp, hero;
    uint32_t member, skill, replace, amount;
    const char *name;
    uint64_t ticket; /* call order with p4.party.set (p4_mod_party_ticket) */
} pending[PENDING];
static size_t pending_count;

static int defer(const struct Pending *change, const char **reason)
{
    if (pending_count == PENDING) { *reason = "too many party changes queued"; return -1; }
    pending[pending_count] = *change;
    pending[pending_count++].ticket = p4_mod_party_ticket();
    p4_mod_safe_point |= P4_MOD_SAFE_SERVICES;
    return 1;
}

/* The first queued change to the member's Persona record, or with hero set to the protagonist's
 * own level; NULL when none. */
static const struct Pending *first_queued(uint32_t member, int hero)
{
    for (size_t i = 0; i < pending_count; ++i)
        if (pending[i].member == member && pending[i].hero == !!hero) return &pending[i];
    return NULL;
}

uint64_t p4_mod_party_first_action(uint32_t member)
{
    const struct Pending *p = first_queued(member, 0);
    return p ? p->ticket : 0;
}

static uint32_t slots_used(const uint8_t *record)
{
    uint32_t used = 0;
    for (uint32_t i = 0; i < P4_MOD_PERSONA_SKILLS; ++i) used += *(const uint16_t *)(record + RECORD_SKILLS + 2 * i) != 0;
    return used;
}

static const char known_reason[] = "the Persona already knows that skill";
static const char unknown_replace_reason[] = "the Persona does not know the skill to replace";
static const char full_reason[] = "all 8 skill slots are full; pass replace with a skill to forget";

/* The skill checks that hold until the change applies. */
static int skill_refusal(uint8_t *record, uint32_t skill, uint32_t replace, const char **reason)
{
    if (datPersonaFindSkillSlot(record, (uint16_t)skill) != -1) { *reason = known_reason; return -1; }
    if (replace && datPersonaFindSkillSlot(record, (uint16_t)replace) == -1) { *reason = unknown_replace_reason; return -1; }
    if (!replace && slots_used(record) == P4_MOD_PERSONA_SKILLS) { *reason = full_reason; return -1; }
    return 0;
}

static int apply_skill(uint32_t member, uint32_t skill, uint32_t replace, const char **reason)
{
    uint8_t *record = record_of(member, reason);
    if (!record || skill_refusal(record, skill, replace, reason)) return -1;
    if (replace) datPersonaReplaceSkill(record, (int16_t)replace, (uint16_t)skill);
    else datPersonaAddSkill(record, (uint16_t)skill);
    return 0;
}

/* The same checks and change over a skill list (slot order, 0 = empty) instead of the record. */
static int list_slot(const uint16_t *skills, uint32_t skill)
{
    for (int i = 0; i < (int)P4_MOD_PERSONA_SKILLS; ++i)
        if (skills[i] == skill) return i;
    return -1;
}
static int list_refusal(const uint16_t *skills, uint32_t skill, uint32_t replace, const char **reason)
{
    if (list_slot(skills, skill) != -1) { *reason = known_reason; return -1; }
    if (replace && list_slot(skills, replace) == -1) { *reason = unknown_replace_reason; return -1; }
    if (!replace && list_slot(skills, 0) == -1) { *reason = full_reason; return -1; }
    return 0;
}

/* The skills the member's Persona will have when a change asked for now applies, as far as call
 * time can tell: a pending p4.party.set's (it writes the whole list, whatever was queued before
 * it) or else the record's, then the queued skill changes asked for after that write, each as its
 * apply treats it (one refused then changes nothing). 0 when nothing is pending: the record itself
 * is the answer, checked by the game's own functions. Not foreseen: skills a queued give.exp
 * level-up teaches, a battle's own results and an open panel's answer; a change they make void is
 * checked again when it applies, logged and skipped, as any queued change that no longer holds. */
static int foreseen_skills(uint32_t member, const uint8_t *record, uint16_t *skills)
{
    P4ModPartyRecord write;
    const uint64_t after = p4_mod_party_write_ticket(member, &write);
    int foreseen = after != 0;
    memcpy(skills, after ? (const void *)write.skills : (const void *)(record + RECORD_SKILLS),
           P4_MOD_PERSONA_SKILLS * sizeof skills[0]);
    for (size_t i = 0; i < pending_count; ++i) { /* in call order */
        const struct Pending *p = &pending[i];
        if (p->exp || p->member != member || p->ticket < after) continue;
        foreseen = 1;
        const char *refused;
        if (list_refusal(skills, p->skill, p->replace, &refused)) continue;
        skills[list_slot(skills, p->replace)] = (uint16_t)p->skill; /* replace's slot, or the first empty one */
    }
    return foreseen;
}

/* While the skill-choice panel is open its skill list is a copy of that Persona's, and the
 * native replace task writes the record from it: a change made meanwhile could be undone, or
 * the offered skill end up twice. Changes to that Persona (not the protagonist's own level)
 * wait in the queue until the session ends, then apply in order and are checked again.
 * Changes to one record also apply in call order with p4.party.set (party_native.c): a write
 * pending at the frame safe point applies before the services step, so a change asked for
 * after it waits for it, and one asked for after a queued change waits behind that. */
static const char held_reason[] = "queued until the skill choice open for that Persona closes";
static const char write_reason[] = "queued behind a pending p4.party.set for that Persona";
static const char behind_reason[] = "queued behind an earlier change to that Persona";
static int held(uint32_t member, int hero)
{
    return !hero && member == p4_mod_field_host_member();
}
/* Why a change asked for now, with the game ready, must queue; NULL when it applies at once. */
static const char *waits(uint32_t member, int hero)
{
    if (held(member, hero)) return held_reason;
    if (!hero && p4_mod_party_write_ticket(member, NULL)) return write_reason;
    if (first_queued(member, hero)) return behind_reason;
    return NULL;
}
/* A queued change still waits (the game ready): the panel shows its Persona, or a write to that
 * record asked for before it is pending. */
static int queued_waits(const struct Pending *p)
{
    if (p->hero) return 0;
    const uint64_t write = p4_mod_party_write_ticket(p->member, NULL);
    return held(p->member, 0) || (write && write < p->ticket);
}

int p4_mod_skill_give(uint32_t member, uint32_t skill, uint32_t replace, const char *name, const char **reason)
{
    if (!skill || skill > UINT16_MAX || !p4_game_skill_active(skill) ||
        (replace && (replace > UINT16_MAX || !p4_game_skill_active(replace)))) {
        *reason = "no such skill";
        return -1;
    }
    uint8_t *record = record_of(member, reason);
    if (!record) return -1;
    /* Checked against the Persona as the changes already asked for leave it (a pending write or
     * queued change means this one queues behind them, see waits). */
    uint16_t foreseen[P4_MOD_PERSONA_SKILLS];
    if (foreseen_skills(member, record, foreseen) ? list_refusal(foreseen, skill, replace, reason)
                                                  : skill_refusal(record, skill, replace, reason))
        return -1;
    const int ready = p4_mod_inventory_ready(reason);
    const char *wait = ready ? NULL : waits(member, 0);
    if (wait) *reason = wait;
    if (ready > 0 || wait) return defer(&(struct Pending){0, 0, member, skill, replace, 0, name, 0}, reason);
    return apply_skill(member, skill, replace, reason);
}

static uint32_t delta_buffer(void)
{
    static uint32_t delta;
    if (!delta) delta = p4_gport_alloc(DELTA_SIZE, 16);
    return delta;
}

static int apply_exp(uint32_t member, int hero, uint32_t amount, const char *name, const char **reason)
{
    if (hero) {
        const uint32_t before = datPersonaGetExpById(1);
        if (datLevelFromExp((int32_t)before) >= LEVEL_MAX) return 0; /* retail skips the hero at 99 */
        const uint32_t after = before + amount;
        datSetHeroExp(1, after);
        datCalcSetLevel(datUnitGetRecord(1), datLevelFromExp((int32_t)after));
        return 0;
    }
    uint8_t *record = record_of(member, reason);
    if (!record) return -1;
    const uint32_t delta = delta_buffer();
    if (!delta) { *reason = "out of port memory"; return -1; }
    /* The result screen's order: preview the level-ups before adding (btlXpAddPartyExp). */
    memset(P4_G2H(delta), 0, DELTA_SIZE);
    datPersonaPreviewLevelUp(record, P4_G2H(delta), (int32_t)amount);
    datPersonaAddExp(record, (int32_t)amount);
    if (!datPersonaCanLevelUp(record)) return 0;
    datPersonaApplyLevelUp(record, P4_G2H(delta));
    for (uint32_t i = 0; i < DELTA_ADDED_COUNT; ++i) {
        const uint16_t skill = P4_G16(delta + DELTA_ADDED + 2 * i);
        if (!skill) continue;
        if (datPersonaFindSkillSlot(record, skill) == -1) {
            fprintf(stderr, "p4pc: mod %s: p4.give.exp: member %u's Persona could not learn skill %u (8 skills); "
                    "p4.give.skill with replace or choice can teach it\n", name, (unsigned)member, (unsigned)skill);
            continue;
        }
        for (size_t n = 0; n < sizeof navigator_flags / sizeof navigator_flags[0]; ++n)
            if (navigator_flags[n].skill == skill) datSetFlag(navigator_flags[n].flag, 1);
    }
    return 0;
}

int p4_mod_exp_give(uint32_t member, int hero, uint32_t amount, const char *name, const char **reason)
{
    if (!amount || amount > P4_MOD_EXP_MAX) { *reason = "EXP must be 1-9,999,999"; return -1; }
    if (hero && member != 1) { *reason = "only the protagonist has a level of his own"; return -1; }
    if (hero ? p4_mod_inventory_ready(reason) < 0 : !record_of(member, reason)) return -1;
    const int ready = p4_mod_inventory_ready(reason);
    const char *wait = ready ? NULL : waits(member, hero);
    if (wait) *reason = wait;
    if (ready > 0 || wait) return defer(&(struct Pending){1, (uint8_t)!!hero, member, 0, 0, amount, name, 0}, reason);
    return apply_exp(member, hero, amount, name, reason);
}

/* give.skill.choice@1. Choices finish in request order; the first unfinished one is the only one
 * the field host may show, and outcomes are handed to done strictly in that order. CLAIMED: the
 * field is ready and the choice is next, but an ended camera lease's input still drains. */
enum { CHOICE_WAITING, CHOICE_CLAIMED, CHOICE_OFFERED, CHOICE_SHOWN, CHOICE_FINISHED };
static struct Choice {
    P4ModSkillChoice outcome; /* member and skill from the request, the rest once finished */
    size_t owner;
    const char *name;         /* NULL once the owner released a choice the player still sees */
    P4ModSkillChoiceDone done;
    void *user;
    int state;
} choices[P4_MOD_SKILL_CHOICES];
static size_t choice_count;

static struct Choice *first_open_choice(void)
{
    for (size_t i = 0; i < choice_count; ++i)
        if (choices[i].state != CHOICE_FINISHED) return &choices[i];
    return NULL;
}

int p4_mod_skill_choice_claimed(void)
{
    const struct Choice *c = first_open_choice();
    return c && (c->state == CHOICE_CLAIMED || c->state == CHOICE_OFFERED);
}

static void choice_finish(struct Choice *c, int learned, uint16_t forgot, const char *reason)
{
    c->outcome.learned = learned;
    c->outcome.forgot = forgot;
    c->outcome.reason = learned ? NULL : reason;
    c->state = CHOICE_FINISHED;
    if (!learned && strcmp(reason, "declined"))
        fprintf(stderr, "p4pc: mod %s: p4.give.skill choice: member %u's Persona did not learn skill %u: %s\n",
                c->name ? c->name : "(released)", (unsigned)c->outcome.member, (unsigned)c->outcome.skill, reason);
}

int p4_mod_skill_choose(uint32_t member, uint32_t skill, size_t owner, const char *name, P4ModSkillChoiceDone done,
                        void *user, const char **reason)
{
    if (!skill || skill > UINT16_MAX || !p4_game_skill_active(skill)) { *reason = "no such skill"; return -1; }
    uint8_t *record = record_of(member, reason);
    if (!record) return -1;
    uint16_t foreseen[P4_MOD_PERSONA_SKILLS];
    if (foreseen_skills(member, record, foreseen) ? list_slot(foreseen, skill) != -1
                                                  : datPersonaFindSkillSlot(record, (uint16_t)skill) != -1) {
        *reason = known_reason;
        return -1;
    }
    if (choice_count == P4_MOD_SKILL_CHOICES) { *reason = "too many skill choices are waiting"; return -1; }
    const int behind = first_open_choice() != NULL;
    struct Choice *c = &choices[choice_count++];
    *c = (struct Choice){.outcome = {.member = member, .skill = skill}, .owner = owner, .name = name, .done = done,
                         .user = user, .state = CHOICE_WAITING};
    p4_mod_safe_point |= P4_MOD_SAFE_SERVICES; /* done runs at the services frame boundary */
    const int ready = p4_mod_inventory_ready(reason);
    if (ready > 0) return 1;
    if (behind) { *reason = "queued behind another skill choice"; return 1; }
    const char *wait = waits(member, 0);
    if (wait) { *reason = wait; return 1; }
    if (slots_used(record) < P4_MOD_PERSONA_SKILLS) {
        datPersonaAddSkill(record, (uint16_t)skill);
        choice_finish(c, 1, 0, NULL);
        return 0;
    }
    *reason = "queued: the player picks a skill to forget once the field is free";
    return 1;
}

void p4_mod_skill_choice_release(size_t owner)
{
    size_t kept = 0;
    for (size_t i = 0; i < choice_count; ++i) {
        struct Choice c = choices[i];
        if (c.owner == owner) {
            if (c.state == CHOICE_OFFERED) {
                int learned;
                uint16_t forgot;
                const char *reason;
                if (p4_mod_field_host_result(0, &learned, &forgot, &reason)) continue; /* it ended: unheard */
                if (!p4_mod_field_host_busy()) {
                    p4_mod_field_host_withdraw();
                    continue;
                }
            } else if (c.state != CHOICE_SHOWN) {
                continue;
            }
            /* On screen: the player finishes it; nobody hears the outcome. */
            c.state = CHOICE_SHOWN;
            c.name = NULL;
            c.done = NULL;
            c.user = NULL;
        }
        choices[kept++] = c;
    }
    choice_count = kept;
}

/* Settles the first unfinished choice and any that follow without the panel, until one waits:
 * for the battle or scene change to end (ready > 0), for a pending p4.party.set or queued change
 * to its Persona to apply, for a free field (offered to the host), or on screen. ready < 0 (no
 * game) ends every choice not on screen, and abandons one that is. */
static void choices_step(int ready, const char *why)
{
    for (struct Choice *c; (c = first_open_choice()) != NULL;) {
        if (c->state != CHOICE_WAITING) {
            int learned;
            uint16_t forgot;
            const char *reason;
            if (p4_mod_field_host_result(ready < 0, &learned, &forgot, &reason)) {
                choice_finish(c, learned, forgot, reason);
                continue;
            }
            if (p4_mod_field_host_busy()) {
                c->state = CHOICE_SHOWN;
                return;
            }
            c->state = CHOICE_WAITING; /* armed but not taken yet: checked again below */
        }
        if (ready != 0) {
            p4_mod_field_host_withdraw();
            if (ready > 0) return;
            choice_finish(c, 0, 0, why);
            continue;
        }
        if (waits(c->outcome.member, 0)) { /* those apply first; the record is checked after them */
            p4_mod_field_host_withdraw();
            return;
        }
        const char *reason;
        uint8_t *record = record_of(c->outcome.member, &reason);
        const uint16_t skill = (uint16_t)c->outcome.skill;
        if (!record || datPersonaFindSkillSlot(record, skill) != -1 || slots_used(record) < P4_MOD_PERSONA_SKILLS) {
            p4_mod_field_host_withdraw();
            if (!record) choice_finish(c, 0, 0, reason);
            else if (datPersonaFindSkillSlot(record, skill) != -1) choice_finish(c, 0, 0, known_reason);
            else choice_finish(c, datPersonaAddSkill(record, skill) != 0, 0, "the game refused the skill");
            continue;
        }
        P4ModSceneState scene;
        p4_mod_scene_state(&scene);
        if (!scene.ready || (scene.context != P4_MOD_SCENE_FIELD && scene.context != P4_MOD_SCENE_DUNGEON)) {
            p4_mod_field_host_withdraw();
            return;
        }
        /* The panel takes the field and the pad as an event does: a camera lease ends first and
         * its exclusive input drains (every button up) before the offer; while the choice claims
         * the field no lease starts (p4_mod_camera_acquire). */
        if (p4_mod_camera_owner()) p4_mod_camera_revoke("the game's skill choice took over the scene");
        c->state = !p4_mod_input_fence && !p4_mod_field_host_offer(c->outcome.member, c->outcome.skill)
            ? CHOICE_OFFERED : CHOICE_CLAIMED;
        return;
    }
}

/* 1 when a done callback ran: it may have queued work whose service already pumped this boundary. */
static int choices_deliver(void)
{
    int ran = 0;
    while (choice_count && choices[0].state == CHOICE_FINISHED) {
        const struct Choice c = choices[0];
        memmove(choices, choices + 1, --choice_count * sizeof choices[0]); /* done may queue another */
        if (c.done) {
            c.done(&c.outcome, c.user);
            ran = 1;
        }
    }
    return ran;
}

int p4_mod_party_pump(void)
{
    const char *why = NULL;
    const int ready = p4_mod_inventory_ready(&why);
    if (pending_count && ready <= 0) {
        size_t kept = 0;
        for (size_t i = 0; i < pending_count; ++i) {
            const struct Pending *p = &pending[i];
            if (!ready && queued_waits(p)) { /* the open panel, or an earlier p4.party.set, holds that Persona */
                pending[kept++] = *p;
                continue;
            }
            const char *reason = why;
            const int result = ready < 0 ? -1
                : p->exp ? apply_exp(p->member, p->hero, p->amount, p->name, &reason)
                         : apply_skill(p->member, p->skill, p->replace, &reason);
            if (result)
                fprintf(stderr, "p4pc: mod %s: queued %s for member %u was not applied: %s\n", p->name,
                        p->exp ? "p4.give.exp" : "p4.give.skill", (unsigned)p->member, reason);
        }
        pending_count = kept;
    }
    choices_step(ready, why);
    /* After a callback, one more boundary: the services pump would otherwise clear the safe point
     * on this result alone and strand what the callback queued elsewhere (items during a battle). */
    const int ran = choices_deliver();
    return pending_count != 0 || choice_count != 0 || ran;
}
