/* The strategic candidate policy's (battle_sim 8) decision rules: which action
 * wins, and where each rule's boundary lies, for situations policy.c hands
 * them. These do not qualify combat; native runs measure the policy itself. */
#undef NDEBUG
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "p4pc/battle_strategy.h"

/* The attentive scores policy.c passes in as `inherited`. */
#define ENEMY_DEBUFF 240
#define PARTY_BUFF 260
#define GUARD_ON_CHARGE 20000

static int64_t attack(int64_t power) { return 200 + power + 400; } /* focused enemy, element already tried */
static int64_t heal(unsigned hp, unsigned max_hp, int hero)
{
    const unsigned percent = hp * 100 / max_hp;
    return 24000 + (int64_t)(100 - percent) * 20 + (hero ? 500 : 0);
}

/* Tarunda (attack down, 0x2) on a boss that already took a hit and showed no weakness beats a plain attack; the same
 * cast on the party side, a mixed mask, a fresh fight or a fight with a known weakness does not. */
static void setup_choice(void)
{
    const P4StrategyTempo ready = {0};
    const P4StrategyEnemy boss = {.tried = 1u << 1, .weak = 0, .living = 1};
    const int durable = p4_strategy_durable(&boss, 1);
    assert(durable);
    assert(!(p4_strategy_setup_value(0x2u, P4_STRATEGY_ENEMY, durable, &ready, ENEMY_DEBUFF) > attack(600)));
    assert(p4_strategy_setup_value(0x1u, P4_STRATEGY_PARTY, durable, &ready, PARTY_BUFF) > attack(120));
    assert(p4_strategy_setup_value(0x2u, P4_STRATEGY_ENEMY, durable, &ready, ENEMY_DEBUFF) > attack(599));
    assert(p4_strategy_setup_value(0x1u, P4_STRATEGY_ENEMY, durable, &ready, ENEMY_DEBUFF) < attack(120));
    assert(p4_strategy_setup_value(0x3u, P4_STRATEGY_PARTY, durable, &ready, PARTY_BUFF) < attack(120));
    assert(p4_strategy_setup_value(0x400u, P4_STRATEGY_PARTY, durable, &ready, PARTY_BUFF) < attack(120));

    const P4StrategyEnemy fresh = {.tried = 0, .weak = 0, .living = 1};
    assert(p4_strategy_setup_value(0x2u, P4_STRATEGY_ENEMY, p4_strategy_durable(&fresh, 1), &ready, ENEMY_DEBUFF) <
           attack(120));
    const P4StrategyEnemy weak = {.tried = 1u << 2, .weak = 1u << 2, .living = 1};
    assert(!p4_strategy_durable(&weak, 1)); /* a seen weakness is a knockdown route, not a setup turn */
    /* A KO'd enemy does not count; every living one must have been hit and show no weakness. */
    P4StrategyEnemy field[3] = {{.tried = 0, .weak = 1u, .living = 0}, {.tried = 1u, .living = 1}, {.tried = 4u, .living = 1}};
    assert(p4_strategy_durable(field, 3));
    field[2].tried = 0;
    assert(!p4_strategy_durable(field, 3));
    field[0].living = field[1].living = field[2].living = 0;
    assert(!p4_strategy_durable(field, 3));
}

/* The review's repro: four attacks, then each member would rather set up. Only the first gets the raise; a heal or a
 * Guard does not reopen it, a damage decision does, and a new wave starts clean. */
static void setup_tempo(void)
{
    const P4StrategyEnemy boss = {.tried = 1u, .living = 1};
    const int durable = p4_strategy_durable(&boss, 1);
    P4StrategyTempo tempo = {0};
    for (unsigned i = 0; i < 4; i++) p4_strategy_tempo_commit(&tempo, P4_STRATEGY_COMMIT_DAMAGE);
    unsigned boosted = 0;
    for (unsigned member = 0; member < 4; member++) {
        const int64_t setup = p4_strategy_setup_value(0x2u, P4_STRATEGY_ENEMY, durable, &tempo, ENEMY_DEBUFF);
        const unsigned kind = setup > attack(120) ? P4_STRATEGY_COMMIT_SETUP : P4_STRATEGY_COMMIT_DAMAGE;
        boosted += kind == P4_STRATEGY_COMMIT_SETUP;
        p4_strategy_tempo_commit(&tempo, kind);
    }
    assert(boosted == 2); /* setup, attack, setup, attack: never two in a row */

    tempo = (P4StrategyTempo){0};
    p4_strategy_tempo_commit(&tempo, P4_STRATEGY_COMMIT_SETUP);
    p4_strategy_tempo_commit(&tempo, P4_STRATEGY_COMMIT_OTHER); /* heal or Guard */
    assert(p4_strategy_setup_value(0x2u, P4_STRATEGY_ENEMY, durable, &tempo, ENEMY_DEBUFF) == ENEMY_DEBUFF);
    p4_strategy_tempo_commit(&tempo, P4_STRATEGY_COMMIT_DAMAGE);
    assert(p4_strategy_setup_value(0x2u, P4_STRATEGY_ENEMY, durable, &tempo, ENEMY_DEBUFF) > attack(120));

    /* 60 decisions, the debuff lapsing every third: setups never exceed and never follow each other. */
    tempo = (P4StrategyTempo){0};
    p4_strategy_tempo_commit(&tempo, P4_STRATEGY_COMMIT_DAMAGE);
    unsigned setups = 0, damages = 1, held = 0, previous = P4_STRATEGY_COMMIT_DAMAGE;
    for (unsigned turn = 0; turn < 60; turn++) {
        if (turn % 3 == 0) held = 0;
        const int64_t debuff = held ? 0 : p4_strategy_setup_value(0x2u, P4_STRATEGY_ENEMY, durable, &tempo, ENEMY_DEBUFF);
        const unsigned kind = debuff > attack(120) ? P4_STRATEGY_COMMIT_SETUP : P4_STRATEGY_COMMIT_DAMAGE;
        assert(!(kind == P4_STRATEGY_COMMIT_SETUP && previous == P4_STRATEGY_COMMIT_SETUP));
        if (kind == P4_STRATEGY_COMMIT_SETUP) setups++, held = 1;
        else damages++;
        p4_strategy_tempo_commit(&tempo, kind);
        previous = kind;
    }
    assert(setups > 0 && setups <= damages);
}

static void encounters(void)
{
    const uint32_t wave1[2] = {11, 12}, summoned[3] = {11, 12, 13}, wave2[1] = {21};
    assert(!p4_strategy_new_encounter(NULL, 0, wave1, 2)); /* the first decision */
    assert(!p4_strategy_new_encounter(wave1, 2, wave1, 2));
    assert(!p4_strategy_new_encounter(wave1, 2, summoned, 3)); /* a summon joins the same fight */
    assert(!p4_strategy_new_encounter(summoned, 3, wave1 + 1, 1)); /* survivors of it */
    assert(p4_strategy_new_encounter(summoned, 3, wave2, 1));
    assert(!p4_strategy_new_encounter(wave1, 2, NULL, 0));
}

/* Kanji seed 5 (strategic, update 7706): the hero at 57/329 under a visible charge used a 20-HP item (skill 288)
 * while a 100-HP one (289) was held. The fixed amounts now decide; a heal still outranks the charge Guard. */
static void heal_amounts(void)
{
    const int64_t base = heal(57, 329, 1);
    const int64_t small = p4_strategy_heal_value(base, 1, 20, 57, 329), large = p4_strategy_heal_value(base, 1, 100, 57, 329);
    assert(large > small);
    assert(small > GUARD_ON_CHARGE);
    /* Covering the whole deficit costs nothing; more power beyond it earns nothing. */
    assert(p4_strategy_heal_value(base, 1, 272, 57, 329) == base);
    assert(p4_strategy_heal_value(base, 1, 999, 57, 329) == base);
    assert(p4_strategy_heal_value(base, 1, 271, 57, 329) == base - 1);
    /* An amount the description does not fix (a formula or percentage heal) keeps its score. */
    assert(p4_strategy_heal_value(base, 0, 20, 57, 329) == base);
    assert(p4_strategy_heal_value(base, 1, 20, 329, 329) == base);
    /* A deficit up to the 999 HP cap leaves any heal above the charge Guard. */
    assert(p4_strategy_heal_value(heal(1, 999, 0), 1, 0, 1, 999) > GUARD_ON_CHARGE);
}

/* A stocked Persona's skill is scored only when the active Persona's pricing is that Persona's own. */
static void pricing(void)
{
    assert(p4_strategy_switch_priceable(0, 0, 0, 0, 0)); /* no cost */
    assert(p4_strategy_switch_priceable(0, 0, 1, 1, 0)); /* no cost: neither halving nor the stage applies */
    assert(p4_strategy_switch_priceable(2, 0, 0, 0, 0));
    assert(p4_strategy_switch_priceable(2, 0, 0, 1, 1));
    /* The review's repro: the lead's Spell Master halves Maragion's 12 SP to 6, which the candidate would not. */
    assert(!p4_strategy_switch_priceable(2, 0, 0, 1, 0));
    assert(!p4_strategy_switch_priceable(2, 0, 0, 0, 1));
    assert(!p4_strategy_switch_priceable(2, 0, 1, 0, 0)); /* a cost stage prices per Persona */
    assert(!p4_strategy_switch_priceable(2, 1, 0, 0, 0)); /* percentage of max SP, which Persona passives change */
    assert(!p4_strategy_switch_priceable(1, 0, 0, 0, 0)); /* HP: percentage of max HP, Arms Master */
}

static void persona_switch(void)
{
    /* A stocked healer is worth a change under a visible charge; an attack never displaces a heal. */
    assert(p4_strategy_switch(GUARD_ON_CHARGE, heal(100, 329, 1), 0));
    assert(!p4_strategy_switch(heal(100, 329, 1), attack(120) + 3000, 0));
    /* Near ties keep the Persona; the margin is exclusive. */
    assert(!p4_strategy_switch(attack(100), attack(100) + P4_STRATEGY_SWITCH_MARGIN, 0));
    assert(p4_strategy_switch(attack(100), attack(100) + P4_STRATEGY_SWITCH_MARGIN + 1, 0));
    /* Once per command, then the reopened panel decides; nothing usable means no change. */
    assert(!p4_strategy_switch(attack(100), attack(100) + 5000, 1));
    assert(!p4_strategy_switch(attack(100), INT64_MIN, 0));
    assert(p4_strategy_switch(INT64_MIN, 0, 0));
    assert(!p4_strategy_switch(INT64_MAX - 1, INT64_MAX - 1, 0));
}

int main(void)
{
    setup_choice();
    setup_tempo();
    encounters();
    heal_amounts();
    pricing();
    persona_switch();
    return 0;
}
