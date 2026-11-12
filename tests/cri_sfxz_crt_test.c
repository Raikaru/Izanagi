/* Invoke the production mirrored SFXZ caller and its real SJ/scanner closure.
 * A supported user converter copies the original integer table, so this checks
 * scanner/slot/index width independently of the separate Z projection policy. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "guest_test.h"
#include "p4pc/image.h"

extern void SFXZ_MakeCnvZTbl(void *object, void *frame_index, void *table);
static unsigned converted;

static void copy_table(uint32_t *original, void *out, float near_z, float far_z)
{
    (void)near_z;
    (void)far_z;
    memcpy(out, original, 256 * sizeof *original);
    converted++;
}

static unsigned tag(unsigned char *out, const char *name, const char *text)
{
    unsigned length = (unsigned)strlen(text) + 1;
    char header[17];
    snprintf(header, sizeof header, "%.7s %07x ", name, length);
    memcpy(out, header, 16);
    memcpy(out + 16, text, length);
    return 16 + length;
}

static int check(void *object, unsigned char *stream, uint32_t *table,
                 const char *stride, const char *record, unsigned index,
                 uint32_t low, uint32_t high)
{
    char records[193];
    unsigned length;
    memset(records, ' ', sizeof records - 1);
    records[sizeof records - 1] = 0;
    memcpy(records, "0 11 22", 7);
    memcpy(records + 64, record, strlen(record));
    length = tag(stream, "ZMFSIZE", stride);
    length += tag(stream + length, "ZMFDATA", records);
    ((uint32_t *)object)[0x28 / 4] = 1;
    ((uint32_t *)object)[0x2c / 4] = P4_H2G(stream);
    ((uint32_t *)object)[0x30 / 4] = length;
    ((uint32_t *)object)[0x44 / 4] = p4_fn_h2g_sig(copy_table, "vppxx");
    memset(table, 0xcd, 0x800);
    unsigned before = converted;
    SFXZ_MakeCnvZTbl(object, P4_G2H(index), table);
    if (converted != before + 1 || table[0] != 0 || table[8] != 0 ||
        table[9] != low || table[16] != low || table[224] != high ||
        table[239] != high || table[240] != 0x7fffffffu || table[255] != 0x7fffffffu) {
        fprintf(stderr, "SFXZ stride=%s record=%s index=%u: %08x/%08x, expected %08x/%08x\n",
                stride, record, index, table[9], table[224], low, high);
        return 1;
    }
    return 0;
}

int main(void)
{
    const char *elf = getenv("P4PC_ELF");
    void *object;
    unsigned char *stream;
    uint32_t *table;
    if (!elf || !*elf) return 77;
    p4pc_load_image(elf);
    p4pc_apply_patches();
    object = p4t_galloc(0x4c, 16);
    stream = p4t_galloc(512, 16);
    table = p4t_galloc(0x800, 16);
    if (!object || !stream || !table) return 1;
    if (check(object, stream, table, "40", "0 123 456", 1, 0x123, 0x456) ||
        check(object, stream, table, "100000040", "0 100000123 100000456", 1, 0x123, 0x456) ||
        check(object, stream, table, "-1", "0 123 456", 0, 0x11, 0x22) ||
        check(object, stream, table, "-1", "0 123 456", 1, 0x11, 0x22) ||
        check(object, stream, table, "80000000", "0 123 456", 2, 0x11, 0x22) ||
        check(object, stream, table, "10000000000000000", "0 123 456", 0, 0x11, 0x22)) return 1;
    puts("SFXZ real mirrored caller: original %lx formats, 64-bit slots, low-word fields/index and saturation passed");
    return 0;
}
