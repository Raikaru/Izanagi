#include "file_index.h"
#include "p4pc/os.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static int list_entry(const char *relative, int directory, void *user)
{
    printf("listed=%s|%d|%s\n", (const char *)user, directory, relative);
    return 0;
}

int main(int argc, char **argv)
{
    p4_os_init();
    if (argc < 4) return 2;
    P4PackageFiles *files = p4_package_files_create(argv[1], "fixture", (unsigned)atoi(argv[2]), (size_t)strtoull(argv[3], NULL, 10));
    printf("index=%d\n", files ? 0 : -1);
    if (!files) return 0;
    size_t memory = p4_package_files_memory(files);
    printf("count=%zu\nmemory=%zu\n", p4_package_files_count(files), memory);
    for (size_t i = 0; i < p4_package_files_count(files); i++) {
        const char *relative; int directory;
        if (p4_package_files_entry(files, i, &relative, &directory)) return 1;
        printf("entry=%d|%s\n", directory, relative);
    }
    for (int i = 4; i < argc; i++) {
        if (argv[i][0] == '@') {
            int result = p4_package_files_list(argv[i] + 1, list_entry, argv[i] + 1, files);
            if (p4_package_files_memory(files) != memory) return 1;
            printf("list_status=%s|%d\n", argv[i] + 1, result);
            continue;
        }
        const char *first = p4_package_files_resolve(argv[i], files);
        const char *second = p4_package_files_resolve(argv[i], files);
        if (first != second || p4_package_files_memory(files) != memory) return 1;
        printf("resolve=%s|%s\n", argv[i], first ? first : "<missing>");
    }
    const char *unused; int directory;
    if (p4_package_files_entry(files, p4_package_files_count(files), &unused, &directory) != -1) return 1;
    p4_package_files_destroy(files);
    return 0;
}
