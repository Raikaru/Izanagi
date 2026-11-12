/* Declarations the palquant overrides expect from their mirror translation unit
 * (build/gen/decomp/src/rw/palquant.c). Force-included into each palquant test TU. */
#ifdef P4_GUEST64
/* decomp type.h clashes with the system headers on a 64-bit host (uintptr_t); the decomp's own units get these names from p4pc/compat.h */
typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef signed long long s64;
typedef float f32;
typedef double f64;
#else
#include "type.h"
#endif

extern u8 *(*D_008873F8[])(u8 *arg0, s32 arg1);
extern u8 *func_003e1210(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
