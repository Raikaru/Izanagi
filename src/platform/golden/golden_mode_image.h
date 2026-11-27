#ifndef P4_GOLDEN_MODE_IMAGE_H
#define P4_GOLDEN_MODE_IMAGE_H
#include "golden_mode_identity.h"

#define P4_GOLDEN_IMAGE_COUNT (8u + P4_GOLDEN_LITERAL_SOURCE_COUNT)
#define P4_GOLDEN_IMAGE_EXTRA_COUNT (P4_GOLDEN_IMAGE_COUNT + 1u)
#define P4_GOLDEN_IMAGE_INDEX_CAP (4096u + 192u * P4_GOLDEN_LITERAL_SOURCE_COUNT)
/* A zero address denotes a widened runtime installer or authenticated metadata.
 * Fixed writes never exceed retail_size, even when the object has more rows. */
typedef struct P4GoldenImageObject {
    const char *name, *converter;
    size_t size;
    uint32_t address, retail_size;
} P4GoldenImageObject;
/* OPEN-retain is explicitly unresolved Golden balance, not a D10c decision. */
typedef struct P4GoldenLiteralObject {
    uint32_t address, size;
    const char *name, *decision, *reason, *proof_path, *supplier, *runtime_status;
} P4GoldenLiteralObject;
/* Proven, compiled source spans; archive member hashes cover the entire FTD,
 * while PE hashes cover exactly the proven span. Output offsets share one bank. */
typedef struct P4GoldenLiteralSource {
    const char *name, *section, *member_chain, *sha256;
    uint32_t offset, size, output_offset, rows, stride;
    uint32_t read_size, converter;
    const uint8_t *constant; /* Authored spec-constant bytes, never image payload. */
} P4GoldenLiteralSource;
const P4GoldenLiteralSource *p4_golden_image_literal_sources(size_t *count);
/* Normalize only reference-table identities, never the virtual queue objects.
 * All validation completes before writing the output. */
int p4_golden_image_normalize_queues(const uint8_t raw[168], const uint8_t remap[8],
    uint64_t image_base, uint32_t source_base, uint32_t guest_base, uint8_t out[84]);
const P4GoldenLiteralObject *p4_golden_image_literal_registry(size_t *count);
int p4_golden_image_literal_prepare(char *reason, size_t capacity);
const P4GoldenImageObject *p4_golden_image_registry(size_t *count);
/* Canonical data-only manifest; exact comparison rejects unknown/duplicate rows,
 * destinations, converter versions, lengths and SHA-256 mismatches. */
int p4_golden_image_index(const P4GoldenCacheTable *objects, size_t count,
    char *index, size_t capacity, size_t *written);
/* Borrow artifacts; own only the two allocated wire buffers. */
int p4_golden_image_encode(const P4GoldenModeIdentityArtifacts *artifacts,
    P4GoldenCacheTable objects[P4_GOLDEN_IMAGE_EXTRA_COUNT],
    uint8_t **recipes, uint8_t **index);
#endif
