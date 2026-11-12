#ifndef P4PC_VFS_SOURCE_H
#define P4PC_VFS_SOURCE_H
#include "p4pc/os.h"
#include "p4pc/vfs_origin.h"
/* Host/game-thread only, same ownership as VFS. Preparation captures the actual
 * selected P4PC_ISO/developer configuration and indexes the ONE disc reader.
 * It neither mounts/exposes override layers nor consumes candidate package
 * authority. Later sceCdInit uses this same selection/index without a rewalk.
 * No demand means the original mount path does no new allocation/stat work. */
int vfs_source_prepare_selected(void);
const char *vfs_source_selected_iso(void);
int vfs_source_mount_selected(void);
/*0 actual selected disc identity,1 no disc,-1 error; metadata is not authority. */
int vfs_source_disc_identity(P4OSFileIdentity *identity);
typedef struct P4VfsSourcePlan P4VfsSourcePlan;
typedef struct P4VfsSourceInfo {
    P4VfsOriginKind kind;
    const char *key; /* Same canonical container-qualified VFS key. */
    uint64_t offset, size;
    P4OSFileIdentity file; /* Actual opened object's revision, not path guessing. */
} P4VfsSourceInfo;
/* Exact DISC or DEVELOPER layer only:0 acquired,1 absent,-1 refused/error.
 * No MOD fallback: candidate replacements require their real provider plans.
 * Resolver checks developer first, candidate packages next, disc last; this
 * explicit-layer API never silently changes that precedence. *out startsNULL.
 * Acquisition is pre-mount only. Plans retain their actual stream/index key;
 * no new filesystem lookup or source adoption happens during read_at. */
int vfs_source_plan_acquire(P4VfsOriginKind layer, const char *path, P4VfsSourcePlan **out);
const P4VfsSourceInfo *vfs_source_plan_info(const P4VfsSourcePlan *plan);
int vfs_source_plan_retain(P4VfsSourcePlan *plan);
void vfs_source_plan_release(P4VfsSourcePlan *plan);
/* Same physical read and ONE existing observer as vfs_read. Exact bounds and
 * before/after object revision checks; no allocation/copy, second reader,
 * observer, worker VFS access or late source re-decode. */
int vfs_source_plan_read_at(P4VfsSourcePlan *plan, uint64_t offset, void *bytes, size_t size);
#endif
