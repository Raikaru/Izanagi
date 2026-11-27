#ifndef P4PC_GOLDEN_LEVELS_H
#define P4PC_GOLDEN_LEVELS_H
#include <stddef.h>
#include <stdint.h>
#define P4_GOLDEN_LEVEL_BYTES (99u * 44u)
/* Pinned plaintext PE data only; no game code is decoded or executed. */
int p4_golden_levels_read(const char *path,uint8_t out[P4_GOLDEN_LEVEL_BYTES],
    char exe_digest[65],char *reason,size_t capacity);
int p4_golden_levels_pair(const uint8_t *table,size_t size,int level,int id,
    uint16_t *hp,uint16_t *sp);
#ifdef P4_GOLDEN_LEVELS_TEST
int p4_golden_levels_test_read(const char *path,uint8_t *out,char exe_digest[65],
    const char *exe,const char *table,char *reason,size_t capacity);
#endif
#endif
