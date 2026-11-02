/* ROFS metadata options through the generated entry: the PC device accepts
 * volume options without constructing guest mount metadata or changing state. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/image.h"
#include "p4pc/tables.h"
#include "guest_test.h"

enum {
    REQUEST = 0x01810000, INFO = 0x01810100, QUERY = 0x01810200,
    LAST_ERROR = 0x00743a54
};

static void require(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "cri_rofs_metadata_test: %s\n", message);
        exit(1);
    }
}

static int test_main(int argc, char **argv)
{
    static const char *const queries[] = {"VOL", "BTL", "BGM", "ENV", "ROFS", "missing", ""};
    int32_t (*native)(void *, int32_t, int32_t, int32_t) = NULL;
    unsigned char initial[0x300];
    unsigned cases = 0;
    const char *elf = getenv("P4PC_ELF");
    (void)argv;
    require(argc == 1, "expected no arguments");
    if (elf == NULL || *elf == '\0') return 77;
    p4pc_load_image(elf);
    for (size_t i = 0; i < p4pc_native_count; i++) {
        const p4pc_native_t *entry = &p4pc_native_table[i];
        if (entry->addr != 0x004f8848) continue;
        require(native == NULL, "duplicate generated option entry");
        native = entry->fn;
#ifdef P4_GUEST64
        require(entry->sig != NULL && strcmp(entry->sig, "ipiii") == 0,
                "generated option register classes changed");
#endif
    }
    require(native != NULL, "missing actual generated option entry");
    for (unsigned error = 0; error < 2; error++) {
        uint32_t last_error = error ? 0x12345678 : 0;
        for (size_t i = 0; i < sizeof queries / sizeof *queries; i++) {
            for (unsigned option = 5; option <= 6; option++) {
                for (unsigned mode = 0; mode < 4; mode++) {
                    uint32_t *request = P4_G2H(REQUEST);
                    memset(request, 0xa5, sizeof initial);
                    strcpy(P4_G2H(QUERY), queries[i]);
                    request[0] = 0;
                    request[1] = mode == 3 ? 0 : QUERY;
                    request[2] = mode == 1 ? 0 : INFO;
                    P4_G32(INFO) = 0x11223344;
                    P4_G32(INFO + 4) = 0x55667788;
                    P4_G32(LAST_ERROR) = last_error;
                    memcpy(initial, request, sizeof initial);
                    require(native(mode == 2 ? NULL : request, (int32_t)option, 0, 0) == 0 &&
                            memcmp(initial, request, sizeof initial) == 0 &&
                            P4_G32(LAST_ERROR) == last_error,
                            "metadata option changed the PC device result or state bytes");
                    cases++;
                }
            }
        }
    }
    printf("ROFS metadata: %u generated-entry cases and state bytes exact\n", cases);
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
