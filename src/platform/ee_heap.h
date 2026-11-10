#ifndef P4PC_EE_HEAP_H
#define P4PC_EE_HEAP_H
#include <stdint.h>

/* All addresses, sizes, chunk headers and links have EE widths, even on LP64. */
uint32_t p4pc_ee_heap_malloc(uint32_t reent, uint32_t size);
void p4pc_ee_heap_free(uint32_t reent, uint32_t pointer);
uint32_t p4pc_ee_heap_calloc(uint32_t reent, uint32_t count, uint32_t size);
uint32_t p4pc_ee_heap_realloc(uint32_t reent, uint32_t pointer, uint32_t size);
/* The only OS allocation boundary: original sbrk semantics over mapped EE RAM. */
uint64_t p4pc_ee_heap_sbrk(int32_t increment);
#endif
