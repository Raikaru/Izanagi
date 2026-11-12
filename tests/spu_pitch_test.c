/* LIBSD Note2Pitch: native vectors plus optional unchanged-IRX differential.
 * No imports are hooked and no IRX instruction or data word is relocated.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "guest_test.h"
#include "spu_pitch.h"
#include "../tools/mipsdiff/r5900.h"

static uint32_t le32(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int load_irx(const char *path)
{
    FILE *file = fopen(path, "rb");
    unsigned char header[52], section[40];
    unsigned stride, count;
    uint32_t offset;
    if (!file)
        return -1;
    if (fread(header, 1, sizeof header, file) != sizeof header || memcmp(header, "\177ELF\1\1", 6)) {
        fclose(file);
        return -1;
    }
    offset = le32(header + 32);
    stride = header[46] | (unsigned)header[47] << 8;
    count = header[48] | (unsigned)header[49] << 8;
    for (unsigned i = 0; i < count; i++) {
        uint32_t kind, flags, address, position, size, skip = 0;
        if (fseek(file, offset + i * stride, SEEK_SET) || fread(section, 1, sizeof section, file) != sizeof section) {
            fclose(file);
            return -1;
        }
        kind = le32(section + 4);
        flags = le32(section + 8);
        address = le32(section + 12);
        position = le32(section + 16);
        size = le32(section + 20);
        if (!(flags & 2) || !size || (kind != 1 && kind != 8))
            continue;
        if (address < 0x1000u) {
            skip = 0x1000u - address;
            if (skip >= size)
                continue;
            address += skip;
            size -= skip;
        }
        if (address > 0x00200000u - size) {
            fclose(file);
            return -1;
        }
        if (kind == 8)
            memset(P4_G2H(address), 0, size);
        else if (fseek(file, position + skip, SEEK_SET) || fread(P4_G2H(address), 1, size, file) != size) {
            fclose(file);
            return -1;
        }
    }
    fclose(file);
    return 0;
}

static int native_vectors(void)
{
    static const struct { int32_t input; uint16_t pitch; } cases[] = {
        {0, 0x1000}, {1, 0x1002}, {-1, 0x0ffe},
        {128, 0x10f4}, {-128, 0x0f1a},
        {1536, 0x2000}, {-1536, 0x0800},
        {3072, 0x4000}, {-12288, 0x0010}, {-32768, 0x0000}
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        uint16_t actual = p4spu_pitch_for_offset(cases[i].input);
        if (actual != cases[i].pitch) {
            fprintf(stderr, "SPU pitch %ld: got%04x expected%04x\n", (long)cases[i].input, actual, cases[i].pitch);
            return 1;
        }
        if (p4spu_pitch_rate(48000.0, cases[i].input) != 48000.0 / 4096.0 * cases[i].pitch)
            return 1;
    }
    puts("SPU pitch: native integer-table/rounding vectors passed");
    return 0;
}

static int compare(int32_t input, unsigned *count, uint64_t *digest)
{
    R5900 cpu;
    uint16_t actual = p4spu_pitch_for_offset(input);
    int status;
    memset(&cpu, 0, sizeof cpu);
    cpu.r[6] = (uint16_t)(input / 128);
    cpu.r[7] = (uint64_t)(int64_t)(input % 128);
    cpu.r[29] = 0x001ff000u;
    cpu.lo_pc = 0x1000;
    cpu.hi_pc = 0x00200000;
    cpu.max_steps = 10000;
    status = r5900_call(&cpu, 0x3bd0);
    if (status != R5900_OK || actual != cpu.r[2]) {
        fprintf(stderr, "SPU pitch offset%ld native=%04x retail=%04llx status=%d pc=%08x instruction=%08x\n",
                (long)input, actual, (unsigned long long)cpu.r[2], status, (unsigned)cpu.bad_pc, (unsigned)cpu.bad_word);
        return 1;
    }
    (*count)++;
    *digest = (*digest ^ (actual & 255u)) * UINT64_C(1099511628211);
    *digest = (*digest ^ (actual >> 8)) * UINT64_C(1099511628211);
    return 0;
}

int main(int argc, char **argv)
{
    const char *path;
    unsigned count = 0;
    uint64_t digest = UINT64_C(1469598103934665603);
    if (native_vectors())
        return 1;
    if (argc == 1)
        return 0;
    path = !strcmp(argv[1], "--require-retail") ? getenv("P4PC_LIBSD_IRX") : argv[1];
    if (!path || !*path)
        return 77;
    if (p4t_guest_map(0x1000, 0x1ff000) != 0)
        return 77; /* low identity maps may be forbidden on an i386 host */
    if (load_irx(path)) {
        fprintf(stderr, "cannot load actual LIBSD.IRX %s\n", path);
        return 1;
    }
    for (int32_t input = -32768; input < 32768; input++)
        if (compare(input, &count, &digest))
            return 1;
    {
        static const int32_t extremes[] = {INT32_MIN, INT32_MIN + 1, -1000000000, 1000000000, INT32_MAX - 1, INT32_MAX};
        for (unsigned i = 0; i < sizeof extremes / sizeof extremes[0]; i++)
            if (compare(extremes[i], &count, &digest))
                return 1;
    }
    printf("SPU pitch: %u unchanged-IRX exact comparisons, digest=%016llx\n", count, (unsigned long long)digest);
    return 0;
}
