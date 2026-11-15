/* The two actual override bodies with scripted CD/DVD and admission facts.
 * This checks call order and refusal before writes, not a native boot. */
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;
typedef struct { void *work; } KwlnTask;
typedef struct { s32 state; void *file; u32 unk08; } InitDataReadWork;
typedef struct { u8 *fileMemory; s32 fileSize; } HCdvd;
typedef struct { u8 *data; u32 size; } HCdvdRequest;
typedef struct { u32 unk00; u8 **destination; } BtlUnitLoadBlock;
#define MEMBER(type, object, structure, field) (((structure *)(object))->field)
#define P4PC_GUEST_H
static char packed_path[] = "battle/SKILL.TBL";
#undef P4_G2H
#undef P4_H2G
#define P4_G2H(word) test_g2h(word)
#define P4_H2G(pointer) ((u32)(uintptr_t)(pointer))
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_items_native.h"
int item_boundary_guest_map(void);
uint32_t item_boundary_guest_base(void);

static char D_00762CB8, D_00762CC0, D_005F6BE0[1], iGpffffa0a0;
static char D_005E46A0[] = "init/itemtbl.bin";
static u8 D_0079BF00[P4_ITEM_RETAIL_FILE_SIZE];
static void *test_g2h(u32 word)
{
    if (word == 42) return packed_path;
    if (word == 0x005e46a0u) return D_005E46A0;
    if (word == 0x0079bf00u) return D_0079BF00;
    return (void *)(uintptr_t)word;
}
static u8 *D_007F17B0;
static void *iGpffffb434;
static u32 iGpffffb430;
static unsigned sequence, item_gate, packed_gate, observer_calls, note_calls, destroyed, initialized;
static int loaded, refuse, golden;
static jmp_buf expected_refusal;
static HCdvd item_request;
static HCdvdRequest packed_request;
static u8 *old_destination;
static const u8 *expected_descriptor;

static void func_00440b68(void *a, const void *b, s32 line) { (void)a; (void)b; (void)line; }
static void *unexpected_acquire(void) { abort(); }
#define H_Cdvd_AcquireDirectory(path, flags) unexpected_acquire()
static int H_Cdvd_IsFileLoaded(void *file) { assert(file); return loaded; }
static void H_Cdvd_ReadSync(void *file) { assert(file == &packed_request); }
static void H_Cdvd_Destroy(void *file) { assert(file == &item_request || file == &packed_request); ++destroyed; ++sequence; }
static void datEntryTableUseBuiltin(void) { ++initialized; ++sequence; }
int p4_game_table_admission_golden_active(void) { return golden; }
int p4_game_table_admission_golden_load_item(const char *path, void *destination, uint32_t capacity)
{
    if (!golden) return 0;
    assert(!strcmp(path, D_005E46A0) && capacity >= (uint32_t)item_request.fileSize);
    uint8_t *bytes = malloc((size_t)item_request.fileSize); assert(bytes);
    memcpy(bytes, item_request.fileMemory, (size_t)item_request.fileSize);
    p4_game_table_admission_item(path, bytes, (uint32_t)item_request.fileSize);
    int result = p4_game_tables_items_native_load(path, bytes, (uint32_t)item_request.fileSize);
    if (result) p4_game_tables_items_native_refuse(result);
    memcpy(destination, bytes, (size_t)item_request.fileSize);
    free(bytes);
    return 1;
}
int p4_game_table_admission_golden_load_packed(const uint8_t *descriptor, const char *path,
    P4GameTablePackedObserver observer)
{
    uint8_t *bytes = malloc(packed_request.size); assert(bytes);
    memcpy(bytes, packed_request.data, packed_request.size);
    int result = p4_game_table_admission_golden_packed(descriptor, path, bytes, packed_request.size, observer);
    free(bytes);
    return result;
}
_Noreturn void p4_game_table_admission_golden_fail(const char *reason, int result)
{ (void)reason; (void)result; longjmp(expected_refusal, 1); }
int p4_game_table_admission_golden_packed(const uint8_t *descriptor, const char *path,
    uint8_t *bytes, uint32_t size, P4GameTablePackedObserver observer)
{
    assert(golden && descriptor == expected_descriptor && path == packed_path &&
        size == packed_request.size && !memcmp(bytes,packed_request.data,size));
    ++packed_gate; observer(descriptor,path,bytes,size,NULL); return 0;
}
void p4_game_table_admission_item(const char *path, uint8_t *bytes, uint32_t size)
{
    assert(!strcmp(path, D_005E46A0) && size == (u32)item_request.fileSize);
    assert(golden ? !memcmp(bytes,item_request.fileMemory,size) : bytes == item_request.fileMemory);
    for (size_t i = 0; i < size; ++i) assert(D_0079BF00[i] == 0xa5);
    ++item_gate; ++sequence;
    if (refuse) longjmp(expected_refusal, 1);
}
void p4_game_tables_observe_packed(const uint8_t *descriptor, const char *path, const uint8_t *bytes,
    uint32_t size, P4GameTableAdmissionReceipt *receipt)
{
    assert(descriptor == expected_descriptor && path == packed_path && size == packed_request.size);
    assert(golden ? !memcmp(bytes,packed_request.data,size) : bytes == packed_request.data);
    assert(receipt == NULL); ++observer_calls; ++sequence;
}
void p4_game_table_admission_packed(const uint8_t *descriptor, const char *path, uint8_t *bytes,
    uint32_t size, P4GameTablePackedObserver observer)
{
    assert(descriptor == expected_descriptor && path == packed_path && bytes == packed_request.data && size == packed_request.size);
    assert(old_destination == D_007F17B0 + 32 && iGpffffb430 == 0);
    for (unsigned i = 0; i < 16; ++i) assert(D_007F17B0[i] == 0xa5);
    ++packed_gate; ++sequence;
    if (refuse) longjmp(expected_refusal, 1);
    observer(descriptor, path, bytes, size, NULL);
}
void p4_game_table_extent_note(u32 slot, u32 base, u32 bytes)
{
    assert(slot == P4_H2G(&old_destination) && base == P4_H2G(D_007F17B0) && bytes == 4);
    assert(!memcmp(D_007F17B0, "PACK", 4)); ++note_calls; ++sequence;
}
#include "../overrides/001117c0.c"
#include "../overrides/00193840.c"

int main(void)
{
    const size_t arena_size = 4096;
#ifdef _WIN32
    u8 *arena = VirtualAlloc((void *)(uintptr_t)0x20000000u, arena_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
    u8 *arena = mmap((void *)(uintptr_t)0x20000000u, arena_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (arena == MAP_FAILED) arena = NULL;
#endif
    if (!arena || (uintptr_t)arena > INT32_MAX - arena_size) return 77;
    if (item_boundary_guest_map() != 0) return 77;
    D_007F17B0 = arena;
    static u8 item_bytes[P4_ITEM_RETAIL_FILE_SIZE];
    item_bytes[1] = 6;
    item_bytes[4] = 1;
    item_request = (HCdvd){item_bytes, sizeof item_bytes};
    InitDataReadWork work = {1, &item_request, 0}; KwlnTask task = {&work};
    memset(D_0079BF00, 0xa5, sizeof D_0079BF00); loaded = 0;
    assert(initDataReadStep((u8 *)&task) == 0 && !item_gate && !destroyed);
    loaded = 1; refuse = 1;
    if (!setjmp(expected_refusal)) { initDataReadStep((u8 *)&task); assert(!"ITEM failure must stop"); }
    assert(item_gate == 1 && !destroyed && !initialized && work.file == &item_request && D_0079BF00[0] == 0xa5);
    refuse = 0; sequence = 0;
    assert(initDataReadStep((u8 *)&task) == -1 && item_gate == 2 && destroyed == 1 && initialized == 1 && !work.file);
    assert(sequence == 3 && !memcmp(D_0079BF00, item_bytes, sizeof item_bytes));
    assert(item_boundary_guest_base() == 0x0079bf10u);

    u8 packed_bytes[16] = {4, 0, 0, 0, 'P', 'A', 'C', 'K'};
    packed_request = (HCdvdRequest){packed_bytes, sizeof packed_bytes};
    memset(arena, 0xa5, arena_size); old_destination = arena + 32;
    BtlUnitLoadBlock descriptors[2] = {{42, &old_destination}, {0, NULL}};
    expected_descriptor = (u8 *)descriptors; iGpffffb434 = &packed_request; iGpffffb430 = 0;
    loaded = 0; assert(btlUnitLoadBlocksFromFile((u8 *)descriptors, 0) == 0 && !packed_gate);
    loaded = 1; refuse = 1;
    if (!setjmp(expected_refusal)) { btlUnitLoadBlocksFromFile((u8 *)descriptors, 0); assert(!"packed failure must stop"); }
    assert(packed_gate == 1 && !observer_calls && !note_calls && old_destination == arena + 32 && !iGpffffb430);
    refuse = 0; sequence = 0;
    assert(btlUnitLoadBlocksFromFile((u8 *)descriptors, 0) == 1);
    assert(packed_gate == 2 && observer_calls == 1 && note_calls == 1 && old_destination == arena && iGpffffb430 == 16);
    assert(!iGpffffb434 && destroyed == 2 && sequence == 4);
    golden = 1; work.state = 1; work.file = &item_request;
    if (!setjmp(expected_refusal)) { initDataReadStep((u8 *)&task); assert(!"stale item request must refuse"); }
    assert(destroyed == 2 && initialized == 1);
    iGpffffb434 = &packed_request;
    if (!setjmp(expected_refusal)) { btlUnitLoadBlocksFromFile((u8 *)descriptors,0); assert(!"stale packed request must refuse"); }
    assert(iGpffffb434 == &packed_request && destroyed == 2);
    iGpffffb434 = 0;
    work.state = 0; work.file = NULL;
    memset(D_0079BF00,0xa5,sizeof D_0079BF00);
    assert(initDataReadStep((u8 *)&task) == -1);
    assert(item_gate == 3 && destroyed == 2 && initialized == 2 && !work.file);
    assert(!memcmp(D_0079BF00,item_bytes,sizeof item_bytes));
    iGpffffb430 = 0;
    assert(btlUnitLoadBlocksFromFile((u8 *)descriptors,0) == 1);
    assert(packed_gate == 3 && observer_calls == 2 && destroyed == 2 && !iGpffffb434 && !iGpffffb430);
    puts("game_table_loader_boundary: actual override ordering and refusal probes passed; no native boot claim");
    return 0;
}
