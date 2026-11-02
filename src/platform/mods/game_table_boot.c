#include "game_table_inputs_internal.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/game_table_xml.h"
#include "p4pc/game_table_inputs_use.h"
#include "p4pc/game_table_manifest.h"
#include "p4pc/game_items_save.h"
#include "p4pc/game_table_admission.h"
#include "p4pc/packages.h"
#include "p4pc/package_memory.h"
#include "p4pc/package_boot.h"
#include "p4pc/platform.h"
#include "p4pc/guest.h"
#include "p4pc/game_elements.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct DefinitionSource {
    P4PackageFilePlan *plan;
    const char *key;
    const P4PackageProvider *provider;
    uint32_t schema, revision;
    char content[P4_CONTENT_DIGEST_HEX + 1];
} DefinitionSource;
static struct TableSourceBoot {
    P4GameTablesDefinitionBuilder builder;
    P4GameTablesBootProduct cpu_product, runtime_product;
    DefinitionSource *definitions;
    P4GameItemsSave *item_save;
    size_t count, capacity;
    int sealed, exposed, native_items, native_skills, native_enemies, native_personas, admission_staged;
    int identity_valid;
    uint8_t identity[32];
} boot;
static P4GameItemActionPrepareFn action_preparer;
static void *action_context;
int p4_game_tables_boot_action_preparer_register(P4GameItemActionPrepareFn prepare, void *context)
{
    if (!prepare) return P4_TABLE_INVALID;
    if (boot.sealed || boot.exposed || boot.builder.sealing) return P4_TABLE_FROZEN;
    if (action_preparer) return action_preparer == prepare && action_context == context ?
        P4_TABLE_OK : P4_TABLE_CONFLICT;
    action_preparer = prepare; action_context = context;
    if (boot.builder.tables) {
        boot.builder.action_prepare = prepare; boot.builder.action_context = context;
    }
    return P4_TABLE_OK;
}
static void *guest_allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user;
    void *host = p4pc_heap_malloc(size);
    if (host) *guest = P4_H2G(host);
    return host;
}
static void guest_release(void *user, uint32_t guest, void *host, uint32_t size)
{
    (void)user; (void)guest; (void)size;
    p4pc_heap_free(host);
}

P4GameTablesDefinitionBuilder *p4_game_tables_boot_builder(void)
{
    if (boot.sealed || boot.exposed) return NULL;
    if (boot.builder.tables) return &boot.builder;
    const P4GameTableAllocator allocator = {NULL, guest_allocate, guest_release};
    if (p4_game_tables_builder_init(&boot.builder, &allocator, 1)) {
        if (p4_game_tables_builder_destroy(&boot.builder)) abort();
        return NULL;
    }
    boot.builder.action_prepare = action_preparer;
    boot.builder.action_context = action_context;
    return &boot.builder;
}
static int destroy(void)
{
    int result = boot.admission_staged ?
        p4_game_table_admission_unstage(boot.builder.inputs, boot.builder.input_count) : P4_TABLE_OK;
    if (result) return result;
    result = p4_game_tables_items_save_unregister(boot.item_save);
    if (result) return result;
    boot.item_save = NULL;
    result = p4_game_tables_builder_destroy(&boot.builder);
    if (result) return result;
    for (size_t i = 0; i < boot.count; ++i) p4_package_file_plan_release(boot.definitions[i].plan);
    free(boot.definitions);
    if (boot.capacity) p4_packages_memory_release(boot.capacity * sizeof *boot.definitions);
    memset(&boot, 0, sizeof boot);
    return P4_TABLE_OK;
}
static int reserve_definition(void)
{
    if (boot.count < boot.capacity) return 0;
    size_t next = boot.capacity ? boot.capacity * 2u : 8u;
    if (next < boot.capacity || next > SIZE_MAX / sizeof *boot.definitions) return -1;
    size_t bytes = next * sizeof *boot.definitions;
    if (p4_packages_memory_reserve(bytes)) return -1;
    DefinitionSource *replacement = malloc(bytes);
    if (!replacement) { p4_packages_memory_cancel(bytes); return -1; }
    if (boot.count) memcpy(replacement, boot.definitions, boot.count * sizeof *replacement);
    p4_packages_memory_commit(bytes);
    free(boot.definitions);
    if (boot.capacity) p4_packages_memory_release(boot.capacity * sizeof *boot.definitions);
    boot.definitions = replacement; boot.capacity = next;
    return 0;
}
static int refuse(const P4PackageProvider *provider, const char *reason)
{
    p4_package_provider_refuse(provider, reason);
    return -1; /* Return immediately even when policy scheduled a fresh candidate. */
}
static int definition(const P4PackageProvider *provider, const P4PackageField *fields, size_t count)
{
    P4GameTableSourceMetadata metadata;
    char error[240];
    int result = p4_game_tables_source_metadata(fields, count, &metadata, error, sizeof error);
    if (result) return refuse(provider, error);
    for (size_t i = 0; i < boot.count; ++i)
        if (!strcmp(boot.definitions[i].key, metadata.key)) return refuse(provider, "Duplicate table-source key across providers");
    P4GameTablesDefinitionBuilder *builder = p4_game_tables_boot_builder();
    if (!builder || reserve_definition()) return refuse(provider, "Table-source candidate metadata allocation failed");
    DefinitionSource *source = boot.definitions + boot.count++;
    *source = (DefinitionSource){.key = metadata.key, .provider = provider,
        .schema = metadata.schema, .revision = metadata.revision};
    uint32_t origin;
    result = p4_game_table_sources_package(builder->sources, provider, metadata.key, metadata.schema, metadata.revision, &origin);
    if (result) return refuse(provider, p4_game_tables_error(result));
    P4PackageFile *input = NULL;
    if (p4_package_file_open(provider, metadata.file, P4_GAME_TABLE_SOURCE_XML_MAX, &input))
        return refuse(provider, "Table source must be an authorized contained indexed regular file within8MiB");
    if (p4_package_file_plan_acquire(input, &source->plan)) {
        p4_package_file_close(input);
        return refuse(provider, "Could not pin the actual immutable table-source file plan");
    }
    P4ContentDigest content;
    p4_content_digest_begin(&content);
    P4GameTableXmlTarget target = {builder, metadata.owner, origin, metadata.revision, &content};
    result = p4_game_tables_source_xml_file(&target, input, error, sizeof error);
    p4_content_digest_end(&content, source->content);
    int closed = p4_package_file_close(input);
    if (result) return refuse(provider, *error ? error : "Table source decode/registration failed");
    if (closed) return refuse(provider, "Table source changed or could not close after CPU validation");
    if (p4_package_provider_claim(provider, P4_PACKAGE_CLAIM_TABLES | P4_PACKAGE_CLAIM_SEMANTIC)) return -1;
    return 0;
}
static void identity_text(P4ContentDigest *digest, const char *text)
{
    p4_content_digest_update(digest, text, strlen(text) + 1);
}
static void identity_word(P4ContentDigest *digest, uint64_t value)
{
    uint8_t bytes[8];
    for (unsigned i = 0; i < 8; ++i) bytes[i] = (uint8_t)(value >> (i * 8u));
    p4_content_digest_update(digest, bytes, sizeof bytes);
}
static void identity_end(P4ContentDigest *digest, uint8_t out[32])
{
    char hex[P4_CONTENT_DIGEST_HEX + 1];
    p4_content_digest_end(digest, hex);
    for (unsigned i = 0; i < 32; ++i) {
        unsigned value = 0;
        for (unsigned j = 0; j < 2; ++j) {
            const char c = hex[i * 2u + j];
            value = value * 16u + (unsigned)(c <= '9' ? c - '0' : c - 'a' + 10);
        }
        out[i] = (uint8_t)value;
    }
}
/* Canonical v1 serialization: every package source in boot order with the
 * SHA-256 of its decoded bytes, every native declaration origin, then every
 * sealed original input with its SHA-256. */
static void seal_identity(void)
{
    P4ContentDigest digest;
    p4_content_digest_begin(&digest);
    identity_text(&digest, "p4pc-table-catalog-v1");
    identity_word(&digest, boot.count);
    for (size_t i = 0; i < boot.count; ++i) {
        const DefinitionSource *source = boot.definitions + i;
        const P4PackageInfo *info = p4_package_provider_info(source->provider);
        identity_text(&digest, source->key);
        identity_text(&digest, info ? info->id : "");
        identity_text(&digest, info ? info->version : "");
        identity_word(&digest, info ? (uint64_t)info->kind : UINT64_MAX);
        identity_word(&digest, source->schema);
        identity_word(&digest, source->revision);
        identity_text(&digest, source->content);
    }
    size_t cursor = 0;
    for (const P4TableSourceOrigin *origin; (origin = p4_game_table_sources_native_mode_next(boot.builder.sources, &cursor));) {
        identity_text(&digest, origin->source_key);
        identity_word(&digest, origin->schema);
        identity_word(&digest, origin->revision);
    }
    identity_word(&digest, boot.builder.input_count);
    for (size_t i = 0; i < boot.builder.input_count; ++i) {
        const P4GameTableInput *input = boot.builder.inputs[i];
        identity_word(&digest, (uint64_t)input->kind);
        identity_word(&digest, input->fingerprint.size);
        identity_text(&digest, input->fingerprint.hex);
    }
    identity_end(&digest, boot.identity);
    boot.identity_valid = 1;
}
int p4_game_table_catalog_identity(uint8_t sha256[32])
{
    if (!sha256) return P4_TABLE_INVALID;
    if (boot.identity_valid && boot.builder.sources && p4_game_table_sources_active() == boot.builder.sources) {
        memcpy(sha256, boot.identity, sizeof boot.identity);
        return 1;
    }
    static const char retail[] = "p4pc-table-catalog-v1:retail";
    P4ContentDigest digest;
    p4_content_digest_begin(&digest);
    p4_content_digest_update(&digest, retail, sizeof retail - 1);
    identity_end(&digest, sha256);
    return 0;
}
static int prepare(void *unused)
{
    (void)unused;
    if (boot.sealed || boot.exposed) return -1;
    for (size_t package = 0; package < p4_packages_count(); ++package) {
        const P4PackageProvider *provider = p4_packages_provider(package);
        size_t count;
        const P4PackageField *fields = p4_package_provider_fields(provider, &count);
        for (size_t first = 0; first < count;) {
            size_t last = first + 1;
            while (last < count && !strcmp(fields[first].section, fields[last].section) &&
                !strcmp(fields[first].argument, fields[last].argument)) ++last;
            if (!strcmp(fields[first].section, "table-source") && definition(provider, fields + first, last - first)) return -1;
            first = last;
        }
    }
    if (!boot.builder.tables) { boot.sealed = 1; return p4_package_boot_provide(NULL); }
    if (!boot.count && !p4_game_elements_requested(boot.builder.tables) && !boot.builder.input_mask &&
        !p4_game_tables_text_count(boot.builder.names) && !p4_game_tables_text_count(boot.builder.descriptions))
        return -1; /* Empty trusted contribution is a programming error. */
    /* The battle loader shares one sequential arena across all eight packed
     * descriptors. A native transaction must authenticate and bound every
     * contribution, not only the families whose semantic rows it consumes. */
    if (p4_game_elements_requested(boot.builder.tables) ||
        (boot.builder.input_mask & ~(1u << P4_TABLE_INPUT_ITEM))) {
        for (unsigned kind = P4_TABLE_INPUT_SKILL; kind < P4_TABLE_INPUT_COUNT; ++kind)
            if (p4_game_tables_builder_request_input(&boot.builder, (P4GameTableInputKind)kind))
                return -1;
    }
    if (p4_packages_source_tracking_require()) return -1;
    const P4PackageProvider *failed_provider = NULL;
    int result = p4_game_tables_builder_seal(&boot.builder, &failed_provider);
    if (result) {
        fprintf(stderr, "p4pc: combined table-source view/input/origin seal refused: %s\n", p4_game_tables_error(result));
        return failed_provider ? refuse(failed_provider, p4_game_tables_error(result)) : -1;
    }
    boot.sealed = 1;
    seal_identity();
    boot.cpu_product = (P4GameTablesBootProduct){
        .tables = boot.builder.tables, .sources = boot.builder.sources,
        .native_uses = boot.builder.native_uses, .skills = boot.builder.skills,
        .items = boot.builder.items, .enemies = boot.builder.enemies, .personas = boot.builder.personas,
        .names = boot.builder.names,
        .descriptions = boot.builder.descriptions, .inputs = boot.builder.inputs,
        .input_count = boot.builder.input_count
    };
    return p4_package_boot_provide(&boot.cpu_product);
}
static int finalize_prepare(void *unused)
{
    (void)unused;
    if (!boot.cpu_product.tables) return P4_TABLE_OK;
    int result = boot.builder.input_count ?
        p4_game_table_admission_stage(boot.builder.inputs, boot.builder.input_count) : P4_TABLE_OK;
    if (!result && boot.builder.input_count) boot.admission_staged = 1;
    if (!result && boot.builder.items) {
        result = p4_game_tables_items_source_stage(boot.builder.items);
        if (!result) {
            boot.native_items = 1;
            result = p4_game_tables_items_boot_stage(boot.builder.items);
        }
    }
    if (!result && boot.builder.skills) {
        result = p4_game_skills_boot_stage(boot.builder.skills);
        if (!result) {
            boot.native_skills = 1;
            result = p4_game_tables_battle_complete_require("skill", "table-source");
        }
    }
    if (!result && boot.builder.enemies) {
        result = p4_game_enemies_boot_stage(boot.builder.enemies);
        if (!result) {
            boot.native_enemies = 1;
            if (p4_game_enemies_count(boot.builder.enemies))
                result = p4_game_tables_battle_complete_require("enemy", "table-source");
        }
    }
    if (!result && boot.builder.personas) {
        result = p4_game_personas_boot_stage(boot.builder.personas);
        if (!result) {
            boot.native_personas = 1;
            if (p4_game_personas_count(boot.builder.personas))
                result = p4_game_tables_battle_complete_require("persona", "table-source");
        }
    }
    if (!result && boot.builder.items)
        result = p4_game_tables_items_save_register(boot.builder.items, boot.builder.sources, &boot.item_save);
    if (result) fprintf(stderr, "p4pc: pre-raw table hook staging refused: %s\n", p4_game_tables_error(result));
    return result ? -1 : 0;
}
static void publish(void *unused) { (void)unused; boot.exposed = 1; }
static void rollback(void *unused)
{
    (void)unused;
    if (boot.exposed) abort(); /* This callback is strictly PRE-exposure. */
    int result = boot.native_skills ? p4_game_skills_boot_unstage(boot.builder.skills) : P4_TABLE_OK;
    if (!result && boot.native_items) result = p4_game_tables_items_boot_unstage(boot.builder.items);
    if (!result && boot.native_enemies) result = p4_game_enemies_boot_unstage(boot.builder.enemies);
    if (!result && boot.native_personas) result = p4_game_personas_boot_unstage(boot.builder.personas);
    if (result) p4_package_boot_runtime_failed("Early table-source rollback could not release its exact unused raw hooks.");
    if (destroy()) p4_package_boot_runtime_failed("Table-source rollback could not unregister its actual native provenance provider.");
}
static int runtime_prepare(void *unused)
{
    (void)unused;
    if (!boot.cpu_product.tables) return p4_package_boot_provide(NULL);
    if (!boot.builder.native_uses) return p4_package_boot_fail(NULL, "Table-source runtime has no actual owning provenance context");
    int result = boot.builder.names ? p4_game_tables_text_materialize(boot.builder.names) : P4_TABLE_OK;
    if (!result && boot.builder.descriptions) result = p4_game_tables_text_materialize(boot.builder.descriptions);
    if (!result) result = p4_game_table_sources_boot_stage(boot.builder.sources);
    if (!result && p4_game_elements_requested(boot.builder.tables))
        result = p4_game_elements_boot_stage(boot.builder.tables, boot.builder.skills, boot.builder.items);
    if (result) return p4_package_boot_fail(NULL, p4_game_tables_error(result));
    boot.runtime_product = boot.cpu_product;
    return p4_package_boot_provide(&boot.runtime_product);
}
int p4_game_tables_boot_session_end(void)
{
    if (boot.builder.sources) {
        int result = p4_game_table_sources_session_end();
        if (result) return result;
    }
    int result = destroy();
    if (!result) { action_preparer = NULL; action_context = NULL; }
    return result;
}
static const char *const sections[] = {"table-source", NULL};
const P4PackageHandler p4_game_tables_sources_boot_provider = {
    .name = P4_GAME_TABLES_BOOT_PROVIDER, .sections = sections,
    .prepare = prepare, .finalize_prepare = finalize_prepare, .publish = publish, .rollback = rollback,
    .boot_phase = P4_PACKAGE_BOOT_TABLES, .runtime_prepare = runtime_prepare
};
