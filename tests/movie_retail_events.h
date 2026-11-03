/* What the game's movie task observes of the player on retail, per movie, measured in PCSX2 with
 * the PCSX2 trace tooling's movie trace (not in the public repository) and reduced by its movie_events.py.
 * movie_task_smoke compares the port's run with these rows. Columns: movie table index, start-up ticks seen
 * before state 3 (informational), updates from state 3 to the first frame (retail varies with the disc:
 * 20.2 and 18.7 measured; the port uses 20 and the test accepts 18..21), frame number when the task enters
 * state 4 (fade-in done), when the fade-out starts (state 5), when it ends (state 6), the fade-out counter
 * at the end, and the last frame number. Retail frame numbers step by +1 per update; the 6 or 7 other steps
 * in each trace are PCSX2 emulation hiccups, the port must have none. */
#ifndef P4PC_TESTS_MOVIE_RETAIL_EVENTS_H
#define P4PC_TESTS_MOVIE_RETAIL_EVENTS_H

typedef struct MovieRetailRow {
    int index, startup;
    double prep;
    int s4_frame, s5_frame, s6_frame, s6_fade_out, last_frame;
} MovieRetailRow;

static const MovieRetailRow kMovieRetail[] = {
    /* sfd/P4CTOP1.sfd, the opening (2520 frames): fade-out at 2500 for 20 frames */
    {21, 10, 20.2, 19, 2500, 2519, 20, 2519},
    /* sfd/P4CT001.sfd (1800 frames): fade-in 60, fade-out at 1770 for 30 frames */
    {1, 10, 18.7, 59, 1770, 1799, 30, 1799},
};

#endif
