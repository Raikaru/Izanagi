/* Short names emitted by the mirror rewriter, without the decomp's host-header
 * conflicts. The same declarations serve both standalone override targets. */
#ifndef P4PC_OVERRIDE_TEST_TYPES_H
#define P4PC_OVERRIDE_TEST_TYPES_H

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned int u32;
typedef int s32;
typedef float f32;

/* Guest-width memcpy declaration used when libc_rename.h maps the override's
 * memcpy call to the retail entry point (p4pc/ps2_libc.h). */
void *func_0043f810(void *destination, const void *source, u32 count);

#endif
