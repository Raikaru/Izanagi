/* Allocation-free SHA-256 shared by asset identities and native consumers. */
#ifndef P4PC_CONTENT_DIGEST_H
#define P4PC_CONTENT_DIGEST_H
#include <stddef.h>
#include <stdint.h>

#define P4_CONTENT_DIGEST_HEX 64

#ifdef __cplusplus
extern "C" {
#endif

/* Caller-owned streaming state. Fields are implementation details, not a wire
 * format. Begin before use; end consumes the state. Begin may reset it for reuse.
 * Updates concatenate bytes; NULL is valid only for an empty update. */
typedef struct P4ContentDigest {
    uint32_t h[8];
    uint64_t bytes;
    size_t used;
    uint8_t block[64];
} P4ContentDigest;

void p4_content_digest_begin(P4ContentDigest *state);
void p4_content_digest_update(P4ContentDigest *state, const void *data, size_t size);
/* Lowercase hexadecimal plus NUL, identical to the one-shot convenience API. */
void p4_content_digest_end(P4ContentDigest *state, char hex[P4_CONTENT_DIGEST_HEX+1]);
void p4_content_digest(const void *data, size_t size, char hex[P4_CONTENT_DIGEST_HEX+1]);

/* A complete nonempty source image, held alongside its actual retained
 * selected file plan and provider by the consumer. Seal only bytes obtained by
 * a successful complete read from that plan. This is content equivalence,
 * not a claim about which subsequent read produced an equal guest buffer. */
typedef struct P4ContentFingerprint {
    size_t size;
    char hex[P4_CONTENT_DIGEST_HEX+1];
} P4ContentFingerprint;
/* 0 on success; invalid arguments return -1 without modifying *out. */
int p4_content_fingerprint_seal(P4ContentFingerprint *out, const void *bytes, size_t size);
/* 1 only when the complete, live readable buffer has exactly the sealed size
 * and SHA-256; 0 for absent, invalid, partial, or different content. No I/O,
 * allocations, cached guest pointers, or semantic-use side effects. */
int p4_content_fingerprint_matches(const P4ContentFingerprint *expected,
                                   const void *bytes, size_t size);

#ifdef __cplusplus
}
#endif
#endif
