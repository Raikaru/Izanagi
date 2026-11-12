/* Exercise the actual 001a06d0 turn-start override with retail-shaped gates.
 * No game state is loaded. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef struct BtlUnit { u32 flags3; u8 genus; u16 charId; void *datUnit; } BtlUnit;
typedef struct BtlAction {
    BtlUnit *unit; struct BtlAction *next; u32 flags; u16 unk_1a;
    s32 unk_28, unk_29, unk_430, unk_43c, unk_41c; void *unk_440;
    int targetBlock; u16 state;
} BtlAction;
typedef struct BtlBattleState {
    u8 *actions, *selectedAction, *uiTask; s32 unkA70; u32 unkA72;
    u32 flags, battleFlags, protagonistTurns;
} BtlBattleState;
static BtlBattleState battle;
u8 *D_0076449C = (u8 *)&battle, *iGpffffb3ac = (u8 *)&battle;
static int guard, skill_mask, window_open, bad_status, dead, delayed;
static int failures;
static BtlUnit *known_units[4];
static int known_unit(void *unit) {
    for (unsigned i = 0; i < 4; i++) if (unit == known_units[i]) return 1;
    return 0;
}
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); failures++; } } while (0)
void btlActionSetState(BtlAction *a, u16 state) { a->state = state; }
void btlActionSetStateWithDelay(BtlAction *a, u16 state, u16 delay) { (void)delay; delayed++; a->state = state; }
u32 datCalcIsDead(void *unit, s32 hp) { CHECK(known_unit(unit)); (void)hp; return dead; }
void btlUnitSelectRecordTable(u8 *a) { (void)a; }
void btlTargetClearWork(void *p) { (void)p; }
s32 btlStatusWindowShowSlot2(u8 *a) { (void)a; return window_open; }
s32 btlTaskSlotLinkedSlot2(void) { return 0; }
s32 btlSideCountNotBad(s32 side, s32 status) { (void)side; (void)status; return 1; }
s16 btlCameraSelectFormationIndex(void) { return 0; }
u32 datCalcChkBadStatus(void *unit, u32 status) { CHECK(known_unit(unit)); (void)status; return bad_status; }
s32 btlTargetCanControlDamagePhase(void) { return 1; }
void btlTargetEndDamagePhase(void) {}
void btlDamageStart(u8 *task, u8 *a) { (void)task; (void)a; }
u8 *func_001f99c0(u8 *a, u16 code, s32 x, s32 y, s32 z) { (void)a; (void)code; (void)x; (void)y; (void)z; return NULL; }
s64 btlPacketEnqueue(u8 *p, u32 n) { (void)p; (void)n; return 0; }
s32 datCalcGetSkillMask(u8 *p) { (void)p; return skill_mask; }
s32 datCalcIsGuarding(u16 *p) { (void)p; return guard; }
s64 datCalcFindStrongBuff(u8 *p) { (void)p; return -1; }
s32 btlSoundRollStatusCure(u8 *a) { (void)a; return 0; }
u8 *btlEffectFindUnitByIdAndStatus(s32 id, s32 side, s32 status) {
    BtlAction *a;
    (void)status;
    for (a = (BtlAction *)battle.actions; a; a = a->next)
        if ((a->unk_1a & 9) == 9 && ((1 << a->unit->genus) & side) && a->unit->charId == id)
            return (u8 *)a;
    return NULL;
}
#include "001a06d0.c"

static void init(BtlAction *a, BtlUnit *u, u8 side, u16 id) {
    memset(a, 0, sizeof(*a)); memset(u, 0, sizeof(*u));
    u->genus = side; u->charId = id; u->datUnit = u;
    a->unit = u; a->unk_1a = 9;
}
static void reset(void) {
    memset(&battle, 0, sizeof(battle)); battle.unkA70 = -1;
    guard = skill_mask = window_open = bad_status = dead = delayed = 0;
}
static void turn(BtlAction *a) { btlActionStartInit(a); }
int main(void) {
    BtlAction yu, yosuke, chie, enemy;
    BtlUnit uy, uo, uc, ue;
    reset(); init(&yu, &uy, 0, 1); init(&yosuke, &uo, 0, 2);
    init(&chie, &uc, 0, 3); init(&enemy, &ue, 1, 1);
    known_units[0] = &uy; known_units[1] = &uo; known_units[2] = &uc; known_units[3] = &ue;
    yu.next = &yosuke; yosuke.next = &chie; chie.next = &enemy;
    battle.actions = (u8 *)&yu; battle.selectedAction = (u8 *)&yu;
    turn(&yosuke); turn(&enemy); CHECK(battle.protagonistTurns == 0);
    turn(&yu); turn(&yu); CHECK(battle.protagonistTurns == 2);
    /* Even if a caller points selectedAction at an ally while Yu is active,
     * the retail character-1 count is unchanged. */
    battle.selectedAction = (u8 *)&yosuke; turn(&yosuke); turn(&yu);
    CHECK(battle.protagonistTurns == 3);
    /* New battle and formation: only the first projected fighter counts. */
    reset(); battle.actions = (u8 *)&yosuke; battle.selectedAction = (u8 *)&yosuke;
    turn(&chie); turn(&enemy); CHECK(battle.protagonistTurns == 0);
    turn(&yosuke); turn(&yosuke); CHECK(battle.protagonistTurns == 2);
    /* Retail's extra-turn and blocked-turn gates must not advance the count. */
    yosuke.unk_28 = 1; turn(&yosuke); yosuke.unk_28 = 0;
    yosuke.unk_29 = 1; turn(&yosuke); yosuke.unk_29 = 0;
    guard = 1; turn(&yosuke); guard = 0;
    skill_mask = 1; turn(&yosuke); skill_mask = 0;
    battle.flags |= 0x80000; turn(&yosuke); battle.flags &= ~0x80000;
    window_open = 1; turn(&yosuke); window_open = 0;
    uo.flags3 = 0x10; turn(&yosuke); uo.flags3 = 0;
    uc.flags3 = 0x10; turn(&yosuke); uc.flags3 = 0;
    yosuke.unk_1a = 0; turn(&yosuke); yosuke.unk_1a = 9;
    battle.unkA70 = 1; battle.unkA72 = 2; turn(&yosuke);
    CHECK(delayed == 1); battle.unkA70 = -1;
    CHECK(battle.protagonistTurns == 2);
    /* A changed selected action is observed immediately, not cached. */
    battle.selectedAction = (u8 *)&chie;
    turn(&yosuke); turn(&chie); CHECK(battle.protagonistTurns == 3);
    battle.selectedAction = NULL; turn(&chie); CHECK(battle.protagonistTurns == 3);
    /* A different battle selects a different leader; no cached roster state. */
    reset(); battle.actions = (u8 *)&chie; battle.selectedAction = (u8 *)&chie;
    turn(&yosuke); turn(&chie); turn(&chie); turn(&chie); turn(&chie); turn(&chie); turn(&chie);
    CHECK(battle.protagonistTurns == 6);
    reset(); battle.actions = (u8 *)&yu; battle.selectedAction = (u8 *)&yu;
    turn(&yu); CHECK(battle.protagonistTurns == 1);
    return failures != 0;
}
