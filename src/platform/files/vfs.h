#ifndef P4PC_PLATFORM_FILES_VFS_H
#define P4PC_PLATFORM_FILES_VFS_H

#include <stddef.h>
#include "p4pc/vfs_origin.h"

/* Bytes per disc sector (CD-ROM mode 1 user data). */
#define CD_SECTOR 2048

/* Virtual file tree behind the PS2 file API. Path rules and lookup order are in vfs.c. */

/* Indexes the disc image (its five CVM containers, each with an embedded ISO 9660
 * volume) and the override directory. Either argument may be NULL: no disc, or no
 * overrides. Returns 0 once mounted (later calls return 0 without re-reading), or -1
 * when iso_path cannot be opened or is not a complete disc. */
int vfs_mount(const char *iso_path, const char *override_dir);

/* Adds a read-only file-mod layer before mount; later layers win. The developer
 * override_dir passed to mount wins over every registered layer. An optional
 * immutable index maps canonical keys to original-cased relative host names
 * and enumerates immediate children. NULL names mean absent; list returns 0
 * for a directory, 1 for absent/not-directory, -1 for a visitor/error failure.
 * Relative names and user remain alive for the mount; paths are prevalidated.
 * NULL index keeps the developer-style uppercase filesystem lookup.
 * Registration is atomic: invalid/full requests leave the list unchanged. */
typedef const char *(*P4VfsPathResolver)(const char *canonical, void *user);
typedef int (*P4VfsPathVisitor)(const char *relative, int directory, void *user);
typedef struct P4VfsPathIndex {
    P4VfsPathResolver resolve;
    int (*list)(const char *canonical_directory, P4VfsPathVisitor visitor,
                void *visitor_user, void *index_user);
} P4VfsPathIndex;
int vfs_override_register(const char *root, const char *provider,
                          const P4VfsPathIndex *index, void *user);
/* Rolls back pre-mount registrations (used when mod activation fails). */
void vfs_override_clear(void);

/* Opens a PS2 path for reading. Returns a descriptor >= 0, or -1 if the path is not
 * found, the tree is not mounted, or `writable` is set (the file system is read-only). */
int vfs_open(const char *path, int writable);
/* Reads n bytes at the current position, fewer only where the file ends. Returns the count
 * (0 at end of file), or -1 for a bad descriptor or a host read error; a short host transfer
 * is continued and never reported as the end of the file. */
int vfs_read(int fd, void *dst, int n);
/* Moves the position (SEEK_SET 0, SEEK_CUR 1, SEEK_END 2). Returns the new position or -1. */
long long vfs_seek(int fd, long long offset, int whence);
int vfs_close(int fd);
/* Disc-only lookup for the CD drive model (override files are ignored). Returns 0 and
 * the absolute byte offset and size of the file on the disc, or -1 if the path is not on
 * the disc or no disc is mounted. */
int vfs_disc_locate(const char *path, long long *offset, long long *size);
/* Constant-time physical extent of an open file. Overrides retain their original
 * disc extent; override-only files have no DVD location and return -1. Container
 * IDs are DATA0, BTL1, BGM2, ENV3, ZZZZZ4, or -1 for a disc-root file. */
int vfs_disc_extent(int fd, long long *offset, int *container);
/* Whether an open descriptor reads a host override rather than the disc. */
int vfs_is_override(int fd);
/* Reads `sectors` whole 2048-byte sectors starting at disc sector `lsn` into dst. Returns
 * 0, or -1 if no disc is mounted or the read is short. */
int vfs_disc_read_sectors(unsigned int lsn, unsigned int sectors, void *dst);
/* Opens a directory listing over the merged tree: each disc child (container labels as
 * directories, disc root files) and each override child, one entry per name, override
 * winning on a name clash. `path` is "" or a device root for the top. Returns a handle >= 0,
 * or -1 if the path names no directory in either layer. */
int vfs_dir_open(const char *path);
/* Copies the next entry into `name` (NUL-terminated, at most cap bytes) and sets *is_dir
 * and *size. Returns 1 for an entry, 0 at the end, or -1 for a bad handle. */
int vfs_dir_next(int dh, char *name, size_t cap, int *is_dir, long long *size);
int vfs_dir_close(int dh);
/* Stat over both layers, with the same precedence as vfs_open. Returns 0 and fills the
 * outputs, or -1 if the path names neither a file nor a directory. */
int vfs_stat(const char *path, int *is_dir, long long *size);

#endif
