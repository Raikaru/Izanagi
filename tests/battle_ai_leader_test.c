/* Host checks for overrides 001e1530 (battle script command 0x205A,
 * btlAICommandSideIdQueryMyHp) and 001e1800 (0x205D,
 * btlAICommandSideIdQueryMyBad) together with 001e6d20 (0x207B,
 * btlEffectScriptPickUnitByKind), on the ally AI's HP and ailment ranking of
 * the leader. The override bodies are included as they are spliced into the
 * decomp mirror; the battle records they read and their callees are defined
 * here, with retail semantics:
 *   btlEffectFindUnitByIdAndStatus (001d9280): active actions (flags 1 and 8)
 *     of the side mask, by character id, skipping the dead under status
 *     0x80000 and any unit with a bad status in the status mask;
 *   btlCond_MYHP (001d9740), condition 1: hp * 100 <= maxHp * N;
 *   btlCond_MYBAD (001d9b60), condition 11: datCalcChkBadStatus(unit, N);
 *   btlEffectScriptPickLowestHpLivingSide0Unit (0x209B, 001e8d80): the living
 *     player-side action with the least absolute HP, its id with the high bit;
 *   GetHp/GetMaxHp (0x20E7/0x20E6): btlActionFindById over id & 0x0FFFFFFF.
 * The retail bodies of 001e1530 and 001e1800 (decomp FUN_001E1530,
 * FUN_001E1800) are kept below as the reference.
 *
 * ally_ranking() replays what AICALC.TBL block 9 does with the HP commands:
 * GET_Player_Parameter's protagonist tier (local 18 from 0x205A for character
 * 1 at 40 and 60), the lowest-HP ally's tier (local 20, computed only when
 * that ally is not the 0x207B kind-0 answer for character 1), and SET_Heal's
 * target (the protagonist action when local 18 >= local 20, else the lowest
 * ally). The AI patterns heal when either tier is 2.
 * ailment_ranking() replays its ailment use of 0x205D for character 1:
 * GET_Point_BadStatus's cure score for one status (affected allies times the
 * status weight, plus 3 when he has it and at least two allies do, else 2
 * when he has it), SET_Bad_Priority's cure target (the first of characters
 * 1, 4, 8, 2, 7, 3, 6 with the status, through 0x207B) and the gate of
 * ACT_EndBattle_GATK/ACT_1more_ATK/ACT_1more_MGK (attack only while he is
 * free of status 1).
 *
 * What is checked: wherever Yu is in the lineup (alive, low, ailing, dead)
 * and for any other id, the enemy side or a battle without a leader action,
 * every answer and every parameter read equals retail. In a lineup without Yu
 * whose first fighter holds the leader role, the leader's own HP reaches
 * local 18 and SET_Heal targets it when it is lowest, and its ailments carry
 * the protagonist's score bonus, cure priority and attack gate; retail's
 * answers there leave it unranked. A dead leader answers as a dead Yu does. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;

typedef struct DatUnit {
    u32 hp;
    u32 maxHp;
    u32 bad;           /* bad-status bits; 0x80000 is KO */
} DatUnit;

typedef struct BtlUnit {
    u8 genus;          /* 0xA2: 0 player side, 1 enemy side */
    u16 charId;        /* 0xA4 */
    DatUnit *datUnit;
} BtlUnit;

typedef struct BtlAction {
    u64 uid;           /* 0x00 */
    u32 id;            /* 0x08 */
    u16 unk_1a;        /* 0x1A: 1 and 8 while the action is active */
    BtlUnit *unit;
    struct BtlAction *next;
} BtlAction;

typedef struct BtlBattleState {
    u8 *actions;
    u8 *selectedAction;  /* 0x170 */
} BtlBattleState;

static int failures;

#define CHECK(cond)                                                           \
    do {                                                                      \
        if (!(cond)) {                                                        \
            fprintf(stderr, "%s:%d: failed: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                       \
        }                                                                     \
    } while (0)

/* The script interpreter: parameters, the published value, and which
 * parameters a command read (retail reads N only for a found member). */
static s32 parameters[2];
static int parameter_reads[2];
static s32 published;
static u8 *ai_state;

s32 scrGetParamIntS32(s32 index)
{
    parameter_reads[index]++;
    return parameters[index];
}
s32 scrGetParamInt(s32 index)
{
    parameter_reads[index]++;
    return parameters[index];
}
void scrSetReturnInt(s32 value)
{
    published = value;
}
u8 *scrGetAiState(void)
{
    return ai_state;
}

static BtlBattleState battle;
u8 *iGpffffb3ac = (u8 *)&battle;

static int is_dead(const BtlUnit *unit)
{
    return unit->datUnit->hp == 0;
}

u8 *btlEffectFindUnitByIdAndStatus(s32 unitId, s32 sideMask, s32 statusMask)
{
    BtlAction *action;

    for (action = (BtlAction *)battle.actions; action != NULL; action = action->next) {
        if ((action->unk_1a & 1) == 0 || (action->unk_1a & 8) == 0)
            continue;
        if (((u16)sideMask & (1 << action->unit->genus)) == 0)
            continue;
        if ((statusMask & 0x80000) != 0 && is_dead(action->unit))
            continue;
        if ((action->unit->datUnit->bad & (u32)statusMask) != 0)
            continue;
        if (action->unit->charId == (u16)unitId)
            return (u8 *)action;
    }
    return NULL;
}

static s32 cond_my_hp(u8 *action, u32 value)
{
    DatUnit *unit = ((BtlAction *)action)->unit->datUnit;
    return unit->hp * 100 <= (unit->maxHp & 0xFFFF) * value;
}
static s32 cond_my_bad(u8 *action, u32 mask)
{
    return (((BtlAction *)action)->unit->datUnit->bad & mask) != 0;
}
void *D_00609850[36];

#include "001e6d20.c"
#include "001e1530.c"
#include "001e1800.c"

/* Decomp FUN_001E1530, the retail body. */
static u32 retail_side_id_query_my_hp(void)
{
    u8 *unit;
    u16 id;
    u8 *context;
    u32 selector;
    u32 dispatchType;
    u32 payload;
    s32 result;
    s32 dispatchResult;

    unit = scrGetAiState();
    id = scrGetParamInt(0) & 0xFFFF;
    context = (u8 *)btlEffectFindUnitByIdAndStatus(id, 1 << (((BtlAction *)unit))->unit->genus & 0xffff, 0x80000);
    if (context != 0) {
        selector = (scrGetParamInt(1) & 0xffffff) | 0x01000000;
        dispatchType = (selector & 0xff000000) >> 24;
        payload = selector & 0xffffff;
        if (dispatchType == 0) {
            dispatchResult = 0;
        } else {
            dispatchResult = ((s32 (*)(u8 *, u32))D_00609850[dispatchType * 3])(context, payload);
        }
        result = dispatchResult;
    } else {
        result = 0;
    }
    scrSetReturnInt(result != 0);
    return 1;
}

/* Decomp FUN_001E1800, the retail body. */
static u32 retail_side_id_query_my_bad(void)
{
    u16 id;
    u8 *unit;
    u32 argument;
    u32 sideMask;
    u8 *context;
    u32 dispatchType;
    u32 payload;
    s32 result;
    s32 dispatchResult;

    argument = scrGetParamInt(1);
    unit = scrGetAiState();
    sideMask = 1 << (((BtlAction *)unit))->unit->genus & 0xffff;
    if ((argument & 0x80000) != 0) {
        id = scrGetParamInt(0);
        context = (u8 *)btlEffectFindUnitByIdAndStatus(id, sideMask, 0);
    } else {
        id = scrGetParamInt(0);
        context = (u8 *)btlEffectFindUnitByIdAndStatus(id, sideMask, 0x80000);
    }
    if (context != 0) {
        dispatchType = (((argument & 0xffffff) | 0x0b000000) & 0xff000000) >> 24;
        payload = (argument | 0x0b000000) & 0xffffff;
        if (dispatchType == 0)
            dispatchResult = 0;
        else
            dispatchResult = ((s32 (*)(u8 *, u32))D_00609850[dispatchType * 3])(context, payload);
        result = dispatchResult;
    } else {
        result = 0;
    }
    scrSetReturnInt(result != 0);
    return 1;
}

/* A battle: up to six actions, linked in order. */
#define MAX_UNITS 6
static DatUnit dat[MAX_UNITS];
static BtlUnit units[MAX_UNITS];
static BtlAction actions[MAX_UNITS];
static int unit_count;

static BtlAction *add(u8 genus, u16 charId, u32 hp, u32 maxHp)
{
    int i = unit_count++;

    dat[i].hp = hp;
    dat[i].maxHp = maxHp;
    units[i].genus = genus;
    units[i].charId = charId;
    units[i].datUnit = &dat[i];
    actions[i].uid = 0;
    actions[i].id = 0x100 + (u32)i;
    actions[i].unk_1a = 1 | 8;
    actions[i].unit = &units[i];
    actions[i].next = NULL;
    if (i > 0)
        actions[i - 1].next = &actions[i];
    return &actions[i];
}

static void reset(void)
{
    memset(dat, 0, sizeof(dat));
    memset(units, 0, sizeof(units));
    memset(actions, 0, sizeof(actions));
    unit_count = 0;
    battle.actions = (u8 *)&actions[0];
    battle.selectedAction = NULL;
}

typedef struct Answer {
    s32 value;
    int reads[2];
} Answer;

static Answer run(u32 (*command)(void), BtlAction *caller, s32 id, s32 percent)
{
    Answer answer;

    parameters[0] = id;
    parameters[1] = percent;
    memset(parameter_reads, 0, sizeof(parameter_reads));
    published = 0x7EADBEEF;
    ai_state = (u8 *)caller;
    command();
    answer.value = published;
    answer.reads[0] = parameter_reads[0];
    answer.reads[1] = parameter_reads[1];
    return answer;
}

static u32 override_query(void)
{
    return btlAICommandSideIdQueryMyHp();
}

static s32 query(u32 (*command)(void), BtlAction *caller, s32 id, s32 percent)
{
    return run(command, caller, id, percent).value;
}

/* Every id, both thresholds: the override answers and reads exactly what
 * retail does. */
static void check_retail_equal(BtlAction *caller)
{
    s32 id;
    int t;
    static const s32 thresholds[] = {40, 60, 100};

    for (id = 0; id <= 9; id++) {
        for (t = 0; t < 3; t++) {
            Answer got = run(override_query, caller, id, thresholds[t]);
            Answer want = run(retail_side_id_query_my_hp, caller, id, thresholds[t]);
            CHECK(got.value == want.value);
            CHECK(got.reads[0] == want.reads[0]);
            CHECK(got.reads[1] == want.reads[1]);
        }
    }
}

static s32 pick_kind0(s32 character)
{
    parameters[0] = 0;
    parameters[1] = character;
    btlEffectScriptPickUnitByKind();
    return published;
}

/* 0x209B over the battle's actions. */
static s32 pick_lowest_living_side0(void)
{
    BtlAction *action;
    BtlAction *best = NULL;
    u32 min = 0x0FFFFFFF;

    for (action = (BtlAction *)battle.actions; action != NULL; action = action->next) {
        if ((action->unk_1a & 1) && (action->unk_1a & 8) && action->unit->genus == 0 && !is_dead(action->unit)
            && (action->unit->datUnit->hp & 0xFFFF) < min) {
            best = action;
            min = action->unit->datUnit->hp & 0xFFFF;
        }
    }
    return (s32)best->id | (s32)0x80000000;
}

static BtlAction *resolve(s32 id)
{
    BtlAction *action;

    for (action = (BtlAction *)battle.actions; action != NULL; action = action->next)
        if ((action->id & 0x0FFFFFFF) == ((u32)id & 0x0FFFFFFF))
            return action;
    return NULL;
}

typedef struct Ranking {
    s32 leaderTier;   /* local 18 */
    s32 lowestTier;   /* local 20 */
    s32 healTarget;   /* SET_Heal's local 11 */
    int heals;        /* a pattern's ACT_Heal gate: either tier is 2 */
} Ranking;

static Ranking ally_ranking(u32 (*command)(void), BtlAction *caller)
{
    Ranking ranking;
    s32 lowest;
    s32 leader;

    ranking.leaderTier = query(command, caller, 1, 40) == 1 ? 2 : query(command, caller, 1, 60) == 1 ? 1 : 0;
    ranking.lowestTier = 0;
    lowest = pick_lowest_living_side0();
    leader = pick_kind0(1);
    if (leader != lowest) {
        BtlAction *found = resolve(lowest);
        s32 percent = (s32)(found->unit->datUnit->hp * 100 / found->unit->datUnit->maxHp);

        ranking.lowestTier = percent <= 40 ? 2 : percent <= 60 ? 1 : 0;
    }
    ranking.healTarget = ranking.leaderTier >= ranking.lowestTier ? leader : lowest;
    ranking.heals = ranking.leaderTier == 2 || ranking.lowestTier == 2;
    return ranking;
}

static u32 override_bad(void)
{
    return btlAICommandSideIdQueryMyBad();
}

/* The statuses GET_Point_BadStatus scores, plus KO, which lets the lookup
 * include the dead. */
static const s32 statuses[] = {1, 2, 4, 8, 16, 32, 64, 128, 0x100000, 0x80000};

/* Every id and status: the override answers and reads exactly what retail
 * does. */
static void check_bad_retail_equal(BtlAction *caller)
{
    s32 id;
    unsigned s;

    for (id = 0; id <= 9; id++) {
        for (s = 0; s < sizeof(statuses) / sizeof(statuses[0]); s++) {
            Answer got = run(override_bad, caller, id, statuses[s]);
            Answer want = run(retail_side_id_query_my_bad, caller, id, statuses[s]);
            CHECK(got.value == want.value);
            CHECK(got.reads[0] == want.reads[0]);
            CHECK(got.reads[1] == want.reads[1]);
        }
    }
}

/* 0x20B2 (btlSideSelectUnits with btlCond_MYBAD): living player-side
 * actions with the status. */
static s32 count_own_bad(s32 status)
{
    BtlAction *action;
    s32 count = 0;

    for (action = (BtlAction *)battle.actions; action != NULL; action = action->next)
        if ((action->unk_1a & 1) && (action->unk_1a & 8) && action->unit->genus == 0 && !is_dead(action->unit)
            && (action->unit->datUnit->bad & (u32)status) != 0)
            count++;
    return count;
}

typedef struct Ailment {
    s32 score;        /* GET_Point_BadStatus's local for the status */
    s32 cureTarget;   /* SET_Bad_Priority's local 11, -1 when nobody */
    int attacks;      /* the ACT_EndBattle_GATK/ACT_1more gate */
} Ailment;

static Ailment ailment_ranking(u32 (*command)(void), BtlAction *caller, s32 status, s32 weight)
{
    static const s32 priority[] = {1, 4, 8, 2, 7, 3, 6};
    Ailment ailment;
    s32 count = count_own_bad(status);
    int has = query(command, caller, 1, status) == 1;
    unsigned i;

    ailment.score = count * weight + (count >= 2 && has ? 3 : has ? 2 : 0);
    ailment.cureTarget = -1;
    for (i = 0; i < sizeof(priority) / sizeof(priority[0]); i++) {
        if (query(command, caller, priority[i], status) == 1) {
            ailment.cureTarget = pick_kind0(priority[i]);
            break;
        }
    }
    ailment.attacks = query(command, caller, 1, 1) != 1;
    return ailment;
}

int main(void)
{
    BtlAction *yu;
    BtlAction *yosuke;
    BtlAction *chie;
    BtlAction *yukiko;
    BtlAction *shadow;
    Ranking got;
    Ranking want;

    D_00609850[3] = (void *)cond_my_hp;

    /* Retail lineup, Yu low: his tier comes from his own lookup and he is
     * the heal target, as in retail. */
    reset();
    yu = add(0, 1, 30, 100);
    yosuke = add(0, 2, 80, 100);
    chie = add(0, 3, 90, 100);
    shadow = add(1, 0x10, 500, 500);
    battle.selectedAction = (u8 *)yu;
    check_retail_equal(chie);
    check_retail_equal(shadow);
    got = ally_ranking(override_query, chie);
    want = ally_ranking(retail_side_id_query_my_hp, chie);
    CHECK(memcmp(&got, &want, sizeof(got)) == 0);
    CHECK(got.leaderTier == 2 && got.heals && resolve(got.healTarget) == yu);

    /* Retail lineup, an ally lowest: ranked through local 20, unchanged. */
    dat[0].hp = 70;
    dat[2].hp = 35;
    check_retail_equal(yosuke);
    got = ally_ranking(override_query, yosuke);
    want = ally_ranking(retail_side_id_query_my_hp, yosuke);
    CHECK(memcmp(&got, &want, sizeof(got)) == 0);
    CHECK(got.lowestTier == 2 && resolve(got.healTarget) == chie);

    /* Yu present but dead: retail answers 0 for him and nothing is
     * substituted, though his leader action still exists. */
    dat[0].hp = 0;
    check_retail_equal(chie);
    CHECK(query(override_query, chie, 1, 100) == 0);

    /* Yu inactive (flags cleared): the presence test misses him too, but the
     * leader action is his own, which the retail lookup still rejects. */
    dat[0].hp = 30;
    yu->unk_1a = 0;
    check_retail_equal(chie);
    CHECK(query(override_query, chie, 1, 40) == 0);
    yu->unk_1a = 1 | 8;

    /* A lineup without Yu: Yosuke holds the leader role (001b1d70) and is
     * lowest. Retail's answer leaves him unranked and no heal is chosen;
     * the override ranks his own HP and SET_Heal targets him. */
    reset();
    yosuke = add(0, 2, 30, 100);
    chie = add(0, 3, 90, 100);
    yukiko = add(0, 4, 95, 100);
    shadow = add(1, 0x10, 500, 500);
    battle.selectedAction = (u8 *)yosuke;
    want = ally_ranking(retail_side_id_query_my_hp, chie);
    CHECK(want.leaderTier == 0 && want.lowestTier == 0 && !want.heals);
    got = ally_ranking(override_query, chie);
    CHECK(got.leaderTier == 2 && got.lowestTier == 0 && got.heals);
    CHECK(resolve(got.healTarget) == yosuke);
    got = ally_ranking(override_query, yukiko);
    CHECK(got.leaderTier == 2 && resolve(got.healTarget) == yosuke);
    /* The leader's middle tier. */
    dat[0].hp = 55;
    CHECK(ally_ranking(override_query, chie).leaderTier == 1);
    dat[0].hp = 61;
    CHECK(ally_ranking(override_query, chie).leaderTier == 0);
    /* Only character 1 on the player side is substituted: every other id,
     * and the enemy side asking for character 1, answers as retail. */
    dat[0].hp = 30;
    {
        s32 id;
        for (id = 0; id <= 9; id++) {
            if (id == 1)
                continue;
            CHECK(run(override_query, chie, id, 40).value == run(retail_side_id_query_my_hp, chie, id, 40).value);
        }
    }
    check_retail_equal(shadow);
    /* The substituted answer reads the condition parameter as a found member
     * does in retail. */
    CHECK(run(override_query, chie, 1, 40).reads[1] == 1);

    /* Without Yu, an ally lower than the leader: local 20 ranks the ally and
     * the leader's tier is compared against it, as Yu's would be. */
    dat[0].hp = 50;
    dat[1].hp = 25;
    got = ally_ranking(override_query, yukiko);
    CHECK(got.leaderTier == 1 && got.lowestTier == 2 && resolve(got.healTarget) == chie);
    /* Once the leader drops below that ally it is the lowest: local 20 is
     * skipped and local 18 alone ranks it. */
    dat[0].hp = 20;
    got = ally_ranking(override_query, yukiko);
    CHECK(got.leaderTier == 2 && got.lowestTier == 0 && got.heals && resolve(got.healTarget) == yosuke);

    /* A dead leader answers 0, like a dead Yu, and reads no condition. */
    dat[0].hp = 0;
    CHECK(run(override_query, chie, 1, 100).value == 0);
    CHECK(run(override_query, chie, 1, 100).reads[1] == 0);

    /* No leader action recorded: the retail answer, with no dereference. */
    dat[0].hp = 30;
    battle.selectedAction = NULL;
    check_retail_equal(chie);

    /* Ailments. Retail lineup: Yu and Yukiko have status 8. Yu's own lookup
     * gives the bonus and first cure place, as in retail. */
    {
        Ailment gotBad;
        Ailment wantBad;

        D_00609850[33] = (void *)cond_my_bad;
        reset();
        yu = add(0, 1, 100, 100);
        yosuke = add(0, 2, 100, 100);
        yukiko = add(0, 4, 100, 100);
        chie = add(0, 3, 100, 100);
        shadow = add(1, 0x10, 500, 500);
        battle.selectedAction = (u8 *)yu;
        dat[0].bad = 8;
        dat[2].bad = 8;
        check_bad_retail_equal(chie);
        check_bad_retail_equal(shadow);
        gotBad = ailment_ranking(override_bad, chie, 8, 8);
        wantBad = ailment_ranking(retail_side_id_query_my_bad, chie, 8, 8);
        CHECK(memcmp(&gotBad, &wantBad, sizeof(gotBad)) == 0);
        CHECK(gotBad.score == 2 * 8 + 3 && resolve(gotBad.cureTarget) == yu && gotBad.attacks);
        /* Yu with status 1 stops the attack gate, as in retail. */
        dat[0].bad = 1 | 8;
        check_bad_retail_equal(chie);
        CHECK(!ailment_ranking(override_bad, chie, 8, 8).attacks);
        /* Yu dead (KO): no status for him, but the KO query finds him, both
         * as retail. */
        dat[0].hp = 0;
        dat[0].bad = 0x80000;
        check_bad_retail_equal(chie);
        CHECK(query(override_bad, chie, 1, 8) == 0);
        CHECK(query(override_bad, chie, 1, 0x80000) == 1);

        /* A lineup without Yu: Yosuke holds the leader role and shares
         * status 8 with Yukiko. Retail's answers give him no bonus and cure
         * Yukiko first; the override weighs him as the protagonist. */
        reset();
        yosuke = add(0, 2, 100, 100);
        yukiko = add(0, 4, 100, 100);
        chie = add(0, 3, 100, 100);
        shadow = add(1, 0x10, 500, 500);
        battle.selectedAction = (u8 *)yosuke;
        dat[0].bad = 8;
        dat[1].bad = 8;
        wantBad = ailment_ranking(retail_side_id_query_my_bad, chie, 8, 8);
        CHECK(wantBad.score == 2 * 8 && resolve(wantBad.cureTarget) == yukiko);
        gotBad = ailment_ranking(override_bad, chie, 8, 8);
        CHECK(gotBad.score == 2 * 8 + 3 && resolve(gotBad.cureTarget) == yosuke && gotBad.attacks);
        /* Alone with the status, the smaller bonus. */
        dat[1].bad = 0;
        CHECK(ailment_ranking(override_bad, chie, 8, 8).score == 8 + 2);
        /* The leader with status 1 closes the attack gate retail leaves open. */
        dat[0].bad = 1;
        CHECK(ailment_ranking(retail_side_id_query_my_bad, chie, 1, 9).attacks);
        CHECK(!ailment_ranking(override_bad, chie, 1, 9).attacks);
        /* Only character 1 on the player side is substituted: the leader's
         * own id and every other id, and the enemy side, answer as retail. */
        {
            s32 id;
            unsigned s;
            for (id = 0; id <= 9; id++) {
                if (id == 1)
                    continue;
                for (s = 0; s < sizeof(statuses) / sizeof(statuses[0]); s++)
                    CHECK(run(override_bad, chie, id, statuses[s]).value
                          == run(retail_side_id_query_my_bad, chie, id, statuses[s]).value);
            }
        }
        check_bad_retail_equal(shadow);
        /* A dead leader answers as a dead Yu: no status, but KO. */
        dat[0].hp = 0;
        dat[0].bad = 0x80000;
        CHECK(query(override_bad, chie, 1, 1) == 0);
        CHECK(query(override_bad, chie, 1, 0x80000) == 1);
        /* No leader action recorded: the retail answer, with no dereference. */
        battle.selectedAction = NULL;
        check_bad_retail_equal(chie);
    }

    if (failures) {
        fprintf(stderr, "battle_ai_leader_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("battle_ai_leader_test: ok\n");
    return 0;
}
