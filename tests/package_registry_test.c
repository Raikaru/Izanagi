/* Synthetic native service witness, not a production decoder or game boot. */
#include "p4pc/packages.h"
#include "p4pc/os.h"
#include "vfs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "package_sort.h"
#include "p4pc/mod_setting_store.h"
void p4pc_state_dump_flip(unsigned flip); /* actual state_dump.c fidelity harness */

static int failures, prepares, publishes, rollbacks, observed, installations, invalid;
static const char *mode;
static P4PackageFile *held;
static const P4PackageProvider *pinned;
static uint32_t old_generation;
static P4PackageKind old_kind;
static int refusals;
static const P4PackageProvider *old_pack;
static unsigned char refused_info[sizeof(P4PackageInfo)];
static P4PackageFilePlan *issued_plan;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "package_registry: FAIL line%d: %s\n", __LINE__, #x); ++failures; } } while (0)

static void observe(const P4PackageFileOrigin *origin, const void *bytes, size_t size, uint64_t offset, void *user)
{
    CHECK(user == &observed);
    CHECK(origin && origin->package && origin->relative);
    CHECK(size && bytes && offset < 64);
    CHECK(origin->package->generation == old_generation);
    CHECK(origin->package->kind == old_kind);
    CHECK(!strcmp(origin->relative, "Textures/p4tex-v1/A1.BIN"));
    CHECK(origin->package == p4_package_provider_info(pinned));
    ++observed;
}
static int installation(const P4PackageInstallation *entry, void *user)
{
    CHECK(user == &installations);
    CHECK(entry->folder && entry->directory);
    ++installations;
    if (!entry->info) {
        CHECK(entry->diagnostic && *entry->diagnostic);
        ++invalid;
        printf("invalid=%s line=%u reason=%s\n", entry->folder, entry->line, entry->diagnostic);
    } else {
        CHECK(!entry->diagnostic);
        CHECK(entry->info->ordinal == SIZE_MAX && entry->info->generation == 0);
        printf("installed=%s api=%u.%u fields=%zu\n", entry->info->id,
            entry->info->api_major, entry->info->api_minor, entry->field_count);
    }
    CHECK(prepares == 0 && publishes == 0 && p4_packages_count() == 0);
    return 0;
}
static int prepare(void *user)
{
    CHECK(user == &prepares);
    ++prepares;
    CHECK(p4_packages_count() == 2);
    CHECK(!strcmp(p4_packages_info(0)->id, "pack.low"));
    P4PackageMemoryUsage metadata = p4_packages_memory_usage();
    CHECK(metadata.resident > 0 && metadata.reserved == 0 && metadata.peak <= metadata.limit);
    CHECK(!strcmp(p4_packages_info(1)->id, "mod.high"));
    for (size_t ordinal = 0; ordinal < p4_packages_count(); ++ordinal) {
        const P4PackageProvider *provider = p4_packages_provider(ordinal);
        const P4PackageInfo *info = p4_package_provider_info(provider);
        CHECK(info == p4_packages_info(ordinal) && info->ordinal == ordinal);
        size_t count;
        const P4PackageField *fields = p4_package_provider_fields(provider, &count);
        CHECK(fields && count >= 1);
        int found_texture = 0;
        size_t enumeration_memory = p4_packages_memory_usage().resident;
        for (size_t i = 0; i < p4_package_provider_files_count(provider); ++i) {
            const char *key;
            int directory;
            CHECK(p4_package_provider_file(provider, i, &key, &directory) == 0);
            if (!directory && !strcmp(key, "Textures/p4tex-v1/A1.BIN")) found_texture = 1;
        }
        CHECK(found_texture);
        CHECK(p4_packages_memory_usage().resident == enumeration_memory);
        CHECK(p4_package_provider_claim(provider, P4_PACKAGE_CLAIM_COSMETIC) == 0);
        if (!strcmp(mode, "cosmetic") || !strcmp(mode, "cosmetic-mixed"))
            CHECK(p4_package_provider_require_cosmetic(provider) == 0);
        if (ordinal == 0) {
            CHECK(p4_package_provider_retain(provider) == 0);
            pinned = provider;
            old_generation = info->generation;
            old_kind = info->kind;
            CHECK(p4_package_file_open(provider, "../outside", 64, &held) == 1);
            CHECK(p4_package_file_open(provider, "Textures", 64, &held) == -1);
            CHECK(p4_package_file_open(provider, "textures/P4TEX-V1/a1.bin", 3, &held) == -1);
            CHECK(p4_package_file_open(provider, "textures/P4TEX-V1/a1.bin", 64, &held) == 0);
            CHECK(p4_package_file_size(held) == 12);
            P4PackageMemoryUsage before = p4_packages_memory_usage();
            size_t available = before.limit - before.resident - before.reserved;
            CHECK(p4_packages_memory_reserve(available) == 0);
            P4PackageFile *denied = NULL;
            CHECK(p4_package_file_open(provider, "Textures/p4tex-v1/A1.BIN", 64, &denied) == -1);
            CHECK(denied == NULL && p4_packages_memory_usage().resident == before.resident);
            p4_packages_memory_cancel(available);
        }
    }
    return !strcmp(mode, "prepare-fail") ? -1 : 0;
}
static void publish(void *user) { CHECK(user == &prepares); ++publishes; }
/* The test's already-issued read handle models independent in-flight work. It
 * stays pinned after rollback; no prepared resource is attached/published. */
static void rollback(void *user) { CHECK(user == &prepares); ++rollbacks; }
static const char *const sections[] = {"content", "texture", "music", NULL};
static const P4PackageHandler handler = {"fixture", sections, prepare, publish, rollback, &prepares};

/* [setting] in a pack is refused through the pack selector; the mod keeps its value. */
static int settings_refusal(const char *id, const char *reason, void *user)
{
    CHECK(user == &refusals && reason && reason[0]);
    ++refusals;
    printf("refused=%s reason=%s\n", id, reason);
    return 0;
}
static int settings_kinds(P4PackageClient *mod_client, P4PackageClient *pack_client, const char *mod_root,
                          const char *pack_root)
{
    P4PackageRegistry *mods = NULL, *packs = NULL;
    char settings[1200], value[32];
    snprintf(settings, sizeof settings, "%s/../settings", mod_root);
    mod_client->refuse = pack_client->refuse = settings_refusal;
    mod_client->user = pack_client->user = &refusals;
    CHECK(p4_mod_setting_store_root(settings) == 0);
    CHECK(p4_package_handler_register(&p4_mod_settings_boot_provider) == 0);
    CHECK(p4_package_registry_load(&mods, mod_client, mod_root, "mod.high") == 0);
    CHECK(p4_package_registry_load(&packs, pack_client, pack_root, "pack.low") == 0);
    CHECK(p4_package_registry_publish() == 0);
    CHECK(refusals == 1 && p4_packages_count() == 1 && !strcmp(p4_packages_info(0)->id, "mod.high"));
    if (p4_packages_count() == 1) {
        CHECK(p4_mod_setting_boot(p4_packages_provider(0), "flag", value, sizeof value) == 1 && !strcmp(value, "on"));
        char reason[128];
        CHECK(p4_mod_setting_store("mod.high", "flag", "off", reason, sizeof reason) == 0);
        CHECK(p4_mod_setting_boot(p4_packages_provider(0), "flag", value, sizeof value) == 1 && !strcmp(value, "on"));
    }
    p4_package_registry_retire();
    printf("setting kinds: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
/* Requirements: version comparison, then the admitted catalog order and every
 * refusal (printed by settings_refusal) for the selection in the environment. */
static int requirements_witness(P4PackageClient *mod_client, P4PackageClient *pack_client, const char *mod_root,
                                const char *pack_root)
{
    CHECK(p4_package_version_satisfies("1.0", 1, 0) == 1 && p4_package_version_satisfies("1.2.3-rc1", 1, 2) == 1);
    CHECK(p4_package_version_satisfies("2", 2, 0) == 1 && p4_package_version_satisfies("1.10+build", 1, 9) == 1);
    CHECK(p4_package_version_satisfies("1.1", 1, 2) == 0 && p4_package_version_satisfies("2.0", 1, 0) == 0);
    CHECK(p4_package_version_satisfies("beta", 1, 0) == -1 && p4_package_version_satisfies("1.0.0.0", 1, 0) == -1);
    CHECK(p4_package_version_satisfies("1.", 1, 0) == -1 && p4_package_version_satisfies("1.0 final", 1, 0) == -1);
    P4PackageRegistry *mods = NULL, *packs = NULL;
    char settings[1200];
    snprintf(settings, sizeof settings, "%s/../settings", mod_root);
    mod_client->refuse = pack_client->refuse = settings_refusal;
    mod_client->user = pack_client->user = &refusals;
    CHECK(p4_mod_setting_store_root(settings) == 0);
    CHECK(p4_package_handler_register(&p4_mod_settings_boot_provider) == 0);
    const char *enabled = getenv("P4PC_TEST_MODS"), *packs_enabled = getenv("P4PC_TEST_PACKS");
    const char *order = getenv("P4PC_TEST_ORDER"), *refused = getenv("P4PC_TEST_REFUSED");
    CHECK(p4_package_registry_load(&mods, mod_client, mod_root, enabled) == 0);
    if (packs_enabled && packs_enabled[0])
        CHECK(p4_package_registry_load(&packs, pack_client, pack_root, packs_enabled) == 0);
    if (order && order[0]) CHECK(p4_package_registry_set_order(mods, order) == 0);
    CHECK(p4_package_registry_publish() == 0);
    printf("admitted=");
    for (size_t i = 0; i < p4_packages_count(); ++i) {
        const P4PackageInfo *info = p4_packages_info(i);
        CHECK(info->ordinal == i);
        printf("%s%s", i ? "," : "", info->id);
    }
    printf("\n");
    /* Every admitted package's answer for each named companion id, -1 when undeclared. */
    const char *companion = getenv("P4PC_TEST_COMPANION");
    for (size_t i = 0; companion && companion[0] && i < p4_packages_count(); ++i)
        printf("companion %s of %s=%d\n", companion, p4_packages_info(i)->id,
               p4_packages_companion_admitted(p4_packages_provider(i), companion));
    if (refused && refused[0]) {
        const char *why = p4_package_refusal(P4_PACKAGE_MOD, refused);
        printf("refusal %s=%s\n", refused, why ? why : "(none)");
    }
    p4_package_registry_retire();
    printf("requirements: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
static int admission_refusal(const char *id, const char *reason, void *user)
{
    CHECK(user == &refusals && !strcmp(id, "mod.high") && reason && reason[0]);
    ++refusals;
    printf("refused=%s reason=%s\n", id, reason);
    return 0;
}
static int admission_preflight(void *user)
{
    CHECK(user == &prepares);
    if (!strcmp(mode, "admit-capability")) CHECK(prepares == 0);
    if (!strcmp(mode, "admit-capability") && p4_packages_count() == 2)
        return p4_package_provider_refuse(p4_packages_provider(1), "Fixture-declared native capability unavailable");
    return 0;
}
static int admission_prepare(void *user)
{
    CHECK(user == &prepares);
    ++prepares;
    if (!strcmp(mode, "admit-prepare") && prepares == 1) {
        CHECK(p4_packages_count() == 2);
        pinned = p4_packages_provider(1);
        old_pack = p4_packages_provider(0);
        CHECK(p4_package_provider_retain(pinned) == 0 && p4_package_provider_retain(old_pack) == 0);
        const P4PackageInfo *info = p4_package_provider_info(pinned);
        old_generation = info->generation;
        old_kind = info->kind;
        memcpy(refused_info, info, sizeof refused_info);
        CHECK(p4_package_file_open(pinned, "Textures/p4tex-v1/A1.BIN", 64, &held) == 0);
        char header[4];
        CHECK(p4_package_file_read_at(held, 0, header, sizeof header) == 0);
        CHECK(memcmp(header, "P4HD", sizeof header) != 0);
        CHECK(p4_package_file_plan_acquire(held, &issued_plan) == 0);
        CHECK(p4_package_provider_refuse(pinned, "Fixture format header is not P4HD") == 0);
        return -1; /* mandatory I/O failed; independent old read work stays pinned */
    }
    CHECK(p4_packages_count() == 1 && !strcmp(p4_packages_info(0)->id, "pack.low"));
    if (old_pack) {
        CHECK(old_pack != p4_packages_provider(0));
        CHECK(p4_package_provider_same_source(old_pack, p4_packages_provider(0)));
        CHECK(p4_package_provider_info(old_pack)->generation != p4_packages_info(0)->generation);
        CHECK(!memcmp(refused_info, p4_package_provider_info(pinned), sizeof refused_info));
    }
    return 0;
}

static int file_plan_witness(void)
{
    CHECK(held && pinned);
    if (!held || !pinned) return 2;
    P4PackageFilePlan *plan = NULL, *cached = NULL;
    if (!strcmp(mode, "plan-budget")) {
        P4PackageMemoryUsage before = p4_packages_memory_usage();
        size_t remaining = before.limit - before.resident - before.reserved;
        CHECK(p4_packages_memory_reserve(remaining) == 0);
        CHECK(p4_package_file_plan_acquire(held, &plan) == -1 && plan == NULL);
        CHECK(p4_packages_memory_usage().resident == before.resident);
        p4_packages_memory_cancel(remaining);
    }
    CHECK(p4_package_file_plan_acquire(held, &plan) == 0);
    size_t resident = p4_packages_memory_usage().resident;
    CHECK(p4_package_file_plan_acquire(held, &cached) == 0 && cached == plan);
    CHECK(p4_packages_memory_usage().resident == resident); /* Same issued lineage, no allocation. */
    p4_package_file_plan_release(cached);
    unsigned char info[sizeof(P4PackageInfo)];
    memcpy(info, p4_package_provider_info(pinned), sizeof info);
    p4_package_registry_retire();
    P4PackageFilePlan *denied = NULL;
    CHECK(p4_package_file_plan_acquire(held, &denied) == -1 && denied == NULL);
    P4PackageFile *fresh = NULL;
    CHECK(p4_package_file_open(pinned, "Textures/p4tex-v1/A1.BIN", 64, &fresh) == -1);
    CHECK(p4_package_file_close(held) == 0);
    held = NULL;
    p4_package_provider_release(pinned); /* Remaining lifetime belongs ONLY to the plan. */
    CHECK(p4_package_file_plan_retain(plan) == 0);
    p4_package_file_plan_release(plan);
    char path[1300], previous[1400];
    int length = snprintf(path, sizeof path, "%s/Textures/p4tex-v1/A1.BIN", p4_package_provider_info(pinned)->directory);
    CHECK(length > 0 && (size_t)length < sizeof path);
    int changed = !strcmp(mode, "plan-replaced") || !strcmp(mode, "plan-grown");
    if (changed) {
        if (!strcmp(mode, "plan-replaced")) {
            length = snprintf(previous, sizeof previous, "%s.old", path);
            CHECK(length > 0 && (size_t)length < sizeof previous);
            CHECK(p4_os_rename(path, previous) == 0);
        }
        FILE *replacement = fopen(path, !strcmp(mode, "plan-grown") ? "ab" : "wb");
        CHECK(replacement != NULL);
        if (replacement) {
            const char *bytes = !strcmp(mode, "plan-grown") ? "extra" : "image\0native";
            size_t size = !strcmp(mode, "plan-grown") ? 5 : 12;
            CHECK(fwrite(bytes, 1, size, replacement) == size);
            CHECK(fclose(replacement) == 0);
        }
    }
    P4PackageFile *delayed = NULL;
    CHECK(p4_package_file_plan_open(plan, &delayed) == (changed ? -1 : 0));
    if (delayed) {
        unsigned char bytes[12];
        CHECK(p4_package_file_size(delayed) == sizeof bytes);
        CHECK(p4_package_file_read_at(delayed, 0, bytes, sizeof bytes) == 0);
        CHECK(!memcmp(bytes, "image\0native", sizeof bytes));
        CHECK(p4_package_file_close(delayed) == 0);
    }
    CHECK(!memcmp(info, p4_package_provider_info(pinned), sizeof info));
    p4_package_file_plan_release(plan);
    pinned = NULL;
    CHECK(p4_package_reads_unobserve(observe, &observed) == 0);
    CHECK(p4_packages_memory_usage().resident == 0 && p4_packages_memory_usage().reserved == 0);
    printf("issued file lineage %s: %s observed=%d\n", mode, failures ? "FAIL" : "PASS", observed);
    return failures ? 1 : 0;
}

typedef struct SortRow { int key; unsigned char payload[16]; } SortRow;
static int sort_compare(const void *left, const void *right)
{
    int a = ((const SortRow *)left)->key, b = ((const SortRow *)right)->key;
    return (a > b) - (a < b);
}
static int sort_witness(void)
{
    SortRow rows[257];
    for (size_t i = 0; i < 257; ++i) {
        rows[i].key = (int)((i * 97) % 257);
        for (size_t j = 0; j < sizeof rows[i].payload; ++j)
            rows[i].payload[j] = (unsigned char)(rows[i].key ^ j);
    }
    p4_package_sort(rows, 0, sizeof rows[0], sort_compare);
    p4_package_sort(rows, 1, sizeof rows[0], sort_compare);
    p4_package_sort(rows, 257, sizeof rows[0], sort_compare);
    for (size_t i = 0; i < 257; ++i) {
        CHECK(rows[i].key == (int)i);
        for (size_t j = 0; j < sizeof rows[i].payload; ++j)
            CHECK(rows[i].payload[j] == (unsigned char)(i ^ j));
    }
    CHECK(p4_packages_memory_usage().peak == 0);
    printf("allocation-free metadata sort: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}

static int memory_policy_witness(void)
{
    CHECK(p4_packages_memory_configure(4096) == 0);
    CHECK(p4_packages_memory_reserve(1024) == 0);
    void *owned = malloc(1024);
    CHECK(owned != NULL);
    if (!owned) { p4_packages_memory_cancel(1024); return 1; }
    p4_packages_memory_commit(1024);
    CHECK(p4_packages_memory_reserve(3072) == 0);
    P4PackageMemoryUsage metadata = p4_packages_memory_usage();
    CHECK(metadata.resident == 1024 && metadata.reserved == 3072 && metadata.peak == 4096);
    CHECK(p4_packages_memory_reserve(1) == -1);
    CHECK(p4_packages_memory_configure(8192) == -1);
    p4_packages_memory_cancel(3072);
    free(owned);
    p4_packages_memory_release(1024);
    metadata = p4_packages_memory_usage();
    CHECK(metadata.resident == 0 && metadata.reserved == 0 && metadata.peak == 4096);
    printf("metadata policy: %s limit=%zu peak=%zu\n", failures ? "FAIL" : "PASS", metadata.limit, metadata.peak);
    return failures ? 1 : 0;
}

int main(int argc, char **argv)
{
    p4_os_init();
    if (argc != 4) return 2;
    mode = argv[1];
    if (!strcmp(mode, "sort")) return sort_witness();
    if (!strcmp(mode, "start-mode")) {
        p4_mod_fidelity_check_start();
        puts("ordinary harness start: PASS");
        return failures ? 1 : 0;
    }
    if (!strcmp(mode, "guard-invalid-witness-dump")) {
        CHECK(p4_packages_count() == 0);
        CHECK(p4_os_setenv("P4PC_EVENT_LOG", "1", 1) == 0);
        p4pc_state_dump_flip(1); /* must refuse before its first guest read */
        return 3;
    }
    if (!strcmp(mode, "guard-witness")) {
        p4_packages_assert_disabled(); /* Strict reference operation still refuses. */
        return 3;
    }
    P4PackageRegistry *mods = NULL, *packs = NULL;
    P4PackageClient mod_client = {.diagnostic_name = "mods", .manifest_name = "mod.ini", .section = "[mod]",
        .kind = P4_PACKAGE_MOD, .compendium = 1, .mount_files = 1};
    P4PackageClient pack_client = {.diagnostic_name = "packs", .manifest_name = "pack.ini", .section = "[pack]",
        .kind = P4_PACKAGE_ENGINE_PACK, .mount_files = 1};
    if (!strcmp(mode, "selector-order")) {
        CHECK(p4_package_registry_load(&mods, &mod_client, argv[2], "mod.low,mod.mid,mod.high") == 0);
        CHECK(p4_package_registry_load(&packs, &pack_client, argv[3], "pack.low") == 0);
        CHECK(p4_package_registry_set_order(mods, "mod.mid,mod.mid") == -1);
        CHECK(p4_package_registry_set_order(mods, "mod.mid,mod.high") == 0);
        CHECK(p4_package_registry_publish() == 0);
        const char *expected[] = {"mod.mid", "pack.low", "mod.high", "mod.low"};
        CHECK(p4_packages_count() == 4);
        for (size_t i = 0; i < 4 && i < p4_packages_count(); ++i)
            CHECK(!strcmp(p4_packages_info(i)->id, expected[i]));
        CHECK(p4_package_registry_set_order(mods, "mod.low") == -1);
        p4_package_registry_retire();
        printf("persisted selector order: %s\n", failures ? "FAIL" : "PASS");
        return failures ? 1 : 0;
    }
    if (!strcmp(mode, "bundled")) {
        /* argv[3] is the bundled root: mod.high also there loses to the player's copy. */
        CHECK(p4_package_registry_load_bundled(&mods, &mod_client, argv[2], argv[3], "mod.high,mod.shipped") == 0);
        CHECK(p4_package_registry_count(mods) == 2);
        for (size_t i = 0; i < p4_package_registry_count(mods); ++i) {
            const P4PackageInfo *info = p4_package_registry_info(mods, i);
            const char *root = !strcmp(info->id, "mod.high") ? argv[2] : argv[3];
            CHECK(!strcmp(info->id, "mod.high") || !strcmp(info->id, "mod.shipped"));
            CHECK(!strncmp(info->directory, root, strlen(root)));
        }
        p4_package_registry_retire();
        printf("bundled root: %s\n", failures ? "FAIL" : "PASS");
        return failures ? 1 : 0;
    }
    if (!strcmp(mode, "bundled-only")) {
        /* No player root at all: the shipped root alone still stages. */
        CHECK(p4_package_registry_load_bundled(&mods, &mod_client, "absent-not-scanned", argv[3], "mod.shipped") == 0);
        CHECK(p4_package_registry_count(mods) == 1);
        p4_package_registry_retire();
        printf("bundled root only: %s\n", failures ? "FAIL" : "PASS");
        return failures ? 1 : 0;
    }
    if (!strcmp(mode, "witness-mode")) {
        p4_mod_fidelity_check_start();
        CHECK(p4_mod_witness_enabled() == 1);
        CHECK(p4_packages_count() == 0); /* selector0 never enables anything */
        CHECK(p4_package_registry_load(&mods, &mod_client, argv[2], "mod.high") == 0);
        CHECK(p4_package_registry_load(&packs, &pack_client, argv[3], "pack.low") == 0);
        p4_mod_fidelity_check_start(); /* Valid witness may actually stage BOTH kinds. */
        p4_package_registry_retire();
        printf("explicit mod witness boundary: %s\n", failures ? "FAIL" : "PASS");
        return failures ? 1 : 0;
    }
    CHECK(p4_packages_source_tracking_required() == 0);
    if (!strcmp(mode, "source-intent")) {
        CHECK(p4_package_registry_publish() == 0);
        CHECK(p4_packages_source_tracking_required() == 0);
        /* Mounted-VFS Lua/native row preparation is later than catalog publish. */
        CHECK(p4_packages_source_tracking_require() == 0);
        CHECK(p4_packages_source_tracking_require() == 0);
        CHECK(p4_packages_source_tracking_required() == 1);
        CHECK(p4_packages_source_tracking_seal() == 1);
        CHECK(p4_packages_source_tracking_require() == -1);
        p4_package_registry_retire();
        CHECK(p4_packages_source_tracking_required() == 0);
        CHECK(p4_packages_source_tracking_require() == -1);
        CHECK(p4_packages_memory_usage().resident == 0);
        printf("explicit source intent: %s\n", failures ? "FAIL" : "PASS");
        return failures ? 1 : 0;
    }
    if (!strcmp(mode, "budget-policy")) return memory_policy_witness();
    P4PackageHandler selected_handler = handler;
    if (!strncmp(mode, "admit-", 6)) {
        mod_client.refuse = admission_refusal;
        mod_client.user = &refusals;
        selected_handler.preflight = admission_preflight;
        selected_handler.prepare = admission_prepare;
    }
    CHECK(p4_package_handler_register(&selected_handler) == 0);
    if (!strcmp(mode, "budget-refuse")) {
        CHECK(p4_packages_memory_configure(1) == 0);
        CHECK(p4_package_registry_load(&packs, &pack_client, argv[3], "pack.low") != 0);
        P4PackageMemoryUsage metadata = p4_packages_memory_usage();
        CHECK(!packs && metadata.resident == 0 && metadata.reserved == 0 && metadata.peak == 0);
        printf("metadata staging refusal: %s\n", failures ? "FAIL" : "PASS");
        return failures ? 1 : 0;
    }
    p4_mod_fidelity_check_start();
    if (!strcmp(mode, "settings-kinds")) return settings_kinds(&mod_client, &pack_client, argv[2], argv[3]);
    if (!strcmp(mode, "requirements")) return requirements_witness(&mod_client, &pack_client, argv[2], argv[3]);
    if (!strcmp(mode, "guard-mod") || !strcmp(mode, "guard-pack")) {
        if (!strcmp(mode, "guard-mod"))
            CHECK(p4_package_registry_load(&mods, &mod_client, argv[2], "mod.high") == 0);
        else
            CHECK(p4_package_registry_load(&packs, &pack_client, argv[3], "pack.low") == 0);
        CHECK(p4_packages_count() == 0); /* staged, not in the published catalog */
        p4_mod_fidelity_check_start();
        return 3; /* A surviving guard is a witness failure, not a success. */
    }
    if (!strcmp(mode, "catalog")) {
        CHECK(p4_package_registry_load(&mods, &mod_client, "absent-not-scanned", "") == 0);
        CHECK(p4_package_registry_load(&packs, &pack_client, "absent-not-scanned", "") == 0);
        CHECK(!mods && !packs && !p4_packages_count());
        CHECK(p4_package_registry_publish() == 0);
        CHECK(p4_packages_source_tracking_seal() == 0);
        p4_packages_assert_disabled();
        CHECK(p4_package_installations_scan(&mod_client, argv[2], installation, &installations) == 0);
        CHECK(p4_package_installations_scan(&pack_client, argv[3], installation, &installations) == 0);
        CHECK(installations >= 4 && invalid >= 2);
        CHECK(!mods && !packs && !p4_packages_count() && !prepares && !publishes);
        printf("catalog=%d invalid=%d activation=0\n", installations, invalid);
        return failures ? 1 : 0;
    }
    if (!strncmp(mode, "admit-", 6)) {
        CHECK(p4_package_registry_load(&mods, &mod_client, argv[2], "mod.high") == 0);
        CHECK(p4_package_registry_load(&packs, &pack_client, argv[3], "pack.low") == 0);
        CHECK(p4_package_reads_observe(observe, &observed) == 0);
        CHECK(p4_package_registry_publish() == 0);
        CHECK(refusals == 1 && publishes == 1 && p4_packages_count() == 1);
        CHECK(p4_package_registry_count(mods) == 0 && p4_package_registry_count(packs) == 1);
        CHECK(vfs_mount(NULL, NULL) == 0);
        int fd = vfs_open("DATA/TEST.TXT", 0);
        char bytes[4] = {0};
        CHECK(fd >= 0 && vfs_read(fd, bytes, 3) == 3 && !memcmp(bytes, "low", 3));
        if (fd >= 0) CHECK(vfs_close(fd) == 0);
        if (held) {
            char header[4];
            CHECK(prepares == 2 && rollbacks == 1);
            CHECK(p4_package_file_read_at(held, 0, header, sizeof header) == 0);
            CHECK(!memcmp(refused_info, p4_package_provider_info(pinned), sizeof refused_info));
            P4PackageFile *new_work = NULL;
            CHECK(p4_package_file_open(pinned, "Textures/p4tex-v1/A1.BIN", 64, &new_work) == -1);
            P4PackageFile *delayed = NULL;
            CHECK(p4_package_file_plan_retain(issued_plan) == 0);
            p4_package_file_plan_release(issued_plan);
            CHECK(p4_package_file_plan_open(issued_plan, &delayed) == 0);
            if (delayed) {
                P4PackageFilePlan *denied = NULL;
                CHECK(p4_package_file_plan_acquire(delayed, &denied) == -1 && denied == NULL);
                CHECK(p4_package_file_read_at(delayed, 0, header, sizeof header) == 0);
                CHECK(p4_package_file_close(delayed) == 0);
            }
        }
        p4_package_registry_retire();
        if (held) CHECK(p4_package_file_close(held) == 0);
        if (pinned) p4_package_provider_release(pinned);
        if (old_pack) p4_package_provider_release(old_pack);
        p4_package_file_plan_release(issued_plan);
        CHECK(p4_package_reads_unobserve(observe, &observed) == 0);
        CHECK(p4_packages_memory_usage().resident == 0 && p4_packages_memory_usage().reserved == 0);
        printf("provider admission %s: %s\n", mode, failures ? "FAIL" : "PASS");
        return failures ? 1 : 0;
    }
    if (!strcmp(mode, "cosmetic")) mod_client.mount_files = pack_client.mount_files = 0;
    CHECK(p4_package_registry_load(&mods, &mod_client, argv[2], "mod.high") == 0);
    CHECK(p4_package_registry_load(&packs, &pack_client, argv[3], "pack.low") == 0);
    CHECK(prepares == 0 && publishes == 0 && p4_packages_count() == 0);
    CHECK(p4_package_reads_observe(observe, &observed) == 0);
    CHECK(p4_package_reads_observe(observe, &observed) == -1);
    int status = p4_package_registry_publish();
    CHECK(p4_packages_source_tracking_required() == 0); /* two enabled packages imply no demand */
    if (!strcmp(mode, "guard-published")) p4_mod_fidelity_check_start();
    int expected_failure = !strcmp(mode, "prepare-fail") || !strcmp(mode, "cosmetic-mixed");
    CHECK((status != 0) == expected_failure);
    CHECK(prepares == 1);
    CHECK(publishes == !expected_failure);
    CHECK(rollbacks == expected_failure);
    if (!expected_failure) {
        CHECK(p4_packages_info(0)->kind == P4_PACKAGE_ENGINE_PACK);
        CHECK(p4_packages_info(1)->kind == P4_PACKAGE_MOD);
        CHECK(p4_package_registry_file_conflicts(mods) == (strcmp(mode, "cosmetic") ? 1u : 0u));
        CHECK(p4_package_registry_file_conflicts(packs) == (strcmp(mode, "cosmetic") ? 1u : 0u));
    } else CHECK(!mods && !packs && p4_packages_count() == 0);
    CHECK(held && pinned);
    if (!strncmp(mode, "plan-", 5)) return file_plan_witness();
    unsigned char bytes[12];
    static const unsigned char expected[12] = {'i','m','a','g','e',0,'n','a','t','i','v','e'};
    if (held) {
        CHECK(p4_package_file_read_at(held, 0, bytes, sizeof bytes) == 0);
        CHECK(!memcmp(bytes, expected, sizeof bytes));
        CHECK(p4_package_file_read_at(held, 1, bytes, sizeof bytes) == -1);
    }
    CHECK(vfs_mount(NULL, NULL) == 0);
    int fd = vfs_open("DATA/TEST.TXT", 0);
    if (expected_failure || !strcmp(mode, "cosmetic")) CHECK(fd < 0);
    else {
        CHECK(fd >= 0);
        char value[8] = {0};
        if (fd >= 0) { CHECK(vfs_read(fd, value, 7) == 4); CHECK(!strcmp(value, "high")); CHECK(vfs_close(fd) == 0); }
    }
    /* No further game/VFS opens after the explicit quiescent retirement. The
     * independent file/pin still reads its exact old identity, not catalog lookup. */
    if (!expected_failure) p4_package_registry_retire();
    CHECK(!mods && !packs && !p4_packages_count());
    CHECK(p4_packages_memory_usage().resident > 0); /* old provider/file pins still own metadata */
    CHECK(p4_package_provider_retain(pinned) == -1);
    CHECK(p4_package_provider_info(pinned)->generation == old_generation);
    CHECK(p4_package_provider_info(pinned)->kind == old_kind);
    CHECK(p4_package_file_read_at(held, 0, bytes, sizeof bytes) == 0);
    CHECK(observed == 2);
    CHECK(p4_package_reads_unobserve(observe, NULL) == -1);
    CHECK(p4_package_reads_unobserve(observe, &observed) == 0);
    CHECK(p4_package_file_close(held) == 0);
    p4_package_provider_release(pinned);
    P4PackageMemoryUsage metadata = p4_packages_memory_usage();
    CHECK(metadata.resident == 0 && metadata.reserved == 0 && metadata.peak <= metadata.limit);
    printf("package_registry: %s mode=%s prepare=%d publish=%d rollback=%d observed=%d\n",
        failures ? "FAIL" : "PASS", mode, prepares, publishes, rollbacks, observed);
    return failures ? 1 : 0;
}
