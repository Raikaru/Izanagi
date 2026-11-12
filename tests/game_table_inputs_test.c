#include "p4pc/game_table_inputs.h"
#include "vfs.h"
#include "iso9660.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const uint32_t blocks[] = {1, 2, 3, 5, 6, 5, 1, 3, 11};
static P4GameTableInput *inputs[P4_TABLE_INPUT_COUNT];
static const char *mode;
static unsigned walks, prepares, publishes, rollbacks;
int p4_test_actual_iso_walk(FILE *, long long, iso_visit_fn, void *);
int iso_walk(FILE *stream, long long base, iso_visit_fn visit, void *user)
{ ++walks; return p4_test_actual_iso_walk(stream, base, visit, user); }
static void release_inputs(void)
{
    for (unsigned i = 0; i < P4_TABLE_INPUT_COUNT; ++i) { p4_game_table_input_destroy(inputs[i]); inputs[i] = NULL; }
}
static int prepare(void *user)
{
    assert(!user); ++prepares;
    for (unsigned kind = 0; kind < P4_TABLE_INPUT_COUNT; ++kind) {
        const P4PackageProvider *failed = (const P4PackageProvider *)1;
        uint64_t bound = !strcmp(mode, "oversize") && kind == P4_TABLE_INPUT_SKILL ? 1 : 1048576;
        int result = p4_game_table_input_prepare((P4GameTableInputKind)kind, bound, &inputs[kind], &failed);
        assert(vfs_open(p4_game_table_input_label((P4GameTableInputKind)kind), 0) == -1);
        if (!strcmp(mode, "absent")) { assert(result == 1 && !inputs[kind] && !failed); continue; }
        if (!strcmp(mode, "oversize") && kind == P4_TABLE_INPUT_SKILL) {
            assert(result == -1 && !inputs[kind] && failed);
            assert(!strcmp(p4_package_provider_info(failed)->id, "source.high"));
            return -1;
        }
        assert(!result && inputs[kind] && !failed && !p4_game_table_input_span_count(inputs[kind]));
        result = p4_game_table_input_seal(inputs[kind]);
        if (!strcmp(mode, "bad-bank") && kind == P4_TABLE_INPUT_SKILL) {
            assert(result == -1 && !p4_game_table_input_span_count(inputs[kind]));
            assert(!strcmp(p4_package_provider_info(p4_game_table_input_provider(inputs[kind]))->id, "source.high"));
            return -1;
        }
        assert(!result && !p4_game_table_input_seal(inputs[kind]));
        assert(p4_game_table_input_kind(inputs[kind]) == (P4GameTableInputKind)kind);
        assert(p4_game_table_input_span_count(inputs[kind]) == blocks[kind]);
        assert(!p4_game_table_input_span(inputs[kind], blocks[kind]));
        const P4PackageProvider *provider = p4_game_table_input_provider(inputs[kind]);
        const P4VfsSourceInfo *native = p4_game_table_input_native(inputs[kind]);
        const char *tag = "disc";
        if (kind == P4_TABLE_INPUT_SKILL && strcmp(mode, "native")) {
            if (!strcmp(mode, "developer")) { assert(!provider && native && native->kind == P4_VFS_DEVELOPER); tag = "developer"; }
            else {
                assert(provider && !native && p4_package_provider_selected(provider));
                assert(!strcmp(p4_package_provider_info(provider)->id, "source.high"));
                tag = !strcmp(mode, "unqualified") ? "unqualified" : "high";
            }
        } else assert(!provider && native && native->kind == P4_VFS_DISC && native->file.size);
        for (unsigned block = 0; block < blocks[kind]; ++block) {
            const P4GameTableInputSpan *span = p4_game_table_input_span(inputs[kind], block);
            assert(span && span == p4_game_table_input_span(inputs[kind], block));
            assert(p4_game_table_input_span_owner(span) == inputs[kind] && p4_game_table_input_span_index(span) == block);
            assert(p4_game_table_input_owns_span(inputs[kind], span));
            assert(!p4_game_table_input_owns_span(inputs[kind], (const uint8_t *)span + 1));
            if (kind) assert(!p4_game_table_input_owns_span(inputs[0], span));
            char expected[64]; int length = snprintf(expected, sizeof expected, "%s-%u-%u", tag, kind, block);
            assert(length > 0 && (size_t)length < sizeof expected);
            const uint8_t *bytes = p4_game_table_input_span_bytes(span);
            if (kind == P4_TABLE_INPUT_ITEM) {
                assert(!p4_game_table_input_span_offset(span) && p4_game_table_input_span_size(span) == (size_t)length + 16u);
                bytes += 16;
            } else assert(p4_game_table_input_span_size(span) == (size_t)length && p4_game_table_input_span_offset(span) % 16u == 4u);
            assert(!memcmp(bytes, expected, (size_t)length));
        }
    }
    if (strcmp(mode, "absent")) {
        assert(walks == 7); /* One real root/five-CVM/root index, shared by every input. */
        assert(p4_game_table_input_span(inputs[P4_TABLE_INPUT_AICALC], 9) != p4_game_table_input_span(inputs[P4_TABLE_INPUT_AICALC], 10));
    } else assert(!walks);
    return 0;
}
static void publish(void *user) { assert(!user); ++publishes; }
static void rollback(void *user) { assert(!user); ++rollbacks; release_inputs(); }
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    mode = argv[1];
    assert(!p4_game_table_input_label((P4GameTableInputKind)-1));
    assert(p4_game_table_input_kind(NULL) == P4_TABLE_INPUT_COUNT);
    assert(!p4_game_table_input_span_owner(NULL) && p4_game_table_input_span_index(NULL) == SIZE_MAX);
    static const P4PackageClient client = {.diagnostic_name = "input fixtures", .manifest_name = "mod.ini",
        .section = "[mod]", .kind = P4_PACKAGE_MOD, .compendium = 1, .mount_files = 1};
    static const P4PackageHandler handler = {.name = "input-fixture", .prepare = prepare, .publish = publish,
        .rollback = rollback, .boot_phase = P4_PACKAGE_BOOT_TABLES};
    assert(!p4_package_handler_register(&handler));
    P4PackageRegistry *registry = NULL;
    if (strcmp(mode, "native") && strcmp(mode, "absent"))
        assert(!p4_package_registry_load(&registry, &client, argv[2], "source.low,source.high"));
    int result = p4_package_registry_publish();
    if (!strcmp(mode, "oversize") || !strcmp(mode, "bad-bank")) {
        assert(result && prepares == 1 && !publishes && rollbacks == 1);
    } else {
        assert(!result && prepares == 1 && publishes == 1 && !rollbacks);
        if (strcmp(mode, "absent")) {
            assert(!vfs_source_mount_selected() && walks == 7);
            for (unsigned kind = 0; kind < P4_TABLE_INPUT_COUNT; ++kind) {
                int fd = vfs_open(p4_game_table_input_label((P4GameTableInputKind)kind), 0); assert(fd >= 0);
                size_t size = p4_game_table_input_size(inputs[kind]);
                uint8_t *bytes = malloc(size); assert(bytes);
                assert(vfs_read(fd, bytes, (int)size) == (int)size && !vfs_close(fd));
                assert(!memcmp(bytes, p4_game_table_input_bytes(inputs[kind]), size));
                assert(p4_game_table_input_matches(inputs[kind], bytes, size));
                assert(!p4_game_table_input_matches(inputs[kind], bytes, size - 1));
                bytes[size - 1] ^= 1;
                assert(!p4_game_table_input_matches(inputs[kind], bytes, size));
                free(bytes);
            }
        }
    }
    p4_package_registry_retire();
    assert(!registry);
    for (unsigned kind = 0; kind < P4_TABLE_INPUT_COUNT; ++kind) {
        if (!inputs[kind]) continue;
        assert(p4_game_table_input_bytes(inputs[kind]) && p4_game_table_input_size(inputs[kind]));
        const P4PackageProvider *provider = p4_game_table_input_provider(inputs[kind]);
        if (provider) assert(!p4_package_provider_selected(provider) && !strcmp(p4_package_provider_info(provider)->version, "2.0.0"));
    }
    release_inputs();
    assert(!p4_packages_memory_usage().resident && !p4_packages_memory_usage().reserved);
    puts("game_table_inputs: actual selected package/native plans, wrapper precedence, exact packed spans, one index and balanced immutable CPU lifetime passed; not native binding qualification");
    return 0;
}
