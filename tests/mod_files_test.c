#include "p4pc/mods.h"
#include "vfs.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    if (argc != 5) return 2;
#ifdef _WIN32
    puts("filesystem=win32");
#else
    puts("filesystem=posix");
#endif
    int rc = p4_mods_stage(argv[1], argv[2]);
    if (!rc) rc = p4_package_registry_publish();
    printf("activation=%d count=%zu conflicts=%zu\n", rc, p4_mods_count(), p4_mods_file_conflicts());
    if (rc) return p4_mods_count() ? 3 : 0;
    for (size_t i = 0; i < p4_mods_count(); i++) printf("mod=%s\n", p4_mods_info(i)->id);
    if (p4_mods_info(p4_mods_count())) return 4;
    if (vfs_mount(NULL, strcmp(argv[3], "-") ? argv[3] : NULL)) return 5;
    if (!p4_mods_count()) p4_mods_assert_disabled();
    if (vfs_open("../ESCAPE", 0) >= 0 || vfs_open(argv[4], 1) >= 0) return 6;
    int fd = vfs_open(argv[4], 0), is_dir;
    long long size;
    if (fd >= 0) {
        char text[128];
        int n = vfs_read(fd, text, sizeof text - 1);
        if (n < 0 || !vfs_is_override(fd) || vfs_stat(argv[4], &is_dir, &size) || is_dir || size != vfs_seek(fd, 0, 2)) return 7;
        text[n] = 0;
        printf("file=%s size=%lld\n", text, size);
        vfs_close(fd);
    } else printf("file=missing\n");
    int dh = vfs_dir_open("DATA");
    if (dh >= 0) {
        char name[256];
        while ((rc = vfs_dir_next(dh, name, sizeof name, &is_dir, &size)) > 0)
            printf("entry=%s dir=%d size=%lld\n", name, is_dir, size);
        if (rc < 0) return 8;
        vfs_dir_close(dh);
    }
    return 0;
}
