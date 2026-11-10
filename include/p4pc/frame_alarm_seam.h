/* The lockstep disc replay seam (src/platform/lockstep_disc.c, docs/lockstep.md). Kept out of
 * p4pc/platform.h so translated units never see it. NULL unless P4PC_LOCKSTEP_DISC_REPLAY is
 * set and a battle qualification anchor was applied; then p4pc_set_next_frame_alarm,
 * p4pc_cancel_alarm and p4pc_poll_devices go through it. Disc reads are the only next-frame
 * alarm users. p4pc_set_frame_boundary_alarm is the unhooked next-frame scheduler. */
#ifndef P4PC_FRAME_ALARM_SEAM_H
#define P4PC_FRAME_ALARM_SEAM_H

#include <stdint.h>

typedef struct P4FrameAlarmSeam {
    int32_t (*set)(uint32_t (*handler)(int32_t, void *), void *arg);
    void (*cancel)(int32_t id);
    void (*poll)(void);
} P4FrameAlarmSeam;

extern const P4FrameAlarmSeam *p4pc_frame_alarm_seam;
int32_t p4pc_set_frame_boundary_alarm(uint32_t (*handler)(int32_t, void *), void *arg);

#endif
