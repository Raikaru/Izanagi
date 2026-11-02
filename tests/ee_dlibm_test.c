/* Compare native soft-double libm with the unchanged retail ELF instructions.
 * No calls are hooked: the interpreter runs the entire fp-bit callee closure.
 * Every return bit (including NaN payload/sign and signed zero) must agree.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/ee_dlibm.h"
#include "p4pc/hostmath.h"
#include "../src/platform/ps2_fpbit.h"
#include "../tools/mipsdiff/r5900.h"

static unsigned comparisons, failures;
static uint64_t digest = UINT64_C(1469598103934665603);

static uint32_t word(const unsigned char *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static int load_retail(const char *path)
{
    unsigned char header[52], ph[32];
    FILE *file = fopen(path, "rb");
    unsigned count, stride;
    uint32_t offset;
    if (!file)
        return -1;
    if (fread(header, 1, sizeof header, file) != sizeof header ||
        memcmp(header, "\177ELF\1\1", 6) != 0) {
        fclose(file);
        return -1;
    }
    offset = word(header + 28);
    stride = header[42] | (unsigned)header[43] << 8;
    count = header[44] | (unsigned)header[45] << 8;
    for (unsigned i = 0; i < count; i++) {
        uint32_t addr, size, memsize, file_offset;
        if (fseek(file, offset + i * stride, SEEK_SET) != 0 ||
            fread(ph, 1, sizeof ph, file) != sizeof ph) {
            fclose(file);
            return -1;
        }
        if (word(ph) != 1)
            continue;
        file_offset = word(ph + 4);
        addr = word(ph + 8);
        size = word(ph + 16);
        memsize = word(ph + 20);
        if (addr < 0x00100000u || memsize < size || addr > 0x02000000u - memsize ||
            fseek(file, file_offset, SEEK_SET) != 0 ||
            fread(P4_G2H(addr), 1, size, file) != size) {
            fclose(file);
            return -1;
        }
        memset((unsigned char *)P4_G2H(addr) + size, 0, memsize - size);
    }
    fclose(file);
    return 0;
}

static uint64_t retail(uint32_t address, uint64_t x, uint64_t y)
{
    R5900 cpu;
    int status;
    memset(&cpu, 0, sizeof cpu);
    cpu.r[4] = x;
    cpu.r[5] = y;
    cpu.r[29] = 0x01fff000u;
    cpu.lo_pc = 0x00100000u;
    cpu.hi_pc = 0x00600000u;
    cpu.max_steps = 1000000;
    status = r5900_call(&cpu, address);
    if (status != R5900_OK) {
        fprintf(stderr, "retail double-libm stopped: function=%08x status=%d pc=%08x instruction=%08x\n",
                (unsigned)address, status, (unsigned)cpu.bad_pc, (unsigned)cpu.bad_word);
        exit(1);
    }
    return cpu.r[2];
}

static double as_double(uint64_t bits)
{
    double value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static uint64_t as_bits(double value)
{
    uint64_t bits;
    memcpy(&bits, &value, sizeof bits);
    return bits;
}

static void compare(uint32_t address, uint64_t x, uint64_t y, uint64_t actual)
{
    uint64_t expected = retail(address, x, y);
    uint64_t adapted = actual;
    switch (address) {
    case 0x004470f0: adapted = as_bits(p4pc_ee_acos(as_double(x))); break;
    case 0x004476b0: adapted = as_bits(p4pc_ee_log(as_double(x))); break;
    case 0x00447b88: adapted = as_bits(p4pc_ee_log10(as_double(x))); break;
    case 0x00447d00: adapted = as_bits(p4pc_ee_pow(as_double(x), as_double(y))); break;
    case 0x00448a00: adapted = as_bits(p4pc_ee_dsqrt(as_double(x))); break;
    }
    if (adapted != actual) {
        fprintf(stderr, "%08x: native-double register adapter changed return bits\n", (unsigned)address);
        failures++;
    }
    comparisons++;
    for (unsigned i = 0; i < 8; i++)
        digest = (digest ^ ((actual >> (8 * i)) & 255u)) * UINT64_C(1099511628211);
    if (actual != expected) {
        if (failures < 12)
            fprintf(stderr, "%08x(%016llx,%016llx): native=%016llx retail=%016llx\n",
                    (unsigned)address, (unsigned long long)x, (unsigned long long)y,
                    (unsigned long long)actual, (unsigned long long)expected);
        failures++;
    }
}

static const uint64_t edges[] = {
    UINT64_C(0x0000000000000000), UINT64_C(0x8000000000000000),
    UINT64_C(0x0000000000000001), UINT64_C(0x8000000000000001),
    UINT64_C(0x000fffffffffffff), UINT64_C(0x0010000000000000),
    UINT64_C(0x0010000000000001), UINT64_C(0x7fefffffffffffff),
    UINT64_C(0xffefffffffffffff), UINT64_C(0x7ff0000000000000),
    UINT64_C(0xfff0000000000000), UINT64_C(0x7ff8000000000000),
    UINT64_C(0xfff8000012345678), UINT64_C(0x7ff0000000000001),
    UINT64_C(0xfff0000012345678), UINT64_C(0x3ff0000000000000),
    UINT64_C(0xbff0000000000000), UINT64_C(0x3fe0000000000000),
    UINT64_C(0xbfe0000000000000), UINT64_C(0x3fefffffffffffff),
    UINT64_C(0x3ff0000000000001), UINT64_C(0xbfefffffffffffff),
    UINT64_C(0xbff0000000000001), UINT64_C(0x4000000000000000),
    UINT64_C(0xc000000000000000), UINT64_C(0x4008000000000000),
    UINT64_C(0xc008000000000000), UINT64_C(0x3fdfffffffffffff),
    UINT64_C(0x3fe0000000000001), UINT64_C(0x3c60000000000000),
    UINT64_C(0x3c60000000000001), UINT64_C(0x4024000000000000),
    UINT64_C(0x408e000000000000), UINT64_C(0x4148b4dc00000000),
    UINT64_C(0x41dfffffffc00000), UINT64_C(0x41e0000000000000),
    UINT64_C(0xc1e0000000000000), UINT64_C(0x4340000000000000),
    UINT64_C(0x433fffffffffffff), UINT64_C(0x43f0000000000000),
    UINT64_C(0x4090000000000000), UINT64_C(0xc090cc0000000000),
    UINT64_C(0x3e45798ee2308c3a), UINT64_C(0x7fd0000000000000)
};

static void unary(uint64_t x)
{
    compare(0x004470f0, x, 0, func_004470f0(x));
    compare(0x004476b0, x, 0, func_004476b0(x));
    compare(0x00447b88, x, 0, func_00447b88(x));
    compare(0x00448a00, x, 0, func_00448a00(x));
    compare(0x0044e708, x, 0, (uint64_t)(int64_t)dptoli(x));
}

static void sweep(void)
{
    const unsigned count = sizeof edges / sizeof edges[0];
    uint64_t state = UINT64_C(0x9e3779b97f4a7c15);
    for (unsigned i = 0; i < count; i++) {
        unary(edges[i]);
        for (unsigned j = 0; j < count; j++) {
            compare(0x00447d00, edges[i], edges[j], func_00447d00(edges[i], edges[j]));
            compare(0x0044e0a8, edges[i], edges[j], func_0044e0a8(edges[i], edges[j]));
            compare(0x0044e3b8, edges[i], edges[j], dpdiv(edges[i], edges[j]));
            compare(0x0044e608, edges[i], edges[j], (uint64_t)(int64_t)dpcmp(edges[i], edges[j]));
        }
    }
    for (unsigned i = 0; i < 256; i++) {
        uint64_t x, y;
        state = state * UINT64_C(6364136223846793005) + 1;
        x = state;
        state = state * UINT64_C(6364136223846793005) + 1;
        y = state;
        unary(x);
        compare(0x00447d00, x, y, func_00447d00(x, y));
        compare(0x0044e658, (uint64_t)(int64_t)(int32_t)x, 0, litodp((int32_t)x));
        /* Dense ordinary inputs exercise the rational/polynomial paths, not
         * just the special-value and huge-exponent fast exits. */
        x = UINT64_C(0x3fe0000000000000) | (x & UINT64_C(0x000fffffffffffff));
        y = UINT64_C(0x3ff0000000000000) | (y & UINT64_C(0x000fffffffffffff));
        unary(x | ((uint64_t)(i & 1u) << 63));
        compare(0x00447d00, y, x, func_00447d00(y, x));
    }
}

int main(int argc, char **argv)
{
    const char *elf = argc > 1 ? argv[1] : getenv("P4PC_ELF");
    if (!elf || !*elf)
        return 77;
    if (p4t_guest_map(0x00100000u, 0x01f00000u) != 0 || load_retail(elf) != 0) {
        fprintf(stderr, "ee_dlibm_test: cannot load retail ELF %s\n", elf);
        return 1;
    }
    sweep();
    if (p4pc_fpu_ee_mode())
        sweep();
    printf("retail double-libm: %u byte-exact comparisons, %u failures, digest=%016llx\n",
           comparisons, failures, (unsigned long long)digest);
    return failures != 0;
}
