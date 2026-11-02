/* Standalone production-handler witness. Never linked into the game. */
#include "p4pc/os.h"
#include "p4pc/guest.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 3) return 2;
    setenv("P4PC_LOG_FILE", argv[1], 1);
    p4_os_init();
    if (p4_guest_init() != 0) return 3;
    int sig = atoi(argv[2]);
    if (sig == SIGSEGV) {
        /* An actual inaccessible page, not a game fault switch or undefined null write. */
        /* The real guest-address note must identify this inaccessible guest word. */
        volatile char *page = (volatile char *)(p4_guest_base + 0xdeadbeefu);
        *page = 1;
    } else {
        raise(sig);
    }
    return 4;
}
