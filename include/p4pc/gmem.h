/* Guest-memory accessors for port-owned code (docs/port64.md 2.9). Additive to p4pc/guest.h.
 *
 * Port code reaches guest memory two ways: through a host pointer it was handed (a parameter or a
 * value converted with P4_G2H), or through an absolute guest address (retail data at its fixed
 * address). Everything stored in guest memory is a guest address (u32), never a host pointer; every
 * value read from guest memory that stands for a pointer goes through P4_G2H / P4_FN_G2H before it
 * is used, and every pointer stored goes through P4_H2G / P4_FN_H2G. All of it is identity on i386.
 *
 *   P4_G8/P4_G16/P4_G32(a)         the byte/half/word at guest address a (an lvalue)
 *   P4_GF32(a)                     the float at guest address a (an lvalue)
 *   P4_GPTR(a)                     the host pointer for guest address a, typed (T *)
 *   p4_gport_alloc(size, align)    zeroed port-owned guest storage that is never freed: objects the game
 *                                  addresses (stream joints, handle tables) live here, not in host statics
 */
#ifndef P4PC_GMEM_H
#define P4PC_GMEM_H

#include <stdint.h>
#include <string.h>

#include "p4pc/guest.h"

#define P4_G8(a) (*(volatile uint8_t *)P4_G2H(a))
#define P4_G16(a) (*(volatile uint16_t *)P4_G2H(a))
#define P4_G32(a) (*(volatile uint32_t *)P4_G2H(a))
#define P4_GF32(a) (*(volatile float *)P4_G2H(a))
#define P4_GPTR(T, a) ((T *)P4_G2H(a))

/* Retail-addressed data, the way the gen_port rewrite spells it (an `extern T D_00887300;` in port code
 * is a linker symbol on i386 only): P4_GDATA(T, addr) is the object of type T at guest address addr,
 * P4_GARRAY(T, addr) the array there, P4_GPDATA(T, off) the gp-relative one (iGpffffb1dc is
 * P4_GPDATA(int32_t, 0xb1dc): gp 0x007690F0 plus the sign-extended 16-bit offset). */
#define P4_GDATA(T, a) (*(T *)P4_G2H(a))
#define P4_GARRAY(T, a) ((T *)P4_G2H(a))
#define P4_GP_VALUE 0x007690F0u
#define P4_GPADDR(off) ((uint32_t)(P4_GP_VALUE + (uint32_t)(int32_t)(int16_t)(off)))
#define P4_GPDATA(T, off) P4_GDATA(T, P4_GPADDR(off))

/* Port-owned guest storage, 16-byte aligned or better, zeroed, never freed. Returns the guest
 * address, or 0 when the port half is exhausted. (src/platform/gport.c) */
uint32_t p4_gport_alloc(uint32_t size, uint32_t align);

/* The guest address of a copy of the string s in port-owned guest memory (interned by the address of s: pass
 * string literals or other storage that outlives the call). For a string the game reads through a pointer
 * it was handed (an error message word, a name); 0 when s is NULL or the port half is exhausted. */
uint32_t p4_gport_str(const char *s);

#endif /* P4PC_GMEM_H */
