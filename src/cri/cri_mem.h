/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* Helpers for the translated retail CRI code (ahx.c, aixp.c): the code keeps the retail data layout, so
 * objects and tables are read by address and offset, and stream joints are called through their vtable.
 *
 * Guest-memory model (docs/port64.md): M8/M16/M32 take a guest address (a retail data address or a u32
 * read from guest memory); P8..S32 take a host pointer to a guest object and read at an offset; a
 * pointer stored in guest memory is a 4-byte guest address, so a pointer read from an object is
 * PP(p, o) (G2H of the stored word) and one stored is SETPP(p, o, ptr) (H2G); the stream joint's
 * vtable holds guest code addresses. All of it is identity on i386. */
#ifndef P4_CRI_MEM_H
#define P4_CRI_MEM_H

#include <stdint.h>

#include "p4pc/gmem.h"

#define M8(a) P4_G8(a)
#define M16(a) P4_G16(a)
#define M32(a) P4_G32(a)
#define P8(p, o) (*(uint8_t *)((uint8_t *)(p) + (o)))
#define S8(p, o) (*(int8_t *)((uint8_t *)(p) + (o)))
#define P16(p, o) (*(uint16_t *)((uint8_t *)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(p) + (o)))
#define P32(p, o) (*(uint32_t *)((uint8_t *)(p) + (o)))
#define S32(p, o) (*(int32_t *)((uint8_t *)(p) + (o)))
/* A pointer field of a guest object: read as a host pointer, store from a host pointer. */
#define PP(p, o) P4_G2H(P32(p, o))
#define SETPP(p, o, ptr) (P32(p, o) = P4_H2G(ptr))

/* The stream joint interface (SJ_IF): +0x14 Reset, +0x18 GetChunk, +0x1c UngetChunk, +0x20 PutChunk,
 * +0x24 GetNumData; the chunk is a data pointer and a length. */
typedef struct {
    uint32_t data; /* guest address */
    int32_t len;
} Ck;

/* The joint's vtable: guest code addresses. */
static inline uint32_t *sj_vt(void *sj)
{
    return (uint32_t *)P4_G2H(*(uint32_t *)sj);
}

static inline int32_t sj_num(void *sj, int32_t line)
{
    return ((int32_t(*)(void *, int32_t))p4_fn_g2h_sig(sj_vt(sj)[9], P4_SIG(2, 0, 1, 0, 0, 0, 0, 0)))(sj, line);
}

static inline void sj_get(void *sj, int32_t line, int32_t n, Ck *ck)
{
    ((void (*)(void *, int32_t, int32_t, Ck *))p4_fn_g2h_sig(sj_vt(sj)[6], P4_SIG(4, 0, 1, 0, 0, 1, 0, 0)))(sj, line, n, ck);
}

static inline void sj_unget(void *sj, int32_t line, Ck *ck)
{
    ((void (*)(void *, int32_t, Ck *))p4_fn_g2h_sig(sj_vt(sj)[7], P4_SIG(3, 0, 1, 0, 1, 0, 0, 0)))(sj, line, ck);
}

static inline void sj_put(void *sj, int32_t line, Ck *ck)
{
    ((void (*)(void *, int32_t, Ck *))p4_fn_g2h_sig(sj_vt(sj)[8], P4_SIG(3, 0, 1, 0, 1, 0, 0, 0)))(sj, line, ck);
}

static inline void sj_reset(void *sj)
{
    ((void (*)(void *))p4_fn_g2h_sig(sj_vt(sj)[5], P4_SIG(1, 0, 1, 0, 0, 0, 0, 0)))(sj);
}

#endif
