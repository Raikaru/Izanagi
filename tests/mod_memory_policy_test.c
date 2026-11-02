/* Policy unit: explicit backend evidence fixtures. The separate heap/kernel/image
 * witnesses exercise their production getters; this never substitutes a loader. */
#include "p4pc/mod_memory.h"
#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/tables.h"
#include <stdio.h>
#include <string.h>

static void fixture_function(void) {}
const p4pc_native_t p4pc_native_table[] = {
    {0x00100100u, (void *)fixture_function, "fixture_function", NULL}
};
const size_t p4pc_native_count = 1;
const p4pc_patch_t p4pc_patch_table[] = {
    {0x00101120u, (void *)fixture_function, NULL},
    {0x00101100u, (void *)fixture_function, NULL}
};
const size_t p4pc_patch_count = 2;
const p4pc_hostfn_t p4pc_hostfn_table[] = {{NULL, NULL}};
const size_t p4pc_hostfn_count = 0;
static int evidence = 1;
static const p4pc_image_guest_range_t image[] = {
    {0x00100000u, 0x00101000u, P4_IMAGE_GUEST_EXEC},
    {0x00101000u, 0x00102000u, P4_IMAGE_GUEST_WRITE},
    {0x00102000u, 0x00103000u, 0},
    {0x00100800u, 0x00100c00u, P4_IMAGE_GUEST_WRITE}
};
const p4pc_image_guest_range_t *p4pc_image_guest_ranges(size_t *count)
{
    *count = evidence ? sizeof image / sizeof image[0] : 0;
    return evidence ? image : NULL;
}
int p4pc_heap_guest_span(uint32_t *begin, uint32_t *end)
{
    *begin = 0x00110000u;
    *end = 0x00120000u;
    return 0;
}
int p4pc_thread_guest_stack_overlap(uint32_t address, size_t size)
{
    return size && address < 0x00112000u && 0x00111000u < (uint64_t)address + size;
}
int p4pc_heap_guest_host_overlap(uint32_t address, size_t size)
{
    return size && address < 0x00113040u && 0x00113000u < (uint64_t)address + size;
}
static int failures;
#define CHECK(value) do { if (!(value)) { \
    fprintf(stderr, "mod_memory_policy: FAIL line %d: %s\n", __LINE__, #value); \
    ++failures; } } while (0)
static void store_word(unsigned char *bytes, uint32_t word)
{
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (unsigned char)(word >> (i * 8));
}

int main(void)
{
#ifdef P4_GUEST64
    p4_fn_register(0x00100100u, (void *)fixture_function);
#endif
    unsigned char bytes[16] = {0};
    uint32_t word = 0x12345678u;
    CHECK(p4_mod_memory_function_encode(0x00100100u, &word) == 0);
    CHECK(p4_mod_memory_function_known(word));
    uint32_t decoded = UINT32_MAX;
    CHECK(p4_mod_memory_function_decode(word, &decoded) == 0 && decoded == 0x00100100u);
    CHECK(p4_mod_memory_function_decode(0, &decoded) == 0 && decoded == 0);
    CHECK(p4_mod_memory_function_decode(word, NULL) == -1);
    decoded = UINT32_MAX;
    CHECK(p4_mod_memory_function_decode(0xdeadbeefu, &decoded) == -1 && decoded == UINT32_MAX);
    CHECK(p4_mod_memory_function_known(0));
    CHECK(!p4_mod_memory_function_known(0xdeadbeefu));
    CHECK(p4_mod_memory_function_encode(0x00100101u, &word) == -1);
    CHECK(p4_mod_memory_function_encode(0x00100100u, NULL) == -1);
    CHECK(p4_mod_memory_check(0x00101080u, 4, 1, 0, bytes) == NULL);
    CHECK(p4_mod_memory_check(0x00100880u, 4, 1, 1, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00102080u, 4, 1, 1, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00110fffu, 2, 1, 0, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00111fffu, 2, 0, 0, NULL) != NULL);
    CHECK(p4_mod_memory_check(0x00113001u, 4, 1, 1, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00113001u, 4, 0, 1, NULL) != NULL);
    CHECK(p4_mod_memory_check(0x00110040u, 4, 1, 0, bytes) == NULL);
    CHECK(p4_mod_memory_check(0x00104000u, 4, 1, 0, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00104000u, 4, 0, 0, NULL) == NULL);
    CHECK(p4_mod_memory_check(0x02002000u, 4, 1, 0, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x02002000u, 4, 1, 1, bytes) == NULL);
    CHECK(p4_mod_memory_check(0xfffffffdu, 4, 1, 1, bytes) != NULL);
    CHECK(p4_mod_memory_check(4, SIZE_MAX, 1, 1, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00101080u, 4, 1, 0, NULL) != NULL);
    CHECK(p4_mod_memory_check(UINT32_MAX, 0, 1, 0, NULL) == NULL);

    P4ModMemoryRegion *readonly = p4_mod_memory_declare(0x00110080u, 16, P4_MOD_MEMORY_READ);
    P4ModMemoryRegion *writeable = p4_mod_memory_declare(0x00110080u, 32,
        P4_MOD_MEMORY_READ | P4_MOD_MEMORY_WRITE);
    P4ModMemoryRegion *host = p4_mod_memory_declare(0x02002000u, 16, P4_MOD_MEMORY_HOST_DATA);
    CHECK(readonly && writeable && host);
    CHECK(p4_mod_memory_check(0x00110080u, 4, 1, 1, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00110080u, 4, 0, 0, NULL) == NULL);
    CHECK(p4_mod_memory_check(0x02002000u, 4, 0, 1, NULL) != NULL);
    CHECK(p4_mod_memory_check(0x02002000u, 4, 1, 1, bytes) != NULL);
    p4_mod_memory_revoke(readonly);
    CHECK(p4_mod_memory_check(0x00110080u, 4, 1, 0, bytes) == NULL);
    p4_mod_memory_revoke(writeable);
    p4_mod_memory_revoke(host);
    CHECK(!p4_mod_memory_declare(1, 0, P4_MOD_MEMORY_READ));
    CHECK(!p4_mod_memory_declare(UINT32_MAX, 2, P4_MOD_MEMORY_READ));
    CHECK(!p4_mod_memory_declare(1, 4, P4_MOD_MEMORY_WRITE));
    CHECK(!p4_mod_memory_declare(1, 4, P4_MOD_MEMORY_HOST_DATA | P4_MOD_MEMORY_READ));
    CHECK(!p4_mod_memory_declare(1, 4, P4_MOD_MEMORY_READ | P4_MOD_MEMORY_FUNCTION_WORDS));

    store_word(bytes, 0xdeadbeefu);
    CHECK(p4_mod_memory_check(0x00101100u, 4, 1, 0, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00101120u, 4, 1, 0, bytes) != NULL);
    store_word(bytes, word);
    CHECK(p4_mod_memory_check(0x00101100u, 4, 1, 0, bytes) == NULL);
    CHECK(p4_mod_memory_check(0x00101120u, 4, 1, 0, bytes) == NULL);
    CHECK(p4_mod_memory_check(0x00101101u, 1, 1, 0, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x001010ffu, 2, 1, 0, bytes) != NULL);
    P4ModMemoryRegion *functions = p4_mod_memory_declare(0x00110500u, 8,
        P4_MOD_MEMORY_READ | P4_MOD_MEMORY_WRITE | P4_MOD_MEMORY_FUNCTION_WORDS);
    CHECK(functions);
    store_word(bytes + 4, word);
    CHECK(p4_mod_memory_check(0x00110500u, 8, 1, 0, bytes) == NULL);
    CHECK(p4_mod_memory_check(0x00110507u, 1, 1, 0, bytes) != NULL);
    store_word(bytes + 4, 0xdeadbeefu);
    CHECK(p4_mod_memory_check(0x00110500u, 8, 1, 0, bytes) != NULL);
    p4_mod_memory_clear();
    CHECK(p4_mod_memory_check(0x00110500u, 8, 1, 0, bytes) != NULL);
    p4_mod_memory_revoke(functions);
    P4ModMemoryRegion *classified = p4_mod_memory_declare(0x00104000u, 4,
        P4_MOD_MEMORY_FUNCTION_WORDS);
    CHECK(classified);
    store_word(bytes, word);
    CHECK(p4_mod_memory_check(0x00104000u, 4, 1, 0, bytes) != NULL); /* no write grant */
    CHECK(p4_mod_memory_check(0x00104000u, 4, 1, 1, bytes) == NULL);
    CHECK(p4_mod_memory_check(0x00104001u, 1, 1, 1, bytes) != NULL);
    store_word(bytes, 0xdeadbeefu);
    CHECK(p4_mod_memory_check(0x00104000u, 4, 1, 1, bytes) != NULL);
    p4_mod_memory_revoke(classified);
    CHECK(p4_mod_memory_check(0x00104000u, 4, 1, 1, bytes) == NULL);
    evidence = 0;
    CHECK(p4_mod_memory_check(0x00110040u, 4, 1, 0, bytes) != NULL);
    CHECK(p4_mod_memory_check(0x00110040u, 4, 0, 0, NULL) == NULL);
    CHECK(p4_mod_memory_check(0x02002000u, 4, 1, 1, bytes) == NULL);
    p4_mod_memory_clear();
    printf("mod_memory_policy: %s (allowlist, code/readonly/stack/host exclusions, capabilities, complete known function words, missing evidence)\n",
           failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
