/* Metadata for the already-selected VFS provider; no package resolution here. */
#ifndef P4PC_VFS_ORIGIN_H
#define P4PC_VFS_ORIGIN_H
#include <stddef.h>
#include <stdint.h>

typedef enum P4VfsOriginKind {
    P4_VFS_DISC = 0,
    P4_VFS_MOD = 1,
    P4_VFS_DEVELOPER = 2
} P4VfsOriginKind;

typedef struct P4VfsOrigin {
    P4VfsOriginKind kind;
    uint32_t rank;          /* disc 0, enabled mods 1..N, developer N+1 */
    const char *provider;   /* "disc", manifest id, or "developer" */
    const char *key;        /* uppercase container-qualified key, or NULL for raw sectors */
} P4VfsOrigin;

/* Returns 0, or -1 for an invalid descriptor. Strings are immutable until the
 * descriptor closes (unmount also closes descriptors). The caller copies any
 * retained metadata; this function never allocates. */
int vfs_origin(int fd, P4VfsOrigin *out);

/* Called after each successful nonempty read with exactly the initialized bytes.
 * file_offset is -1 for raw sector reads; disc_offset is -1 for host overrides.
 * Origin itself is callback-scoped. Raw sectors can span files: their key is NULL,
 * not a guessed path. Do not re-enter VFS or mutate the bytes from this callback.
 * All registration and VFS calls follow the existing game-thread ownership. */
typedef void (*P4VfsReadObserver)(const P4VfsOrigin *origin, const void *bytes,
    size_t size, int64_t file_offset, int64_t disc_offset, void *user);

/* One native observer. Repeating the same pair succeeds; a different occupied
 * registration returns -1 without replacing it. Removal must match both values.
 * No observer means one null branch, no metadata lookup/hash/copy/allocation. */
int vfs_read_observer_register(P4VfsReadObserver observer, void *user);
int vfs_read_observer_remove(P4VfsReadObserver observer, void *user);
#endif
