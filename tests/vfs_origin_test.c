#include "vfs.h"
#include "p4pc/os.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "vfs_origin: FAIL line %d: %s\n", __LINE__, #x); failures++; } } while (0)
typedef struct Observation {
    unsigned calls;
    P4VfsOriginKind kind;
    uint32_t rank;
    char provider[64], key[520];
    const void *bytes;
    size_t size;
    int64_t file_offset, disc_offset;
    int has_key;
} Observation;
static void observe(const P4VfsOrigin *origin, const void *bytes, size_t size,
    int64_t file_offset, int64_t disc_offset, void *user)
{
    Observation *out = user;
    out->calls++;
    out->kind = origin->kind; out->rank = origin->rank;
    snprintf(out->provider, sizeof out->provider, "%s", origin->provider);
    out->has_key = origin->key != NULL;
    snprintf(out->key, sizeof out->key, "%s", origin->key ? origin->key : "");
    out->bytes = bytes; out->size = size;
    out->file_offset = file_offset; out->disc_offset = disc_offset;
}
static void unexpected(const P4VfsOrigin *origin, const void *bytes, size_t size,
    int64_t file_offset, int64_t disc_offset, void *user)
{
    (void)origin; (void)bytes; (void)size; (void)file_offset; (void)disc_offset; (void)user;
    CHECK(0 && "a conflicting observer replaced its owner");
}
static int opened(const char *path, P4VfsOriginKind kind, uint32_t rank, const char *provider, const char *key)
{
    int fd = vfs_open(path, 0);
    CHECK(fd >= 0);
    if (fd >= 0) {
        P4VfsOrigin origin;
        int result = vfs_origin(fd, &origin);
        CHECK(result == 0);
        if (result) return fd;
        CHECK(origin.kind == kind && origin.rank == rank);
        CHECK(!strcmp(origin.provider, provider) && !strcmp(origin.key, key));
        const char *stable = origin.key;
        CHECK(vfs_origin(fd, &origin) == 0 && origin.key == stable);
    }
    return fd;
}
int main(int argc, char **argv)
{
    p4_os_init();
    if (argc != 3) return 2;
    char low[1200], high[1200], developer[1200];
    snprintf(low, sizeof low, "%s/low", argv[2]);
    snprintf(high, sizeof high, "%s/high", argv[2]);
    snprintf(developer, sizeof developer, "%s/developer", argv[2]);
    CHECK(vfs_override_register(low, NULL, NULL, NULL) == -1);
    CHECK(vfs_override_register(low, "", NULL, NULL) == -1);
    CHECK(vfs_override_register(low, "developer", NULL, NULL) == 0);
    CHECK(vfs_override_register(high, "layer.high", NULL, NULL) == 0);
    CHECK(vfs_mount("not-an-iso", developer) == -1);
    CHECK(vfs_mount(argv[1], developer) == 0);
    CHECK(vfs_override_register(low, "late", NULL, NULL) == -1);
    P4VfsOrigin invalid;
    CHECK(vfs_origin(-1, &invalid) == -1 && vfs_origin(999, &invalid) == -1);
    int lower = opened("DATA/LOW.TXT", P4_VFS_MOD, 1, "developer", "DATA/LOW.TXT");
    int replaced = opened("PLAIN.TXT", P4_VFS_MOD, 2, "layer.high", "DATA/PLAIN.TXT");
    int explicit = opened("DATA/PLAIN.TXT", P4_VFS_MOD, 2, "layer.high", "DATA/PLAIN.TXT");
    int highest = opened("DATA/CLASH.TXT", P4_VFS_DEVELOPER, 3, "developer", "DATA/CLASH.TXT");
    int original = opened("BARE.TXT", P4_VFS_DISC, 0, "disc", "DATA/BARE.TXT");
    int root = opened("NOTE.TXT", P4_VFS_DISC, 0, "disc", "NOTE.TXT");
    char bytes[32] = {0};
    CHECK(vfs_read(replaced, bytes, sizeof bytes) == 8 && !memcmp(bytes, "mod-data", 8));
    int directory; long long size, offset;
    CHECK(vfs_stat("PLAIN.TXT", &directory, &size) == 0 && !directory && size == 8);
    CHECK(vfs_disc_locate("DATA/PLAIN.TXT", &offset, &size) == 0);
    long long extent; int container;
    CHECK(vfs_disc_extent(replaced, &extent, &container) == 0 && extent == offset && container == 0);
    CHECK(vfs_is_override(replaced) && !vfs_is_override(original));
    Observation observation = {0}, other = {0};
    CHECK(vfs_read_observer_register(NULL, &observation) == -1);
    CHECK(vfs_read_observer_register(observe, &observation) == 0);
    CHECK(vfs_read_observer_register(observe, &observation) == 0);
    CHECK(vfs_read_observer_register(observe, &other) == -1);
    CHECK(vfs_read_observer_register(unexpected, &observation) == -1);
    CHECK(vfs_read_observer_remove(observe, &other) == -1);
    CHECK(vfs_seek(replaced, 2, 0) == 2 && vfs_read(replaced, bytes, 3) == 3);
    CHECK(observation.calls == 1 && observation.bytes == bytes && observation.size == 3);
    CHECK(!memcmp(bytes, "d-d", 3) && observation.kind == P4_VFS_MOD && observation.rank == 2);
    CHECK(!strcmp(observation.provider, "layer.high") && !strcmp(observation.key, "DATA/PLAIN.TXT"));
    CHECK(observation.file_offset == 2 && observation.disc_offset == -1);
    CHECK(vfs_read(original, bytes, 4) == 4);
    CHECK(vfs_disc_locate("DATA/BARE.TXT", &offset, &size) == 0);
    CHECK(observation.calls == 2 && observation.kind == P4_VFS_DISC && observation.rank == 0);
    CHECK(observation.file_offset == 0 && observation.disc_offset == offset);
    CHECK(!strcmp(observation.key, "DATA/BARE.TXT") && observation.size == 4);
    unsigned char sector[CD_SECTOR];
    CHECK(vfs_disc_read_sectors((unsigned)(offset / CD_SECTOR), 1, sector) == 0);
    CHECK(observation.calls == 3 && observation.bytes == sector && observation.size == CD_SECTOR);
    CHECK(!observation.has_key && observation.file_offset == -1 && observation.disc_offset == offset);
    CHECK(!memcmp(sector, "disc-bare", 9));
    CHECK(vfs_read(original, bytes, 0) == 0 && vfs_read(-1, bytes, 1) == -1);
    CHECK(vfs_read(original, bytes, -1) == -1 && observation.calls == 3);
    CHECK(vfs_disc_read_sectors(0x7fffffffu, 1, sector) == -1 && observation.calls == 3);
    CHECK(vfs_read_observer_remove(observe, &observation) == 0);
    CHECK(vfs_read(original, bytes, 1) == 1 && observation.calls == 3);
    CHECK(vfs_read_observer_remove(observe, &observation) == -1);
    CHECK(vfs_read_observer_register(observe, &other) == 0);
    CHECK(vfs_read(root, bytes, 1) == 1 && other.calls == 1);
    CHECK(vfs_read_observer_remove(observe, &other) == 0);
    int descriptors[] = {lower, replaced, explicit, highest, original, root};
    for (size_t i = 0; i < sizeof descriptors / sizeof descriptors[0]; i++) {
        CHECK(vfs_close(descriptors[i]) == 0);
        CHECK(vfs_origin(descriptors[i], &invalid) == -1);
    }
    printf("vfs_origin: %s (providers, ranks, canonical aliases, extents, observed bytes/offsets, exclusive observer)\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
