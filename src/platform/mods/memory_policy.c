#include "p4pc/mod_memory.h"
#include "p4pc/guest.h"
#include "p4pc/image.h"
#include "p4pc/platform.h"
#include "p4pc/tables.h"
#include <stdlib.h>

struct P4ModMemoryRegion {
    uint32_t address;
    size_t size;
    unsigned flags;
    P4ModMemoryRegion *next;
};
static P4ModMemoryRegion *regions;
static size_t region_count;
static uint32_t *function_cells;
static size_t function_cell_count;
#ifndef P4_GUEST64
typedef struct FunctionWord { uint32_t word, address; } FunctionWord;
static FunctionWord *function_words;
static size_t function_word_count;
#endif
static int index_state;

static int valid_span(uint32_t address, size_t size)
{
    return size <= UINT64_C(0x100000000) - address;
}

static int overlaps(uint32_t address, size_t size, uint32_t other, size_t extent)
{
    return size != 0 && extent != 0 && address < (uint64_t)other + extent &&
           other < (uint64_t)address + size;
}

static int contains(uint32_t outer, size_t extent, uint32_t address, size_t size)
{
    return address >= outer && (uint64_t)address - outer <= extent &&
           size <= extent - (size_t)((uint64_t)address - outer);
}

static int compare_word(const void *left, const void *right)
{
    uint32_t a = *(const uint32_t *)left, b = *(const uint32_t *)right;
    return (a > b) - (a < b);
}

#ifndef P4_GUEST64
static int compare_function_word(const void *left, const void *right)
{
    const FunctionWord *a = left, *b = right;
    if (a->word != b->word) return (a->word > b->word) - (a->word < b->word);
    /* Canonical retail entries precede non-retail host/patch-only targets. */
    if (!a->address != !b->address) return !a->address ? 1 : -1;
    return (a->address > b->address) - (a->address < b->address);
}
#endif

static int prepare_index(void)
{
    if (index_state != 0) return index_state > 0 ? 0 : -1;
    index_state = -1;
    if (p4pc_patch_count > SIZE_MAX / sizeof *function_cells) return -1;
    if (p4pc_patch_count != 0) {
        function_cells = malloc(p4pc_patch_count * sizeof *function_cells);
        if (function_cells == NULL) return -1;
        for (size_t i = 0; i < p4pc_patch_count; ++i)
            function_cells[i] = p4pc_patch_table[i].addr;
        qsort(function_cells, p4pc_patch_count, sizeof *function_cells, compare_word);
    }
    function_cell_count = p4pc_patch_count;
#ifndef P4_GUEST64
    if (p4pc_native_count > SIZE_MAX - p4pc_patch_count ||
        p4pc_native_count + p4pc_patch_count > SIZE_MAX - p4pc_hostfn_count)
        return -1;
    size_t count = p4pc_native_count + p4pc_patch_count + p4pc_hostfn_count;
    if (count > SIZE_MAX / sizeof *function_words) return -1;
    if (count != 0) {
        function_words = malloc(count * sizeof *function_words);
        if (function_words == NULL) return -1;
        size_t n = 0;
        for (size_t i = 0; i < p4pc_native_count; ++i)
            function_words[n++] = (FunctionWord){
                (uint32_t)(uintptr_t)p4pc_native_table[i].fn, p4pc_native_table[i].addr};
        for (size_t i = 0; i < p4pc_patch_count; ++i)
            function_words[n++] = (FunctionWord){(uint32_t)(uintptr_t)p4pc_patch_table[i].target, 0};
        for (size_t i = 0; i < p4pc_hostfn_count; ++i)
            function_words[n++] = (FunctionWord){(uint32_t)(uintptr_t)p4pc_hostfn_table[i].fn, 0};
        qsort(function_words, count, sizeof *function_words, compare_function_word);
    }
    function_word_count = count;
#endif
    index_state = 1;
    return 0;
}

#ifndef P4_GUEST64
static const FunctionWord *find_function_word(uint32_t word)
{
    if (prepare_index() != 0) return NULL;
    size_t low = 0, high = function_word_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (function_words[middle].word < word) low = middle + 1;
        else high = middle;
    }
    return low < function_word_count && function_words[low].word == word
        ? &function_words[low] : NULL;
}
#endif

int p4_mod_memory_function_known(uint32_t word)
{
    if (word == 0) return 1;
#ifdef P4_GUEST64
    return p4_fn_known(word);
#else
    return find_function_word(word) != NULL;
#endif
}

int p4_mod_memory_function_encode(uint32_t address, uint32_t *word)
{
    if (word == NULL) return -1;
    if (address == 0) { *word = 0; return 0; }
    /* gen_port emits native_table in ascending retail-entry order. */
    size_t low = 0, high = p4pc_native_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (p4pc_native_table[middle].addr < address) low = middle + 1;
        else high = middle;
    }
    if (low == p4pc_native_count || p4pc_native_table[low].addr != address) return -1;
#ifdef P4_GUEST64
    if (!p4_fn_known(address)) return -1;
    *word = address;
#else
    *word = (uint32_t)(uintptr_t)p4pc_native_table[low].fn;
#endif
    return 0;
}

int p4_mod_memory_function_decode(uint32_t word, uint32_t *address)
{
    if (address == NULL) return -1;
    if (word == 0) { *address = 0; return 0; }
#ifdef P4_GUEST64
    uint32_t encoded;
    if (p4_mod_memory_function_encode(word, &encoded) != 0) return -1;
    *address = word;
#else
    const FunctionWord *entry = find_function_word(word);
    if (entry == NULL || entry->address == 0) return -1;
    *address = entry->address;
#endif
    return 0;
}

P4ModMemoryRegion *p4_mod_memory_declare(uint32_t address, size_t size, unsigned flags)
{
    const unsigned allowed = P4_MOD_MEMORY_READ | P4_MOD_MEMORY_WRITE |
                             P4_MOD_MEMORY_HOST_DATA | P4_MOD_MEMORY_FUNCTION_WORDS;
    if (size == 0 || !valid_span(address, size) || flags == 0 || (flags & ~allowed) ||
        ((flags & P4_MOD_MEMORY_WRITE) && !(flags & P4_MOD_MEMORY_READ)) ||
        ((flags & P4_MOD_MEMORY_HOST_DATA) && flags != P4_MOD_MEMORY_HOST_DATA) ||
        ((flags & P4_MOD_MEMORY_FUNCTION_WORDS) && ((address & 3u) || (size & 3u))) ||
        region_count == 4096)
        return NULL;
    P4ModMemoryRegion *region = malloc(sizeof *region);
    if (region == NULL) return NULL;
    *region = (P4ModMemoryRegion){address, size, flags, regions};
    regions = region;
    ++region_count;
    return region;
}

void p4_mod_memory_revoke(P4ModMemoryRegion *region)
{
    P4ModMemoryRegion **link = &regions;
    while (*link != NULL && *link != region) link = &(*link)->next;
    if (*link != NULL) {
        *link = region->next;
        --region_count;
        free(region);
    }
}

static const char *check_function_word(uint32_t cell, uint32_t address,
                                       size_t size, const unsigned char *bytes)
{
    if (!contains(address, size, cell, 4))
        return "partial function-pointer writes are refused";
    const unsigned char *p = bytes + (size_t)(cell - address);
    uint32_t word = (uint32_t)p[0] | (uint32_t)p[1] << 8 |
                    (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
    return p4_mod_memory_function_known(word) ? NULL : "unknown function-pointer entry";
}

const char *p4_mod_memory_check(uint32_t address, size_t size, int write,
                              int owned_scratch, const void *bytes)
{
    if (!valid_span(address, size)) return "guest range overflows address space";
    if (size == 0) return NULL;
    if (write && bytes == NULL) return "missing guest write payload";
    if (p4pc_thread_guest_stack_overlap(address, size))
        return "native coroutine stacks are protected";
    if (p4pc_heap_guest_host_overlap(address, size))
        return "native heap metadata or quarantined storage is protected";
    int admitted = owned_scratch != 0;
    for (const P4ModMemoryRegion *r = regions; r != NULL; r = r->next) {
        if (overlaps(address, size, r->address, r->size) &&
            ((r->flags & P4_MOD_MEMORY_HOST_DATA) ||
             (write && (r->flags & P4_MOD_MEMORY_READ) && !(r->flags & P4_MOD_MEMORY_WRITE))))
            return "native-owned or readonly guest storage is protected";
        if ((r->flags & (write ? P4_MOD_MEMORY_WRITE : P4_MOD_MEMORY_READ)) &&
            contains(r->address, r->size, address, size))
            admitted = 1;
    }
    size_t count;
    const p4pc_image_guest_range_t *image = p4pc_image_guest_ranges(&count);
    for (size_t i = 0; i < count; ++i) {
        if (write && (image[i].flags & P4_IMAGE_GUEST_EXEC) &&
            overlaps(address, size, image[i].begin, image[i].end - image[i].begin))
            return "guest code is not writable by mods";
        if (write && !(image[i].flags & P4_IMAGE_GUEST_WRITE) &&
            overlaps(address, size, image[i].begin, image[i].end - image[i].begin))
            return "readonly ELF sections are protected";
        if (write && (image[i].flags & P4_IMAGE_GUEST_WRITE) &&
            contains(image[i].begin, image[i].end - image[i].begin, address, size))
            admitted = 1;
    }
    int retail = contains(0x00100000u, 0x01f00000u, address, size);
    if (!write && retail) admitted = 1;
    if (write && retail && !owned_scratch && image == NULL)
        return "ELF section evidence is unavailable for guest writes";
    uint32_t begin, end;
    if (write && image != NULL && p4pc_heap_guest_span(&begin, &end) == 0 &&
        contains(begin, end - begin, address, size))
        admitted = 1;
    if (!admitted) return "guest range is outside the mod memory allowlist";
    if (!write) return NULL;
    if (prepare_index() != 0) return "function-pointer policy allocation failed";
    uint64_t limit = (uint64_t)address + size;
    uint32_t first = address >= 3 ? address - 3 : 0;
    size_t low = 0, high = function_cell_count;
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        if (function_cells[middle] < first) low = middle + 1;
        else high = middle;
    }
    for (size_t i = low; i < function_cell_count && function_cells[i] < limit; ++i) {
        const char *error = check_function_word(function_cells[i], address, size, bytes);
        if (error != NULL) return error;
    }
    for (const P4ModMemoryRegion *r = regions; r != NULL; r = r->next) {
        if (!(r->flags & P4_MOD_MEMORY_FUNCTION_WORDS) ||
            !overlaps(address, size, r->address, r->size))
            continue;
        uint64_t cell = r->address;
        if (address > cell) cell += ((uint64_t)address - cell) / 4 * 4;
        uint64_t end_cell = (uint64_t)r->address + r->size;
        for (; cell < limit && cell < end_cell; cell += 4) {
            const char *error = check_function_word((uint32_t)cell, address, size, bytes);
            if (error != NULL) return error;
        }
    }
    return NULL;
}

void p4_mod_memory_clear(void)
{
    free(function_cells);
    function_cells = NULL;
    function_cell_count = 0;
#ifndef P4_GUEST64
    free(function_words);
    function_words = NULL;
    function_word_count = 0;
#endif
    index_state = 0;
}
