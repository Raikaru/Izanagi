#ifndef P4PC_MOD_SAVE_HASH_H
#define P4PC_MOD_SAVE_HASH_H
#include <stddef.h>
#include <stdint.h>
typedef struct P4SaveHash {
    uint32_t state[8];
    uint64_t bytes;
    size_t used;
    unsigned char block[64];
} P4SaveHash;
void p4_save_hash_init(P4SaveHash *);
void p4_save_hash_update(P4SaveHash *, const void *, size_t);
void p4_save_hash_final(P4SaveHash *, unsigned char digest[32]);
void p4_save_hash(const void *, size_t, unsigned char digest[32]);
#endif
