/* All image destinations and converter revisions have one compiled owner. */
#include "golden_mode_image.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "golden_mode_literal_manifest.inc"

static const P4GoldenImageObject registry[] = {
    {"image/charts", "fusion@1", 3u * 28u * 28u, 0, 0},
    {"image/experience", "exp-pe@1", 99u * 4u, 0x005dd6e0u, 99u * 4u},
    {"image/gates", "fusion@1", 26u * 8u, 0, 0},
    {"image/inheritance", "inheritance@1", 624, 0x00641e60u, 576},
    {"image/l1-ledger", "l1-ledger@1", sizeof literal_manifest - 1u, 0, 0},
#include "golden_mode_literal_rows.inc"
    {"image/recipes", "recipe@2", 17u * 27u, 0, 0},
    {"image/tiers", "fusion@1", 18, 0x00641c20u, 18},
    {"image/weights", "fusion@1", 532, 0x00641c40u, 532}
};
_Static_assert(sizeof registry / sizeof registry[0] == P4_GOLDEN_IMAGE_COUNT, "image registry count");
const P4GoldenImageObject *p4_golden_image_registry(size_t *count)
{
    if (count) *count = sizeof registry / sizeof registry[0];
    return registry;
}
int p4_golden_image_index(const P4GoldenCacheTable *objects, size_t count,
    char *index, size_t capacity, size_t *written)
{
    size_t used = 0;
    if (!objects || count != P4_GOLDEN_IMAGE_COUNT || !index || !capacity || !written) return -1;
    int n = snprintf(index, capacity, "{\"format\":1,\"objects\":[\n");
    if (n < 0 || (size_t)n >= capacity) return -1;
    used = (size_t)n;
    for (size_t i = 0; i < count; ++i) {
        const P4GoldenImageObject *r = registry + i;
        const P4GoldenCacheTable *o = objects + i;
        char hash[65];
        if (!o->name || strcmp(o->name, r->name) || !o->data || o->size != r->size) return -1;
        if (!strcmp(r->name, "image/l1-ledger") &&
            memcmp(o->data, literal_manifest, sizeof literal_manifest - 1u)) return -1;
        p4_content_digest(o->data, o->size, hash);
        n = snprintf(index + used, capacity - used, "%s[\"%s\",\"%s\",%zu,%u,%u,\"%s\"]",
            i ? ",\n" : "", r->name, r->converter, r->size, r->address, r->retail_size, hash);
        if (n < 0 || (size_t)n >= capacity - used) return -1;
        used += (size_t)n;
    }
    n = snprintf(index + used, capacity - used, "\n]}\n");
    if (n < 0 || (size_t)n >= capacity - used) return -1;
    *written = used + (size_t)n;
    return 0;
}
static void put16(uint8_t *p, unsigned v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); }
int p4_golden_image_encode(const P4GoldenModeIdentityArtifacts *a,
    P4GoldenCacheTable objects[P4_GOLDEN_IMAGE_EXTRA_COUNT], uint8_t **recipes, uint8_t **index)
{
    if (!a || !objects || !recipes || !index || *recipes || *index || a->inheritance_size != 624) return -1;
    uint8_t *wire = malloc(17u * 27u), *manifest = malloc(P4_GOLDEN_IMAGE_INDEX_CAP);
    size_t written;
    if (!wire || !manifest) { free(wire); free(manifest); return -1; }
    for (unsigned i = 0; i < 17; ++i) {
        const P4GoldenRecipe *r = a->fusion.recipes + i;
        uint8_t *p = wire + i * 27u;
        put16(p, r->result);
        for (unsigned j = 0; j < 12; ++j) put16(p + 2u + j * 2u, r->materials[j]);
        p[26] = r->material_count;
    }
    uint8_t *data[P4_GOLDEN_IMAGE_COUNT] = {(uint8_t *)a->fusion.charts, (uint8_t *)a->experience,
        (uint8_t *)a->fusion.gates, (uint8_t *)a->inheritance, (uint8_t *)literal_manifest};
    for (unsigned i = 0; i < P4_GOLDEN_LITERAL_SOURCE_COUNT; ++i)
        data[5u + i] = (uint8_t *)a->literal_bytes + literal_sources[i].output_offset;
    data[5u + P4_GOLDEN_LITERAL_SOURCE_COUNT] = wire;
    data[6u + P4_GOLDEN_LITERAL_SOURCE_COUNT] = (uint8_t *)a->fusion.limits;
    data[7u + P4_GOLDEN_LITERAL_SOURCE_COUNT] = (uint8_t *)a->fusion.affinities;
    for (unsigned i = 0; i < P4_GOLDEN_IMAGE_COUNT; ++i)
        objects[i] = (P4GoldenCacheTable){registry[i].name, data[i], registry[i].size};
    if (p4_golden_image_index(objects, P4_GOLDEN_IMAGE_COUNT, (char *)manifest,
                              P4_GOLDEN_IMAGE_INDEX_CAP, &written)) {
        free(wire); free(manifest); return -1;
    }
    objects[P4_GOLDEN_IMAGE_COUNT] = (P4GoldenCacheTable){"image-index.json", manifest, written};
    *recipes = wire; *index = manifest;
    return 0;
}

const P4GoldenLiteralObject *p4_golden_image_literal_registry(size_t *count)
{
    if (count) *count = sizeof literal_registry / sizeof literal_registry[0];
    return literal_registry;
}
const P4GoldenLiteralSource *p4_golden_image_literal_sources(size_t *count)
{
    if (count) *count = sizeof literal_sources / sizeof literal_sources[0];
    return literal_sources;
}

int p4_golden_image_literal_prepare(char *reason, size_t capacity)
{
    size_t count, bytes = 0, open = 0, bound = 0, pending = 0, nonsupply = 0;
    const P4GoldenLiteralObject *rows = p4_golden_image_literal_registry(&count);
    for (size_t i = 0; i < count; ++i) {
        const P4GoldenLiteralObject *r = rows + i;
        int supplied = !strcmp(r->decision, "bound");
        int retained = !strcmp(r->decision, "D10c") || !strcmp(r->decision, "OPEN-retain") ||
                       !strcmp(r->decision, "pending-seam");
        if ((!retained && !supplied) || !r->size || !r->name[0] || !r->reason[0] ||
            (i && r->address < rows[i - 1].address + rows[i - 1].size)) {
            if (reason && capacity) snprintf(reason, capacity, "invalid or unsupplied L1 literal: %s", r->name);
            return -1;
        }
        bytes += r->size;
        if (supplied) {
            const char *name = r->supplier;
            size_t j = 0;
            while (j < P4_GOLDEN_IMAGE_COUNT && strcmp(registry[j].name, name)) ++j;
            if (j == P4_GOLDEN_IMAGE_COUNT || !registry[j].size) {
                if (reason && capacity) snprintf(reason, capacity, "missing proven L1 image object: %s", name);
                return -1;
            }
            ++bound;
        }
        if (!strcmp(r->decision, "pending-seam")) ++pending;
        if (!strcmp(r->decision, "D10c")) ++nonsupply;
        if (!strcmp(r->decision, "pending-seam") || !strcmp(r->decision, "D10c"))
            fprintf(stderr, "p4pc: L1 %s: %s (0x%08x): %s\n",
                    r->decision, r->name, r->address, r->runtime_status);
        if (!strcmp(r->decision, "OPEN-retain")) {
            if (!r->proof_path[0]) {
                if (reason && capacity) snprintf(reason, capacity, "L1-open literal lacks proof path: %s", r->name);
                return -1;
            }
            ++open;
        }
    }
    if (count != 1043u || bytes != 249428u) {
        if (reason && capacity) snprintf(reason, capacity, "incomplete L1 supplier ledger");
        return -1;
    }
    for (size_t i = 0; i < count; ++i)
        if (!strcmp(rows[i].decision, "OPEN-retain"))
            fprintf(stderr, "p4pc: L1-open: %s (0x%08x, %u bytes): PS2 retained; %s\n",
                    rows[i].name, rows[i].address, rows[i].size, rows[i].proof_path);
    fprintf(stderr, "p4pc: L1 supplier ledger: %zu objects; bound=%zu; pending-seam=%zu; non-supply=%zu; L1-open=%zu (unresolved before M12)\n",
            count, bound, pending, nonsupply, open);
    if (reason && capacity) reason[0] = 0;
    return 0;
}

int p4_golden_image_normalize_queues(const uint8_t raw[168], const uint8_t remap[8],
    uint64_t image_base, uint32_t source_base, uint32_t guest_base, uint8_t out[84])
{
    static const uint8_t ps2_remap[8] = {0, 1, 4, 6, 2, 3, 5, 0}; /* tnEffect.c */
    if (!raw || !remap || !out || memcmp(remap, ps2_remap, sizeof ps2_remap) ||
        source_base > UINT32_MAX - 6u * 96u || guest_base > UINT32_MAX - 6u * 48u ||
        image_base > UINT64_MAX - source_base - 6u * 96u) return -1;
    for (unsigned bank = 0; bank < 3; ++bank) for (unsigned j = 0; j < 7; ++j) {
        const uint8_t *p = raw + (bank * 7u + j) * 8u;
        uint64_t address = 0;
        for (unsigned byte = 0; byte < 8; ++byte) address |= (uint64_t)p[byte] << (byte * 8u);
        if (address != image_base + source_base + j * 96u) return -1;
    }
    for (unsigned bank = 0; bank < 3; ++bank) for (unsigned j = 0; j < 7; ++j) {
        uint32_t address = guest_base + j * 48u;
        uint8_t *p = out + (bank * 7u + j) * 4u;
        for (unsigned byte = 0; byte < 4; ++byte) p[byte] = (uint8_t)(address >> (byte * 8u));
    }
    return 0;
}
