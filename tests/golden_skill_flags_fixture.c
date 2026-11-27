/* Actual spliced threshold and packet/counter statements, not copied equations. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "override_test_types.h"
typedef long long s64;
typedef unsigned long long u64;
typedef double f64;
#define TYPE_H
#include "btl_unit_record_internal.h"
#include "btl_action_internal.h"
#include "dat_skill_entry_internal.h"
#include "member_access.h"
#include "p4pc/golden_battle_rows_native.h"
#include "p4pc/gmem.h"
/*@@TYPES@@*/
static unsigned golden, flags, cost_calls, skill_sprites, action_sprites, enqueues;
static u8 metadata[440 * 40], D_00635938[1];
static u8 *iGpffffb3b8 = metadata;
static DatPartySlot party_slots[14];
static DatCharRecord protagonist;
static u8 *D_00796E50 = (u8 *)party_slots, *D_007973C4 = (u8 *)&protagonist;
static BtlPacketTiming packet;
int p4_golden_battle_rows_enabled(void) { return (int)golden; }
uint32_t p4_golden_skill_flags(uint32_t skill) { assert(skill == 10); return flags; }
static void kAssertFail(const void *file, int line) { (void)file; fprintf(stderr, "assert %d\n", line); abort(); }
static u32 datCalcSkillCost(DatUnit *unit, s32 skill) { (void)unit; assert(skill == 10); ++cost_calls; return 50; }
static BtlPacketTiming *btlEffectCreateSpriteFramePacket(u32 unit, u16 skill)
{ assert(unit == 0x1234 && skill == 10); ++skill_sprites; return &packet; }
static BtlPacketTiming *btlPacketCreateSpriteStep(u32 unit, s32 step)
{ assert(unit == 0x1234 && step == 12); ++action_sprites; return &packet; }
static s64 btlPacketEnqueue(u8 *p, u32 priority)
{ assert(p == (u8 *)&packet && priority == 3 && packet.delay == 3 && packet.actionUid == 77); ++enqueues; return 0; }
/*@@THRESHOLD@@*/
/*@@UNIT_ACCESSOR@@*/
static void sprite(int isKind3)
{
    BtlSkillAction action = {0}, *act = &action;
    u8 *actionBytes = (u8 *)&action;
    s32 skill = 10, hitModeDelay = 3;
    s64 actionId = 77;
    BtlPacketTiming *skillSpritePacket, *actionSpritePacket;
    action.unit = (u8 *)0x1234; action.field70 = 12;
    /*@@SPRITE@@*/
}
static unsigned counter(int recordResult, int r1, int r2, int special)
{
    BtlAction action = {0};
    u8 *unit = (u8 *)&action;
    s32 status = 0, critCount = 3, id = 10;
    action.targetBlock.specialHitFlag = special;
    /*@@COUNTER@@*/
    assert(status == ((recordResult != 1 || special == 1 || r2 == 1) ? 2 : 0));
    return (unsigned)critCount;
}
int main(void)
{
    DatUnit unit = {0};
    uint8_t *row = metadata + 10 * 40;
    for (unsigned selector = 1; selector <= 15; ++selector) {
        const void *expected = selector == 1 ? (void *)&protagonist : (void *)&party_slots[selector - 2].character;
        assert(p4_golden_skill_unit_guest(selector) == P4_H2G(expected));
    }
    for (golden = 0; golden < 2; ++golden)
        for (unsigned set = 0; set < 2; ++set) {
            flags = set ? 0x100 : 0;
            for (int kind = 0; kind < 2; ++kind) {
                skill_sprites = action_sprites = enqueues = 0;
                sprite(kind);
                assert(action_sprites == (unsigned)(kind != 0));
                assert(skill_sprites == (unsigned)(!kind && (!golden || !set)));
                assert(enqueues == skill_sprites + action_sprites);
            }
            flags = set ? 0x800 : 0;
            for (int result = 0; result < 3; ++result)
                for (int special = 0; special < 2; ++special)
                    for (int r1 = 0; r1 < 2; ++r1)
                        for (int r2 = 0; r2 < 2; ++r2)
                            assert(counter(result, r1, r2, special) == 3u +
                                (unsigned)(result == 1 && !special && r2 != 1 && r1 && (!golden || !set)));
        }
    golden = 1; flags = 0x200; unit.bad = 8; cost_calls = 0;
    assert(datCalcThresholdTrigger(&unit, 10) == 0 && !cost_calls);
    flags = 0; row[0x11] = 0x10;
    assert(datCalcThresholdTrigger(&unit, 10) == 4 && !cost_calls); /* Golden no longer keys the PS2 byte shortcut. */
    golden = 0;
    assert(datCalcThresholdTrigger(&unit, 10) == 0 && !cost_calls);
    row[0x11] = 0;
    assert(datCalcThresholdTrigger(&unit, 10) == 4 && !cost_calls);
    golden = 1; unit.bad = 0; unit.hp = unit.sp = 50; row[3] = 1;
    assert(datCalcThresholdTrigger(&unit, 10) == 1);
    row[0] = 1; assert(datCalcThresholdTrigger(&unit, 10) == 0);
    unit.hp = 49; assert(datCalcThresholdTrigger(&unit, 10) == 1);
    row[3] = 2; assert(datCalcThresholdTrigger(&unit, 10) == 0);
    unit.sp = 49; assert(datCalcThresholdTrigger(&unit, 10) == 2);
    flags = 0x80000000u; assert(datCalcThresholdTrigger(&unit, 10) == 2);
    puts("golden_skill_flags: threshold ordering, category-specific sprite bypass and qualified counter suppression passed");
    return 0;
}
