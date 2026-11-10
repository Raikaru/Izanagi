/* State-registered real-play captures. P4PC_CAPTURE_STATE selects a procedure timer,
 * field/player/camera, inner event, rank-up draw phase or ready party COMMAND menu;
 * flips are sanity bounds only. */
#ifndef P4PC_CAPTURE_STATE_H
#define P4PC_CAPTURE_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

void p4pc_capture_state_flip(unsigned frame);
int p4pc_capture_state_wanted(unsigned frame);
int p4pc_capture_state_draw(void);
/* -1: ordinary fixed-flip gate; 0: state capture pending; 1: captured on the previous flip. */
int p4pc_capture_state_gate(unsigned frame);

#ifdef __cplusplus
}
#endif
#endif
