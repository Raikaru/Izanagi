/* Port replacement for the PS2 start-up path (asm/code1.s `_start`). See src/crt.c. */
#ifndef P4PC_CRT_H
#define P4PC_CRT_H
#include <stdlib.h>

/* Runs the game from start-up to exit. Does not return: it exits the process. */
void p4pc_start(int argc, char **argv);
/* Native/host observer only. Installed before game entry; code0/normal return
 * invokes it, fatal/nonzero/_Exit paths do not. No optional-module link
 * dependency is imposed on standalone CRT fixtures. One process-lifetime owner. */
int p4pc_on_normal_exit(void (*callback)(void));
/* Runs that observer, if any. The game's exit glue and normal return call it. */
void p4pc_notify_normal_exit(void);
/* Deliberate process completion outside the exit glue: automation gate/harness
 * results, tape/session ends and restart requests. Code 0 is a normal exit and
 * notifies the observer first; nonzero codes are failures and do not. Fatal
 * paths keep plain exit/_Exit/abort. Inline so the CRT-boundary inventory keeps
 * attributing each exit to its actual caller. */
static inline _Noreturn void p4pc_exit(int code)
{
    if (code == 0) p4pc_notify_normal_exit();
    exit(code);
}

#endif /* P4PC_CRT_H */
