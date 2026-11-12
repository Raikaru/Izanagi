#ifndef P4PC_PACKAGE_MEMORY_H
#define P4PC_PACKAGE_MEMORY_H
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Shared package/native-resolver owned metadata, not payload/decode buffers,
 * opaque C-runtime/OS allocations, VM heaps or baseline thread stacks. Counts
 * requested allocation bytes; borrowed inventories must not be charged twice. */
typedef struct P4PackageMemoryUsage {
    size_t limit, resident, reserved, peak; /* peak=max(resident+reserved) */
} P4PackageMemoryUsage;
/* Before first successful nonzero reserve. Defaults:32MiB desktop,16MiB mobile. */
int p4_packages_memory_configure(size_t limit);
P4PackageMemoryUsage p4_packages_memory_usage(void);
/* Reserve BEFORE allocation, commit only on success, cancel on failure. A grow
 * reserves the complete replacement allocation while the old bytes stay live;
 * release old bytes only after success. Publish must need no further reserve.
 * Native owners keep committed charges until their last queued/pinned user frees
 * the allocation, including retired generations. All operations are thread-safe.
 * Unbalanced commit/cancel/release is a native programming error and fail-stops. */
int p4_packages_memory_reserve(size_t bytes);
void p4_packages_memory_commit(size_t bytes);
void p4_packages_memory_cancel(size_t bytes);
void p4_packages_memory_release(size_t bytes);
#ifdef __cplusplus
}
#endif
#endif
