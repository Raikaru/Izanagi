/* Real row capture/getters over a synthetic native allocator and completion fence. */
#include "../src/platform/golden/golden_battle_rows.h"
#include "p4pc/golden_battle_rows_native.h"
#include "p4pc/game_mode.h"
#include "p4pc/game_table_extents.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef P4_GOLDEN_LIFETIME_FIXTURE
static void lifetime_vectors(P4GameMode *selector);
#endif
static P4GameMode mode;
P4GameMode p4_game_mode(void) { return mode; }
static P4GameTableCompletePrepareFn prepare;
static P4GameTableCompleteCommitFn commit;
static unsigned registrations, removals, allocations, releases;
static uint32_t metadata_guest, presentation_guest;
static uint8_t *effect_host;
typedef struct CommonOwnerFixture {
    uint32_t battle, handles[49];
    unsigned ready;
} CommonOwnerFixture;
static uint32_t common_interception(void *context, uint32_t battle)
{
    CommonOwnerFixture *owner = context;
    if (!owner->ready || owner->battle != battle) return 0;
    for (unsigned i = 0; i < 49; ++i) if (!owner->handles[i]) return 0;
    return owner->handles[48];
}
int p4_game_tables_battle_complete_register(const char *key, P4GameTableCompletePrepareFn p,
    P4GameTableCompleteCommitFn c, void *user)
{
    assert(!strcmp(key, "golden-rows") && p && c && !user && !prepare && !commit);
    prepare = p; commit = c; ++registrations; return P4_TABLE_OK;
}
int p4_game_tables_battle_complete_unregister(const char *key, P4GameTableCompletePrepareFn p,
    P4GameTableCompleteCommitFn c, void *user)
{
    assert(!strcmp(key, "golden-rows") && p == prepare && c == commit && !user);
    prepare = NULL; commit = NULL; ++removals; return P4_TABLE_OK;
}
int p4_game_table_extent_rows(uint32_t slot, uint32_t stride, uint32_t *base, uint32_t *count)
{
    if (slot == 0x7644a8 && stride == 40) *base = metadata_guest;
    else if (slot == 0x7644ac && stride == 4) *base = presentation_guest;
    else return 0;
    *count = 440; return 1;
}
static void *allocate(void *user, uint32_t size, uint32_t *guest)
{
    assert(!user && size == 472 * 12);
    ++allocations;
    /* Explicit surrounding native backing: unchecked readers must see it. */
    effect_host = p4t_galloc(size + 32, 16); assert(effect_host);
    *guest = P4_H2G(effect_host);
    memset(effect_host + size, 0xef, 32);
    return effect_host;
}
static void release(void *user, uint32_t guest, void *host, uint32_t size)
{
    assert(!user && host == effect_host && guest == P4_H2G(host) && size == 472 * 12);
    ++releases;
}
static void word(uint8_t *p, uint32_t v)
{ for (unsigned i = 0; i < 4; ++i) p[i] = (uint8_t)(v >> (i * 8)); }
static void clock_and_cue_vectors(void)
{
    uint64_t now = 1000000000ull;
    p4_golden_frame_step_reset();
    assert(p4_golden_live_frame_step() == 1.0f);
    p4_golden_frame_step_publish(now);
    assert(p4_golden_live_frame_step() == 1.0f);
    p4_golden_frame_step_publish(now);
    assert(p4_golden_live_frame_step() == 0.01f);
    now += 1000000000ull;
    p4_golden_frame_step_publish(now);
    assert(p4_golden_live_frame_step() == 3.0f);
    now += 16666667ull;
    p4_golden_frame_step_publish(now);
    assert(p4_golden_live_frame_step() == 1.0f);
    p4_golden_cue_cache_reset();
    for (unsigned channel = 0; channel < 16; ++channel) {
        assert(p4_golden_cue_cache_match(channel, -1, -1));
        p4_golden_cue_cache_store(channel, 2, 0xB);
    }
    for (unsigned frame = 0; frame < 5; ++frame) {
        p4_golden_cue_cache_advance();
        for (unsigned channel = 0; channel < 16; ++channel)
            assert(p4_golden_cue_cache_match(channel, 2, 0xB));
    }
    /* A hit is a read, not an age refresh: the sixth half-tick expires it. */
    p4_golden_cue_cache_advance();
    for (unsigned channel = 0; channel < 16; ++channel)
        assert(p4_golden_cue_cache_match(channel, -1, -1));
    now += 33333334ull;
    p4_golden_frame_step_publish(now);
    assert(p4_golden_live_frame_step() == 2.0f);
    p4_golden_cue_cache_reset();
    assert(p4_golden_live_frame_step() == 2.0f);
    p4_golden_cue_cache_store(4, 2, 0xB);
    for (unsigned frame = 0; frame < 2; ++frame) {
        p4_golden_cue_cache_advance();
        assert(p4_golden_cue_cache_match(4, 2, 0xB));
    }
    p4_golden_cue_cache_advance();
    assert(p4_golden_cue_cache_match(4, -1, -1));
    p4_golden_cue_cache_store(4, 2, 0xB);
    p4_golden_frame_step_reset();
    assert(p4_golden_cue_cache_match(4, 2, 0xB));
    p4_golden_cue_cache_reset();
    puts("R2 live clock/cache: scheduler ns -> f32 60Hz step; clamp=.01..3; all16 rows expire at step1/6 and step2/3; resets independent");
}
int main(void)
{
    clock_and_cue_vectors();
    if (p4t_guest_init()) return 77;
    CommonOwnerFixture common = {.battle = 0xb00u, .ready = 1};
    for (unsigned i = 0; i < 49; ++i) common.handles[i] = 0x10000u + 16u * i;
    P4GoldenCommonResourceOwner owner = {common_interception, &common};
    assert(p4_golden_common_resource_owner_install(&owner)); /* No PS2 registration. */
    static uint8_t skill[472 * 44], effect[472 * 12], expected_skill[sizeof skill], expected_effect[sizeof effect];
    static const uint32_t flags[] = {0, 0x100, 0x200, 0x400, 0x800, 0x1000, 0x1e00, 0xfffff1a5};
    for (size_t i = 0; i < sizeof skill; ++i) skill[i] = (uint8_t)(i * 37 + 11);
    for (size_t i = 0; i < sizeof effect; ++i) effect[i] = (uint8_t)(i * 43 + 7);
    for (unsigned i = 0; i < sizeof flags / sizeof flags[0]; ++i) word(skill + (10 + i) * 44, flags[i]);
    word(skill + 471 * 44, 0x1000); skill[471 * 44 + 7] = 0xc3;
    effect[10 * 12] = 0; effect[11 * 12] = 255;
    effect[10 * 12 + 8] = 0xf4; effect[10 * 12 + 9] = 0xff;
    word(effect + 10 * 12 + 4, 0xf00fc241);
    effect[12 * 12 + 8] = 0; effect[12 * 12 + 9] = 0x80;
    effect[13 * 12 + 8] = 0xff; effect[13 * 12 + 9] = 0x7f;
    /* Synthetic interception timing rows; no game payload. */
    for (unsigned i = 20; i < 25; ++i) word(effect + i * 12 + 4, i == 22 ? 4 : i >= 23 ? 0x800 : 0x804);
    effect[20 * 12 + 8] = effect[22 * 12 + 8] = 0xf4;
    effect[20 * 12 + 9] = effect[22 * 12 + 9] = 0xff;
    effect[21 * 12 + 8] = 12; effect[21 * 12 + 9] = 0;
    effect[23 * 12 + 8] = 0; effect[23 * 12 + 9] = 0x80;
    effect[24 * 12 + 8] = 0xff; effect[24 * 12 + 9] = 0x7f;
    word(effect + 440 * 12 + 4, 0x804);
    effect[440 * 12 + 8] = 0xf4; effect[440 * 12 + 9] = 0xff;
    memcpy(expected_skill, skill, sizeof skill); memcpy(expected_effect, effect, sizeof effect);
    uint8_t *metadata = p4t_galloc(440 * 40, 16), *presentation = p4t_galloc(440 * 4, 16);
    assert(metadata && presentation);
    metadata_guest = P4_H2G(metadata); presentation_guest = P4_H2G(presentation);
    for (unsigned i = 0; i < 440; ++i) {
        metadata[i * 40] = skill[i * 44];
        memcpy(metadata + i * 40 + 1, skill + i * 44 + 4, 3);
        memcpy(metadata + i * 40 + 4, skill + i * 44 + 8, 36);
        memcpy(presentation + i * 4, effect + i * 12, 2);
        memcpy(presentation + i * 4 + 2, effect + i * 12 + 4, 2);
    }
    P4GameTablesDefinitionBuilder builder = {0};
    builder.allocator = (P4GameTableAllocator){NULL, allocate, release};
    P4GoldenCacheTable extra[] = {{"rows/effect0", effect, sizeof effect}, {"rows/skill1", skill, sizeof skill}};
    char reason[160];
    assert(p4_golden_battle_rows_capture(NULL, &builder, extra, 2, reason, sizeof reason) && !registrations);
    mode = P4_GAME_MODE_GOLDEN;
    extra[1].name = NULL;
    assert(p4_golden_battle_rows_capture(NULL, &builder, extra, 2, reason, sizeof reason) && !registrations);
    extra[1].name = "rows/skill1"; --extra[1].size;
    assert(p4_golden_battle_rows_capture(NULL, &builder, extra, 2, reason, sizeof reason) && !registrations);
    ++extra[1].size;
    assert(!p4_golden_battle_rows_capture(NULL, &builder, extra, 2, reason, sizeof reason));
    p4_golden_battle_rows_reset(); assert(removals == 1 && !allocations);
    assert(!p4_golden_battle_rows_capture(NULL, &builder, extra, 2, reason, sizeof reason));
    assert(p4_golden_battle_rows_capture(NULL, &builder, extra, 2, reason, sizeof reason));
    memset(skill, 0, sizeof skill); memset(effect, 0, sizeof effect); /* Cache load buffers are released before native publication. */
    metadata[0] ^= 1;
    assert(prepare(NULL) == P4_TABLE_CONFLICT && !allocations);
    metadata[0] ^= 1; presentation[2] ^= 1;
    assert(prepare(NULL) == P4_TABLE_CONFLICT && !allocations);
    presentation[2] ^= 1;
    assert(prepare(NULL) == P4_TABLE_OK && allocations == 1);
    assert(prepare(NULL) == P4_TABLE_OK && allocations == 1); /* Other participant may need another prepare pass. */
    commit(NULL);
    assert(p4_golden_common_resource_owner_install(NULL));
    assert(!p4_golden_common_resource_owner_install(&owner));
    assert(p4_golden_common_resource_owner_install(&owner)); /* One shared M8f owner. */
    assert(p4_golden_common_interception_resource(common.battle) == common.handles[48]);
    P4GoldenCommonResourceOwner wrong = {common_interception, NULL};
    assert(p4_golden_common_resource_owner_remove(&wrong));
    assert(!p4_golden_common_resource_owner_remove(&owner));
    assert(!p4_golden_common_resource_owner_install(&owner));
    for (unsigned i = 0; i < 472; ++i) {
        unsigned port = i < 440 ? i : i + 136;
        assert(!memcmp(p4_golden_skill_raw(port), expected_skill + i * 44, 44));
        assert(!memcmp(p4_golden_effect_raw(port), expected_effect + i * 12, 12));
    }
    for (unsigned i = 0; i < sizeof flags / sizeof flags[0]; ++i) {
        assert(p4_golden_skill_flags(10 + i) == flags[i]);
        assert(p4_golden_skill_selector(10 + i) == ((flags[i] >> 9) & 15));
    }
    assert(p4_golden_skill_selector(607) == 8 && p4_golden_skill_raw(607)[7] == 0xc3);
    assert(!p4_golden_skill_raw(440) && !p4_golden_effect_raw(440));
    assert(p4_golden_effect_flags(10) == 0xf00fc241 && p4_golden_effect_timing(10) == -12);
    assert(p4_golden_effect_interval(10) == 1 && p4_golden_effect_interval(11) == 255);
    assert(p4_golden_effect_flags(440) == 0xefefefef); /* Actual allocator backing, not clamped or synthesized. */
#ifdef P4_GOLDEN_LIFETIME_FIXTURE
    lifetime_vectors(&mode);
#endif
    p4_golden_battle_rows_reset(); assert(releases == 1 && removals == 1);
    assert(p4_golden_common_resource_owner_remove(&owner)); /* Session reset detached it. */
    puts("golden_battle_rows: full raw rows, widened flags/selector, native fence, cache lifetime, backing and teardown passed");
    return 0;
}
