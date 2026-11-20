/* The production mirrored sfd_set.c, not a copied getter/setter implementation.
 * Only external handle, driver and error seams are isolated. No retail input,
 * player, decoder or R5900 execution is needed. */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sfd.h"

SFLIB_WORK SFLIB_libwork;
static struct SFD_OBJ handle;
static unsigned checks, failures, handle_checks, setup_checks, errors;

Sint32 SFD_GetCond(struct SFD_OBJ *sfd, Sint32 id, Sint32 *val);
Sint32 SFD_SetCond(struct SFD_OBJ *sfd, Sint32 id, Sint32 val);

Sint32 SFLIB_CheckHn(SFD sfd)
{
    handle_checks++;
    return sfd == &handle ? 0 : -1;
}

Sint32 SFTRN_IsSetup(SFD sfd, Sint32 strm)
{
    (void)sfd;
    (void)strm;
    setup_checks++;
    return 1;
}

Sint32 SFLIB_SetErr(SFD sfd, Sint32 code)
{
    (void)sfd;
    (void)code;
    errors++;
    return -1;
}

#define CHECK(condition) do { \
    checks++; \
    if (!(condition)) { \
        failures++; \
        fprintf(stderr, "SFD condition bounds line %d: %s\n", __LINE__, #condition); \
    } \
} while (0)

static Sint32 word(uint32_t bits)
{
    Sint32 result;
    memcpy(&result, &bits, sizeof result);
    return result;
}

int main(void)
{
    static const uint32_t invalid[] = {
        100u, UINT32_MAX, UINT32_C(0x7fffffff), UINT32_C(0x80000000),
        101u, UINT32_C(0x40000000), UINT32_C(0x40000006), UINT32_C(0xc0000063)
    };
    unsigned char saved_global[sizeof SFLIB_libwork];
    unsigned char saved_handle[sizeof handle];
    Sint32 value;

    _Static_assert(sizeof(Sint32) == 4 && sizeof(Uint32) == 4, "32-bit keys and words");
    _Static_assert(SFD_COND_NUM == 100, "the mutable condition table has 100 words");
    _Static_assert(sizeof SFLIB_libwork.cond == 400, "400-byte condition span");
    memset(&SFLIB_libwork, 0, sizeof SFLIB_libwork);
    memset(&handle, 0, sizeof handle);
    /* Adjacent non-condition state is deliberately nonzero, so key100 cannot
     * accidentally pass by returning the next field's initial zero. */
    memset((unsigned char *)&SFLIB_libwork + sizeof SFLIB_libwork.cond, 0x5a, 16);
    for (unsigned i = 0; i < SFD_COND_NUM; i++) {
        SFLIB_libwork.cond[i] = word(UINT32_C(0x80010203) ^ i * UINT32_C(0x01030507));
        handle.cond[i] = word(UINT32_C(0xfedcba98) ^ i * UINT32_C(0x03050709));
        handle.cond_def[i] = word(UINT32_C(0x12345678));
    }
    /* All100 keys include the required0,6,15,61,99 boundaries and callers. */
    memcpy(saved_global, &SFLIB_libwork, sizeof saved_global);
    memcpy(saved_handle, &handle, sizeof saved_handle);
    for (Sint32 key = 0; key < SFD_COND_NUM; key++) {
        value = word(UINT32_C(0x13579bdf));
        CHECK(SFD_GetCond(NULL, key, &value) == 0);
        CHECK((Uint32)value == (Uint32)SFLIB_libwork.cond[key]);
        CHECK(SFD_GetCond(&handle, key, &value) == 0);
        CHECK((Uint32)value == (Uint32)handle.cond[key]);
    }
    CHECK(memcmp(saved_global, &SFLIB_libwork, sizeof saved_global) == 0);
    CHECK(memcmp(saved_handle, &handle, sizeof saved_handle) == 0);
    for (Sint32 key = 0; key < SFD_COND_NUM; key++) {
        Sint32 replacement = word(UINT32_C(0x90123456) ^ (Uint32)key * 1009u);
        CHECK(SFD_SetCond(NULL, key, replacement) == 0);
        CHECK((Uint32)SFLIB_libwork.cond[key] == (Uint32)replacement);
        CHECK(SFD_SetCond(&handle, key, replacement) == 0);
        CHECK((Uint32)handle.cond[key] == (Uint32)replacement);
        CHECK((Uint32)handle.cond_def[key] == (Uint32)replacement);
    }
    memcpy(saved_global, &SFLIB_libwork, sizeof saved_global);
    memcpy(saved_handle, &handle, sizeof saved_handle);
    handle_checks = setup_checks = errors = 0;
    for (unsigned i = 0; i < sizeof invalid / sizeof *invalid; i++) {
        Sint32 key = word(invalid[i]);
        for (unsigned use_handle = 0; use_handle < 2; use_handle++) {
            SFD sfd = use_handle ? &handle : NULL;
            value = word(UINT32_C(0x13579bdf));
            CHECK(SFD_GetCond(sfd, key, &value) == 0);
            CHECK(value == 0);
            CHECK(SFD_SetCond(sfd, key, word(UINT32_C(0xaabbccdd))) == 0);
            CHECK(memcmp(saved_global, &SFLIB_libwork, sizeof saved_global) == 0);
            CHECK(memcmp(saved_handle, &handle, sizeof saved_handle) == 0);
        }
    }
    CHECK(handle_checks == 0 && setup_checks == 0 && errors == 0);
    printf("SFD condition bounds: %u checks, %u failures\n", checks, failures);
    return failures != 0;
}
