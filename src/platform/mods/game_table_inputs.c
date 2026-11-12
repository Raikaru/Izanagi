#include "game_table_inputs_internal.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static const struct InputSpec { const char *label; uint32_t blocks; } specs[] = {
    {"init/itemtbl.bin", 1}, {"battle/SKILL.TBL", 2}, {"battle/UNIT.TBL", 3},
    {"battle/MSG.TBL", 5}, {"battle/PERSONA.TBL", 6}, {"battle/ENCOUNT.TBL", 5},
    {"battle/EFFECT.TBL", 1}, {"battle/MODEL.TBL", 3}, {"battle/AICALC.TBL", 11}
};
_Static_assert(sizeof specs / sizeof specs[0] == P4_TABLE_INPUT_COUNT, "native source specification count");
static uint32_t word(const uint8_t *bytes)
{ return bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24; }
/* The original file's headers give the actual arena consumption; file size
 * alone is insufficient when a block's four-byte header changes its padding. */
static int retail_packed_bounds(P4VfsSourcePlan *disc, P4GameTableInputKind kind,
    uint64_t *file_size, uint64_t *footprint)
{
    const P4VfsSourceInfo *info = vfs_source_plan_info(disc);
    if (!info || !info->size || info->size > UINT32_MAX) return -1;
    uint64_t offset = 0, total = 0;
    for (uint32_t block = 0; block < specs[kind].blocks; ++block) {
        uint8_t header[4];
        if (offset > info->size || info->size - offset < 4u ||
            vfs_source_plan_read_at(disc, offset, header, sizeof header)) return -1;
        uint32_t size = word(header);
        if ((uint64_t)size > info->size - offset - 4u) return -1;
        uint64_t next = (offset + 4u + size + 15u) & ~UINT64_C(15);
        if (next > info->size) return -1;
        total += ((uint64_t)size + 15u) & ~UINT64_C(15);
        offset = next;
    }
    if (offset != info->size) return -1;
    *file_size = info->size; *footprint = total;
    return 0;
}
const char *p4_game_table_input_label(P4GameTableInputKind kind)
{ return (unsigned)kind < P4_TABLE_INPUT_COUNT ? specs[kind].label : NULL; }
static unsigned upper(unsigned c) { return c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c; }
static int same_key(const char *left, const char *right)
{
    while (*left && upper((unsigned char)*left) == upper((unsigned char)*right)) { ++left; ++right; }
    return !*left && !*right;
}
/* Root inventory positions are also projection positions; the exposed key may
 * strip a Compendium wrapper while file_open uses its original contained key. */
static int replacement(const char *request, const char *qualified,
    const P4PackageProvider **winner, const char **relative)
{
    for (size_t ordinal = p4_packages_count(); ordinal; --ordinal) {
        const P4PackageProvider *provider = p4_packages_provider(ordinal - 1u);
        if (!provider || !p4_package_provider_selected(provider)) continue;
        size_t alternate = SIZE_MAX, selected = SIZE_MAX;
        for (size_t i = 0, count = p4_package_provider_files_count(provider); i < count; ++i) {
            const char *key; int directory;
            int result = p4_package_provider_replacement(provider, i, &key, &directory);
            if (result < 0) return -1;
            if (result || directory) continue;
            if (same_key(key, request)) { selected = i; break; }
            if (qualified && same_key(key, qualified)) alternate = i;
        }
        if (selected == SIZE_MAX) selected = alternate;
        if (selected != SIZE_MAX) {
            int directory;
            *winner = provider;
            return p4_package_provider_file(provider, selected, relative, &directory) || directory ? -1 : 0;
        }
    }
    return 1;
}
static P4GameTableInput *allocate_input(P4GameTableInputKind kind, uint64_t size,
    const P4PackageProvider *provider, P4PackageFilePlan *package, P4VfsSourcePlan *native)
{
    size_t metadata = sizeof(P4GameTableInput) + specs[kind].blocks * sizeof(P4GameTableInputSpan);
    if (!size || size > UINT32_MAX || size > SIZE_MAX - metadata || p4_packages_memory_reserve(metadata)) return NULL;
    /* Payload/decode bytes are outside the shared metadata budget. Coallocate
     * them with the exact required span metadata, with only one allocation. */
    P4GameTableInput *input = malloc(metadata + (size_t)size);
    if (!input) { p4_packages_memory_cancel(metadata); return NULL; }
    p4_packages_memory_commit(metadata);
    input->kind = kind; input->provider = provider; input->package = package; input->native = native;
    input->size = (size_t)size; input->sealed = 0;
    input->retail_size = input->retail_footprint = 0;
    input->retail_bounds_valid = input->retail_is_selected = 0;
    input->native_uses = NULL; input->native_used = 0; input->digest_valid = 0;
    input->bytes = (uint8_t *)(input->spans + specs[kind].blocks);
    return input;
}
void p4_game_table_input_destroy(P4GameTableInput *input)
{
    if (!input) return;
    if (input->native_uses) abort(); /* Unregister the owning use context first. */
    if (input->native) vfs_source_plan_release(input->native);
    if (input->package) p4_package_file_plan_release(input->package);
    if (input->provider) p4_package_provider_release(input->provider);
    size_t metadata = sizeof *input + specs[input->kind].blocks * sizeof *input->spans;
    free(input);
    p4_packages_memory_release(metadata);
}
int p4_game_table_input_prepare(P4GameTableInputKind kind, uint64_t max_bytes,
    P4GameTableInput **out, const P4PackageProvider **failed_provider)
{
    if (failed_provider) *failed_provider = NULL;
    const char *label = p4_game_table_input_label(kind);
    if (!label || !max_bytes || !out || *out || vfs_source_prepare_selected()) return -1;
    P4VfsSourcePlan *disc = NULL, *native = NULL;
    int result = vfs_source_plan_acquire(P4_VFS_DISC, label, &disc);
    if (result < 0) return -1;
    result = vfs_source_plan_acquire(P4_VFS_DEVELOPER, label, &native);
    if (result < 0) { vfs_source_plan_release(disc); return -1; }
    const P4PackageProvider *provider = NULL;
    P4PackageFile *file = NULL;
    P4PackageFilePlan *package = NULL;
    if (!native) {
        const char *relative = NULL;
        const P4VfsSourceInfo *info = vfs_source_plan_info(disc);
        result = replacement(label, info ? info->key : NULL, &provider, &relative);
        if (result < 0) { vfs_source_plan_release(disc); return -1; }
        if (!result) {
            if (failed_provider) *failed_provider = provider;
            if (p4_package_file_open(provider, relative, max_bytes, &file) ||
                p4_package_file_plan_acquire(file, &package)) {
                if (file) (void)p4_package_file_close(file);
                vfs_source_plan_release(disc);
                return -1;
            }
            if (p4_package_provider_retain(provider)) {
                (void)p4_package_file_close(file); p4_package_file_plan_release(package);
                vfs_source_plan_release(disc); return -1;
            }
        } else { native = disc; disc = NULL; }
    }
    uint64_t retail_size = 0, retail_footprint = 0;
    int retail_valid = kind != P4_TABLE_INPUT_ITEM && disc &&
        !retail_packed_bounds(disc, kind, &retail_size, &retail_footprint);
    vfs_source_plan_release(disc);
    if (!native && !package) return 1;
    const P4VfsSourceInfo *native_info = vfs_source_plan_info(native);
    uint64_t size = native ? native_info->size : p4_package_file_size(file);
    P4GameTableInput *input = size <= max_bytes ? allocate_input(kind, size, provider, package, native) : NULL;
    if (!input) {
        if (file) (void)p4_package_file_close(file);
        if (native) vfs_source_plan_release(native);
        if (package) p4_package_file_plan_release(package);
        if (provider) p4_package_provider_release(provider);
        return -1;
    }
    input->retail_size = retail_size; input->retail_footprint = retail_footprint;
    input->retail_bounds_valid = retail_valid;
    input->retail_is_selected = kind != P4_TABLE_INPUT_ITEM && native_info &&
        native_info->kind == P4_VFS_DISC;
    for (size_t offset = 0; offset < input->size;) {
        size_t count = input->size - offset;
        if (count > INT_MAX) count = INT_MAX;
        result = native ? vfs_source_plan_read_at(native, offset, input->bytes + offset, count) :
            p4_package_file_read_at(file, offset, input->bytes + offset, count);
        if (result) break;
        offset += count;
    }
    if (file && p4_package_file_close(file)) result = -1;
    if (result) { p4_game_table_input_destroy(input); return -1; }
    *out = input;
    if (failed_provider) *failed_provider = NULL;
    return 0;
}
const uint8_t *p4_game_table_input_bytes(const P4GameTableInput *input) { return input ? input->bytes : NULL; }
size_t p4_game_table_input_size(const P4GameTableInput *input) { return input ? input->size : 0; }
P4GameTableInputKind p4_game_table_input_kind(const P4GameTableInput *input)
{ return input ? input->kind : P4_TABLE_INPUT_COUNT; }
const P4PackageProvider *p4_game_table_input_provider(const P4GameTableInput *input) { return input ? input->provider : NULL; }
const P4VfsSourceInfo *p4_game_table_input_native(const P4GameTableInput *input)
{ return input ? vfs_source_plan_info(input->native) : NULL; }
/* Decode selected packed blocks below; `word` also parses original headers. */
int p4_game_table_input_seal(P4GameTableInput *input)
{
    if (!input) return -1;
    if (input->sealed) return 0;
    if (input->kind == P4_TABLE_INPUT_ITEM) {
        if (input->size < 16u) return -1;
        input->spans[0] = (P4GameTableInputSpan){input, 0, input->size};
    } else {
        size_t offset = 0;
        for (uint32_t block = 0; block < specs[input->kind].blocks; ++block) {
            if (offset > input->size || input->size - offset < 4u) return -1;
            uint32_t size = word(input->bytes + offset);
            if (size > input->size - offset - 4u) return -1;
            input->spans[block] = (P4GameTableInputSpan){input, offset + 4u, size};
            uint64_t next = ((uint64_t)offset + 4u + size + 15u) & ~UINT64_C(15);
            if (next > input->size) return -1;
            offset = (size_t)next;
        }
        if (offset != input->size) return -1;
        if (input->retail_is_selected) {
            uint64_t total = 0;
            for (uint32_t block = 0; block < specs[input->kind].blocks; ++block)
                total += ((uint64_t)input->spans[block].size + 15u) & ~UINT64_C(15);
            input->retail_size = input->size; input->retail_footprint = total;
            input->retail_bounds_valid = 1;
        }
    }
    if (p4_content_fingerprint_seal(&input->fingerprint, input->bytes, input->size)) return -1;
    input->sealed = 1;
    return 0;
}
int p4_game_table_input_matches(const P4GameTableInput *input, const void *bytes, size_t size)
{
    return input && input->sealed && p4_content_fingerprint_matches(&input->fingerprint, bytes, size);
}
size_t p4_game_table_input_span_count(const P4GameTableInput *input)
{ return input && input->sealed ? specs[input->kind].blocks : 0; }
const P4GameTableInputSpan *p4_game_table_input_span(const P4GameTableInput *input, size_t index)
{ return index < p4_game_table_input_span_count(input) ? &input->spans[index] : NULL; }
const uint8_t *p4_game_table_input_span_bytes(const P4GameTableInputSpan *span)
{ return span && span->input->sealed ? span->input->bytes + span->offset : NULL; }
size_t p4_game_table_input_span_size(const P4GameTableInputSpan *span)
{ return span && span->input->sealed ? span->size : 0; }
uint64_t p4_game_table_input_span_offset(const P4GameTableInputSpan *span)
{ return span && span->input->sealed ? span->offset : 0; }
const P4GameTableInput *p4_game_table_input_span_owner(const P4GameTableInputSpan *span)
{ return span && span->input->sealed ? span->input : NULL; }
size_t p4_game_table_input_span_index(const P4GameTableInputSpan *span)
{ return span && span->input->sealed ? (size_t)(span - span->input->spans) : SIZE_MAX; }
int p4_game_table_input_owns_span(const P4GameTableInput *input, const void *identity)
{
    if (!input || !input->sealed || !identity) return 0;
    uintptr_t first = (uintptr_t)input->spans, address = (uintptr_t)identity;
    return address >= first && address - first < specs[input->kind].blocks * sizeof *input->spans &&
        (address - first) % sizeof *input->spans == 0;
}
