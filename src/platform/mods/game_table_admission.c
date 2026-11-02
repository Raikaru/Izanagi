#include "game_table_inputs_internal.h"
#include "p4pc/game_table_admission.h"
#include "p4pc/game_items_native.h"
#include "p4pc/game_tables.h"
#include "p4pc/package_boot.h"
#include "p4pc/packages.h"
#include <string.h>

/* Actual SLUS_217.82 005f66c0 descriptors; these are pointer SLOT addresses,
 * not the dynamic payload addresses stored in them. Source files are the
 * exact labels in that descriptor array, not a guessed basename/owner. */
typedef struct Descriptor {
    P4GameTableInputKind kind;
    unsigned count;
    uint32_t slots[11];
} Descriptor;
static const Descriptor descriptors[] = {
    {P4_TABLE_INPUT_SKILL, 2, {0x7644a4,0x7644a8}},
    {P4_TABLE_INPUT_UNIT, 3, {0x7644b4,0x7644b8,0x7644c8}},
    {P4_TABLE_INPUT_MSG, 5, {0x76453c,0x764538,0x764534,0x764530,0x76452c}},
    {P4_TABLE_INPUT_PERSONA, 6, {0x7644c4,0x7644cc,0x7644d4,0x7644d8,0x7644dc,0x7644e0}},
    {P4_TABLE_INPUT_ENCOUNT, 5, {0x764504,0x76450c,0x764510,0x764508,0x764514}},
    {P4_TABLE_INPUT_EFFECT, 1, {0x7644ac}},
    {P4_TABLE_INPUT_MODEL, 3, {0x7644b0,0x7644bc,0x7644d0}},
    {P4_TABLE_INPUT_AICALC, 11, {0x7644e4,0x7644e8,0x7644ec,0x7644f0,0x7644f4,
        0x7644f8,0x7644fc,0x764500,0x7644c0,0x764518,0x76451c}}
};
static P4GameTableInput *const *staged_inputs;
static size_t staged_count;
static int staged_packed;
static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static const P4GameTableInput *input_for(P4GameTableInputKind kind)
{
    for (size_t i = 0; i < staged_count; ++i)
        if (p4_game_table_input_kind(staged_inputs[i]) == kind) return staged_inputs[i];
    return NULL;
}
static int verify(const P4GameTableInput *input, const void *bytes, size_t size,
    const P4PackageProvider **failed)
{
    const P4PackageProvider *provider = p4_game_table_input_provider(input);
    if (failed) *failed = provider;
    if (provider && !p4_package_provider_selected(provider)) return P4_TABLE_NOT_READY;
    if (!p4_game_table_input_matches(input, bytes, size)) return P4_TABLE_INVALID;
    if (failed) *failed = NULL;
    return P4_TABLE_OK;
}
int p4_game_table_admission_check_item(const char *path, const uint8_t *bytes,
    size_t size, const P4PackageProvider **failed)
{
    if (failed) *failed = NULL;
    const P4GameTableInput *input = input_for(P4_TABLE_INPUT_ITEM);
    if (!input) return P4_TABLE_NOT_FOUND;
    if (!path || strcmp(path, p4_game_table_input_label(P4_TABLE_INPUT_ITEM))) {
        if (failed) *failed = p4_game_table_input_provider(input);
        return P4_TABLE_INVALID;
    }
    return verify(input, bytes, size, failed);
}
void p4_game_table_admission_item(const char *path, const uint8_t *bytes, uint32_t size)
{
    const P4GameTableInput *item = input_for(P4_TABLE_INPUT_ITEM);
    if (!item) return;
    const P4PackageProvider *failed = p4_game_table_input_provider(item);
    int result = size > P4_ITEM_RETAIL_FILE_SIZE ? P4_TABLE_INVALID :
        p4_game_table_admission_check_item(path, bytes, size, &failed);
    if (result) {
        if (failed) p4_package_boot_source_failed(failed, "Selected item input differs before copy");
        p4_package_boot_runtime_failed("Native item input differs before copy");
    }
}
static int prepare(void *user, const char *table, const char *path, uint32_t rows,
    const uint8_t *bytes, size_t size)
{
    (void)user; (void)rows;
    if (!table) return P4_TABLE_INVALID;
    const P4PackageProvider *failed = NULL;
    if (!strcmp(table, "item")) {
        int result = p4_game_table_admission_check_item(path, bytes, size, &failed);
        if (result && failed) p4_package_boot_source_failed(failed, "Selected item input differs from the loaded full buffer");
        return result;
    }
    if (!path) return P4_TABLE_INVALID;
    const P4GameTableInput *input = NULL;
    const int skill = !strcmp(table, "skill"), enemy = !strcmp(table, "enemy"), persona = !strcmp(table, "persona");
    for (size_t i = 0; (skill || enemy || persona) && i < staged_count; ++i) {
        P4GameTableInputKind kind = p4_game_table_input_kind(staged_inputs[i]);
        const int owned = skill ? kind == P4_TABLE_INPUT_SKILL || kind == P4_TABLE_INPUT_EFFECT ||
            kind == P4_TABLE_INPUT_MSG : enemy ? kind == P4_TABLE_INPUT_UNIT || kind == P4_TABLE_INPUT_MODEL ||
            kind == P4_TABLE_INPUT_AICALC || kind == P4_TABLE_INPUT_MSG : kind == P4_TABLE_INPUT_PERSONA ||
            kind == P4_TABLE_INPUT_MODEL || kind == P4_TABLE_INPUT_MSG;
        if (owned && !strcmp(path, p4_game_table_input_label(kind))) { input = staged_inputs[i]; break; }
    }
    if (!input) return P4_TABLE_NOT_FOUND;
    failed = NULL;
    int result = verify(input, bytes, size, &failed);
    if (result && failed) p4_package_boot_source_failed(failed, "Selected table input differs from the loaded full buffer");
    return result;
}
int p4_game_table_admission_stage(P4GameTableInput *const *inputs, size_t count)
{
    if (!inputs || !count || count > P4_TABLE_INPUT_COUNT) return P4_TABLE_INVALID;
    if (staged_inputs) return staged_inputs == inputs && staged_count == count ? P4_TABLE_OK : P4_TABLE_CONFLICT;
    unsigned seen = 0;
    for (size_t i = 0; i < count; ++i) {
        P4GameTableInputKind kind = p4_game_table_input_kind(inputs[i]);
        if ((unsigned)kind >= P4_TABLE_INPUT_COUNT || !p4_game_table_input_span_count(inputs[i]) ||
            (seen & (1u << (unsigned)kind))) return P4_TABLE_INVALID;
        if (kind == P4_TABLE_INPUT_ITEM) {
            if (inputs[i]->size > P4_ITEM_RETAIL_FILE_SIZE) return P4_TABLE_INVALID;
        } else {
            /* Bound both physical file and aligned arena contribution against
             * that same key's actual disc plan, not a guessed arena end. */
            if (!inputs[i]->retail_bounds_valid || inputs[i]->size > inputs[i]->retail_size)
                return P4_TABLE_INVALID;
            uint64_t footprint = 0;
            for (size_t block = 0; block < p4_game_table_input_span_count(inputs[i]); ++block) {
                size_t size = p4_game_table_input_span_size(p4_game_table_input_span(inputs[i], block));
                footprint += ((uint64_t)size + 15u) & ~UINT64_C(15);
            }
            if (footprint > inputs[i]->retail_footprint) return P4_TABLE_INVALID;
        }
        seen |= 1u << (unsigned)kind;
    }
    int result = p4_game_tables_prepare_register(prepare, NULL);
    if (result) return result;
    staged_inputs = inputs; staged_count = count;
    staged_packed = (seen & ~(1u << P4_TABLE_INPUT_ITEM)) != 0;
    return P4_TABLE_OK;
}
int p4_game_table_admission_unstage(P4GameTableInput *const *inputs, size_t count)
{
    if (!staged_inputs) return P4_TABLE_OK;
    if (staged_inputs != inputs || staged_count != count) return P4_TABLE_CONFLICT;
    int result = p4_game_tables_prepare_unregister(prepare, NULL);
    if (result) return result;
    staged_inputs = NULL; staged_count = 0; staged_packed = 0;
    return P4_TABLE_OK;
}
static int check_packed(const uint8_t *descriptor, const char *path,
    const uint8_t *bytes, size_t size, const P4PackageProvider **failed,
    const P4GameTableInput **accepted)
{
    if (failed) *failed = NULL;
    if (accepted) *accepted = NULL;
    if (!staged_inputs || !staged_packed) return P4_TABLE_OK; /* Original loader unchanged. */
    if (!descriptor || !path || !bytes) return P4_TABLE_INVALID;
    const Descriptor *chosen = NULL;
    const P4GameTableInput *input = NULL;
    for (size_t i = 0; i < sizeof descriptors / sizeof descriptors[0]; ++i) {
        const Descriptor *candidate = &descriptors[i];
        if (!strcmp(path, p4_game_table_input_label(candidate->kind))) {
            const P4GameTableInput *owned = input_for(candidate->kind);
            if (!owned) return P4_TABLE_NOT_READY;
            chosen = candidate; input = owned; break;
        }
    }
    if (!chosen) {
        /* A forged/mismatched path must not bypass checks for an owned slot. */
        for (unsigned block = 0; block < 20; ++block) {
            uint32_t destination = word(descriptor + 8u * block + 4u);
            if (!destination) return P4_TABLE_OK;
            for (size_t i = 0; i < sizeof descriptors / sizeof descriptors[0]; ++i) {
                const Descriptor *candidate = &descriptors[i];
                const P4GameTableInput *owned = input_for(candidate->kind);
                /* Even an unstaged known slot must not be laundered through
                 * an unrelated path when a packed admission tuple is active. */
                for (unsigned j = 0; j < candidate->count; ++j)
                    if (destination == candidate->slots[j]) {
                        if (failed) *failed = p4_game_table_input_provider(owned);
                        return P4_TABLE_INVALID;
                    }
            }
        }
        return P4_TABLE_INVALID;
    }
    if (failed) *failed = p4_game_table_input_provider(input);
    if (p4_game_table_input_span_count(input) != chosen->count) return P4_TABLE_INVALID;
    for (unsigned i = 0; i < chosen->count; ++i)
        if (word(descriptor + 8u * i + 4u) != chosen->slots[i]) return P4_TABLE_INVALID;
    if (word(descriptor + 8u * chosen->count + 4u)) return P4_TABLE_INVALID;
    int result = verify(input, bytes, size, failed);
    if (!result && accepted) *accepted = input;
    return result;
}
int p4_game_table_admission_check_packed(const uint8_t *descriptor, const char *path,
    const uint8_t *bytes, size_t size, const P4PackageProvider **failed)
{
    return check_packed(descriptor, path, bytes, size, failed, NULL);
}
struct P4GameTableAdmissionReceipt {
    const struct P4GameTableAdmissionReceipt *self;
    const P4GameTableInput *input;
    const uint8_t *descriptor, *bytes;
    const char *path;
    uint32_t size;
    int consumed;
};
int p4_game_table_admission_consume(P4GameTableAdmissionReceipt *receipt,
    const uint8_t *descriptor, const char *path, const uint8_t *bytes, uint32_t size)
{
    if (!receipt || receipt->self != receipt || receipt->consumed ||
        receipt->descriptor != descriptor || receipt->path != path ||
        receipt->bytes != bytes || receipt->size != size ||
        input_for(p4_game_table_input_kind(receipt->input)) != receipt->input)
        return P4_TABLE_INVALID;
    receipt->consumed = 1;
    return P4_TABLE_OK;
}
void p4_game_table_admission_packed(const uint8_t *descriptor, const char *path,
    const uint8_t *bytes, uint32_t size, P4GameTablePackedObserver observer)
{
    const P4PackageProvider *failed = NULL;
    const P4GameTableInput *accepted = NULL;
    if (!observer) p4_package_boot_runtime_failed("Missing packed table observer");
    int result = check_packed(descriptor, path, bytes, size, &failed, &accepted);
    if (result) {
        if (failed) p4_package_boot_source_failed(failed, "Selected packed table input or descriptor differs before copy");
        p4_package_boot_runtime_failed("Native packed table input or descriptor differs before copy");
    }
    P4GameTableAdmissionReceipt receipt = {&receipt, accepted, descriptor, bytes, path, size, 0};
    observer(descriptor, path, bytes, size, accepted ? &receipt : NULL);
    receipt.self = NULL;
}
