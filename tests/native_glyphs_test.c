#include "p4pc/native_glyphs.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t list_address, releases, palette_seen;
static int32_t width_pixels;
static int fail_create;
uint32_t frFontExCreateText(float x, float y, float depth, int32_t color, int8_t palette,
    int32_t style, const char *text, int32_t flags, int32_t *width)
{
    assert(!x && !y && !depth && !color && style == 1 && flags == 1 && width);
    assert(!strcmp(text, "AV wide"));
    assert(*(uint32_t *)P4_G2H(0x007645a0) == 96);
    assert(!(*(uint16_t *)P4_G2H(0x00763810) & 4));
    *(uint32_t *)P4_G2H(0x00764594) = 0;
    *(uint32_t *)P4_G2H(0x007637f8) = 0;
    *(uint16_t *)P4_G2H(0x00763810) = 2;
    palette_seen = (uint8_t)palette; *width = width_pixels;
    return fail_create ? 0 : list_address;
}
uint32_t frFontListRelease(uint32_t list)
{ assert(list == list_address); ++releases; return 0; }

static void syntax(void)
{
    for (unsigned c = 0; c < 256; ++c) {
        uint8_t text[] = {(uint8_t)c, 0};
        int valid = c >= 32 && c < 127;
        assert((p4_native_glyphs_validate(text, sizeof text, P4_GLYPHS_RAW, 0) == 0) == valid);
    }
    for (unsigned lead = 0x80; lead < 256; ++lead)
        for (unsigned trail = 0x80; trail < 256; ++trail) {
            uint8_t pair[] = {(uint8_t)lead, (uint8_t)trail, 0};
            assert((p4_native_glyphs_validate(pair, sizeof pair, P4_GLYPHS_MESSAGE, 0) == 0) == (lead < 0xf0));
            assert((p4_native_glyphs_validate(pair, sizeof pair, P4_GLYPHS_RAW, 0) == 0) == (lead < 0xc0));
        }
    uint8_t newline[] = {'A', 10, 'B', 0}, truncated[] = {0x83, 0};
    uint8_t embedded[] = {'A', 0, 'B', 0};
    assert(p4_native_glyphs_validate(newline, sizeof newline, P4_GLYPHS_RAW, 1) == -1);
    assert(!p4_native_glyphs_validate(newline, sizeof newline, P4_GLYPHS_MESSAGE, 1));
    assert(p4_native_glyphs_validate(newline, sizeof newline, P4_GLYPHS_MESSAGE, 0) == -1);
    assert(p4_native_glyphs_validate(truncated, sizeof truncated, P4_GLYPHS_RAW, 0) == -1);
    assert(p4_native_glyphs_validate(embedded, sizeof embedded, P4_GLYPHS_RAW, 0) == -1);
    assert(p4_native_glyphs_validate(NULL, 0, P4_GLYPHS_RAW, 0) == -1);
    assert(p4_native_glyphs_validate((const uint8_t *)"A", 2, 0, 0) == -1);
}
static void state_preserved(void)
{
    assert(*(uint32_t *)P4_G2H(0x007645a0) == 96);
    assert(*(uint32_t *)P4_G2H(0x00764594) == 3);
    assert(*(uint32_t *)P4_G2H(0x007637f8) == 88);
    assert(*(uint16_t *)P4_G2H(0x00763810) == 0x14);
}
int main(void)
{
    syntax();
    if (p4t_guest_map(0x00763000, 0x3000) || p4t_guest_init()) return 77;
    uint8_t *text = p4t_galloc(8, 16); memcpy(text, "AV wide", 8);
    list_address = P4_H2G(p4t_galloc(64, 16));
    *(uint32_t *)P4_G2H(0x007645a0) = 96;
    *(uint32_t *)P4_G2H(0x00764594) = 3;
    *(uint32_t *)P4_G2H(0x007637f8) = 88;
    *(uint16_t *)P4_G2H(0x00763810) = 0x14;
    width_pixels = 42;
    assert(p4_native_glyphs_measure_raw(text, 1, 8) == 672 && palette_seen == 8); state_preserved();
    assert(p4_native_glyphs_measure_raw(text, 1, 6) == 672 && palette_seen == 6); state_preserved();
    width_pixels = 0;
    assert(p4_native_glyphs_measure_raw(text, 1, 8) == 0); state_preserved();
    width_pixels = INT32_MAX;
    assert(p4_native_glyphs_measure_raw(text, 1, 8) == -1); state_preserved();
    fail_create = 1;
    assert(p4_native_glyphs_measure_raw(text, 1, 8) == -1); state_preserved();
    assert(releases == 4);
    puts("native_glyphs: RAW/MSG syntax and exact Config measure/release/state recipe passed");
    return 0;
}
