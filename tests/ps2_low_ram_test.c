/* Port-owned null-frame semantics, exercised through the production mirror and
 * hierarchy/bounding-sphere overrides. No retail image or kernel data needed. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/ps2_low_ram.h"

int RwFrameDirty(const void *frame);
void *RwFrameGetLTM(void *frame);
void *func_003bfae0(void *object);
void func_003ed850(void *frame, unsigned flags);
void func_003ed960(void *frame);
int32_t func_004010c0(int32_t request, void *out, void *in, int32_t count);

#define AREA 0x01c00000u
#define CHECK(test) do { if (!(test)) { \
    fprintf(stderr, "ps2_low_ram: line %d: %s\n", __LINE__, #test); return 1; \
} } while (0)

static void word(unsigned char *p, unsigned offset, uint32_t value)
{
    memcpy(p + offset, &value, sizeof value);
}

static void scalar(unsigned char *p, unsigned offset, float value)
{
    memcpy(p + offset, &value, sizeof value);
}

static float value(const unsigned char *p, unsigned offset)
{
    float out;
    memcpy(&out, p + offset, sizeof out);
    return out;
}

static void identity(unsigned char *matrix)
{
    memset(matrix, 0, 64);
    scalar(matrix, 0, 1.0f);
    scalar(matrix, 20, 1.0f);
    scalar(matrix, 40, 1.0f);
}

static int zero_transform(const unsigned char *matrix)
{
    unsigned row, column;
    for (row = 0; row < 4; row++) {
        for (column = 0; column < 3; column++) {
            if (value(matrix, row * 16 + column * 4) != 0.0f) return 0;
        }
    }
    return 1;
}

static int test_main(int argc, char **argv)
{
    unsigned i;
    unsigned char *frame, *parent, *child, *object;
    const unsigned char *matrix;
    unsigned char cached[16];
    (void)argc;
    (void)argv;

    /* The complete constructed range, byte and little-endian word readers,
     * root indirection, and the masked sync bits used by RwFrameDirty(NULL). */
    for (i = 0; i < 256; i++) {
        CHECK(*p4_ps2_low_ram_ptr(i) == 0);
        CHECK(p4_ps2_low_ram_u8(i) == 0);
        CHECK(p4_ps2_low_ram_u32(i) == 0);
    }
    CHECK(p4_ps2_low_ram_u32(0xA0) == 0);
    CHECK((p4_ps2_low_ram_u8(p4_ps2_low_ram_u32(0xA0) + 3) & 3) == 0);
    CHECK(RwFrameDirty(NULL) == 0);
    matrix = RwFrameGetLTM(NULL);
    CHECK(matrix != NULL);
    CHECK(((uintptr_t)matrix & 15u) == 0);
    for (i = 0; i < 64; i++) CHECK(matrix[i] == 0);

    if (p4t_guest_map(AREA, 0x1000) != 0) return 1;
    if (p4t_guest_map(0x00764000u, 0x1000) != 0) return 1;
    frame = P4_G2H(AREA);
    parent = frame + 0x100;
    child = frame + 0x200;
    object = frame + 0x300;
    memset(frame, 0, 0x600);
    /* As render_smoke does for Im3D, point the real module records at owned
     * guest storage. Addresses come from decomp/config/symbol_data_addrs.txt:
     * matrixModule=0x764858, vectorModule=0x764890, ourGlobals=0x8872E0.
     * FINALIZESTART installs the production sky math callbacks without opening
     * a GL device; no test double or extracted engine image is required. */
    word(P4_G2H(0x00764858u), 0, AREA + 0x500u - 0x008872E0u);
    word(P4_G2H(0x00764858u), 4, 1);
    word(P4_G2H(0x00764890u), 0, AREA + 0x400u - 0x008872E0u);
    word(P4_G2H(0x00764890u), 4, 1);
    CHECK(func_004010c0(17, NULL, NULL, 0) == 1);

    /* A real frame with no root is clean, irrespective of its own flag byte;
     * the GetLTM root test must not try to sync a null hierarchy. */
    frame[3] = 3;
    identity(frame + 0x50);
    scalar(frame, 0x80, 7.0f);
    CHECK(RwFrameDirty(frame) == 0);
    CHECK(RwFrameGetLTM(frame) == frame + 0x50);
    CHECK(value(frame, 0x80) == 7.0f);
    word(frame, 0xA0, P4_H2G(parent));
    parent[3] = 3;
    CHECK(RwFrameDirty(frame) == 3);
    word(frame, 0xA0, 0);

    /* Bounding sphere: clean null frame keeps its cached result. Dirty sphere
     * reads the zero matrix's basis, flags and translation, not kernel words. */
    memset(object + 0x2C, 0x5A, sizeof cached);
    memcpy(cached, object + 0x2C, sizeof cached);
    CHECK(func_003bfae0(object) == object + 0x2C);
    CHECK(memcmp(cached, object + 0x2C, sizeof cached) == 0);
    object[3] = 1;
    scalar(object, 0x1C, 2.0f);
    scalar(object, 0x20, -3.0f);
    scalar(object, 0x24, 4.0f);
    scalar(object, 0x28, 5.0f);
    CHECK(func_003bfae0(object) == object + 0x2C);
    for (i = 0; i < 4; i++) CHECK(value(object, 0x2C + 4 * i) == 0.0f);
    CHECK((object[3] & 1) == 0);

    /* The sibling walker and the root/child walker each multiply by a null
     * parent's zero LTM. Test translation too: address 0x80 is not exempt. */
    memset(frame, 0, 0x300);
    frame[3] = 4;
    identity(frame + 0x10);
    memset(frame + 0x50, 0x5A, 64);
    func_003ed850(frame, 0);
    CHECK(zero_transform(frame + 0x50));
    CHECK((frame[3] & 4) == 0);
    word(frame, 0x98, P4_H2G(child));
    child[3] = 4;
    identity(child + 0x10);
    memset(child + 0x50, 0x5A, 64);
    func_003ed960(frame);
    CHECK(zero_transform(child + 0x50));
    CHECK((child[3] & 4) == 0);

    /* Non-null parents still supply their actual world transform. */
    identity(parent + 0x50);
    scalar(parent, 0x80, 3.0f);
    scalar(parent, 0x84, 4.0f);
    scalar(parent, 0x88, 5.0f);
    word(child, 4, P4_H2G(parent));
    child[3] = 4;
    func_003ed850(child, 0);
    CHECK(value(child, 0x50) == 1.0f);
    CHECK(value(child, 0x80) == 3.0f);
    CHECK(value(child, 0x84) == 4.0f);
    CHECK(value(child, 0x88) == 5.0f);
    puts("ps2_low_ram: 256 zero bytes; production root, sync, sphere and hierarchy reads passed");
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
