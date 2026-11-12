/* Golden -> PS2 row translation for the p4pc.golden derived source
 * (include/p4pc/golden_tables.h). Layouts: decomp include/dat_skill_entry_internal.h,
 * dat_enemy_internal.h, g_data_entry_internal.h and src/Main/Battle/Data/datPersona.c;
 * the measured Golden differences are in docs/modding/golden.md. */
#include "p4pc/golden_tables.h"
#include "p4pc/game_encounters.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* PS2 (retail NTSC-U) layouts. */
#define SKILL_NAMES 576u
#define SKILL_ACTIVE 440u
#define SKILL_ROW 40u
#define SKILL_NAME_WIDTH 19u
#define ENEMIES 336u
#define ENEMY_ROW 60u
#define ENEMY_NAME_WIDTH 21u
#define AFFINITY_ROW 32u
#define PERSONAS 256u
#define PERSONA_META 14u
#define PERSONA_GROWTH 70u
#define PARTY_ROWS 24u
#define PARTY_ROW 0x26Eu
#define CURVES 9u
#define CURVE_ROW 392u
#define ITEM_HEADER 16u
#define ITEM_ROW 68u
#define ITEMS 1536u
/* ENCOUNT blocks (p4pc/game_encounters.h): retail row counts. */
#define FORMATIONS 912u
#define SELECTORS 160u
#define WEIGHTED 56u
#define RECORDS 75u
/* FldUnit_UpdateEnemySlot (0x001662D0) places a selector's treasure count in
 * eight field-object slots. */
#define TREASURE_SLOTS 8u
/* Golden layouts. */
#define G_SKILL_NAMES 624u
#define G_SKILL_ACTIVE 472u
#define G_SKILL_ROW 44u
#define G_SKILL_NAME_WIDTH 23u
#define G_ENEMIES 368u
#define G_ENEMY_NAME_WIDTH 21u
#define G_ITEMS 2560u
#define PASSIVE_SHIFT 0x20u
#define G_FORMATIONS 944u
#define G_SELECTORS 180u
#define G_WEIGHTED 68u
#define G_WEIGHTED_ROW 252u
#define G_WEIGHTED_HEADER 12u
#define G_RECORDS 84u

/* Party Personas 0xC0..0xCD (second tiers included); 0xCE.. are bosses and Golden's third tiers. */
#define PARTY_FIRST 0xC0u
#define PARTY_COMBAT_ROWS 14u
#define COMPENDIUM_PERSONAS 0xC0u

typedef struct Base {
    const uint8_t *bytes;
    size_t size;
} Base;

typedef struct Ctx {
    const P4GoldenSource *golden;
    P4GoldenBaseFn base;
    void *user;
    char *reason;
    size_t capacity;
    Base skill_elements, skill_rows, skill_names, enemy_names, enemy_rows, enemy_affinity;
    Base persona[6], persona_affinity, item, encount[5];
} Ctx;

static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }
static uint32_t rd32(const uint8_t *p) { return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void wr16(uint8_t *p, uint16_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }

static int fail(Ctx *ctx, const char *what)
{
    snprintf(ctx->reason, ctx->capacity, "%s", what);
    return -1;
}

static int need(Ctx *ctx, Base *out, P4GameTableInputKind input, uint32_t span, size_t size, const char *what)
{
    if (out->bytes) return 0;
    if (ctx->base(ctx->user, input, span, &out->bytes, &out->size) || !out->bytes || out->size != size) {
        snprintf(ctx->reason, ctx->capacity, "base %s is not the retail layout", what);
        out->bytes = NULL;
        return -1;
    }
    return 0;
}

static const P4GoldenSpan *golden_span(const Ctx *ctx, int table, int span, size_t size)
{
    const P4GoldenTable *t = &ctx->golden->tables[table];
    if (span >= t->span_count || t->spans[span].size != size) return NULL;
    return &t->spans[span];
}

/* "Blank", empty, or a hex placeholder ("116"): no skill or enemy of PS2's own. */
static int placeholder(const uint8_t *name, size_t width)
{
    size_t length = strnlen((const char *)name, width);
    if (!length || (length == 5 && !strncmp((const char *)name, "Blank", 5)) ||
        (length == 5 && !strncmp((const char *)name, "blank", 5))) {
        return 1;
    }
    if (length < 2 || length > 3) return 0;
    for (size_t i = 0; i < length; ++i) {
        if (!isxdigit(name[i]) || islower(name[i])) return 0;
    }
    return 1;
}

static const uint8_t *base_skill_name(const Ctx *ctx, uint32_t id) { return ctx->skill_names.bytes + id * SKILL_NAME_WIDTH; }

/* Golden 0x2C-byte skill row to PS2's 0x28: Golden widens the flag byte at 0
 * to a word (bytes 1..3) and pads after bytes 1..3 (byte 7). 0 when the row
 * fits PS2's layout exactly, 1 when Golden-only high flag bits were dropped
 * (only Golden-only skills use them), -1 out of range. */
static int skill_row(const Ctx *ctx, uint32_t id, uint8_t out[SKILL_ROW])
{
    const P4GoldenSpan *span = golden_span(ctx, P4_GOLDEN_SKILL, 1, G_SKILL_ACTIVE * G_SKILL_ROW);
    const uint8_t *g;
    if (!span || id >= G_SKILL_ACTIVE) return -1;
    g = span->bytes + id * G_SKILL_ROW;
    out[0] = g[0];
    memcpy(out + 1, g + 4, 3);
    memcpy(out + 4, g + 8, SKILL_ROW - 4);
    return g[1] || g[2] || g[3] || g[7];
}

/* A Golden skill id as the PS2 id of the same named skill, or -1 when PS2
 * has no such skill. A PS2 slot without a name ("Blank" or a hex placeholder)
 * is never a target: Golden fills several of them (Vigor Song in 0x11F keeps
 * PS2's unused row bytes), and item-effect rows are matched by their item. */
static int map_skill(const Ctx *ctx, uint32_t golden)
{
    uint32_t ps2;
    if (!golden) return 0;
    if (golden < SKILL_ACTIVE) ps2 = golden;
    else if (golden < G_SKILL_ACTIVE) return -1;
    else ps2 = golden - PASSIVE_SHIFT;
    if (ps2 >= SKILL_NAMES || placeholder(base_skill_name(ctx, ps2), SKILL_NAME_WIDTH)) return -1;
    return (int)ps2;
}

typedef struct Rows {
    P4GoldenRows *set;
    size_t capacity;
} Rows;

static int rows_begin(Ctx *ctx, P4GoldenBuild *build, Rows *rows, P4GameTableInputKind input, uint32_t span, uint32_t row_size)
{
    if (build->count == P4_GOLDEN_MAX_ROW_SETS) return fail(ctx, "too many row sets");
    rows->set = &build->sets[build->count++];
    memset(rows->set, 0, sizeof *rows->set);
    rows->set->input = input;
    rows->set->span = span;
    rows->set->row_size = row_size;
    rows->capacity = 0;
    return 0;
}

static int rows_push(Ctx *ctx, Rows *rows, uint32_t id, const uint8_t *bytes)
{
    P4GoldenRows *set = rows->set;
    if (set->count == rows->capacity) {
        size_t capacity = rows->capacity ? rows->capacity * 2 : 32;
        uint32_t *ids = realloc(set->ids, capacity * sizeof *ids);
        uint8_t *data;
        if (!ids) return fail(ctx, "out of memory");
        set->ids = ids;
        if (!(data = realloc(set->bytes, capacity * set->row_size))) return fail(ctx, "out of memory");
        set->bytes = data;
        rows->capacity = capacity;
    }
    set->ids[set->count] = id;
    memcpy(set->bytes + set->count * set->row_size, bytes, set->row_size);
    ++set->count;
    return 0;
}

/* Drops an empty trailing set so a group with no differences produces nothing. */
static void rows_end(P4GoldenBuild *build, Rows *rows)
{
    if (!rows->set->count) {
        --build->count;
        memset(rows->set, 0, sizeof *rows->set);
    }
}

static int prepare_skills(Ctx *ctx)
{
    if (need(ctx, &ctx->skill_rows, P4_TABLE_INPUT_SKILL, 1, SKILL_ACTIVE * SKILL_ROW, "SKILL.TBL rows") ||
        need(ctx, &ctx->skill_elements, P4_TABLE_INPUT_SKILL, 0, SKILL_NAMES * 2, "SKILL.TBL elements") ||
        need(ctx, &ctx->skill_names, P4_TABLE_INPUT_MSG, 1, SKILL_NAMES * SKILL_NAME_WIDTH, "MSG.TBL skill names")) {
        return -1;
    }
    if (!golden_span(ctx, P4_GOLDEN_SKILL, 1, G_SKILL_ACTIVE * G_SKILL_ROW)) {
        return fail(ctx, "Golden SKILL.TBL rows have an unexpected size");
    }
    return 0;
}

/* Skill rows and elements of skills PS2 has; Golden-only skills stay out. */
static int build_skills(Ctx *ctx, P4GoldenBuild *build)
{
    const P4GoldenSpan *elements = golden_span(ctx, P4_GOLDEN_SKILL, 0, G_SKILL_NAMES * 2);
    Rows rows;
    if (!elements) return fail(ctx, "Golden SKILL.TBL elements have an unexpected size");
    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_SKILL, 1, SKILL_ROW)) return -1;
    for (uint32_t id = 0; id < SKILL_ACTIVE; ++id) {
        const uint8_t *base = ctx->skill_rows.bytes + id * SKILL_ROW;
        uint8_t row[SKILL_ROW];
        if (placeholder(base_skill_name(ctx, id), SKILL_NAME_WIDTH)) continue;
        if (skill_row(ctx, id, row)) return fail(ctx, "Golden uses widened skill flags on a skill PS2 has");
        /* Byte 1 changes only on Healing Wave, with unmeasured meaning: keep PS2's. */
        row[1] = base[1];
        if (memcmp(row, base, SKILL_ROW) && rows_push(ctx, &rows, id, row)) return -1;
    }
    rows_end(build, &rows);
    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_SKILL, 0, 2)) return -1;
    for (uint32_t id = 0; id < SKILL_NAMES; ++id) {
        uint32_t golden = id < SKILL_ACTIVE ? id : id + PASSIVE_SHIFT;
        const uint8_t *g = elements->bytes + golden * 2;
        if (placeholder(base_skill_name(ctx, id), SKILL_NAME_WIDTH)) continue;
        if (memcmp(g, ctx->skill_elements.bytes + id * 2, 2) && rows_push(ctx, &rows, id, g)) return -1;
    }
    rows_end(build, &rows);
    return 0;
}

/* Learn events {u8 level, s8 kind, u16 skill}: remap skills; drop Golden-only
 * learns and a forget at the same level (Golden's upgrade pairs), compacting
 * the list in order. 0, or -1 for an unmappable forget. */
static int remap_events(const Ctx *ctx, uint8_t *events, unsigned count)
{
    uint8_t out[32 * 4];
    unsigned kept = 0;
    uint8_t dropped_levels[32];
    unsigned dropped = 0;

    for (unsigned k = 0; k < count; ++k) {
        const uint8_t *e = events + k * 4;
        int mapped;
        if (!e[1] && !rd16(e + 2)) continue;
        mapped = map_skill(ctx, rd16(e + 2));
        if (mapped < 0 && (int8_t)e[1] == 1) dropped_levels[dropped++] = e[0];
    }
    for (unsigned k = 0; k < count; ++k) {
        const uint8_t *e = events + k * 4;
        int mapped, skip = 0;
        if (!e[1] && !rd16(e + 2)) continue;
        mapped = map_skill(ctx, rd16(e + 2));
        for (unsigned d = 0; d < dropped; ++d) skip |= dropped_levels[d] == e[0] && (int8_t)e[1] != 1;
        if (mapped < 0 || skip) {
            if ((int8_t)e[1] != 1 && !skip) return -1;
            continue;
        }
        memcpy(out + kept * 4, e, 4);
        wr16(out + kept * 4 + 2, (uint16_t)mapped);
        ++kept;
    }
    memset(events, 0, count * 4);
    memcpy(events, out, kept * 4);
    return 0;
}

static int build_compendium(Ctx *ctx, P4GoldenBuild *build)
{
    const P4GoldenSpan *growth = golden_span(ctx, P4_GOLDEN_PERSONA, 1, PERSONAS * PERSONA_GROWTH);
    Rows rows;
    if (!growth) return fail(ctx, "Golden PERSONA.TBL growth has an unexpected size");
    if (need(ctx, &ctx->persona[1], P4_TABLE_INPUT_PERSONA, 1, PERSONAS * PERSONA_GROWTH, "PERSONA.TBL growth") ||
        rows_begin(ctx, build, &rows, P4_TABLE_INPUT_PERSONA, 1, PERSONA_GROWTH)) {
        return -1;
    }
    for (uint32_t id = 1; id < COMPENDIUM_PERSONAS; ++id) {
        uint8_t row[PERSONA_GROWTH];
        memcpy(row, growth->bytes + id * PERSONA_GROWTH, PERSONA_GROWTH);
        if (remap_events(ctx, row + 6, 16)) return fail(ctx, "Golden compendium learnset forgets a Golden-only skill");
        if (memcmp(row, ctx->persona[1].bytes + id * PERSONA_GROWTH, PERSONA_GROWTH) && rows_push(ctx, &rows, id, row)) return -1;
    }
    rows_end(build, &rows);
    return 0;
}

/* Item rows of equipment and consumables PS2 already has (price above the
 * dummy value 1). PS2 keeps its flags and its availability window; a row
 * granting or using a Golden-only skill stays PS2's. */
static int build_items(Ctx *ctx, P4GoldenBuild *build)
{
    const P4GoldenTable *golden = &ctx->golden->tables[P4_GOLDEN_ITEM];
    Rows rows;
    if (golden->size < ITEM_HEADER + G_ITEMS * ITEM_ROW || rd32(golden->bytes) != G_ITEMS) {
        return fail(ctx, "Golden itemtbl.bin has an unexpected layout");
    }
    if (need(ctx, &ctx->item, P4_TABLE_INPUT_ITEM, 0, ITEM_HEADER + ITEMS * ITEM_ROW, "ITEMTBL.BIN") ||
        rows_begin(ctx, build, &rows, P4_TABLE_INPUT_ITEM, 0, ITEM_ROW)) {
        return -1;
    }
    for (uint32_t id = 0; id < 0x400; ++id) {
        const uint8_t *base = ctx->item.bytes + ITEM_HEADER + id * ITEM_ROW;
        uint8_t row[ITEM_ROW];
        uint16_t effect;
        int skill;
        if (rd32(base + 0x34) <= 1) continue;
        memcpy(row, golden->bytes + ITEM_HEADER + id * ITEM_ROW, ITEM_ROW);
        /* An item keeps its own effect row (consumables use unnamed ones);
         * a changed effect or grant must name a skill PS2 has. */
        effect = rd16(row + 0x24);
        skill = effect == rd16(base + 0x24) ? effect : map_skill(ctx, effect);
        if (skill < 0) continue;
        wr16(row + 0x24, (uint16_t)skill);
        memcpy(row + 0x04, base + 0x04, 4);
        memcpy(row + 0x38, base + 0x38, ITEM_ROW - 0x38);
        if (memcmp(row, base, ITEM_ROW) && rows_push(ctx, &rows, id, row)) return -1;
    }
    rows_end(build, &rows);
    return 0;
}

/* Enemies PS2 has, with Golden numbers but PS2's skill lists (the PS2 AI
 * scripts name their skills). Drops of items PS2 lacks are removed. */
static int build_enemies(Ctx *ctx, P4GoldenBuild *build)
{
    const P4GoldenSpan *meta = golden_span(ctx, P4_GOLDEN_UNIT, 0, G_ENEMIES * ENEMY_ROW);
    const P4GoldenSpan *affinity = golden_span(ctx, P4_GOLDEN_UNIT, 1, G_ENEMIES * AFFINITY_ROW);
    Rows rows;
    if (!meta || !affinity) return fail(ctx, "Golden UNIT.TBL has an unexpected layout");
    if (need(ctx, &ctx->enemy_rows, P4_TABLE_INPUT_UNIT, 0, ENEMIES * ENEMY_ROW, "UNIT.TBL enemies") ||
        need(ctx, &ctx->enemy_affinity, P4_TABLE_INPUT_UNIT, 1, ENEMIES * AFFINITY_ROW, "UNIT.TBL enemy affinities") ||
        need(ctx, &ctx->enemy_names, P4_TABLE_INPUT_MSG, 2, ENEMIES * ENEMY_NAME_WIDTH, "MSG.TBL enemy names") ||
        need(ctx, &ctx->item, P4_TABLE_INPUT_ITEM, 0, ITEM_HEADER + ITEMS * ITEM_ROW, "ITEMTBL.BIN") ||
        rows_begin(ctx, build, &rows, P4_TABLE_INPUT_UNIT, 0, ENEMY_ROW)) {
        return -1;
    }
    for (uint32_t id = 0; id < ENEMIES; ++id) {
        const uint8_t *base = ctx->enemy_rows.bytes + id * ENEMY_ROW;
        uint8_t row[ENEMY_ROW];
        int special;
        if (placeholder(ctx->enemy_names.bytes + id * ENEMY_NAME_WIDTH, ENEMY_NAME_WIDTH)) continue;
        memcpy(row, meta->bytes + id * ENEMY_ROW, ENEMY_ROW);
        memcpy(row + 0x0E, base + 0x0E, 16);
        for (unsigned k = 0; k < 4; ++k) {
            uint8_t *drop = row + 0x22 + k * 4;
            uint16_t item = rd16(drop);
            if (item && (item >= ITEMS || rd32(ctx->item.bytes + ITEM_HEADER + item * ITEM_ROW + 0x34) == 1)) {
                memset(drop, 0, 4);
            }
        }
        special = map_skill(ctx, rd16(row + 0x34));
        if (special < 0) memcpy(row + 0x34, base + 0x34, 2);
        else wr16(row + 0x34, (uint16_t)special);
        if (memcmp(row, base, ENEMY_ROW) && rows_push(ctx, &rows, id, row)) return -1;
    }
    rows_end(build, &rows);
    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_UNIT, 1, AFFINITY_ROW)) return -1;
    for (uint32_t id = 0; id < ENEMIES; ++id) {
        const uint8_t *g = affinity->bytes + id * AFFINITY_ROW;
        if (placeholder(ctx->enemy_names.bytes + id * ENEMY_NAME_WIDTH, ENEMY_NAME_WIDTH)) continue;
        if (memcmp(g, ctx->enemy_affinity.bytes + id * AFFINITY_ROW, AFFINITY_ROW) && rows_push(ctx, &rows, id, g)) return -1;
    }
    rows_end(build, &rows);
    return 0;
}

static int build_party(Ctx *ctx, P4GoldenBuild *build)
{
    const P4GoldenSpan *meta = golden_span(ctx, P4_GOLDEN_PERSONA, 0, PERSONAS * PERSONA_META);
    const P4GoldenSpan *party = golden_span(ctx, P4_GOLDEN_PERSONA, 2, PARTY_ROWS * PARTY_ROW);
    const P4GoldenSpan *affinity = golden_span(ctx, P4_GOLDEN_UNIT, 2, PERSONAS * AFFINITY_ROW);
    Rows rows;
    if (!meta || !party || !affinity) return fail(ctx, "Golden PERSONA.TBL/UNIT.TBL have an unexpected layout");
    if (need(ctx, &ctx->persona[0], P4_TABLE_INPUT_PERSONA, 0, PERSONAS * PERSONA_META, "PERSONA.TBL metadata") ||
        need(ctx, &ctx->persona[2], P4_TABLE_INPUT_PERSONA, 2, PARTY_ROWS * PARTY_ROW, "PERSONA.TBL party block") ||
        need(ctx, &ctx->persona_affinity, P4_TABLE_INPUT_UNIT, 2, PERSONAS * AFFINITY_ROW, "UNIT.TBL persona affinities")) {
        return -1;
    }
    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_PERSONA, 0, PERSONA_META)) return -1;
    for (uint32_t id = PARTY_FIRST; id < PARTY_FIRST + PARTY_COMBAT_ROWS; ++id) {
        const uint8_t *g = meta->bytes + id * PERSONA_META;
        if (memcmp(g, ctx->persona[0].bytes + id * PERSONA_META, PERSONA_META) && rows_push(ctx, &rows, id, g)) return -1;
    }
    rows_end(build, &rows);
    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_PERSONA, 2, PARTY_ROW)) return -1;
    for (uint32_t row_id = 0; row_id < PARTY_COMBAT_ROWS; ++row_id) {
        uint8_t row[PARTY_ROW];
        memcpy(row, party->bytes + row_id * PARTY_ROW, PARTY_ROW);
        if (remap_events(ctx, row + 4, 32)) return fail(ctx, "Golden party learnset forgets a Golden-only skill");
        if (memcmp(row, ctx->persona[2].bytes + row_id * PARTY_ROW, PARTY_ROW) && rows_push(ctx, &rows, row_id, row)) return -1;
    }
    rows_end(build, &rows);
    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_UNIT, 2, AFFINITY_ROW)) return -1;
    for (uint32_t id = PARTY_FIRST; id < PARTY_FIRST + PARTY_COMBAT_ROWS; ++id) {
        const uint8_t *g = affinity->bytes + id * AFFINITY_ROW;
        if (memcmp(g, ctx->persona_affinity.bytes + id * AFFINITY_ROW, AFFINITY_ROW) && rows_push(ctx, &rows, id, g)) return -1;
    }
    rows_end(build, &rows);
    return 0;
}

static int build_curves(Ctx *ctx, P4GoldenBuild *build)
{
    const P4GoldenSpan *curves = golden_span(ctx, P4_GOLDEN_PERSONA, 3, CURVES * CURVE_ROW);
    Rows rows;
    if (!curves) return fail(ctx, "Golden PERSONA.TBL curves have an unexpected size");
    if (need(ctx, &ctx->persona[3], P4_TABLE_INPUT_PERSONA, 3, CURVES * CURVE_ROW, "PERSONA.TBL EXP curves") ||
        rows_begin(ctx, build, &rows, P4_TABLE_INPUT_PERSONA, 3, CURVE_ROW)) {
        return -1;
    }
    for (uint32_t id = 0; id < CURVES; ++id) {
        const uint8_t *g = curves->bytes + id * CURVE_ROW;
        if (memcmp(g, ctx->persona[3].bytes + id * CURVE_ROW, CURVE_ROW) && rows_push(ctx, &rows, id, g)) return -1;
    }
    rows_end(build, &rows);
    return 0;
}

/* ---- ENCOUNT: Golden's random encounters and dungeon treasure ---- */

#define NOT_PLACED 0xFFFFu

typedef struct Formations {
    const P4GoldenSpan *golden;
    uint8_t *rows;                  /* the base rows, then appended Golden rows */
    uint32_t count;
    uint16_t place[G_FORMATIONS];   /* Golden ID -> port ID; 0 dropped */
} Formations;

static int formation_empty(const uint8_t *row)
{
    for (unsigned k = 0; k < P4_ENCOUNT_FORMATION_ENEMY_COUNT; ++k) {
        if (rd16(row + P4_ENCOUNT_FORMATION_ENEMIES + k * 2)) return 0;
    }
    return 1;
}

/* The port ID of a Golden random formation. PS2 rows a script may name stay
 * PS2's: the Golden row takes its own ID only where the base row is the same
 * or names no enemy, and otherwise a new ID after the base rows. 0 (none) when
 * it names an enemy PS2 lacks (Golden-only Shadows, the Hands in PS2's blank
 * slots). */
static uint16_t place_formation(const Ctx *ctx, Formations *f, uint32_t id)
{
    const uint8_t *g;
    if (!id || id >= G_FORMATIONS) return 0;
    if (f->place[id] != NOT_PLACED) return f->place[id];
    g = f->golden->bytes + id * P4_ENCOUNT_FORMATION_SIZE;
    for (unsigned k = 0; k < P4_ENCOUNT_FORMATION_ENEMY_COUNT; ++k) {
        uint16_t enemy = rd16(g + P4_ENCOUNT_FORMATION_ENEMIES + k * 2);
        if (enemy && (enemy >= ENEMIES || placeholder(ctx->enemy_names.bytes + enemy * ENEMY_NAME_WIDTH, ENEMY_NAME_WIDTH))) {
            return f->place[id] = 0;
        }
    }
    if (id < FORMATIONS) {
        uint8_t *at = f->rows + id * P4_ENCOUNT_FORMATION_SIZE;
        if (!memcmp(at, g, P4_ENCOUNT_FORMATION_SIZE) || formation_empty(at)) {
            memcpy(at, g, P4_ENCOUNT_FORMATION_SIZE);
            return f->place[id] = (uint16_t)id;
        }
    }
    memcpy(f->rows + f->count * P4_ENCOUNT_FORMATION_SIZE, g, P4_ENCOUNT_FORMATION_SIZE);
    return f->place[id] = (uint16_t)f->count++;
}

/* Encount_PickEncounter (0x00161630) draws a category by the header rates,
 * then an entry of that category from the day's list of 30. */
static const struct {
    unsigned first, last;
} categories[3] = {{0, 20}, {20, 25}, {25, 30}};

/* Golden's 252-byte group to PS2's 248: Golden's header has a third column
 * per rate (normal, special, rare) that PS2's picker has no use for. An entry
 * whose formation was dropped is cleared; a special or rare category that lost
 * every entry that way gets rate 0, so the picker never draws it. The normal
 * rate stays: with all rates 0 the picker asserts, while an empty normal
 * category returns "none" as PS2's own empty group 55 does. */
static void weighted_row(const Ctx *ctx, Formations *f, const uint8_t *g, uint8_t out[P4_ENCOUNT_WEIGHTED_SIZE])
{
    const unsigned per_list = P4_ENCOUNT_WEIGHTED_ENTRIES / 2;
    memset(out, 0, P4_ENCOUNT_WEIGHTED_SIZE);
    for (unsigned c = 0; c < 3; ++c) {
        out[c * 2] = g[c * 3];
        out[c * 2 + 1] = g[c * 3 + 1];
    }
    for (unsigned list = 0; list < 2; ++list) {
        for (unsigned c = 0; c < 3; ++c) {
            unsigned kept = 0, dropped = 0;
            for (unsigned k = categories[c].first; k < categories[c].last; ++k) {
                const unsigned entry = list * per_list + k;
                const uint8_t *in = g + G_WEIGHTED_HEADER + entry * 4;
                uint8_t *at = out + P4_ENCOUNT_WEIGHTED_HEADER + entry * 4;
                uint16_t port;
                if (!rd16(in)) {
                    memcpy(at, in, 4);
                    continue;
                }
                if (!(port = place_formation(ctx, f, rd16(in)))) {
                    ++dropped;
                    continue;
                }
                wr16(at, port);
                memcpy(at + 2, in + 2, 2);
                kept += rd16(in + 2);
            }
            if (c && dropped && !kept) out[c * 2 + list] = 0;
        }
    }
}

/* A treasure item PS2 has: a retail ID that is not a dummy row, or one the
 * PS2 group already holds. */
static int record_item(const Ctx *ctx, uint16_t item, const uint8_t *base)
{
    if (item >= ITEMS) return 0;
    if (rd32(ctx->item.bytes + ITEM_HEADER + item * ITEM_ROW + 0x34) != 1) return 1;
    for (unsigned k = 0; k < P4_ENCOUNT_RECORD_COUNT; ++k) {
        if (rd16(base + k * P4_ENCOUNT_RECORD_STRIDE + P4_ENCOUNT_RECORD_ITEM) == item) return 1;
    }
    return 0;
}

/* The weight Encount_FindRecord (0x00161C80) draws from for one chest kind:
 * records with an item whose byte 7 bit 0 is set (mode 1) or clear (mode 0). */
static unsigned record_weight(const uint8_t *group, unsigned kind)
{
    unsigned total = 0;
    for (unsigned k = 0; k < P4_ENCOUNT_RECORD_COUNT; ++k) {
        const uint8_t *record = group + k * P4_ENCOUNT_RECORD_STRIDE;
        if (rd16(record + P4_ENCOUNT_RECORD_ITEM) && (record[7] & 1u) == kind) total += rd16(record);
    }
    return total;
}

/* Golden's whole random-encounter set for PS2's dungeon floors: selectors
 * (shadow and treasure counts, groups) by field step, the weighted groups and
 * the formations they name, and the treasure groups. Rows only Golden's own
 * floors use (selectors from 160, groups from 56, treasure from 75) stay out. */
static int build_encounters(Ctx *ctx, P4GoldenBuild *build)
{
    const P4GoldenSpan *selectors = golden_span(ctx, P4_GOLDEN_ENCOUNT, P4_ENCOUNT_SELECTOR_SPAN, G_SELECTORS * P4_ENCOUNT_SELECTOR_SIZE);
    const P4GoldenSpan *weighted = golden_span(ctx, P4_GOLDEN_ENCOUNT, P4_ENCOUNT_WEIGHTED_SPAN, G_WEIGHTED * G_WEIGHTED_ROW);
    const P4GoldenSpan *records = golden_span(ctx, P4_GOLDEN_ENCOUNT, P4_ENCOUNT_RECORD_SPAN, G_RECORDS * P4_ENCOUNT_RECORD_SIZE);
    Formations f;
    Rows rows;
    int result = -1;

    f.golden = golden_span(ctx, P4_GOLDEN_ENCOUNT, P4_ENCOUNT_FORMATION_SPAN, G_FORMATIONS * P4_ENCOUNT_FORMATION_SIZE);
    if (!f.golden || !selectors || !weighted || !records) return fail(ctx, "Golden ENCOUNT.TBL has an unexpected layout");
    if (need(ctx, &ctx->encount[P4_ENCOUNT_FORMATION_SPAN], P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_FORMATION_SPAN,
             FORMATIONS * P4_ENCOUNT_FORMATION_SIZE, "ENCOUNT.TBL formations") ||
        need(ctx, &ctx->encount[P4_ENCOUNT_SELECTOR_SPAN], P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_SELECTOR_SPAN,
             SELECTORS * P4_ENCOUNT_SELECTOR_SIZE, "ENCOUNT.TBL selectors") ||
        need(ctx, &ctx->encount[P4_ENCOUNT_WEIGHTED_SPAN], P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_WEIGHTED_SPAN,
             WEIGHTED * P4_ENCOUNT_WEIGHTED_SIZE, "ENCOUNT.TBL weighted groups") ||
        need(ctx, &ctx->encount[P4_ENCOUNT_RECORD_SPAN], P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_RECORD_SPAN,
             RECORDS * P4_ENCOUNT_RECORD_SIZE, "ENCOUNT.TBL record groups") ||
        need(ctx, &ctx->enemy_names, P4_TABLE_INPUT_MSG, 2, ENEMIES * ENEMY_NAME_WIDTH, "MSG.TBL enemy names") ||
        need(ctx, &ctx->item, P4_TABLE_INPUT_ITEM, 0, ITEM_HEADER + ITEMS * ITEM_ROW, "ITEMTBL.BIN")) {
        return -1;
    }
    /* Selector 0 is the game's "none" (Encount_FindRecord 0x00161C80 returns no
     * treasure for it), so it stays PS2's. */
    for (uint32_t id = 1; id < SELECTORS; ++id) {
        const uint8_t *g = selectors->bytes + id * P4_ENCOUNT_SELECTOR_SIZE;
        if (rd16(g + P4_ENCOUNT_SELECTOR_WEIGHTED) >= WEIGHTED || rd16(g + P4_ENCOUNT_SELECTOR_RECORDS) >= RECORDS ||
            g[4] > TREASURE_SLOTS) {
            return fail(ctx, "Golden ENCOUNT.TBL selectors name groups or treasure counts outside the PS2 layout");
        }
    }
    if (!(f.rows = malloc((size_t)(FORMATIONS + G_FORMATIONS) * P4_ENCOUNT_FORMATION_SIZE))) return fail(ctx, "out of memory");
    memcpy(f.rows, ctx->encount[P4_ENCOUNT_FORMATION_SPAN].bytes, FORMATIONS * P4_ENCOUNT_FORMATION_SIZE);
    f.count = FORMATIONS;
    memset(f.place, 0xFF, sizeof f.place);

    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_WEIGHTED_SPAN, P4_ENCOUNT_WEIGHTED_SIZE)) goto done;
    for (uint32_t id = 0; id < WEIGHTED; ++id) {
        uint8_t row[P4_ENCOUNT_WEIGHTED_SIZE];
        weighted_row(ctx, &f, weighted->bytes + id * G_WEIGHTED_ROW, row);
        if (memcmp(row, ctx->encount[P4_ENCOUNT_WEIGHTED_SPAN].bytes + id * P4_ENCOUNT_WEIGHTED_SIZE, P4_ENCOUNT_WEIGHTED_SIZE) &&
            rows_push(ctx, &rows, id, row)) {
            goto done;
        }
    }
    rows_end(build, &rows);

    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_FORMATION_SPAN, P4_ENCOUNT_FORMATION_SIZE)) goto done;
    for (uint32_t id = 0; id < f.count; ++id) {
        const uint8_t *row = f.rows + id * P4_ENCOUNT_FORMATION_SIZE;
        if ((id >= FORMATIONS ||
             memcmp(row, ctx->encount[P4_ENCOUNT_FORMATION_SPAN].bytes + id * P4_ENCOUNT_FORMATION_SIZE, P4_ENCOUNT_FORMATION_SIZE)) &&
            rows_push(ctx, &rows, id, row)) {
            goto done;
        }
    }
    rows_end(build, &rows);

    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_SELECTOR_SPAN, P4_ENCOUNT_SELECTOR_SIZE)) goto done;
    for (uint32_t id = 1; id < SELECTORS; ++id) {
        const uint8_t *g = selectors->bytes + id * P4_ENCOUNT_SELECTOR_SIZE;
        if (memcmp(g, ctx->encount[P4_ENCOUNT_SELECTOR_SPAN].bytes + id * P4_ENCOUNT_SELECTOR_SIZE, P4_ENCOUNT_SELECTOR_SIZE) &&
            rows_push(ctx, &rows, id, g)) {
            goto done;
        }
    }
    rows_end(build, &rows);

    /* A treasure group stays PS2's whole when Golden's names an item PS2 lacks,
     * or yields other chest kinds than PS2's: a dungeon board in a save keeps
     * each chest's kind and re-picks its record from the current group
     * (fldStepRestoreBoard 0x0015AC60), and an empty kind asserts, in either
     * direction (a board saved here and restored without the package). */
    if (rows_begin(ctx, build, &rows, P4_TABLE_INPUT_ENCOUNT, P4_ENCOUNT_RECORD_SPAN, P4_ENCOUNT_RECORD_SIZE)) goto done;
    for (uint32_t id = 0; id < RECORDS; ++id) {
        const uint8_t *g = records->bytes + id * P4_ENCOUNT_RECORD_SIZE;
        const uint8_t *b = ctx->encount[P4_ENCOUNT_RECORD_SPAN].bytes + id * P4_ENCOUNT_RECORD_SIZE;
        int usable = 1;
        for (unsigned k = 0; k < P4_ENCOUNT_RECORD_COUNT && usable; ++k) {
            uint16_t item = rd16(g + k * P4_ENCOUNT_RECORD_STRIDE + P4_ENCOUNT_RECORD_ITEM);
            usable = !item || record_item(ctx, item, b);
        }
        for (unsigned kind = 0; kind < 2 && usable; ++kind) {
            usable = !record_weight(b, kind) == !record_weight(g, kind);
        }
        if (usable && memcmp(g, b, P4_ENCOUNT_RECORD_SIZE) && rows_push(ctx, &rows, id, g)) goto done;
    }
    rows_end(build, &rows);
    result = 0;
done:
    free(f.rows);
    return result;
}

void p4_golden_build_free(P4GoldenBuild *build)
{
    for (size_t i = 0; i < P4_GOLDEN_MAX_ROW_SETS; ++i) {
        free(build->sets[i].ids);
        free(build->sets[i].bytes);
    }
    memset(build, 0, sizeof *build);
}

int p4_golden_build(const P4GoldenSource *source, unsigned groups, P4GoldenBaseFn base, void *user,
                    P4GoldenBuild *out, char *reason, size_t capacity)
{
    Ctx ctx;
    int result = 0;

    memset(out, 0, sizeof *out);
    memset(&ctx, 0, sizeof ctx);
    ctx.golden = source;
    ctx.base = base;
    ctx.user = user;
    ctx.reason = reason;
    ctx.capacity = capacity;
    if (!groups) return 0;
    /* Every group maps Golden skill ids, so the skill tables come first. */
    if (prepare_skills(&ctx)) result = -1;
    if (!result && (groups & P4_GOLDEN_GROUP_NUMBERS)) {
        result = build_skills(&ctx, out) || build_compendium(&ctx, out) || build_items(&ctx, out) ? -1 : 0;
    }
    if (!result && (groups & P4_GOLDEN_GROUP_ENEMIES)) result = build_enemies(&ctx, out);
    if (!result && (groups & P4_GOLDEN_GROUP_PARTY)) result = build_party(&ctx, out);
    if (!result && (groups & P4_GOLDEN_GROUP_EXP)) result = build_curves(&ctx, out);
    if (!result && (groups & P4_GOLDEN_GROUP_ENCOUNTERS)) result = build_encounters(&ctx, out);
    if (result) p4_golden_build_free(out);
    return result;
}
