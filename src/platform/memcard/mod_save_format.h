#ifndef P4PC_MOD_SAVE_FORMAT_H
#define P4PC_MOD_SAVE_FORMAT_H
#include <stddef.h>
#include <stdint.h>
#define P4_MOD_SAVE_MAX_BYTES (64u * 1024u * 1024u)
#define P4_MOD_SAVE_MAX_BLOCKS 1024u
#define P4_MOD_SAVE_MAX_REQUIREMENTS 4096u
#define P4_MOD_SAVE_MAX_HISTORY 4096u
#define P4_MOD_SAVE_HEADER_BYTES 96u
#define P4_MOD_SAVE_WITNESS 1u
#define P4_MOD_SAVE_MARKER_BYTES 128u
#define P4_MOD_SAVE_SIDECAR ".p4mods"
#define P4_MOD_SAVE_COMMIT ".p4mods-commit"
/* Pure integrity/format binding, also used by offline import. It deliberately
 * does not pretend that required runtime providers/mods are currently present. */
int p4_save_sidecar_verify(uint64_t retail_size,const unsigned char retail_hash[32],
    const void *sidecar,size_t sidecar_size,char *error,size_t capacity);
void p4_save_marker_make(uint64_t retail_size,const unsigned char retail_hash[32],
    uint64_t sidecar_size,const unsigned char sidecar_hash[32],
    unsigned char marker[P4_MOD_SAVE_MARKER_BYTES]);
int p4_save_marker_verify(uint64_t retail_size,const unsigned char retail_hash[32],
    uint64_t sidecar_size,const unsigned char sidecar_hash[32],
    const void *marker,size_t marker_size,char *error,size_t capacity);
#endif
