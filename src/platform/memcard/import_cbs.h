#ifndef P4PC_IMPORT_CBS_H
#define P4PC_IMPORT_CBS_H
#include "p4pc/save_import.h"
/* The caller supplies a zeroed archive, and frees each files[i].data even on
 * failure. No partial archive is exposed on failure. Input is never modified.
 * CBS framing and zlib Adler-32 are checked; no additional CBS CRC exists. */
int p4save_decode_cbs(const unsigned char *blob, size_t size,
                      P4SaveArchive *out, char *error, size_t cap);
#endif
