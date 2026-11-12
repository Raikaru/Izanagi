#include "p4pc/mod_failure.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/mod_events.h"
#include <stdio.h>
#include <stdlib.h>

static P4ModFailureFn handler;
static void *handler_user;

int p4_mod_failure_handler(P4ModFailureFn callback, void *user)
{
    if (!p4_mod_hooks_quiescent() || !p4_mod_events_quiescent()) return -1;
    handler = callback;
    handler_user = user;
    return 0;
}
_Noreturn void p4_mod_failure(size_t owner, const char *reason)
{
    if (handler) handler(owner, reason, handler_user);
    /* Not a gameplay fallback: an absent/returning host recovery owner is a
     * native embedding error. The game installs its SDL/INI/restart owner. */
    fprintf(stderr, "p4pc: native mod recovery owner absent or returned (owner=%zu): %s\n",
            owner, reason ? reason : "unknown failure");
    fflush(stderr);
    _Exit(EXIT_FAILURE);
}
