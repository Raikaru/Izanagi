/* Party actions (src/platform/mods/game_party.c) over stub Persona records and
 * level code: member validity and joining, skill slots and replacement, EXP
 * with level-ups (learned, dropped and navigator-flagged skills), the
 * protagonist's own level, queueing during a battle, and the skill choice's
 * queue over a stub field host (an answer writes the record as the native
 * replace task does). With party_native.c (p4.party.set): a write and the
 * actions to one Persona apply in call order across the frame boundary's
 * party and services steps, and a write waits while the panel shows that
 * Persona. The game's own functions are witnessed in game
 * (tests/mod_actions_game_test.py). */
#include "guest_test.h"
#include "p4pc/mod_party.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_field_host.h"
#include "p4pc/mod_failure.h"
#include "p4pc/mod_services.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { FIELD, BATTLE, TITLE };
static int scene = TITLE;
static uint8_t records[9][0x30];
static uint16_t hero_personas;
static uint32_t hero_exp;
static uint8_t hero_level = 1, hero_unit[0x30];
static uint8_t flags[0x1600];
uint32_t p4_mod_safe_point;

int p4_mod_inventory_ready(const char **reason)
{
    if (scene == FIELD) return 0;
    *reason = scene == BATTLE ? "queued until the battle ends" : "no game is loaded";
    return scene == BATTLE ? 1 : -1;
}
int p4_game_skill_active(uint32_t id) { return id && id < 800; }
void *datPersonaGetRecord(uint16_t partyId) { assert(partyId >= 1 && partyId <= 8); return records[partyId]; }
uint16_t datPersonaCountValidMembers(void) { return hero_personas; }
static uint16_t *skills(void *persona) { return (uint16_t *)((uint8_t *)persona + 0x0C); }
int32_t datPersonaFindSkillSlot(void *persona, uint16_t skill)
{
    assert(skill);
    for (int i = 0; i < 8; ++i) if (skills(persona)[i] == skill) return i;
    return -1;
}
int32_t datPersonaAddSkill(void *persona, uint16_t skill)
{
    for (int i = 0; i < 8; ++i) if (!skills(persona)[i]) { skills(persona)[i] = skill; return 1; }
    return 0;
}
int32_t datPersonaReplaceSkill(void *persona, int16_t old, uint16_t skill)
{
    const int32_t slot = datPersonaFindSkillSlot(persona, (uint16_t)old);
    if (slot != -1) skills(persona)[slot] = skill;
    return slot;
}
/* Every 100 EXP is a level; reaching level 5 teaches skill 50, level 6 the navigator skill 0x112. */
static int32_t *exp_of(void *persona) { return (int32_t *)((uint8_t *)persona + 8); }
void datPersonaPreviewLevelUp(void *persona, void *delta, int32_t added)
{
    uint8_t *d = delta;
    const int level = ((uint8_t *)persona)[4], target = (*exp_of(persona) + added) / 100 + 1;
    d[0] = (uint8_t)(target > level ? target - level : 0);
    int n = 0;
    for (int l = level + 1; l <= target; ++l) {
        if (l == 5) { d[2 + 2 * n] = 50; ++n; }
        if (l == 6) { d[2 + 2 * n] = 0x12; d[3 + 2 * n] = 0x01; ++n; }
    }
}
void datPersonaAddExp(void *persona, int32_t added) { assert(added >= 0); *exp_of(persona) += added; }
int32_t datPersonaCanLevelUp(void *persona) { return *exp_of(persona) / 100 + 1 > ((uint8_t *)persona)[4]; }
void datPersonaApplyLevelUp(void *persona, void *delta)
{
    const uint8_t *d = delta;
    ((uint8_t *)persona)[4] += d[0];
    for (int i = 0; i < 32; ++i) {
        const uint16_t skill = (uint16_t)(d[2 + 2 * i] | d[3 + 2 * i] << 8);
        if (skill) datPersonaAddSkill(persona, skill);
    }
}
uint32_t datPersonaGetExpById(int16_t unit) { assert(unit == 1); return hero_exp; }
void datSetHeroExp(int16_t unit, uint32_t exp) { assert(unit == 1); hero_exp = exp; }
uint8_t datLevelFromExp(int32_t exp) { return (uint8_t)(exp / 100 + 1 > 99 ? 99 : exp / 100 + 1); }
void *datUnitGetRecord(int16_t unit) { assert(unit == 1); return hero_unit; }
void datCalcSetLevel(void *unit, uint8_t level) { assert(unit == hero_unit); hero_level = level; hero_unit[6] = level; }
int32_t datGetFlag(int32_t flag) { return flags[flag]; }
void datSetFlag(int32_t flag, int32_t on) { flags[flag] = (uint8_t)on; }

static void persona(int member, uint16_t id, uint8_t level, int32_t exp, const uint16_t *known, int count)
{
    memset(records[member], 0, sizeof records[member]);
    *(uint16_t *)(records[member] + 2) = id;
    records[member][4] = level;
    *exp_of(records[member]) = exp;
    for (int i = 0; i < count; ++i) skills(records[member])[i] = known[i];
}

/* The scene services and the field host (game_services.c, field_host.c) as game_party.c sees them. */
static int scene_ready = 1;
int p4_mod_input_fence;
void p4_mod_scene_state(P4ModSceneState *out)
{
    memset(out, 0, sizeof *out);
    out->context = scene == FIELD ? P4_MOD_SCENE_FIELD : scene == BATTLE ? P4_MOD_SCENE_BATTLE : P4_MOD_SCENE_TITLE;
    out->ready = scene == FIELD && scene_ready;
}
/* A camera lease: its name while active; revoking it drains an exclusive input (fence 1 -> 2). */
static const char *lease;
static int revoked;
const char *p4_mod_camera_owner(void) { return lease; }
void p4_mod_camera_revoke(const char *why)
{
    assert(why && *why);
    if (!lease) return;
    lease = NULL;
    ++revoked;
    if (p4_mod_input_fence == 1) p4_mod_input_fence = 2;
}
enum { HOST_IDLE, HOST_ARMED, HOST_BUSY, HOST_ENDED };
static int host_state, host_learned;
static uint32_t host_member, host_skill;
static uint16_t host_forgot, host_panel[8];
static const char *host_reason;
int p4_mod_field_host_offer(uint32_t member, uint32_t skill)
{
    if (host_state == HOST_BUSY || host_state == HOST_ENDED) return -1;
    host_state = HOST_ARMED;
    host_member = member;
    host_skill = skill;
    return 0;
}
void p4_mod_field_host_withdraw(void) { if (host_state == HOST_ARMED) host_state = HOST_IDLE; }
int p4_mod_field_host_busy(void) { return host_state == HOST_BUSY; }
uint32_t p4_mod_field_host_member(void) { return host_state == HOST_BUSY ? host_member : 0; }
int p4_mod_field_host_result(int abandon, int *learned, uint16_t *forgot, const char **reason)
{
    if (host_state == HOST_BUSY && abandon) {
        host_state = HOST_ENDED;
        host_learned = 0;
        host_reason = "no game is loaded any more";
    }
    if (host_state != HOST_ENDED) return 0;
    *learned = host_learned;
    *forgot = host_forgot;
    *reason = host_reason;
    host_state = HOST_IDLE;
    return 1;
}
/* The field controller takes the slot: the panel copies the Persona's skill list, as SetDisplayState does. */
static void host_take(void)
{
    assert(host_state == HOST_ARMED);
    memcpy(host_panel, skills(records[host_member]), sizeof host_panel);
    host_state = HOST_BUSY;
}
/* The player's answer: forget the panel's row slot (the replace task's datPersonaReplaceSkill from the
 * panel's own list), or -1 to keep every skill; the outcome is read back from the record. */
static void host_answer(int slot)
{
    assert(host_state == HOST_BUSY);
    void *record = records[host_member];
    const int replaced = slot >= 0 && datPersonaReplaceSkill(record, (int16_t)host_panel[slot], (uint16_t)host_skill) != -1;
    host_learned = datPersonaFindSkillSlot(record, (uint16_t)host_skill) != -1;
    host_forgot = replaced ? host_panel[slot] : 0;
    host_reason = host_learned ? NULL : "declined";
    host_state = HOST_ENDED;
}
static int copies(int member, uint16_t skill)
{
    int n = 0;
    for (int i = 0; i < 8; ++i) n += skills(records[member])[i] == skill;
    return n;
}
static P4ModSkillChoice outcomes[16];
static intptr_t outcome_user[16];
static int outcome_count;
static void on_done(const P4ModSkillChoice *outcome, void *user)
{
    outcome_user[outcome_count] = (intptr_t)user;
    outcomes[outcome_count++] = *outcome;
}

/* party_native.c's game calls: every 100 EXP is a level (as above), HP/SP already within the
 * maxima, the battle task alive in battle, every active skill registered. */
uint32_t datPersonaGetExpForLevel(void *persona, uint16_t level) { (void)persona; return (uint32_t)(level - 1) * 100u; }
uint32_t datCalcGetHpById(int16_t id) { (void)id; return 1; }
uint32_t datCalcGetSpById(int16_t id) { (void)id; return 1; }
uint16_t datCalcMaxHpById(int16_t id) { (void)id; return 999; }
uint16_t datCalcMaxSpById(int16_t id) { (void)id; return 999; }
void datSetUnitHp(int16_t id, int16_t hp) { (void)id; (void)hp; assert(!"HP stays within the maxima"); }
void datSetUnitSp(int16_t id, int16_t sp) { (void)id; (void)sp; assert(!"SP stays within the maxima"); }
void *kwlnTaskGetTaskByName(const void *name) { (void)name; return scene == BATTLE ? records : NULL; }
int p4_game_skill_registered(uint32_t id) { return p4_game_skill_active(id); }
_Noreturn void p4_mod_failure(size_t owner, const char *reason)
{
    fprintf(stderr, "unexpected mod failure of owner %zu: %s\n", owner, reason);
    abort();
}
static P4ModPartyRecord party_record(uint16_t id, uint8_t level, uint32_t exp, const uint16_t *known, int count)
{
    P4ModPartyRecord r = {.persona = id, .level = level, .exp = exp, .stats = {3, 3, 3, 3, 3}};
    for (int i = 0; i < count; ++i) r.skills[i] = known[i];
    return r;
}
/* A done callback that writes the record and teaches a skill to the same Persona (reentrancy). */
static int reentry_write, reentry_give;
static const char *reentry_reason;
static void on_done_write(const P4ModSkillChoice *outcome, void *user)
{
    on_done(outcome, user);
    const P4ModPartyRecord r = party_record(0x11, 2, 150, (const uint16_t[]){1, 68}, 2);
    char why[160];
    reentry_write = p4_mod_party_queue(7, 2, &r, why, sizeof why);
    reentry_give = p4_mod_skill_give(2, 69, 0, "t", &reentry_reason);
}

int main(void)
{
    if (p4t_guest_init()) return 77;
    p4_mod_party_set_actions(p4_mod_party_first_action); /* as p4_lua_party_initialize does */
    const char *reason = NULL;
    P4ModPersona read;
    uint8_t level;
    uint32_t exp;
    /* No game: nothing reads or changes. */
    assert(p4_mod_party_persona(1, &read, &reason) == -1 && !strcmp(reason, "no game is loaded"));
    assert(p4_mod_skill_give(2, 10, 0, "t", &reason) == -1);
    assert(p4_mod_exp_give(1, 1, 10, "t", &reason) == -1);
    scene = FIELD;
    /* Who: range, joining, a Persona at all. */
    assert(p4_mod_party_persona(9, &read, &reason) == -1 && strstr(reason, "no such party member"));
    assert(p4_mod_party_persona(1, &read, &reason) == -1 && !strcmp(reason, "the protagonist has no Persona yet"));
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2}, 2);
    assert(p4_mod_party_persona(2, &read, &reason) == -1 && !strcmp(reason, "that member has not joined the party yet"));
    flags[0x30] = 1;
    assert(!p4_mod_party_persona(2, &read, &reason) && read.id == 0x11 && read.level == 3 && read.exp == 200);
    assert(read.skills[0] == 1 && read.skills[1] == 2 && !read.skills[2]);
    flags[0x34] = 1; /* Rise (5) joined, with no Persona in her record */
    assert(p4_mod_party_persona(5, &read, &reason) == -1 && !strcmp(reason, "that member has no Persona yet"));

    /* Skills: a free slot, refusals, replacement. */
    assert(p4_mod_skill_give(2, 900, 0, "t", &reason) == -1 && !strcmp(reason, "no such skill"));
    assert(p4_mod_skill_give(2, 10, 900, "t", &reason) == -1 && !strcmp(reason, "no such skill"));
    assert(!p4_mod_skill_give(2, 10, 0, "t", &reason) && skills(records[2])[2] == 10);
    assert(p4_mod_skill_give(2, 10, 0, "t", &reason) == -1 && !strcmp(reason, "the Persona already knows that skill"));
    assert(p4_mod_skill_give(2, 11, 99, "t", &reason) == -1 && strstr(reason, "does not know the skill to replace"));
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    assert(p4_mod_skill_give(2, 11, 0, "t", &reason) == -1 && strstr(reason, "all 8 skill slots are full"));
    assert(!p4_mod_skill_give(2, 11, 4, "t", &reason) && skills(records[2])[3] == 11);

    /* EXP to a Persona: level-ups teach skills; one that does not fit is dropped; navigator skills set their flag. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2}, 2);
    assert(p4_mod_exp_give(2, 0, 0, "t", &reason) == -1 && strstr(reason, "EXP must be"));
    assert(p4_mod_exp_give(2, 1, 10, "t", &reason) == -1 && strstr(reason, "only the protagonist"));
    assert(!p4_mod_exp_give(2, 0, 50, "t", &reason) && records[2][4] == 3 && *exp_of(records[2]) == 250);
    assert(!p4_mod_exp_give(2, 0, 300, "t", &reason) && records[2][4] == 6 && *exp_of(records[2]) == 550);
    assert(datPersonaFindSkillSlot(records[2], 50) == 2 && datPersonaFindSkillSlot(records[2], 0x112) == 3);
    assert(flags[0x1012] && !flags[0x1013]);
    flags[0x1012] = 0;
    persona(2, 0x11, 4, 300, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    assert(!p4_mod_exp_give(2, 0, 200, "t", &reason) && records[2][4] == 6); /* both skills dropped, logged */
    assert(datPersonaFindSkillSlot(records[2], 50) == -1 && !flags[0x1012]);

    /* The protagonist: his own level by default, his current Persona with persona set. */
    assert(p4_mod_party_hero_level(&level, &reason) == -1 && !strcmp(reason, "the protagonist has no level yet"));
    assert(!p4_mod_exp_give(1, 1, 250, "t", &reason) && hero_exp == 250 && hero_level == 3);
    assert(!p4_mod_party_hero(&level, &exp, &reason) && level == 3 && exp == 250);
    assert(!p4_mod_party_hero_level(&level, &reason) && level == 3); /* his unit record's stored level */
    assert(p4_mod_exp_give(1, 0, 100, "t", &reason) == -1 && !strcmp(reason, "the protagonist has no Persona yet"));
    hero_personas = 1;
    persona(1, 0x01, 1, 0, (const uint16_t[]){1}, 1);
    assert(!p4_mod_exp_give(1, 0, 100, "t", &reason) && records[1][4] == 2 && hero_level == 3);
    hero_exp = 9800 * 100;
    assert(!p4_mod_exp_give(1, 1, 1000, "t", &reason) && hero_exp == 9800 * 100); /* retail skips him at 99 */
    /* EXP set without a level-up leaves the stored level where the game's own read finds it. */
    assert(!p4_mod_party_hero_level(&level, &reason) && level == 3);

    /* In battle: validated now, applied once outside; a change no longer valid then is logged, not applied. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2}, 2);
    scene = BATTLE;
    p4_mod_safe_point = 0;
    assert(p4_mod_skill_give(2, 10, 0, "t", &reason) == 1 && !strcmp(reason, "queued until the battle ends"));
    assert(p4_mod_exp_give(2, 0, 100, "t", &reason) == 1 && (p4_mod_safe_point & P4_MOD_SAFE_SERVICES));
    assert(p4_mod_skill_give(2, 12, 0, "t", &reason) == 1);
    assert(p4_mod_skill_give(2, 1, 0, "t", &reason) == -1); /* refused now, never queued */
    assert(skills(records[2])[2] == 0 && records[2][4] == 3);
    assert(p4_mod_party_pump() == 1 && skills(records[2])[2] == 0);
    skills(records[2])[5] = 12; /* learned meanwhile */
    scene = FIELD;
    assert(p4_mod_party_pump() == 0);
    assert(skills(records[2])[2] == 10 && records[2][4] == 4 && datPersonaFindSkillSlot(records[2], 12) == 5);
    assert(p4_mod_party_pump() == 0);

    /* The skill choice: refusals, an empty slot at once, a full Persona through the host. */
    p4_mod_safe_point = 0;
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2}, 2);
    assert(p4_mod_skill_choose(2, 900, 7, "t", on_done, NULL, &reason) == -1 && !strcmp(reason, "no such skill"));
    assert(p4_mod_skill_choose(2, 1, 7, "t", on_done, NULL, &reason) == -1 && !strcmp(reason, "the Persona already knows that skill"));
    assert(p4_mod_skill_choose(2, 20, 7, "t", on_done, (void *)1, &reason) == 0 && skills(records[2])[2] == 20);
    assert(!outcome_count && (p4_mod_safe_point & P4_MOD_SAFE_SERVICES)); /* done runs at the frame boundary */
    /* A done callback ran, so one more boundary although nothing is left here: what it queued for
     * a service that already pumped (items during a battle) is not stranded. */
    assert(p4_mod_party_pump() == 1 && outcome_count == 1 && outcome_user[0] == 1);
    assert(outcomes[0].learned && outcomes[0].member == 2 && outcomes[0].skill == 20 && !outcomes[0].forgot && !outcomes[0].reason);
    assert(p4_mod_party_pump() == 0);
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    assert(p4_mod_skill_choose(2, 21, 7, "t", on_done, (void *)2, &reason) == 1 && strstr(reason, "picks a skill to forget"));
    assert(p4_mod_skill_choose(2, 22, 7, "t", on_done, (void *)3, &reason) == 1 && !strcmp(reason, "queued behind another skill choice"));
    scene_ready = 0; /* the field is not free: nothing is offered */
    assert(p4_mod_party_pump() == 1 && host_state == HOST_IDLE && !p4_mod_skill_choice_claimed());
    scene_ready = 1;
    assert(p4_mod_party_pump() == 1 && host_state == HOST_ARMED && host_member == 2 && host_skill == 21);
    assert(p4_mod_skill_choice_claimed());
    scene_ready = 0; /* armed, not taken, and the field stopped being free: withdrawn */
    assert(p4_mod_party_pump() == 1 && host_state == HOST_IDLE && !p4_mod_skill_choice_claimed());
    scene_ready = 1;
    assert(p4_mod_party_pump() == 1 && host_state == HOST_ARMED);
    p4_mod_field_host_withdraw(); /* a scene request accepted after the offer goes first */
    assert(p4_mod_party_pump() == 1 && host_state == HOST_ARMED); /* offered again once the field is free */
    host_take(); /* the field controller took the slot */
    assert(p4_mod_party_pump() == 1 && host_state == HOST_BUSY && outcome_count == 1);
    host_answer(3); /* forget skill 4 */
    assert(p4_mod_party_pump() == 1 && outcome_count == 2 && outcome_user[1] == 2);
    assert(outcomes[1].learned && outcomes[1].skill == 21 && outcomes[1].forgot == 4 && skills(records[2])[3] == 21);
    assert(host_state == HOST_ARMED && host_skill == 22); /* the next one is offered in the same pump */
    host_take();
    host_answer(-1);
    assert(p4_mod_party_pump() == 1 && outcome_count == 3 && outcome_user[2] == 3);
    assert(!outcomes[2].learned && !strcmp(outcomes[2].reason, "declined") && datPersonaFindSkillSlot(records[2], 22) == -1);
    assert(p4_mod_party_pump() == 0);

    /* In battle: checked now, waits; a slot freed meanwhile teaches it silently once outside. */
    scene = BATTLE;
    assert(p4_mod_skill_choose(2, 23, 7, "t", on_done, (void *)4, &reason) == 1 && !strcmp(reason, "queued until the battle ends"));
    assert(p4_mod_party_pump() == 1 && host_state == HOST_IDLE && outcome_count == 3);
    skills(records[2])[7] = 0;
    scene = FIELD;
    assert(p4_mod_party_pump() == 1 && outcome_count == 4 && outcomes[3].learned && skills(records[2])[7] == 23);
    assert(p4_mod_party_pump() == 0);

    /* While the panel shows a Persona, its skills are the panel's copy: another change to that Persona
     * waits and is checked again after the answer, so the offered skill never ends up twice. Other
     * members and the protagonist's own level are not held. */
    const char *held = "queued until the skill choice open for that Persona closes";
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    flags[0x31] = 1; /* Chie (3) */
    persona(3, 0x12, 3, 200, (const uint16_t[]){1}, 1);
    assert(p4_mod_skill_choose(2, 30, 7, "t", on_done, (void *)9, &reason) == 1);
    assert(p4_mod_party_pump() == 1 && host_state == HOST_ARMED && host_skill == 30);
    host_take();
    assert(p4_mod_party_pump() == 1 && host_state == HOST_BUSY);
    assert(p4_mod_skill_give(2, 30, 5, "t", &reason) == 1 && !strcmp(reason, held));
    assert(p4_mod_exp_give(2, 0, 50, "t", &reason) == 1 && !strcmp(reason, held));
    assert(!p4_mod_skill_give(3, 40, 0, "t", &reason) && datPersonaFindSkillSlot(records[3], 40) == 1);
    assert(datPersonaFindSkillSlot(records[2], 30) == -1 && skills(records[2])[4] == 5 && *exp_of(records[2]) == 200);
    assert(p4_mod_party_pump() == 1 && skills(records[2])[4] == 5 && *exp_of(records[2]) == 200); /* still held */
    host_answer(3); /* the player forgets skill 4 for 30, from the panel's list */
    assert(p4_mod_party_pump() == 1 && outcome_count == 5 && outcome_user[4] == 9);
    assert(outcomes[4].learned && outcomes[4].forgot == 4);
    assert(copies(2, 30) == 1 && skills(records[2])[3] == 30 && skills(records[2])[4] == 5); /* the held give was refused */
    assert(*exp_of(records[2]) == 250 && p4_mod_party_pump() == 0);

    /* A camera lease ends before the panel takes the field; its exclusive input drains before the
     * offer, and the choice claims the field meanwhile (p4_mod_camera_acquire refuses then). */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    lease = "camera";
    p4_mod_input_fence = 1;
    assert(p4_mod_skill_choose(2, 31, 7, "t", on_done, (void *)10, &reason) == 1);
    assert(p4_mod_party_pump() == 1 && revoked == 1 && !lease && p4_mod_input_fence == 2);
    assert(host_state == HOST_IDLE && p4_mod_skill_choice_claimed());
    assert(p4_mod_party_pump() == 1 && host_state == HOST_IDLE && p4_mod_skill_choice_claimed()); /* draining */
    p4_mod_input_fence = 0; /* every button up */
    assert(p4_mod_party_pump() == 1 && host_state == HOST_ARMED && host_skill == 31 && p4_mod_skill_choice_claimed());
    host_take();
    host_answer(-1);
    assert(p4_mod_party_pump() == 1 && outcome_count == 6 && !outcomes[5].learned && revoked == 1);
    assert(p4_mod_party_pump() == 0 && !p4_mod_skill_choice_claimed());

    /* Release: a choice on screen finishes unheard, an armed one is withdrawn. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    assert(p4_mod_skill_choose(2, 24, 7, "t", on_done, (void *)5, &reason) == 1);
    assert(p4_mod_skill_choose(2, 25, 8, "u", on_done, (void *)6, &reason) == 1);
    assert(p4_mod_party_pump() == 1 && host_state == HOST_ARMED && host_skill == 24);
    host_take();
    p4_mod_skill_choice_release(7);
    assert(p4_mod_party_pump() == 1 && host_state == HOST_BUSY);
    host_answer(0);
    assert(p4_mod_party_pump() == 1 && outcome_count == 6 && skills(records[2])[0] == 24);
    assert(host_state == HOST_ARMED && host_skill == 25);
    p4_mod_skill_choice_release(8);
    assert(host_state == HOST_IDLE && p4_mod_party_pump() == 0 && outcome_count == 6);

    /* No game any more: a choice on screen is abandoned, one waiting ends unlearned. */
    assert(p4_mod_skill_choose(2, 26, 7, "t", on_done, (void *)7, &reason) == 1);
    assert(p4_mod_skill_choose(2, 27, 7, "t", on_done, (void *)8, &reason) == 1);
    assert(p4_mod_party_pump() == 1 && host_state == HOST_ARMED && host_skill == 26);
    host_take();
    scene = TITLE;
    assert(p4_mod_party_pump() == 1 && outcome_count == 8 && host_state == HOST_IDLE);
    assert(!outcomes[6].learned && !strcmp(outcomes[6].reason, "no game is loaded any more"));
    assert(!outcomes[7].learned && !strcmp(outcomes[7].reason, "no game is loaded") && outcome_user[7] == 8);
    assert(p4_mod_party_pump() == 0);
    assert(p4_mod_skill_choose(2, 28, 7, "t", on_done, NULL, &reason) == -1 && !strcmp(reason, "no game is loaded"));

    /* p4.party.set and the actions to one Persona apply in call order. A frame boundary runs the
     * party step (p4_mod_party_apply_pending) before the services step (p4_mod_party_pump), so an
     * action asked for after a pending write queues behind it instead of applying at once and
     * being overwritten by the older full-record snapshot. Other members are not held. */
    const char *after_write = "queued behind a pending p4.party.set for that Persona";
    const char *behind = "queued behind an earlier change to that Persona";
    char why[160];
    scene = FIELD;
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2}, 2);
    persona(3, 0x12, 3, 200, (const uint16_t[]){1}, 1);
    const P4ModPartyRecord low = party_record(0x11, 2, 150, (const uint16_t[]){1}, 1);
    p4_mod_safe_point = 0;
    assert(!p4_mod_party_queue(7, 2, &low, why, sizeof why) && (p4_mod_safe_point & P4_MOD_SAFE_PARTY));
    assert(p4_mod_skill_give(2, 60, 0, "t", &reason) == 1 && !strcmp(reason, after_write));
    assert(p4_mod_exp_give(2, 0, 100, "t", &reason) == 1 && !strcmp(reason, after_write));
    assert(p4_mod_skill_choose(2, 61, 7, "t", on_done, (void *)11, &reason) == 1 && !strcmp(reason, after_write));
    assert(!p4_mod_skill_give(3, 62, 0, "t", &reason) && skills(records[3])[1] == 62);
    assert(records[2][4] == 3 && *exp_of(records[2]) == 200 && skills(records[2])[1] == 2); /* nothing yet */
    assert(p4_mod_party_apply_pending() == 0); /* the write first */
    assert(records[2][4] == 2 && *exp_of(records[2]) == 150 && skills(records[2])[1] == 0 && records[2][0x1C] == 3);
    assert(p4_mod_party_pump() == 1); /* then the actions on top of it, in call order */
    assert(skills(records[2])[1] == 60 && *exp_of(records[2]) == 250 && records[2][4] == 3 && skills(records[2])[2] == 61);
    assert(outcome_count == 9 && outcome_user[8] == 11 && outcomes[8].learned);
    assert(p4_mod_party_pump() == 0 && p4_mod_party_apply_pending() == 0);

    /* While the panel shows a Persona its skill list is the panel's copy, which the native replace
     * task writes back: a write to that Persona waits for the answer, writes to other ids apply.
     * A write also waits for the actions to that Persona asked for before it; the actions asked
     * for after it wait for the write. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    assert(p4_mod_skill_choose(2, 63, 7, "t", on_done, (void *)12, &reason) == 1);
    assert(p4_mod_party_pump() == 1 && host_state == HOST_ARMED && host_skill == 63);
    host_take();
    assert(p4_mod_party_pump() == 1 && host_state == HOST_BUSY);
    assert(p4_mod_exp_give(2, 0, 50, "t", &reason) == 1 && !strcmp(reason, held));
    const P4ModPartyRecord full = party_record(0x11, 4, 300, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 9}, 8);
    assert(!p4_mod_party_queue(7, 2, &full, why, sizeof why));
    const P4ModPartyRecord chie = party_record(0x12, 5, 400, (const uint16_t[]){1}, 1);
    assert(!p4_mod_party_queue(8, 3, &chie, why, sizeof why));
    assert(p4_mod_skill_give(2, 64, 1, "t", &reason) == 1 && !strcmp(reason, held));
    assert(p4_mod_party_apply_pending() == 1 && records[3][4] == 5 && records[2][4] == 3); /* Chie's only */
    assert(p4_mod_party_pump() == 1 && *exp_of(records[2]) == 200 && skills(records[2])[0] == 1); /* still held */
    assert(p4_mod_party_apply_pending() == 1 && records[2][4] == 3 && skills(records[2])[7] == 8);
    host_answer(7); /* forget skill 8 for 63, from the panel's copy */
    assert(skills(records[2])[7] == 63);
    assert(p4_mod_party_apply_pending() == 1 && records[2][4] == 3); /* the EXP asked for before it goes first */
    assert(p4_mod_party_pump() == 1 && *exp_of(records[2]) == 250 && skills(records[2])[0] == 1);
    assert(outcome_count == 10 && outcome_user[9] == 12 && outcomes[9].learned && outcomes[9].forgot == 8);
    assert(p4_mod_party_apply_pending() == 0 && records[2][4] == 4 && *exp_of(records[2]) == 300);
    assert(skills(records[2])[7] == 9 && datPersonaFindSkillSlot(records[2], 63) == -1);
    assert(p4_mod_party_pump() == 0 && skills(records[2])[0] == 64);

    /* Owner teardown drops its pending write; the actions behind it then apply. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1}, 1);
    assert(!p4_mod_party_queue(7, 2, &low, why, sizeof why));
    assert(p4_mod_skill_give(2, 65, 0, "t", &reason) == 1 && !strcmp(reason, after_write));
    assert(p4_mod_party_pump() == 1 && skills(records[2])[1] == 0);
    p4_mod_party_clear_owner(7);
    assert(p4_mod_party_apply_pending() == 0 && records[2][4] == 3);
    assert(p4_mod_party_pump() == 0 && skills(records[2])[1] == 65);

    /* A battle before the safe point: the write waits for its end, and the actions asked for during
     * it and after it stay behind the write. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1}, 1);
    assert(!p4_mod_party_queue(7, 2, &low, why, sizeof why));
    scene = BATTLE;
    assert(p4_mod_party_queue(8, 3, &chie, why, sizeof why) == -1 && strstr(why, "during a battle"));
    assert(p4_mod_exp_give(2, 0, 100, "t", &reason) == 1 && !strcmp(reason, "queued until the battle ends"));
    assert(p4_mod_party_apply_pending() == 1 && p4_mod_party_pump() == 1 && records[2][4] == 3 && *exp_of(records[2]) == 200);
    scene = FIELD; /* mods run before the next boundary */
    assert(p4_mod_skill_give(2, 66, 0, "t", &reason) == 1 && !strcmp(reason, after_write));
    assert(p4_mod_party_apply_pending() == 0 && records[2][4] == 2 && *exp_of(records[2]) == 150);
    assert(p4_mod_party_pump() == 0 && *exp_of(records[2]) == 250 && records[2][4] == 3 && skills(records[2])[1] == 66);

    /* Actions queued by a battle, then one asked for after the battle and a write: the write waits
     * for both, and the later action does not pass the queued one. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1}, 1);
    scene = BATTLE;
    assert(p4_mod_exp_give(2, 0, 100, "t", &reason) == 1);
    scene = FIELD;
    assert(p4_mod_skill_give(2, 67, 0, "t", &reason) == 1 && !strcmp(reason, behind));
    assert(!p4_mod_party_queue(7, 2, &low, why, sizeof why));
    assert(p4_mod_party_apply_pending() == 1 && records[2][4] == 3);
    assert(p4_mod_party_pump() == 0 && records[2][4] == 4 && skills(records[2])[1] == 67);
    assert(p4_mod_party_apply_pending() == 0 && records[2][4] == 2 && skills(records[2])[1] == 0); /* last asked for */

    /* A done callback writes the record and teaches a skill to the same Persona: the write applies at
     * the next boundary's party step, the skill after it. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1}, 1);
    assert(!p4_mod_skill_choose(2, 68, 7, "t", on_done_write, (void *)13, &reason) && skills(records[2])[1] == 68);
    assert(p4_mod_party_pump() == 1 && outcome_count == 11 && outcome_user[10] == 13);
    assert(!reentry_write && reentry_give == 1 && !strcmp(reentry_reason, after_write) && skills(records[2])[2] == 0);
    assert(p4_mod_party_apply_pending() == 0 && records[2][4] == 2 && skills(records[2])[1] == 68);
    assert(p4_mod_party_pump() == 0 && skills(records[2])[2] == 69);

    /* Behind a pending write, a give is checked against the skills the write and the queued gives
     * after it leave, not the record as it is: a skill the write drops is taught again after it. */
    const char *known = "the Persona already knows that skill";
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2, 3}, 3);
    const P4ModPartyRecord drops = party_record(0x11, 3, 200, (const uint16_t[]){1, 3}, 2);
    assert(!p4_mod_party_queue(7, 2, &drops, why, sizeof why));
    assert(p4_mod_skill_give(2, 2, 0, "t", &reason) == 1 && !strcmp(reason, after_write)); /* the write drops 2 */
    assert(p4_mod_skill_give(2, 2, 0, "t", &reason) == -1 && !strcmp(reason, known)); /* the queued give teaches it */
    assert(p4_mod_skill_give(2, 72, 3, "t", &reason) == 1 && !strcmp(reason, after_write));
    assert(p4_mod_skill_give(2, 73, 9, "t", &reason) == -1 && !strcmp(reason, "the Persona does not know the skill to replace"));
    assert(p4_mod_skill_choose(2, 74, 7, "t", on_done, (void *)14, &reason) == 1 && !strcmp(reason, after_write));
    assert(skills(records[2])[1] == 2 && skills(records[2])[2] == 3 && !skills(records[2])[3]); /* nothing yet */
    assert(p4_mod_party_apply_pending() == 0 && skills(records[2])[1] == 3 && !skills(records[2])[2]);
    assert(p4_mod_party_pump() == 1 && outcome_count == 12 && outcome_user[11] == 14 && outcomes[11].learned);
    assert(skills(records[2])[0] == 1 && skills(records[2])[1] == 72 && skills(records[2])[2] == 2 && skills(records[2])[3] == 74);
    assert(p4_mod_party_pump() == 0);

    /* A give the write's list makes invalid is refused at once, though the record would take it;
     * one valid only against the write's list is accepted and applies after it. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1}, 1);
    const P4ModPartyRecord eight = party_record(0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    assert(!p4_mod_party_queue(7, 2, &eight, why, sizeof why));
    assert(p4_mod_skill_give(2, 8, 0, "t", &reason) == -1 && !strcmp(reason, known));
    assert(p4_mod_skill_give(2, 75, 0, "t", &reason) == -1 && strstr(reason, "all 8 skill slots are full"));
    assert(p4_mod_skill_choose(2, 8, 7, "t", on_done, NULL, &reason) == -1 && !strcmp(reason, known));
    assert(p4_mod_skill_give(2, 75, 8, "t", &reason) == 1 && !strcmp(reason, after_write)); /* 8: the write's, not the record's */
    assert(p4_mod_party_apply_pending() == 0 && p4_mod_party_pump() == 0 && skills(records[2])[7] == 75);

    /* Accepted against a write that then never happens (its owner shut down): the give is checked
     * again against the record when it applies, logged and skipped, never applied twice. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2}, 2);
    const P4ModPartyRecord only = party_record(0x11, 3, 200, (const uint16_t[]){1}, 1);
    assert(!p4_mod_party_queue(7, 2, &only, why, sizeof why));
    assert(p4_mod_skill_give(2, 2, 0, "t", &reason) == 1 && !strcmp(reason, after_write));
    p4_mod_party_clear_owner(7);
    assert(p4_mod_party_apply_pending() == 0 && p4_mod_party_pump() == 0);
    assert(copies(2, 2) == 1 && skills(records[2])[1] == 2 && !skills(records[2])[2]);

    /* A choice still waiting for its turn holds no place in call order (the documented "slot freed
     * meanwhile"): a write asked for after it applies first, and it settles against that record. */
    persona(2, 0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4, 5, 6, 7, 8}, 8);
    scene_ready = 0;
    assert(p4_mod_skill_choose(2, 76, 7, "t", on_done, (void *)15, &reason) == 1);
    assert(p4_mod_party_pump() == 1 && host_state == HOST_IDLE);
    const P4ModPartyRecord four = party_record(0x11, 3, 200, (const uint16_t[]){1, 2, 3, 4}, 4);
    assert(!p4_mod_party_queue(7, 2, &four, why, sizeof why));
    assert(p4_mod_party_apply_pending() == 0 && !skills(records[2])[4]);
    assert(p4_mod_party_pump() == 1 && outcome_count == 13 && outcome_user[12] == 15 && outcomes[12].learned);
    assert(skills(records[2])[4] == 76 && host_state == HOST_IDLE);
    scene_ready = 1;
    assert(p4_mod_party_pump() == 0);
    puts("game_party: members, joining, skills, replacement, EXP level-ups, the protagonist, queueing, skill choices "
         "and call order with party record writes passed");
    return 0;
}
