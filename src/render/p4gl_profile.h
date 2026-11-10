#ifndef P4GL_PROFILE_H
#define P4GL_PROFILE_H
#include <stdint.h>

enum P4GlProfilePass {
    P4GL_PROFILE_OTHER,
    P4GL_PROFILE_SCENE,
    P4GL_PROFILE_FEEDBACK,
    P4GL_PROFILE_UI,
    P4GL_PROFILE_PRESENT,
    P4GL_PROFILE_MOVIE
};

/* Null unless P4PC_GPU_TRACE selected a supported native timestamp-query API.
 * Pass markers never query the driver when tracing is disabled. */
extern void (*p4gl_profile_pass)(unsigned pass);
void p4gl_profile_init(int is_es);
void p4gl_profile_close(void);
unsigned p4gl_profile_feedback_begin(void);
void p4gl_profile_feedback_end(unsigned previous);
void p4gl_profile_touch(void);
void p4gl_profile_frame_end(unsigned frame);
uint64_t p4gl_profile_now_ns(void);
#endif
