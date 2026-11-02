#include "package_memory_internal.h"
#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__ANDROID__) || defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
#define DEFAULT_LIMIT (16u * 1024u * 1024u)
#else
#define DEFAULT_LIMIT (32u * 1024u * 1024u)
#endif
static atomic_flag lock = ATOMIC_FLAG_INIT;
static P4PackageMemoryUsage usage = {.limit = DEFAULT_LIMIT};
static int started;
static void acquire(void)
{
    while (atomic_flag_test_and_set_explicit(&lock, memory_order_acquire)) {}
}
static void unlock(void) { atomic_flag_clear_explicit(&lock, memory_order_release); }
static void imbalance(const char *operation)
{
    fprintf(stderr, "p4pc: native package metadata accounting imbalance: %s\n", operation);
    abort();
}
int p4_packages_memory_configure(size_t limit)
{
    acquire();
    if (started) { unlock(); return -1; }
    usage.limit = limit;
    unlock();
    return 0;
}
P4PackageMemoryUsage p4_packages_memory_usage(void)
{
    acquire();
    P4PackageMemoryUsage result = usage;
    unlock();
    return result;
}
int p4_packages_memory_reserve(size_t bytes)
{
    if (!bytes) return 0;
    acquire();
    size_t total = usage.resident + usage.reserved;
    if (bytes > usage.limit - total) { unlock(); return -1; }
    started = 1;
    usage.reserved += bytes;
    total += bytes;
    if (usage.peak < total) usage.peak = total;
    unlock();
    return 0;
}
void p4_packages_memory_commit(size_t bytes)
{
    if (!bytes) return;
    acquire();
    if (bytes > usage.reserved) { unlock(); imbalance("commit"); }
    usage.reserved -= bytes;
    usage.resident += bytes;
    unlock();
}
void p4_packages_memory_cancel(size_t bytes)
{
    if (!bytes) return;
    acquire();
    if (bytes > usage.reserved) { unlock(); imbalance("cancel"); }
    usage.reserved -= bytes;
    unlock();
}
void p4_packages_memory_release(size_t bytes)
{
    if (!bytes) return;
    acquire();
    if (bytes > usage.resident) { unlock(); imbalance("release"); }
    usage.resident -= bytes;
    unlock();
}
static int reserve_allocation(size_t bytes)
{
    if (p4_packages_memory_reserve(bytes) == 0) return 0;
    P4PackageMemoryUsage snapshot = p4_packages_memory_usage();
    fprintf(stderr, "p4pc: package/native metadata budget exhausted: request=%zu resident=%zu reserved=%zu limit=%zu peak=%zu\n",
        bytes, snapshot.resident, snapshot.reserved, snapshot.limit, snapshot.peak);
    errno = ENOMEM;
    return -1;
}
void *p4_package_metadata_alloc(size_t bytes, int clear)
{
    if (!bytes || reserve_allocation(bytes)) return NULL;
    void *allocation = clear ? calloc(1, bytes) : malloc(bytes);
    if (allocation) p4_packages_memory_commit(bytes);
    else p4_packages_memory_cancel(bytes);
    return allocation;
}
void *p4_package_metadata_resize(void *allocation, size_t old_bytes, size_t new_bytes)
{
    int replacing = allocation != NULL;
    if (!new_bytes || reserve_allocation(new_bytes)) return NULL;
    void *replacement = realloc(allocation, new_bytes);
    if (!replacement) { p4_packages_memory_cancel(new_bytes); return NULL; }
    p4_packages_memory_commit(new_bytes);
    if (replacing) p4_packages_memory_release(old_bytes);
    return replacement;
}
void p4_package_metadata_free(void *allocation, size_t bytes)
{
    if (!allocation) return;
    free(allocation);
    p4_packages_memory_release(bytes);
}
