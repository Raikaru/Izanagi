/* battlerule: parsing and the ordered rule selection over a native battle snapshot (include/p4pc/walk_script.h). */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "p4pc/walk_script.h"

static int failures;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "walk_battle_rules: line %d: %s\n", __LINE__, #condition); \
        failures++; \
    } \
} while (0)

static uint32_t read_zero(void *user, uint32_t address)
{
    (void)user;
    (void)address;
    return 0;
}

static void parse_rules(void)
{
    P4WalkBattleRule rule;
    static const char *const bad[] = {
        "", "dead", "dead=", "=cross", "hp<0=cross", "hp<=cross", "hp< 5=cross", "hp<-3=cross", "hp<70000=cross",
        "hp<5x=cross", "charge:=cross", "charge:0=cross", "charge:0x10000=cross", "status:0=cross", "status:x=cross",
        "dead@9=cross", "dead@0=cross", "dead@=cross", "dead@1,=cross", "dead@1,,2=cross", "dead@a=cross",
        "bogus=cross", "dead=fly", "dead@1@2=cross",
    };

    CHECK(p4walk_battle_rule_parse("dead@1=circle,down,down,down,cross,down,down,down,cross,cross", &rule));
    CHECK(rule.kind == P4WALK_RULE_DEAD && rule.actors == 1u && rule.key_count == 10);
    CHECK(rule.keys[0] == 1u << P4W_CIRCLE && rule.keys[9] == 1u << P4W_CROSS);
    CHECK(p4walk_battle_rule_parse("hp<150@1,2,6=circle,down,cross,cross,cross", &rule));
    CHECK(rule.kind == P4WALK_RULE_HP_BELOW && rule.value == 150u && rule.actors == (1u | 2u | 32u) && rule.key_count == 5);
    CHECK(p4walk_battle_rule_parse("charge:0x105=circle,up,cross", &rule));
    CHECK(rule.kind == P4WALK_RULE_CHARGE && rule.value == 261u && rule.actors == 0u && rule.key_count == 3);
    CHECK(p4walk_battle_rule_parse("status:8=circle,up,cross", &rule));
    CHECK(rule.kind == P4WALK_RULE_STATUS && rule.value == 8u);
    CHECK(p4walk_battle_rule_parse("sp<14@1=circle,down,down,down,cross,cross,cross", &rule));
    CHECK(rule.kind == P4WALK_RULE_SP_BELOW && rule.value == 14u && rule.actors == 1u);
    CHECK(!p4walk_battle_rule_parse("sp<0=cross", &rule) && !p4walk_battle_rule_parse("sp<=cross", &rule));
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        int accepted = p4walk_battle_rule_parse(bad[i], &rule);
        if (accepted) fprintf(stderr, "walk_battle_rules: accepted '%s'\n", bad[i]);
        CHECK(!accepted);
    }
}

static void select_rules(void)
{
    static const char *const text[] = {
        "dead@1=circle,down,down,down,cross,cross",    /* 0: Yu revives */
        "hp<150=circle,down,cross,cross,cross",        /* 1: anyone heals the lowest ally under 150 */
        "status:0x8=circle,cross,cross",                /* 2: silenced: Attack */
        "charge:261=circle,up,cross",                   /* 3: a charged Shadow Teddie: guard */
    };
    P4WalkBattleRule rules[4];
    for (int i = 0; i < 4; ++i) CHECK(p4walk_battle_rule_parse(text[i], &rules[i]));

    P4WalkBattleUnit party[4] = {{.id = 1, .hp = 120, .sp = 40, .bad = 8}, {.id = 2, .hp = 0, .sp = 30},
                                 {.id = 3, .hp = 90, .sp = 10}, {.id = 6, .hp = 300}};
    P4WalkBattleUnit enemies[1] = {{.id = 261, .hp = 5000, .sp = 1500, .status = 0x80000}};
    P4WalkBattleView view = {1, party, 4, enemies, 1, 0};
    uint32_t ally = 99, enemy = 99;

    /* Order decides: Yu's revive outranks the heal, and binds the first downed ally. */
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == 0 && ally == 2);
    /* The revive is Yu's only: Chie heals instead, binding the lowest living ally under 150 (herself). */
    view.actor = 3;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == 1 && ally == 3);
    /* Equal HP: party order breaks the tie. */
    party[0].hp = 90;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == 1 && ally == 1);
    /* Nobody down or under 150: Yu's own silence status selects its rule, with no bound ally. */
    party[0].hp = 200;
    party[1].hp = 180;
    party[2].hp = 150;
    view.actor = 1;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == 2 && ally == 0);
    /* The status is the actor's own: Kanji falls through to the charged enemy. */
    view.actor = 6;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == 3 && ally == 0);
    /* No charge flag, or a dead charger: no rule, the default first/plan stands. */
    enemies[0].status = 0x8;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == -1 && ally == 0);
    enemies[0].status = 0x40000;
    enemies[0].hp = 0;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == -1);
    enemies[0].hp = 1;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == 3);
    /* Another enemy ID is not the charged one. */
    enemies[0].id = 262;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == -1);
    /* An actor outside 1..8 never selects a rule. */
    view.actor = 0;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == -1 && ally == 0);
    view.actor = 9;
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy) == -1);
    /* Selection reads only the snapshot: the same view gives the same answer. */
    view.actor = 1;
    party[2].hp = 40;
    uint32_t again = 0;
    int first = p4walk_battle_rule_select(rules, 4, &view, &ally, &enemy);
    CHECK(first == 1 && ally == 3);
    CHECK(p4walk_battle_rule_select(rules, 4, &view, &again, &enemy) == first && again == ally);

    /* sp<N reads the actor's own SP only and keeps its place in the order. */
    P4WalkBattleRule sp_rules[2];
    CHECK(p4walk_battle_rule_parse("sp<20=circle,down,down,down,cross,cross,cross", &sp_rules[0]));
    CHECK(p4walk_battle_rule_parse("hp<150=circle,down,cross,cross,cross", &sp_rules[1]));
    view.actor = 1;                                   /* Yu: 40 SP */
    CHECK(p4walk_battle_rule_select(sp_rules, 2, &view, &ally, &enemy) == 1 && ally == 3);
    view.actor = 3;                                   /* Chie: 10 SP */
    CHECK(p4walk_battle_rule_select(sp_rules, 2, &view, &ally, &enemy) == 0 && ally == 0);
    view.actor = 8;                                   /* not in the party: no own SP to read */
    CHECK(p4walk_battle_rule_select(sp_rules, 1, &view, &ally, &enemy) == -1);

    /* buff:M decodes the actor's signed level nibbles as datCalcHasBuffMask does: 0x80 is defense (index 3, +0x1D high) down. */
    P4WalkBattleRule buff_rules[3], rule_scratch;
    CHECK(p4walk_battle_rule_parse("buff:0x80=circle,up,cross", &buff_rules[0]));
    CHECK(buff_rules[0].kind == P4WALK_RULE_BUFF && buff_rules[0].value == 0x80u);
    CHECK(p4walk_battle_rule_parse("buff:0x1=circle,cross,cross", &buff_rules[1]));
    CHECK(p4walk_battle_rule_parse("buff:0x208=circle,cross,cross", &buff_rules[2]));
    CHECK(!p4walk_battle_rule_parse("buff:0=cross", &rule_scratch) && !p4walk_battle_rule_parse("buff:0x400=cross", &rule_scratch));
    memset(party[0].buffs, 0, sizeof party[0].buffs);
    view.actor = 1;
    CHECK(p4walk_battle_rule_select(buff_rules, 3, &view, &ally, &enemy) == -1);
    party[0].buffs[1] = 0xE0;                         /* defense -1 */
    CHECK(p4walk_battle_rule_select(buff_rules, 3, &view, &ally, &enemy) == 0 && ally == 0);
    party[0].buffs[1] = 0x20;                         /* defense +2: neither defense down nor the others */
    CHECK(p4walk_battle_rule_select(buff_rules, 3, &view, &ally, &enemy) == -1);
    party[0].buffs[0] = 0x01;                         /* attack +1 */
    CHECK(p4walk_battle_rule_select(buff_rules, 3, &view, &ally, &enemy) == 1);
    party[0].buffs[0] = 0xF0;                         /* index 1 nibble 15: level 0, not lowered */
    CHECK(p4walk_battle_rule_select(buff_rules, 3, &view, &ally, &enemy) == -1);
    party[0].buffs[2] = 0x09;                         /* index 4 at -6 */
    CHECK(p4walk_battle_rule_select(buff_rules, 3, &view, &ally, &enemy) == 2);
    party[0].buffs[2] = 0x00;
    view.actor = 3;                                   /* another character's debuff does not count */
    party[0].buffs[1] = 0xE0;
    CHECK(p4walk_battle_rule_select(buff_rules, 3, &view, &ally, &enemy) == -1);

    /* afflicted:M binds the first living member in party order whose status matches (fear 0x4 before Ghastly Wail). */
    P4WalkBattleRule fear;
    CHECK(p4walk_battle_rule_parse("afflicted:0x4@4=circle,down,cross,skill:200,cross", &fear));
    CHECK(fear.kind == P4WALK_RULE_AFFLICTED && fear.value == 4u && fear.actors == 1u << 3);
    CHECK(!p4walk_battle_rule_parse("afflicted:=cross", &rule_scratch) && !p4walk_battle_rule_parse("afflicted:0=cross", &rule_scratch));
    view.actor = 4;
    party[3].status = 0x4;                            /* charge flags (+0x14) are not ailments */
    CHECK(p4walk_battle_rule_select(&fear, 1, &view, &ally, &enemy) == -1);
    party[2].bad = 0x4;
    party[1].bad = 0x4 | 0x8;
    CHECK(p4walk_battle_rule_select(&fear, 1, &view, &ally, &enemy) == 0 && ally == party[1].id);
    party[1].hp = 0;                                  /* a downed member is not cured */
    CHECK(p4walk_battle_rule_select(&fear, 1, &view, &ally, &enemy) == 0 && ally == party[2].id);
    view.actor = 1;                                   /* the rule is Yukiko's alone */
    CHECK(p4walk_battle_rule_select(&fear, 1, &view, &ally, &enemy) == -1);

    /* A rule whose skill:/item: entry the actor lacks (a spent Revival Bead) is skipped for the next one. */
    P4WalkBattleRule revive[2];
    CHECK(p4walk_battle_rule_parse("dead@1=circle,down,down,down,cross,item:0x30F,cross", &revive[0]));
    CHECK(p4walk_battle_rule_parse("dead@1=circle,down,down,down,cross,item:0x310,cross", &revive[1]));
    view.unusable = 0;
    CHECK(p4walk_battle_rule_select(revive, 2, &view, &ally, &enemy) == 0 && ally == 2);
    view.unusable = 1u;
    CHECK(p4walk_battle_rule_select(revive, 2, &view, &ally, &enemy) == 1 && ally == 2);
    view.unusable = 3u;
    CHECK(p4walk_battle_rule_select(revive, 2, &view, &ally, &enemy) == -1);
    view.unusable = 0;

    /* enemy:ID: a living enemy with that ID is in the fight (Escape from the rare unit 138); no bound ally. */
    P4WalkBattleRule flee;
    CHECK(p4walk_battle_rule_parse("enemy:138@1=circle,down,down,down,down,cross", &flee));
    CHECK(flee.kind == P4WALK_RULE_ENEMY && flee.value == 138u);
    CHECK(!p4walk_battle_rule_parse("enemy:=cross", &rule_scratch) && !p4walk_battle_rule_parse("enemy:0x10000=cross", &rule_scratch));
    enemies[0].id = 138;
    enemies[0].hp = 546;
    CHECK(p4walk_battle_rule_select(&flee, 1, &view, &ally, &enemy) == 0 && ally == 0);
    enemies[0].hp = 0;
    CHECK(p4walk_battle_rule_select(&flee, 1, &view, &ally, &enemy) == -1);
    CHECK(enemy == 0);
}

/* datCalcGetAffinity for enemies: row flags/percent, passives, Guard, Break and Wall. */
static void affinities(void)
{
    uint32_t a;
    a = p4walk_enemy_affinity(0x0814, 0, 0, 0, 0);                         /* unit 50, elec: weak, 0x14 * 5 = 100% */
    CHECK((a & 0x8000000u) && (a & 0xFFFFu) == 100);
    a = p4walk_enemy_affinity(0x0800, 0, 0, 0, 0);                         /* weak with no percent: 125% */
    CHECK((a & 0x8000000u) && (a & 0xFFFFu) == 125);
    a = p4walk_enemy_affinity(0x0400, 0, 0, 0, 0);                         /* Shadow Teddie, ice: absorb */
    CHECK((a & 0x4000000u) && !(a & 0x8000000u) && (a & 0xFFFFu) == 100);
    a = p4walk_enemy_affinity(0x0100, 0, 0, 0, 0);                         /* boss light/dark: null */
    CHECK(a & 0x1000000u);
    a = p4walk_enemy_affinity(0x1000, 0, 0, 0, 0);                         /* Naoto, fire: resist at 50% */
    CHECK((a & 0x10000000u) && (a & 0xFFFFu) == 50);
    a = p4walk_enemy_affinity(0x000F, 0, 0, 0, 0);                         /* Mitsuo, physical: 75%, no flag */
    CHECK(!(a & 0xFF000000u) && (a & 0xFFFFu) == 75);
    a = p4walk_enemy_affinity(0x0814, 0, 1, 0, 0);                         /* a guarding enemy loses the weakness */
    CHECK(!(a & 0x8000000u) && (a & 0xFFFFu) == 100);
    a = p4walk_enemy_affinity(0x0814, 2, 0, 0, 0);                         /* a Null passive overrides the weakness */
    CHECK((a & 0x1000000u) && !(a & 0x8000000u));
    a = p4walk_enemy_affinity(0x0014, 8, 0, 0, 0);                         /* an Absorb passive */
    CHECK(a & 0x4000000u);
    a = p4walk_enemy_affinity(0x0400, 0, 0, 1, 0);                         /* a Break clears absorb */
    CHECK(!(a & 0xFF000000u) && (a & 0xFFFFu) == 100);
    a = p4walk_enemy_affinity(0x0814, 0, 0, 0, 1);                         /* a Wall turns a weakness into resist */
    CHECK((a & 0x10000000u) && !(a & 0x8000000u) && (a & 0xFFFFu) == 50);
    a = p4walk_enemy_affinity(0x0400, 0, 0, 0, 1);                         /* but leaves absorb */
    CHECK(a & 0x4000000u);

    /* weak:E binds the first living enemy weak to E; hits:E the first not resisting, nulling, repelling or absorbing it. */
    P4WalkBattleRule weak, hits;
    CHECK(p4walk_battle_rule_parse("weak:3@6=circle,down,cross,skill:40,cross", &weak) && weak.kind == P4WALK_RULE_WEAK && weak.value == 3u);
    CHECK(p4walk_battle_rule_parse("hits:2@3=circle,down,cross,skill:27,cross", &hits) && hits.kind == P4WALK_RULE_HITS && hits.value == 2u);
    P4WalkBattleRule scratch;
    CHECK(!p4walk_battle_rule_parse("weak:8=cross", &scratch) && !p4walk_battle_rule_parse("hits:=cross", &scratch) &&
          !p4walk_battle_rule_parse("weak:0x3=cross", &scratch));
    P4WalkBattleUnit party[2] = {{.id = 3, .hp = 300}, {.id = 6, .hp = 380}};
    P4WalkBattleUnit foes[3] = {{.id = 244, .hp = 0, .weak = 1u << 3}, {.id = 7, .hp = 90, .resist = 1u << 2},
                                {.id = 220, .hp = 200, .weak = 1u << 3, .block = 1u << 2}};
    P4WalkBattleView view = {6, party, 2, foes, 3, 0};
    uint32_t ally = 99, enemy = 99;
    CHECK(p4walk_battle_rule_select(&weak, 1, &view, &ally, &enemy) == 0 && enemy == 220 && ally == 0);   /* the dead 244 is skipped */
    view.actor = 3;
    CHECK(p4walk_battle_rule_select(&hits, 1, &view, &ally, &enemy) == -1 && enemy == 0);   /* 7 resists ice, 220 blocks it */
    foes[1].resist = 0;
    CHECK(p4walk_battle_rule_select(&hits, 1, &view, &ally, &enemy) == 0 && enemy == 7);
    view.actor = 6;
    CHECK(p4walk_battle_rule_select(&hits, 1, &view, &ally, &enemy) == -1);                 /* the rule is Chie's */

    /* nobuff:M re-applies a lapsed -kaja; foebuff:M binds the first enemy with a matching raised stat (Dekaja). */
    P4WalkBattleRule lapse, strip;
    CHECK(p4walk_battle_rule_parse("nobuff:0x41=circle,down,cross,skill:219,cross", &lapse) && lapse.kind == P4WALK_RULE_NOBUFF);
    CHECK(p4walk_battle_rule_parse("foebuff:0x55@1=circle,down,cross,skill:224,cross", &strip) && strip.kind == P4WALK_RULE_FOEBUFF);
    CHECK(!p4walk_battle_rule_parse("nobuff:0=cross", &scratch) && !p4walk_battle_rule_parse("foebuff:0x400=cross", &scratch));
    view.actor = 6;
    CHECK(p4walk_battle_rule_select(&lapse, 1, &view, &ally, &enemy) == 0 && ally == 0 && enemy == 0);
    party[1].buffs[0] = 0x01;                                             /* Kanji's attack +1 */
    CHECK(p4walk_battle_rule_select(&lapse, 1, &view, &ally, &enemy) == -1);
    party[1].buffs[0] = 0xE0;                                             /* attack 0, agility... index 1 at -1: not a raise */
    CHECK(p4walk_battle_rule_select(&lapse, 1, &view, &ally, &enemy) == 0);
    view.actor = 1;
    CHECK(p4walk_battle_rule_select(&strip, 1, &view, &ally, &enemy) == -1);
    foes[2].buffs[1] = 0x10;                                              /* 220's defense (index 3, high nibble) +1 */
    CHECK(p4walk_battle_rule_select(&strip, 1, &view, &ally, &enemy) == 0 && enemy == 220);
    foes[2].hp = 0;
    CHECK(p4walk_battle_rule_select(&strip, 1, &view, &ally, &enemy) == -1);
}

static void walk_lines(void)
{
    char error[160];
    P4WalkHost host = {read_zero, NULL, NULL, NULL, NULL};
    P4Walk *walk = p4walk_parse("battle:plan=cross\nbattlerule:hp<100@1=cross\nbattlerule:dead=cross\n", error, sizeof error);
    CHECK(walk != NULL);
    p4walk_free(walk);
    walk = p4walk_parse("battlerule:hp<=cross\n", error, sizeof error);
    CHECK(walk == NULL);
    p4walk_free(walk);
    /* A rule needs a driver to attach to: without battle: it fails the walk instead of being dropped. */
    walk = p4walk_parse("battlerule:dead=cross\n", error, sizeof error);
    CHECK(walk != NULL);
    if (walk) {
        p4walk_frame(walk, &host, 1);
        CHECK(p4walk_status(walk) == P4WALK_FAILED);
    }
    p4walk_free(walk);
    /* Seventeen rules overflow the driver's sixteen. */
    char script[1024] = "battle:plan=cross\n";
    for (int i = 0; i < 17; ++i) strcat(script, "battlerule:dead=cross\n");
    walk = p4walk_parse(script, error, sizeof error);
    CHECK(walk != NULL);
    if (walk) {
        for (uint32_t frame = 1; frame < 4 && p4walk_status(walk) == P4WALK_RUNNING; ++frame) p4walk_frame(walk, &host, frame);
        CHECK(p4walk_status(walk) == P4WALK_FAILED);
    }
    p4walk_free(walk);
}

/* skill:ID / item:ID keys: parsed into P4WALK_KEY_SKILL/ITEM | ID, navigated the shorter way around the wrapping list. */
static void skill_keys(void)
{
    P4WalkBattleRule rule;
    CHECK(p4walk_battle_rule_parse("dead@4=circle,down,cross,skill:0xC5,cross,cross", &rule));
    CHECK(rule.key_count == 6 && rule.keys[3] == (P4WALK_KEY_SKILL | 0xC5u) && rule.keys[4] == 1u << P4W_CROSS);
    CHECK(!p4walk_battle_rule_parse("dead=skill:0", &rule) && !p4walk_battle_rule_parse("dead=skill:0x1B8", &rule) &&
          !p4walk_battle_rule_parse("dead=skill:abc", &rule) && !p4walk_battle_rule_parse("dead=skill:", &rule));
    CHECK(p4walk_battle_rule_parse("dead@1=circle,down,down,down,cross,item:0x30F,cross,cross", &rule));
    CHECK(rule.key_count == 8 && rule.keys[5] == (P4WALK_KEY_ITEM | 0x30Fu));
    CHECK(!p4walk_battle_rule_parse("dead=item:0x300", &rule) && !p4walk_battle_rule_parse("dead=item:0x400", &rule) &&
          !p4walk_battle_rule_parse("dead=item:", &rule) && !p4walk_battle_rule_parse("dead=item:5x", &rule));
    /* Yukiko on 08/03: Dia, Agi, Me Patra, Maragi, Media, Agilao, Recarm. */
    const uint16_t list[] = {192, 1, 200, 4, 195, 2, 209};
    int moves = 99;
    CHECK(p4walk_battle_list_moves(list, 7, 0, 195, &moves) && moves == -3);     /* Dia -> Media: Up three (wraps) beats Down four */
    CHECK(p4walk_battle_list_moves(list, 7, 4, 195, &moves) && moves == 0);
    CHECK(p4walk_battle_list_moves(list, 7, 4, 2, &moves) && moves == 1);
    CHECK(p4walk_battle_list_moves(list, 7, 6, 192, &moves) && moves == 1);      /* Recarm -> Dia: Down wraps */
    CHECK(p4walk_battle_list_moves(list, 6, 0, 4, &moves) && moves == 3);        /* a tie goes Down */
    CHECK(!p4walk_battle_list_moves(list, 7, 0, 30, &moves));                    /* not learned */
    CHECK(!p4walk_battle_list_moves(list, 7, 7, 195, &moves) && !p4walk_battle_list_moves(list, 7, -1, 195, &moves));
    CHECK(!p4walk_battle_list_moves(list, 0, 0, 192, &moves));
}

/* Results-screen skill offer: decline through the real list and Yes/No, never forgetting a known skill. */
static void skill_offer(void)
{
    P4WalkSkillPrompt p = {0};
    CHECK(p4walk_skill_prompt_buttons(&p) == 0);                          /* no text row yet */
    p.row_waiting = 1;
    CHECK(p4walk_skill_prompt_buttons(&p) == 1u << P4W_CROSS);
    p = (P4WalkSkillPrompt){.state = 2, .list_active = 1, .list_cursor = 3, .pending = 1};
    CHECK(p4walk_skill_prompt_buttons(&p) == 1u << P4W_LEFT);            /* walk to the offered skill */
    p.list_cursor = 8;
    CHECK(p4walk_skill_prompt_buttons(&p) == 1u << P4W_CROSS);
    p.pending = 0;
    CHECK(p4walk_skill_prompt_buttons(&p) == 0);
    p = (P4WalkSkillPrompt){.state = 3, .choice_active = 1, .replacement_pending = 1, .choice_cursor = 1};
    CHECK(p4walk_skill_prompt_buttons(&p) == 1u << P4W_UP);              /* the default No: move to Yes */
    p.choice_cursor = 0;
    CHECK(p4walk_skill_prompt_buttons(&p) == 1u << P4W_CROSS);
    p.replacement_pending = 0;                                            /* a known skill is chosen: answer No */
    CHECK(p4walk_skill_prompt_buttons(&p) == 1u << P4W_DOWN);
    p.state = 4;
    CHECK(p4walk_skill_prompt_buttons(&p) == 0);
}

int main(void)
{
    parse_rules();
    select_rules();
    affinities();
    walk_lines();
    skill_keys();
    skill_offer();
    if (failures) {
        fprintf(stderr, "walk_battle_rules: %d failure(s)\n", failures);
        return 1;
    }
    printf("walk_battle_rules: ok\n");
    return 0;
}
