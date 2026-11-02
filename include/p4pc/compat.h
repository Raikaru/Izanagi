/* Force-included (-include) into every decomp translation unit.
 *
 * Maps MWCC/PS2 extensions onto clang -m32 so the unmodified decomp C compiles.
 * Nothing here changes data layout: the 128-bit types keep their 16-byte size. */
#ifndef P4PC_COMPAT_H
#define P4PC_COMPAT_H

/* Windows x64 decomp units: include/rw/std/stddef.h (`typedef unsigned int size_t`) shadows the system <stddef.h> on the include path and
 * clashes with mingw's 8-byte size_t, which mingw's own <stdint.h> reaches first. The shim is pre-empted, and what it provided besides the
 * typedefs (NULL, offsetof) comes from the compiler. (On Linux the shim is the first <stddef.h> and stays; see the 64-bit block below.) */
#if defined(P4_DECOMP_UNIT) && defined(_WIN32) && __SIZEOF_POINTER__ == 8
#define RW_SHIM_STDDEF_H
#ifndef NULL
#define NULL ((void *)0)
#endif
#define offsetof(t, m) __builtin_offsetof(t, m)
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include "p4pc/align.h"
#include "p4pc/guest.h"

/* RenderWare's global block is one array (retail `ourGlobals`, see docs/porting.md).
 * ps2/ostypes.h defines RWGLOBALSIZE, but baresour.h includes balibtyp.h
 * before batypes.h does, so some units saw the pointer form of
 * RwEngineInstance and referenced an undefined symbol. The same value here
 * makes every unit take the array form. ostypes.h's identical redefinition
 * is allowed by C. */
#define RWGLOBALSIZE (16*1024)

/* 128-bit MWCC types (`__int128`, `mode(TI)`) are rewritten to P4_Q128 by
 * tools/gen_port.py when it mirrors the decomp. Storage only: 16 bytes,
 * 16-aligned, like the PS2 quadword. No arithmetic on it is supported. */
typedef struct __attribute__((aligned(16))) P4Q128 {
    uint32_t w[4];
} P4_Q128;

/* MMI builtins (`_pcpyld`) on P4_Q128. */
#include "p4pc/mmi.h"

/* VU0 macro-mode bridge for the PS2 `__asm__` VU0/COP2 sites (PortVu0). */
#include "p4pc/vu0.h"

/* 64-bit host: the decomp's type.h typedefs size_t, intptr_t and uintptr_t as 32-bit integers, which clash with
 * the host's. No game struct holds one of them (docs/port64.md, inventory), so the decomp's own units use the
 * host types and type.h keeps only the fixed-width names: this block pre-empts its include guard. */
#if defined(P4_DECOMP_UNIT) && defined(P4_GUEST64)
#define TYPE_H
#define RW_SHIM_STDDEF_H   /* include/rw/std/stddef.h: `typedef unsigned int size_t`; the host's stddef.h is already in */
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
#endif

/* Decomp units only (CMake defines P4_DECOMP_UNIT): the libc names they call bind to retail
 * functions. Host and port code keep glibc. See libc_rename.h. */
#ifdef P4_DECOMP_UNIT
#include "p4pc/libc_rename.h"
#else
#include "p4pc/hostmath.h"
#endif

#endif /* P4PC_COMPAT_H */
