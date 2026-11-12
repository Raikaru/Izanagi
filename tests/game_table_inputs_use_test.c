/* Actual package/input/save-use services. ONLY the nonreturning recovery
 * frontend is intercepted; this is not native UI/VM/battle qualification. */
#include "p4pc/game_table_inputs_use.h"
#include "p4pc/game_table_sources.h"
#include "p4pc/mod_save.h"
#include "p4pc/package_boot.h"
#include "mod_save_internal.h"
#include "mod_save_hash.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static P4GameTableInput *input, *alternate;
static P4GameTableNativeUses *uses;
static P4GameTables *mode_tables;
static P4GameTableSources *mode_sources;
static unsigned registry_uses;
static const char *mode;
static jmp_buf failure_return;
static int expecting_failure;
static int failed_runtime;
static unsigned failures;
static const P4PackageProvider *failed_provider;
static char failed_reason[192];
static _Noreturn void recover(const P4PackageProvider *provider, const char *reason, int runtime)
{
    assert(expecting_failure && reason && *reason);
    failed_provider = provider; failed_runtime = runtime;
    snprintf(failed_reason, sizeof failed_reason, "%s", reason); ++failures;
    longjmp(failure_return, 1);
}
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ recover(provider, reason, 0); }
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ recover(NULL, reason, 1); }
static int registry_use(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(view == mode_tables && context == &uses && uses);
    ++registry_uses;
    return p4_game_table_sources_semantic_use(view, domain, ids, uses);
}
static void prepare_mode(void)
{
    mode_tables = p4_game_tables_create(); assert(mode_tables);
    P4ElementDesc leaf = {.id = 19, .flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY,
        .revision = 2, .key = "native.use:leaf", .display_name = "Mode fixture", .short_name = "Mode"};
    assert(p4_game_tables_element(mode_tables, "native.use", &leaf) == P4_TABLE_OK);
    assert(p4_game_tables_bind_runtime_use(mode_tables, NULL, &uses) == P4_TABLE_INVALID);
    assert(!p4_game_tables_bind_runtime_use(mode_tables, registry_use, &uses));
    assert(!p4_game_tables_bind_runtime_use(mode_tables, registry_use, &uses));
    assert(p4_game_tables_freeze(mode_tables, 1) == P4_TABLE_OK);
    assert(p4_game_tables_bind_runtime_use(mode_tables, registry_use, &uses) == P4_TABLE_FROZEN);
    assert(!p4_game_tables_validate_runtime_sources(mode_tables));
    /* Exact unowned builtin, not missing-origin fallback. No use context yet. */
    assert(!p4_game_tables_runtime_use_row(mode_tables, P4_TABLE_SOURCE_ELEMENT,
        p4_game_tables_find_element(mode_tables, 0)));
    assert(p4_game_tables_runtime_use_row(mode_tables, P4_TABLE_SOURCE_ELEMENT, &leaf) == P4_TABLE_INVALID);
    assert(p4_game_tables_runtime_use_row(mode_tables, P4_TABLE_SOURCE_STRING,
        p4_game_tables_find_element(mode_tables, 19)) == P4_TABLE_INVALID);
    assert(!registry_uses);
    mode_sources = p4_game_table_sources_create(); assert(mode_sources);
    uint32_t source;
    assert(p4_game_table_sources_native(mode_sources, "fixture:native-use", 1, 7, &source) == P4_TABLE_OK);
    assert(p4_game_table_sources_view(mode_sources, P4_TABLE_SOURCE_REGISTRY, mode_tables) == P4_TABLE_OK);
    P4TableSourceCursor cursor = {0}; P4TableSourceEntry entry;
    assert(p4_game_table_sources_next(P4_TABLE_SOURCE_REGISTRY, mode_tables, &cursor, &entry) == P4_TABLE_OK);
    assert(p4_game_table_sources_bind(mode_sources, mode_tables, &entry, source) == P4_TABLE_OK);
    assert(p4_game_table_sources_next(P4_TABLE_SOURCE_REGISTRY, mode_tables, &cursor, &entry) == P4_TABLE_NOT_FOUND);
    assert(p4_game_table_sources_seal(mode_sources) == P4_TABLE_OK &&
        p4_game_table_sources_freeze(mode_sources) == P4_TABLE_OK);
    /* This proves metadata provenance only, never a new-leaf gameplay route. */
}
static int prepare(void *user)
{
    assert(!user && !p4_mod_save_needs_codec());
    const P4PackageProvider *failed = NULL;
    int result = p4_game_table_input_prepare(P4_TABLE_INPUT_SKILL, !strcmp(mode, "oversize") ? 1 : 1048576, &input, &failed);
    if (!strcmp(mode, "absent")) { assert(result == 1 && !input && !failed); return 0; }
    if (!strcmp(mode, "oversize")) { assert(result == -1 && !input && failed); return -1; }
    assert(!result && input && !failed && !p4_mod_save_needs_codec());
    result = p4_game_table_input_seal(input);
    assert(!p4_mod_save_needs_codec()); /* Authorization/seal never records semantic use. */
    if (!strcmp(mode, "bad-bank")) { assert(result == -1); return -1; }
    assert(!result);
    if (!strcmp(mode, "developer")) {
        assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_UNIT, 1048576, &alternate, &failed) && alternate && !failed);
        assert(!p4_game_table_input_seal(alternate));
    }
    prepare_mode();
    assert(!p4_game_table_native_uses_prepare(&input, 1, mode_sources, &uses) && uses);
    assert(p4_mod_save_needs_codec()); /* Decoder consultation, deliberately NOT a modded bit. */
    return 0;
}
static void rollback(void *user)
{
    assert(!user && !p4_game_table_native_uses_destroy(uses)); uses = NULL;
    p4_game_table_input_destroy(alternate); alternate = NULL;
    p4_game_table_sources_destroy(mode_sources); mode_sources = NULL;
    p4_game_tables_destroy(mode_tables); mode_tables = NULL;
    p4_game_table_input_destroy(input); input = NULL;
}
static void expect_failed_use(const void *identity, const P4PackageProvider *provider)
{
    expecting_failure = 1;
    if (!setjmp(failure_return)) {
        (void)p4_game_table_input_semantic_use(identity, input);
        assert(!"unknown/retired source executed instead of entering recovery");
    }
    expecting_failure = 0;
    assert(failed_provider == provider && failed_runtime && *failed_reason);
}
static void word(unsigned char *bytes, uint32_t value)
{ for (unsigned i = 0; i < 4; ++i) bytes[i] = (unsigned char)(value >> (i * 8)); }
static uint32_t read_word(const unsigned char *bytes)
{ return bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24; }
static P4ModSaveCapture *capture(const P4ModSaveImage *image)
{
    P4ModSaveCapture *result = NULL;
    char error[256] = {0};
    assert(!p4_mod_save_capture_image(image, &result, error, sizeof error));
    return result;
}
static unsigned char *copy_sidecar(const P4ModSaveCapture *capture)
{
    unsigned char *bytes = malloc(capture->sidecar_size); assert(bytes);
    memcpy(bytes, capture->sidecar, capture->sidecar_size);
    return bytes;
}
static void developer_history(const P4ModSaveImage *image, P4ModSaveCapture *saved)
{
    assert(saved && !saved->witness);
    assert(read_word(saved->sidecar + 56) == 1); /* Actual optional native block. */
    assert(!read_word(saved->sidecar + 60) && !read_word(saved->sidecar + 64)); /* No invented package. */
    P4ModSaveLoad *load = NULL;
    char error[256] = {0};
    assert(!p4_mod_save_stage_image(image, copy_sidecar(saved), saved->sidecar_size, &load, error, sizeof error) && load);
    assert(!p4_mod_save_load_warning(load));
    p4_mod_save_load_commit(load); load = NULL;
    P4ModSaveCapture *same = capture(image); assert(same);
    assert(!read_word(same->sidecar + 60) && !read_word(same->sidecar + 64));
    p4_mod_save_capture_free(same);

    /* Explicitly synthetic historical content: a different equal-size source
     * image, not a fake current provider or a permissive codec stub. */
    unsigned char *different = copy_sidecar(saved);
    size_t size = p4_game_table_input_size(input);
    unsigned char *old_content = malloc(size); assert(old_content && size);
    memcpy(old_content, p4_game_table_input_bytes(input), size); old_content[size - 1u] ^= 1u;
    size_t payload = P4_MOD_SAVE_HEADER_BYTES + 32u + strlen("p4pc") + strlen("native-table-use");
    uint8_t canonical_digest[32];
    p4_save_hash(p4_game_table_input_bytes(input), size, canonical_digest);
    assert(!memcmp(saved->sidecar + payload + 24u, canonical_digest, sizeof canonical_digest));
    assert(read_word(different + payload) == 1);
    p4_save_hash(old_content, size, different + payload + 24u); free(old_content);
    p4_save_hash(different, saved->sidecar_size - 32u, different + saved->sidecar_size - 32u);
    assert(!p4_mod_save_stage_image(image, different, saved->sidecar_size, &load, error, sizeof error) && load);
    assert(strstr(p4_mod_save_load_warning(load), "Historical developer table source") &&
        strstr(p4_mod_save_load_warning(load), "SKILL.TBL"));
    p4_mod_save_load_abort(load); load = NULL; /* Rejected warning leaves active history unchanged. */

    /* A real prepared DISC input set cannot authenticate the old DEV source;
     * it produces the normal precommit warning, not a package requirement. */
    assert(alternate && p4_game_table_input_native(alternate)->kind == P4_VFS_DISC);
    assert(!p4_game_table_native_uses_destroy(uses)); uses = NULL;
    /* A newly installed provider cannot overwrite an already-loaded opaque
     * block; this simulates a fresh source session through the real reset. */
    assert(!p4_mod_save_reset_session(error, sizeof error));
    assert(!p4_game_table_native_uses_prepare(&alternate, 1, mode_sources, &uses));
    assert(!p4_mod_save_stage_image(image, copy_sidecar(saved), saved->sidecar_size, &load, error, sizeof error) && load);
    assert(strstr(p4_mod_save_load_warning(load), "Historical developer table source"));
    p4_mod_save_load_abort(load);
    assert(!p4_game_table_native_uses_destroy(uses)); uses = NULL;
    assert(!p4_mod_save_reset_session(error, sizeof error));
    assert(!p4_game_table_native_uses_prepare(&input, 1, mode_sources, &uses));
}
static void native_mode_history(const P4ModSaveImage *image, int developer)
{
    const P4ElementDesc *leaf = p4_game_tables_find_element(mode_tables, 19);
    assert(leaf && !registry_uses);
    assert(!p4_game_tables_runtime_use_row(mode_tables, P4_TABLE_SOURCE_ELEMENT, leaf));
    assert(!p4_game_tables_runtime_use_row(mode_tables, P4_TABLE_SOURCE_ELEMENT, leaf));
    assert(registry_uses == 2);
    P4ModSaveCapture *saved = capture(image); assert(saved);
    assert(read_word(saved->sidecar + 56) == 1 && !read_word(saved->sidecar + 60) && !read_word(saved->sidecar + 64));
    size_t payload = P4_MOD_SAVE_HEADER_BYTES + 32u + strlen("p4pc") + strlen("native-table-use");
    assert(read_word(saved->sidecar + payload) == (developer ? 2u : 1u)); /* ONE context/block, versioned variants. */
    char error[256] = {0};
    assert(!p4_mod_save_reset_session(error, sizeof error));
    P4ModSaveCapture *sticky = capture(image); assert(sticky);
    assert(read_word(sticky->sidecar + payload) == (developer ? 2u : 1u)); /* New Game retains actual mode use too. */
    p4_mod_save_capture_free(sticky);
    P4ModSaveLoad *load = NULL;
    assert(!p4_mod_save_stage_image(image, copy_sidecar(saved), saved->sidecar_size, &load, error, sizeof error) && load);
    assert(!p4_mod_save_load_warning(load));
    p4_mod_save_load_commit(load); load = NULL;

    size_t cursor = payload + 4u, mode_record = SIZE_MAX;
    for (unsigned i = 0, count = read_word(saved->sidecar + payload); i < count; ++i) {
        assert(cursor <= saved->sidecar_size - 32u && saved->sidecar_size - 32u - cursor >= 16u);
        uint32_t kind = read_word(saved->sidecar + cursor);
        if (kind == 1) cursor += 52u + read_word(saved->sidecar + cursor + 8u);
        else {
            assert(kind == 2); mode_record = cursor;
            cursor += 16u + read_word(saved->sidecar + cursor + 12u);
        }
    }
    assert(mode_record != SIZE_MAX && cursor == saved->sidecar_size - 32u);
    /* Synthetic prior mode identities, rehashed by the actual envelope codec.
     * Neither can authorize current required refs or fabricate a provider. */
    for (unsigned missing = 0; missing < 2; ++missing) {
        unsigned char *different = copy_sidecar(saved);
        if (missing) different[mode_record + 16u] = 'x';
        else word(different + mode_record + 8u, 8);
        p4_save_hash(different, saved->sidecar_size - 32u, different + saved->sidecar_size - 32u);
        assert(!p4_mod_save_stage_image(image, different, saved->sidecar_size, &load, error, sizeof error) && load);
        assert(strstr(p4_mod_save_load_warning(load), "Historical compiled-native table mode"));
        p4_mod_save_load_abort(load); load = NULL;
    }
    p4_mod_save_capture_free(saved);
}

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    mode = argv[1];
    unsigned char retail[4100] = {0}, original[sizeof retail];
    word(retail + 56, 0x2000); word(retail + 60, 1); word(retail + 65, UINT32_MAX);
    memcpy(original, retail, sizeof retail);
    P4ModSaveImage image = {retail, sizeof retail, "input-use-fixture"};
    static const P4PackageClient client = {.diagnostic_name = "input use fixtures", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD, .compendium = 1, .mount_files = 1};
    static const P4PackageHandler handler = {.name = "input-use-fixture", .prepare = prepare,
        .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    P4PackageRegistry *registry = NULL;
    if (strcmp(mode, "native") && strcmp(mode, "absent"))
        assert(!p4_package_registry_load(&registry, &client, argv[2], "source.low,source.high"));
    int result = p4_package_registry_publish();
    assert(!capture(&image)); /* Installation, CPU prepare, seal AND registered omitted encoder create nothing. */
    if (!strcmp(mode, "bad-bank") || !strcmp(mode, "oversize")) assert(result && !input && !uses);
    else {
        assert(!result);
        if (input) {
            const P4PackageProvider *provider = p4_game_table_input_provider(input);
            const P4GameTableInputSpan *first = p4_game_table_input_span(input, 0), *second = p4_game_table_input_span(input, 1);
            assert(first && second && uses);
            assert(p4_game_table_input_owns_span(input, first) && p4_game_table_input_owns_span(input, second));
            assert(!p4_game_table_input_owns_span(input, (const unsigned char *)first + 1));
            expect_failed_use(p4_game_table_input_bytes(input), NULL);
            assert(!capture(&image) && failures == 1);
            P4GameTableSourceUseFn use = p4_game_table_input_semantic_use;
            assert(!use(first, input) && !use(second, input) && !use(first, input));
            int developer = !strcmp(mode, "developer");
            P4ModSaveCapture *saved = capture(&image);
            assert(!!saved == !!(provider || developer));
            if (developer) developer_history(&image, saved);
            if (provider) assert(!read_word(saved->sidecar + 56) && read_word(saved->sidecar + 64) == 1);
            p4_mod_save_capture_free(saved);
            char error[256] = {0};
            assert(!p4_mod_save_reset_session(error, sizeof error));
            saved = capture(&image);
            assert(!!saved == !!(provider || developer)); /* Actual process use survives New Game. */
            if (developer) assert(read_word(saved->sidecar + 56) == 1 && !read_word(saved->sidecar + 64));
            p4_mod_save_capture_free(saved);
            if (!strcmp(mode, "native") || developer) native_mode_history(&image, developer);
            p4_package_registry_retire(); assert(!registry);
            if (provider) {
                assert(!p4_package_provider_selected(provider)); expect_failed_use(first, NULL);
                assert(failures == 2);
            } else assert(!use(first, input));
        }
    }
    p4_package_registry_retire();
    assert(!p4_game_table_native_uses_destroy(uses)); uses = NULL;
    p4_game_table_sources_destroy(mode_sources); mode_sources = NULL;
    p4_game_tables_destroy(mode_tables); mode_tables = NULL;
    p4_game_table_input_destroy(alternate); alternate = NULL;
    p4_game_table_input_destroy(input); input = NULL;
    char error[256] = {0}; assert(!p4_mod_save_reset_session(error, sizeof error));
    assert(!memcmp(retail, original, sizeof retail));
    assert(!registry && !p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts("game_table_input_use: actual package/DEV/compiled-mode use, no-tag omission, warning round trips, exact span/refusal, retail bytes and sticky New Game passed; recovery frontend intercepted, not native VM qualification");
    return 0;
}
