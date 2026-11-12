#include "game_knowledge_runtime_internal.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/package_boot.h"
#include "p4pc/packages.h"
#include <stdio.h>
#include <string.h>

/* Root owns the borrowed tables/sources. Only an expanded element cohort
 * needs knowledge; retail and item/skill-only tables stay untouched. */
static struct KnowledgeBoot {
    const P4GameTables *tables;
    const P4GameTableSources *sources;
    int exposed;
} boot;

static int prepare(void *unused)
{
    const void *product = NULL;
    (void)unused;
    if (boot.tables || boot.exposed) return -1;
    if (p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &product)) return -1;
    const P4GameTablesBootProduct *root = product;
    if (!root || !p4_game_elements_requested(root->tables)) return p4_package_boot_provide(NULL);
    int result = p4_game_knowledge_save_register(root->tables, root->sources);
    if (result) {
        fprintf(stderr, "p4pc: enemy-knowledge save block registration refused: %s\n", p4_game_tables_error(result));
        return -1;
    }
    boot.tables = root->tables;
    boot.sources = root->sources;
    return p4_package_boot_provide(NULL);
}
static void publish(void *unused)
{
    (void)unused;
    boot.exposed = 1;
}
static void rollback(void *unused)
{
    (void)unused;
    if (boot.exposed) p4_package_boot_runtime_failed("Enemy knowledge rollback attempted after native raw exposure.");
    if (p4_game_knowledge_save_unregister())
        p4_package_boot_runtime_failed("Early enemy-knowledge rollback could not detach its own save block.");
    memset(&boot, 0, sizeof boot);
}
static int runtime_prepare(void *unused)
{
    const void *product = NULL;
    (void)unused;
    if (p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &product))
        return p4_package_boot_fail(NULL, "Enemy knowledge runtime has no table-source product.");
    const P4GameTablesBootProduct *root = product;
    if (!boot.tables) {
        if (root && p4_game_elements_requested(root->tables))
            return p4_package_boot_fail(NULL, "Expanded enemy knowledge has no registered save block.");
        return p4_package_boot_provide(NULL);
    }
    if (!root || root->tables != boot.tables || root->sources != boot.sources)
        return p4_package_boot_fail(NULL, "Enemy knowledge runtime tables differ from its CPU registration.");
    int result = p4_game_knowledge_native_boot_stage(boot.tables);
    if (result) return p4_package_boot_fail(NULL, p4_game_tables_error(result));
    return p4_package_boot_provide(NULL);
}
int p4_game_knowledge_boot_session_end(void)
{
    int result = p4_game_knowledge_save_unregister();
    if (result) return result;
    p4_game_knowledge_native_discard();
    memset(&boot, 0, sizeof boot);
    return P4_TABLE_OK;
}
static const char *const dependencies[] = {P4_GAME_TABLES_BOOT_PROVIDER, NULL};
const P4PackageHandler p4_game_knowledge_boot_provider = {
    .name = "game-tables.enemy-knowledge", .prepare = prepare, .publish = publish,
    .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES,
    .dependencies = dependencies, .runtime_prepare = runtime_prepare
};
