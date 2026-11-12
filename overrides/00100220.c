/* PS2 syscall 0x23 (`addiu $3, $0, 0x23; syscall 0`). The decomp has no caller.
 * Its kernel service has no port equivalent, so it traps by name rather than
 * returning a made-up result. */
#include "p4pc/tables.h"

void kwlnExitThread(void) {
    p4pc_trap("kwlnExitThread (syscall 0x23)", 0x00100220u, __builtin_return_address(0));
}
