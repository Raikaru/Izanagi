#include "p4pc/image.h"
#include "p4pc/guest.h"
#include "p4pc/os.h"
#include "p4pc/tables.h"
#include <stdio.h>

const p4pc_native_t p4pc_native_table[] = {{0, NULL, NULL, NULL}};
const size_t p4pc_native_count = 0;
const p4pc_patch_t p4pc_patch_table[] = {{0, NULL, NULL}};
const size_t p4pc_patch_count = 0;
const p4pc_alias_t p4pc_alias_table[] = {{0, NULL}};
const size_t p4pc_alias_count = 0;
const p4pc_hostfn_t p4pc_hostfn_table[] = {{NULL, NULL}};
const size_t p4pc_hostfn_count = 0;

int main(int argc, char **argv)
{
    p4_os_init();
    if (argc != 2) return 2;
    size_t count = 123;
    if (p4pc_image_guest_ranges(&count) != NULL || count != 0) return 3;
    uint32_t entry = p4pc_load_image(argv[1]);
    const p4pc_image_guest_range_t *ranges = p4pc_image_guest_ranges(&count);
    if (p4pc_image_guest_ranges(NULL) != ranges) return 4;
    printf("entry=%08x ranges=%zu loaded=%02x bss=%02x\n", entry, count,
           *(const unsigned char *)P4_G2H(0x00100000u),
           *(const unsigned char *)P4_G2H(0x00101000u));
    for (size_t i = 0; i < count; ++i)
        printf("range=%08x:%08x:%u\n", ranges[i].begin, ranges[i].end, ranges[i].flags);
    return 0;
}
