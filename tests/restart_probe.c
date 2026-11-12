#include "p4pc/os.h"
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    const char *value = getenv("P4PC_RESTART_ENV_TEST");
    if (argc != 4 || strcmp(argv[1], "with spaces") || strcmp(argv[2], "quote\"inside") ||
        strcmp(argv[3], "trailing\\") || !value || strcmp(value, "preserved environment")) return 2;
    if (p4_os_restart_init(argc, argv) != 0) return 3;
    return 4; /* P4PC_TEST_RESTART must finish in the second, fresh image. */
}
