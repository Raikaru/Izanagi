#include <assert.h>
#include "movie_events.h"

int main(void)
{
    MovieEvents e = MOVIE_EVENTS_INIT;
    MovieExpected expected = {2, 5, 2, 4, 6, 3, 6};
    movie_events_step(&e, 1, 3, 0, 0, 0, 1);
    movie_events_step(&e, 3, 3, 0, 1, 0, 1);
    movie_events_step(&e, 4, 3, 1, 1, 0, 1);
    movie_events_step(&e, 5, 4, 2, 1, 0, 1);
    movie_events_step(&e, 6, 4, 3, 1, 0, 1);
    movie_events_step(&e, 7, 5, 4, 1, 1, 1);
    movie_events_step(&e, 8, 5, 5, 1, 2, 1);
    movie_events_step(&e, 9, 6, 6, 1, 3, 1);
    assert(movie_events_compare(&e, 6, &expected) == 0);
    e.t_first = 2; assert(movie_events_compare(&e, 6, &expected) == MOVIE_BAD_PREP);
    e.t_first = 6; assert(movie_events_compare(&e, 6, &expected) == 0);
    e.t_first = 7; assert(movie_events_compare(&e, 6, &expected) == MOVIE_BAD_PREP);
    e.t_first = 3;
    expected.s4_frame++; assert(movie_events_compare(&e, 6, &expected) == MOVIE_BAD_S4); expected.s4_frame--;
    expected.s5_frame++; assert(movie_events_compare(&e, 6, &expected) == MOVIE_BAD_S5); expected.s5_frame--;
    expected.s6_frame++; assert(movie_events_compare(&e, 6, &expected) == MOVIE_BAD_S6); expected.s6_frame--;
    expected.s6_fade_out++; assert(movie_events_compare(&e, 6, &expected) == MOVIE_BAD_S6); expected.s6_fade_out--;
    assert(movie_events_compare(&e, 5, &expected) == MOVIE_BAD_LAST);
    movie_events_step(&e, 26, 5, 8, 1, 2, 1);
    assert(movie_events_compare(&e, 6, &expected) == MOVIE_BAD_STEP);
    return 0;
}
