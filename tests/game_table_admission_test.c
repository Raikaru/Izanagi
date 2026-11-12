#include "p4pc/game_table_inputs.h"
#include "p4pc/game_table_admission.h"
#include "p4pc/game_items_native.h"
#include "p4pc/game_tables.h"
#include "p4pc/game_table_use.h"
#include "vfs.h"
#include "iso9660.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>

static const uint32_t blocks[] = {1, 2, 3, 5, 6, 5, 1, 3, 11};
static P4GameTableInput *inputs[P4_TABLE_INPUT_COUNT];
static const char *mode;
static unsigned walks, prepares, publishes, rollbacks;
static jmp_buf expected_refusal;
static int expect_refusal, source_refusal;
/* Fail-stop adapters isolate this CPU fixture from the native recovery UI. */
_Noreturn void p4_package_boot_runtime_failed(const char *reason)
{ if (expect_refusal) { source_refusal = 0; longjmp(expected_refusal, 1); } fprintf(stderr, "Unexpected packed admission failure: %s\n", reason); abort(); }
_Noreturn void p4_package_boot_source_failed(const P4PackageProvider *provider, const char *reason)
{ if (expect_refusal) { source_refusal = provider != NULL; longjmp(expected_refusal, 1); } fprintf(stderr, "Unexpected selected source failure: %s\n", reason); abort(); }
static void word(uint8_t *bytes, uint32_t value)
{ for (unsigned i = 0; i < 4; ++i) bytes[i] = (uint8_t)(value >> (8u * i)); }
static unsigned receipt_calls, unchecked_calls;
static void observe_receipt(const uint8_t *descriptor, const char *path,
    const uint8_t *bytes, uint32_t size, P4GameTableAdmissionReceipt *receipt)
{
    if (!receipt) { ++unchecked_calls; return; }
    ++receipt_calls;
    assert(p4_game_table_admission_consume(receipt, descriptor + 1, path, bytes, size) == P4_TABLE_INVALID);
    assert(p4_game_table_admission_consume(receipt, descriptor, "battle/WRONG.TBL", bytes, size) == P4_TABLE_INVALID);
    assert(p4_game_table_admission_consume(receipt, descriptor, path, bytes + 1, size) == P4_TABLE_INVALID);
    assert(p4_game_table_admission_consume(receipt, descriptor, path, bytes, size - 1) == P4_TABLE_INVALID);
    assert(!p4_game_table_admission_consume(receipt, descriptor, path, bytes, size));
    assert(p4_game_table_admission_consume(receipt, descriptor, path, bytes, size) == P4_TABLE_INVALID);
}
static void admission(void)
{
    static const struct { unsigned count; uint32_t slots[11]; } vectors[] = {
        {0,{0}}, {2,{0x7644a4,0x7644a8}}, {3,{0x7644b4,0x7644b8,0x7644c8}},
        {5,{0x76453c,0x764538,0x764534,0x764530,0x76452c}},
        {6,{0x7644c4,0x7644cc,0x7644d4,0x7644d8,0x7644dc,0x7644e0}},
        {5,{0x764504,0x76450c,0x764510,0x764508,0x764514}},
        {1,{0x7644ac}}, {3,{0x7644b0,0x7644bc,0x7644d0}},
        {11,{0x7644e4,0x7644e8,0x7644ec,0x7644f0,0x7644f4,0x7644f8,
            0x7644fc,0x764500,0x7644c0,0x764518,0x76451c}}
    };
    const P4PackageProvider *failed = (const P4PackageProvider *)1;
    uint8_t descriptor[0xa4] = {0};
    /* No staged tuple leaves the original loader alone. */
    assert(!p4_game_table_admission_check_packed(NULL, NULL, NULL, 0, &failed) && !failed);
    p4_game_table_admission_packed(NULL, NULL, NULL, 0, observe_receipt);
    assert(unchecked_calls == 1 && !receipt_calls);
    p4_game_table_admission_item(NULL, NULL, 0);
    if (!strcmp(mode, "large-file") || !strcmp(mode, "large-footprint")) {
        assert(p4_game_table_admission_stage(inputs, P4_TABLE_INPUT_COUNT) == P4_TABLE_INVALID);
        assert(p4_game_tables_prepare("item", "init/itemtbl.bin", 1536, NULL, 0) == P4_TABLE_PREPARE_REQUIRED);
        return;
    }
    assert(!p4_game_table_admission_stage(inputs, P4_TABLE_INPUT_COUNT));
    assert(!p4_game_table_admission_stage(inputs, P4_TABLE_INPUT_COUNT));
    P4GameTableInput *wrong[] = {inputs[P4_TABLE_INPUT_SKILL]};
    assert(p4_game_table_admission_stage(wrong, 1) == P4_TABLE_CONFLICT);
    assert(p4_game_table_admission_unstage(wrong, 1) == P4_TABLE_CONFLICT);
    const P4GameTableInput *item = inputs[P4_TABLE_INPUT_ITEM];
    size_t item_size = p4_game_table_input_size(item);
    uint8_t *item_copy = malloc(item_size); assert(item_copy);
    memcpy(item_copy, p4_game_table_input_bytes(item), item_size);
    assert(!p4_game_table_admission_check_item("init/itemtbl.bin", item_copy, item_size, &failed) && !failed);
    p4_game_table_admission_item("init/itemtbl.bin", item_copy, (uint32_t)item_size);
    assert(!p4_game_tables_prepare("item", "init/itemtbl.bin", 1536, item_copy, item_size));
    item_copy[item_size - 1] ^= 1;
    assert(p4_game_table_admission_check_item("init/itemtbl.bin", item_copy, item_size, &failed) == P4_TABLE_INVALID && !failed);
    expect_refusal = 1;
    if (!setjmp(expected_refusal)) {
        p4_game_table_admission_item("init/itemtbl.bin", item_copy, (uint32_t)item_size);
        assert(!"changed ITEM input must fail before copying");
    }
    expect_refusal = 0;
    assert(source_refusal == (p4_game_table_input_provider(item) != NULL));
    expect_refusal = 1;
    if (!setjmp(expected_refusal)) {
        p4_game_table_admission_item("init/itemtbl.bin", item_copy, P4_ITEM_RETAIL_FILE_SIZE + 1u);
        assert(!"oversize ITEM input must fail before copying");
    }
    expect_refusal = 0;
    assert(source_refusal == (p4_game_table_input_provider(item) != NULL));
    assert(p4_game_tables_prepare("item", "init/itemtbl.bin", 1536, item_copy, item_size) == P4_TABLE_INVALID);
    assert(p4_game_table_admission_check_item("wrong", item_copy, item_size, &failed) == P4_TABLE_INVALID);
    free(item_copy);
    for (unsigned kind = 1; kind < P4_TABLE_INPUT_COUNT; ++kind) {
        const P4GameTableInput *input = inputs[kind];
        size_t size = p4_game_table_input_size(input);
        uint8_t *copy = malloc(size); assert(copy);
        memcpy(copy, p4_game_table_input_bytes(input), size);
        memset(descriptor, 0, sizeof descriptor);
        for (unsigned i = 0; i < vectors[kind].count; ++i) word(descriptor + 4 + i * 8, vectors[kind].slots[i]);
        const char *label = p4_game_table_input_label((P4GameTableInputKind)kind);
        assert(!p4_game_table_admission_check_packed(descriptor, label, copy, size, &failed) && !failed);
        p4_game_table_admission_packed(descriptor, label, copy, (uint32_t)size, observe_receipt);
        assert(receipt_calls == kind && unchecked_calls == 1);
        if (kind == P4_TABLE_INPUT_SKILL || kind == P4_TABLE_INPUT_MSG || kind == P4_TABLE_INPUT_EFFECT)
            assert(!p4_game_tables_prepare("skill", label, 1, copy, size));
        assert(p4_game_table_admission_check_packed(descriptor, label, copy, size - 1, &failed) == P4_TABLE_INVALID);
        assert(failed == p4_game_table_input_provider(input));
        copy[size - 1] ^= 1;
        assert(p4_game_table_admission_check_packed(descriptor, label, copy, size, &failed) == P4_TABLE_INVALID);
        assert(failed == p4_game_table_input_provider(input));
        copy[size - 1] ^= 1;
        word(descriptor + 4, vectors[kind].slots[0] + 4);
        assert(p4_game_table_admission_check_packed(descriptor, label, copy, size, &failed) == P4_TABLE_INVALID);
        word(descriptor + 4, vectors[kind].slots[0]);
        word(descriptor + 4 + vectors[kind].count * 8, 0x765ff0);
        assert(p4_game_table_admission_check_packed(descriptor, label, copy, size, &failed) == P4_TABLE_INVALID);
        word(descriptor + 4 + vectors[kind].count * 8, 0);
        assert(p4_game_table_admission_check_packed(descriptor, "battle/WRONG.TBL", copy, size, &failed) == P4_TABLE_INVALID);
        assert(failed == p4_game_table_input_provider(input));
        free(copy);
    }
    memset(descriptor, 0, sizeof descriptor);
    word(descriptor + 4, 0x765ff0);
    uint8_t other[] = {'x'};
    assert(!p4_game_table_admission_check_packed(descriptor, "battle/OTHER.TBL", other, 1, &failed));
    p4_game_table_admission_packed(descriptor, "battle/OTHER.TBL", other, 1, observe_receipt);
    assert(unchecked_calls == 2);
    assert(!p4_game_table_admission_unstage(inputs, P4_TABLE_INPUT_COUNT));
    assert(!p4_game_table_admission_unstage(inputs, P4_TABLE_INPUT_COUNT));
    assert(p4_game_tables_prepare("item", "init/itemtbl.bin", 1536, NULL, 0) == P4_TABLE_PREPARE_REQUIRED);
    P4GameTableInput *item_only[] = {inputs[P4_TABLE_INPUT_ITEM]};
    assert(!p4_game_table_admission_stage(item_only, 1));
    assert(!p4_game_table_admission_check_packed(NULL, NULL, NULL, 0, &failed) && !failed);
    assert(!p4_game_table_admission_unstage(item_only, 1));
    P4GameTableInput *skill_only[] = {inputs[P4_TABLE_INPUT_SKILL]};
    assert(!p4_game_table_admission_stage(skill_only, 1));
    memset(descriptor, 0, sizeof descriptor); word(descriptor + 4, 0x7644b4);
    assert(p4_game_table_admission_check_packed(descriptor, "battle/UNIT.TBL", (const uint8_t *)"x", 1, &failed) == P4_TABLE_NOT_READY);
    assert(p4_game_table_admission_check_packed(descriptor, "battle/OTHER.TBL", (const uint8_t *)"x", 1, &failed) == P4_TABLE_INVALID);
    assert(!p4_game_table_admission_unstage(skill_only, 1));
}
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
    if (!strcmp(mode, "no-disc-package")) {
        const P4PackageProvider *failed = NULL;
        assert(!p4_game_table_input_prepare(P4_TABLE_INPUT_SKILL, 1048576,
                                            &inputs[P4_TABLE_INPUT_SKILL], &failed));
        assert(!failed && !p4_game_table_input_seal(inputs[P4_TABLE_INPUT_SKILL]));
        return 0;
    }
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
            if (kind == P4_TABLE_INPUT_SKILL &&
                (!strcmp(mode, "large-file") || !strcmp(mode, "large-footprint"))) {
                assert(p4_game_table_input_span_size(span) == 28);
                continue;
            }
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
        if (!strcmp(mode, "no-disc-package")) {
            P4GameTableInput *skill_only[] = {inputs[P4_TABLE_INPUT_SKILL]};
            assert(p4_game_table_admission_stage(skill_only, 1) == P4_TABLE_INVALID);
        } else if (strcmp(mode, "absent")) {
            assert(!vfs_source_mount_selected() && walks == 7);
            for (unsigned kind = 0; kind < P4_TABLE_INPUT_COUNT; ++kind) {
                int fd = vfs_open(p4_game_table_input_label((P4GameTableInputKind)kind), 0); assert(fd >= 0);
                size_t size = p4_game_table_input_size(inputs[kind]);
                uint8_t *bytes = malloc(size); assert(bytes);
                assert(vfs_read(fd, bytes, (int)size) == (int)size && !vfs_close(fd));
                assert(!memcmp(bytes, p4_game_table_input_bytes(inputs[kind]), size)); free(bytes);
            }
            admission();
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
    puts("game_table_admission: selected buffers, retail descriptor vectors, wrong paths and one-use receipts passed; no native image or gameplay claim");
    return 0;
}
