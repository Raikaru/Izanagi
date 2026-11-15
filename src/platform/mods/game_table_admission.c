#include "game_table_derived_internal.h"
#include "game_table_inputs_internal.h"
#include "p4pc/game_table_admission.h"
#include "p4pc/game_items_native.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/game_tables.h"
#include "p4pc/package_boot.h"
#include "p4pc/packages.h"
#include <stdio.h>
#include <stdlib.h>
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
static P4GameTableAllocator golden_allocator;
static struct GoldenArena { void *host; uint32_t guest, size; } golden_arenas[P4_TABLE_INPUT_COUNT];
static int staged_golden;
_Noreturn void p4_game_table_admission_golden_fail(const char *reason, int result)
{
    fprintf(stderr, "p4pc: Golden %s (%d)\n", reason, result);
    abort();
}
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
    /* A loader buffer already rewritten to the derived composition (the item
     * path verifies its full buffer twice) is the same authenticated input. */
    if (!p4_game_table_input_matches(input, bytes, size) && !p4_game_tables_derived_matches(input, bytes, size))
        return P4_TABLE_INVALID;
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
void p4_game_table_admission_item(const char *path, uint8_t *bytes, uint32_t size)
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
    p4_game_tables_derived_rewrite(item, bytes, size);
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
            kind == P4_TABLE_INPUT_MODEL || kind == P4_TABLE_INPUT_MSG || kind == P4_TABLE_INPUT_UNIT;
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
    if (!inputs[0]) return P4_TABLE_INVALID;
    int golden = !!inputs[0]->golden_origin[0];
    for (size_t i = 0; i < count; ++i) {
        P4GameTableInputKind kind = p4_game_table_input_kind(inputs[i]);
        if (!inputs[i] || !!inputs[i]->golden_origin[0] != golden) return P4_TABLE_INVALID;
        if ((unsigned)kind >= P4_TABLE_INPUT_COUNT || !p4_game_table_input_span_count(inputs[i]) ||
            (seen & (1u << (unsigned)kind))) return P4_TABLE_INVALID;
        if (kind == P4_TABLE_INPUT_ITEM) {
            if (inputs[i]->size > P4_ITEM_RETAIL_FILE_SIZE) return P4_TABLE_INVALID;
        } else if (!golden) {
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
    staged_golden = golden;
    staged_packed = (seen & ~(1u << P4_TABLE_INPUT_ITEM)) != 0;
    return P4_TABLE_OK;
}
int p4_game_table_admission_golden_allocator(const P4GameTableAllocator *allocator)
{
    if (!staged_inputs || !staged_golden || !allocator || !allocator->allocate || !allocator->release ||
        golden_allocator.allocate) return P4_TABLE_INVALID;
    golden_allocator = *allocator;
    return P4_TABLE_OK;
}
int p4_game_table_admission_unstage(P4GameTableInput *const *inputs, size_t count)
{
    if (!staged_inputs) return P4_TABLE_OK;
    if (staged_inputs != inputs || staged_count != count) return P4_TABLE_CONFLICT;
    int result = p4_game_tables_prepare_unregister(prepare, NULL);
    if (result) return result;
    for (size_t i = 0; i < P4_TABLE_INPUT_COUNT; ++i) {
        struct GoldenArena *arena = &golden_arenas[i];
        if (arena->host) golden_allocator.release(golden_allocator.user,arena->guest,arena->host,arena->size);
        memset(arena,0,sizeof *arena);
    }
    memset(&golden_allocator,0,sizeof golden_allocator);
    staged_golden = 0;
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
    uint8_t *bytes, uint32_t size, P4GameTablePackedObserver observer)
{
    const P4PackageProvider *failed = NULL;
    const P4GameTableInput *accepted = NULL;
    if (!observer) p4_package_boot_runtime_failed("Missing packed table observer");
    int result = check_packed(descriptor, path, bytes, size, &failed, &accepted);
    if (result) {
        if (failed) p4_package_boot_source_failed(failed, "Selected packed table input or descriptor differs before copy");
        p4_package_boot_runtime_failed("Native packed table input or descriptor differs before copy");
    }
    if (accepted) p4_game_tables_derived_rewrite(accepted, bytes, size);
    P4GameTableAdmissionReceipt receipt = {&receipt, accepted, descriptor, bytes, path, size, 0};
    observer(descriptor, path, bytes, size, accepted ? &receipt : NULL);
    receipt.self = NULL;
}
int p4_game_table_admission_golden_active(void) { return staged_golden; }

int p4_game_table_admission_golden_copy(const char *path, uint8_t **bytes, uint32_t *size)
{
    if (!bytes || !size) return -1;
    *bytes = NULL; *size = 0;
    if (!staged_golden) return 0;
    if (!path) return -1;
    for (size_t i = 0; i < staged_count; ++i) {
        P4GameTableInput *input = staged_inputs[i];
        if (strcmp(path,p4_game_table_input_label(input->kind))) continue;
        if (!input->sealed || !input->golden_origin[0] || input->size > UINT32_MAX) return -1;
        uint8_t *copy = malloc(input->size);
        if (!copy) return -1;
        memcpy(copy,input->bytes,input->size);
        *bytes = copy; *size = (uint32_t)input->size;
        return 1;
    }
    return -1; /* Never fall back to disc for an unrecognised table descriptor. */
}

int p4_game_table_admission_golden_packed(const uint8_t *descriptor, const char *path,
    uint8_t *bytes, uint32_t size, P4GameTablePackedObserver observer)
{
    const P4GameTableInput *input = NULL;
    const P4PackageProvider *failed = NULL;
    if (!staged_golden || !golden_allocator.allocate || !observer ||
        check_packed(descriptor,path,bytes,size,&failed,&input) || !input || !input->golden_origin[0])
        return P4_TABLE_INVALID;
    size_t total = 0, count = p4_game_table_input_span_count(input);
    for (size_t i = 0; i < count; ++i) {
        size_t n = p4_game_table_input_span_size(p4_game_table_input_span(input,i));
        if (n > UINT32_MAX - 15u || total > UINT32_MAX - ((n + 15u) & ~(size_t)15u))
            return P4_TABLE_INVALID;
        total += (n + 15u) & ~(size_t)15u;
    }
    if (!total) return P4_TABLE_INVALID;
    struct GoldenArena *arena = &golden_arenas[input->kind];
    if (!arena->host) {
        uint32_t guest = 0;
        void *host = golden_allocator.allocate(golden_allocator.user,(uint32_t)total,&guest);
        if (!host) return P4_TABLE_NOMEM;
        if (!guest || guest > UINT32_MAX - total) {
            golden_allocator.release(golden_allocator.user,guest,host,(uint32_t)total);
            return P4_TABLE_INVALID;
        }
        *arena = (struct GoldenArena){host,guest,(uint32_t)total};
    } else if (arena->size != total) return P4_TABLE_CONFLICT;
    p4_game_table_admission_packed(descriptor,path,bytes,size,observer);
    memset(arena->host,0,arena->size);
    size_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        const P4GameTableInputSpan *span = p4_game_table_input_span(input,i);
        size_t n = p4_game_table_input_span_size(span);
        size_t source = (size_t)p4_game_table_input_span_offset(span);
        memcpy((uint8_t *)arena->host + offset,bytes + source,n);
        p4_game_table_slot_publish(word(descriptor + 8u*i + 4u),arena->guest + (uint32_t)offset,(uint32_t)n);
        offset += (n + 15u) & ~(size_t)15u;
    }
    return P4_TABLE_OK;
}


int p4_game_table_admission_golden_load_packed(const uint8_t *descriptor, const char *path,
    P4GameTablePackedObserver observer)
{
    uint8_t *bytes = NULL;
    uint32_t size = 0;
    if (p4_game_table_admission_golden_copy(path, &bytes, &size) != 1) return P4_TABLE_INVALID;
    int result = p4_game_table_admission_golden_packed(descriptor, path, bytes, size, observer);
    free(bytes);
    return result;
}
