/* Synthetic raw-backing vectors through the actual spliced guest consumers. */
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
#include "btl_unit_record_internal.h"
#include "dat_skill_entry_internal.h"
#include "byte_address_internal.h"
#include "operand_order_internal.h"
#include "p4pc/game_mode.h"
#include "p4pc/golden_battle_rows_native.h"

static P4GameMode mode;
P4GameMode p4_game_mode(void) { return mode; }
static u8 party[11 * 382 + 64], enemy[368 * 232], child[256 * 88], skill[440 * 40];
static u8 *iGpffffb3c0 = party, *iGpffffb3cc = enemy, *iGpffffb3e0 = child, *iGpffffb3b8 = skill;
static u8 D_005F6CA0[3 * 30];
static char D_00635938[1];
static unsigned random_value, random_calls;
static unsigned equipment[2];
static void kAssertFail(const void *file, int line) { (void)file; fprintf(stderr, "assert %d\n", line); abort(); }
u32 datUnitInit(DatUnit *unit, u8 genus, u16 id) { (void)genus; unit->id = id; return 0; }
static void btlUiInitSpritePair(u8 *unit, u8 *work) { (void)unit; (void)work; }
static s32 datCalcChkBadStatus(s32 unit, u32 status) { (void)unit; (void)status; return 0; }
static s32 btlFormationLookupSlot(u8 *unit, u16 motion) { (void)unit; (void)motion; return -1; }
static u32 RpRandom(void) { ++random_calls; return random_value; }
static s32 datGetEquippedItemByUnit(s16 id, s32 slot) { (void)id; return slot; }
static s32 datItemValue(s32 item) { return (s32)equipment[item]; }
/*@@CONSUMERS@@*/
static void half(unsigned offset, int value) { party[offset] = (u8)value; party[offset + 1] = (u8)((unsigned)value >> 8); }
int main(void)
{
    BtlUnit unit = {0}; DatUnit data = {0};
    unit.datUnit = &data;
    for (unsigned i = 0; i < sizeof D_005F6CA0; ++i) D_005F6CA0[i] = (u8)i;
    D_005F6CA0[5] = 4; D_005F6CA0[6] = 5; D_005F6CA0[7] = 6;
    half(382 + 0x14, 125); half(382, -4); half(382 + 6, 90); half(382 + 8, 20);
    mode = P4_GAME_MODE_GOLDEN;
    btlUnitInitFromRecord((u8 *)&unit, 1);
    assert(unit.motionCount == 32 && unit.scale == 1.25f && unit.sphereCenter.x == -4.0f);
    assert(unit.unk_8c == 90.0f && unit.sphereRadius == 20.0f);
    assert((u8 *)unit.motionRates == party + 382 + 0x3C);
    assert(btlUnitGetRecordPtr((u8 *)&unit, 0) == party + 382);
    assert(btlUnitGetRecordPtr((u8 *)&unit, 9) == party + 382);
    assert(btlUnitGetRecordPtr((u8 *)&unit, 10) == party + 382 + 10);
    for (unsigned motion = 30; motion < 34; ++motion)
        assert(btlUnitGetMotionId((u8 *)&unit, (u16)motion) == motion - 2);
    party[382 + 0x17C] = 0x7F; party[382 + 0x18] = 4;
    data.id = 1;
    assert(datCalcGetCharacterTableKind(&data) == 0x7F);
    assert(datCalcSkillValueBase((u8 *)&data, 2) == 4 && datCalcSkillValueBase((u8 *)&data, 0) == 1);
    data.flags = 4; assert(datCalcGetCharacterTableKind(&data) == 0); data.flags = 0;
    unit.charId = 0; unit.motionRates = (BtlMotionRate *)(party + 0x3C); unit.motionCount = 32;
    half(0x3C + 4 * 10 + 2, 100); half(0x3C + 5 * 10 + 2, 100); half(0x3C + 6 * 10 + 2, 100);
    half(0x1E, -4); half(0x30, -4);
    assert(btlUnitMotionHitFrames((u8 *)&unit, 5, 1.0f, 2) == -4);
    assert(btlUnitMotionHitFrames((u8 *)&unit, 6, 1.0f, 2) == -4);
    assert(btlUnitMotionHitFrames((u8 *)&unit, 7, 1.0f, 2) == -4);
    half(0x17C, -254);
    assert(btlUnitMotionHitFrames((u8 *)&unit, 5, 1.0f, 177) == -254);
    assert(btlUnitMotionHitFrames((u8 *)&unit, 6, 1.0f, 168) == -254);
    half(382 + 0x16, 9); half(382 + 0x18, -32766);
    assert(btlUnitMotionHitFrames((u8 *)&unit, 5, 1.0f, 189) == 9);
    assert(btlUnitMotionHitFrames((u8 *)&unit, 5, 1.0f, 190) == -32766);
    /* Beyond-payload bytes belong to the supplied native backing. */
    half(11 * 382, -123);
    assert(btlUnitMotionHitFrames((u8 *)&unit, 5, 1.0f, (11 * 382 - 0x1A) / 2) == -123);
    half(360, 12); half(362, 50); half(364, -1);
    assert(btlUnitMotionFrames((u8 *)&unit, 32, 1.0f) == 24);
    assert(btlUnitMotionFollowFrames((u8 *)&unit, 32, 1.0f) == 0);
    assert(p4_golden_motion_quotient(1, 0, 1.0f) == INT_MAX);
    assert(p4_golden_motion_quotient(-1, 0, 1.0f) == INT_MIN);
    assert(p4_golden_motion_quotient(0, 0, 1.0f) == 0);
    DatUnit target = {0}; skill[0] = 2; party[382 + 0x17C] = 2;
    random_value = 9; random_calls = 0;
    assert(datCalcRollSupportSkillHit((u8 *)&data, (u8 *)&target, 0, 2, 0, 0) == 1 && random_calls == 1);
    random_value = 10;
    assert(datCalcRollSupportSkillHit((u8 *)&data, (u8 *)&target, 0, 2, 0, 0) == 0);
    equipment[0] = 0x71; equipment[1] = 0x73; random_value = 0;
    assert(datCalcRollSupportSkillHit((u8 *)&data, (u8 *)&target, 0, 2, 0, 0) == 0);
    data.bad = 0x80; random_calls = 0;
    assert(datCalcRollSupportSkillHit((u8 *)&data, (u8 *)&target, 0, 2, 0, 0) == 1 && !random_calls);
    target.flags = 0x2000;
    assert(datCalcRollSupportSkillHit((u8 *)&data, (u8 *)&target, 0, 2, 0, 0) == 0 && !random_calls);
    target.flags = 0; data.bad = 0;
    mode = P4_GAME_MODE_PS2; memset(party, 0, sizeof party);
    half(332 + 0x14, 100); party[332 + 0x14A] = 3; party[332 + 0x18] = 2;
    btlUnitInitFromRecord((u8 *)&unit, 1);
    assert(unit.motionCount == 27 && unit.scale == 1.0f && (u8 *)unit.motionRates == party + 332 + 0x3C);
    assert(btlUnitGetRecordPtr((u8 *)&unit, 10) == party + 332 + 10);
    assert(btlUnitGetRecordPtr((u8 *)&unit, 9) == party + 332 + 10);
    assert(datCalcGetCharacterTableKind(&data) == 3 && datCalcSkillValueBase((u8 *)&data, 2) == 2);
    puts("golden_model_rows: ok");
    return 0;
}
