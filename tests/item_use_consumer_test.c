/* Function bodies come from the production mirror. History-call pointer casts
 * are adapted for a 64-bit host and the stubs ignore their pointer arguments;
 * synthetic layouts and accessors test control flow, not the guest ABI. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/game_items_native.h"
#include "p4pc/table_result.h"
#include "p4pc/game_elements_ai_script_body.inc"
/* Give the real decode adapter an opaque synthetic VM work pointer. */
#undef P4_AI_SCRIPT_WORK
#define P4_AI_SCRIPT_WORK() ((void *)&scriptVm)
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16; typedef int32_t s32; typedef uint32_t u32; typedef int64_t s64;
typedef struct { s16 item, count; } RadarItemEntry;
typedef struct {
    u8 pad0[0x22]; s16 unk22, unk24, unk26; u8 pad28[2]; s16 unk2A;
    u8 pad2C[8]; s16 recipients[4], unk3C; RadarItemEntry entries[768];
    s16 itemCount; u8 padC40[4]; s32 unkC44;
} RadarItemMenuWork;
typedef struct { s16 item; } BtlPanelActionChoice;
typedef struct { u16 flags, id, id2; u8 rest[42]; } DatUnit;
typedef struct { u8 genus; void *datUnit; int hp, sp; u32 flags3, status; u16 charId; } BtlUnit;
typedef struct {
    union { u8 pad[0x30]; struct { u8 padFlags[0x18]; u16 flags; }; };
    union { BtlUnit *unit; u8 unit_slot[8]; }; u8 unk_38[0x34]; u16 unk_6c, unk_6e;
    union { BtlPanelActionChoice *unk_70; struct { u16 item; } parameters; };
    union { u8 unk_e8[8]; struct { s16 messageCode; } message; } targetBlock;
} BtlAction;
typedef struct {
    u8 pad[0x34]; u16 kind, command, skill; u8 pad3a[4]; u16 plan;
} BtlAiCommandWork;
_Static_assert(offsetof(BtlAction, unk_38) == 0x38, "synthetic AI work offset");
_Static_assert(offsetof(BtlAiCommandWork, kind) == 0x34, "synthetic AI kind offset");
typedef struct { BtlAction *action; s32 slideFrame, flags; } BtlPanelWork;
typedef struct { s16 selected; } BtlSelection;
typedef struct { u32 flags; void *uiTask; } BtlBattleState;
static BtlBattleState battle;
static void *D_0076449C = &battle;
static u8 *iGpffffb3ac = (u8 *)&battle;
static P4GameItemInventoryRow inventory[4];
static P4GameItemBattleRow battleItems[4];
static struct ItemEntry { s32 effect; u32 kind; } itemEntry;
static int input, stock, applications, flagWrites, sounds, resets, releases, retracts, randomizes;
static int fieldHp = 10, fieldSp = 20;
static int metadataReads, peekReads, kindReads, categoryCalls, categoryResult, collects;
static int opened, anim, seenSource, seenItem, seenEffect, denySource = -1;
static BtlAction *scriptAction;
static BtlAction *predictionTarget;
static s32 scriptReturn;
typedef struct { u8 pad[4], unknown04[36]; } DatSkillEntry;
static DatSkillEntry skillEntries[2];
static void *iGpffffb3b8 = skillEntries;
static s32 scriptParam;
static int underlyingCalls;
static int scriptVm, metadataEnabled, metadataBegins, metadataEnds, metadataDecodes, metadataVmDecodes;
static uint32_t metadataKind, metadataParameter;
static void *metadataWork;
int p4_game_elements_ai_native_enabled(void) { return metadataEnabled; }
_Noreturn void p4_game_elements_ai_native_fail(void) { abort(); }
int p4_game_elements_ai_native_decoded(void *source, uint32_t kind, uint32_t command) {
    assert(source == scriptAction); (void)command; ++metadataDecodes; metadataKind = kind; return 0;
}
int p4_game_elements_ai_vm_decoded(void *work, void *source, uint32_t parameter) {
    assert(work == &scriptVm && source == scriptAction); ++metadataVmDecodes; metadataParameter = parameter; return 0;
}
int p4_game_elements_ai_native_decode_begin(void *source, void *work, uint32_t parameter) {
    assert(source == scriptAction && work == &scriptVm); ++metadataBegins; metadataWork = work; metadataParameter = parameter; return 0;
}
int p4_game_elements_ai_native_decode_work(void *source, uint32_t *work, uint32_t *parameter) {
    assert(source == scriptAction); *work = metadataWork ? 1u : 0u; *parameter = metadataParameter; return 0;
}
int p4_game_elements_ai_native_decode_end(void *source) { assert(source == scriptAction); ++metadataEnds; return 0; }
u8 D_005ED9D0[1];
_Noreturn void p4_mod_failure(size_t owner, const char *reason) { (void)owner; (void)reason; abort(); }
static int decision(const P4ModEvent *ctx, char *reason, size_t cap, void *user) {
    (void)reason; (void)cap; (void)user;
    assert(ctx->count == 3);
    seenSource = (int)ctx->fields[0].value; seenItem = (int)ctx->fields[1].value; seenEffect = (int)ctx->fields[2].value;
    return seenSource != denySource;
}
P4GameItemInventoryRow *p4_game_tables_items_native_inventory_rows(void *p) { (void)p; return inventory; }
P4GameItemBattleRow *p4_game_tables_items_native_battle_rows(u8 *p) { (void)p; return battleItems; }
static struct ItemEntry *datEntryAt(s16 id) { (void)id; ++metadataReads; return &itemEntry; }
uint32_t p4_game_tables_items_native_peek_effect(uint32_t id) { (void)id; ++peekReads; return (uint32_t)itemEntry.effect; }
uint32_t p4_game_tables_items_native_peek_kind(uint32_t id) { (void)id; ++kindReads; return itemEntry.kind; }
int p4_game_tables_items_native_action(uint32_t id) { (void)id; ++categoryCalls; if (categoryResult == P4_ITEM_ACTION_HANDLED) { --stock; ++fieldHp; ++fieldSp; } return categoryResult; }
_Noreturn void p4_game_tables_items_native_refuse(int result) { (void)result; abort(); }
static s32 radarEntryTypeIsActive(s16 id) { (void)id; return 0; }
static u16 datGetSkillPropertyByte(s32 id) { (void)id; return 0; }
static s32 titleSetMenuLayout(u8 *work, u32 state) { (void)work; (void)state; return 0; }
static void radarPushSlotState(u8 *work) { (void)work; }
static s32 nLineSelectSet(s32 state) { (void)state; return 0; }
static s32 datItemCount(s16 id) { (void)id; return stock; }
static void datItemSetCount(s16 id, s32 value) { (void)id; stock = value; }
static s32 radarSkillEffectMissing(s16 a, s16 b, s32 c) { (void)a; (void)b; (void)c; return 1; }
static void datCalcApplySkillEffectById(s16 a, s16 b, u32 c, u32 d) { (void)a; (void)b; (void)c; (void)d; ++applications; ++fieldHp; ++fieldSp; }
static s32 FldStepNotAtBoundaryQuery(void) { return 1; }
static void datSetFlag(s32 a, s32 b) { (void)a; (void)b; ++flagWrites; }
static void radarRandomizeEntries(u8 *p) { (void)p; ++randomizes; }
void titleCollectFlaggedIds(u8 *p) { (void)p; ++collects; }
static s32 radarPushHistoryValue(s32 p, s32 a, s32 b) { (void)p; (void)a; (void)b; return 0; }
static void kAssertFail(void *p, s32 n) { (void)p; (void)n; abort(); }
static void sdkSndStartStream(s32 a, s32 b, s32 c, s32 d) { (void)a; (void)b; (void)c; (void)d; ++sounds; }
static u16 btlSelectionReadInput(u8 *p) { (void)p; return (u16)input; }
static u16 datCalcUnderlyingSkillId(s16 item) { ++underlyingCalls; return (u16)(item + 1); }
static u8 *scrGetAiState(void) { return (u8 *)scriptAction; }
static s32 scrGetParamInt(s32 index) { assert(index == 0); return scriptParam; }
static u8 *btlActionFindById(s32 id) { (void)id; return (u8 *)predictionTarget; }
static void scrSetReturnInt(s32 value) { scriptReturn = value; }
static s32 datCalcGetHp(uintptr_t unit) { (void)unit; return 50; }
static s32 datCalcMaxHp(DatUnit *unit) { (void)unit; return 100; }
static int p4_game_skill_active(u16 skill) { return skill < 0x1b8; }
static s32 func_0043c6a0(s32 value) { return value < 0 ? -value : value; }
static s32 datCalcSkillAmount(s32 id, u8 *source, u8 *target, int a, int b, int c, int d, int e) {
    (void)id; (void)a; (void)b; (void)c; (void)d; (void)e;
    u32 value = 0x800000; memcpy(source + 0x18, &value, 4); memcpy(target + 0x18, &value, 4);
    return -1;
}
static void datCalcClearField18(uintptr_t unit) { u32 zero = 0; memcpy((u8 *)unit + 0x18, &zero, 4); }
static void datCalcClearBuffLevel(uintptr_t address, u32 index) {
    u8 *unit = (u8 *)address;
    u32 flags; memcpy(&flags, unit + 0x14, sizeof flags);
    flags &= ~(1u << index); memcpy(unit + 0x14, &flags, sizeof flags);
    unsigned nibble = index & 1 ? 0x0f : 0xf0;
    unit[0x1c + index / 2] &= (u8)nibble;
    unit[0x24 + index / 2] &= (u8)nibble;
}
static s32 datCalcDefaultSkillId(u8 *unit) { (void)unit; return 0; }
static s32 datCalcPickSpecialItem(u8 *unit) { (void)unit; return 0; }
static void btlAiChooseUsableAttackSkill(u8 *action, u8 *work) { (void)action; (void)work; }
static void nmMesWindowStepCurrentRelease(void) {}
static s32 nmMesWindowCurrentOpenState(void) { return opened; }
static s32 nmMesWindowCurrentAnimFlag(void) { return anim; }
static void btlEncounterEntryClearRetracted(void *p) { (void)p; ++retracts; }
static void nmMesWindowReleaseCurrent(void) { ++releases; }
static void nmMesWindowResetCurrent(void) { ++resets; }
/* The production mirror's optional entry hooks are disabled in this fixture. */
/* Wait packets are retained as ordered synthetic records; the native wait
 * body is extracted from the production mirror, not reimplemented here. */
typedef struct { s64 uid, actionUid, dependencyUid; int dependencyKind, kind; } BtlPacketTiming;
typedef struct { u32 flags; } LargeBattleHitResult;
#define MEMBER(type, ptr, record, member) (((record *)(ptr))->member)
static BtlPacketTiming packets[32];
static int packetCount, waitState;
static BtlPacketTiming *newPacket(int kind) {
    assert(packetCount < 32);
    BtlPacketTiming *p = &packets[packetCount];
    memset(p, 0, sizeof *p); p->uid = packetCount + 1; p->kind = kind; ++packetCount; return p;
}
static void btlActionFinishPresentation(BtlAction *p) { (void)p; }
static void *btlUnitCreateLookAtDeactivatePacket(int a, int b) { (void)a; (void)b; return newPacket(1); }
static void *btlPacketCreateEffectCounter(s32 a, s16 b) { (void)a; (void)b; return newPacket(2); }
static u8 *btlPacketCreateSpriteTable(u32 a, u16 b) { (void)a; (void)b; return (u8 *)newPacket(3); }
static u8 *func_001f99c0(u8 *a, int b, int c, int d, int e) { (void)a; (void)b; (void)c; (void)d; (void)e; return (u8 *)newPacket(4); }
static void *btlCameraCreateSetStatePacket(BtlAction *a, int b) { (void)a; (void)b; return newPacket(5); }
static s32 btlUnitGetMotionState(u8 *p) { (void)p; return 0; }
static void *btlUnitCreateMotionSettlePacket(BtlUnit *p) { (void)p; return newPacket(6); }
static u8 *btlPacketCreateImmediate(void) { return (u8 *)newPacket(7); }
static void btlTargetClearResultBuffer(u8 *p) { memset(p, 0, sizeof(LargeBattleHitResult)); }
static void *btlTargetCreateDataPacket(s32 a, s32 b, void *r, int c, int d) {
    (void)a; (void)b; (void)c; (void)d; assert(((LargeBattleHitResult *)r)->flags == 0x80); return newPacket(8);
}
static void *btlUnitCreateAnimPacket(BtlUnit *a, int b, int c, float d, int e) { (void)a; (void)b; (void)c; (void)d; (void)e; return newPacket(9); }
static void *btlTargetCreateActionPacket(u8 *a, int b) { (void)a; (void)b; return newPacket(10); }
static void btlPacketEnqueue(void *p, int queue) { ((BtlPacketTiming *)p)->kind |= queue << 8; }
static int btlSoundCanReceiveStatus(u8 *a) { (void)a; return 0; }
static void btlActionSetState(BtlAction *a, int state) { (void)a; waitState = state; }
static int btlEffectHasAiProcedure(u8 *p) { (void)p; return 1; }
static int btlAICommandTryReleaseProc(u8 *p) { (void)p; return 1; }
static void btlAICommandSetUnitAction(u8 *a, u8 *b) { (void)a; (void)b; abort(); }
static unsigned p4_battle_decide_interest;
static void p4_battle_decide_ready(u32 a) { (void)a; }
#undef P4_H2G
#define P4_H2G(p) ((u32)0)
/* The decode_work stub hands out the handle 1 for the synthetic VM work; resolve exactly that handle on 32- and 64-bit hosts. */
#undef P4_G2H
#define P4_G2H(a) ((uint32_t)(a) == 1u ? (void *)&scriptVm : (void *)0)
P4ModEntry p4_mod_entry_001a2b50;
void p4_mod_guard_001a2b50(void *p) { (void)p; abort(); }
P4ModEntry p4_mod_entry_001acf50;
void p4_mod_guard_001acf50(void *p) { (void)p; abort(); }
P4ModEntry p4_mod_entry_0013c700, p4_mod_entry_0013c5a0, p4_mod_entry_002091f0, p4_mod_entry_001b3fb0;
P4ModEntry p4_mod_entry_001de640, p4_mod_entry_001df040;
P4ModEntry p4_mod_entry_001e5d90, p4_mod_entry_001e5e40, p4_mod_entry_001e5ec0;
P4ModEntry p4_mod_entry_001dd1c0, p4_mod_entry_001dd3a0, p4_mod_entry_001e6740;
int32_t p4_mod_guard_001dd1c0(void *a, void *b, void *c, uint16_t d, uint32_t e) {
    (void)a; (void)b; (void)c; (void)d; (void)e; abort();
}
int32_t p4_mod_guard_001dd3a0(void *a, void *b, void *c, uint16_t d, int32_t e) {
    (void)a; (void)b; (void)c; (void)d; (void)e; abort();
}
int32_t p4_mod_guard_001e6740(void) { abort(); }
int32_t p4_mod_guard_001e5d90(void) { abort(); }
int32_t p4_mod_guard_001e5e40(void) { abort(); }
int32_t p4_mod_guard_001e5ec0(void) { abort(); }
int32_t p4_mod_guard_0013c700(int16_t a, int16_t b, void *p) { (void)a; (void)b; (void)p; abort(); }
uint16_t p4_mod_guard_002091f0(void *a, void *b) { (void)a; (void)b; abort(); }
int32_t p4_mod_guard_001b3fb0(void) { abort(); }
void p4_mod_guard_0013c5a0(void *a, void *b) { (void)a; (void)b; abort(); }
void p4_mod_guard_001de640(void *action, void *work, uint16_t code) { (void)action; (void)work; (void)code; abort(); }
int32_t p4_mod_guard_001df040(void) { abort(); }
/* The mirrored AI bodies call the Golden native wait hooks (src/platform/golden/golden_ai_native.c); this consumer test runs no Golden AI. */
#include "p4pc/golden_ai_native.h"
#include "p4pc/game_mode.h"
#include "p4pc/golden_items_native.h"
uint16_t p4_golden_ai_native_wait_kind(void *action, uint16_t kind) { (void)action; return kind; }
void p4_golden_ai_native_message_wait(void *action) { (void)action; }
/* ... and route Golden items; this test runs the PS2 mode only. */
P4GameMode p4_game_mode(void) { return P4_GAME_MODE_PS2; }
int32_t p4_golden_item_native_route(uint32_t item) { (void)item; return P4_GOLDEN_ITEM_UNAVAILABLE; }
_Noreturn void p4_golden_item_native_return_pending(void) { abort(); }
#include "item_use_generated.inc"
int main(void) {
    P4ModAuthorizer *auth = p4_mod_authorizer_add(1, "item_use", decision, NULL);
    assert(auth);
    RadarItemMenuWork work = {0}; work.unk3C = 2; work.recipients[0] = 1; work.recipients[1] = 2;
    inventory[0].id = 0x301; inventory[0].quantity = 3; stock = 3; itemEntry.effect = 12;
    denySource = 0;
    assert(radarApplyItemStep(1, -1, (u8 *)&work) == 0);
    assert(seenSource == 0 && seenItem == 0x301 && seenEffect == 12 && !applications && stock == 3 && !flagWrites && !randomizes);
    assert(radarApplyItemStep(1, 1, (u8 *)&work) == 0 && !applications && stock == 3);
    itemEntry.effect = 246;
    assert(radarApplyItemStep(1, -1, (u8 *)&work) == 0 && !flagWrites && stock == 3);
    assert(fieldHp == 10 && fieldSp == 20 && inventory[0].quantity == 3);
    denySource = -1; itemEntry.effect = 12;
    assert(radarApplyItemStep(1, -1, (u8 *)&work) == 1 && applications == 2 && stock == 2 && inventory[0].quantity == 2);
    assert(fieldHp == 12 && fieldSp == 22);
    inventory[0].id = 0x600; inventory[0].quantity = 2; stock = 2;
    metadataReads = peekReads = applications = 0; denySource = 0;
    assert(radarApplyItemStep(1, -1, (u8 *)&work) == 0);
    assert(peekReads == 1 && metadataReads == 0 && applications == 0 && stock == 2);
    assert(fieldHp == 12 && fieldSp == 22 && inventory[0].quantity == 2);
    denySource = -1;
    assert(radarApplyItemStep(1, -1, (u8 *)&work) == 1);
    assert(peekReads == 2 && metadataReads == 1 && applications == 2 && stock == 1);
    inventory[0].id = 0x301; stock = 2;
    BtlUnit unit = {0}; BtlPanelActionChoice choice = {0}; BtlAction action = {.unk_70 = &choice, .unit = &unit};
    BtlPanelWork panel = {&action, 9, 0}; BtlSelection selection = {0}; battleItems[0].id = 0x302; input = 2; denySource = 1;
    assert(btlSelectionHandleItemInput((u8 *)&selection, (u8 *)&panel) == 1);
    assert(seenSource == 1 && seenItem == 0x302 && action.unk_6c == 0 && action.unk_6e == 0 && choice.item == 0 && panel.slideFrame == 9);
    denySource = -1;
    assert(btlSelectionHandleItemInput((u8 *)&selection, (u8 *)&panel) == 4);
    assert(action.unk_6c == 3 && action.unk_6e == 0x303 && choice.item == 0x302 && panel.slideFrame == 0);
    unit.genus = 1; denySource = 1; seenSource = -1;
    action.unk_6c = action.unk_6e = choice.item = 0; panel.slideFrame = 9;
    assert(btlSelectionHandleItemInput((u8 *)&selection, (u8 *)&panel) == 4);
    assert(seenSource == -1 && action.unk_6c == 3 && choice.item == 0x302);
    battle.flags = 0x800000; denySource = 2; opened = anim = 0;
    assert(btlMainRevivalMesUpdate() == 8 && seenItem == 0x31e && !(battle.flags & 0x800000));
    assert(releases == 1 && resets == 1 && !retracts && stock == 2);
    denySource = -1; battle.flags = 0x800000;
    assert(btlMainRevivalMesUpdate() == 10 && retracts == 1 && stock == 2);
    opened = 1; denySource = 2;
    assert(btlMainRevivalMesUpdate() == 0 && releases == 2 && resets == 2);
    denySource = -1; itemEntry.effect = 246;
    assert(radarApplyItemStep(1, -1, (u8 *)&work) == 3 && flagWrites == 1 && stock == 1);
    s32 state = 42;
    inventory[0].id = 1550; stock = 3; itemEntry.kind = 6; work.itemCount = 1;
    int categoryHp = fieldHp, categorySp = fieldSp;
    metadataReads = kindReads = categoryCalls = collects = 0; denySource = 3; categoryResult = P4_ITEM_ACTION_HANDLED;
    radarHandleEntrySelect(&state, (u8 *)&work);
    assert(seenSource == 3 && seenItem == 1550 && seenEffect == 0);
    assert(kindReads == 1 && metadataReads == 0 && categoryCalls == 0 && !collects && stock == 3 && state == 42);
    assert(fieldHp == categoryHp && fieldSp == categorySp);
    denySource = -1;
    radarHandleEntrySelect(&state, (u8 *)&work);
    assert(kindReads == 2 && metadataReads == 1 && categoryCalls == 1 && collects == 1 && stock == 2 && state == 42);
    assert(fieldHp == categoryHp + 1 && fieldSp == categorySp + 1);
    categoryResult = P4_ITEM_ACTION_DEFERRED;
    radarHandleEntrySelect(&state, (u8 *)&work);
    assert(kindReads == 3 && metadataReads == 2 && categoryCalls == 2 && collects == 1 && stock == 2 && state == 42);
    itemEntry.kind = 3; seenSource = -1;
    radarHandleEntrySelect(&state, (u8 *)&work);
    assert(seenSource == -1 && categoryCalls == 2 && stock == 2);
    BtlUnit actor = {.genus = 0, .hp = 123, .sp = 45};
    BtlAction scripted = {.unit = &actor};
    BtlAiCommandWork *ai = (BtlAiCommandWork *)scripted.unk_38;
    scriptAction = &scripted; scriptParam = 0x301; denySource = 4;
    inventory[0].quantity = 7; stock = 7; ai->kind = 1; ai->skill = 0x123; scripted.unk_6e = 0x456;
    BtlAction beforeDenied = scripted;
    int beforeApplications = applications; underlyingCalls = 0;
    assert(btlAICommandScriptSetSkillAction() == 1);
    assert(seenSource == 4 && seenItem == 0x301 && seenEffect == 0);
    assert(ai->kind == 7 && ai->skill == 0x123 && scripted.unk_6e == 0x456 && underlyingCalls == 0);
    assert(stock == 7 && inventory[0].quantity == 7 && actor.hp == 123 && actor.sp == 45 && applications == beforeApplications);
    /* At the setter boundary, denial must produce the same action as the
     * native negative-ID wait path from exactly the same pre-state. */
    BtlAction denied = scripted;
    actor.flags3 = 0x4000; actor.status = 0x1122;
    packetCount = waitState = 0;
    btlActionWaitUpdate((u8 *)&scripted);
    BtlPacketTiming deniedPackets[32];
    int deniedCount = packetCount, deniedState = waitState;
    memcpy(deniedPackets, packets, sizeof packets);
    int deniedHp = actor.hp, deniedSp = actor.sp;
    u32 deniedStatus = actor.status;
    scripted = beforeDenied;
    scriptParam = -1; seenSource = -1;
    assert(btlAICommandScriptSetSkillAction() == 1 && seenSource == -1);
    assert(memcmp(&scripted, &denied, sizeof scripted) == 0);
    assert(stock == 7 && inventory[0].quantity == 7 && actor.hp == 123 && actor.sp == 45);
    packetCount = waitState = 0;
    btlActionWaitUpdate((u8 *)&scripted);
    assert(packetCount == deniedCount && waitState == deniedState);
    assert(memcmp(packets, deniedPackets, (size_t)packetCount * sizeof packets[0]) == 0);
    assert(stock == 7 && inventory[0].quantity == 7 && actor.hp == deniedHp && actor.sp == deniedSp && actor.status == deniedStatus);
    scriptParam = 0x301;
    metadataEnabled = 1; ai->kind = 1; seenSource = -1;
    assert(btlAICommandScriptSetSkillAction() == 1 && seenSource == 4 && ai->kind == 7);
    assert(metadataBegins == 1 && metadataEnds == 1 && metadataDecodes == 1 && metadataVmDecodes == 1);
    assert(metadataKind == 7 && metadataParameter == UINT32_MAX);
    assert(stock == 7 && inventory[0].quantity == 7 && actor.hp == 123 && actor.sp == 45 && applications == beforeApplications);
    metadataEnabled = 0;
    scriptParam = -1; seenSource = -1; ai->kind = 1;
    assert(btlAICommandScriptSetSkillAction() == 1 && seenSource == -1 && ai->kind == 7);
    scriptParam = 0x301; actor.genus = 1; seenSource = -1;
    assert(btlAICommandScriptSetSkillAction() == 1 && seenSource == -1);
    assert(ai->kind == 3 && ai->skill == 0x301 && scripted.unk_6e == 0x302 && stock == 7);
    actor.genus = 0; denySource = -1;
    assert(btlAICommandScriptSetSkillAction() == 1 && seenSource == 4 && ai->kind == 3);
    assert(ai->skill == 0x301 && scripted.unk_6e == 0x302 && stock == 7);
    union { u32 align; u8 bytes[48]; } unitData = {0};
    u8 savedUnit[48];
    unitData.bytes[4] = 0x11; unitData.bytes[5] = 0x11;
    unitData.bytes[0x15] = 0x0f;
    unitData.bytes[0x20] = unitData.bytes[0x21] = 0xab;
    unitData.bytes[0x28] = unitData.bytes[0x29] = 0xcd;
    memcpy(savedUnit, unitData.bytes, sizeof savedUnit);
    actor.charId = 1; actor.datUnit = unitData.bytes; scriptAction = &scripted;
    /* Opening policy only selects journaling; authorization remains live. */
    denySource = 4; p4_ai_item_begin(1, &scripted, unitData.bytes, 1);
    scriptParam = 0; assert(btlAICommandScriptSetDownFlag() == 1);
    assert(p4_mod_authorizer_enable(auth, 0) == 0);
    scriptParam = 0x301; assert(btlAICommandScriptSetSkillAction() == 1 && ai->kind == 3);
    assert(!p4_ai_item_finish(1, &scripted) && (unitData.bytes[0] & 8));
    memcpy(unitData.bytes, savedUnit, sizeof savedUnit);
    p4_ai_item_begin(1, &scripted, unitData.bytes, 0);
    scriptParam = 0; assert(btlAICommandScriptSetDownFlag() == 1);
    assert(p4_mod_authorizer_enable(auth, 1) == 0);
    scriptParam = 0x301; assert(btlAICommandScriptSetSkillAction() == 1 && ai->kind == 7);
    assert(p4_ai_item_finish(1, &scripted) && (unitData.bytes[0] & 8));
    memcpy(unitData.bytes, savedUnit, sizeof savedUnit);
    union { u32 align; u8 bytes[48]; } targetData = {0};
    u32 targetScratch = 0x10003, actorScratch = 0x50;
    memcpy(targetData.bytes + 0x18, &targetScratch, 4);
    memcpy(unitData.bytes + 0x18, &actorScratch, 4);
    memcpy(savedUnit, unitData.bytes, sizeof savedUnit);
    BtlUnit targetUnit = {.genus = 1, .datUnit = targetData.bytes, .hp = 80, .sp = 20};
    BtlAction targetAction = {.unit = &targetUnit};
    predictionTarget = &targetAction;
    u16 commands[] = {1};
    BtlAction beforePrediction = scripted;
    p4_ai_item_begin(1, &scripted, unitData.bytes, 1);
    assert(btlAICommandPickSkillTargetByHp((u8 *)&scripted, (u8 *)&targetAction, commands, 1, 0xffff) == 1);
    assert(btlAICommandPickSkillTargetNearMaxHp((u8 *)&scripted, (u8 *)&targetAction, commands, 1, 0xffff) == 1);
    scriptParam = 0;
    assert(btlAICommandScriptWouldBeKilled() == 1 && scriptReturn == 0);
    scriptParam = 0x301; denySource = 4;
    assert(btlAICommandScriptSetSkillAction() == 1 && ai->kind == 7);
    btlActionAiUpdate((u8 *)&scripted);
    u32 restoredTarget; memcpy(&restoredTarget, targetData.bytes + 0x18, 4);
    assert(restoredTarget == targetScratch && memcmp(savedUnit, unitData.bytes, sizeof savedUnit) == 0);
    BtlAction deniedPrediction = scripted;
    packetCount = waitState = 0; btlActionWaitUpdate((u8 *)&scripted);
    BtlPacketTiming predictionPackets[32]; memcpy(predictionPackets, packets, sizeof packets);
    int predictionCount = packetCount, predictionState = waitState;
    scripted = beforePrediction;
    scriptParam = -1;
    assert(btlAICommandScriptSetSkillAction() == 1);
    btlActionAiUpdate((u8 *)&scripted);
    assert(memcmp(&scripted, &deniedPrediction, sizeof scripted) == 0);
    packetCount = waitState = 0; btlActionWaitUpdate((u8 *)&scripted);
    assert(packetCount == predictionCount && waitState == predictionState);
    assert(memcmp(predictionPackets, packets, (size_t)packetCount * sizeof packets[0]) == 0);
    memcpy(&restoredTarget, targetData.bytes + 0x18, 4);
    assert(restoredTarget == targetScratch && targetUnit.hp == 80 && targetUnit.sp == 20 && stock == 7);
    p4_ai_item_begin(1, &scripted, unitData.bytes, 1);
    scriptParam = 0x55;
    assert(btlAICommandScriptSetId2ClearBuffs() == 1);
    scriptParam = 0;
    assert(btlAICommandScriptSetDownFlag() == 1);
    assert(btlAICommandScriptSetSlipFlag() == 1);
    scriptParam = 1;
    assert(btlAICommandScriptSetDownFlag() == 1);
    assert(btlAICommandScriptSetSlipFlag() == 1);
    assert((unitData.bytes[0] & 0x18) == 0);
    scriptParam = 0;
    assert(btlAICommandScriptSetDownFlag() == 1);
    assert(btlAICommandScriptSetSlipFlag() == 1);
    assert(memcmp(savedUnit, unitData.bytes, sizeof savedUnit) != 0);
    scriptParam = 0x301; denySource = 4;
    assert(btlAICommandScriptSetSkillAction() == 1 && ai->kind == 7);
    waitState = 0; btlActionAiUpdate((u8 *)&scripted);
    assert(waitState == 0xf && (scripted.flags & 2) && ai->kind == 7);
    assert(memcmp(savedUnit, unitData.bytes, sizeof savedUnit) == 0);
    assert(stock == 7 && actor.hp == 123 && actor.sp == 45);
    p4_ai_item_begin(1, &scripted, unitData.bytes, 1);
    assert(!p4_ai_item_before(1, &scripted, 0xffff));
    ai->kind = 1; waitState = 0; btlActionAiUpdate((u8 *)&scripted);
    assert(waitState == 0xf && ai->kind == 7);
    assert(memcmp(savedUnit, unitData.bytes, sizeof savedUnit) == 0 && stock == 7);
    actor.genus = 1; scriptParam = 0;
    assert(btlAICommandScriptSetDownFlag() == 1 && (unitData.bytes[0] & 8));
    actor.genus = 0; unitData.bytes[0] = 0;
    assert(btlAICommandScriptSetDownFlag() == 1 && (unitData.bytes[0] & 8));
    /* READY may receive an item without either selection hook. Exercise the
     * exact guard copied from the mirrored override before simulated dispatch. */
    memcpy(unitData.bytes, savedUnit, sizeof savedUnit);
    BtlAction readyAction = {.unit = &actor};
    scriptAction = &readyAction; actor.datUnit = unitData.bytes;
    p4_ai_item_begin(1, &readyAction, unitData.bytes, 1);
    scriptParam = 0x66;
    assert(btlAICommandScriptSetId2ClearBuffs() == 1);
    readyAction.unk_6c = 3; readyAction.parameters.item = 0x301;
    int readyApplications = applications, readyHp = fieldHp, readySp = fieldSp;
    denySource = 5; seenSource = -1;
    fixture_ready((u8 *)&readyAction);
    assert(seenSource == 5 && readyAction.unk_6c == 7);
    assert(memcmp(savedUnit, unitData.bytes, sizeof savedUnit) == 0);
    assert(stock == 7 && applications == readyApplications && fieldHp == readyHp && fieldSp == readySp);
    readyAction.unk_6c = 3; readyAction.parameters.item = 0x301; seenSource = -1;
    fixture_ready((u8 *)&readyAction);
    assert(seenSource == 5 && readyAction.unk_6c == 7 && stock == 7);
    denySource = -1; readyAction.unk_6c = 3; readyAction.parameters.item = 0x301;
    fixture_ready((u8 *)&readyAction);
    assert(readyAction.unk_6c == 3 && stock == 6 && applications == readyApplications + 1);
    stock = 7; denySource = 5;
    actor.genus = 1; readyAction.unk_6c = 3; seenSource = -1;
    fixture_ready((u8 *)&readyAction);
    assert(seenSource == -1 && readyAction.unk_6c == 3 && stock == 6);
    stock = 7; actor.genus = 0; scriptAction = &scripted;
    actor.datUnit = NULL; denySource = -1; scriptParam = 0x301;
    stock = 2;
    p4_mod_authorizers_clear();
    seenSource = -1; ai->kind = 1;
    assert(btlAICommandScriptSetSkillAction() == 1 && seenSource == -1 && ai->kind == 3);
    assert(ai->skill == 0x301 && scripted.unk_6e == 0x302 && stock == 2);
    itemEntry.kind = 6; kindReads = metadataReads = 0; categoryResult = P4_ITEM_ACTION_HANDLED;
    radarHandleEntrySelect(&state, (u8 *)&work);
    assert(kindReads == 0 && metadataReads == 1 && categoryCalls == 3 && collects == 2 && stock == 1 && state == 42);
    inventory[0].id = 0x600; inventory[0].quantity = 2; stock = 2; itemEntry.effect = 12;
    metadataReads = peekReads = 0;
    assert(radarApplyItemStep(1, -1, (u8 *)&work) == 1);
    assert(metadataReads == 1 && peekReads == 0 && stock == 1);
    puts("item_use: mirrored field, custom category, menu, revival and AI consumers");
    return 0;
}
