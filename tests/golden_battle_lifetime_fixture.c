/* Engine providers record allocations/callbacks; consumers are extracted unchanged. */
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "override_test_types.h"
typedef long long s64;
typedef unsigned long long u64;
typedef double f64;
#define TYPE_H
#include "btl_action_internal.h"
#include "btl_effect_internal.h"
#include "member_access.h"
#include "p4pc/game_mode.h"
#include "p4pc/golden_battle_rows_native.h"
#include "guest_test.h"
#include "btl_unit_record_internal.h"
#include "p4pc/golden_interception_guest.h"
#include "btl_sound_internal.h"
#include "byte_address_internal.h"
static unsigned voice_draws, indexed_requests, stream_requests, panel_requests;
static u16 last_voice;
static u32 panel_units[2];
static char preparation[4];
static unsigned preparation_count;
static void btlSoundCancelPlayback(void) { preparation[preparation_count++] = 'R'; }
static void *engine_allocations[128];
static s32 engine_sizes[128];
static unsigned engine_allocation_count;
static s32 engine_allocation_size(void *pointer)
{
    for (unsigned i = 0; i < engine_allocation_count; ++i)
        if (engine_allocations[i] == pointer) return engine_sizes[i];
    assert(0); return 0;
}
static void btlSoundStopBank3(void) { preparation[preparation_count++] = 'F'; }
s32 sdkSndStartIndexedSource(s32 source, s32 channel, s32 cue)
{
    assert(source == 0xD && channel == 3);
    ++indexed_requests; last_voice = (u16)cue;
    return 1;
}
s32 sdkSndStartStream(s16 index, s16 channel, s16 cue, s16 variant)
{
    assert(index == 1 && channel == 4 && cue == 2 && variant == 0xB);
    ++stream_requests;
    return 1;
}
void btlPanelStartValues(s32 task, s32 unit, s32 window, s32 code)
{
    assert(task == 0x77 && window == 0x88 && code == 0xBB);
    assert((u32)unit == panel_units[0] || (u32)unit == panel_units[1]);
    ++panel_requests;
}
static u32 datGetFlag(s32 flag) { assert(flag == 0x38); return 1; }
u16 cmmCommunityForArcana(s64 arcana) { assert(arcana == 5); return 3; }
u16 cmmCommunityRecordValue(s32 id) { assert(id == 3); return 9; }
typedef BtlPacketList PacketList;
static u8 *iGpffffb3ac;
static u8 D_005F6C00[1], D_005F6FE8[1];
static u32 iGpffffa0b8;
static unsigned engine_frees, updates, random_draws, sidecar_live;
static u64 next_uid;
void *p4_test_sidecar_malloc(size_t size)
{ void *p = malloc(size); assert(p); ++sidecar_live; return p; }
void p4_test_sidecar_free(void *p)
{ assert(p && sidecar_live); --sidecar_live; free(p); }
static u8 *engine_allocate(s32 size, s32 flags)
{ assert(size > 0 && flags == 0x40000); u8 *p = p4t_galloc((u32)size, 16); assert(p);
  assert(engine_allocation_count < 128); engine_allocations[engine_allocation_count] = p;
  engine_sizes[engine_allocation_count++] = size; return p; }
static void engine_free(void *p) { assert(p); ++engine_frees; }
static u8 *(*D_008873E8[])(s32, s32) = {engine_allocate};
#define jtbl_008873E8 D_008873E8
static void (*jtbl_008873EC[])(void *) = {engine_free};
static void chkmemAllocSiteTag(const void *file, s32 line)
{ assert(file && (line == 0x51 || line == 0x3a)); }
static void btlTargetClearRecord(u8 *record) { assert(record); }
static s64 btlGetUID(void) { return (s64)++next_uid; }
static u32 datCalcRandomBelow(u32 limit)
{
    if (limit == 2) { ++voice_draws; preparation[preparation_count++] = 'G'; return 1; }
    assert(limit == 0x3c); ++random_draws; return 7;
}
static void action_initial(u8 *action) { assert(action); }
static void (*D_005F6E20[])(u8 *) = {action_initial};
static s32 btlPacketWaitConditionMet(u8 *condition, s32 elapsed)
{ (void)elapsed; assert(*condition == 1); return 1; }
static u32 packet_wait(void *work) { assert(work); ++updates; return 0; }
static u32 packet_done(void *work) { assert(work); ++updates; return 1; }
/* Rendering/audio providers are outside this headless packet-timing witness. */
static u32 btlFormationRefresh(void *work) { assert(work); return 0; }
static u32 btlFormationIsReady(void *work) { assert(work); return 1; }
static s32 btlFormationHasEntry(u8 *work) { assert(work); return 1; }
static u32 btlFormationRelease(void *work) { assert(work); return 0; }
static u32 btlSoundRefreshSlotChannel(void *work) { assert(work); return 0; }
/*@@CONSUMERS@@*/
static void arm(BtlPacket *packet, BtlPacketFunc callback)
{
    BtlBattleState *battle = (BtlBattleState *)iGpffffb3ac;
    packet->phase = 1; packet->flags = 8; packet->listIndex = 0;
    packet->previous = packet->next = NULL; packet->updateFunc = callback;
    battle->packetLists[0].head = battle->packetLists[0].tail = (u8 *)packet;
}
static s32 publish_opening(BtlAction *action, BtlPacket *packet)
{
    u8 *actionBytes = (u8 *)action;
    BtlPacket *skillFormationPacket = packet;
    s32 goldenLead = 0;
    /*@@OPENING@@*/
    return goldenLead;
}
static void publish_primary(BtlAction *target, u16 skill, BtlPacket *packet, u16 hitTimeTotal)
{
    u8 *targetAction = (u8 *)target;
    BtlPacketTiming *skillFormationPacket = (BtlPacketTiming *)packet;
    /*@@PRIMARY_DELAY@@*/
}
static void follower_vectors(BtlAction *actor, BtlAction *target)
{
    BtlBattleState *battle = (BtlBattleState *)iGpffffb3ac;
    BtlAction other = {0};
    other.unit = p4t_galloc(sizeof(BtlUnit), 16);
    assert(other.unit);
    panel_units[0] = P4_H2G(target->unit);
    panel_units[1] = P4_H2G(other.unit);
    battle->uiTask = 0x77; battle->messageWindow = 0x88;
    uint64_t now = 1000000000ull;
    p4_golden_frame_step_reset();
    p4_golden_frame_step_publish(now);
    BtlPacketTiming *first = p4_golden_interception_create(
        0x12345678, P4_H2G(actor->unit), panel_units[0], 576, 12, 0, 101, 202, actor->uid);
    BtlPacket *sound = (BtlPacket *)battle->packetLists[1].head;
    assert(!sound->initFunc);
    sound->phase = 1;
    btlSoundResetSlotQueue();
    btlPacketStep((u8 *)sound);
    assert(stream_requests == 1);
    BtlSoundQueue *queue = (BtlSoundQueue *)(iGpffffb3ac + 0xAC8);
    assert(queue->channel == 5 && queue->frame == -1);
    queue->frame = 0;
    for (unsigned frame = 0; frame < 5; ++frame) {
        now += 16666667ull; p4_golden_frame_step_publish(now);
        btlSoundAdvanceSlotQueue();
        assert(p4_golden_live_frame_step() == 1.0f);
        assert(p4_golden_cue_cache_match(4, 2, 0xB));
        if (frame >= 2) assert(queue->frame == -1);
    }
    BtlPacket *hit = btlSoundCreateChannelCuePacket(4, 2, 0xB);
    btlPacketEnqueue((u8 *)hit, 1); hit->phase = 1;
    btlPacketStep((u8 *)hit); assert(stream_requests == 1);
    now += 16666667ull; p4_golden_frame_step_publish(now);
    btlSoundAdvanceSlotQueue();
    assert(queue->frame == -1 && p4_golden_cue_cache_match(4, -1, -1));
    BtlPacket *replay = btlSoundCreateChannelCuePacket(4, 2, 0xB);
    btlPacketEnqueue((u8 *)replay, 1); replay->phase = 1;
    btlPacketStep((u8 *)replay); assert(stream_requests == 2);
    puts("R2 902: empty init; native reset/advance; step=1 rotating-byte expires at3; cache expires at6 despite fifth-frame hit; stream requests=2");
    p4_golden_interception_followers(actor, target, first, 1);
    BtlPacket *voice = (BtlPacket *)battle->packetLists[1].head;
    BtlPacket *message = (BtlPacket *)battle->packetLists[3].head;
    assert(voice->type == 0xA02 && engine_allocation_size(voice) == 0x90 + 2);
    assert(voice->dependencyKind == 5 && voice->dependencyUID == first->uid);
    assert(voice->actionUID == actor->uid && voice->listIndex == 1);
    assert(!voice->initFunc && !voice->destroyFunc);
    assert(message->type == 0x506 && engine_allocation_size(message) == 0x90 + 24);
    assert(message->dependencyKind == 5 && message->dependencyUID == first->uid);
    assert(message->actionUID == actor->uid && message->listIndex == 3);
    assert(!memcmp(preparation, "RFG", 3) && preparation_count == 3 && voice_draws == 1);
    BtlPacketTiming *second = p4_golden_interception_create(
        0x12345678, P4_H2G(actor->unit), panel_units[1], 576, 12, 1, 101, 303, actor->uid);
    p4_golden_interception_followers(actor, &other, second, 0);
    BtlPacket *other_message = (BtlPacket *)battle->packetLists[3].head;
    assert(other_message != message && other_message->dependencyUID == second->uid);
    assert(voice_draws == 1 && preparation_count == 3);
    voice->phase = 1;
    unsigned freed = engine_frees;
    btlPacketStep((u8 *)voice);
    assert(indexed_requests == 1 && last_voice == 0x6AD && engine_frees == freed + 1);
    assert(p4_golden_live_frame_step() == 1.0f);
    message->phase = 1;
    for (unsigned frame = 0; frame < 64; ++frame) btlPacketStep((u8 *)message);
    assert(message->phase == 2 && panel_requests == 1);
    assert(((P4GoldenInterceptionMessageWork *)message->workData)->elapsed == 32.0f);
    freed = engine_frees;
    btlPacketStep((u8 *)message);
    assert(engine_frees == freed + 1 && panel_requests == 1);
    now += 33333334ull; p4_golden_frame_step_publish(now);
    assert(p4_golden_live_frame_step() == 2.0f);
    other_message->phase = 1;
    for (unsigned frame = 0; frame < 32; ++frame) btlPacketStep((u8 *)other_message);
    assert(other_message->phase == 2 && panel_requests == 2);
    assert(((P4GoldenInterceptionMessageWork *)other_message->workData)->elapsed == 32.0f);
    freed = engine_frees;
    btlPacketStep((u8 *)other_message);
    assert(engine_frees == freed + 1 && panel_requests == 2);
    printf("R2 followers: skill=576 Ii=%llu/%llu A02=0x6AD bank=13 channel=3 RNG=1 prep=RFG; 506 code=0xBB duration=32 step=1 ticks=65 step=2 ticks=33 strict-boundary=32 pending\n",
           (unsigned long long)first->uid, (unsigned long long)second->uid);
    btlPacketFlushAll();
}
static void timing_vectors(void)
{
    BtlBattleState *battle = (BtlBattleState *)iGpffffb3ac;
    BtlAction *actor = (BtlAction *)btlActionCreate();
    BtlAction *target = (BtlAction *)btlActionCreate();
    BtlUnit *enemy = p4t_galloc(sizeof(BtlUnit), 16);
    BtlUnit *party = p4t_galloc(sizeof(BtlUnit), 16);
    assert(enemy && party);
    actor->unit = enemy; enemy->genus = 1;
    target->unit = party; party->genus = 0;
    actor->unk_6a = 3;
    actor->targets[0] = actor->targets[1] = (u8 *)actor;
    actor->targets[2] = (u8 *)target;
    target->targetBlock.reactionFlag = 1;
    battle->flags &= ~0x80000000u;
    p4_golden_interception_resolve(actor, battle, 1);
    assert(p4_golden_result_intercepted(P4_H2G(target)));
    const u16 skills[] = {10, 12, 13, 576};
    const u32 leads[] = {12, 32768, 0, 12};
    for (unsigned vector = 0; vector < 4; ++vector) {
        actor->unk_6e = skills[vector];
        BtlPacket *packet = (BtlPacket *)btlPacketAlloc(0x301, 0x20);
        assert(publish_opening(actor, packet) == (s32)leads[vector]);
        arm(packet, packet_done); updates = 0;
        for (u32 frame = 0; frame < leads[vector]; ++frame) btlPacketStep((u8 *)packet);
        assert(!updates && packet->phase == 1);
        btlPacketStep((u8 *)packet);
        assert(updates == 1);
        printf("R2 timing trace: skill=%u x=%d intercept=1 lead=%u waited=%u dispatch=%u\n",
               skills[vector], p4_golden_effect_timing(skills[vector]), leads[vector], leads[vector], leads[vector]);
    }
    const u16 interception_skills[] = {20, 21, 22, 23, 24, 576};
    const u32 expected_lead[] = {12, 0, 12, 32768, 0, 12};
    const u32 expected_delay[] = {10, 22, 10, 10, 32777, 10};
    const u32 expected_kind[] = {5, 5, 0xB, 5, 5, 5};
    const s64 expected_uid[] = {202, 101, 202, 202, 101, 202};
    for (unsigned vector = 0; vector < 6; ++vector) {
        actor->unk_6e = interception_skills[vector];
        s32 lead = p4_golden_effect_lead(actor);
        assert(lead == (s32)expected_lead[vector]);
        BtlPacket *packet = (BtlPacket *)p4_golden_interception_create(
            0x12345678u, P4_H2G(enemy), P4_H2G(party), actor->unk_6e,
            lead, 2, 101, 202, actor->uid);
        assert(packet->type == 0x301 && packet->listIndex == 1 && packet->actionUID == actor->uid);
        assert(packet->dependencyKind == expected_kind[vector] && packet->dependencyUID == expected_uid[vector]);
        u32 *work = packet->workData;
        assert(work[0] == 0x12345678u && work[1] == P4_H2G(enemy) && work[2] == P4_H2G(party));
        assert(((BtlFormationEffectWork *)work)->side == 1 && !work[7]);
        BtlPacket *sound = (BtlPacket *)battle->packetLists[1].head;
        assert(sound != packet && sound->type == 0x902 && sound->dependencyKind == 5 && sound->dependencyUID == packet->uid);
        u16 *tuple = sound->workData;
        assert(tuple[0] == 4 && tuple[1] == 2 && tuple[2] == 0xB);
        assert(packet->uid != 101 && packet->uid != 202 && sound->uid != packet->uid);
        printf("R2 Ii trace: skill=%u x=%d lead=%d target=2 P0=101 Pi=202 Ii=%llu kind=%u dep=%llu delay=%u mode=1 options=0 queue=1 follower902.dep=%llu\n",
               actor->unk_6e, p4_golden_effect_timing(actor->unk_6e), lead,
               (unsigned long long)packet->uid, packet->dependencyKind,
               (unsigned long long)packet->dependencyUID, expected_delay[vector],
               (unsigned long long)sound->dependencyUID);
        /* Dependencies are now satisfied; retain each native delay-step/release. */
        battle->packetLists[1].head = battle->packetLists[1].tail = (u8 *)sound;
        sound->previous = sound->next = NULL;
        arm(packet, packet_done); updates = 0;
        for (u32 frame = 0; frame < expected_delay[vector]; ++frame) btlPacketStep((u8 *)packet);
        assert(!updates && packet->phase == 1);
        btlPacketStep((u8 *)packet); assert(updates == 1);
        btlPacketFlushAll();
    }
    const u16 primary_skills[] = {10, 20};
    const u32 primary_delays[] = {65565, 65535};
    for (unsigned vector = 0; vector < 2; ++vector) {
        BtlPacket *packet = (BtlPacket *)btlPacketAlloc(0x301, 0x20);
        publish_primary(target, primary_skills[vector], packet, 65535);
        arm(packet, packet_done); updates = 0;
        for (u32 frame = 0; frame < primary_delays[vector]; ++frame) btlPacketStep((u8 *)packet);
        assert(!updates && packet->phase == 1);
        btlPacketStep((u8 *)packet); assert(updates == 1);
        printf("R2 Pi trace: skill=%u accumulator=65535 marker=1 flags4=%u delay=%u\n",
               primary_skills[vector], !!(p4_golden_effect_flags(primary_skills[vector]) & 4), primary_delays[vector]);
    }
    follower_vectors(actor, target);
    btlActionListDestroyAll();
    assert(!sidecar_live);
}
static void lifetime_vectors(P4GameMode *selector)
{
    iGpffffb3ac = p4t_galloc(sizeof(BtlBattleState), 16); assert(iGpffffb3ac);
    BtlAction *first = (BtlAction *)btlActionCreate(), *second = (BtlAction *)btlActionCreate();
    assert(random_draws == 2 && !p4_golden_result_intercepted(P4_H2G(first)));
    p4_golden_result_intercept(P4_H2G(first)); assert(sidecar_live == 1);
    p4_golden_result_copy(P4_H2G(second), P4_H2G(first)); assert(sidecar_live == 2);
    p4_golden_result_copy(P4_H2G(first), P4_H2G(first)); assert(sidecar_live == 2);
    second->targetBlock.message.messageCode = 0;
    assert(p4_golden_result_intercepted(P4_H2G(second))); /* Message-only clearing preserves the marker. */
    ++second->uid;
    assert(!p4_golden_result_intercepted(P4_H2G(second)));
    p4_golden_result_intercept(P4_H2G(second)); assert(sidecar_live == 2);
    second->unk_1a |= 2; second->flags = 0;
    btlActionListUpdate();
    assert(engine_frees == 1 && sidecar_live == 1 && !p4_golden_result_intercepted(P4_H2G(second)));
    btlActionListDestroyAll();
    assert(engine_frees == 2 && !sidecar_live && !p4_golden_result_intercepted(P4_H2G(first)));

    BtlPacket *packet = (BtlPacket *)btlPacketAlloc(0x301, 0x20);
    const int32_t timing = p4_golden_effect_timing(12);
    assert(timing == INT16_MIN);
    const uint32_t lead = (uint32_t)-timing;
    assert(lead == 32768);
    p4_golden_packet_delay_set(P4_H2G(packet), lead);
    packet->uid = ++next_uid; /* Enqueue assigns UID after delay setup. */
    arm(packet, packet_wait); updates = 0;
    for (uint32_t i = 0; i < lead; ++i) btlPacketStep((u8 *)packet);
    assert(!updates && packet->phase == 1 && packet->delay == 0 && sidecar_live == 1);
    btlPacketStep((u8 *)packet); assert(updates == 1 && packet->phase == 2);
    packet->flags |= 4; btlPacketStep((u8 *)packet);
    assert(engine_frees == 3 && !sidecar_live && p4_golden_packet_delay_step(P4_H2G(packet)) == -1);

    packet = (BtlPacket *)btlPacketAlloc(0x301, 0x20);
    const uint32_t large_delay = (uint32_t)p4_golden_effect_timing(13) + 5u * 11u;
    assert(large_delay == 32822);
    p4_golden_packet_delay_set(P4_H2G(packet), large_delay);
    arm(packet, packet_done); updates = 0;
    for (uint32_t i = 0; i < large_delay; ++i) btlPacketStep((u8 *)packet);
    assert(!updates && sidecar_live == 1);
    btlPacketStep((u8 *)packet);
    assert(updates == 1 && engine_frees == 4 && !sidecar_live);

    packet = (BtlPacket *)btlPacketAlloc(0x301, 0x20);
    p4_golden_packet_delay_set(P4_H2G(packet), 40000); arm(packet, packet_wait);
    btlPacketFlushAll();
    assert(engine_frees == 5 && !sidecar_live && p4_golden_packet_delay_step(P4_H2G(packet)) == -1);
    packet = (BtlPacket *)btlPacketCreateImmediate();
    assert(packet->type == 0xff00 && p4_golden_packet_delay_step(P4_H2G(packet)) == -1);
    p4_golden_packet_delay_set(P4_H2G(packet), 40000); assert(sidecar_live == 1);
    p4_golden_packet_delay_set(P4_H2G(packet), 7); assert(!sidecar_live && packet->delay == 7);

    *selector = P4_GAME_MODE_PS2;
    packet = (BtlPacket *)btlPacketAlloc(0x301, 0x20);
    packet->delay = 0xffff; arm(packet, packet_wait); updates = 0;
    btlPacketStep((u8 *)packet); assert(updates == 1 && packet->phase == 2 && packet->delay == 0xffff);
    packet->phase = 1; packet->delay = 3; updates = 0;
    for (unsigned i = 0; i < 3; ++i) btlPacketStep((u8 *)packet);
    assert(!updates && packet->delay == 0 && packet->phase == 1);
    btlPacketStep((u8 *)packet); assert(updates == 1 && packet->phase == 2 && !sidecar_live);
    *selector = P4_GAME_MODE_GOLDEN;
    timing_vectors();
    /* Teardown must also reclaim still-live result and packet sidecars. */
    BtlAction *live = (BtlAction *)btlActionCreate();
    p4_golden_result_intercept(P4_H2G(live));
    p4_golden_packet_delay_set(P4_H2G(packet), 40000);
    assert(sidecar_live == 2);
    puts("golden_battle_lifetime: native action/packet release, full-width delays and unchanged PS2 delay branch passed");
}
