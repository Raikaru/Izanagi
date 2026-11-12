#ifndef P4PC_PACKAGE_MEMORY_INTERNAL_H
#define P4PC_PACKAGE_MEMORY_INTERNAL_H
#include "p4pc/package_memory.h"
void *p4_package_metadata_alloc(size_t bytes, int clear);
void *p4_package_metadata_resize(void *allocation, size_t old_bytes, size_t new_bytes);
void p4_package_metadata_free(void *allocation, size_t bytes);
#endif
