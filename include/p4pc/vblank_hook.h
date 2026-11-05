/* The kernel's per-VBLANK-period sink (src/platform/kernel_thread.c). Kept out of p4pc/platform.h
 * so translated units never see it. On a virtual clock (p4_os_virtual_clock) the kernel calls it
 * once for every VBLANK period that passes, before the VBLANK chain, interrupt enabled or not.
 * p4pc_platform_init installs p4snd_virtual_tick: the SPU2 plays in real time, so a game update
 * spanning two vsyncs hears two periods. NULL: no sink. */
#ifndef P4PC_VBLANK_HOOK_H
#define P4PC_VBLANK_HOOK_H

extern void (*p4pc_vblank_period_hook)(void);

#endif
