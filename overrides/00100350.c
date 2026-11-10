/* PS2 IOP reset, module loading and RPC binding. The PC port has no IOP, and the
 * retail retry loops (func_0042b098, sceSifSyncIop) wait for replies that never
 * come. The body is replaced by the platform's EE-side boot,
 * src/platform/kernel_init.c. */
#include "p4pc/platform.h"

void kwlnInitIop(void) {
    p4pc_platform_init();
}
