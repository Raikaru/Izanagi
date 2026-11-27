#ifndef P4_GOLDEN_PE_DATA_H
#define P4_GOLDEN_PE_DATA_H
#include <stddef.h>
#include <stdint.h>
/* Reads only named initialized non-executable file-backed sections after
 * streaming the complete executable for its exact allowlisted identity. */
int p4_golden_pe_data(const char *path,const char *expected_exe,
    const char *const *names,size_t name_count,int virtual_extent,
    int (*visit)(const uint8_t *,size_t,void *,char *,size_t),void *context,
    char exe_digest[65],char *reason,size_t capacity);
#endif
