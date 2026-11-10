/* The p4pc:golden derived-table producer (docs/modding/golden.md): locates and
 * reads the player's own Golden install, translates the selected groups to
 * PS2-layout rows and hands them to the table service. Registered by core boot
 * before package admission; it runs only when a package selects it. */
#include "p4pc/golden_producer.h"
#include "p4pc/game_table_derived.h"
#include "p4pc/golden_tables.h"
#include "p4pc/mod_setting_store.h"
#include "../config/p4cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The package's [setting] keys, one per phase-1 group. */
static const struct {
    const char *key;
    unsigned group;
} groups[] = {
    {"numbers", P4_GOLDEN_GROUP_NUMBERS},
    {"enemies", P4_GOLDEN_GROUP_ENEMIES},
    {"party", P4_GOLDEN_GROUP_PARTY},
    {"exp_curves", P4_GOLDEN_GROUP_EXP},
    {"encounters", P4_GOLDEN_GROUP_ENCOUNTERS},
};

static int base(void *user, P4GameTableInputKind input, uint32_t span, const uint8_t **bytes, size_t *size)
{
    return p4_table_derived_base((const P4TableDerivedRequest *)user, input, span, bytes, size);
}

/* Spans the table service replaces only whole (PERSONA's party block and EXP
 * curves, the ENCOUNT blocks): the base span with the rows laid over it,
 * extended to the last row when rows follow the base ones (ENCOUNT
 * formations; the sink refuses other length changes). */
static int push_span(const P4TableDerivedRequest *request, P4TableDerivedSink *sink, const P4GoldenRows *rows,
                     char *error, size_t capacity)
{
    const uint8_t *bytes;
    size_t size, total;
    uint8_t *composed;
    P4TableDerivedSpan span;
    int result;

    if (p4_table_derived_base(request, rows->input, rows->span, &bytes, &size)) {
        snprintf(error, capacity, "Golden: base span %u of input %d unavailable", (unsigned)rows->span, (int)rows->input);
        return -1;
    }
    total = rows->count ? ((size_t)rows->ids[rows->count - 1] + 1) * rows->row_size : 0;
    if (total < size) total = size;
    if (!(composed = calloc(total ? total : 1, 1))) {
        snprintf(error, capacity, "Golden: out of memory");
        return -1;
    }
    memcpy(composed, bytes, size);
    for (size_t i = 0; i < rows->count; ++i) {
        memcpy(composed + (size_t)rows->ids[i] * rows->row_size, rows->bytes + i * rows->row_size, rows->row_size);
    }
    span.input = rows->input;
    span.span = rows->span;
    span.size = total;
    span.bytes = composed;
    result = p4_table_derived_sink_span(sink, &span);
    free(composed);
    return result;
}

static int whole_span(const P4GoldenRows *rows)
{
    return (rows->input == P4_TABLE_INPUT_PERSONA && (rows->span == 2 || rows->span == 3)) ||
           rows->input == P4_TABLE_INPUT_ENCOUNT;
}

static int produce(void *user, const P4TableDerivedRequest *request, P4TableDerivedSink *sink, char *error, size_t capacity)
{
    unsigned selected = 0;
    const char *configured = getenv("P4PC_GOLDEN");
    char directory[1024], value[16];
    P4GoldenSource source;
    P4GoldenBuild build;
    int result;

    (void)user;
    for (size_t i = 0; i < sizeof groups / sizeof groups[0]; ++i) {
        if (p4_mod_setting_boot(request->provider, groups[i].key, value, sizeof value) == 1 && !strcmp(value, "on")) {
            selected |= groups[i].group;
        }
    }
    /* Golden moved party skills from the learnsets to Social Link ranks; without
     * the script's rank skills the party learnsets would lose them. */
    if ((selected & P4_GOLDEN_GROUP_PARTY) &&
        (p4_mod_setting_boot(request->provider, "rank_skills", value, sizeof value) != 1 || strcmp(value, "on"))) {
        selected &= ~(unsigned)P4_GOLDEN_GROUP_PARTY;
        fprintf(stderr, "p4pc: golden party Personas need PERSONA SKILLS; party group not applied\n");
    }
    /* Golden's floors send its Shadows at its numbers; with PS2's enemies the
     * encounter set would pair Golden placement with PS2 stats. */
    if ((selected & P4_GOLDEN_GROUP_ENCOUNTERS) && !(selected & P4_GOLDEN_GROUP_ENEMIES)) {
        selected &= ~(unsigned)P4_GOLDEN_GROUP_ENCOUNTERS;
        fprintf(stderr, "p4pc: golden random encounters need ENEMIES; encounters group not applied\n");
    }
    if (!configured || !configured[0]) configured = p4cfg_str("golden.install", NULL);
    if (p4_golden_locate(configured, directory, sizeof directory)) {
        snprintf(error, capacity, configured && configured[0]
                 ? "Golden: no Persona 4 Golden install (data_e.cpk) in the golden.install folder"
                 : "Golden: no Persona 4 Golden install found; set golden.install in p4pc.ini or P4PC_GOLDEN");
        return 1;
    }
    if ((result = p4_golden_load(&source, directory, error, capacity))) return result > 0 ? 1 : -1;
    for (int t = 0; t < P4_GOLDEN_TABLE_COUNT; ++t) {
        if (p4_table_derived_sink_fingerprint(sink, p4_golden_table_name(t), source.tables[t].digest)) {
            p4_golden_free(&source);
            return -1;
        }
    }
    if (p4_golden_build(&source, selected, base, (void *)request, &build, error, capacity)) {
        p4_golden_free(&source);
        return -1;
    }
    result = 0;
    for (size_t i = 0; i < build.count && !result; ++i) {
        const P4GoldenRows *rows = &build.sets[i];
        if (whole_span(rows)) {
            result = push_span(request, sink, rows, error, capacity);
        } else {
            P4TableDerivedRows out = {rows->input, rows->span, rows->row_size, rows->count, rows->ids, rows->bytes};
            result = p4_table_derived_sink_rows(sink, &out);
        }
    }
    if (!result) {
        size_t total = 0;
        for (size_t i = 0; i < build.count; ++i) total += build.sets[i].count;
        fprintf(stderr, "p4pc: golden derived from %s: groups 0x%x, %zu row sets, %zu rows\n",
                directory, selected, build.count, total);
    }
    p4_golden_build_free(&build);
    p4_golden_free(&source);
    return result ? -1 : 0;
}

static const P4TableDerivedProducer producer = {"p4pc:golden", 1, produce, NULL};

int p4_golden_producer_register(void)
{
    return p4_game_tables_derived_register(&producer);
}
