#include "p4pc/game_mode.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef int8_t s8; typedef uint8_t u8; typedef int16_t s16; typedef uint16_t u16; typedef int32_t s32; typedef uint32_t u32;
typedef struct { u16 flags, id; u8 rest[44]; } DatPersonaRecord;
typedef struct { u16 flags; u8 arcana, level, pad[10]; } YListPersonaMetadata;
typedef struct { u8 ready, pad[3]; union { s32 value; u32 unsignedValue; } kind; union { s32 value; s16 half; } count; u8 other[0x3014 - 12]; } YListPersonaWork;
typedef struct { u8 *work; } KwlnTask;
#define MEMBER(type, ptr, structure, field) (((structure *)(ptr))->field)
static P4GameMode mode;
P4GameMode p4_game_mode(void) { return mode; }
static YListPersonaMetadata metadata[256];
static u8 *iGpffffb3d4 = (u8 *)metadata;
static DatPersonaRecord roster, cached;
static int datPersonaRosterCapacity(void) { return 1; }
static int datPersonaRosterSlotActive(int n) { return n == 0; }
static void *datPersonaRosterSlot(int n) { (void)n; return &roster; }
static void *datPersonaGetRecord(int n) { (void)n; return &cached; }
static void *datPersonaCachedRecord(int n) { (void)n; return &cached; }
static int yListRecordsDiffer(u8 *a, u8 *b) { (void)a; (void)b; return 1; }
static void datPersonaInitRecord(void *p, u16 id) { DatPersonaRecord *r = p; memset(r, 0, sizeof *r); r->id = id; }
static void datPersonaInitAllRecords(void) {}
#include "../overrides/002e4090.c"
int main(void)
{
    YListPersonaWork work; KwlnTask task = {(u8 *)&work};
    roster.id = 220; metadata[220].arcana = 27;
    memset(&work, 0, sizeof work); work.kind.value = 6; mode = P4_GAME_MODE_PS2;
    yListSlotRefresh((u8 *)&task); assert(work.count.value == 0);
    memset(&work, 0, sizeof work); work.kind.value = 6; mode = P4_GAME_MODE_GOLDEN;
    yListSlotRefresh((u8 *)&task); assert(work.count.value == 1);
    memset(&work, 0, sizeof work); work.kind.value = 8; mode = P4_GAME_MODE_PS2;
    yListSlotRefresh((u8 *)&task); assert(work.count.value == 191);
    memset(&work, 0, sizeof work); work.kind.value = 8; mode = P4_GAME_MODE_GOLDEN;
    yListSlotRefresh((u8 *)&task); assert(work.count.value == 255);
    assert(((DatPersonaRecord *)((u8 *)&work + 0x14))[254].id == 255);
    memset(&work, 0, sizeof work); work.kind.value = 2;
    yListSlotRefresh((u8 *)&task); assert(work.count.value == 256);
    return 0;
}
