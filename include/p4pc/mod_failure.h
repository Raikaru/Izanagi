#ifndef P4PC_MOD_FAILURE_H
#define P4PC_MOD_FAILURE_H
#include <stddef.h>
/* Native startup installs the real presentation/recovery owner before any mod
 * code runs. A runtime failure cannot safely invent a game return value. */
typedef void (*P4ModFailureFn)(size_t owner, const char *reason, void *user);
int p4_mod_failure_handler(P4ModFailureFn callback, void *user);
_Noreturn void p4_mod_failure(size_t owner, const char *reason);
#endif
