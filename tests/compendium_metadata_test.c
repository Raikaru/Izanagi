#include "compendium.h"
#include "p4pc/os.h"
#include <stdio.h>

int main(int argc, char **argv)
{
    p4_os_init();
    if (argc != 3) return 2;
    P4PackageInfo info;
    char files[1200];
    int result = p4_compendium_manifest(argv[1], argv[2], &info, files, sizeof files);
    printf("metadata=%d\n", result);
    if (result == 0) {
        printf("id=%s\nname=%s\nversion=%s\nfiles=%s\n", info.id, info.name, info.version, files);
        printf("safe=%d\n", p4_compendium_root_safe(argv[1], NULL));
    }
    return 0;
}
