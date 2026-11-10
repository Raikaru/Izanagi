#ifndef P4PC_MOD_MEMORY_H
#define P4PC_MOD_MEMORY_H
#include <stddef.h>
#include <stdint.h>

/* Defence in depth for trusted, opt-in mods, not hostile-code isolation.
 * These declarations are native-owner capabilities, never exposed to scripts.
 * A readonly/host-data overlap takes precedence over every writable span. */
enum {
    P4_MOD_MEMORY_READ = 1u,
    P4_MOD_MEMORY_WRITE = 2u,
    P4_MOD_MEMORY_HOST_DATA = 4u,
    P4_MOD_MEMORY_FUNCTION_WORDS = 8u
};
typedef struct P4ModMemoryRegion P4ModMemoryRegion;
P4ModMemoryRegion *p4_mod_memory_declare(uint32_t address, size_t size, unsigned flags);
void p4_mod_memory_revoke(P4ModMemoryRegion *region);

/* NULL permits the access; otherwise returns a static refusal diagnostic.
 * owned_scratch is set only after the binding proves ownership of the complete
 * span. It does not bypass code/stack/host-data/readonly protection. For writes,
 * bytes is the entire proposed payload; validation completes before any store.
 * Known function words require complete, validated four-byte replacements. */
const char *p4_mod_memory_check(uint32_t address, size_t size, int write,
                              int owned_scratch, const void *bytes);
/* Maps a retail entry to the ABI's stored guest function word. Unknown entries
 * are refused. NULL function words are valid; arbitrary host addresses are not. */
int p4_mod_memory_function_encode(uint32_t retail_address, uint32_t *word);
int p4_mod_memory_function_decode(uint32_t word, uint32_t *retail_address);
int p4_mod_memory_function_known(uint32_t word);
/* Releases cached dispatch indices only. Region lifetimes remain with owners. */
void p4_mod_memory_clear(void);
#endif
