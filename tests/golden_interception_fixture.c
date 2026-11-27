/* Synthetic live guest records exercise the production Golden helpers. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "override_test_types.h"
typedef long long s64;
typedef unsigned long long u64;
typedef double f64;
#define TYPE_H
#include "btl_unit_record_internal.h"
#include "btl_action_internal.h"
#include "btl_effect_internal.h"
#include "btl_skill_internal.h"
#include "g_data_internal.h"
#include "p4pc/golden_interception_guest.h"
extern s32 btlCurrentUnitId(void);
/*@@CONSUMERS@@*/

static u32 flag38, effect_flags, marked[4];
static u16 rank3, rank4;
static s32 effect_timing;
static int exists3, exists4, selected, selector_calls, value_calls, encounter;
u32 datGetFlag(s32 id) { assert(id == 0x38); return flag38; }
u16 cmmCommunityForArcana(s64 arcana)
{ assert(arcana == 5); ++selector_calls; selected = exists3 ? 3 : exists4 ? 4 : 0; return selected; }
u16 cmmCommunityRecordValue(s32 id)
{ assert(id == selected); ++value_calls; return id == 3 ? rank3 : id == 4 ? rank4 : 0; }
void p4_golden_result_intercept(uint32_t action)
{ unsigned i = 0; while (marked[i] && marked[i] != action) { ++i; assert(i < 4); } marked[i] = action; }
void p4_golden_result_clear(uint32_t action)
{ for (unsigned i = 0; i < 4; ++i) if (marked[i] == action) marked[i] = 0; }
static u32 expected_skill = 10, flag_reads;
uint32_t p4_golden_effect_flags(uint32_t id) { assert(id == expected_skill); ++flag_reads; return effect_flags; }
s32 btlCurrentUnitId(void) { return encounter; }
int p4_golden_result_intercepted(uint32_t action)
{ for (unsigned i = 0; i < 4; ++i) if (marked[i] == action) return 1; return 0; }
int32_t p4_golden_effect_timing(uint32_t id) { assert(id == 10); return effect_timing; }
static unsigned golden = 1, packets, rng_calls, status_present;
static u8 *iGpffffb3ac;
static BtlPacket look_packet;
int p4_golden_battle_rows_enabled(void) { return golden; }
/* Domain service provider; the camera must use its existing active query. */
int p4_game_skill_active(u32 id)
{ return id < 440u || (golden && id - 576u < 32u); }
static BtlPacket *btlUnitCreateLookAtDeactivatePacket(BtlUnit *unit, u16 flags)
{ assert(!unit && flags == 3); look_packet.uid = 999; return &look_packet; }
static s64 btlPacketEnqueue(u8 *packet, u32 list)
{ assert(packet == (u8 *)&look_packet && list == 1 && look_packet.actionUID == 91); ++packets; return look_packet.uid; }
static s32 btlActionAnyTargetHitRowStatus(u8 *action)
{ assert(action); return status_present; }
static u32 datCalcRandomBelow(u32 limit)
{ assert(limit == 100); ++rng_calls; return 99; }
static void residual_vectors(void)
{
    for (unsigned set = 0; set < 2; ++set) {
        BtlBattleState battle = {0}; iGpffffb3ac = (u8 *)&battle;
        s64 sceneUid = 7, chainUid = 8, animEndUid = 9, actionId = 91;
        BtlPacket *animPacket = &look_packet;
        u8 *groupLookAtStopPacket;
        s32 timingFrames = 20, damageFrames = 21, windupFrames = 22, restoreBeforeHits = 0;
        s16 skill = 10;
        effect_flags = set ? 0x1000 : 0; packets = 0;
        if (0) {}
        /*@@RESIDUAL@@*/
        assert(!sceneUid && !chainUid && !animPacket && !animEndUid && !timingFrames && !damageFrames && windupFrames == 6);
        assert(packets == set && battle.flags == (set ? 0x400000u : 0) && battle.actionEndFlags == (set ? 6 : 0));
        assert(restoreBeforeHits == (s32)set);
    }
}
static void cutin_vectors(void)
{
    for (unsigned set = 0; set < 2; ++set)
        for (unsigned prior = 0; prior < 2; ++prior)
            for (status_present = 0; status_present < 2; ++status_present) {
                BtlBattleState battle = {0}; iGpffffb3ac = (u8 *)&battle;
                BtlAction action = {0}, *act = &action;
                u8 *actionBytes = (u8 *)&action, *cutinBattle;
                s16 skill = 10; s32 hasCutin = 0;
                action.flags = prior ? 0x40 : 0;
                effect_flags = set ? 0x20000 : 0; rng_calls = 0;
                /*@@CUTIN@@*/
                assert(rng_calls == (!prior && status_present));
                assert(hasCutin == (s32)set && !battle.battleFlags);
                assert(action.flags == ((set || prior) ? 0x40 : 0));
            }
}

static unsigned count506, count800, pending_uid, uid_calls, published_state;
static u32 btlCountUnitRecordsById(u32 id)
{ assert(id == 0x506 || id == 0x800); return id == 0x506 ? count506 : count800; }
static u8 *btlUnitRecordFindByActionUID(u64 uid, u64 mask)
{ assert(uid == 77 && mask == 0x3FFFFFFFFFFFFFFFULL); ++uid_calls; return pending_uid ? (u8 *)1 : 0; }
static void btlActionSetState(BtlAction *action, u16 state)
{ assert(action->uid == 77); published_state = state; }
/*@@ASSIST@@*/
static void assist_vectors(void)
{
    for (golden = 0; golden < 2; ++golden)
        for (count506 = 0; count506 < 2; ++count506)
            for (count800 = 0; count800 < 2; ++count800)
                for (pending_uid = 0; pending_uid < 2; ++pending_uid) {
                    BtlAction action = {0}; action.uid = 77;
                    uid_calls = published_state = 0;
                    btlActionAssistUpdate(&action);
                    unsigned ready = !count506 && !count800 && (!golden || !pending_uid);
                    assert(action.flags == (ready ? 0x100 : 0) && published_state == (ready ? 0xF : 0));
                    assert(uid_calls == (golden && !count506 && !count800));
                }
    golden = 1;
}
static BtlSkillFlags effect_prefix[608];
BtlSkillFlags *iGpffffb3bc = effect_prefix;
static void high_flag_vectors(void)
{
    for (golden = 0; golden < 2; ++golden)
        for (unsigned bits = 0; bits < 4; ++bits) {
            s32 randomFlag = 0, baseDelay = 0, skillFlagsOffset = 10 * 4;
            s16 skill = 10;
            effect_flags = (bits & 1 ? 0x10000 : 0) | (bits & 2 ? 0x4000 : 0);
            effect_prefix[10].flags = (u16)effect_flags;
            /*@@HIGH@@*/
            assert(randomFlag == ((bits & 2) != 0 || (golden && (bits & 1))));
            assert(baseDelay == (bits & 2 ? 2 : 0));
            s32 hitFlags = 0x1000;
            effect_flags = 0x80000;
            /*@@OPTIONS@@*/
            assert(hitFlags == (golden ? 0x101000 : 0x1000));
        }
    for (golden = 0; golden < 2; ++golden)
        for (s8 unitSide = 0; unitSide < 2; ++unitSide)
            for (unsigned override = 0; override < 2; ++override) {
                BtlAction action = {0}, *act = &action;
                s64 side;
                action.flags = override ? 0x100 : 0;
                action.targetBlock.resultFlags = 2;
                /*@@GENUS@@*/
                assert(side == (golden && override ? -1 : unitSide));
            }
    golden = 1;
}
static void reset_actor(BtlAction *action)
{
    u8 *context = (u8 *)action;
    /*@@ACTOR_RESET@@*/
}
static void reset_target(BtlAction *action)
{
    u8 *unit = (u8 *)action;
    /*@@TARGET_RESET@@*/
}
static void reset_vectors(void)
{
    BtlAction action = {0};
    for (golden = 0; golden < 2; ++golden)
        for (unsigned target = 0; target < 2; ++target) {
            memset(marked, 0, sizeof marked);
            p4_golden_result_intercept((u32)&action);
            action.targetBlock.specialHitFlag = 1;
            action.targetBlock.reactionFlag = 1;
            action.targetBlock.message.messageCode = 99;
            if (target) reset_target(&action); else reset_actor(&action);
            assert(!action.targetBlock.specialHitFlag && !action.targetBlock.reactionFlag && !action.targetBlock.message.messageCode);
            assert(p4_golden_result_intercepted((u32)&action) == !golden);
        }
    golden = 1;
    memset(marked, 0, sizeof marked);
}
static f32 camera_distance(u16 id, f32 distance)
{
    BtlAction action = {0}; action.unk_6e = id;
    BtlCamera camera = {0}, *cameraWork = &camera;
    camera.action = &action;
    u16 skill;
    /*@@CAMERA@@*/
    return distance;
}
static void camera_vectors(void)
{
    const u16 ids[] = {10, 439, 440, 575, 576, 607, 608};
    for (golden = 0; golden < 2; ++golden)
        for (unsigned i = 0; i < sizeof ids / sizeof ids[0]; ++i)
            for (unsigned set = 0; set < 2; ++set) {
                expected_skill = ids[i]; effect_flags = set ? 0x400 : 0; flag_reads = 0;
                if (ids[i] < 608) effect_prefix[ids[i]].flags = (u16)effect_flags;
                unsigned active = ids[i] < 440 || (golden && ids[i] >= 576 && ids[i] < 608);
                assert(camera_distance(ids[i], 150.0f) == 200.0f + (active && set ? 150.0f : 0));
                assert(flag_reads == (golden && active));
            }
    expected_skill = 10; golden = 1;
}
int main(void)
{
    BtlBattleState battle = {0};
    BtlAction actor = {0}, targets[3] = {{0}}, partner = {0}, null_action = {0};
    BtlUnit enemy = {0}, party[2] = {{0}}, duplicate = {0};
    actor.unit = &enemy; enemy.genus = 1; actor.unk_6e = 10; actor.unk_6a = 3;
    targets[0].unit = &party[0]; targets[1].unit = &party[1]; targets[2].unit = &enemy;
    for (unsigned i = 0; i < 3; ++i) {
        actor.targets[i] = (u8 *)&targets[i];
        targets[i].targetBlock.hitRows[0].spDelta = 77;
        targets[i].targetBlock.hitRows[0].addedStatus = 0x1234;
        targets[i].targetBlock.hitRows[0].removedStatus = 0x5678;
        targets[i].targetBlock.hitRows[0].field10 = 99;
        targets[i].targetBlock.hitRows[0].flags = 0x20;
    }
    targets[0].targetBlock.reactionFlag = 1;
    flag38 = 1; exists3 = exists4 = 1; rank3 = 8; rank4 = 10;
    p4_golden_interception_resolve(&actor, &battle, 1);
    assert(!marked[0] && selected == 3 && !battle.flags);
    exists3 = 0; rank4 = 9;
    exists4 = 0;
    p4_golden_interception_resolve(&actor, &battle, 1);
    assert(!marked[0] && selected == 0 && !battle.flags);
    exists4 = 1;
    actor.unk_6a = 0;
    p4_golden_interception_resolve(&actor, &battle, 1);
    assert(!marked[0] && !battle.flags);
    actor.unk_6a = 3; enemy.genus = 0;
    p4_golden_interception_resolve(&actor, &battle, 1);
    assert(!marked[0] && !battle.flags);
    enemy.genus = 1;
    p4_golden_interception_resolve(&actor, &battle, 3);
    assert(!marked[0] && !battle.flags);
    targets[0].targetBlock.reactionFlag = 0;
    p4_golden_interception_resolve(&actor, &battle, 1);
    assert(!marked[0] && !battle.flags);
    targets[0].targetBlock.reactionFlag = 1;
    p4_golden_interception_resolve(&actor, &battle, 2);
    assert(selected == 4 && battle.flags == 0x80000000u);
    assert(marked[0] == (u32)&targets[0] && marked[1] == (u32)&targets[1] && !marked[2]);
    for (unsigned i = 0; i < 2; ++i) {
        BtlActionTargetBlock *r = &targets[i].targetBlock;
        assert(r->hitRowCount == 1 && !r->reactionFlag && !r->specialHitFlag);
        assert(r->hitOutcome == 0x100 && r->targetFlags == 1 && !r->resultFlags);
        assert(r->message.messageCode == 0 && r->message.unk06 == -1 && !r->message.blocked);
        assert(r->hitRows[0].flags == 0x420 && r->hitRows[0].motion == -1 && !r->hitRows[0].hpDelta && !r->hitRows[0].field10);
        assert(r->hitRows[0].spDelta == 77 && r->hitRows[0].addedStatus == 0x1234 && r->hitRows[0].removedStatus == 0x5678);
    }
    assert(targets[2].targetBlock.hitRows[0].field10 == 99);
    effect_timing = -12; assert(p4_golden_effect_lead(&actor) == 12);
    effect_timing = 12; assert(p4_golden_effect_lead(&actor) == 0);
    effect_timing = -32768; assert(p4_golden_effect_lead(&actor) == 32768);
    memset(marked, 0, sizeof marked);
    targets[0].targetBlock.specialHitFlag = 1; effect_timing = -12;
    assert(p4_golden_effect_lead(&actor) == 0); /* Reflection is not interception. */
    selector_calls = value_calls = 0;
    p4_golden_interception_resolve(&actor, &battle, 1);
    assert(!selector_calls && !value_calls);
    battle.flags = 0; flag38 = 0;
    p4_golden_interception_resolve(&actor, &battle, 1);
    assert(!selector_calls && !value_calls);

    encounter = 0x206; enemy.charId = 0x106;
    party[0].charId = duplicate.charId = 0x10F;
    party[0].nextUnit = &duplicate;
    battle.unitLists[2] = (u8 *)&party[0];
    partner.unit = &party[0]; battle.actions = (u8 *)&partner;
    assert(p4_golden_fade_context(&actor, &battle) == &partner);
    assert(!p4_golden_fade_candidate(&actor, &enemy, &battle));
    partner.unit = &duplicate;
    assert(!p4_golden_fade_context(&actor, &battle)); /* No retry with second10F. */
    battle.unitLists[2] = 0; battle.actions = (u8 *)&null_action;
    assert(p4_golden_fade_context(&actor, &battle) == &null_action);
    encounter = 0x205; assert(!p4_golden_fade_context(&actor, &battle));
    encounter = 0x206; enemy.charId = 0x10F; assert(!p4_golden_fade_context(&actor, &battle));
    residual_vectors();
    cutin_vectors();
    assist_vectors();
    high_flag_vectors();
    reset_vectors();
    camera_vectors();
    puts("golden_interception: live gate selection, all-party partial rewrite, latch and exact fade lookup passed");
    return 0;
}
