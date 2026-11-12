#include "p4pc/game_text.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint32_t allocations, releases, requested;
static int fail_allocate;
static uint32_t captured[9], list_address;
static float drawn_scale;
static uint32_t drawn_color, drawn_aux, drawn_slot, draw_count, list_releases;
static int32_t width_result;
static uint32_t expected_font, expected_vertical, build_count, width_queries;
static uint32_t expected_multiline;
static const P4TextProfile profile = {P4_TEXT_ENCODING_US_NATIVE, P4_TEXT_FONT_US_NATIVE};
static uint8_t *queued_node, *queue_target;
static uint32_t append_count, pair_count, node_width_count, second_address;
static float second_x, second_y;


static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    assert(user == &allocations); ++allocations; requested = size;
    if (fail_allocate) return NULL;
    void *host = p4t_galloc(size, 16); assert(host);
    *guest = P4_H2G(host); return host;
}
static void release(void *user, uint32_t guest, void *host, uint32_t size)
{
    assert(user == &allocations && P4_G2H(guest) == host && size == requested); ++releases;
}
void *frFontBuildTextFull(uint32_t x, uint32_t y, uint32_t duration, uint32_t style,
    uint32_t palette, uint32_t effect, uint32_t intensity, uint32_t stream, uint32_t list)
{
    assert(*(uint32_t *)P4_G2H(0x007645a0) == expected_font);
    assert(*(uint32_t *)P4_G2H(0x007645a4) == expected_vertical);
    ++build_count;
    assert(!!(*(uint16_t *)P4_G2H(0x00763810) & 4u) == expected_multiline);
    *(uint32_t *)P4_G2H(0x00764594) = 0;
    *(uint32_t *)P4_G2H(0x007637f8) = 0;
    *(uint16_t *)P4_G2H(0x00763810) |= 2;
    uint32_t args[] = {x, y, duration, style, palette, effect, intensity, stream, list};
    memcpy(captured, args, sizeof captured);
    assert(!strcmp(P4_G2H(stream), "Native card"));
    return P4_G2H(list_address);
}
int32_t itfMesListWidth(int32_t list) { assert((uint32_t)list == list_address); ++width_queries; return width_result; }
int32_t frFontListKeepOrRelease(int32_t list) { assert((uint32_t)list == list_address); ++list_releases; return 0; }
void frFontSetGlyphAux(int32_t list, uint8_t palette) { assert((uint32_t)list == list_address); drawn_aux = palette; }
void frFontSetGlyphColor(int32_t list, uint32_t color) { assert((uint32_t)list == list_address); drawn_color = color; }
void frFontExSetScale(int32_t list, float scale) { assert((uint32_t)list == list_address); drawn_scale = scale; }
void frFontSetListSlot(int32_t list, uint8_t slot) { assert((uint32_t)list == list_address); drawn_slot = slot; }
void frFontDrawListMode(void *list, uint32_t mode) { assert(P4_H2G(list) == list_address && mode == 1); ++draw_count; }
void itfMesWindowDrawCallback(uint8_t *node, int32_t list);
uint8_t *PrimAllocQueueNode(void)
{
    queued_node = p4t_galloc(0x30, 16); assert(queued_node);
    memset(queued_node, 0, 0x30);
    return queued_node;
}
void PrimAppendNode(uint8_t *queue, uint8_t *node)
{
    assert(queue == queue_target && node == queued_node);
    assert(P4_G32(P4_H2G(node) + 8u) == p4_fn_h2g_sig(itfMesWindowDrawCallback, "vpi"));
    assert(P4_G32(P4_H2G(node) + 16u) == list_address);
    assert(!P4_G32(P4_H2G(node) + 12u));
    ++append_count;
}
void itfMesWindowDrawCallback(uint8_t *node, int32_t list)
{
    assert(node == queued_node);
    frFontDrawListMode(P4_G2H((uint32_t)list), 1);
    frFontListKeepOrRelease(list);
}
int32_t frFontNodeWidth(void *list)
{
    assert(P4_H2G(list) == list_address); ++node_width_count; return 18;
}
void *itfMesWindowBuildEntry(float x, float y, float scale, int32_t color, int32_t style,
    int32_t effect, int32_t flags, int32_t bank, int32_t record)
{
    assert(scale == 0.75f && color == 0x12345678 && style == 0x103 && effect == 0x105);
    assert(flags == 1 && bank == 25 && record == 7);
    second_x = x; second_y = y; ++pair_count;
    return P4_G2H(second_address);
}
int32_t frFontAppendList(int32_t first, int32_t second, int32_t flags)
{
    assert((uint32_t)first == list_address && (uint32_t)second == second_address && !flags);
    return first;
}
static uint32_t raw_releases, raw_palette;
uint32_t frFontExCreateText(float x, float y, float depth, int32_t color, int8_t palette,
    int32_t style, const char *text, int32_t flags, int32_t *width)
{
    assert(!x && !y && !depth && !color && style == 1 && flags == 1 && width);
    assert(*(uint32_t *)P4_G2H(0x007645a0) == 17);
    assert(!strcmp(text, "Native card"));
    raw_palette = (uint8_t)palette; *width = 42;
    *(uint16_t *)P4_G2H(0x00763810) = 2;
    return list_address;
}
uint32_t frFontListRelease(uint32_t list) { assert(list == list_address); ++raw_releases; return 0; }

static void grammar(P4GameText *t)
{
    const uint8_t command[] = {0xf1, 0x01, 0};
    const uint8_t truncated[] = {0x80, 0};
    const uint8_t bad_trail[] = {0x80, 'a', 0};
    const uint8_t embedded_nul[] = {'a', 0, 'b', 0};
    const uint8_t no_nul[] = {'a'};
    assert(p4_game_tables_text_register(t, "cards", 0, 1536, 0, 1, command, sizeof command) == P4_TABLE_INVALID);
    assert(p4_game_tables_text_register(t, "cards", 0, 1536, 0, 1, truncated, sizeof truncated) == P4_TABLE_INVALID);
    assert(p4_game_tables_text_register(t, "cards", 0, 1536, 0, 1, bad_trail, sizeof bad_trail) == P4_TABLE_INVALID);
    assert(p4_game_tables_text_register(t, "cards", 0, 1536, 0, 1, embedded_nul, sizeof embedded_nul) == P4_TABLE_INVALID);
    assert(p4_game_tables_text_register(t, "cards", 0, 1536, 0, 1, no_nul, sizeof no_nul) == P4_TABLE_INVALID);
    assert(p4_game_tables_text_register(t, "cards", 0, 1536, 0, 1, (const uint8_t *)"Line\nName", 10) == P4_TABLE_INVALID);
    const uint8_t glyph[] = {0x83, 0xf6, 0};
    assert(p4_game_tables_text_register(t, "cards", 1, 1536, 0, 1, glyph, sizeof glyph) == P4_TABLE_OK);
    assert(!p4_native_glyphs_validate(glyph, sizeof glyph, P4_GLYPHS_RAW, 0));
    const uint8_t alternate[] = {0xc0, 0x80, 0};
    assert(!p4_native_glyphs_validate(alternate, sizeof alternate, P4_GLYPHS_MESSAGE, 0));
    assert(p4_native_glyphs_validate(alternate, sizeof alternate, P4_GLYPHS_RAW, 0) == -1);
    assert(p4_native_glyphs_validate((const uint8_t *)"Line\nTwo", 9, P4_GLYPHS_RAW, 1) == -1);
}
static void text_id_width(const P4GameTableAllocator *allocator)
{
    static const uint32_t ids[] = {65535u, 65536u, 0x80000000u, UINT32_MAX};
    static const uint8_t name[] = "Wide name";
    P4GameText *text = p4_game_tables_text_create(allocator, &profile); assert(text);
    for (size_t n = sizeof ids / sizeof ids[0]; n; --n)
        assert(p4_game_tables_text_register(text, "wide", P4_TEXT_SKILL, ids[n - 1],
            P4_TEXT_NAME, 1, name, sizeof name) == P4_TABLE_OK);
    assert(p4_game_tables_text_freeze(text, 1) == P4_TABLE_OK);
    for (size_t i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
        const P4GameTextRow *row = p4_game_tables_text_at(text, i);
        assert(row && row->id == ids[i]);
        assert(row == p4_game_tables_text_find(text, P4_TEXT_SKILL, ids[i], P4_TEXT_NAME));
        assert(!strcmp((const char *)P4_G2H(row->stream), (const char *)name));
        assert(!strcmp(p4_game_tables_text_owner(text, i), "wide"));
    }
    assert(!p4_game_tables_text_find(text, P4_TEXT_SKILL, 0, P4_TEXT_NAME));
    assert(!p4_game_tables_text_find(text, P4_TEXT_SKILL, 0x7fffffffu, P4_TEXT_NAME));
    p4_game_tables_text_destroy(text);
}

static void semantic_phase(const P4GameTableAllocator *allocator)
{
    unsigned before = allocations, freed = releases, builds = build_count, widths = width_queries, draws = draw_count;
    P4TextProfile raw = {P4_TEXT_ENCODING_US_RAW_NATIVE, P4_TEXT_FONT_US_NATIVE};
    P4GameText *text = p4_game_tables_text_create(allocator, &raw); assert(text);
    const uint8_t name[] = "Semantic", glyph[] = {0x83, 0xf6, 0};
    assert(p4_game_tables_text_register(text, "native.text", P4_TEXT_SKILL, UINT32_MAX, P4_TEXT_NAME, 4, glyph, sizeof glyph) == P4_TABLE_OK);
    assert(p4_game_tables_text_register(text, "native.text", P4_TEXT_SKILL, 2000, P4_TEXT_NAME, 3, name, sizeof name) == P4_TABLE_OK);
    P4GameTextDefinition definition = {.table = 0xdeadbeefu};
    assert(p4_game_tables_text_definition_find(text, P4_TEXT_SKILL, 2000, P4_TEXT_NAME, &definition) == P4_TABLE_NOT_READY);
    assert(p4_game_tables_text_definition_at(text, 0, &definition) == P4_TABLE_NOT_READY && definition.table == 0xdeadbeefu);
    assert(p4_game_tables_text_materialize(text) == P4_TABLE_NOT_READY && allocations == before);
    assert(p4_game_tables_text_seal(text, 11) == P4_TABLE_OK && allocations == before);
    assert(p4_game_tables_text_semantic_revision(text) == 11 && !p4_game_tables_text_revision(text));
    assert(!p4_game_tables_text_find(text, P4_TEXT_SKILL, 2000, P4_TEXT_NAME) && !p4_game_tables_text_at(text, 0));
    assert(!p4_game_tables_text_bytes(text, 0) && !p4_game_tables_text_owner(text, 0));
    assert(p4_game_tables_text_seal(text, 11) == P4_TABLE_OK && p4_game_tables_text_seal(text, 12) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_text_definition_find(text, P4_TEXT_SKILL, 2000, P4_TEXT_NAME, &definition) == P4_TABLE_OK);
    assert(definition.id == 2000 && definition.revision == 3 && definition.size == sizeof name);
    assert(!strcmp(definition.owner, "native.text") && !memcmp(definition.bytes, name, sizeof name));
    assert(p4_game_tables_text_definition_at(text, 1, &definition) == P4_TABLE_OK && definition.id == UINT32_MAX);
    assert(!memcmp(definition.bytes, glyph, sizeof glyph));
    assert(p4_game_tables_text_definition_at(text, 2, &definition) == P4_TABLE_NOT_FOUND && definition.id == UINT32_MAX);
    assert(p4_game_tables_text_register(text, "native.text", P4_TEXT_SKILL, 2001, P4_TEXT_NAME, 1, name, sizeof name) == P4_TABLE_FROZEN);
    fail_allocate = 1;
    assert(p4_game_tables_text_materialize(text) == P4_TABLE_NOMEM && !p4_game_tables_text_revision(text));
    assert(p4_game_tables_text_semantic_revision(text) == 11 && !p4_game_tables_text_at(text, 0));
    assert(p4_game_tables_text_definition_find(text, P4_TEXT_SKILL, 2000, P4_TEXT_NAME, &definition) == P4_TABLE_OK);
    assert(!memcmp(definition.bytes, name, sizeof name));
    fail_allocate = 0;
    assert(p4_game_tables_text_materialize(text) == P4_TABLE_OK && p4_game_tables_text_revision(text) == 11);
    assert(p4_game_tables_text_semantic_revision(text) == 11);
    assert(p4_game_tables_text_definition_find(text, P4_TEXT_SKILL, 2000, P4_TEXT_NAME, &definition) == P4_TABLE_OK);
    const P4GameTextRow *row = p4_game_tables_text_find(text, P4_TEXT_SKILL, 2000, P4_TEXT_NAME);
    assert(row && definition.bytes == P4_G2H(row->stream) && !memcmp(definition.bytes, name, sizeof name));
    assert(p4_game_tables_text_materialize(text) == P4_TABLE_FROZEN && allocations == before + 2);
    p4_game_tables_text_destroy(text);
    assert(releases == freed + 1 && builds == build_count && widths == width_queries && draws == draw_count);
    text = p4_game_tables_text_create(NULL, &raw); assert(text);
    assert(p4_game_tables_text_seal(text, 7) == P4_TABLE_OK && !p4_game_tables_text_revision(text));
    assert(p4_game_tables_text_materialize(text) == P4_TABLE_OK && p4_game_tables_text_revision(text) == 7);
    assert(allocations == before + 2); p4_game_tables_text_destroy(text);
}

int main(void)
{
    if (p4t_guest_map(0x00763000, 0x3000) || p4t_guest_init()) return 77;
    P4GameTableAllocator allocator = {&allocations, allocate, release};
    const P4TextProfile unsupported = {3, P4_TEXT_FONT_US_NATIVE};
    assert(!p4_game_tables_text_create(&allocator, &unsupported));
    assert(!p4_game_tables_text_create(&allocator, NULL));
    P4GameText *empty = p4_game_tables_text_create(NULL, &profile); assert(empty);
    assert(p4_game_tables_text_freeze(empty, 1) == P4_TABLE_OK && !allocations);
    p4_game_tables_text_destroy(empty);
    P4GameText *t = p4_game_tables_text_create(&allocator, &profile); assert(t);
    grammar(t);
    uint8_t name[] = "Native card";
    assert(p4_game_tables_text_register(t, "cards", 0, 1536, 0, 3, name, sizeof name) == P4_TABLE_OK);
    name[0] = 'X';
    assert(p4_game_tables_text_register(t, "other", 0, 1536, 0, 3, name, sizeof name) == P4_TABLE_CONFLICT);
    const uint8_t desc[] = "Authored description\nSecond line";
    assert(p4_game_tables_text_register(t, "cards", 0, 1536, 1, 4, desc, sizeof desc) == P4_TABLE_OK);
    assert(!p4_game_tables_text_find(t, 0, 1536, 0));
    fail_allocate = 1;
    assert(p4_game_tables_text_freeze(t, 9) == P4_TABLE_NOMEM);
    assert(!p4_game_tables_text_revision(t) && !p4_game_tables_text_at(t, 0));
    assert(p4_game_tables_text_semantic_revision(t) == 9); /* Failed guest work never reopens CPU definitions. */
    fail_allocate = 0;
    assert(p4_game_tables_text_freeze(t, 9) == P4_TABLE_OK);
    assert(p4_game_tables_text_revision(t) == 9 && p4_game_tables_text_count(t) == 3);
    assert(p4_game_tables_text_profile(t)->encoding == profile.encoding);
    assert(p4_game_tables_text_profile(t)->font == profile.font);
    assert(p4_game_tables_text_freeze(t, 10) == P4_TABLE_FROZEN);
    assert(p4_game_tables_text_register(t, "cards", 0, 2000, 0, 3, name, sizeof name) == P4_TABLE_FROZEN);
    const P4GameTextRow *row = p4_game_tables_text_find(t, 0, 1536, 0); assert(row);
    assert(row->revision == 3 && row->size == 12);
    assert(!strcmp(P4_G2H(row->stream), "Native card"));
    for (size_t i = 0; i < 3; ++i) {
        const P4GameTextRow *r = p4_game_tables_text_at(t, i); assert(r);
        assert(!memcmp(P4_G2H(r->stream), p4_game_tables_text_bytes(t, i), r->size));
        assert(!strcmp(p4_game_tables_text_owner(t, i), "cards"));
    }
    assert(!p4_game_tables_text_at(t, 3) && !p4_game_tables_text_bytes(t, 3) && !p4_game_tables_text_owner(t, 3));
    assert(!p4_game_tables_text_find(t, 0, 1537, 0));
    list_address = P4_H2G(p4t_galloc(64, 16));
    uint32_t *font = P4_G2H(0x007645a0); *font = 0;
    *(uint32_t *)P4_G2H(0x007645a4) = expected_vertical = 37;
    *(uint32_t *)P4_G2H(0x007645a8) = 23;
    *(uint32_t *)P4_G2H(0x00764594) = 3;
    *(uint32_t *)P4_G2H(0x007637f8) = 88;
    *(uint16_t *)P4_G2H(0x00763810) = 0x14;
    expected_font = 0; width_result = 1279;
    assert(p4_game_tables_text_measure(row->stream, 0x103, 0) == 1279);
    assert(!*font && list_releases == 1 && !draw_count);
    assert(captured[0] == 0 && captured[1] == 0 && captured[2] == 0 && captured[3] == 3);
    assert(captured[4] == 0 && captured[5] == 0 && captured[6] == 255 && captured[7] == row->stream && !captured[8]);
    *font = 17; expected_font = 17; width_result = 2001;
    assert(p4_game_tables_text_measure(row->stream, 3, 0) == 2001 && *font == 17);
    p4_game_tables_text_draw(row->stream, 12.5f, -4.0f, 0.75f, 0x12345678, 0x103, 0x105, 1, 0);
    assert(captured[0] == 200 && (int32_t)captured[1] == -32 && captured[3] == 3 && captured[4] == 5);
    assert(drawn_aux == 5 && drawn_color == 0x12345678 && drawn_slot == 3 && drawn_scale == 0.75f);
    assert(draw_count == 1 && list_releases == 3 && *font == 17);
    expected_multiline = 1;
    assert(p4_game_tables_text_measure(row->stream, 3, 1) == 2001);
    assert(list_releases == 4 && *font == 17);
    assert(*(uint32_t *)P4_G2H(0x00764594) == 3);
    assert(*(uint32_t *)P4_G2H(0x007637f8) == 88);
    assert(*(uint16_t *)P4_G2H(0x00763810) == 0x14);
    assert(p4_native_glyphs_measure_raw(P4_G2H(row->stream), 1, 8) == 42 * 16 && raw_palette == 8);
    assert(p4_native_glyphs_measure_raw(P4_G2H(row->stream), 1, 6) == 42 * 16 && raw_palette == 6);
    assert(raw_releases == 2 && draw_count == 1 && *font == 17);
    assert(*(uint16_t *)P4_G2H(0x00763810) == 0x14);
    expected_multiline = 0;
    queue_target = p4t_galloc(16, 16); assert(queue_target);
    p4_game_tables_text_queue(row->stream, 12.5f, -4.0f, 0.75f, 0x12345678,
        0x103, 0x105, 1, 0, P4_H2G(queue_target));
    assert(append_count == 1 && draw_count == 1 && list_releases == 4);
    assert(drawn_slot == 3 && *font == 17);
    assert(*(uint32_t *)P4_G2H(0x00764594) == 3 && *(uint32_t *)P4_G2H(0x007637f8) == 88);
    assert(*(uint16_t *)P4_G2H(0x00763810) == 0x14);
    itfMesWindowDrawCallback(queued_node, (int32_t)P4_G32(P4_H2G(queued_node) + 16u));
    assert(draw_count == 2 && list_releases == 5);
    second_address = P4_H2G(p4t_galloc(64, 16)); assert(second_address);
    P4_G32(list_address + 0x24u) = 1;
    p4_game_tables_text_draw_pair(row->stream, 12.5f, -4.0f, 0.75f, 0x12345678,
        0x103, 0x105, 1, 0, 25, 7);
    assert(pair_count == 1 && node_width_count == 1 && second_x == 30.5f && second_y == 21.0f);
    assert(draw_count == 3 && list_releases == 6 && drawn_slot == 3);
    P4_G32(list_address + 0x24u) = 0;
    p4_game_tables_text_draw_pair(row->stream, 12.5f, -4.0f, 0.75f, 0x12345678,
        0x103, 0x105, 1, 0, 25, 7);
    assert(pair_count == 2 && node_width_count == 1 && second_x == 12.5f && second_y == 21.0f);
    p4_game_tables_text_draw_pair(row->stream, 12.5f, -4.0f, 0.75f, 0x12345678,
        0x103, 0x105, 1, 0, 25, 0);
    assert(pair_count == 2 && node_width_count == 1 && draw_count == 5 && list_releases == 8);
    assert(*font == 17 && *(uint32_t *)P4_G2H(0x00764594) == 3);
    assert(*(uint32_t *)P4_G2H(0x007637f8) == 88 && *(uint16_t *)P4_G2H(0x00763810) == 0x14);
    expected_font = expected_vertical = 128; width_result = 1027;
    assert(p4_game_tables_text_measure_geometry(row->stream, 1, 0, 128, 128) == 1027);
    assert(captured[3] == 1 && list_releases == 9 && *font == 17 && *(uint32_t *)P4_G2H(0x007645a4) == 37);
    expected_font = 64; expected_vertical = 32; expected_multiline = 1;
    const uint32_t widths_before_draw = width_queries;
    assert(!p4_game_tables_text_draw_geometry(row->stream, 12.5f, -4.0f, 2.75f, 0x12345678,
        1, 0x105, 1, 1, 64, 32));
    assert(captured[0] == 200 && (int32_t)captured[1] == -32 && captured[3] == 1 && captured[4] == 5);
    assert(drawn_slot == 1 && drawn_scale == 2.75f && drawn_aux == 5 && drawn_color == 0x12345678);
    assert(draw_count == 6 && list_releases == 10 && width_queries == widths_before_draw);
    assert(*font == 17 && *(uint32_t *)P4_G2H(0x007645a4) == 37 && *(uint32_t *)P4_G2H(0x007645a8) == 23);
    assert(*(uint32_t *)P4_G2H(0x00764594) == 3 && *(uint32_t *)P4_G2H(0x007637f8) == 88);
    assert(*(uint16_t *)P4_G2H(0x00763810) == 0x14);
    const uint32_t builds_before_collapse = build_count;
    assert(!p4_game_tables_text_measure_geometry(row->stream, 1, 0, 0, 128));
    assert(!p4_game_tables_text_measure_geometry(row->stream, 1, 0, 128, 0));
    assert(!p4_game_tables_text_draw_geometry(row->stream, 12.5f, -4.0f, 2.75f, 0x12345678,
        1, 0x105, 1, 1, 0, 64));
    assert(!p4_game_tables_text_draw_geometry(row->stream, 12.5f, -4.0f, 2.75f, 0x12345678,
        1, 0x105, 1, 1, 64, 0));
    assert(p4_game_tables_text_measure_geometry(row->stream, 1, 0, 129, 128) == -1);
    assert(p4_game_tables_text_draw_geometry(row->stream, 12.5f, -4.0f, 2.75f, 0x12345678,
        1, 0x105, 1, 1, 64, UINT32_MAX) == -1);
    assert(build_count == builds_before_collapse && draw_count == 6 && list_releases == 10 && width_queries == widths_before_draw);
    assert(*font == 17 && *(uint32_t *)P4_G2H(0x007645a4) == 37 && *(uint32_t *)P4_G2H(0x007645a8) == 23);
    assert(*(uint32_t *)P4_G2H(0x00764594) == 3 && *(uint32_t *)P4_G2H(0x007637f8) == 88);
    assert(*(uint16_t *)P4_G2H(0x00763810) == 0x14);
    p4_game_tables_text_destroy(t);
    assert(allocations == 2 && releases == 1);
    text_id_width(&allocator);
    assert(allocations == 3 && releases == 2);
    semantic_phase(&allocator);
    assert(allocations == 5 && releases == 3);
    puts("game_text: owned encoded arena, conflicts, grammar, rollback and native draw/width recipe passed");
    return 0;
}
