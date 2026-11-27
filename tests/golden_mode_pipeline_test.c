/* Synthetic adapters, real cache, packed-input admission and M4 declarations.
 * The family/source sinks are the explicit recording fixture, not native game
 * publication; no game content is used. */
#define P4_GOLDEN_REAL_INPUT_FIXTURE
#define main golden_declare_fixture_main
#include "golden_mode_declare_builder_test.c"
#undef main
#include "../src/platform/golden/golden_mode_boot.h"
#include "../src/platform/golden/golden_mode_cache_publish.h"
#include "p4pc/os.h"
#include "p4pc/golden_skill_convert.h"
#include "p4pc/golden_source.h"
#include "p4pc/golden_exp.h"
#include "p4pc/content_digest.h"
#include "golden_cpk_fixture.h"
#include "golden_cache_fixture.h"
#include "iso9660.h"

static void cleanup_entry(const char *name, void *user)
{
    const char *directory = user;
    char path[1600];
    if (!strcmp(name, ".") || !strcmp(name, "..")) return;
    int n = snprintf(path, sizeof path, "%s/%s", directory, name);
    if (n <= 0 || (size_t)n >= sizeof path) abort();
    if (p4_os_is_dir_nofollow(path)) {
        if (p4_os_list_dir_utf8(path, cleanup_entry, path) || p4_os_rmdir(path)) abort();
    } else if (p4_os_remove_utf8(path)) abort();
}

int p4_golden_exp_test_read(const char *, uint8_t [396], char [65],
                            const char *, const char *, char *, size_t);
P4GameMode p4_game_mode(void) { return P4_GAME_MODE_GOLDEN; }
int p4_test_actual_iso_walk(FILE *, long long, iso_visit_fn, void *);
int iso_walk(FILE *stream, long long base, iso_visit_fn visit, void *user)
{ return p4_test_actual_iso_walk(stream, base, visit, user); }
#ifdef P4_GOLDEN_STANDALONE_FIXTURE
static _Noreturn void unexpected_source(void) { abort(); }
int vfs_source_prepare_selected(void) { unexpected_source(); }
int vfs_source_plan_acquire(P4VfsOriginKind k, const char *p, P4VfsSourcePlan **o)
{ (void)k; (void)p; (void)o; unexpected_source(); }
const P4VfsSourceInfo *vfs_source_plan_info(const P4VfsSourcePlan *p) { (void)p; unexpected_source(); }
int vfs_source_plan_read_at(P4VfsSourcePlan *p, uint64_t o, void *b, size_t n)
{ (void)p; (void)o; (void)b; (void)n; unexpected_source(); }
size_t p4_packages_count(void) { unexpected_source(); }
const P4PackageProvider *p4_packages_provider(size_t i) { (void)i; unexpected_source(); }
int p4_package_provider_selected(const P4PackageProvider *p) { (void)p; unexpected_source(); }
size_t p4_package_provider_files_count(const P4PackageProvider *p) { (void)p; unexpected_source(); }
int p4_package_provider_replacement(const P4PackageProvider *p, size_t i, const char **k, int *d)
{ (void)p; (void)i; (void)k; (void)d; unexpected_source(); }
int p4_package_provider_file(const P4PackageProvider *p, size_t i, const char **k, int *d)
{ (void)p; (void)i; (void)k; (void)d; unexpected_source(); }
int p4_package_provider_retain(const P4PackageProvider *p) { (void)p; unexpected_source(); }
int p4_package_file_open(const P4PackageProvider *p, const char *n, uint64_t m, P4PackageFile **o)
{ (void)p; (void)n; (void)m; (void)o; unexpected_source(); }
int p4_package_file_plan_acquire(P4PackageFile *f, P4PackageFilePlan **o)
{ (void)f; (void)o; unexpected_source(); }
int p4_package_file_close(P4PackageFile *f) { (void)f; unexpected_source(); }
uint64_t p4_package_file_size(const P4PackageFile *f) { (void)f; unexpected_source(); }
int p4_package_file_read_at(P4PackageFile *f, uint64_t o, void *b, size_t n)
{ (void)f; (void)o; (void)b; (void)n; unexpected_source(); }
int p4_packages_memory_reserve(size_t n) { (void)n; return 0; }
void p4_packages_memory_commit(size_t n) { (void)n; }
void p4_packages_memory_cancel(size_t n) { (void)n; }
void p4_packages_memory_release(size_t n) { (void)n; }
void vfs_source_plan_release(P4VfsSourcePlan *plan) { if (plan) abort(); }
void p4_package_file_plan_release(P4PackageFilePlan *plan) { if (plan) abort(); }
void p4_package_provider_release(const P4PackageProvider *provider) { if (provider) abort(); }
int p4_test_actual_iso_walk(FILE *f, long long b, iso_visit_fn v, void *u)
{ (void)f; (void)b; (void)v; (void)u; return -1; }
#endif
static char fixture_cpk[1200], fixture_exe[1200], exe_hash[65], exp_hash[65];
static int write_fixture_exe(const char *path)
{
    uint8_t pe[1024] = {0};
    pe[0] = 'M'; pe[1] = 'Z'; le32(pe + 60, 64);
    memcpy(pe + 64, "PE\0\0", 4); pe[68] = 0x64; pe[69] = 0x86; pe[70] = 1;
    memcpy(pe + 88, ".sdata", 6); le32(pe + 104, 512); le32(pe + 108, 512);
    le32(pe + 124, 0x40000040);
    for (unsigned i = 0; i < 99; ++i) le32(pe + 516 + i * 4, i * 100);
    p4_content_digest(pe, sizeof pe, exe_hash);
    p4_content_digest(pe + 516, 396, exp_hash);
    FILE *out = fopen(path, "wb");
    if (!out) return -1;
    int ok = fwrite(pe, 1, sizeof pe, out) == sizeof pe;
    return fclose(out) || !ok ? -1 : 0;
}
static int write_fixture_cpk(const char *path)
{
    static uint8_t file[0x2000 + 441 * 44], packet[8192];
    static const UtfCol header[] = {{"TocOffset", 6}, {"ContentOffset", 6}};
    static const UtfCol toc[] = {{"DirName", 0x0A}, {"FileName", 0x0A},
                                 {"FileSize", 4}, {"ExtractSize", 4}, {"FileOffset", 6}};
    const char *strings[] = {"init", "skill.bin", NULL, NULL, NULL};
    uint64_t header_values[] = {0x800, 0x2000};
    uint64_t toc_values[] = {0, 0, 441 * 44, 441 * 44, 0x1800};
    size_t size = utf_build(packet, header, 2, 1, NULL, header_values, 0);
    memcpy(file, "CPK ", 4); le32(file + 8, (uint32_t)size); memcpy(file + 0x10, packet, size);
    size = utf_build(packet, toc, 5, 1, strings, toc_values, 0);
    memcpy(file + 0x800, "TOC ", 4); le32(file + 0x808, (uint32_t)size); memcpy(file + 0x810, packet, size);
    file[0x2000 + 440 * 44] = 3;
    FILE *out = fopen(path, "wb");
    if (!out) return -1;
    int ok = fwrite(file, 1, sizeof file, out) == sizeof file;
    return fclose(out) || !ok ? -1 : 0;
}
static unsigned captures, capture_rollbacks;
static int capture_failure;
int p4_data_catalog_capture_golden(const uint8_t *msg, size_t msg_size,
                                    const uint8_t *items, size_t items_size)
{
    CHECK(msg && msg_size == 80 && items && items_size == 1);
    CHECK(msg[4] == 0 && items[0] == 42); /* cache hit ignores mutated sources */
    ++captures;
    return capture_failure ? -1 : 0;
}
void p4_data_catalog_golden_rollback(void) { ++capture_rollbacks; }
static int reject_declarations(void *context, P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCacheTable *extra, size_t count, char *reason, size_t capacity)
{ (void)context; (void)builder; (void)extra; (void)count; (void)reason; (void)capacity; return -1; }
static unsigned callbacks, rollbacks;
static int convert(void *context, char *reason, size_t capacity)
{ (void)reason; (void)capacity; if (!context) return -1; ++callbacks; return 0; }
static int project_skill(void *context, char *reason, size_t capacity)
{
    P4GoldenModeCachePlan *plan = context;
    P4GoldenCpk cpk; P4GoldenCpkEntry entry;
    uint8_t *source = NULL; size_t size = 0, written = 0;
    if (!plan || !plan->context) return -1;
    char actual_hash[65]; int exp_found = 0;
    for (size_t i = 0; i < plan->table_count; ++i)
        if (!strcmp(plan->tables[i].name, "decl/experience")) {
            if (plan->tables[i].size != 396 ||
                p4_golden_exp_test_read(fixture_exe, plan->tables[i].data, actual_hash,
                                        exe_hash, exp_hash, reason, capacity)) return -1;
            exp_found = 1;
        }
    if (!exp_found) return -1;
    if (p4_golden_cpk_open(&cpk, fixture_cpk, reason, capacity)) return -1;
    int result = p4_golden_cpk_find(&cpk, "init/skill.bin", &entry) ||
                 p4_golden_cpk_read(&cpk, &entry, &source, &size, reason, capacity);
    p4_golden_cpk_close(&cpk);
    if (result) return -1;
    for (size_t i = 0; i < plan->table_count; ++i)
        if (!strcmp(plan->tables[i].name, "decl/skill-metadata")) {
            ++callbacks;
            result = p4_golden_skill_metadata(plan->context, source, size,
                (uint8_t *)plan->tables[i].data, plan->tables[i].size, &written, reason, capacity) ||
                written != plan->tables[i].size ? -1 : 0;
            free(source);
            return result;
        }
    free(source);
    return -1;
}
static void rollback(void *context) { (void)context; ++rollbacks; }
static int publish(void *context, P4GameTablesDefinitionBuilder *builder, char *reason, size_t capacity)
{ return p4_golden_mode_cache_publish(context, builder, reason, capacity); }
static int table_compare(const void *a, const void *b)
{ return strcmp(((const P4GoldenCacheTable *)a)->name, ((const P4GoldenCacheTable *)b)->name); }
int main(int argc, char **argv)
{
    static uint8_t banks[P4_TABLE_INPUT_COUNT][176];
    static uint8_t aux[577], elements[577 * 4], groups[577], metadata[577 * P4_SKILL_ROW_SIZE];
    static uint8_t presentation[577 * 4], rates[577], experience[396];
    static uint8_t item_names[] = {42};
    static const unsigned blocks[] = {1,2,3,5,6,5,1,3,11};
    char root[1024], pref[1100], install[1100], program[1100], reason[192] = "";
    (void)argv;
    CHECK(argc == 1);
    const char *temp = golden_cache_fixture_temp();
    if (!temp) return 1;
    int root_len = snprintf(root, sizeof root, "%s/golden-mode-pipeline-%llu", temp, (unsigned long long)p4_os_now_ns());
    CHECK(root_len > 0 && (size_t)root_len < sizeof root);
    CHECK(!p4_os_mkdir(root));
    snprintf(pref, sizeof pref, "%s/pref", root); CHECK(!p4_os_mkdir(pref));
    snprintf(install, sizeof install, "%s/install", root); CHECK(!p4_os_mkdir(install));
    snprintf(fixture_cpk, sizeof fixture_cpk, "%s/data_e.cpk", install);
    CHECK(!write_fixture_cpk(fixture_cpk));
    snprintf(fixture_exe, sizeof fixture_exe, "%s/P4G.exe", install);
    CHECK(!write_fixture_exe(fixture_exe));
    snprintf(program, sizeof program, "%s/program", root); CHECK(!p4_os_mkdir(program));
    P4GoldenCachePaths paths = {pref, install, program, NULL};
    P4GoldenCacheIdentityEntry sources[] = {{"P4G.exe", exe_hash}};
    P4GoldenCacheIdentityEntry versions[] = {{"schema", "1"}};
    P4GoldenCacheIdentity identity = {sources, 1, versions, 1};
    P4GoldenDomainMap map; const P4GoldenDomainShape shape = {577,441,336,1536,256};
    CHECK(!p4_golden_domain_map_build(&map, &shape, reason, sizeof reason));
    uint8_t text[40] = {2,0,0,0, 1,0,0,0, 0x40,2,0,0, 0,0,0,0, 2,0,0,0, 'x',0,
                        1,0,0,0, 0x40,2,0,0, 1,0,0,0, 2,0,0,0, 'y',0};
    P4GoldenCacheTable extra[] = {
        {"decl/experience", NULL, 0},
        {"decl/item-names", NULL, 0},
        {"decl/skill-aux", NULL, 0}, {"decl/skill-elements", NULL, 0}, {"decl/skill-groups", NULL, 0},
        {"decl/skill-metadata", NULL, 0}, {"decl/skill-presentation", NULL, 0}, {"decl/skill-rates", NULL, 0},
        {"decl/text", NULL, 0}
    };
    P4GoldenCacheTable tables[P4_TABLE_INPUT_COUNT + 9];
    for (unsigned i = 0; i < P4_TABLE_INPUT_COUNT; ++i)
        tables[i] = (P4GoldenCacheTable){p4_game_table_input_label((P4GameTableInputKind)i), banks[i], blocks[i] * 16u};
    tables[9] = (P4GoldenCacheTable){extra[0].name, experience, sizeof experience};
    tables[10] = (P4GoldenCacheTable){extra[1].name, item_names, sizeof item_names};
    tables[11] = (P4GoldenCacheTable){extra[2].name, aux, sizeof aux};
    tables[12] = (P4GoldenCacheTable){extra[3].name, elements, sizeof elements};
    tables[13] = (P4GoldenCacheTable){extra[4].name, groups, sizeof groups};
    tables[14] = (P4GoldenCacheTable){extra[5].name, metadata, sizeof metadata};
    tables[15] = (P4GoldenCacheTable){extra[6].name, presentation, sizeof presentation};
    tables[16] = (P4GoldenCacheTable){extra[7].name, rates, sizeof rates};
    tables[17] = (P4GoldenCacheTable){extra[8].name, text, sizeof text};
    qsort(tables, sizeof tables / sizeof tables[0], sizeof tables[0], table_compare);
    P4GoldenModeCachePlan plan = {&paths, &identity, tables, sizeof tables / sizeof tables[0], extra,
                                  sizeof extra / sizeof extra[0], p4_golden_mode_declare_cached, &map};
    P4GoldenModeAdapter entries[] = {
        {"verified sources and projections", project_skill}, {"EFFECT0 Golden rows (472x12)", convert},
        {"MODEL0 Golden rows (11x382)", convert},
        {"AICALC9/10 compatibility", convert}, {"expanded enemy model sources", convert},
        {"expanded item icon sources", convert}, {"expanded item categories and actions", convert},
        {"expanded item models", convert},
        {"expanded entity names and descriptions", convert}, {"passive skill semantic elements", convert},
        {"widened skill flags", convert}, {"skill and persona name widths", convert},
        {"R2 EFFECT0 packet timing", convert}, {"M8f-res:battle-common-14A8-1490", convert},
        {"image supplier ledger", convert},
        {"R7a-dep:M11-skill-card-UI-U1-U4", convert},
        {"R7a-dep:M11-return-field-flow", convert},
        {"R7a-dep:shop-permissions-context-flags-U2-U3-U5", convert},
        {"P10-dep:E2-forced-retry-liveness", convert},
        {"Golden BF command compatibility", convert},
        {"Golden script feature compatibility", convert},
        {"native Golden asset families", convert},
        {"M6 image-table cache binder (chart, gate, recipe, EXP, inheritance)", convert}
    };
    P4GoldenModeAdapters adapters = {entries, sizeof entries / sizeof entries[0], publish, rollback, &plan};
    for (unsigned pass = 0; pass < 2; ++pass) {
        P4GameTablesDefinitionBuilder builder = {0};
        builder.tables = (P4GameTables *)(uintptr_t)3; builder.sources = (P4GameTableSources *)(uintptr_t)2;
        builder.registry_revision = 1;
        builder.input_mask = (1u << P4_TABLE_INPUT_MSG) | (1u << P4_TABLE_INPUT_ITEM);
        if (pass) {
            banks[P4_TABLE_INPUT_SKILL][4] = 0x55;
            banks[P4_TABLE_INPUT_MSG][4] = 0x55;
            item_names[0] = 99;
        }
        if (p4_golden_mode_boot_prepare(P4_GAME_MODE_GOLDEN, &builder, &adapters, reason, sizeof reason)) {
            fprintf(stderr,"Golden boot pass %u: %s (pref=%s)\n",pass,reason,pref);
            return 1;
        }
        CHECK(builder.input_count == P4_TABLE_INPUT_COUNT && registrations == pass + 1 && text_registrations == (pass + 1) * 2);
        const P4GameTableInput *skill_input = NULL;
        for (size_t i = 0; i < builder.input_count; ++i)
            if (p4_game_table_input_kind(builder.inputs[i]) == P4_TABLE_INPUT_SKILL) skill_input = builder.inputs[i];
        CHECK(skill_input && p4_game_table_input_bytes(skill_input)[4] == 0);
        for (size_t i = 0; i < builder.input_count; ++i) p4_game_table_input_destroy(builder.inputs[i]);
    }
    CHECK(!rollbacks && callbacks == 2u * (sizeof entries / sizeof entries[0]));
    CHECK(captures == 2 && !capture_rollbacks);
    {
        P4GameTablesDefinitionBuilder builder = {0};
        builder.tables = (P4GameTables *)(uintptr_t)3; builder.sources = (P4GameTableSources *)(uintptr_t)2;
        builder.registry_revision = 1;
        capture_failure = 1;
        CHECK(p4_golden_mode_cache_publish(&plan, &builder, reason, sizeof reason));
        CHECK(captures == 3 && capture_rollbacks == 1 && registrations == 2);
        for (size_t i = 0; i < builder.input_count; ++i) p4_game_table_input_destroy(builder.inputs[i]);
    }
    {
        P4GameTablesDefinitionBuilder builder = {0};
        builder.tables = (P4GameTables *)(uintptr_t)3; builder.sources = (P4GameTableSources *)(uintptr_t)2;
        builder.registry_revision = 1;
        capture_failure = 0;
        plan.declare = reject_declarations;
        CHECK(p4_golden_mode_cache_publish(&plan, &builder, reason, sizeof reason));
        CHECK(captures == 4 && capture_rollbacks == 2);
        for (size_t i = 0; i < builder.input_count; ++i) p4_game_table_input_destroy(builder.inputs[i]);
    }
    CHECK(!p4_os_list_dir_utf8(root, cleanup_entry, root) && !p4_os_rmdir(root));
    puts("golden_mode_pipeline: real synthetic cache build, verified reload and M4 declaration sink passed"); return 0;
}
