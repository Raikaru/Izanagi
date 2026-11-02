#include "vfs.h"
#include "iso9660.h"
#include "p4pc/vfs_source.h"
#include "p4pc/content_digest.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned walks, observations;
static P4VfsOriginKind observed_kind;
static uint32_t observed_rank;
static char observed_key[520];
int p4_test_actual_iso_walk(FILE *, long long, iso_visit_fn, void *);
int iso_walk(FILE *stream, long long base, iso_visit_fn visit, void *user)
{
    ++walks;
    return p4_test_actual_iso_walk(stream, base, visit, user);
}
static void observe(const P4VfsOrigin *origin, const void *bytes, size_t size,
    int64_t offset, int64_t disc_offset, void *user)
{
    assert(!user && origin && origin->key && bytes && size && offset == 0);
    assert(origin->kind == P4_VFS_DISC ? disc_offset >= 0 : disc_offset == -1);
    ++observations; observed_kind = origin->kind; observed_rank = origin->rank;
    snprintf(observed_key, sizeof observed_key, "%s", origin->key);
}
static void read_plan(P4VfsSourcePlan *plan, const char *expected, P4VfsOriginKind kind, uint32_t rank)
{
    const P4VfsSourceInfo *info = vfs_source_plan_info(plan);
    assert(info && info->kind == kind && info->size == strlen(expected));
    assert(info->file.size >= info->size);
    char bytes[40] = {0}; unsigned before = observations;
    assert(!vfs_source_plan_read_at(plan, 0, bytes, strlen(expected)));
    assert(!memcmp(bytes, expected, strlen(expected)) && observations == before + 1);
    assert(observed_kind == kind && observed_rank == rank && !strcmp(observed_key, info->key));
    memset(bytes, 0x7e, sizeof bytes);
    assert(vfs_source_plan_read_at(plan, info->size, bytes, 1) == -1 && bytes[0] == 0x7e && observations == before + 1);
    assert(!vfs_source_plan_read_at(plan, info->size, NULL, 0) && observations == before + 1);
}
static void append_byte(const char *path)
{
    FILE *file = fopen(path, "ab"); assert(file);
    assert(fputc('x', file) == 'x' && !fclose(file));
}
int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    p4_os_init();
    const char *mode = argv[1];
    assert(!vfs_source_prepare_selected());
    assert(vfs_open("DATA/COMMON.BIN", 0) == -1); /* No mount or raw exposure. */
    P4OSFileIdentity identity = {.size = 123};
    if (!strcmp(mode, "absent")) {
        assert(!walks && vfs_source_disc_identity(&identity) == 1 && identity.size == 123);
        P4VfsSourcePlan *missing = NULL;
        assert(vfs_source_plan_acquire(P4_VFS_DISC, "DATA/COMMON.BIN", &missing) == 1 && !missing);
        assert(vfs_source_plan_acquire(P4_VFS_DEVELOPER, "DATA/COMMON.BIN", &missing) == 1 && !missing);
        assert(!vfs_source_mount_selected() && !walks);
        puts("vfs_source: absent selection remains explicit, no invented source");
        return 0;
    }
    assert(walks == 7 && !vfs_source_disc_identity(&identity) && identity.size);
    assert(!vfs_source_prepare_selected() && walks == 7);
    assert(!vfs_read_observer_register(observe, NULL));
    P4VfsSourcePlan *data = NULL, *battle = NULL, *developer = NULL, *missing = NULL;
    assert(!vfs_source_plan_acquire(P4_VFS_DISC, "COMMON.BIN", &data));
    assert(!vfs_source_plan_acquire(P4_VFS_DISC, "cdrom0:BTL\\COMMON.BIN;1", &battle));
    assert(!vfs_source_plan_acquire(P4_VFS_DEVELOPER, "COMMON.BIN", &developer));
    assert(!strcmp(vfs_source_plan_info(data)->key, "DATA/COMMON.BIN"));
    assert(!strcmp(vfs_source_plan_info(battle)->key, "BTL/COMMON.BIN"));
    assert(!memcmp(&vfs_source_plan_info(data)->file, &identity, sizeof identity));
    assert(vfs_source_plan_acquire(P4_VFS_MOD, "DATA/COMMON.BIN", &missing) == -1 && !missing);
    assert(vfs_source_plan_acquire(P4_VFS_DISC, "DATA/MISSING.BIN", &missing) == 1 && !missing);
    assert(vfs_source_plan_acquire(P4_VFS_DISC, "DATA/../COMMON.BIN", &missing) == -1 && !missing);
    P4VfsSourcePlan *same = data;
    assert(vfs_source_plan_acquire(P4_VFS_DISC, "DATA/COMMON.BIN", &same) == -1 && same == data);
    assert(!vfs_source_plan_retain(data)); vfs_source_plan_release(data);
    read_plan(data, "data-base", P4_VFS_DISC, 0);
    read_plan(battle, "btl-base", P4_VFS_DISC, 0);
    read_plan(developer, "developer-base", P4_VFS_DEVELOPER, 1);
    if (!strcmp(mode, "disc-change")) {
        append_byte(vfs_source_selected_iso());
        char bytes[4];
        assert(vfs_source_plan_read_at(data, 0, bytes, sizeof bytes) == -1);
        assert(vfs_source_mount_selected() == -1 && walks == 7);
    } else if (!strcmp(mode, "developer-change")) {
        char path[1200]; snprintf(path, sizeof path, "%s/developer/DATA/COMMON.BIN", argv[2]);
        append_byte(path);
        char bytes[4];
        assert(vfs_source_plan_read_at(developer, 0, bytes, sizeof bytes) == -1);
        read_plan(data, "data-base", P4_VFS_DISC, 0);
    } else {
        char path[1200]; snprintf(path, sizeof path, "%s/candidate", argv[2]);
        assert(!vfs_override_register(path, "candidate.fixture", NULL, NULL));
#ifdef _WIN32
        assert(!_putenv_s("P4PC_ISO", "not-the-captured-source"));
        assert(!_putenv_s("P4PC_OVERRIDE_DIR", "not-the-captured-developer"));
#else
        assert(!setenv("P4PC_ISO", "not-the-captured-source", 1));
        assert(!setenv("P4PC_OVERRIDE_DIR", "not-the-captured-developer", 1));
#endif
        assert(!vfs_source_mount_selected() && walks == 7); /* No second ISO walk, lookup or reader. */
        assert(vfs_source_plan_acquire(P4_VFS_DISC, "DATA/COMMON.BIN", &missing) == -1 && !missing);
        int fd = vfs_open("DATA/COMMON.BIN", 0); assert(fd >= 0);
        P4VfsOrigin origin; assert(!vfs_origin(fd, &origin) && origin.kind == P4_VFS_DEVELOPER && origin.rank == 2);
        /* The selected developer plan and the lower disc plan are distinct;
         * consume the actual whole loaded image against the selected content. */
        char selected[40], lower[40], loaded[40];
        const size_t selected_size=(size_t)vfs_source_plan_info(developer)->size;
        const size_t lower_size=(size_t)vfs_source_plan_info(data)->size;
        P4ContentFingerprint selected_digest, lower_digest;
        assert(selected_size<=sizeof selected && lower_size<=sizeof lower);
        assert(!vfs_source_plan_read_at(developer,0,selected,selected_size));
        assert(!vfs_source_plan_read_at(data,0,lower,lower_size));
        assert(!p4_content_fingerprint_seal(&selected_digest,selected,selected_size));
        assert(!p4_content_fingerprint_seal(&lower_digest,lower,lower_size));
        assert(vfs_read(fd,loaded,sizeof loaded)==(int)selected_size);
        assert(p4_content_fingerprint_matches(&selected_digest,loaded,selected_size));
        assert(!p4_content_fingerprint_matches(&lower_digest,loaded,selected_size));
        loaded[0]^=1;
        assert(!p4_content_fingerprint_matches(&selected_digest,loaded,selected_size));
        loaded[0]^=1;
        assert(!p4_content_fingerprint_matches(&selected_digest,loaded,selected_size-1));
        assert(!vfs_close(fd));
        fd = vfs_open("DATA/MOD.BIN", 0); assert(fd >= 0);
        assert(!vfs_origin(fd, &origin) && origin.kind == P4_VFS_MOD && !strcmp(origin.provider, "candidate.fixture"));
        assert(!vfs_close(fd));
        read_plan(developer, "developer-base", P4_VFS_DEVELOPER, 2);
        read_plan(battle, "btl-base", P4_VFS_DISC, 0);
    }
    assert(walks == 7);
    vfs_source_plan_release(data); vfs_source_plan_release(battle); vfs_source_plan_release(developer);
    assert(!vfs_read_observer_remove(observe, NULL));
    puts("vfs_source: actual selected immutable plans, canonical containers, one observer/reader/index and later mount reuse passed");
    return 0;
}
