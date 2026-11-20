/* Shared event tracking and comparison for movie-task tests. */
#ifndef P4PC_TESTS_MOVIE_EVENTS_H
#define P4PC_TESTS_MOVIE_EVENTS_H

typedef struct MovieEvents {
    int t_s3, t_first, s4_frame, s5_frame, s6_frame, s6_fade_out;
    int prev_step_frame, non_unit_steps;
} MovieEvents;

typedef struct MovieExpected {
    int prep_min, prep_max, s4_frame, s5_frame, s6_frame, s6_fade_out, last_frame;
} MovieExpected;
#define MOVIE_EVENTS_INIT {-1, -1, -1, -1, -1, -1, -1, 0}

enum {
    MOVIE_BAD_PREP = 1, MOVIE_BAD_S4 = 2, MOVIE_BAD_S5 = 4,
    MOVIE_BAD_S6 = 8, MOVIE_BAD_LAST = 16, MOVIE_BAD_STEP = 32
};

static void movie_events_step(MovieEvents *e, int ticks, int state, int frame,
                              int fade_in, int fade_out, int playing)
{
    if (state >= 3 && e->t_s3 < 0) e->t_s3 = ticks;
    if (fade_in >= 1 && e->t_first < 0) e->t_first = ticks;
    if (state == 4 && e->s4_frame < 0) e->s4_frame = frame;
    if (state == 5 && e->s5_frame < 0) e->s5_frame = frame;
    if (state == 6 && e->s6_frame < 0) { e->s6_frame = frame; e->s6_fade_out = fade_out; }
    if (playing && state >= 3 && state <= 5) {
        if (e->prev_step_frame >= 0 && frame != e->prev_step_frame && frame != e->prev_step_frame + 1)
            e->non_unit_steps++;
        e->prev_step_frame = frame;
    }
}

static unsigned movie_events_compare(const MovieEvents *e, int last_frame, const MovieExpected *r)
{
    unsigned bad = 0;
    const int prep = e->t_first - e->t_s3;
    if (prep < r->prep_min || prep > r->prep_max) bad |= MOVIE_BAD_PREP;
    if (e->s4_frame != r->s4_frame) bad |= MOVIE_BAD_S4;
    if (e->s5_frame != r->s5_frame) bad |= MOVIE_BAD_S5;
    if (e->s6_frame != r->s6_frame || e->s6_fade_out != r->s6_fade_out) bad |= MOVIE_BAD_S6;
    if (last_frame != r->last_frame) bad |= MOVIE_BAD_LAST;
    if (e->non_unit_steps) bad |= MOVIE_BAD_STEP;
    return bad;
}
#endif
