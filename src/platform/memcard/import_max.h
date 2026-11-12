#ifndef P4PC_IMPORT_MAX_H
#define P4PC_IMPORT_MAX_H

#include "p4pc/save_import.h"

/* Decode an Action Replay MAX container. out must initially be zeroed.
 * Each successful file allocation is recorded in out->count even on failure;
 * the caller owns and frees these buffers. No timestamps exist in MAX.
 * Returns 1 on success, 0 with an error message on failure. */
int p4save_decode_max(const unsigned char *blob, size_t size,
                      P4SaveArchive *out, char *error, size_t cap);

#endif
