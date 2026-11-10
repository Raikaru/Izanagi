#include "guest_test.h"
#include <stdio.h>

extern uint64_t p4pc_ee_heap_sbrk(int32_t increment);
static int failures;
#define CHECK(value) do { if (!(value)) { \
    fprintf(stderr, "heap_guest_span: FAIL line %d: %s\n", __LINE__, #value); \
    ++failures; } } while (0)

int main(void)
{
    p4_os_init();
    /* Retail morecore writes its errno word at 0x009389e0 even when it
     * succeeds; this fixture does not load an ELF to map that page. */
    if (p4t_guest_map(0x00700000u, 0x20000u) != 0 ||
        p4t_guest_map(0x00938000u, 0x1000u) != 0 ||
        p4t_guest_map(0x01000000u, 0x100000u) != 0)
        return 1;
    uint32_t begin = 0x12345678u, end = 0x87654321u;
    CHECK(p4pc_heap_guest_span(&begin, &end) == -1);
    CHECK(begin == 0x12345678u && end == 0x87654321u);
    CHECK(p4pc_heap_guest_span(NULL, &end) == -1);
    CHECK(p4pc_heap_guest_span(&begin, NULL) == -1);
    CHECK(p4pc_heap_init(P4_G2H(0x01000000u), 0x100000u) == 0);
    uint32_t old_break = P4_G32(0x0070c5d4u);
    CHECK(p4pc_heap_guest_span(&begin, &end) == 0);
    CHECK(begin == 0x01000000u && end == 0x01100000u);
    CHECK(P4_G32(0x0070c5d4u) == old_break);
    CHECK(p4pc_ee_heap_sbrk(16) == old_break);
    CHECK(p4pc_heap_guest_span(&begin, &end) == 0);
    CHECK(begin == 0x01000000u && end == 0x01100000u);
    CHECK(P4_G32(0x0070c5d4u) == old_break + 16);
    CHECK(p4pc_heap_init(P4_G2H(0x01010000u), 4096) == -1);
    CHECK(p4pc_heap_guest_span(&begin, &end) == 0 && begin == 0x01000000u);

    /* Real debug heap, including its real EE allocation decisions. */
    CHECK(p4_os_setenv("P4PC_HEAP_CHECK", "1", 1) == 0);
    P4_G32(0x00710070u) = 0x00700100u;
    unsigned char *payload = p4pc_heap_malloc(64);
    CHECK(payload != NULL);
    if (payload != NULL) {
        uint32_t address = P4_H2G(payload);
        CHECK(!p4pc_heap_guest_host_overlap(address, 64));
        CHECK(p4pc_heap_guest_host_overlap(address - 1, 1));
        CHECK(p4pc_heap_guest_host_overlap(address + 64, 1));
        CHECK(!p4pc_heap_guest_host_overlap(address, 0));
        p4pc_heap_free(payload);
        CHECK(p4pc_heap_guest_host_overlap(address, 64));
    }
    printf("heap_guest_span: %s (initialization, immutable bounds, break independence, real debug metadata/quarantine)\n",
           failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
