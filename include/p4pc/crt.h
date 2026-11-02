/* Port replacement for the PS2 start-up path (asm/code1.s `_start`). See src/crt.c. */
#ifndef P4PC_CRT_H
#define P4PC_CRT_H

/* Runs the game from start-up to exit. Does not return: it exits the process. */
void p4pc_start(int argc, char **argv);
/* Native/host observer only. Installed before game entry; code0/normal return
 * invokes it, fatal/nonzero/_Exit paths do not. No optional-module link
 * dependency is imposed on standalone CRT fixtures. One process-lifetime owner. */
int p4pc_on_normal_exit(void (*callback)(void));

#endif /* P4PC_CRT_H */
