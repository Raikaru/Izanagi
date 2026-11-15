/* Practice speed for real-time play (src/platform/kernel_thread.c), driven by the mod services'
 * p4.pace (src/platform/mods/game_services.c). Kept out of p4pc/platform.h so translated units
 * never see it. Rates are in quarters: 1 = 0.25x, 2 = 0.5x, 4 = 1x, 8 = 2x, 16 = 4x. */
#ifndef P4PC_PACE_H
#define P4PC_PACE_H

/* Takes the rate at the next VBLANK. -1 on a virtual clock (P4PC_VIRTUAL_CLOCK, P4PC_TURBO,
 * lockstep), whose time is the run's own. */
int p4pc_pace_request(unsigned quarters);
/* The rate in force; 4 when no pace was ever applied. */
unsigned p4pc_pace_effective(void);
/* Called at the VBLANK that changes whether the rate is other than 1x; p4pc_platform_init installs
 * p4snd_set_frame_driven, so paced audio advances per VBLANK period with its output muted. */
extern void (*p4pc_pace_audio_hook)(int frame_driven);

#endif
