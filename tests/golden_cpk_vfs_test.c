/* Synthetic archives only; driver owns and cleans up the fixture directory. */
#include "../src/platform/golden/golden_cpk_vfs.h"
#include "../src/platform/files/vfs.h"
#include "p4pc/content_digest.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "golden_cpk_vfs:%d: %s\n", __LINE__, #x); return 1; } } while (0)
const char *p4cfg_get(const char *key) { (void)key; return NULL; }

int main(int argc, char **argv)
{
    char bytes[65536], hash[65];
    P4VfsOrigin origin;
    long long size;
    int is_dir;
    CHECK(argc == 2 || argc == 3 || argc == 5);
    if (argc == 5) {
        P4ContentDigest digest;
        CHECK(!strcmp(argv[2], "steam"));
        CHECK(!p4_golden_cpk_vfs_register(argv[1], 0) && !vfs_mount(NULL, NULL));
        int sample = vfs_open(argv[3], 0);
        CHECK(sample >= 0 && !vfs_origin(sample, &origin) && origin.kind == P4_VFS_GOLDEN);
        p4_content_digest_begin(&digest);
        int n;
        while ((n = vfs_read(sample, bytes, sizeof bytes)) > 0)
            p4_content_digest_update(&digest, bytes, (size_t)n);
        CHECK(n == 0);
        p4_content_digest_end(&digest, hash);
        CHECK(!strcmp(hash, argv[4]) && !strcmp(origin.sha256, argv[4]));
        CHECK(!vfs_close(sample));
        return 0;
    }
    if (argc == 3) {
        CHECK(!strcmp(argv[2], "dormant"));
        CHECK(!p4_golden_cpk_vfs_register_dormant(argv[1]));
        CHECK(!vfs_mount(NULL, argv[1]));
        int retail = vfs_open("foo.bin", 0);
        CHECK(retail >= 0 && vfs_read(retail, bytes, sizeof bytes) == 6 && !memcmp(bytes, "retail", 6));
        CHECK(!vfs_origin(retail, &origin) && origin.kind == P4_VFS_DEVELOPER);
        CHECK(!vfs_close(retail));
        puts("golden_cpk_vfs dormant: ok");
        return 0;
    }
    CHECK(!p4_golden_cpk_vfs_register(argv[1], 0));
    CHECK(!vfs_mount(NULL, NULL));
    int fd = vfs_open("DATA/foo.bin", 0);
    CHECK(fd >= 0 && vfs_read(fd, bytes, sizeof bytes) == 6 && !memcmp(bytes, "locale", 6));
    CHECK(!vfs_origin(fd, &origin) && origin.kind == P4_VFS_GOLDEN);
    p4_content_digest("locale", 6, hash);
    CHECK(origin.sha256 && !strcmp(origin.sha256, hash));
    CHECK(origin.chain && !strcmp(origin.chain, "data_e.cpk::FOO.BIN"));
    CHECK(vfs_is_override(fd) && !vfs_close(fd));
    fd = vfs_open("data/stuff.pack/inner.bin", 0);
    CHECK(fd >= 0 && vfs_read(fd, bytes, sizeof bytes) == 5 && !memcmp(bytes, "inner", 5));
    CHECK(!vfs_close(fd));
    fd = vfs_open("stuff.pack/inner.pack/deep.bin", 0);
    CHECK(fd >= 0 && vfs_read(fd, bytes, sizeof bytes) == 4 && !memcmp(bytes, "deep", 4));
    CHECK(!vfs_close(fd));
    fd = vfs_open("compressed.bin", 0);
    CHECK(fd >= 0 && vfs_read(fd, bytes, sizeof bytes) == 300);
    for (int i = 0; i < 300; ++i) CHECK(bytes[i] == 'x');
    CHECK(!vfs_close(fd));
    CHECK(vfs_open("missing.bin", 0) == -1 && vfs_open("foo.bin", 1) == -1);
    CHECK(!vfs_stat("data/foo.bin", &is_dir, &size) && !is_dir && size == 6);
    CHECK(!vfs_stat("data", &is_dir, &size) && is_dir);
    int dir = vfs_dir_open("data");
    CHECK(dir >= 0);
    int seen = 0;
    while (vfs_dir_next(dir, bytes, sizeof bytes, &is_dir, &size) > 0)
        if (!strcmp(bytes, "FOO.BIN")) seen = 1;
    CHECK(seen && !vfs_dir_close(dir));
    puts("golden_cpk_vfs: ok");
    return 0;
}
