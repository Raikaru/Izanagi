#include "p4pc/game_elements_ai.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/game_table_inputs_use.h"
#include "p4pc/package_boot.h"
#include "p4pc/gmem.h"
#include <stdio.h>
#include <string.h>

/* The source provider owns every borrowed object. CPU and runtime products are
 * deliberately obtained in their own Core passes, never through an active-view
 * lookup or by reopening AICALC after package publication. Knowledge has its
 * own handler (game-tables.enemy-knowledge); this one owns only the AI product. */
static struct EnemyAiBoot {
    const P4GameTablesBootProduct *cpu;
    const P4GameTableInput *input;
    P4GameAiPrepared *prepared;
    int exposed;
} boot;

static int prepare(void *unused)
{
    const void *product = NULL;
    (void)unused;
    if (boot.cpu || boot.prepared || boot.exposed) return -1;
    if (p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &product)) return -1;
    const P4GameTablesBootProduct *root = product;
    if (!root || !p4_game_elements_requested(root->tables))
        return p4_package_boot_provide(NULL);
    if (!root->tables || !root->sources || !root->skills || !root->native_uses ||
        !p4_game_tables_revision(root->tables) ||
        !p4_game_table_sources_is_sealed(root->sources) ||
        p4_game_table_sources_validate_view(root->sources, root->tables)) return -1;
    boot.cpu = root;
    for (size_t i = 0; i < root->input_count; ++i) {
        const P4GameTableInput *input = root->inputs[i];
        if (p4_game_table_input_kind(input) != P4_TABLE_INPUT_AICALC) continue;
        if (boot.input) return -1;
        boot.input = input;
    }
    if (!boot.input || p4_game_table_input_span_count(boot.input) != 11u) return -1;
    const P4GameTableInputSpan *rows = p4_game_table_input_span(boot.input, 8);
    const P4GameTableInputSpan *player = p4_game_table_input_span(boot.input, 9);
    const P4GameTableInputSpan *enemy = p4_game_table_input_span(boot.input, 10);
    P4GameAiScriptSource scripts[2] = {
        { .bytes = p4_game_table_input_span_bytes(player),
          .size = p4_game_table_input_span_size(player), .source_identity = player },
        { .bytes = p4_game_table_input_span_bytes(enemy),
          .size = p4_game_table_input_span_size(enemy), .source_identity = enemy }
    };
    int result = p4_game_elements_ai_sources_prepare(root->tables, 1, scripts, 2, &boot.prepared);
    if (!result) {
        const P4GameAiBankSource bank = {
            .rows = p4_game_table_input_span_bytes(rows),
            .rows_size = p4_game_table_input_span_size(rows), .rows_identity = rows,
            .player_script_identity = player, .enemy_script_identity = enemy,
            .skills = root->skills, .use = p4_game_table_input_semantic_use,
            .use_context = (void *)boot.input
        };
        result = p4_game_elements_ai_bank_stage(boot.prepared, &bank);
    }
    if (result) {
        fprintf(stderr, "p4pc: retained AICALC CPU preparation refused: %s\n", p4_game_tables_error(result));
        const P4PackageProvider *provider = p4_game_table_input_provider(boot.input);
        if (provider && result != P4_TABLE_NOMEM)
            p4_package_provider_refuse(provider, p4_game_tables_error(result));
        return -1;
    }
    return p4_package_boot_provide(boot.prepared);
}
static void publish(void *unused)
{
    (void)unused;
    boot.exposed = 1;
}
static void rollback(void *unused)
{
    (void)unused;
    if (boot.exposed) p4_package_boot_runtime_failed("Enemy AI rollback attempted after native raw exposure.");
    if (p4_game_elements_ai_sources_destroy(boot.prepared))
        p4_package_boot_runtime_failed("Early enemy AI rollback could not release its unused CPU product.");
    memset(&boot, 0, sizeof boot);
}
static int same_sources(const P4GameTablesBootProduct *a, const P4GameTablesBootProduct *b)
{
    return a && b && a->tables == b->tables && a->sources == b->sources &&
        a->skills == b->skills && a->items == b->items && a->native_uses == b->native_uses &&
        a->names == b->names && a->descriptions == b->descriptions &&
        a->inputs == b->inputs && a->input_count == b->input_count;
}
static int runtime_prepare(void *unused)
{
    const void *product = NULL;
    (void)unused;
    if (p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &product)) return -1;
    const P4GameTablesBootProduct *root = product;
    if (!boot.cpu) {
        if (root && p4_game_elements_requested(root->tables))
            return p4_package_boot_fail(NULL, "Expanded enemy AI has no retained CPU preparation.");
        return p4_package_boot_provide(NULL);
    }
    if (!same_sources(root, boot.cpu))
        return p4_package_boot_fail(NULL, "Enemy AI runtime source product differs from its sealed CPU product.");
    /* These are the actual roots consumed by001ded30, not addresses fabricated
     * from CPU span offsets. source_bind/bank_bind prove guest mappings, exact
     * loaded sizes and bytes without parsing, allocation or semantic-use touch.
     * Source/element attestation and the full cohort still gate native commit. */
    const P4GameTableInputSpan *rows = p4_game_table_input_span(boot.input, 8);
    const P4GameTableInputSpan *scripts[2] = {
        p4_game_table_input_span(boot.input, 9), p4_game_table_input_span(boot.input, 10)
    };
    const uint32_t headers[2] = {P4_GPDATA(uint32_t, 0xb428), P4_GPDATA(uint32_t, 0xb42c)};
    int result = P4_TABLE_OK;
    for (size_t i = 0; i < 2 && !result; ++i)
        result = p4_game_elements_ai_source_bind(boot.prepared, scripts[i],
            P4_G2H(headers[i]), p4_game_table_input_span_size(scripts[i]), headers[i]);
    const uint32_t native_rows = P4_GPDATA(uint32_t, 0xb3d0);
    if (!result) result = p4_game_elements_ai_bank_bind(boot.prepared, rows,
        P4_G2H(native_rows), p4_game_table_input_span_size(rows), native_rows);
    if (!result) result = p4_game_elements_ai_boot_stage(root->tables, 1, boot.prepared);
    if (result) return p4_package_boot_fail(NULL, p4_game_tables_error(result));
    return p4_package_boot_provide(boot.prepared);
}
int p4_game_elements_ai_boot_session_end(void)
{
    /* Core has already quiesced script/action/sidecar consumers and will clear
     * the shared completion graph before releasing Root's borrowed products. */
    if (!boot.cpu) { memset(&boot, 0, sizeof boot); return P4_TABLE_OK; }
    int result = p4_game_elements_ai_session_end();
    if (result) return result;
    result = p4_game_elements_ai_sources_destroy(boot.prepared);
    if (!result) memset(&boot, 0, sizeof boot);
    return result;
}
static const char *const dependencies[] = {P4_GAME_TABLES_BOOT_PROVIDER, NULL};
const P4PackageHandler p4_game_elements_ai_boot_provider = {
    .name = "game-tables.enemy-ai", .prepare = prepare, .publish = publish,
    .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES,
    .dependencies = dependencies, .runtime_prepare = runtime_prepare
};
