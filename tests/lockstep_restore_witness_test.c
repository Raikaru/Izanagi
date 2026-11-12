/* Exercise the production card-restore override, with synthetic guest data and
 * a witness spy in place of the alignment runtime. No game or card is needed. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/lockstep_align_guest.h"
#include "p4pc/mod_save_native.h"

typedef int32_t s32;
typedef uint8_t u8;
typedef uint32_t u32;

#define D_0079B67C ((u32 *)P4_G2H(0x0079b67c))
#define iGpffffb19c (*(u32 *)P4_G2H(0x0076428c))
#define D_005E45C0 ((char *)P4_G2H(0x005e45c0))
#define DAT_00796de0 ((char *)P4_G2H(0x00796de0))
#define DAT_00796e00 ((char *)P4_G2H(0x00796e00))
#define DAT_00796e20 ((char *)P4_G2H(0x00796e20))
#define DAT_007973a0 ((char *)P4_G2H(0x007973a0))
#define DAT_007973b2 ((char *)P4_G2H(0x007973b2))
#define iGpffff9bb0 (*(char *)P4_G2H(0x00762ca0))

static unsigned phase, sections, witness_count, commits;
int func_00440b68(const char *format_string, ...)
{
    (void)format_string;
    return 0;
}
int func_00442088(char *buffer, const char *format_string, ...)
{
    va_list args;
    int result;
    va_start(args, format_string);
    result = vsnprintf(buffer, 0x24, format_string, args);
    va_end(args);
    return result;
}

int p4_mod_save_restore_begin(int32_t version, const void *image, int32_t size)
{
    assert(version == 7 && image && size == 0x44 && phase == 0);
    phase = 1;
    return 0;
}
_Noreturn void p4_mod_save_restore_failed(void) { abort(); }
void datSaveRestoreSection(s32 version, u32 tag, u32 size, u8 *data)
{
    assert(phase == 1 && version == 7 && tag == 0x1234 && size == 4);
    memcpy(&D_0079B67C[0], data, 4);
    sections++;
}
void p4pc_align_restore_applied(void)
{
    assert(phase == 1 && sections == 1 && iGpffffb19c == 0x4567);
    witness_count++;
    phase = 2;
}
void p4_mod_save_restore_commit(void)
{
    /* A completed card restore without the actual override's witness fails. */
    assert(phase == 2 && witness_count == 1);
    assert(strcmp(DAT_00796e20, "last first") == 0);
    commits++;
    phase = 3;
}

#ifdef P4_GUEST64
void func_0010e710(s32, u8 *, s32);
#else
#include "../overrides/0010e710.c"
#endif

int main(void)
{
    const u32 tag = 0x1234, size = 4, saved_playtime = 0x4567, end = 0x2000;
    u8 *image;
    if (p4t_guest_map(0x005e4000, 0x1b8000) != 0)
        return 77;
    image = p4t_galloc(0x44, 4);
    assert(image);
    memset(image, 0, 0x44);
    memcpy(image + 0x34, &tag, 4);
    memcpy(image + 0x38, &size, 4);
    memcpy(image + 0x3c, &saved_playtime, 4);
    memcpy(image + 0x40, &end, 4);
    strcpy(&iGpffff9bb0, "%s %s");
    strcpy(DAT_007973a0, "first");
    strcpy(DAT_007973b2, "last");
    iGpffffb19c = 0xdead;
    func_0010e710(7, image, 0x44);
    assert(phase == 3 && commits == 1 && witness_count == 1);
    return 0;
}
