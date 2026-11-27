#include "p4pc/game_mode.h"
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
static u8 retail[576 * 19];
static u8 *iGpffffb448 = retail;
static const char *D_00635938 = "test";
static P4GameMode mode;
static uint32_t golden_address, expanded_address;
static unsigned assertions;
static jmp_buf refused;
static u8 guest[1024];
P4GameMode p4_game_mode(void) { return mode; }
uint32_t p4_golden_skill_name(uint32_t id) { return id == 42 ? golden_address : 0; }
uint32_t p4_game_skill_name(uint32_t id) { return id == 600 ? expanded_address : 0; }
void kAssertFail(const char *file, int line) { (void)file; (void)line; ++assertions; }
void p4_package_boot_runtime_failed(const char *reason) { assert(reason); longjmp(refused, 1); }
static uint32_t address_of(unsigned offset)
{
#if __SIZEOF_POINTER__ == 8
    return offset;
#else
    return (uint32_t)(uintptr_t)(guest + offset);
#endif
}
#include "../overrides/00243840.c"

int main(void)
{
#if __SIZEOF_POINTER__ == 8
    p4_guest_base = guest;
#endif
    mode = P4_GAME_MODE_PS2;
    assert(datCalcGetIdRecord(42) == retail + 42 * 19);
    assert(datCalcGetIdRecord(0x1002a) == retail + 42 * 19);
    expanded_address = address_of(300);
    assert(datCalcGetIdRecord(600) == guest + 300);
    golden_address = address_of(400);
    memcpy(guest + 400, "1234567890123456789012", 23);
    mode = P4_GAME_MODE_GOLDEN;
    assert(datCalcGetIdRecord(42) == guest + 400);
    assert(strlen((const char *)datCalcGetIdRecord(42)) == 22);
    if (!setjmp(refused)) {
        (void)datCalcGetIdRecord(43);
        assert(!"missing Golden row must refuse rather than read PS2");
    }
    assert(assertions == 0);
    return 0;
}
