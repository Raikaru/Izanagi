#ifndef P4PC_PLATFORM_FILES_ISO9660_H
#define P4PC_PLATFORM_FILES_ISO9660_H

#include <stdio.h>

/* Called once per file of an ISO 9660 volume. `path` is upper-case, '/'-separated,
 * relative to the volume root, without the ";1" version suffix. `offset` is the
 * absolute byte offset of the file data in the file that was walked. */
typedef void (*iso_visit_fn)(void *ctx, const char *path, long long offset, long long size);

/* Walks the ISO 9660 volume that starts at byte `base` of `fp`. Returns 0, or -1 if
 * there is no ISO 9660 primary volume descriptor there or a directory cannot be read. */
int iso_walk(FILE *fp, long long base, iso_visit_fn visit, void *ctx);
/* Reads exactly n bytes at offset: interrupted and partial transfers continue. Returns 0, or -1
 * (errno set) on an I/O error or an end of file inside the requested range. Every disc and
 * container read goes through it, so a short host transfer never looks like the end of a file. */
int iso_read_at(FILE *fp, long long offset, void *dst, size_t n);

#endif
