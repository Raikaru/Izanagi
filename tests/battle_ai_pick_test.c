/* Host checks for override 001e6d20 (battle script command 0x207B,
 * btlEffectScriptPickUnitByKind). The override body is included as it is
 * spliced into the decomp mirror; the integer types, the two battle records it
 * reads (BtlAction.id at 0x08, BtlBattleState.selectedAction at 0x170) and the
 * script callees are defined here.
 *
 * What is checked: the retail lookup (side mask and status mask per kind, and
 * the published id with the retail high bit) is unchanged whenever the
 * character is in the battle, the protagonist's failed kind-0 lookup answers
 * with the leader action the battle recorded, no other failed lookup is
 * substituted, and no failed lookup dereferences NULL. The ally AI's SET_Heal
 * hands the protagonist's published id to GetHp/GetMaxHp, which resolve it the
 * way retail btlActionFindById does (id & 0x0FFFFFFF over the battle's
 * actions); the substituted id must resolve there, where -1 would not. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t s32;

typedef struct BtlAction {
    u64 uid;  /* 0x00 */
    u32 id;   /* 0x08 */
} BtlAction;

typedef struct BtlBattleState {
    u8 pad00[0x170];
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

/* The script interpreter seams the spliced unit calls. */
static s32 parameters[2];
static s32 published;
static int publish_count;
static u8 *lookup_result;
static s32 lookup_id, lookup_side_mask, lookup_status_mask;
static int lookup_count;

s32 scrGetParamIntS32(s32 index)
{
    return parameters[index];
}
void scrSetReturnInt(s32 value)
{
    published = value;
    publish_count++;
}
u8 *btlEffectFindUnitByIdAndStatus(s32 unitId, s32 sideMask, s32 statusMask)
{
    lookup_count++;
    lookup_id = unitId;
    lookup_side_mask = sideMask;
    lookup_status_mask = statusMask;
    return lookup_result;
}

u8 *iGpffffb3ac;

#include "001e6d20.c"

static BtlBattleState battle;
static BtlAction leader = {0, 0x1234};
static BtlAction member = {0, 0x5678};

static s32 pick(s32 kind, s32 character, u8 *found)
{
    parameters[0] = kind;
    parameters[1] = character;
    lookup_result = found;
    lookup_count = publish_count = 0;
    published = 0;
    lookup_id = lookup_side_mask = lookup_status_mask = -1;
    btlEffectScriptPickUnitByKind();
    CHECK(publish_count == 1);
    return published;
}

/* GetHp/GetMaxHp (0x20E7/0x20E6): retail btlActionFindById over the actions
 * of this battle, with the commands' 0x0FFFFFFF mask. */
static BtlAction *resolve(s32 id)
{
    BtlAction *actions[2] = {&leader, &member};
    int i;

    for (i = 0; i < 2; i++) {
        if ((s32)actions[i]->id == (id & 0x0FFFFFFF))
            return actions[i];
    }
    return NULL;
}

int main(void)
{
    iGpffffb3ac = (u8 *)&battle;
    battle.selectedAction = (u8 *)&leader;

    /* Retail: the player side is searched with no status filter, so a dead or
     * downed member still matches; the enemy side skips the dead. */
    CHECK(pick(0, 1, (u8 *)&member) == (s32)(0x5678 | 0x80000000));
    CHECK(lookup_count == 1 && lookup_id == 1 && lookup_side_mask == 1 && lookup_status_mask == 0);
    CHECK(pick(1, 0x10C, (u8 *)&member) == (s32)(0x5678 | 0x80000000));
    CHECK(lookup_count == 1 && lookup_id == 0x10C && lookup_side_mask == 2 &&
          lookup_status_mask == 0x80000);
    /* Only kinds 0 and 1 search; retail leaves any other kind unresolved. */
    CHECK(pick(2, 1, (u8 *)&member) == -1);
    CHECK(lookup_count == 0);

    /* The protagonist's absent action answers with the action that carries his
     * leader role; the lookup itself is still the retail one. */
    CHECK(pick(0, 1, NULL) == (s32)(0x1234 | 0x80000000));
    CHECK(lookup_count == 1 && lookup_id == 1 && lookup_side_mask == 1 && lookup_status_mask == 0);
    /* The id SET_Heal then hands to GetHp resolves to the leader's action;
     * the "no unit" value would resolve to nothing and fault there. */
    CHECK(resolve(pick(0, 1, NULL)) == &leader);
    CHECK(resolve(-1) == NULL);
    /* A lineup that still holds Yu keeps the found action. */
    CHECK(pick(0, 1, (u8 *)&member) == (s32)(0x5678 | 0x80000000));

    /* No other request is substituted: another absent party member, the enemy
     * side and an unknown kind all publish the "no unit" value. */
    CHECK(pick(0, 4, NULL) == -1);
    CHECK(pick(1, 1, NULL) == -1);
    CHECK(pick(2, 1, NULL) == -1);

    /* A projected lineup whose leader role was never handed on (no state-5
     * action) must still not fault. */
    battle.selectedAction = NULL;
    CHECK(pick(0, 1, NULL) == -1);
    battle.selectedAction = (u8 *)&leader;

    /* The published id is the action's own id with the retail high bit, for
     * both the found and the substituted action. */
    leader.id = 0x0FFFFFFF;
    CHECK(pick(0, 1, NULL) == (s32)(0x0FFFFFFF | 0x80000000));
    member.id = 0;
    CHECK(pick(0, 1, (u8 *)&member) == (s32)0x80000000);

    if (failures) {
        fprintf(stderr, "battle_ai_pick_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("battle_ai_pick_test: ok\n");
    return 0;
}
