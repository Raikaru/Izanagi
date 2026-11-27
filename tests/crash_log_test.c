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
    if (argc != 4) return 2;
    setenv("P4PC_LOG_FILE", argv[1], 1);
    p4_os_init();
    /* The lifecycle crash marker: armed before the fault, filled only by the handler. */
    static const char marker[] = "P4PC-CRASH 1 session=7\ntest.mod\tR1:B3\n";
    if (p4_os_crash_marker_open(argv[3]) || p4_os_crash_marker_set("stale", 5) ||
        p4_os_crash_marker_set(marker, sizeof marker - 1)) return 5;
    if (p4_guest_init() != 0) return 3;
    /* The context line (build, area) of the player diagnostics: replaced as often as wanted, the last text is the one reported. */
    p4_os_crash_note_set("build=stale area=f0:r0");
    p4_os_crash_note_set("build=v9.9-test area=f12:r3 ev 176.1.0 t=42s flip=2520");
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
