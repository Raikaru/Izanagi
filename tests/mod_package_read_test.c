#include "p4pc/mods.h"
#include "p4pc/os.h"
#include "vfs.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

/* Emits the requested winning bytes, with metadata on stderr. One activation
 * serves every requested file: real package witnesses do not rescan per asset. */
int main(int argc, char **argv)
{
    if (argc < 4) return 2;
    p4_os_init();
#ifdef _WIN32
    if (_setmode(_fileno(stdout), _O_BINARY) == -1) return 2;
#endif
    if (p4_mods_stage(argv[1], argv[2]) || p4_package_registry_publish() || vfs_mount(NULL, NULL)) return 3;
    unsigned char bytes[65536];
    for (int i = 3; i < argc; i++) {
        int fd = vfs_open(argv[i], 0), directory;
        long long size;
        P4VfsOrigin origin;
        if (fd < 0 || vfs_origin(fd, &origin) || origin.kind != P4_VFS_MOD ||
            !origin.key || vfs_stat(argv[i], &directory, &size) || directory ||
            vfs_seek(fd, 0, SEEK_END) != size || vfs_seek(fd, 0, SEEK_SET) != 0) return 4;
        fprintf(stderr, "source=key:%s provider:%s rank:%" PRIu32 " size:%lld\n",
                origin.key, origin.provider, origin.rank, size);
        long long consumed = 0;
        int n;
        while ((n = vfs_read(fd, bytes, sizeof bytes)) > 0) {
            if (fwrite(bytes, 1, (size_t)n, stdout) != (size_t)n) return 5;
            consumed += n;
        }
        if (n < 0 || consumed != size || vfs_close(fd)) return 6;
    }
    return fflush(stdout) ? 7 : 0;
}
